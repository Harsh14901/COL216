#ifndef DRAM_H
#define DRAM_H

#include <bits/stdc++.h>

#include "structures.hpp"

using namespace std;
typedef int32_t hd_t;
class Dram {
 private:
  const static int NUM_ROWS = 1 << 10;
  const static int NUM_COLS = 1 << 8;

  int active_row = -1;
  hd_t memory[Dram::NUM_ROWS][Dram::NUM_COLS] = {0};
  hd_t row_buff[Dram::NUM_COLS] = {0};

  Stats* stats;

 public:
  const static int MAX_MEMORY = sizeof(memory);  // In bytes
  int ROW_ACCESS_DELAY;
  int COL_ACCESS_DELAY;
  int busy_until = -1;
  int start_from = -1;

  Dram(int row_access_delay = 10, int col_access_delay = 2);

  void issue_request(int addr);
  hd_t get_mem_word(int addr, int dst_reg);
  void set_mem_word(int addr, hd_t val);
  pair<int, int> addr2rowcol(int addr);
  int get_num_rows();
  int get_active_row();
  void load_stats(Stats* stats);

 protected:
  void check_word_aligned(int addr);
  void row2buffer(int row);
  void buffer2row(int& row);
};
#endif
