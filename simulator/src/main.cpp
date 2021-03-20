#include <compiler.hpp>
#include <hardware.hpp>
#include <iostream>

using namespace std;
int main(int argc, char* argv[]) {
  string name = argv[1];
  cout << "[+] Compiling file : " << name << endl;

  vector<Instruction> ins = compile(name);
  cout << "[+] Program compiled successfully !" << endl;
  cout << "[+] Executing program ..." << endl;
  auto mips = Hardware(ins);

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