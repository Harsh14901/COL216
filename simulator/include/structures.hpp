#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <bits/stdc++.h>

#include <string>
#include <vector>

using namespace std;
typedef int32_t hd_t;

class InvalidInstruction : public std::runtime_error {
 public:
  InvalidInstruction() : std::runtime_error("") {}
  InvalidInstruction(const string& msg) : std::runtime_error(msg) {}
};

class InvalidArgument : public std::runtime_error {
 public:
  InvalidArgument() : std::runtime_error("") {}
  InvalidArgument(const string& msg) : std::runtime_error(msg) {}
};

enum class Operator { ADD, SUB, MUL, BEQ, BNE, SLT, J, LW, SW, ADDI };

extern vector<string> validTokens;

Operator getOperator(string op_str);

struct UnprocessedInstruction {
  Operator op;
  string arg1, arg2, arg3;
  string raw;
};

struct Instruction {
  Operator op;
  int arg1, arg2, arg3;
  string raw;

  string to_string() {
    stringstream ss;
    ss << arg1 << " " << arg2 << " " << arg3;
    return ss.str();
  };
};

struct Branch {
  std::string name;
  int pos;
};

struct Log {
  pair<int, int> cycle_period;
  string instruction = "";
  vector<hd_t> registers;
  map<int, hd_t> memory_updates;
  map<int, hd_t> rowbuff_updates;
  vector<string> remarks;
  int core = -1;

  void print_verbose();
  void print_registers();
};

struct Stats {
  int clock_cycles = 0;
  int rowbuff_update_count = 0;
  int instr_count = 0;
  map<int, hd_t> updated_memory;
  vector<Log> logs;
  void print_verbose();
};

extern map<string, string> reverse_register_map;
extern map<string, string> register_map;

#endif