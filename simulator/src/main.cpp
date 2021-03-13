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
  mips.start_execution();
  cout << "[+] Program terminated" << endl;
  auto stats = mips.get_stats();
  stats.print_verbose();
  return 0;
}