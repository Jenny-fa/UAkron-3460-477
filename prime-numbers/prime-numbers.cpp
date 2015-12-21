/**
 * @file		prime-numbers.cpp
 * A program that, given positive integer N, attempts to print the first N
 * prime numbers using a parallel algorithm.
 *
 * @author		Jennifer Yao
 * @date		2015
 * @copyright	All rights reserved.
 */

#include "config.hpp"

#include <cinttypes>
#include <cstdlib>
#include <algorithm>
#include <future>
#include <iostream>
#include <random>
#include <vector>

#define PRIMALITY_TEST_COUNT 100

template<class CharT, class Traits>
void show_usage(std::basic_ostream<CharT, Traits>& out);

template<class IntType>
IntType random_int(IntType min, IntType max);

std::uintmax_t mod_pow(std::uintmax_t x, std::uintmax_t y, std::uintmax_t n);

bool is_prime(std::uintmax_t n);

std::vector<bool> test_primes_in_range(std::uintmax_t offset, std::size_t size);

int main(int argc, char* argv[]) {
	if (argc != 3) {
		show_usage(std::cerr);
		return 1;
	}

	// Parse command-line arguments.
	char* prime_count_end;
	char* thread_count_end;

	std::intmax_t prime_count = std::strtoimax(argv[1], &prime_count_end, 10);
	std::intmax_t thread_count = std::strtoimax(argv[2], &thread_count_end, 10);

#define check_argument(var, arg_idx) \
	do { \
		if (var ## _end == argv[(arg_idx)]) { \
			std::cerr << PACKAGE_NAME << ": Argument " << (arg_idx) \
			          << " is invalid." << std::endl; \
			return 1; \
		} \
		if (var < 0) { \
			std::cerr << PACKAGE_NAME << ": Argument " << (arg_idx) \
			          << " must be non-negative." << std::endl; \
			return 1; \
		} \
	} \
	while (false)

	check_argument(prime_count, 1);
	check_argument(thread_count, 2);

	// If prime_count is 0, do nothing and exit.
	if (prime_count == 0)
		return 0;

	if (thread_count == 0)
		thread_count = std::min(INTMAX_C(CPU_COUNT), prime_count);

	if (thread_count > prime_count) {
		std::cerr << PACKAGE_NAME
		          << ": The number of threads must not exceed the number of primes."
		          << std::endl;
		return 1;
	}

	// Use Rosser's theorem to calculate the upper bound of the nth prime
	// number, where n is prime_count.
	const std::uintmax_t max_prime = prime_count < 6 ? 12 : prime_count * (std::log(prime_count) + std::log(std::log(prime_count)));

	// Divide the set of integers in [0, max_prime) into chunks/ranges, one
	// for each thread.
	const auto range_div = std::div(max_prime, thread_count);

	std::vector<std::uintmax_t> range_offsets(thread_count);
	std::vector<std::future<std::vector<bool>>> prime_table_futures(thread_count);

	// Perform primality tests on each range of integers.
	for (std::size_t i = 0; i < thread_count; i++) {
		const std::size_t range_size = range_div.quot + (i == 0 ? range_div.rem : 0);
		const std::uintmax_t range_offset = i * range_size + (i > 0 ? range_div.rem : 0);
		range_offsets[i] = range_offset;
		prime_table_futures[i] = std::async(std::launch::async, test_primes_in_range, range_offset, range_size);
	}

	// Write the list of prime numbers to standard output.
	for (std::size_t i = 0; i < thread_count; i++) {
		const std::vector<bool> prime_table = prime_table_futures[i].get();
		for (std::size_t j = 0; j < prime_table.size(); j++) {
			if (prime_table[j]) {
				std::cout << (range_offsets[i] + j) << std::endl;
				if (--prime_count == 0)
					goto exit;
			}
		}
	}

exit:
	return 0;
}

template<class CharT, class Traits>
void show_usage(std::basic_ostream<CharT, Traits>& out) {
	out << "Usage: " << PACKAGE_NAME << " <number of primes> <number of threads>\n"
	    << "Write the first <number of primes> prime numbers to standard output using an\n"
	    << "algorithm that executes <number of threads> tasks in parallel.\n\n"
	    << "If the specified number of threads is 0, the program uses " << CPU_COUNT << " by default.\n"
	    << "Prime numbers are separated by newlines."
	    << std::endl;
}

// Generates random integers in a thread-safe manner.
template<class IntType>
IntType random_int(IntType min, IntType max) {
	static thread_local std::default_random_engine generator(std::random_device{}());
	std::uniform_int_distribution<IntType> uniform_dist(min, max);
	return uniform_dist(generator);
}

// Precondition: n != 0.
std::uintmax_t mod_pow(std::uintmax_t x, std::uintmax_t y, std::uintmax_t n) {
	if (y == 0)
		return 1;
	std::uintmax_t z = mod_pow(x, y / 2, n);
	if (y % 2 == 0)
		return z * z % n;
	return x * z * z % n;
}

// NOTE: Implemented using Fermat's little theorem. The probability of the
// primality test returning a false positive is 1 / 2^k, where
// k = PRIMALITY_TEST_COUNT.
bool is_prime(std::uintmax_t n) {
	if (n < 2)
		return false;
	for (std::size_t i = 0; i < PRIMALITY_TEST_COUNT; i++) {
		const std::uintmax_t a = random_int<std::uintmax_t>(1, n - 1);
		if (mod_pow(a, n - 1, n) != 1)
			return false;
	}
	return true;
}

std::vector<bool> test_primes_in_range(std::uintmax_t offset, std::size_t size) {
	std::vector<bool> prime_table(size, false);

	for (std::size_t i = 0; i < size; i++)
		prime_table[i] = is_prime(offset + i);

	return prime_table;
}
