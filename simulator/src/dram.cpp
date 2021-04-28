#include "dram.hpp"

using namespace std;

Dram::Dram(int row_ad, int col_ad)
    : ROW_ACCESS_DELAY(row_ad), COL_ACCESS_DELAY(col_ad) {}

void Dram::row2buffer(int row, Stats& stats) {
  auto log = Log{};
  log.cycle_period = {busy_until - ROW_ACCESS_DELAY - COL_ACCESS_DELAY + 1,
                      busy_until - COL_ACCESS_DELAY};

  // make_pair(stats.clock_cycles + 1, stats.clock_cycles + ROW_ACCESS_DELAY);
  log.remarks.push_back("DRAM: activating row " + to_string(row));
  // stats.clock_cycles += ROW_ACCESS_DELAY;
  stats.rowbuff_update_count++;
  stats.logs.push_back(log);

  copy(memory[row], memory[row] + Dram::NUM_COLS, row_buff);
  active_row = row;
}

int Dram::get_num_rows() { return Dram::NUM_ROWS; }

int Dram::get_active_row() { return active_row; }

void Dram::buffer2row(int& row, Stats& stats) {
  if (row == -1) {
    return;
  }

  auto log = Log{};
  log.cycle_period = {start_from, start_from + ROW_ACCESS_DELAY - 1};
  // make_pair(stats.clock_cycles + 1, stats.clock_cycles + ROW_ACCESS_DELAY);
  log.remarks.push_back("DRAM: flushing row buffer to memory for row: " +
                        to_string(row));
  // stats.clock_cycles += ROW_ACCESS_DELAY;

  for (int i = 0; i < Dram::NUM_COLS; i++) {
    if (memory[row][i] != row_buff[i]) {
      log.memory_updates[(row * Dram::NUM_COLS + i) * sizeof(hd_t)] =
          row_buff[i];
    }
    memory[row][i] = row_buff[i];
  }
  stats.logs.push_back(log);
  // active_row = row;

  // row = -1;
}

void Dram::check_word_aligned(int addr) {
  if (!(addr % sizeof(hd_t) == 0)) {
    throw runtime_error("Address not word aligned : " + to_string(addr));
  }
}

pair<int, int> Dram::addr2rowcol(int addr) {
  check_word_aligned(addr);
  addr /= sizeof(hd_t);
  int row = addr / (Dram::NUM_COLS);
  int col = addr % (Dram::NUM_COLS);

  if (!(row < NUM_ROWS && col < NUM_COLS)) {
    throw runtime_error("Address out of bounds : " + to_string(addr));
  }
  return make_pair(row, col);
}

void Dram::issue_request(int addr, Stats& stats) {
  int new_active_row = addr2rowcol(addr).first;
  start_from = stats.clock_cycles;
  busy_until = start_from + COL_ACCESS_DELAY - 1;
  if (active_row != new_active_row) {
    busy_until += ROW_ACCESS_DELAY;
    if (active_row != -1) {
      busy_until += ROW_ACCESS_DELAY;
    }
  }

  // active_row = new_active_row;
}

hd_t Dram::get_mem_word(int addr, int dst_reg, Stats& stats) {
  auto point = addr2rowcol(addr);

  if (active_row != point.first) {
    buffer2row(active_row, stats);
    row2buffer(point.first, stats);
  }

  stats.logs.push_back(Log{});
  auto& log = stats.logs.back();

  log.cycle_period = {busy_until - COL_ACCESS_DELAY + 1, busy_until};
  // make_pair(stats.clock_cycles + 1, stats.clock_cycles + COL_ACCESS_DELAY);
  log.remarks.push_back(
      "DRAM: Read " + to_string(row_buff[point.second]) +
      " from row buffer @ " + to_string(addr) +
      " to register: " + reverse_register_map["$" + to_string(dst_reg)]);
  // stats.clock_cycles += COL_ACCESS_DELAY;
  // busy_until = stats.clock_cycles;
  return row_buff[point.second];
}

void Dram::set_mem_word(int addr, hd_t val, Stats& stats) {
  auto point = addr2rowcol(addr);
  if (active_row != point.first) {
    buffer2row(active_row, stats);
    row2buffer(point.first, stats);
  }

  auto log = Log{};

  log.cycle_period = {busy_until - COL_ACCESS_DELAY + 1, busy_until};
  // make_pair(stats.clock_cycles + 1, stats.clock_cycles + COL_ACCESS_DELAY);

  log.rowbuff_updates[addr] = val;
  stats.updated_memory[addr] = val;

  log.remarks.push_back("DRAM: Writing " + to_string(val) +
                        " to row buffer @ " + to_string(addr));
  // stats.clock_cycles += COL_ACCESS_DELAY;
  stats.rowbuff_update_count += 1;
  stats.logs.push_back(log);
  // busy_until = stats.clock_cycles;
  row_buff[point.second] = val;
}
