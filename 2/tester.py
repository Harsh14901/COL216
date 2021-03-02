from pwn import *
from random import *
from tqdm import tqdm
import matplotlib.pyplot as plt
import time
import random
import argparse
import sys

cmd = "spim -f main.asm"
start_text = "Enter the postfix expression:\n"
valid_text = "The value is : "
invalid_text = "Fatal Error: Invalid postfix expression.\n"

class WrongInputException(Exception):
	pass

def execute_asm(expr):
	valid = False
	context.log_level = 'critical'

	p = process(cmd.split())
	p.recvuntil(start_text)
	p.send(expr)

	t1 = time.time()

	out = p.recv().decode()
	t2 = time.time()

	if out.startswith(valid_text):
		valid = True
		out = out[len(valid_text):]
	if out.startswith(invalid_text):
		out = out[len(invalid_text):]

	p.kill()
	context.log_level = 'info'

	return out,valid,t2-t1


def evaluate_postfix(expr):
	stack = []
	val = 0

	for char in expr:
		if char == '\n' : 
			break
		if is_operand(char):
			stack.append(int(char))
		else:
			if len(stack) <= 0:
				abort()
			a1, a2 = stack.pop(), stack.pop()
			if char == "+":
				val = a1 + a2
			elif char == "*":
				val = a1 * a2
			elif char == "-":
				val = a2 - a1
			else:
				abort()
			stack.append(val)

	if len(stack) != 1:
		abort()
	return val

def is_operand(char):
	key = ord(char)
	return 48 <= key <= 57


def abort():
	raise WrongInputException()


def parse():
	args = argparse.ArgumentParser()

	args.add_argument("-i", "--input", dest="file", type=str, help="Input file path for custom test cases")
	args.add_argument("-n", dest="n" ,type=int, help="Number of automated test cases to run", default=1)
	args.add_argument("-k", dest="k" ,type=int, help="Number of operands", default=randint(1,30))
	args.add_argument("-m", "--manual", dest="manual", help="Enter test cases manually in STDIN",action='store_true')
	args.add_argument("-t", "--timing", dest="timing", help="Perform timing analysis and plot graphs",action='store_true')

	
	return args.parse_args()


def get_automated_postfix(k):
	from random import random,randint
	s = 0
	num_op = 0
	line = []
	oper = ['+','-','*']
	while num_op != k or s != 1:
		if random() > 0.5 and num_op < k:
			line.append(str(randint(0,9)))
			s += 1
			num_op += 1
		else:
			if s <= 1: continue
			index = randint(0,2)
			line.append(oper[index])
			s -= 1
	return ''.join(line)

			

def check_validity(line):
	testcase = line[:-1]
	
	log.info(f"Executing test case - {testcase}")
	val,valid,_ = execute_asm(line)
	context.log_level = 'debug'
	try:
		expected_val = evaluate_postfix(line)
		val = int(val)
		if val == expected_val:
			log.success("Test Passed")
			log.info(f"Expected value: {expected_val}, Computed value: {val}")
			log.indented("="*100)
			return True
		else:
			log.failure("Test Failed")
			log.debug(f"Expected value: {expected_val}, Computed value: {val}")
			log.indented("="*100)
			return False
	except WrongInputException:
		context.log_level = 'error'
		detail = "Error" if not valid else val
		msg = "Correct" if not valid else "Incorrect"
		log.error(f"{msg} --- Expected Error, Got : {detail}")
		log.indented("="*100)
		return False
	


if __name__ == "__main__":
	pass
	args = parse()
	if args.file :
		path = args.file
		with open(path) as f:
			for line in f.readlines():
				check_validity(line)
	elif args.manual:
		line = input()
		check_validity(line)
	elif args.timing:
		times = []
		value = []
		for k in range(1,50):
			line = get_automated_postfix(k)
			val,valid,interval = execute_asm(line+'\n')
			# print(valid,line)
			if valid:
				times.append(interval)
				value.append(k)
		# print(value)
		# print(times)
		plt.plot(value,times)
		plt.show()
	elif args.n:
		count = 0
		for _ in range(args.n):
			line = get_automated_postfix(args.k)
			if check_validity(line+'\n'):
				count += 1
		log.info(f"Passed {count}/{args.n} test cases!")
	else:
		pass
