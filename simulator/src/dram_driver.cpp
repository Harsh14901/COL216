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

bool DramDriver::req_queue_not_empty() { return req_queue.size() > 0; }

void DramDriver::issue_queue_requests(Stats& stats) {
  if (req_queue.empty()) {
    return;
  }
  dram.issue_request(curr->addr, stats);
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

void DramDriver::delete_stranded_SW(Request& request,
                                    q_t::reverse_iterator reverse_it,
                                    q_t::iterator forward_it) {
  auto it = forward_it;
  for (; it != req_queue.end(); it++) {
    if (it->addr != forward_it->addr) return;
    if (it->dst != nullptr && it != forward_it) return;
    if (it->dst == nullptr) break;
  }
  if (it == req_queue.end()) return;
  auto helper_it = forward_it;
  auto it2 = reverse_it;
  for (; it2 != req_queue.rend(); it2++, helper_it--) {
    if (it2->addr != forward_it->addr) return;
    if (it2->dst != nullptr && it2 != reverse_it) return;
    if (it2->dst == nullptr && helper_it != curr) break;
  }
  if (it2 == req_queue.rend()) return;

  req_queue.erase(helper_it);
}

void DramDriver::queue_request(Request& request) {
  if (req_queue.empty()) {
    req_queue.push_back(request);
    curr = req_queue.begin();
    return;
  }

  int request_row = dram.addr2rowcol(request.addr).first;
  q_t::iterator inserted;

  if (request.dst == nullptr) {  // is SW
    auto helper_it = req_queue.end();
    auto reverse_it = req_queue.rbegin();
    for (; reverse_it != req_queue.rend(); reverse_it++) {
      helper_it--;
      if (reverse_it->addr == request.addr) {
        if (reverse_it->dst == nullptr) {  // is SW
          if (helper_it == curr) continue;

          req_queue.erase(helper_it);
        }  // is LW
        break;
      }
    }
  } else {  // is LW
    auto reverse_it = req_queue.rend();
    for (auto forward_it = req_queue.begin(); forward_it != req_queue.end();
         forward_it++) {
      reverse_it--;
      if (forward_it->dst_reg == request.dst_reg && forward_it != curr) {
        delete_stranded_SW(request, reverse_it, forward_it);
        req_queue.erase(forward_it);
        break;
      }
    }
  }

  auto prev_it = req_queue.end();
  auto it = req_queue.rbegin();

  for (it = req_queue.rbegin(); it != req_queue.rend(); it++) {
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

  // int inserted_row = dram.addr2rowcol(inserted->addr).first;
  // int curr_row = dram.addr2rowcol(curr->addr).first;

  // ???
  // if (inserted_row == dram_row && curr_row != dram_row) {
  //   curr = inserted;
  // }
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