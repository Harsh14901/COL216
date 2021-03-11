#include <string>
#include <vector>

class InvalidInstruction : public std::exception {};

enum class Operator { ADD, SUB, MUL, BEQ, BNE, SLT, J, LW, SW, ADDI };

std::vector<std::string> validTokens{"add", "sub", "mul", "beq", "bne",
                                     "slt", "j",   "lw",  "sw",  "addi"};

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
    throw InvalidInstruction();
}

struct UnprocessedInstruction {
  Operator op;
  std::string arg1, arg2, arg3;
};

struct Instruction {
  Operator op;
  int arg1, arg2, arg3;
};
