#include <hardware.hpp>
using namespace std;

Hardware::Hardware() {
  registers = vector<hd_t>(REGISTER_NUM, 0);
  // memory = vector<hd_t>(MAX_MEMORY, 0);
  memory[MAX_MEMORY] = {0};
  registers[SP_REG_ID] = hd_t(&memory);
}

Hardware::Hardware(vector<Instruction> program)
    : program(program), pc(program.begin()) {
  auto program_size = program.size() * (BITS / 8);
  auto mem_size = MAX_MEMORY - program_size;

  assert(mem_size <= MAX_MEMORY && mem_size > 0);

  registers = vector<hd_t>(REGISTER_NUM, 0);

  memory[mem_size] = {0};
  registers[SP_REG_ID] = hd_t(&memory);
}

void Hardware::execute_current() {
  switch (pc->op) {
    case Operator::ADD:
      add(pc->arg1, pc->arg2, pc->arg3);
      break;
    case Operator::ADDI:
      addi(pc->arg1, pc->arg2, pc->arg3);
      break;
    case Operator::SUB:
      sub(pc->arg1, pc->arg2, pc->arg3);
      break;
    case Operator::MUL:
      mul(pc->arg1, pc->arg2, pc->arg3);
      break;
    case Operator::BEQ:
      beq(pc->arg1, pc->arg2, pc->arg3);
      break;
    case Operator::BNE:
      bne(pc->arg1, pc->arg2, pc->arg3);
      break;
    case Operator::SLT:
      slt(pc->arg1, pc->arg2, pc->arg3);
      break;
    case Operator::J:
      j(pc->arg1);
      break;
    case Operator::SW:
      sw(pc->arg1, pc->arg2, pc->arg3);
      break;
    case Operator::LW:
      lw(pc->arg1, pc->arg2, pc->arg3);
      break;

    default:
      break;
  }

  if (pc->op != Operator::J) {
    advance_pc();
  }
}

void Hardware::advance_pc() { pc += 1; }
void Hardware::terminate() { pc = program.end(); }

void Hardware::start_execution() {
  while (pc != program.end()) {
    execute_current();
    print_contents();
  }
}

void Hardware::print_contents() {
  // cout << "[#] Current instruction - " << endl;
  // cout << validTokens[int(pc->op)] << " " << pc->arg1 << ", " << pc->arg2
  //      << ", " << pc->arg3 << endl;

  cout << "[#] Register contents -" << endl;
  for (int i = 0; i < registers.size(); i++) {
    cout << i << " : " << registers[i] << endl;
  }
}

void Hardware::is_valid_reg(int id) {
  assert(id < Hardware::REGISTER_NUM && id >= 0);
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
  assert(jump % Hardware::BYTES == 0);
  jump /= Hardware::BYTES;

  assert(jump < program.size());

  pc = program.begin() + jump;
}

void Hardware::beq(int src1, int src2, int jump) {
  is_valid_reg(src1, src2);

  if (registers[src1] == registers[src2]) {
    int current_idx = pc - program.begin();
    j(current_idx + jump);
  }
}

void Hardware::bne(int src1, int src2, int jump) {
  is_valid_reg(src1, src2);

  if (registers[src1] != registers[src2]) {
    int current_idx = pc - program.begin();
    j(current_idx + jump);
  }
}

void Hardware::is_valid_memory(hd_t* p) {
  assert(p >= (hd_t*)(&memory) &&
         p <= (hd_t*)(memory + (sizeof(memory) / sizeof(memory[0])) - 1));
}

void Hardware::lw(int dst, int src, int offset) {
  is_valid_reg(dst, src);

  hd_t* p = (hd_t*)(registers[src] - offset);
  is_valid_memory(p);

  registers[dst] = *p;
}
void Hardware::sw(int src, int dst, int offset) {
  is_valid_reg(dst, src);

  hd_t* p = (hd_t*)(registers[dst] - offset);
  is_valid_memory(p);

  *p = registers[src];
}
