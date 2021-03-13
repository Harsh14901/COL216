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

void Hardware::update_stats(string reg) {
  stats.frequency[reg] += 1;
  stats.clock_cycles += 1;
}

void Hardware::execute_current() {
  switch (pc->op) {
    case Operator::ADD:
      add(pc->arg1, pc->arg2, pc->arg3);
      update_stats("ADD");
      advance_pc();

      break;
    case Operator::ADDI:
      addi(pc->arg1, pc->arg2, pc->arg3);
      update_stats("ADDI");

      advance_pc();

      break;
    case Operator::SUB:
      sub(pc->arg1, pc->arg2, pc->arg3);
      update_stats("SUB");

      advance_pc();

      break;
    case Operator::MUL:
      mul(pc->arg1, pc->arg2, pc->arg3);
      update_stats("MUL");

      advance_pc();

      break;
    case Operator::BEQ:
      beq(pc->arg1, pc->arg2, pc->arg3);
      update_stats("BEQ");

      break;
    case Operator::BNE:
      bne(pc->arg1, pc->arg2, pc->arg3);
      update_stats("BNE");

      break;
    case Operator::SLT:
      slt(pc->arg1, pc->arg2, pc->arg3);
      update_stats("SLT");

      advance_pc();

      break;
    case Operator::J:
      j(pc->arg1);
      update_stats("J");

      break;
    case Operator::SW:
      sw(pc->arg1, pc->arg2, pc->arg3);
      update_stats("SW");

      advance_pc();

      break;
    case Operator::LW:
      lw(pc->arg1, pc->arg2, pc->arg3);
      update_stats("LW");

      advance_pc();

      break;

    default:
      break;
  }
}

void Hardware::advance_pc() { pc += 1; }
void Hardware::terminate() { pc = program.end(); }
Stats Hardware::get_stats() { return stats; }

void Hardware::start_execution() {
  stats.start_time = clock();
  while (pc != program.end()) {
    print_instruction();
    execute_current();
    print_contents();
  }
  stats.end_time = clock();
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

  fstream out_file;
  string file_name = "log";
  out_file.open(file_name, ios::app);
  if (!out_file) {
    cerr << "[-] Error writing to the output file: " << file_name << endl;
  } else {
    for (int i = 0; i < registers.size() - 1; i++) {
      out_file << registers[i] << ", ";
    }
    out_file << registers[registers.size() - 1] << endl;
  }

  out_file.close();
}

void Hardware::is_valid_reg(int id) {
  assert(("Invalid Register ID", id < Hardware::REGISTER_NUM && id >= 0));
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
  assert(("Access Denied: Cannot modify a kernel reserved register: $26",
          dst != 26));
  assert(("Access Denied: Cannot modify a kernel reserved register: $27",
          dst != 27));
  registers[dst] = value;
}

void Hardware::check_overflow(long long val) {
  assert(("Artithmetic overflow occured",
          val <= Hardware::HD_T_MAX && val >= Hardware::HD_T_MIN));
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
    j((current_idx + 1) * Hardware::BYTES + jump);
  }
}

void Hardware::bne(int src1, int src2, int jump) {
  is_valid_reg(src1, src2);

  if (registers[src1] != registers[src2]) {
    int current_idx = pc - program.begin();
    j((current_idx + 1) * Hardware::BYTES + jump);
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

  set_register(dst, *p);
}
void Hardware::sw(int src, int offset, int dst) {
  is_valid_reg(dst, src);

  hd_t* p = (hd_t*)(registers[dst] + offset);
  is_valid_memory(p);

  *p = registers[src];
}
