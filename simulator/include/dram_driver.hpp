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

  int dependency_lw_index = -1;

  bool is_LW();
  bool is_SW();
  bool is_NULL();
  void nullify();
  string to_string();
};
class DramDriver {
 private:
  const static int NUM_QUEUES = 8;
  const static int QUEUE_SIZE = 8;
  const static int REG_COUNT = 32;
  const static int MAX_CORES = 16;
  const static int MAX_BLOCKED_REG = 3;
  static string DRIVER_ID;

  Dram dram;
  int cores;
  Request queues[NUM_QUEUES][QUEUE_SIZE];

  // A 2-D LUT of core, reg -> {queue_num, queue_index} pair
  pair<int, int> __core_reg2offsets_LUT[MAX_CORES][REG_COUNT];

  // An LUT indicating the row that a queue represents
  int __queue2row_LUT[NUM_QUEUES];

  // An LUT storing the number of active requests in each queue
  int __queue2size_LUT[NUM_QUEUES];

  // An LUT indicating the frequency of DRAM accesses by a core
  int __core2freq_LUT[MAX_CORES];

  // LUT indicating the total number of instructions executed by a core
  int __core2instr_LUT[MAX_CORES];

  // An LUT indicating the VA to PA offsets of a core
  int __core2PA_offsets_LUT[MAX_CORES];

  // An LUT indicating the blocked registers of a core
  int __core2blocked_reg_LUT[MAX_CORES][MAX_BLOCKED_REG];

  int curr_queue = -1;
  int curr_index = 0;

  int round_counter = 0;
  int busy_until = -1;

  int* reg_updates;
  Stats* stats;

  void complete_request();
  void enqueue_request(Request& request);            // Throws QueueFull
  void insert_request(Request& request, int q_num);  // Throws QueueFull
  void choose_next_queue();
  void addr_V2P(int& addr, int core);
  void add_delay(int delay, string remark = "");
  int get_empty_slot(int q_num);
  bool is_empty_queue(int q_num);
  Request* lookup_SW(int q_num, int addr);
  Request* lookup_LW(int core, int reg);

  // Returns the index in the queue q_num of a LW request for the address addr
  int get_LW_index(int q_num, int addr);

  Request* lookup_request(int core, int reg);
  Request* get_curr_request();

  void __init_LUT();

 public:
  DramDriver(Dram Dram, int cores, int* reg_updates);
  void issue_write(int core, int addr, hd_t val);  // Throws QueueFull
  void issue_read(int core, int addr, hd_t* dst,
                  int dst_reg);  // Throws QueueFull

  // Call at every clock cycle
  void perform_tasks();
  bool is_blocking_reg(int core, int reg);
  void set_blocking_regs(int core, vector<int>& regs);
  void update_instr_count(int core);
  bool is_idle();
  void load_stats(Stats* stats);
  string queueToStr();
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

  class ControllerBusy : public std::runtime_error {
   public:
    ControllerBusy() : std::runtime_error("") {}
    ControllerBusy(const string& msg) : std::runtime_error(msg) {}
  };
};
#endif