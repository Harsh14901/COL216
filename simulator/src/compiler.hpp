#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "structures.hpp"

typedef std::pair<bool, int> bip;

static inline void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

static inline void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

static inline void trim(std::string &s) {
  ltrim(s);
  rtrim(s);
}

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

    getline(stream, token, ' ');

    instruction.op = getOperator(token);

    int index = 1;

    while (getline(stream, token, ',')) {
      // std::cout << "this " << token << " is as a token" << std::endl;
      trim(token);
      if (index == 1)
        instruction.arg1 = token;
      else if (index == 2)
        instruction.arg2 = token;
      else if (index == 3)
        instruction.arg3 = token;
      else
        throw InvalidInstruction();
      index++;
    }

    instructions.push_back(instruction);
  }
  // std::cout << instructions.size() << " and " << instructions[1].arg1
  // << std::endl;
  return instructions;
}
bool isNum(std::string s) {
  if (s == "") return false;
  for (char c : s)
    if (!isdigit(c)) return false;
  return true;
}

bip getRegister(std::string reg) {
  bip ans(false, 0);
  if (reg == "") return ans;
  std::string reg_val = reg.substr(1);
  if (!isNum(reg_val)) return ans;
  int val = stoi(reg_val);
  bool start_with_dollar = reg.at(0) == '$';
  bool valid_reg_addr = 0 <= val && val < 32;
  ans = std::make_pair(start_with_dollar && valid_reg_addr, val);
  return ans;
}

std::vector<Instruction> compile(std::string fileName) {
  std::vector<UnprocessedInstruction> instructions =
      parseInstructions(fileName);
  std::vector<Instruction> processedInstructions;
  for (UnprocessedInstruction instr : instructions) {
    bip p1 = getRegister(instr.arg1), p2 = getRegister(instr.arg2),
        p3 = getRegister(instr.arg3);
    // std::cout << "here " << p1.second << " " << p2.second << " " << p3.second
    //           << std::endl;
    switch (instr.op) {
      case Operator::ADD:
      case Operator::SUB:
      case Operator::MUL:
        if (!p1.first || !p2.first || !p3.first) throw InvalidInstruction();
        processedInstructions.push_back(
            Instruction{instr.op, p1.second, p2.second, p3.second});
        break;

      case Operator::ADDI:
      case Operator::SLT:
      case Operator::BEQ:
      case Operator::BNE:
        if (!p1.first || !p2.first || !isNum(instr.arg3))
          throw InvalidInstruction();
        processedInstructions.push_back(
            Instruction{instr.op, p1.second, p2.second, stoi(instr.arg3)});
        break;

      case Operator::LW:
      case Operator::SW: {
        bool b1 = p1.first;
        bool b2 = instr.arg3 == "";
        bool b3 = instr.arg2.at(instr.arg2.size() - 1) == ')';
        int pos = instr.arg2.find('(');
        bool b4 = pos >= 0;
        std::string sub_p1 = instr.arg2.substr(0, pos),
                    sub_p2 =
                        instr.arg2.substr(pos + 1, instr.arg2.size() - pos - 2);
        bip p3 = getRegister(sub_p2);
        bool b5 = isNum(sub_p1);
        bool b6 = p3.first;
        if (!b1 || !b2 || !b3 || !b4 || !b5) throw InvalidInstruction();
        processedInstructions.push_back(
            Instruction{instr.op, p1.second, stoi(sub_p1), p3.second});
        break;
      }

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