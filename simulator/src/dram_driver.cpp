#include <dram_driver.hpp>
#include <sstream>

using namespace std;

bool Request::is_LW() { return dst != nullptr && dst_reg != -1 && !is_NULL(); }

bool Request::is_SW() { return !is_LW() && !is_NULL(); }

bool Request::is_NULL() { return timestamp == -1 && core == -1; }

void Request::nullify() {
  timestamp = -1;
  core = -1;
}

string Request::to_string() {
  ostringstream ss;
  if (core == -1)
    ss << "NULL";
  else
    ss << "(c:" << core << ",a:" << addr << ",LW:" << is_LW() << ")";
  return ss.str();
}

string DramDriver::DRIVER_ID = "DRAM DRIVER";
DramDriver::DramDriver(Dram dram, int cores, int* reg_updates)
    : dram(dram), cores(cores), reg_updates(reg_updates), curr_queue(-1) {
  __init_LUT();

  for (int i = 0; i < NUM_QUEUES; i++) {
    for (int j = 0; j < QUEUE_SIZE; j++) {
      queues[i][j].nullify();
    }
  }
}

string DramDriver::queueToStr() {
  int i = 0;
  ostringstream ss;
  for (auto& row : queues) {
    ss << "queue " << i++ << "  --   ";
    for (auto& req : row) {
      ss << setw(25) << left << req.to_string();
    }
    ss << "\n";
  }
  ss << "\tCurrent queue: " << curr_queue << endl;
  ss << "\tCurrent index: " << curr_index << endl;
  ss << "\n\n";
  return ss.str();
}

void DramDriver::load_stats(Stats* stats) { this->stats = stats; }

void DramDriver::add_delay(int delay, string remark) {
  auto start_time = max(this->busy_until, stats->clock_cycles) + 1;
  this->busy_until = start_time - 1 + delay;
  if (delay == 0) {
    return;
  }
  auto log = Log{};
  log.cycle_period = make_pair(start_time, this->busy_until);
  log.device = DRIVER_ID;
  log.remarks.push_back(remark);
  log.queue_details = queueToStr();

  stats->logs.push_back(log);
}

int getNextPow2(int n) { return 1 << int(ceil(log2(n))); }

void DramDriver::__init_LUT() {
  for (int i = 0; i < cores; i++) {
    for (int j = 0; j < REG_COUNT; j++) {
      __core_reg2offsets_LUT[i][j] = make_pair(-1, -1);
    }
  }

  memset(__queue2row_LUT, -1, sizeof(__queue2row_LUT));

  memset(__core2freq_LUT, 0, sizeof(__core2freq_LUT));
  memset(__core2instr_LUT, 0, sizeof(__core2instr_LUT));
  memset(__queue2size_LUT, 0, sizeof(__queue2size_LUT));

  int total_mem = Dram::MAX_MEMORY;
  int partition_size = total_mem / getNextPow2(cores);

  __core2PA_offsets_LUT[0] = 0;
  for (int i = 1; i < cores; i++) {
    __core2PA_offsets_LUT[i] = __core2PA_offsets_LUT[i - 1] + partition_size;
  }

  memset(__core2blocked_reg_LUT, -1, sizeof(__core2blocked_reg_LUT));
}

void DramDriver::addr_V2P(int& addr, int core) {
  if (addr >= Dram::MAX_MEMORY / getNextPow2(cores)) {
    throw InvalidMemory("received request with memory out of bounds : " +
                        to_string(addr));
  }
  addr += __core2PA_offsets_LUT[core];
}

void DramDriver::issue_write(int core, int addr, hd_t val) {
  addr_V2P(addr, core);
  if (stats->clock_cycles <= this->busy_until) {
    throw ControllerBusy();
  }

  auto req = Request{addr, val, stats->clock_cycles, core};
  enqueue_request(req);
  __core2freq_LUT[core]++;
}

void DramDriver::issue_read(int core, int addr, hd_t* dst, int dst_reg) {
  addr_V2P(addr, core);
  if (stats->clock_cycles <= this->busy_until) {
    throw ControllerBusy();
  }

  auto req = Request{addr, -1, stats->clock_cycles, core, dst, dst_reg};
  enqueue_request(req);
  __core2freq_LUT[core]++;
}

bool DramDriver::is_blocking_reg(int core, int reg) {
  auto req_ptr = lookup_request(core, reg);
  return req_ptr != nullptr && !req_ptr->is_NULL();
}

int DramDriver::get_empty_slot(int q_num) {
  for (int i = 0; i < QUEUE_SIZE; i++) {
    if (queues[q_num][i].is_NULL()) {
      return i;
    }
  }
  throw QueueFull();
}

bool DramDriver::is_empty_queue(int q_num) {
  return q_num == -1 || __queue2size_LUT[q_num] == 0;
}

