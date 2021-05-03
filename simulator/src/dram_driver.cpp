#include <dram_driver.hpp>
using namespace std;

bool Request::is_LW() { return dst != nullptr && dst_reg != -1 && !is_NULL(); }

bool Request::is_SW() { return !is_LW() && !is_NULL(); }

bool Request::is_NULL() { return timestamp == -1 && core == -1; }

void Request::nullify() {
  timestamp = -1;
  core = -1;
}

DramDriver::DramDriver(Dram dram, int cores, int* reg_updates)
    : dram(dram), cores(cores), reg_updates(reg_updates), curr_queue(-1) {
  __init_LUT();
}

void DramDriver::__init_LUT() {
  for (int i = 0; i < cores; i++) {
    for (int j = 0; j < REG_COUNT; j++) {
      __core_reg2offsets_LUT[i][j] = make_pair(-1, -1);
    }
  }

  memset(__queue2offset_LUT, 0, sizeof(__queue2offset_LUT));
  memset(__queue2row_LUT, -1, sizeof(__queue2row_LUT));

  memset(__core2freq_LUT, 0, sizeof(__core2freq_LUT));
  memset(__core2instr_LUT, 0, sizeof(__core2instr_LUT));

  int total_mem = Dram::MAX_MEMORY;
  int partition_size = total_mem / cores;

  __core2PA_offsets_LUT[0] = 0;
  for (int i = 1; i < cores; i++) {
    __core2PA_offsets_LUT[i] = __core2PA_offsets_LUT[i - 1] + partition_size;
  }

  memset(__core2blocked_reg_LUT, -1, sizeof(__core2blocked_reg_LUT));
}

void DramDriver::addr_V2P(int& addr, int core) {
  if (addr >= Dram::MAX_MEMORY / cores) {
    throw InvalidMemory("received request with memory out of bounds : " +
                        to_string(addr));
  }
  addr += __core2PA_offsets_LUT[core];
}

void DramDriver::issue_write(int core, int addr, hd_t val, Stats& stats) {
  addr_V2P(addr, core);

  auto req = Request{addr, val, stats.clock_cycles, core};
  enqueue_request(req, stats);
  __core2freq_LUT[core]++;
}

void DramDriver::issue_read(int core, int addr, hd_t* dst, Stats& stats,
                            int dst_reg) {
  addr_V2P(addr, core);

  auto req = Request{addr, -1, stats.clock_cycles, core, dst, dst_reg};
  enqueue_request(req, stats);
  __core2freq_LUT[core]++;
}

// Executes request in an order determined by the blocked registers
// The method assumes that it is called at every iteration of clock cycles.
void DramDriver::perform_tasks(Stats& stats) {
  auto curr_req = get_curr_request();
  if (curr_req == nullptr) {
    return;
  }
  if (dram.busy_until == stats.clock_cycles) {
    complete_request(stats);

    if (queues[curr_queue].empty() || round_counter == QUEUE_SIZE) {
      choose_next_queue();  // changes curr_queue
    }
  } else if (dram.busy_until + 1 <= stats.clock_cycles) {
    if (curr_queue == -1) {
      return;
    } else if (curr_req->is_NULL()) {
      // NOTE: processing a null request takes 1 clock cycle time
      complete_request(stats);
    } else {
      dram.issue_request(curr_req->addr, stats);
    }
  }
}

bool DramDriver::is_blocking_reg(int core, int reg) {
  auto req_ptr = lookup_request(core, reg);
  return req_ptr != nullptr && !req_ptr->is_NULL();
}

void DramDriver::enqueue_request(Request& request, Stats& stats) {
  auto row = dram.addr2rowcol(request.addr).first;

  int unallocated_q = -1;
  for (int i = 0; i < NUM_QUEUES; i++) {
    if (__queue2row_LUT[i] == row) {
      insert_request(request, i, stats);
      return;
    } else if (__queue2row_LUT[i] == -1 && unallocated_q == -1) {
      unallocated_q = i;
    }
  }

  if (unallocated_q == -1) {
    throw QueueFull();
  } else {
    __queue2row_LUT[unallocated_q] = row;
    insert_request(request, unallocated_q, stats);
  }
}

void DramDriver::insert_request(Request& request, int q_num, Stats& stats) {
  if (request.is_NULL()) {
    return;
  }
  if (queues[q_num].size() == QUEUE_SIZE) {
    throw QueueFull();
  } else {
    if (request.is_LW()) {
      auto core = request.core;
      auto reg = request.dst_reg;

      // Eliminate redundant LW
      auto redundant_lw_ptr = lookup_LW(core, reg);
      if (redundant_lw_ptr != nullptr) {
        redundant_lw_ptr->nullify();
        __core_reg2offsets_LUT[core][reg] = make_pair(-1, -1);
      }

      // Forwarding
      auto existing_sw = lookup_SW(q_num, request.addr);

      if (existing_sw != nullptr) {
        *request.dst = existing_sw->val;
        stats.logs.push_back(Log{});
        auto& log = stats.logs.back();

        log.cycle_period =
            make_pair(stats.clock_cycles + 1, stats.clock_cycles + 1);
        log.remarks.push_back("Forwarding values from DRAM driver");

        reg_updates[core] = stats.clock_cycles + 1;
        return;
      }

      // Update LUT
      __core_reg2offsets_LUT[core][reg] =
          make_pair(q_num, queues[q_num].size() + __queue2offset_LUT[q_num]);
    } else {
      // check for redundant SW
      auto existing_sw = lookup_SW(q_num, request.addr);
      if (existing_sw != nullptr) {
        existing_sw->nullify();
      }
    }
    queues[q_num].push_back(request);
  }
  if (curr_queue == -1) {
    curr_queue = q_num;
  }
}

