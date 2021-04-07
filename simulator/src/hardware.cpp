#include "hardware.hpp"
using namespace std;

void Hardware::initialize_registers() {
  registers = vector<hd_t>(REGISTER_NUM, 0);
  registers[SP_REG_ID] = hd_t(Dram::MAX_MEMORY);
}

Hardware::Hardware() {
  initialize_registers();
  dram = Dram();
}

Hardware::Hardware(vector<Instruction> program, int row_delay, int col_delay)
    : program(program) {
  auto program_size = program.size() * (BITS / 8);

  if (!(program_size <= Dram::MAX_MEMORY && program_size > 0)) {
    throw runtime_error("Unable to allocate memory more than " +
                        to_string(program_size) + " bytes.");
  }

  initialize_registers();
  pc = this->program.begin();

  dram = Dram(row_delay, col_delay);
}

void Hardware::set_blocking_mode(bool block) { blocking = block; }

void Hardware::execute_current(Stats& stats) {
  switch (pc->op) {
    case Operator::ADD:
      add(pc->arg1, pc->arg2, pc->arg3);
      advance_pc();

      break;
    case Operator::ADDI:
      addi(pc->arg1, pc->arg2, pc->arg3);

      advance_pc();

      break;
    case Operator::SUB:
      sub(pc->arg1, pc->arg2, pc->arg3);

      advance_pc();

      break;
    case Operator::MUL:
      mul(pc->arg1, pc->arg2, pc->arg3);

      advance_pc();

      break;
    case Operator::BEQ:
      beq(pc->arg1, pc->arg2, pc->arg3);

      advance_pc();

      break;
    case Operator::BNE:
      bne(pc->arg1, pc->arg2, pc->arg3);

      advance_pc();

      break;
    case Operator::SLT:
      slt(pc->arg1, pc->arg2, pc->arg3);

      advance_pc();

      break;
    case Operator::J:
      j(pc->arg1);

      break;
    case Operator::SW:
      sw(pc->arg1, pc->arg2, pc->arg3, stats);

      advance_pc();

      break;
    case Operator::LW:
      lw(pc->arg1, pc->arg2, pc->arg3, stats);

      advance_pc();

      break;

    default:
      break;
  }
}

void Hardware::check_blocking(Stats& stats) {
  auto block = [&]() {
    auto log = Log{};
    log.cycle_period = make_pair(stats.clock_cycles + 1, dram.busy_until);
    log.remarks.push_back("Blocking on DRAM call to finish");
    log.instruction = pc->raw;
    stats.logs.push_back(log);
    stats.clock_cycles = dram.busy_until;
    if (this->blocking_reg != -1) {
      this->set_register(this->blocking_reg, this->pending_value);
      this->blocking_reg = -1;
    }
  };

  if (dram.busy_until <= stats.clock_cycles) {
    return;
  }
  if (this->blocking) {
    block();
    return;
  }

  if (pc->op == Operator::LW || pc->op == Operator::SW) {
    block();
  } else if (blocking_reg != -1) {
    switch (pc->op) {
      case Operator::ADD:
      case Operator::SUB:
      case Operator::MUL:
      case Operator::SLT:
        if (pc->arg1 == blocking_reg || pc->arg2 == blocking_reg ||
            pc->arg3 == blocking_reg) {
          block();
        }
        break;
      case Operator::ADDI:
      case Operator::BEQ:
      case Operator::BNE:
        if (pc->arg1 == blocking_reg || pc->arg2 == blocking_reg) {
          block();
        }
        break;
      default:
        return;
    }
  }
}

void Hardware::advance_pc() { pc += 1; }
void Hardware::terminate() { pc = program.end(); }

void Hardware::start_execution(Stats& stats) {
  while (pc != program.end()) {
    check_blocking(stats);

    stats.clock_cycles += 1;
    stats.logs.push_back(Log{});

    auto& log = stats.logs.back();
    log.cycle_period = make_pair(stats.clock_cycles, stats.clock_cycles);
    log.instruction = pc->raw;

    auto instr = pc->op;

    execute_current(stats);
    if (instr != Operator::LW && instr != Operator::SW) {
      log.registers = registers;
    }
  }
  stats.clock_cycles = max(stats.clock_cycles, dram.busy_until);
}

