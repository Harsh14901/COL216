# MIPS simulator with DRAM timing model

## Usage

First build the executable by running the `make` command in the root directory of the project.

The executable `main` takes in 1 required parameter and 3 optional parameters as command line arguments

`main <file_name> [row_access_delay] [col_access_delay] [block]`

### Examples

```bash
$ ./output/main /tmp/a.asm 10 2 0
```


