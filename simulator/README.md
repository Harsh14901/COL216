# MIPS simulator with DRAM timing model

## Usage

First build the executable by running the `make` command in the root directory of the project.

The executable `main` takes in 1 required parameter and 3 optional parameters as command line arguments

`main <file_name> [row_access_delay] [col_access_delay] [block]`

### Examples

```bash
$ ./output/main /tmp/a.asm 10 2 0
```

This will execute the file `/tmp/a.asm` with row access delay 10 and column access delay 2 for DRAM. The last argument 0 indicates non blocking mode. If the last argument is 1, it executes the processor in blocking mode.
