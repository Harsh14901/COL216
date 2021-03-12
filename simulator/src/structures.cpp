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