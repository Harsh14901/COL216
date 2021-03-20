#include <dram.hpp>

using namespace std;

Dram::Dram(int row_ad, int col_ad)
    : ROW_ACCESS_DELAY(row_ad), COL_ACCESS_DELAY(col_ad) {}

void Dram::row2buffer(int row) {
  copy(memory[row], memory[row] + Dram::NUM_COLS, row_buff);
  active_row = row;
}

void Dram::buffer2row() {
  if (active_row == -1) {
    return;
  }
  copy(row_buff, row_buff + Dram::NUM_COLS, memory[active_row]);
  active_row = -1;
}

void Dram::check_word_aligned(int addr) {
  assert(("Address not word aligned : " + to_string(addr),
          addr % sizeof(hd_t) == 0));
}

pair<int, int> Dram::addr2rowcol(int addr) {
  check_word_aligned(addr);
  addr /= sizeof(hd_t);
  int row = addr / (Dram::NUM_COLS);
  int col = addr % (Dram::NUM_COLS);

  assert(("Address out of bounds : " + to_string(addr),
          row < NUM_ROWS && col < NUM_COLS));
  return make_pair(row, col);
}

hd_t Dram::get_mem_word(int addr) {
  auto point = addr2rowcol(addr);

  if (active_row != point.first) {
    buffer2row();
    row2buffer(point.first);
  }

  return row_buff[point.second];
}

void Dram::set_mem_word(int addr, hd_t val) {
  auto point = addr2rowcol(addr);

  if (active_row != point.first) {
    buffer2row();
    row2buffer(point.first);
  }

  row_buff[point.second] = val;
}