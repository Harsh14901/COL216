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
  int timestamp = -1;
  int core = -1;

  hd_t* dst = nullptr;
  int dst_reg = -1;

  bool is_LW();
  bool is_SW();
  bool is_NULL();
  void nullify();
};
typedef deque<Request> q_t;
class DramDriver {
 private:
  const static int NUM_QUEUES = 8;
  const static int QUEUE_SIZE = 8;
  const static int REG_COUNT = 32;
  const static int MAX_CORES = 16;
  const static int MAX_BLOCKED_REG = 3;

  Dram dram;
  int cores;
  vector<q_t> queues = vector<q_t>(NUM_QUEUES);

  // A 2-D LUT of core, reg -> {queue_num, queue_offset} pair
  pair<int, int> __core_reg2offsets_LUT[MAX_CORES][REG_COUNT];

  // An LUT indicating the change in size of queue since the
  // __core_reg2offsets_LUT was updated with the indices
  // Always positive
  int __queue2offset_LUT[NUM_QUEUES];

  // An LUT indicating the row that a queue represents
  int __queue2row_LUT[NUM_QUEUES];

  // An LUT indicating the frequency of DRAM accesses by a core
  int __core2freq_LUT[MAX_CORES];

  // LUT indicating the total number of instructions executed by a core
  int __core2instr_LUT[MAX_CORES];

  // An LUT indicating the VA to PA offsets of a core
  int __core2PA_offsets_LUT[MAX_CORES];

  // An LUT indicating the blocked registers of a core
  int __core2blocked_reg_LUT[MAX_CORES][MAX_BLOCKED_REG];

  int curr_queue = -1;
  int round_counter = 0;

  void complete_request(Stats& stats);
  void enqueue_request(Request& request);            // Throws QueueFull
  void insert_request(Request& request, int q_num);  // Throws QueueFull
  void choose_next_queue();
  Request* lookup_SW(int q_num, int addr);
  Request* lookup_LW(int core, int reg);
  Request* lookup_request(int core, int reg);
  Request* get_curr_request();

  void __init_LUT();

 public:
  DramDriver(Dram Dram, int cores);
  void issue_write(int core, int addr, hd_t val,
                   Stats& stats);  // Throws QueueFull
  void issue_read(int core, int addr, hd_t* dst, Stats& stats,
                  int dst_reg);  // Throws QueueFull

  // Call at every clock cycle
  int perform_tasks(Stats& stats);
  bool is_blocking_reg(int core, int reg);
  void set_blocking_regs(int core, vector<int>& regs);
  void update_instr_count(int core);
  ~DramDriver();
  class QueueFull : public std::runtime_error {
   public:
    QueueFull() : std::runtime_error("") {}
    QueueFull(const string& msg) : std::runtime_error(msg) {}
  };

  class InvalidMemory : public std::runtime_error {
   public:
    InvalidMemory() : std::runtime_error("") {}
    InvalidMemory(const string& msg) : std::runtime_error(msg) {}
  };
};
#endif