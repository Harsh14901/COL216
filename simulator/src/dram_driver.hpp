#ifndef DRAM_DRIVER_H
#define DRAM_DRIVER_H

#include <bits/stdc++.h>

#include <dram.hpp>
#include <structures.hpp>
using namespace std;

typedef int32_t hd_t;
struct Request {
  int addr;
  hd_t val;
  int timestamp;
  hd_t* dst = nullptr;
  int dst_reg = -1;
};
typedef list<Request> q_t;

class DramDriver {
 private:
  q_t req_queue;
  q_t::iterator curr;
  Dram dram;
  int dram_row = -1;
  q_t::iterator execute_pending_for_register(Stats& stats,
                                             hd_t blocked_register);
  q_t::iterator execute_pending(q_t::iterator start_it, q_t::iterator end_it,
                                Stats& stats, bool exec_all = false);
  void execute_request(Request& req, Stats& stats);
  void queue_request(Request& request);
  void update_queue(Stats& stats);
  void issue_queue_requests(Stats& stats);
  void goto_blocking_request_batch(vector<int> blocked_regs);

 public:
  DramDriver(Dram Dram);
  void issue_write(int addr, hd_t val, Stats& stats);
  void issue_read(int addr, hd_t* dst, Stats& stats, int dst_reg);
  void execute_all(Stats& stats, hd_t blocked_register = -1);
  void execute_current_in_queue(Stats& stats, vector<int> blocked_regs);
  void initialize_request(Request& req, Stats& stats);
  bool req_queue_not_empty();

  bool is_blocking_reg(int reg);
};
#endif