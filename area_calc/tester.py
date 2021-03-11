#!/usr/bin/python3
from pwn import *
from tqdm import tqdm
import random
import argparse

bits = 6
BOUNDS = []
def set_bounds(b):
  BOUNDS.clear()
  BOUNDS.append(-2**(b-1))
  BOUNDS.append(2**(b-1) - 1)
set_bounds(bits)
epsilon = 1e-2

def get_tolerance(area):
  return epsilon * area/100


cmd = "spim -file main.asm"
start_text = "Please Enter number of points: "
end_text = "The area is : "

def execute_asm(input_pts):
  context.log_level = 'critical'
  n = len(input_pts)
  
  p = process(cmd.split())

  p.recvuntil(start_text)
  p.send(f"{n}\n")
  for i in range(n):
    pt = input_pts[i]

    p.recvline()
    p.send(f"{pt[0]}\n")
    p.recvline()
    p.send(f"{pt[1]}\n")

  p.recvuntil(end_text)
  out = p.recv()
  p.kill()

  context.log_level = 'info'
  return float(out)

def positive_area(x1, y1, x2, y2):
  if y1*y2 > 0 :
    return abs((x2 - x1)*(y1 + y2)/2)
  else:
    m = (x1*y2 - x2*y1)/(y2-y1)
    a = 0
    a += abs((m-x1)*y1/2)
    a += abs((x2-m)*y2/2)
    return a


def compute_area(input_pts):
  area = 0
  prev_pt = None
  for i, pt in enumerate(input_pts):
    if(i != 0):
      area += positive_area(prev_pt[0], prev_pt[1], pt[0], pt[1])
    prev_pt = pt
  return area

def generate_pts(n):
  log.info("Generating points")
  pts = []
  for _ in range(n):
    x = random.randint(BOUNDS[0], BOUNDS[1])
    y = random.randint(BOUNDS[0], BOUNDS[1])
    pts.append((x,y))  
  return pts

def test(n, input_pts=[]):
  assert(n >= 2)

  pts = input_pts
  if len(pts) == 0:
    pts = generate_pts(n)

  pts = sorted(pts, key=lambda x: x[0])

  out = execute_asm(input_pts=pts)
  area = compute_area(pts)

  if abs(out - area) > get_tolerance(area):
    log.failure("Test failed!")
    context.log_level = 'debug'
    log.debug(f"Program returned area: {out}")
    log.debug(f"Expected area: {area}")
    log.debug("Input points were: "+ str(pts))
    context.log_level = 'info'
    return False
  else:
    log.success("Test Passed!")
    return True

def read_input(filename):
  inputs_pts = []

  with open(filename, 'r') as f:
    n = int(f.readline())
    for _ in range(n):
      x = int(f.readline())
      y = int(f.readline())
      inputs_pts.append((x,y))
  return inputs_pts

def parse():
  args = argparse.ArgumentParser()
  args.add_argument("-m", dest="m" ,type=int, help="Number of points in test case", default=5)
  args.add_argument("-n", dest="n" ,type=int, help="Number of test cases to run", default=1)
  args.add_argument("-e", "--epsilon", dest="e", type=float, help="Error tolerance for test in percentage", default=epsilon)
  args.add_argument("-b", "--bits",dest="b" ,type=int, help="Number of bits to be used for the point coordinates", default=bits)
  args.add_argument("-i", "--input", dest="file", type=str, help="Input file path for custom test cases")
  return args.parse_args()

if __name__ == "__main__":

  args = parse()
  set_bounds(args.b)
  epsilon = args.e

  if args.file is not None:
    pts = read_input(args.file)
    test(n=len(pts), input_pts=pts)
  else:
    count = 0
    for i in tqdm(range(args.n)):
      if test(args.m):
        count += 1
    
    log.info(f"Passed {count}/{args.n} test cases!")
