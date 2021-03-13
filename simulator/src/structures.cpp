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

double Stats::get_execution_time() {
  return double(end_time - start_time) / CLOCKS_PER_SEC;
}

void Stats::print_verbose() {
  cout << endl << endl << "    ------ EXECUTION STATISTICS -----" << endl;
  cout << "[$] Frequency of instructions: " << endl;
  for (auto& it : frequency) {
    cout << it.first << " : " << it.second << endl;
  }
  cout << "[$] Clock cycles: " << clock_cycles << endl;
  printf("[$] Processor execution time: %#.7fs\n", get_execution_time());
}