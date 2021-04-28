#include <dram_driver.hpp>
using namespace std;

DramDriver::DramDriver(Dram dram) : dram(dram) {
  req_queue_list = q_t_list();
  for (int i = 0; i < dram.get_num_rows(); i++) {
    req_queue_list.push_back(q_t());
  }
  // curr = req_queue_list[0].begin();
  // curr_queue = req_queue_list.
}

void DramDriver::issue_write(int core, int addr, hd_t val, Stats& stats) {
  auto req = Request{addr, val, stats.clock_cycles, core};
  enqueue_request(req);
}

void DramDriver::issue_read(int core, int addr, hd_t* dst, Stats& stats,
                            int dst_reg) {
  auto req = Request{addr, -1, stats.clock_cycles, core, dst, dst_reg};
  enqueue_request(req);
}

bool DramDriver::req_queue_not_empty() {
  for (auto& q : req_queue_list) {
    if (q.size() > 0) {
      return true;
    }
  }
  return false;
}

pair<bool, q_t::iterator> DramDriver::get_blocked_request(int core, int reg) {
  for (auto& q : req_queue_list) {
    for (auto r_it = q.begin(); r_it != q.end(); r_it++) {
      if (r_it->core == core && r_it->dst_reg == reg) return {true, r_it};
    }
  }
  return {false, req_queue_list.at(0).end()};
}

pair<bool, q_t::iterator> DramDriver::get_pending_SW(int core, int addr) {
  int row = dram.addr2rowcol(addr).first;

  auto& req_queue = req_queue_list[row];
  for (auto it = req_queue.begin(); it != req_queue.end(); ++it) {
    if (it->core == core && it->addr == addr && it->dst == nullptr &&
        it != curr) {
      return {true, it};
    }
  }
  return {false, req_queue.end()};
}

int DramDriver::get_row(q_t::iterator it) {
  return dram.addr2rowcol(it->addr).first;
}

// Executes request in an order determined by the blocked registers
// The method assumes that it is called at every iteration of clock cycles.
void DramDriver::perform_tasks(Stats& stats,
                               unordered_map<int, vector<int>> blocked_regs) {
  if (dram.busy_until + 1 == stats.clock_cycles) {
    int curr_row = get_row(curr);

    complete_request(stats);

    auto prev = curr;

    choose_next_request(stats, blocked_regs);  // now always switch rows

    if (prev == curr) {
      curr = req_queue_list[curr_row].erase(curr);
    } else {
      req_queue_list[curr_row].erase(prev);
    }

    if (req_queue_list.at(curr_row).size() == 0 ||
        curr == req_queue_list.at(curr_row).end()) {
      for (auto& q : req_queue_list) {
        if (q.size() > 0) {
          curr = q.begin();
        }
      }
    }
  }

  // auto [next_row, _] = dram.addr2rowcol(curr->addr);
  // // moving to next row
  // if (dram.get_active_row() != next_row) {
  //   goto_blocking_request_row(blocked_regs);
  // }
  // // moving to next registers
  // if (curr->addr != deleted_curr_addr) {
  //   goto_blocking_request_address(blocked_regs);
  // }

  if (dram.busy_until + 1 <= stats.clock_cycles) {
    if (!req_queue_not_empty()) return;
    dram.issue_request(curr->addr, stats);
  }
}

// if DRAM is free

void DramDriver::choose_next_request(
    Stats& stats, unordered_map<int, vector<int>> blocked_regs) {
  for (auto& regs_it : blocked_regs) {
    auto [core, row_regs] = regs_it;
    for (auto& reg : row_regs) {
      auto [found, iter] = get_blocked_request(core, reg);
      if (found) {
        auto [pending_sw, sw_iter] = get_pending_SW(core, iter->addr);
        if (pending_sw) {
          curr = sw_iter;

        } else {
          curr = iter;
        }
        return;
      }
    }
  }
}

bool DramDriver::is_blocking_reg(int core, int reg) {
  return get_blocked_request(core, reg).first;
}

void DramDriver::enqueue_request(Request& request) {
  int row = dram.addr2rowcol(request.addr).first;

  // delete LW / SW
  if (request.dst == nullptr) {
    // SW
    delete_redundant_SW(request);
  } else {
    // LW
    delete_redundant_LW(request);
  }

  bool init_curr = !req_queue_not_empty();
  req_queue_list.at(row).push_back(request);
  if (init_curr) {
    curr = req_queue_list.at(row).begin();
  }
}

void DramDriver::delete_redundant_SW(Request& request) {
  int row = dram.addr2rowcol(request.addr).first;

  auto& req_queue = req_queue_list[row];
  q_t::iterator del_it = req_queue.end();
  bool trailing_lw_found = false;

  for (auto it = req_queue.begin(); it != req_queue.end(); ++it) {
    if (it->addr == request.addr) {
      if (it->dst == nullptr) {
        del_it = it;
      } else if (del_it != req_queue.end()) {
        trailing_lw_found = true;
        break;
      }
    }
  }

  if (del_it != req_queue.end() && del_it != curr && !trailing_lw_found) {
    req_queue.erase(del_it);
  }
}

void DramDriver::delete_redundant_LW(Request& request) {
  int q_row = 0;
  for (auto& q : req_queue_list) {
    q_t::iterator del_it = q.end();
    for (auto it = q.begin(); it != q.end(); it++) {
      if (it->core == request.core && it->dst != nullptr &&
          it->dst_reg == request.dst_reg) {
        del_it = it;
      }
    }
    if (del_it != q.end() && del_it != curr) {
      q.erase(del_it);
    }
    q_row++;
  }
}

void DramDriver::complete_request(Stats& stats) {
  Request req = *curr;
  if (req.dst == nullptr) {
    dram.set_mem_word(req.addr, req.val, stats);
  } else {
    auto val = dram.get_mem_word(req.addr, req.dst_reg, stats);
    *req.dst = val;
  }
  dram_row = dram.addr2rowcol(req.addr).first;
}