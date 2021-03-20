#ifndef DRAM_H
#define DRAM_H

#include <bits/stdc++.h>

#include <structures.hpp>

using namespace std;
typedef int32_t hd_t;
enum ENDIAN { BIG, LITTLE };
class Dram {
 private:
  const static int NUM_ROWS = 1 << 10;
  const static int NUM_COLS = 1 << 8;

  int active_row = -1;
  hd_t memory[Dram::NUM_ROWS][Dram::NUM_COLS] = {0};
  hd_t row_buff[Dram::NUM_COLS] = {0};

 public:
  const static int MAX_MEMORY = sizeof(memory);  // In bytes
  const static ENDIAN endianness = BIG;
  int ROW_ACCESS_DELAY;
  int COL_ACCESS_DELAY;

  Dram(int row_access_delay = 10, int col_access_delay = 2);

  hd_t get_mem_word(int addr, Stats& stats);
  void set_mem_word(int addr, hd_t val, Stats& stats);

 protected:
  void check_word_aligned(int addr);
  pair<int, int> addr2rowcol(int addr);
  void row2buffer(int row, Stats& stats);
  void buffer2row(int& row, Stats& stats);
};
#endif