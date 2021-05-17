#include <filesystem>
#include <iostream>

#include "compiler.hpp"
#include "hardware.hpp"

using namespace std;
namespace fs = std::filesystem;

void print_usage();
int main(int argc, char* argv[]) {
  srand(time(NULL));
  if (argc < 3) {
    print_usage();
    return -1;
  }
  int N = stoi(argv[1]);
  int M = stoi(argv[2]);

  vector<vector<Instruction>> programs;

  const fs::path path(argv[3]);
  std::error_code ec;
  std::cout << "Current path is " << fs::current_path() << '\n';  // (1)
  if (fs::is_directory(path, ec)) {
    set<fs::path> sorted_by_name;

    for (auto& entry : fs::directory_iterator(path))
      sorted_by_name.insert(entry.path());

    for (auto& filename : sorted_by_name) {
      cout << filename.c_str() << endl;
      programs.push_back(compile(filename));
    }

  } else {
    if (argc < N + 3) {
      print_usage();
      return -1;
    }
    for (int i = 0; i < N; i++) {
      string name = argv[i + 3];
      programs.push_back(compile(name));
    }
  }

  int register_write_times[N];
  memset(register_write_times, -1, sizeof(register_write_times));
  Stats stats;
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

  dram.load_stats(&stats);

  auto driver = DramDriver(dram, N, register_write_times);
  driver.load_stats(&stats);
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
    cores[i].load_stats(&stats);
    cores[i].set_blocking_mode(blocking);
    cores[i].set_id(i);
  }

  unordered_set<int> fault_cores;
  vector<int> remaining_cores;

  for (int i = 0; i < M; i++) {
    stats.clock_cycles = i + 1;

    driver.perform_tasks();

    for (auto& core : cores) {
      remaining_cores.push_back(core.get_id());
    }

    int idx = 0;
    while (!remaining_cores.empty()) {
      idx = rand() % remaining_cores.size();
      // idx = 0;
      auto& core = cores[remaining_cores[idx]];
      remaining_cores.erase(remaining_cores.begin() + idx);

      int id = core.get_id();
      if (stats.clock_cycles == register_write_times[id] ||
          fault_cores.find(id) != fault_cores.end())
        continue;  // if a writeback is performed to this core or it is a core
                   // that has undergone a fault do not execute in this cycle

      try {
        core.start_execution();
      } catch (Hardware::Terminated& e) {
        if (driver.is_idle()) {
          fault_cores.insert(id);
        }
      } catch (const std::exception& e) {
        fault_cores.insert(id);
        cerr << "[-] A runtime error occured in CORE : " << id << " - "
             << e.what() << '\n';
      }
    }
    remaining_cores.clear();
    if (fault_cores.size() == N && driver.is_idle()) {
      break;
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