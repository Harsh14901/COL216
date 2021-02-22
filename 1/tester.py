#!/usr/bin/python3
from pwn import *

cmd = "spim -file main.asm"
start_text = "Please Enter number of points: "
end_text = "The area is : "

def execute_asm(input_pts):
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
  return float(out)

  
print(execute_asm([(10,10), (20,20)]))