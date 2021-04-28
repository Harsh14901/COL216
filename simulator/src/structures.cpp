#include "structures.hpp"

using namespace std;

map<string, string> register_map{
    {"$zero", "$0"}, {"$at", "$1"},  {"$v0", "$2"},  {"$v1", "$3"},
    {"$a0", "$4"},   {"$a1", "$5"},  {"$a2", "$6"},  {"$a3", "$7"},
    {"$t0", "$8"},   {"$t1", "$9"},  {"$t2", "$10"}, {"$t3", "$11"},
    {"$t4", "$12"},  {"$t5", "$13"}, {"$t6", "$14"}, {"$t7", "$15"},
    {"$s0", "$16"},  {"$s1", "$17"}, {"$s2", "$18"}, {"$s3", "$19"},
    {"$s4", "$20"},  {"$s5", "$21"}, {"$s6", "$22"}, {"$s7", "$23"},
    {"$t8", "$24"},  {"$t9", "$25"}, {"$k0", "$26"}, {"$k1", "$27"},
    {"$gp", "$28"},  {"$sp", "$29"}, {"$fp", "$30"}, {"$ra", "$31"},
};

map<string, string> reverse_register_map{
    {"$0", "$zero"}, {"$1", "$at"},  {"$2", "$v0"},  {"$3", "$v1"},
    {"$4", "$a0"},   {"$5", "$a1"},  {"$6", "$a2"},  {"$7", "$a3"},
    {"$8", "$t0"},   {"$9", "$t1"},  {"$10", "$t2"}, {"$11", "$t3"},
    {"$12", "$t4"},  {"$13", "$t5"}, {"$14", "$t6"}, {"$15", "$t7"},
    {"$16", "$s0"},  {"$17", "$s1"}, {"$18", "$s2"}, {"$19", "$s3"},
    {"$20", "$s4"},  {"$21", "$s5"}, {"$22", "$s6"}, {"$23", "$s7"},
    {"$24", "$t8"},  {"$25", "$t9"}, {"$26", "$k0"}, {"$27", "$k1"},
    {"$28", "$gp"},  {"$29", "$sp"}, {"$30", "$fp"}, {"$31", "$ra"},
};

vector<string> validTokens = vector<string>(
    {"add", "sub", "mul", "beq", "bne", "slt", "j", "lw", "sw", "addi"});
Operator getOperator(std::string op_str) {
  if (op_str == "add")
    return Operator::ADD;
  else if (op_str == "sub")
    return Operator::SUB;
  else if (op_str == "mul")
    return Operator::MUL;
  else if (op_str == "beq")
    return Operator::BEQ;
  else if (op_str == "bne")
    return Operator::BNE;
  else if (op_str == "slt")
    return Operator::SLT;
  else if (op_str == "j")
    return Operator::J;
  else if (op_str == "lw")
    return Operator::LW;
  else if (op_str == "sw")
    return Operator::SW;
  else if (op_str == "addi")
    return Operator::ADDI;
  else
    throw InvalidInstruction(op_str);
}

void Log::print_registers() {
  cout << "[#] Register contents -" << endl;
  auto n = registers.size() / 2;
  for (int i = 0; i < n; i++) {
    // printf("\t%2d : %#010x\t\t%ld : %#010x\n", i, registers[i], i + n,
    //        registers[i + n]);
    printf("\t%6s : %10d\t\t%6s : %10d\n",
           reverse_register_map["$" + to_string(i)].c_str(), registers[i],
           reverse_register_map["$" + to_string(i + n)].c_str(),
           registers[i + n]);
  }
}

void Log::print_verbose() {
  printf("[$] Processor: %d | CYCLE %d-%d :\n", core, cycle_period.first,
         cycle_period.second);
  printf("[#] Executing current instruction -> %s\n", instruction.c_str());
  print_registers();

  cout << "[#] Row buffer updates:" << endl;
  for (auto& v : rowbuff_updates) {
    printf("\t%d-%ld : %d\n", v.first, v.first + sizeof(hd_t) - 1, v.second);
  }
  cout << "[#] Memory updates:" << endl;
  for (auto& v : memory_updates) {
    printf("\t%d-%ld : %d\n", v.first, v.first + sizeof(hd_t) - 1, v.second);
  }
  cout << "[#] Remarks: " << endl;
  for (auto& v : remarks) {
    cout << "\t" << v << endl;
  }
  cout << "============================================================"
       << endl;
}

void Stats::print_verbose() {
  cout << "---------- Execution logs ----------" << endl;
  for (auto& log : logs) {
    log.print_verbose();
  }
  cout << "---------- Execution statistics ----------" << endl;
  cout << "[#] Clock cycles: " << clock_cycles << endl;
  cout << "[#] Number of row buffer updates: " << rowbuff_update_count << endl;
  cout << "[#] Updated memory : " << endl;
  for (auto& v : updated_memory) {
    printf("\t%d-%ld : %d\n", v.first, v.first + sizeof(hd_t) - 1, v.second);
  }
}
