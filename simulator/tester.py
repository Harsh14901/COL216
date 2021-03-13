import os
from pwn import *
from random import *
import numpy as np
import pandas as pd
import random
import argparse
import subprocess
import re

valid_regs = list(range(2, 26))
valid_regs.remove(5)
valid_regs.remove(6)
BOUNDS = (-(2 ** 15), 2 ** 15 - 1)
REG_NUM = 32
LOG_FILE = "./log"
prompt = "(spim) "


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
    log.progress("Fetching registers from SPIM")
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


def execute_program(args, filename):
    log.progress("Extracting program output")
    p = subprocess.Popen(
        f"make run file={filename}".split(),
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    p.wait()
    program_regs = pd.read_csv(
        "log", delimiter=",", names=list(range(REG_NUM)), dtype=int
    ).to_numpy()

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
    if valid:
        log.success("Test case passed!")
    else:
        log.failure("Test failed!")
    log.indented("-" * 50)
    return valid


def cleanup(filename):
    os.remove(LOG_FILE)
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

        program_regs = execute_program(args, out_file)
        spim_regs = execute_spim(out_asm)
        cleanup(out_asm)

        return check_valid(program_regs, spim_regs)

    except Exception as e:
        cleanup(out_asm)
        print(e)
    return False


if __name__ == "__main__":
    args = parse()
    count = 0
    for i in range(args.n):
        if run(args, i):
            count += 1
        asm_statements.clear()
    log.info(f"Passed {count}/{args.n} test cases")
