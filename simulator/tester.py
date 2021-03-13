import os
from pwn import *
from random import *
import numpy as np
import pandas as pd
import random
import argparse
import re

valid_regs = list(range(0, 26))
valid_regs.remove(0)
valid_regs.remove(1)
valid_regs.remove(5)
valid_regs.remove(6)
BOUNDS = (-(2 ** 15), 2 ** 15 - 1)
REG_NUM = 32
LOG_FILE = "./log"
prompt = "(spim) "
program_path = "output/main"


instructions = (["add", "sub", "mul", "slt"], ["addi"], ["lw", "sw"])

asm_statements = []


def get_regs(num):
    regs = ["$" + str(random.sample(valid_regs, 1)[0]) for i in range(num)]
    return ",".join(regs)


def get_bounded_val():
    return random.randint(*BOUNDS)


def get_instr(category=0):
    instrs = instructions[category]
    op = random.sample(instrs, 1)[0]

    if category == 0:
        return f"{op} {get_regs(3)}"
    elif category == 1:
        return f"{op} {get_regs(2)},{get_bounded_val()}"
    # elif category == 2:
    #     return f"{op} {get_regs(1)},{get_bounded_val()}({get_regs(1)})"


def parse():
    args = argparse.ArgumentParser()

    args.add_argument(
        "-i",
        "--input",
        dest="input",
        type=str,
        help="Input file path for custom test cases",
    )
    args.add_argument(
        "-o",
        "--output",
        dest="output",
        type=str,
        help="Output directory for the test case(s) generated",
        default="./input",
    )
    args.add_argument(
        "-m",
        dest="m",
        type=int,
        help="Number of instructions to run per test case",
        default=10,
    )
    args.add_argument(
        "-n",
        dest="n",
        type=int,
        help="Number of test cases",
        default=1,
    )
    args.add_argument(
        "-v",
        "--verbose",
        dest="verbose",
        action="store_true",
        help="Display the instructions generated verbosely",
        default=False,
    )

    return args.parse_args()


def query_spim_regs(p):
    reg_vals = []
    for i in range(REG_NUM):
        p.send(f"print ${i}\n")
        text = p.recvuntil(prompt)
        val = int(re.match(b".*\((.*)\).*", text).groups()[0])
        reg_vals.append(val)
    return reg_vals


def execute_spim(filename):
    log.info("Fetching registers from SPIM")
    output = []
    global asm_statements
    context.log_level = "critical"
    p = process("spim", stdin=PTY)
    p.recvuntil(prompt)
    p.send(f'load "{filename}"\n')
    p.recvuntil(prompt)
    p.send(f"breakpoint main\n")
    p.recvuntil(prompt)
    p.send(f"run main\n")
    p.recvuntil(prompt)

    for i in range(len(asm_statements)):
        p.send(f"step\n")
        p.recvuntil(prompt)
        output.append(query_spim_regs(p))

    p.kill()

    context.log_level = "info"
    return np.array(output)


def build_program():
    context.log_level = "critical"
    p = process(f"make".split())
    p.wait()
    context.log_level = "info"


def execute_program(args, filename):
    log.info("Extracting program output for : " + filename)

    context.log_level = "critical"
    p = process(f"{program_path} {filename}".split())
    text = p.recvall()
    context.log_level = "info"

    try:
        program_regs = pd.read_csv(
            "log", delimiter=",", names=list(range(REG_NUM)), dtype=int
        ).to_numpy()
    except Exception as e:
        log.failure("Program returned error: " + text.decode())
        raise e

    # print(program_regs.shape)
    assert program_regs.shape == (args.m, REG_NUM)
    return program_regs


def check_valid(program_regs, spim_regs):
    global asm_statements
    valid = True
    for i in range(len(asm_statements)):
        for reg in valid_regs:
            if program_regs[i][reg] != spim_regs[i][reg]:
                valid = False
                log.critical(
                    f'Test case failed for instruction ({i+1}/{len(asm_statements)}) "{asm_statements[i]}" for register ${reg}: C++({program_regs[i][reg]}), spim({spim_regs[i][reg]})'
                )

    return valid


def cleanup(filename):
    try:
        os.remove(LOG_FILE)
    except:
        pass

    try:
        os.remove(filename)
        log.success("ASM file cleared successfully")
    except:
        log.failure("Could not delete file :" + filename)


def run(args, idx):
    global asm_statements
    try:
        out_file = args.output + f"/test{idx}.in"
        out_asm = out_file + ".asm"
        log.info(f"Generating test case ({idx + 1}/{args.n})")

        if args.input:
            asm_statements = open(args.input, "r").read().split("\n")
        else:
            with open(out_file, "w") as f:
                for i in range(args.m):
                    instr = get_instr(random.randint(0, 1))
                    asm_statements.append(instr)
                    f.writelines(instr + "\n")

        with open(out_asm, "w") as f:
            f.writelines(".text\n.globl main\nmain:\n")
            f.writelines("\n".join(asm_statements) + "\n")

        if args.verbose:
            log.indented("\n".join(asm_statements))
        program_regs = execute_program(args, out_file)
        spim_regs = execute_spim(out_asm)
        cleanup(out_asm)

        valid = check_valid(program_regs, spim_regs)
        if valid:
            log.success("Test Passed")
        else:
            log.failure("Test Failed")
        log.indented("-" * 50)
        return valid

    except Exception as e:
        cleanup(out_asm)
        log.debug(str(e))
        log.failure("Test Failed")
        log.indented("-" * 50)
        return False


if __name__ == "__main__":
    args = parse()
    count = 0
    log.progress("Building program")
    build_program()
    for i in range(args.n):
        if run(args, i):
            count += 1
        asm_statements.clear()
    log.info(f"Passed {count}/{args.n} test cases")
