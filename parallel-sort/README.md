# Homework 3: Parallel Sort

## Building

This project is configured using [CMake](https://cmake.org) version 3.0 or
later, and requires a C++11 compiler.

The default build target creates an executable named `parallel-sort`.

## Usage

The `parallel-sort` program takes two command-line arguments: the name of an
input text file, and the number of threads to use. The number of threads must
be a non-negative integer. For more complete instructions, run `parallel-sort`
without any arguments.

`parallel-sort` behaves somewhat similarly to the POSIX
[`sort`](http://pubs.opengroup.org/onlinepubs/9699919799/utilities/sort.html)
utility.

An example invocation that runs two tasks in parallel:

```shell
./parallel-sort input.txt 2
```

## Notes

`parallel-sort` implements a parallel variant of the merge sort algorithm.
