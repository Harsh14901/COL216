#include <hardware.hpp>
using namespace std;

void Hardware::initialize_registers() {
  registers = vector<hd_t>(REGISTER_NUM, 0);
  registers[SP_REG_ID] = hd_t(memory + mem_size - 1);
}

Hardware::Hardware() {
  mem_size = MAX_MEMORY;
  initialize_registers();
}

Hardware::Hardware(vector<Instruction> program) : program(program) {
  auto program_size = program.size() * (BITS / 8);
  mem_size = MAX_MEMORY - program_size;

  assert(
      ("Unable to allocate memory more than " + to_string(mem_size) + " bytes.",
       mem_size <= MAX_MEMORY && mem_size > 0));

  initialize_registers();
  pc = this->program.begin();
}

void Hardware::execute_current() {
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
      sw(pc->arg1, pc->arg2, pc->arg3);
      advance_pc();

      break;
    case Operator::LW:
      lw(pc->arg1, pc->arg2, pc->arg3);
      advance_pc();

      break;

    default:
      break;
  }
}

void Hardware::advance_pc() { pc += 1; }
void Hardware::terminate() { pc = program.end(); }

void Hardware::start_execution() {
  while (pc != program.end()) {
    print_instruction();
    execute_current();
    print_contents();
  }
}

void Hardware::print_instruction() {
  cout << "[#] Executing current instruction - " << pc->raw << endl;
}

void Hardware::print_contents() {
  cout << "[#] Register contents -" << endl;
  auto n = registers.size() / 2;
  for (int i = 0; i < n; i++) {
    printf("%2d : %#010x\t\t%d : %#010x\n", i, registers[i], i + n,
           registers[i + n]);
  }
  cout << "----------------------------------------------" << endl;
}

void Hardware::is_valid_reg(int id) {
  assert(("Invalid Register ID", id < Hardware::REGISTER_NUM && id >= 0));
}

void Hardware::is_valid_reg(int s1, int s2, int s3) {
  is_valid_reg(s1);
  is_valid_reg(s2);
  is_valid_reg(s3);
}

void Hardware::add(int dst, int src1, int src2) {
  is_valid_reg(dst, src1, src2);

  registers[dst] = registers[src1] + registers[src2];
}

void Hardware::addi(int dst, int src, hd_t value) {
  is_valid_reg(dst, src);

  registers[dst] = registers[src] + value;
}

void Hardware::sub(int dst, int src1, int src2) {
  is_valid_reg(dst, src1, src2);

  registers[dst] = registers[src1] - registers[src2];
}

void Hardware::mul(int dst, int src1, int src2) {
  is_valid_reg(dst, src1, src2);

  registers[dst] = registers[src1] * registers[src2];
}

void Hardware::slt(int dst, int src1, int src2) {
  is_valid_reg(dst, src1, src2);

  registers[dst] = (registers[src1] < registers[src2]) ? 1 : 0;
}

void Hardware::j(int jump) {
  assert(("Unable to jump, The address is not word aligned",
          jump % Hardware::BYTES == 0));
  jump /= Hardware::BYTES;

  assert(("Unable to jump to a non instruction", jump < program.size()));

  pc = program.begin() + jump;
}

void Hardware::beq(int src1, int src2, int jump) {
  is_valid_reg(src1, src2);

  if (registers[src1] == registers[src2]) {
    int current_idx = pc - program.begin();
    j(current_idx * Hardware::BYTES + jump);
  }
}

void Hardware::bne(int src1, int src2, int jump) {
  is_valid_reg(src1, src2);

  if (registers[src1] != registers[src2]) {
    int current_idx = pc - program.begin();
    j(current_idx * Hardware::BYTES + jump);
  }
}

void Hardware::is_valid_memory(hd_t* p) {
  assert(("Out of bounds memory access prevented",
          p >= (hd_t*)(&memory) && p <= (hd_t*)(memory + mem_size) - 1));
}

void Hardware::lw(int dst, int offset, int src) {
  is_valid_reg(dst, src);

  hd_t* p = (hd_t*)(registers[src] + offset);
  is_valid_memory(p);

  registers[dst] = *p;
}
void Hardware::sw(int src, int offset, int dst) {
  is_valid_reg(dst, src);

  hd_t* p = (hd_t*)(registers[dst] + offset);
  is_valid_memory(p);

  *p = registers[src];
}