void Hardware::is_valid_reg(int id) {
  if (!(id < Hardware::REGISTER_NUM && id >= 0)) {
    throw runtime_error("Invalid Register ID");
  }
}

void Hardware::is_valid_reg(int s1, int s2, int s3) {
  is_valid_reg(s1);
  is_valid_reg(s2);
  is_valid_reg(s3);
}
void Hardware::set_register(int dst, hd_t value) {
  is_valid_reg(dst);
  if (dst == 0) {
    cerr << "[-] Warning: Modifying the $0 register has no effect" << endl;
    return;
  }
  if (dst == 26 || dst == 27) {
    throw runtime_error(
        "Access Denied: Cannot modify a kernel reserved register: $" +
        to_string(dst));
  }
  registers[dst] = value;
}

void Hardware::check_overflow(long long val) {
  if (!(val <= Hardware::HD_T_MAX && val >= Hardware::HD_T_MIN)) {
    throw runtime_error("Artithmetic overflow occured");
  }
}

void Hardware::add(int dst, int src1, int src2) {
  is_valid_reg(dst, src1, src2);
  check_overflow((long long)(registers[src1]) + (long long)(registers[src2]));

  set_register(dst, registers[src1] + registers[src2]);
}

void Hardware::addi(int dst, int src, hd_t value) {
  is_valid_reg(dst, src);
  check_overflow((long long)(registers[src]) + (long long)(value));

  set_register(dst, registers[src] + value);
}

void Hardware::sub(int dst, int src1, int src2) {
  is_valid_reg(dst, src1, src2);
  check_overflow((long long)(registers[src1]) - (long long)(registers[src2]));

  set_register(dst, registers[src1] - registers[src2]);
}

void Hardware::mul(int dst, int src1, int src2) {
  is_valid_reg(dst, src1, src2);

  set_register(dst, registers[src1] * registers[src2]);
}

void Hardware::slt(int dst, int src1, int src2) {
  is_valid_reg(dst, src1, src2);

  set_register(dst, (registers[src1] < registers[src2]) ? 1 : 0);
}

void Hardware::j(int jump) {
  if (!(jump % Hardware::BYTES == 0)) {
    throw runtime_error("Unable to jump, The address is not word aligned : " +
                        to_string(jump));
  }
  jump /= Hardware::BYTES;

  if (!(jump < program.size())) {
    throw runtime_error("Unable to jump to a non instruction: " +
                        to_string(jump));
  }

  pc = program.begin() + jump;
}

void Hardware::beq(int src1, int src2, int jump) {
  is_valid_reg(src1, src2);
  if (registers[src1] == registers[src2]) {
    int current_idx = pc - program.begin();
    j((current_idx)*Hardware::BYTES + jump);
  }
}

void Hardware::bne(int src1, int src2, int jump) {
  is_valid_reg(src1, src2);

  if (registers[src1] != registers[src2]) {
    int current_idx = pc - program.begin();
    j((current_idx)*Hardware::BYTES + jump);
  }
}

void Hardware::lw(int dst, int offset, int src, Stats& stats) {
  is_valid_reg(dst, src);

  int p = registers[src] + offset;

  auto clock_time = stats.clock_cycles;
  stats.logs.back().remarks.push_back("DRAM request issued");

  blocking_reg = dst;
  pending_value = dram.get_mem_word(p, stats);

  stats.logs.back().registers = registers;
  stats.logs.back().registers[blocking_reg] = pending_value;
  stats.clock_cycles = clock_time;
}
void Hardware::sw(int src, int offset, int dst, Stats& stats) {
  is_valid_reg(dst, src);

  int p = registers[dst] + offset;

  auto clock_time = stats.clock_cycles;
  stats.logs.back().remarks.push_back("DRAM request issued");

  dram.set_mem_word(p, registers[src], stats);
  stats.clock_cycles = clock_time;
}
