#include <dram_driver.hpp>
using namespace std;

DramDriver::DramDriver(Dram dram)
    : req_queue(q_t()), curr(req_queue.begin()), dram(dram) {}

void DramDriver::issue_write(int addr, hd_t val, Stats& stats) {
  auto req = Request{addr, val, stats.clock_cycles};
  update_queue(stats);
  queue_request(req);
}

void DramDriver::issue_read(int addr, hd_t* dst, Stats& stats) {
  auto req = Request{addr, -1, stats.clock_cycles, dst};
  update_queue(stats);
  queue_request(req);
}

q_t::iterator DramDriver::execute_pending(q_t::iterator start_it,
                                          q_t::iterator end_it, Stats& stats,
                                          bool exec_all) {
  auto before_time = stats.clock_cycles;
  for (start_it; start_it != end_it; start_it++) {
    auto req_time = max(start_it->timestamp, dram.busy_until);
    if (!exec_all && req_time >= before_time) {
      break;
    }
    stats.clock_cycles = req_time;
    execute_request(*start_it, stats);
  }
  if (!exec_all) {
    stats.clock_cycles = before_time;
  }
  return start_it;
}

void DramDriver::update_queue(Stats& stats) {
  if (req_queue.empty()) {
    return;
  }

  auto issue_time = stats.clock_cycles;

  auto it = execute_pending(curr, req_queue.end(), stats);

  if (it == req_queue.end()) {
    auto it2 = execute_pending(req_queue.begin(), curr, stats);
    req_queue.erase(req_queue.begin(), it2);
    req_queue.erase(curr, it);
    curr = req_queue.begin();
  } else {
    req_queue.erase(curr, it);
    curr = it;
  }

  if (curr == req_queue.end()) {
    curr = req_queue.begin();
  }
}

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

  if (inserted_row == dram_row && curr_row != dram_row) {
    curr = inserted;
  }
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

void DramDriver::execute_all(Stats& stats) {
  auto issue_time = stats.clock_cycles;
  execute_pending(curr, req_queue.end(), stats, true);
  execute_pending(req_queue.begin(), curr, stats, true);
  stats.clock_cycles = max(issue_time, dram.busy_until);

  req_queue.clear();
  curr = req_queue.begin();
}