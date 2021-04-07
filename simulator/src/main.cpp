#include "compiler.hpp"
#include "hardware.hpp"
#include <iostream>

using namespace std;
int main(int argc, char* argv[]) {
  string name = argv[1];
  cout << "[+] Compiling file : " << name << endl;

  vector<Instruction> ins = compile(name);
  cout << "[+] Program compiled successfully !" << endl;
  cout << "[+] Executing program ..." << endl;

  Hardware mips;

  if (argc >= 4) {
    auto row_delay = stoi(argv[2]);
    auto col_delay = stoi(argv[3]);
    mips = Hardware(ins, row_delay, col_delay);
  } else if (argc >= 3) {
    auto row_delay = stoi(argv[2]);
    mips = Hardware(ins, row_delay);

  } else {
    mips = Hardware(ins);
  }

  if (argc >= 5 && stoi(argv[4]) == 1) {
    cout << "[+] Executing in BLOCKING MODE" << endl;
    mips.set_blocking_mode(true);
  } else {
    cout << "[+] Executing in NON-BLOCKING MODE" << endl;
  }

  Stats stats;

  try {
    mips.start_execution(stats);
    stats.print_verbose();
    cout << "[+] Program terminated" << endl;

  } catch (const std::exception& e) {
    stats.print_verbose();
    cerr << "[-] A runtime error occured: " << e.what() << '\n';
    return -1;
  }

  return 0;
}
