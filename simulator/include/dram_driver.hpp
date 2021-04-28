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
  int core = -1;

  hd_t* dst = nullptr;
  int dst_reg = -1;
};
typedef list<Request> q_t;
typedef vector<q_t> q_t_list;
class DramDriver {
 private:
  q_t_list req_queue_list;
  q_t_list::iterator curr_queue;
  q_t::iterator curr;
  Dram dram;
  int dram_row = -1;
  // q_t::iterator execute_pending_for_register(Stats& stats,
  //                                            hd_t blocked_register);
  // q_t::iterator execute_pending(q_t::iterator start_it, q_t::iterator end_it,
  //                               Stats& stats, bool exec_all = false);
  void complete_request(Stats& stats);
  void enqueue_request(Request& request);
  void choose_next_request(Stats& stats,
                           unordered_map<int, vector<int>> blocked_regs);
  pair<bool, q_t::iterator> get_blocked_request(int core, int reg);
  pair<bool, q_t::iterator> get_pending_SW(int core, int addr);

  int get_row(q_t::iterator it);
  void delete_redundant_SW(Request& r);
  void delete_redundant_LW(Request& r);
  // void issue_queue_requests(Stats& stats);
  // void goto_blocking_request_row(vector<int> blocked_regs);
  // void goto_blockqing_request_address(vector<int> blocked_regs);
  // void delete_stranded_SW(Request& request, q_t::reverse_iterator reverse_it,
  //                         q_t::iterator helper_it);

 public:
  DramDriver(Dram Dram);
  int get_curr_row();
  void issue_write(int core, int addr, hd_t val, Stats& stats);
  void issue_read(int core, int addr, hd_t* dst, Stats& stats, int dst_reg);
  void perform_tasks(Stats& stats,
                     unordered_map<int, vector<int>> blocked_regs);
  // void exec_not_blocked(Stats& stats);
  bool is_blocking_reg(int core, int reg);
  bool req_queue_not_empty();
};
#endif