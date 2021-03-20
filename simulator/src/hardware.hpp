#ifndef HARDWARE_H
#define HARDWARE_H

#include <bits/stdc++.h>

#include <structures.hpp>
using namespace std;

typedef int32_t hd_t;
enum ENDIAN { BIG, LITTLE };
class Hardware {
 public:
  //  TODO:
  const static int MAX_MEMORY = 1 << 20;  // In Bytes
  // const static int MAX_MEMORY = 2 << 10;  // In Bytes
  const static int REGISTER_NUM = 32;
  const static int SP_REG_ID = 29;
  const static int BITS = 32;
  const static int BYTES = BITS / 8;

  const static int INT_TYPE_J_SIZE = 26;
  const static int INT_TYPE_R_SIZE = 16;
  const static int INT_TYPE_I_SIZE = 16;

  const static long HD_T_MAX = 2147483647;
  const static long HD_T_MIN = -2147483648;
  const static ENDIAN endianness = BIG;

  Hardware();
  Hardware(vector<Instruction> program);

  void print_instruction();
  void print_contents();
  void execute_current();
  void advance_pc();
  void terminate();
  void start_execution();
  Stats get_stats();

 private:
  vector<hd_t> registers;
  // hd_t memory[MAX_MEMORY] = {0};
  byte memory[MAX_MEMORY] = {byte{0x00}};

  unsigned int mem_size;
  vector<Instruction> program;
  vector<Instruction>::iterator pc;

  Stats stats;

 protected:
  void is_valid_reg(int id);
  void is_valid_reg(int s1, int s2, int s3 = 0);
  void is_valid_memory(int p);
  hd_t get_mem_word(int p);
  void set_mem_word(int p, hd_t val);
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
  void update_stats(string reg);
};

#endif