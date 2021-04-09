#include "hardware.hpp"
using namespace std;

void Hardware::initialize_registers() {
  registers = vector<hd_t>(REGISTER_NUM, 0);
  registers[SP_REG_ID] = hd_t(Dram::MAX_MEMORY);

  for (int i = 0; i < 32; i++) {
    blocking_registers[i] = 0;
  }
}

Hardware::Hardware(DramDriver dram_driver) : dram_driver(dram_driver) {
  initialize_registers();
}

Hardware::Hardware(DramDriver dram_driver, vector<Instruction> program)
    : program(program), dram_driver(dram_driver) {
  auto program_size = program.size() * (BITS / 8);

  if (!(program_size <= Dram::MAX_MEMORY && program_size > 0)) {
    throw runtime_error("Unable to allocate memory more than " +
                        to_string(program_size) + " bytes.");
  }

  initialize_registers();
  pc = this->program.begin();
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

vector<int> Hardware::get_blocked_registers(Stats& stats) {
  vector<int> current_blocked_registers;
  switch (pc->op) {
    case Operator::ADD:
    case Operator::SUB:
    case Operator::MUL:
    case Operator::SLT:
      current_blocked_registers = {pc->arg1, pc->arg2, pc->arg3};
      break;
    case Operator::ADDI:
    case Operator::BEQ:
    case Operator::BNE:
      current_blocked_registers = {pc->arg1, pc->arg2};
      break;
    case Operator::LW:
      current_blocked_registers = {pc->arg3};
      break;
    case Operator::SW:
      current_blocked_registers = {pc->arg1, pc->arg3};
      break;
  }
  return current_blocked_registers;
}

void Hardware::advance_pc() { pc += 1; }
void Hardware::terminate() { pc = program.end(); }

void Hardware::start_execution(Stats& stats) {
  while (pc != program.end() || dram_driver.req_queue_not_empty()) {
    vector<int> blocked_regs = get_blocked_registers(stats);

    stats.clock_cycles += 1;

    dram_driver.execute_current_in_queue(stats, blocked_regs);

    bool blocking_instr = false;
    for (int reg : blocked_regs) {
      if (dram_driver.is_blocking_reg(reg)) {
        blocking_instr = true;
        break;
      }
    }

    if (blocking_instr) continue;

    if (pc != program.end()) {
      stats.logs.push_back(Log{});
      auto& log = stats.logs.back();
      log.cycle_period = make_pair(stats.clock_cycles, stats.clock_cycles);
      log.instruction = pc->raw;

      auto instr = pc->op;

      execute_current(stats);
      if (instr != Operator::LW && instr != Operator::SW) {
        log.registers = registers;
      }
    } else if (!dram_driver.req_queue_not_empty()) {
      stats.clock_cycles--;
    }
  }
  stats.logs.back().registers = registers;
  // dram_driver.execute_all(stats);
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

  // blocking_reg = dst;
  blocking_registers[dst]++;
  // pending_value = dram.get_mem_word(p, stats);

  // stats.logs.back().registers = registers;
  // stats.logs.back().registers[blocking_reg] = pending_value;
  dram_driver.issue_read(p, &registers[dst], stats, dst);
  stats.clock_cycles = clock_time;
}
void Hardware::sw(int src, int offset, int dst, Stats& stats) {
  is_valid_reg(dst, src);

  int p = registers[dst] + offset;

  auto clock_time = stats.clock_cycles;
  stats.logs.back().remarks.push_back("DRAM request issued");

  dram_driver.issue_write(p, registers[src], stats);
  // dram.set_mem_word(p, registers[src], stats);
  stats.clock_cycles = clock_time;
}