void DramDriver::complete_request(Stats& stats) {
  auto& req = queues[curr_queue].front();
  auto core = req.core;
  if (req.is_SW()) {
    dram.set_mem_word(req.addr, req.val, stats);
  } else if (req.is_LW()) {
    auto val = dram.get_mem_word(req.addr, req.dst_reg, stats);
    *req.dst = val;

    // Set the latest time for register update
    reg_updates[core] = stats.clock_cycles;

    // Reset the __core_reg2offsets_LUT for the LW request
    auto [q_num, q_offset] = __core_reg2offsets_LUT[req.core][req.dst_reg];
    if (q_num != -1 && q_offset - __queue2offset_LUT[q_num] == 0) {
      __core_reg2offsets_LUT[req.core][req.dst_reg] = make_pair(-1, -1);
    }
  }

  queues[curr_queue].pop_front();
  __queue2offset_LUT[curr_queue]++;

  if (queues[curr_queue].empty()) {
    __queue2row_LUT[curr_queue] = -1;
    __queue2offset_LUT[curr_queue] = 0;
  }
  if (!req.is_NULL()) {
    round_counter++;
  }
}

void DramDriver::set_blocking_regs(int core, vector<int>& regs) {
  assert(int(regs.size()) <= MAX_BLOCKED_REG);

  memset(__core2blocked_reg_LUT[core], -1,
         sizeof(__core2blocked_reg_LUT[core]));

  int i = 0;
  for (auto& r : regs) {
    __core2blocked_reg_LUT[core][i] = r;
    i++;
  }
}

Request* DramDriver::lookup_SW(int q_num, int addr) {
  int i = 0;
  for (auto& req : queues[q_num]) {
    if (req.is_SW() && req.addr == addr && !(q_num == curr_queue && i == 0)) {
      return &req;
    }
    i++;
  }
  return nullptr;
}

Request* DramDriver::lookup_request(int core, int reg) {
  auto [q_num, q_offset] = __core_reg2offsets_LUT[core][reg];
  if (q_num == -1) {
    return nullptr;
  }
  return &queues[q_num][q_offset - __queue2offset_LUT[q_num]];
}

Request* DramDriver::lookup_LW(int core, int reg) {
  auto [q_num, q_offset] = __core_reg2offsets_LUT[core][reg];
  if (q_num == -1) {
    return nullptr;
  }
  if (q_num == curr_queue && q_offset - __queue2offset_LUT[q_num] == 0) {
    return nullptr;
  }
  return &queues[q_num][q_offset - __queue2offset_LUT[q_num]];
}

Request* DramDriver::get_curr_request() {
  if (curr_queue == -1 || queues[curr_queue].empty()) {
    return nullptr;
  }
  return &queues[curr_queue].front();
}
void DramDriver::choose_next_queue() {
  int q_offsets[NUM_QUEUES];  // metric denoting the importance of the blocked
                              // requests in a queue
  int core_req_freqs[MAX_CORES];  // ratio of Request frequency to total number
                                  // of instructions

  memset(q_offsets, 0, sizeof(q_offsets));
  memset(core_req_freqs, 0, sizeof(core_req_freqs));

  for (int core = 0; core < MAX_CORES; core++) {
    auto& blocked_regs = __core2blocked_reg_LUT[core];

    for (int j = 0; j < MAX_BLOCKED_REG; j++) {
      int reg = blocked_regs[j];
      if (reg == -1) continue;  // register not blocked
      Request* rq = lookup_LW(core, reg);
      auto [q_num, q_offset] = __core_reg2offsets_LUT[core][reg];
      if (q_num != -1) {  // a blocking request of that register exists
        q_offsets[q_num] +=
            (QUEUE_SIZE - q_offset);  //  offset => importance of queue
      }
    }
    // add 1 to prevent div by 0
    core_req_freqs[core] = __core2freq_LUT[core] / (1 + __core2instr_LUT[core]);
  }

  int best_q = 0;
  double freq_offset = 0.01;  // to prevent division by 0

  for (int i = 1; i < NUM_QUEUES; i++) {
    // offset +  , freq - , size +
    double prev_metric = q_offsets[best_q] /
                         (freq_offset + core_req_freqs[best_q]) *
                         queues[best_q].size();
    double new_metric =
        q_offsets[i] / (freq_offset + core_req_freqs[i]) * queues[i].size();
    if (new_metric > prev_metric) {
      best_q = i;
    }
    // cout << new_metric << endl;
  }
  cout << endl;

  curr_queue = best_q;
  round_counter = 0;
}

void DramDriver::update_instr_count(int core) { __core2instr_LUT[core]++; }

bool DramDriver::is_idle() { return get_curr_request() == nullptr; }

DramDriver::~DramDriver() {
  for (auto& q : queues) {
    q.clear();
  }
}