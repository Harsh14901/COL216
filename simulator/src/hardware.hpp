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

  Hardware(DramDriver dram_driver);
  Hardware(DramDriver dram_driver, vector<Instruction> program);

  void execute_current(Stats& stats);
  void advance_pc();
  void terminate();
  void start_execution(Stats& stats);
  void set_blocking_mode(bool block);

 private:
  vector<hd_t> registers;

  vector<Instruction> program;
  vector<Instruction>::iterator pc;

  DramDriver dram_driver;

  // int blocking_reg = -1;
  int blocked_register = -1;
  map<int, int> blocking_registers;
  bool blocking = false;

 protected:
  void is_valid_reg(int id);
  void is_valid_reg(int s1, int s2, int s3 = 0);
  void check_overflow(long long val);
  vector<int> get_blocked_registers(Stats& stats);
  void add(int dst, int src1, int src2);
  void sub(int dst, int src1, int src2);
  void mul(int dst, int src1, int src2);
  void slt(int dst, int src1, int src2);
  void addi(int dst, int src, hd_t value);
  void beq(int src1, int src2, int jump);
  void bne(int src1, int src2, int jump);
  void j(int jump);
  void lw(int dst, int src, int offset, Stats& stats);
  void sw(int src, int dst, int offset, Stats& stats);
  void initialize_registers();
  void set_register(int dst, hd_t value);
};

#endif