// Executes request in an order determined by the blocked registers
// The method assumes that it is called at every iteration of clock cycles.
void DramDriver::perform_tasks() {
  auto curr_req = get_curr_request();
  if (curr_req == nullptr || stats->clock_cycles < this->busy_until) {
    return;
  }
  if (dram.busy_until == stats->clock_cycles) {
    complete_request();

    if (is_empty_queue(curr_queue) || round_counter == QUEUE_SIZE) {
      choose_next_queue();  // changes curr_queue
    }
  } else if (dram.busy_until + 1 <= stats->clock_cycles) {
    if (curr_queue == -1) {
      return;
    } else if (curr_req->is_NULL()) {
      // NOTE: processing a null request takes 1 clock cycle time
      complete_request();
    } else {
      dram.issue_request(curr_req->addr);
    }
  }
}

void DramDriver::enqueue_request(Request& request) {
  // int enqueue_delay = 1;
  // string enqueue_remark = "Selecting queue";

  auto row = dram.addr2rowcol(request.addr).first;

  int unallocated_q = -1;
  for (int i = 0; i < NUM_QUEUES; i++) {
    if (__queue2row_LUT[i] == row) {
      // add_delay(enqueue_delay, enqueue_remark);
      // add_delay(0, enqueue_remark);
      insert_request(request, i);

      return;
    } else if (__queue2row_LUT[i] == -1 && unallocated_q == -1) {
      unallocated_q = i;
    }
  }

  if (unallocated_q == -1) {
    throw QueueFull();
  } else {
    __queue2row_LUT[unallocated_q] = row;

    // add_delay(enqueue_delay, enqueue_remark);
    // add_delay(0, enqueue_remark);
    insert_request(request, unallocated_q);
  }
}

void DramDriver::insert_request(Request& request, int q_num) {
  int insertion_delay = 1;
  string insertion_remark = "Inserting into queue";
  if (request.is_NULL()) {
    return;
  }
  bool lw_eliminated = false;
  auto slot = get_empty_slot(q_num);

  if (request.is_LW()) {
    auto core = request.core;
    auto reg = request.dst_reg;

    // Eliminate redundant LW
    auto redundant_lw_ptr = lookup_LW(core, reg);
    if (redundant_lw_ptr != nullptr) {
      auto [red_q, _] = __core_reg2offsets_LUT[core][reg];

      redundant_lw_ptr->nullify();
      __queue2size_LUT[red_q]--;
      __core_reg2offsets_LUT[core][reg] = make_pair(-1, -1);

      lw_eliminated = true;
    }

    // Forwarding
    auto existing_sw = lookup_SW(q_num, request.addr);

    if (existing_sw != nullptr) {
      *request.dst = existing_sw->val;

      auto log = Log{};
      log.device = DRIVER_ID;
      log.cycle_period =
          make_pair(stats->clock_cycles + 1, stats->clock_cycles + 1);
      log.remarks.push_back("Forwarding values from DRAM driver");
      log.queue_details = queueToStr();

      stats->logs.push_back(log);

      reg_updates[core] = stats->clock_cycles + 1;

      add_delay(insertion_delay, insertion_remark);
      return;
    }

    // Update LUT
    __core_reg2offsets_LUT[core][reg] = make_pair(q_num, slot);

    if (lw_eliminated) {
      // If a redundant LW was removed then a write must occur to
      // __core_reg2offsets_LUT in this cycle.
      // The above write will occur on next cycle.
      insertion_delay++;
    }
  } else {
    // check for redundant SW
    auto existing_sw = lookup_SW(q_num, request.addr);
    if (existing_sw != nullptr) {
      *existing_sw = request;

      add_delay(insertion_delay, insertion_remark);
      return;
    }

    // check for a LW on which this SW request depends
  }

  queues[q_num][slot] = request;
  __queue2size_LUT[q_num]++;

  if (curr_queue == -1) {
    curr_queue = q_num;
    curr_index = slot;
  }

  add_delay(insertion_delay, insertion_remark);
}

void DramDriver::move_index() {
  if (curr_queue == -1) {
    return;
  }
  int first_lw = -1, first_sw = -1;
  for (int i = 0; i < QUEUE_SIZE; i++) {
    if (queues[curr_queue][i].is_NULL()) {
      continue;
    }
    if (queues[curr_queue][i].is_LW() && first_lw == -1) {
      first_lw = i;
    } else if (queues[curr_queue][i].is_SW() && first_sw == -1) {
      first_sw = i;
    }
  }

  if (first_lw == -1 && first_sw == -1) {
    curr_index = 0;
    return;
  } else if (first_lw != -1) {
    curr_index = first_lw;
  } else if (first_sw != -1) {
    curr_index = first_sw;
  }
  auto curr_req = get_curr_request();
  int lw_index = get_pending_lw_index(curr_queue, *curr_req);
  if (lw_index != -1 && curr_req->is_SW()) {
    curr_index = lw_index;
  }
}

