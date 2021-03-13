#include <compiler.hpp>
#include <hardware.hpp>

std::vector<Branch> branches;

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

bool branch_exists(std::string branch_name) {
  for (auto branch : branches) {
    if (branch.name == branch_name) return true;
  }
  return false;
}

std::vector<UnprocessedInstruction> parseInstructions(std::string fileName) {
  std::vector<UnprocessedInstruction> instructions;

  std::fstream file;
  file.open(fileName, std::ios::in);
  if (!file.is_open()) {
    std::cout << "[-] File not found... exiting" << std::endl;
    return instructions;
  }

  std::string line;
  int line_no = -1;
  while (getline(file, line)) {
    trim(line);
    if (line == "") continue;

    line_no++;

    if (line.at(line.size() - 1) == ':') {
      if (line.find(' ') != -1 || line.size() <= 1)
        throw InvalidInstruction(line + " is not an instruction or label");

      std::string branch_name = line.substr(0, line.size() - 1);
      if (isNum(branch_name))
        throw InvalidInstruction(line + " is not a valid label");

      if (!branch_exists(branch_name))
        branches.push_back({branch_name, line_no});
      else
        throw InvalidArgument(branch_name + " already exists");

      line_no--;  // Decrement line_no as the label definition is not present in
                  // the compiled code
      continue;
    }

    UnprocessedInstruction instruction;
    instruction.raw = line;

    std::stringstream stream(line);

    std::string token;

    getline(stream, token, ' ');

    instruction.op = getOperator(token);

    int index = 1;

    while (getline(stream, token, ',')) {
      trim(token);
      if (index == 1)
        instruction.arg1 = token;
      else if (index == 2)
        instruction.arg2 = token;
      else if (index == 3)
        instruction.arg3 = token;
      else
        throw InvalidInstruction(line);
      index++;
    }

    instructions.push_back(instruction);
  }
  // std::cout << instructions.size() << " and " << instructions[1].arg1
  // << std::endl;
  return instructions;
}
bool isPositiveNum(std::string s) {
  for (char c : s)
    if (!isdigit(c)) return false;
  return true;
}

bool isNum(std::string s) {
  if (s == "") return false;
  if (s.at(0) == '-') return isPositiveNum(s.substr(1));
  return isPositiveNum(s);
}

bip isValidNum(std::string s, int max_bytes) {
  bool is_num = isNum(s);
  if (!is_num) return bip(false, 0);

  int limit = 1 << max_bytes - 1;
  int val = stoi(s);
  return {is_num && (-limit <= val) && (val < limit), val};
}

bip getRegister(std::string reg) {
  bip ans(false, 0);
  if (reg == "") return ans;
  std::string reg_val = reg.substr(1);
  if (!isNum(reg_val)) return ans;

  int val;
  try {
    val = stoi(reg_val);
  } catch (std::invalid_argument e) {
    throw InvalidArgument(reg_val + " is not an integer");
  }
  bool start_with_dollar = reg.at(0) == '$';
  bool valid_reg_addr = 0 <= val && val < 32;
  ans = std::make_pair(start_with_dollar && valid_reg_addr, val);
  return ans;
}

int get_label_addr(std::string label) {
  for (auto branch : branches) {
    if (branch.name == label) return branch.pos * Hardware::BYTES;
  }
  throw InvalidArgument("Cannot jump to non-existent label: " + label);
}

std::vector<Instruction> compile(std::string fileName) {
  auto instructions = parseInstructions(fileName);
  std::vector<Instruction> processedInstructions;

  int line_no = -1;
  for (auto &instr : instructions) {
    line_no++;

    bip p1 = getRegister(instr.arg1), p2 = getRegister(instr.arg2),
        p3 = getRegister(instr.arg3);

    switch (instr.op) {
      case Operator::ADD:
      case Operator::SUB:
      case Operator::MUL:
      case Operator::SLT: {
        if (!p1.first || !p2.first || !p3.first)
          throw InvalidInstruction(instr.raw);
        processedInstructions.push_back(
            Instruction{instr.op, p1.second, p2.second, p3.second, instr.raw});
        break;
      }

      case Operator::BEQ:
      case Operator::BNE: {
        auto [valid, arg3_int] =
            isValidNum(instr.arg3, Hardware::INT_TYPE_I_SIZE);
        if (!valid) {
          int abs_pos = get_label_addr(instr.arg3);
          arg3_int = abs_pos - (line_no + 1) * Hardware::BYTES;
        }
        if (!p1.first || !p2.first) throw InvalidInstruction(instr.raw);
        processedInstructions.push_back(
            Instruction{instr.op, p1.second, p2.second, arg3_int, instr.raw});
        break;
      }

      case Operator::ADDI: {
        auto [arg3_is_int, arg3_int] =
            isValidNum(instr.arg3, Hardware::INT_TYPE_I_SIZE);

        if (!p1.first || !p2.first || !arg3_is_int)
          throw InvalidInstruction(instr.raw);

        processedInstructions.push_back(
            Instruction{instr.op, p1.second, p2.second, arg3_int, instr.raw});
        break;
      }

      case Operator::LW:
      case Operator::SW: {
        bool extra_args_present = (instr.arg3 == "");
        bool offset_present_1 = instr.arg2.at(instr.arg2.size() - 1) == ')';

        int pos = instr.arg2.find('(');
        bool offset_present_2 = pos >= 0;

        std::string sub_p1 = instr.arg2.substr(0, pos),
                    sub_p2 =
                        instr.arg2.substr(pos + 1, instr.arg2.size() - pos - 2);

        auto [arg1_is_register, arg1_register] = p1;
        auto [arg2_is_offset, arg2_offset] =
            isValidNum(sub_p1, Hardware::INT_TYPE_R_SIZE);
        auto [arg3_is_register, arg3_register] = getRegister(sub_p2);

        if (!arg1_is_register || !extra_args_present || !offset_present_1 ||
            !offset_present_2 || !arg2_is_offset || !arg3_is_register)
          throw InvalidInstruction(instr.raw);

        processedInstructions.push_back(Instruction{
            instr.op, arg1_register, arg2_offset, arg3_register, instr.raw});
        break;
      }

      case Operator::J: {
        auto [arg1_is_address, arg1_address] =
            isValidNum(instr.arg1, Hardware::INT_TYPE_J_SIZE);

        if (!arg1_is_address) {
          arg1_address = get_label_addr(instr.arg1);
        }

        if (instr.arg2 != "" || instr.arg3 != "")
          throw InvalidInstruction(instr.raw);
        processedInstructions.push_back(
            Instruction{instr.op, arg1_address, 0, 0, instr.raw});
        break;
      }
    }
  }
  return processedInstructions;
}
