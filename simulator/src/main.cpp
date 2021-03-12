#include <compiler.hpp>
#include <hardware.hpp>
#include <iostream>

using namespace std;
int main(int argc, char* argv[]) {
  string name = argv[1];
  cout << "[+] Loading file : " << name << endl;

  vector<Instruction> ins = compile(name);
  cout << "[+] Program compiled successfully !" << endl;
  // for (auto& in : ins) {
  //   cout << in.arg1 << " " << in.arg2 << " " << in.arg3 << endl;
  // }
  cout << "[+] Executing program ..." << endl;
  auto mips = Hardware(ins);
  mips.start_execution();
}