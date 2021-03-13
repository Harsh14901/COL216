#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <bits/stdc++.h>

#include <string>
#include <vector>

using namespace std;
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

struct Stats {
  unordered_map<string, int> frequency;
  int clock_cycles = 0;
  clock_t start_time;
  clock_t end_time;
  double get_execution_time();
  void print_verbose();
};

#endif