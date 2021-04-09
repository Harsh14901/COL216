#include <dram_driver.hpp>
using namespace std;

DramDriver::DramDriver(Dram dram)
    : req_queue(q_t()), curr(req_queue.begin()), dram(dram) {}

bool DramDriver::is_blocking_reg(int reg) {
  for (auto req : req_queue) {
    if (req.dst_reg == reg) return true;
  }
  return false;
}

void DramDriver::issue_write(int addr, hd_t val, Stats& stats) {
  auto req = Request{addr, val, stats.clock_cycles};
  // update_queue(stats);
  queue_request(req);
}

void DramDriver::issue_read(int addr, hd_t* dst, Stats& stats, int dst_reg) {
  auto req = Request{addr, -1, stats.clock_cycles, dst, dst_reg};
  // update_queue(stats);
  queue_request(req);
}

// q_t::iterator DramDriver::execute_pending_for_register(Stats& stats,
//                                                        hd_t blocked_register)
//                                                        {
//   q_t::iterator temp_start, temp_end;
//   for (; temp_start != req_queue.end(); temp_start++) {
//     hd_t* dst = (*temp_start).dst;
//     if (dst != nullptr && *dst == blocked_register) break;
//   }
//   temp_end = execute_pending(temp_start, req_queue.end(), stats);
//   req_queue.erase(temp_start, temp_end);
//   // TODO is it correct

//   return temp_end;
// }

// q_t::iterator DramDriver::execute_pending(q_t::iterator start_it,
//                                           q_t::iterator end_it, Stats& stats,
//                                           bool exec_all) {
//   auto before_time = stats.clock_cycles;
//   for (start_it; start_it != end_it; start_it++) {
//     auto req_time = max(start_it->timestamp, dram.busy_until);
//     if (!exec_all && req_time >= before_time) {
//       break;
//     }
//     stats.clock_cycles = req_time;
//     execute_request(*start_it, stats);
//   }
//   if (!exec_all) {
//     stats.clock_cycles = before_time;
//   }
//   return start_it;
// }

bool DramDriver::req_queue_not_empty() { return req_queue.size() > 0; }

void DramDriver::issue_queue_requests(Stats& stats) {
  if (req_queue.empty()) {
    return;
  }
  dram.issue_request(curr->addr, stats);

  // auto next = curr;
  // next++;
  // req_queue.erase(curr);
  // curr = next;
}

void DramDriver::goto_blocking_request_batch(vector<int> blocked_regs) {
  q_t::iterator temp = req_queue.begin();
  for (; temp != req_queue.end(); temp++) {
    if (count(blocked_regs.begin(), blocked_regs.end(), temp->dst_reg)) {
      q_t::iterator start_of_batch = temp;
      while (start_of_batch->addr == temp->addr) start_of_batch--;
      curr = ++start_of_batch;
      return;
    }
  }

  // if none are blocking start from beginning of queue
  curr = req_queue.begin();
}

void DramDriver::execute_current_in_queue(Stats& stats,
                                          vector<int> blocked_regs) {
  auto prev_it = curr;

  // terminate previous request and decide on next request
  if (dram.busy_until + 1 == stats.clock_cycles) {
    execute_request(*curr, stats);

    auto next = curr;
    next++;
    if (next == req_queue.end()) next = req_queue.begin();
    req_queue.erase(curr);
    curr = next;

    auto [next_row, _] = dram.addr2rowcol(curr->addr);
    if (dram.get_active_row() != next_row) {
      goto_blocking_request_batch(blocked_regs);
    }
  }

  // if DRAM is free
  if (dram.busy_until + 1 <= stats.clock_cycles) {
    issue_queue_requests(stats);
  }
}

// void DramDriver::update_queue(Stats& stats) {
//   if (req_queue.empty()) {
//     return;
//   }

//   auto issue_time = stats.clock_cycles;

//   auto it = execute_pending(curr, req_queue.end(), stats);

//   if (it == req_queue.end()) {
//     auto it2 = execute_pending(req_queue.begin(), curr, stats);
//     req_queue.erase(req_queue.begin(), it2);
//     req_queue.erase(curr, it);
//     curr = req_queue.begin();
//   } else {
//     req_queue.erase(curr, it);
//     curr = it;
//   }

//   if (curr == req_queue.end()) {
//     curr = req_queue.begin();
//   }
// }

void DramDriver::queue_request(Request& request) {
  if (req_queue.empty()) {
    req_queue.push_back(request);
    curr = req_queue.begin();
    return;
  }
  auto prev_it = req_queue.end();
  auto it = req_queue.rbegin();
  int request_row = dram.addr2rowcol(request.addr).first;
  q_t::iterator inserted;

  for (it; it != req_queue.rend(); it++) {
    auto it_row = dram.addr2rowcol(it->addr).first;
    if (it_row <= request_row) {
      inserted = req_queue.insert(prev_it, request);
      break;
    }
    prev_it--;
  }
  if (it == req_queue.rend()) {
    inserted = req_queue.insert(req_queue.begin(), request);
  }

  int inserted_row = dram.addr2rowcol(inserted->addr).first;
  int curr_row = dram.addr2rowcol(curr->addr).first;

  // ???
  // if (inserted_row == dram_row && curr_row != dram_row) {
  //   curr = inserted;
  // }
}

void DramDriver::initialize_request(Request& req, Stats& stats) {
  // if (dram.addr2rowcol(req.addr).first == dram.get_active_row()){
  //   dram.busy_until = dram
  // }
  // dram.start_operation(req)
}

void DramDriver::execute_request(Request& req, Stats& stats) {
  if (req.dst == nullptr) {
    dram.set_mem_word(req.addr, req.val, stats);
  } else {
    auto val = dram.get_mem_word(req.addr, stats);
    *req.dst = val;
  }
  dram_row = dram.addr2rowcol(req.addr).first;
}

// void DramDriver::execute_all(Stats& stats, hd_t blocked_register) {
//   auto issue_time = stats.clock_cycles;
//   auto initial_pos = curr;

//   if (blocked_register != -1) {
//     auto it = execute_pending(curr, req_queue.end(), stats);
//     req_queue.erase(curr, it);

//     curr = execute_pending_for_register(stats, blocked_register);

//   } else {
//     execute_pending(curr, req_queue.end(), stats, true);
//     execute_pending(req_queue.begin(), curr, stats, true);

//     req_queue.clear();
//     curr = req_queue.begin();
//   }
//   stats.clock_cycles = max(issue_time, dram.busy_until);
// }