void DramDriver::complete_request() {
  auto req = get_curr_request();
  auto core = req->core;

  int completion_delay = 0;
  string completion_remark = "Completing request";

  if (req->is_SW()) {
    dram.set_mem_word(req->addr, req->val);
  } else if (req->is_LW()) {
    auto val = dram.get_mem_word(req->addr, req->dst_reg);
    *(req->dst) = val;

    // Set the latest time for register update
    reg_updates[core] = stats->clock_cycles;

    // Reset the __core_reg2offsets_LUT for the LW request
    auto [q_num, q_index] = __core_reg2offsets_LUT[req->core][req->dst_reg];
    if (q_num != -1 && q_index == curr_index) {
      __core_reg2offsets_LUT[req->core][req->dst_reg] = make_pair(-1, -1);
    }
  }
  if (!req->is_NULL()) {
    __queue2size_LUT[curr_queue]--;
    queues[curr_queue][curr_index].nullify();

    completion_delay++;
  }

  move_index();

  if (is_empty_queue(curr_queue)) {
    __queue2row_LUT[curr_queue] = -1;
    curr_queue = -1;
  }
  if (!req->is_NULL()) {
    round_counter++;
  }
  add_delay(completion_delay, completion_remark);
  // add_delay(0, completion_remark);
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
    if (req.is_SW() && req.addr == addr &&
        !(q_num == curr_queue && i == curr_index)) {
      return &req;
    }
    i++;
  }
  return nullptr;
}

Request* DramDriver::lookup_request(int core, int reg) {
  auto [q_num, q_index] = __core_reg2offsets_LUT[core][reg];
  if (q_num == -1) {
    return nullptr;
  }
  return &queues[q_num][q_index];
}

Request* DramDriver::lookup_LW(int core, int reg) {
  auto [q_num, q_index] = __core_reg2offsets_LUT[core][reg];
  if (q_num == -1) {
    return nullptr;
  }
  if (q_num == curr_queue && q_index == curr_index) {
    return nullptr;
  }
  return &queues[q_num][q_index];
}

int DramDriver::get_pending_lw_index(int q_num, Request& req) {
  for (int i = 0; i < QUEUE_SIZE; i++) {
    auto& r = queues[q_num][i];
    if (r.is_LW() && r.timestamp < req.timestamp && r.addr == req.addr &&
        !(q_num == curr_queue && i == curr_index)) {
      return i;
    }
  }
  return -1;
}

Request* DramDriver::get_curr_request() {
  if (curr_queue == -1 || is_empty_queue(curr_queue)) {
    return nullptr;
  }
  return &queues[curr_queue][curr_index];
}
void DramDriver::choose_next_queue() {
  int scheduling_delay = 1;
  string scheduling_remark = "Scheduling next queue";

  int lw_pos_in_queue[NUM_QUEUES];   // metric denoting the importance of the
                                     // blocked requests in a queue
  double core_req_freqs[MAX_CORES];  // ratio of Request frequency to total
                                     // number of instructions

  memset(lw_pos_in_queue, -1, sizeof(lw_pos_in_queue));
  memset(core_req_freqs, 0, sizeof(core_req_freqs));

  for (int core = 0; core < MAX_CORES; core++) {
    auto& blocked_regs = __core2blocked_reg_LUT[core];

    for (int j = 0; j < MAX_BLOCKED_REG; j++) {
      int reg = blocked_regs[j];
      if (reg == -1) continue;  // register not blocked
      Request* rq = lookup_LW(core, reg);
      auto [q_num, q_offset] = __core_reg2offsets_LUT[core][reg];
      if (q_num != -1) {  // a blocking request of that register exists
        lw_pos_in_queue[q_num] = q_offset;
      }
    }
    // add 1 to prevent div by 0
    core_req_freqs[core] =
        1.0 * __core2freq_LUT[core] / (1 + __core2instr_LUT[core]);
  }

  int best_q = 0;
  double freq_offset = 0.01;  // to prevent division by 0

  int sum = 0;
  for (int i = 0; i < NUM_QUEUES; i++) sum += lw_pos_in_queue[i];

  if (sum == -NUM_QUEUES) {
    for (int i = 1; i < NUM_QUEUES; i++) {
      // freq - , size +
      double prev_metric =
          __queue2size_LUT[best_q] / (freq_offset + core_req_freqs[best_q]);
      double new_metric =
          __queue2size_LUT[i] / (freq_offset + core_req_freqs[i]);
      if (new_metric > prev_metric) {
        best_q = i;
      }
    }
  } else {
    for (int i = 1; i < NUM_QUEUES; i++) {
      // freq - , size +
      double prev_metric =
          __queue2size_LUT[best_q] / (freq_offset + core_req_freqs[best_q]);
      double new_metric =
          __queue2size_LUT[i] / (freq_offset + core_req_freqs[i]);
      if (lw_pos_in_queue[i] != -1 && new_metric > prev_metric) {
        best_q = i;
      }
    }
  }

  // Floating point operations
  scheduling_delay += 2;

  curr_queue = best_q;
  curr_index = std::max(0, lw_pos_in_queue[best_q]);

  round_counter = 0;

  add_delay(scheduling_delay, scheduling_remark);
}

void DramDriver::update_instr_count(int core) { __core2instr_LUT[core]++; }

bool DramDriver::is_idle() { return get_curr_request() == nullptr; }

DramDriver::~DramDriver() { memset(queues, 0, sizeof(queues)); }