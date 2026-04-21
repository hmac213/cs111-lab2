## UID: 506380176

## Pipe Up

The `pipe` program runs a series of shell commands as a pipeline where the stdout of one command is sequentially fed to the stdin of the next.

## Building

From this directory, run

```bash
make
```

## Running

Example:

```bash
./pipe ls cat wc
```

The program should output (just as `ls | cat | wc` would) a line of three numbers from `wc` (line/word/byte counts of `ls`’s output).

## Cleaning up

Remove the build with

```bash
make clean
```

That deletes `pipe` and `pipe.o`.
