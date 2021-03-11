#include <iostream>
#include "compiler.hpp"

int main(int argc, char *argv[])
{
	std::string name = argv[1];
	std::cout << name << std::endl;

	std::vector<Instruction> ins = compile(name);
	for (Instruction in : ins)
	{
		std::cout << " " << in.arg1 << " " << in.arg2 << " " << in.arg3 << std::endl;
	}
}