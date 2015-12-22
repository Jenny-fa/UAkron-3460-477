/**
 * @file		distributed-prime-numbers-helper.cpp
 * A helper program for 'distributed-prime-numbers'.
 *
 * Defines the main entry point of a program that receives a range of
 * integers from 'distributed-prime-numbers' and performs primality testing
 * on that range.
 *
 * This program is meant to be run only by 'distributed-prime-numbers'.
 *
 * @author		Jennifer Yao
 * @date		2015
 * @copyright	All rights reserved.
 */

#include "config.hpp"

#include <cinttypes>
#include <iostream>
#include <random>

#include <boost/interprocess/sync/named_semaphore.hpp>

#include "shared_memory.hpp"

#define PRIMALITY_TEST_COUNT 100

template<class CharT, class Traits>
void show_usage(std::basic_ostream<CharT, Traits>& out);

template<class IntType>
IntType random_int(IntType min, IntType max);

std::uintmax_t mod_pow(std::uintmax_t x, std::uintmax_t y, std::uintmax_t n);

bool is_prime(std::uintmax_t n);

int main(int argc, char* argv[]) {
	if (argc != 4) {
		show_usage(std::cerr);
		return 1;
	}

	// Parse command-line arguments.
	char* range_id_end;
	char* offset_end;
	char* size_end;

	const std::intmax_t range_id = std::strtoimax(argv[1], &range_id_end, 10);
	const std::intmax_t offset = std::strtoimax(argv[2], &offset_end, 10);
	const std::intmax_t size = std::strtoimax(argv[3], &size_end, 10);

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

	check_argument(range_id, 1);
	check_argument(offset, 2);
	check_argument(size, 3);

	// Open the shared memory segment.
	boost::interprocess::managed_shared_memory segment(boost::interprocess::open_only, kSharedMemorySegmentName);

	// Open the semaphore.
	boost::interprocess::named_semaphore n_done(boost::interprocess::open_only, kSemaphoreName);

	shm_vector<bool>* prime_tables = segment.find<shm_vector<bool>>(kPrimeTableArrayName).first;

	// Perform primality testing on selected range.
	for (std::size_t i = 0; i < size; i++)
		prime_tables[range_id][i] = is_prime(offset + i);

	// Signal the driver that primality testing is done on this range.
	n_done.post();

	return 0;
}

template<class CharT, class Traits>
void show_usage(std::basic_ostream<CharT, Traits>& out) {
	out << "Usage: " << PACKAGE_NAME << "-helper <range-id> <offset> <size>\n"
	    << "Test the integers in range [<offset>, <size>) for primality."
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
