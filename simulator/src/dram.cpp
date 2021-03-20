#include <dram.hpp>

using namespace std;

Dram::Dram(int row_ad, int col_ad)
    : ROW_ACCESS_DELAY(row_ad), COL_ACCESS_DELAY(col_ad) {}

void Dram::row2buffer(int row, Stats& stats) {
  auto log = Log{};
  log.cycle_period =
      make_pair(stats.clock_cycles + 1, stats.clock_cycles + ROW_ACCESS_DELAY);
  log.remarks.push_back("DRAM: activating row " + to_string(row));
  stats.clock_cycles += ROW_ACCESS_DELAY;
  stats.logs.push_back(log);

  copy(memory[row], memory[row] + Dram::NUM_COLS, row_buff);
  active_row = row;
}

void Dram::buffer2row(int& row, Stats& stats) {
  if (row == -1) {
    return;
  }

  auto log = Log{};
  log.cycle_period =
      make_pair(stats.clock_cycles + 1, stats.clock_cycles + ROW_ACCESS_DELAY);
  log.remarks.push_back("DRAM: flushing row buffer to memory for row: " +
                        to_string(row));
  stats.clock_cycles += ROW_ACCESS_DELAY;

  for (int i = 0; i < Dram::NUM_COLS; i++) {
    if (memory[row][i] != row_buff[i]) {
      log.memory_updates[(row * Dram::NUM_COLS + i) * sizeof(hd_t)] =
          row_buff[i];
    }
    memory[row][i] = row_buff[i];
  }
  stats.logs.push_back(log);

  row = -1;
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

hd_t Dram::get_mem_word(int addr, Stats& stats) {
  auto point = addr2rowcol(addr);

  if (active_row != point.first) {
    buffer2row(active_row, stats);
    row2buffer(point.first, stats);
  }

  stats.logs.push_back(Log{});
  auto& log = stats.logs.back();

  log.cycle_period =
      make_pair(stats.clock_cycles + 1, stats.clock_cycles + COL_ACCESS_DELAY);
  log.remarks.push_back("DRAM: Read from row buffer");
  stats.clock_cycles += COL_ACCESS_DELAY;

  return row_buff[point.second];
}

void Dram::set_mem_word(int addr, hd_t val, Stats& stats) {
  auto point = addr2rowcol(addr);
  if (active_row != point.first) {
    buffer2row(active_row, stats);
    row2buffer(point.first, stats);
  }

  auto log = Log{};

  log.cycle_period =
      make_pair(stats.clock_cycles + 1, stats.clock_cycles + COL_ACCESS_DELAY);

  log.rowbuff_updates[addr] = val;
  stats.updated_memory[addr] = val;

  log.remarks.push_back("DRAM: Writing to row buffer");
  stats.clock_cycles += COL_ACCESS_DELAY;
  stats.rowbuff_update_count += 1;
  stats.logs.push_back(log);

  row_buff[point.second] = val;
}