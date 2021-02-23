# Area calculator

This program calculates the area on a 2-D xy plane
## Testing

We use `spim` to emulate MIPS assembly code on our machines. To install `spim` use the following command. Also install the python requirements for the `tester.py`.

```bash
$ sudo apt install spim
$ python3  -m pip  install -r requirements.txt
```

To test the assembly code run the `tester.py` script. 

```bash
$ python3  tester.py -n 20 -m 100 -b 10 -e 0.0001
```
The above command will run 20 random test cases with 100 coordinates of at max 10 bitseach, with an error tolerance of 0.0001 percent.


```bash
$ python3  tester.py -i <input_file_path>
```

The above command runs a custom test case on the assembly code.
  The test case file should begin with the number of points and the following lines will contain the coordinates of each point in a separateline (x and y coordinate on separate line as well).