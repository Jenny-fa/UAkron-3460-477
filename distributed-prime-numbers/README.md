# Homework 6: Distributed Prime Numbers

## Dependencies

This project requires the following packages to be installed:

- [Boost](http://www.boost.org) version 1.57 or later

## Building

This project is configured using [CMake](https://cmake.org) version 3.0 or
later, and requires a C++11 compiler.

The default build target creates two executables named
`distributed-prime-numbers` and `distributed-prime-numbers-helper`.

## Usage

The `distributed-prime-numbers` program works exactly the same as
`prime-numbers`, except the work is distributed over multiple processes
instead of threads. Like `prime-numbers`, it takes two command-line
arguments: the number of primes to generate, and the number of processes to
use. Both arguments must be non-negative integers. For more complete
instructions, run `distributed-prime-numbers` without any arguments.

An example invocation that outputs 1000 primes and runs two tasks in
parallel:

```shell
./distributed-prime-numbers 1000 2
```

## Known Bugs

The `distributed-prime-numbers` program presumably suffers from the same
integer overflow problem as `prime-numbers` (see that project's README.md for
details).
