#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "structures.hpp"

typedef std::pair<bool, int> bip;

std::vector<UnprocessedInstruction> parseInstructions(std::string fileName) {
  std::vector<UnprocessedInstruction> instructions;

  std::fstream file;
  file.open(fileName, std::ios::in);
  if (!file.is_open()) {
    std::cout << "File not found... exiting" << std::endl;
    return instructions;
  }

  std::string line;
  while (getline(file, line)) {
    UnprocessedInstruction instruction;

    std::cout << line << std::endl;
    std::stringstream stream(line);

    std::string token;

    stream >> token;

    getline(stream, token, ' ');
    std::cout << "token" << token << std::endl;

    instruction.op = getOperator(token);

    int index = 1;

    while (getline(stream, token, ',')) {
      if (index == 1)
        instruction.arg1 = token;
      else if (index == 2)
        instruction.arg2 = token;
      else if (index == 3)
        instruction.arg3 = token;
      else
        throw InvalidInstruction();
    }

    instructions.push_back(instruction);
  }

  return instructions;
}
bool isNum(std::string s) {
  for (char c : s)
    if (!isdigit(c)) return false;
  return true;
}

bip getRegister(std::string reg) {
  bip ans(0, false);
  std::string reg_val = reg.substr(1, reg.size() - 1);
  if (!isNum(reg_val)) return ans;
  int val = stoi(reg_val);
  bool start_with_dollar = reg.at(0) == '$';
  bool valid_reg_addr = 0 <= val && val <= 32;
  ans = std::make_pair(val, start_with_dollar && valid_reg_addr);
  return ans;
}

std::vector<Instruction> compile(std::string fileName) {
  std::vector<UnprocessedInstruction> instructions =
      parseInstructions(fileName);
  std::vector<Instruction> processedInstructions;
  for (UnprocessedInstruction instr : instructions) {
    bip p1 = getRegister(instr.arg1), p2 = getRegister(instr.arg2),
        p3 = getRegister(instr.arg3);
    switch (instr.op) {
      case Operator::ADD:
      case Operator::SUB:
      case Operator::MUL:
        if (!p1.second || !p2.second || !p3.second) throw InvalidInstruction();
        processedInstructions.push_back(
            Instruction{instr.op, p1.first, p2.first, p3.first});
        break;
      case Operator::ADDI:
      case Operator::SLT:
      case Operator::BEQ:
      case Operator::BNE:
        if (!p1.second || !p2.second || !isNum(instr.arg3))
          throw InvalidInstruction();
        processedInstructions.push_back(
            Instruction{instr.op, p1.first, p2.first, stoi(instr.arg3)});
        break;
      case Operator::LW:
      case Operator::SW:
        // TODO
        processedInstructions.push_back(
            Instruction{instr.op, p1.first, p2.first, 0});
      case Operator::J:
        if (!isNum(instr.arg1) || instr.arg2 != "" || instr.arg3 != "")
          throw InvalidInstruction();
        processedInstructions.push_back(
            Instruction{instr.op, stoi(instr.arg1), 0, 0});
        break;
    }
  }
  return processedInstructions;
}