#include <structures.hpp>

using namespace std;

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
    printf("\t%2d : %#010x\t\t%ld : %#010x\n", i, registers[i], i + n,
           registers[i + n]);
  }
}

void Log::print_verbose() {
  printf("[$] CYCLE %d-%d :\n", cycle_period.first, cycle_period.second);
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