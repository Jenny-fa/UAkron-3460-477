# Homework 3: Convolution

## Dependencies

This project requires the following packages to be installed:

- [Boost](http://www.boost.org) version 1.57 or later
- [JPEG](http://www.infai.org/jpeg) (libjpeg)

## Building

This project is configured using [CMake](https://cmake.org) version 3.0 or
later, and requires a C++11 compiler.

The default build target creates an executable named `convolution`.

## Usage

The `convolution` program takes three command-line arguments: the name of an
input image file, the name of the output image file, and the number of
threads to use. The input image must be in JPEG format, and the number of
threads must be a non-negative integer. For more complete instructions, run
`convolution` without any arguments.

An example invocation that runs two tasks in parallel:

```shell
./convolution input.jpg output.jpg 2
```
