#include <iostream>

#include "compiler.hpp"
#include "hardware.hpp"

using namespace std;
void print_usage();
int main(int argc, char* argv[]) {
  if (argc < 3) {
    print_usage();
    return -1;
  }
  int N = stoi(argv[1]);
  int M = stoi(argv[2]);
  if (argc < N + 3) {
    print_usage();
    return -1;
  }
  vector<vector<Instruction>> programs(N);
  for (int i = 0; i < N; i++) {
    string name = argv[i + 3];
    programs[i] = compile(name);
  }

  Dram dram;

  if (argc >= N + 5) {
    auto row_delay = stoi(argv[N + 3]);
    auto col_delay = stoi(argv[N + 4]);
    dram = Dram(row_delay, col_delay);
  } else if (argc >= N + 4) {
    auto row_delay = stoi(argv[N + 3]);
    dram = Dram(row_delay);
  } else {
    dram = Dram();
  }

  auto driver = DramDriver(dram, N);
  bool blocking = false;
  vector<Hardware> cores(N);

  if (argc >= N + 6 && stoi(argv[N + 5]) == 1) {
    blocking = true;
    cout << "[+] Executing in BLOCKING MODE" << endl;
  } else {
    cout << "[+] Executing in NON-BLOCKING MODE" << endl;
  }

  for (int i = 0; i < N; i++) {
    cores[i].load_dram_driver(&driver);
    cores[i].load_program(programs[i]);
    cores[i].set_blocking_mode(blocking);
    cores[i].set_id(i);
  }

  Stats stats;

  for (int i = 0; i < M; i++) {
    stats.clock_cycles = i + 1;

    int skip_core = driver.perform_tasks(stats);

    for (auto& core : cores) {
      if (skip_core ==
          core.get_id())  // if a writeback is performed to this core
        continue;         // do not execute anything in this cycle
      try {
        core.start_execution(stats);

      } catch (const std::exception& e) {
        cerr << "[-] A runtime error occured in CORE : " << core.get_id()
             << " - " << e.what() << '\n';
      }
    }
  }
  stats.print_verbose();

  return 0;
}

void print_usage() {
  printf(
      "Usage ./main N M <file1> <file2> ... <fileN> [ROW_ACCESS_DELAY] "
      "[COL_ACCESS_DELAY] [BLOCKING]\n");
}