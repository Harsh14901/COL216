#ifndef HARDWARE_H
#define HARDWARE_H

#include <bits/stdc++.h>

#include <dram_driver.hpp>
#include <structures.hpp>
using namespace std;

class Hardware {
 public:
  const static int REGISTER_NUM = 32;
  const static int SP_REG_ID = 29;
  const static int BITS = 32;
  const static int BYTES = BITS / 8;

  const static int INT_TYPE_J_SIZE = 26;
  const static int INT_TYPE_R_SIZE = 16;
  const static int INT_TYPE_I_SIZE = 16;

  const static long HD_T_MAX = 2147483647;
  const static long HD_T_MIN = -2147483648;

  Hardware();

  void execute_current();
  void advance_pc();
  void terminate();
  void start_execution();
  void set_blocking_mode(bool block);
  void set_id(int id);
  int get_id();
  vector<hd_t> getRegisters();
  void load_dram_driver(DramDriver* driver);
  void load_program(vector<Instruction> program);
  vector<int> get_blocked_registers();
  void load_stats(Stats* stats);

  class Terminated : public std::runtime_error {
   public:
    Terminated() : std::runtime_error("") {}
    Terminated(const string& msg) : std::runtime_error(msg) {}
  };
  bool last_blocked = false;

 private:
  vector<hd_t> registers;

  vector<Instruction> program;
  vector<Instruction>::iterator pc;

  DramDriver* dram_driver;
  Stats* stats;

  int blocked_register = -1;
  map<int, int> blocking_registers;
  bool blocking = false;
  int CORE_ID = -1;

 protected:
  void is_valid_reg(int id);
  void is_valid_reg(int s1, int s2, int s3 = 0);
  void check_overflow(long long val);
  void add(int dst, int src1, int src2);
  void sub(int dst, int src1, int src2);
  void mul(int dst, int src1, int src2);
  void slt(int dst, int src1, int src2);
  void addi(int dst, int src, hd_t value);
  void beq(int src1, int src2, int jump);
  void bne(int src1, int src2, int jump);
  void j(int jump);
  void lw(int dst, int src, int offset);
  void sw(int src, int dst, int offset);
  void initialize_registers();
  void set_register(int dst, hd_t value);
};

#endif
