#ifndef COMPILER_H
#define COMPILER_H

#include <bits/stdc++.h>

#include "hardware.hpp"

typedef std::pair<bool, int> bip;

static inline void ltrim(std::string &s);
static inline void rtrim(std::string &s);
static inline void trim(std::string &s);

bool isPositiveNum(std::string s);
bool isNum(std::string s);
bip isValidNum(std::string s, int max_bytes);

bip getRegister(std::string reg);
int get_label_addr(std::string label);

std::vector<UnprocessedInstruction> parseInstructions(std::string fileName);
std::vector<Instruction> compile(std::string fileName);

#endif
