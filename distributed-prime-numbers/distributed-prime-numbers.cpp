/**
 * @file		distributed-prime-numbers.cpp
 * A multi-process version of the 'prime-numbers' program.
 *
 * Defines the main entry point of a program that spawns a number of worker
 * processes that perform primality testing on ranges of integers.
 *
 * @author		Jennifer Yao
 * @date		2015
 * @copyright	All rights reserved.
 */

#include "config.hpp"

#include <cinttypes>
#include <cmath>
#include <cstdlib>
#if !HAVE_STD_SNPRINTF
#include <stdio.h>
#else
#include <cstdio>
#endif
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

#include <boost/interprocess/sync/named_semaphore.hpp>

#include "shared_memory.hpp"

#if !HAVE_STD_SNPRINTF
namespace std {
	using ::snprintf;
}
#endif

static constexpr std::size_t max_command_length = 38 + 2 * std::numeric_limits<std::size_t>::digits10 + std::numeric_limits<std::uintmax_t>::digits10;

template<class CharT, class Traits>
void show_usage(std::basic_ostream<CharT, Traits>& out);

void clean_up();

int main(int argc, char* argv[]) {
	std::atexit(clean_up);

	if (argc != 3) {
		show_usage(std::cerr);
		return 1;
	}

	// Parse command-line arguments.
	char* prime_count_end;
	char* process_count_end;

	std::intmax_t prime_count = std::strtoimax(argv[1], &prime_count_end, 10);
	std::intmax_t process_count = std::strtoimax(argv[2], &process_count_end, 10);

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
	check_argument(process_count, 2);

	// If prime_count is 0, do nothing and exit.
	if (prime_count == 0)
		return 0;

	if (process_count == 0)
		process_count = std::min(INTMAX_C(CPU_COUNT), prime_count);

	if (process_count > prime_count) {
		std::cerr << PACKAGE_NAME
		          << ": The number of processes must not exceed the number of primes."
		          << std::endl;
		return 1;
	}

	// Use Rosser's theorem to calculate the upper bound of the nth prime
	// number, where n is prime_count.
	const std::uintmax_t max_prime = prime_count < 6 ? 12 : prime_count * (std::log(prime_count) + std::log(std::log(prime_count)));

	// Divide the set of integers in [0, max_prime) into chunks/ranges, one
	// for each process.
	const auto range_div = std::div(max_prime, process_count);

	std::vector<std::size_t> range_sizes(process_count);
	std::vector<std::uintmax_t> range_offsets(process_count);

#define LOG_EXPR(x) std::cerr << #x << " = " << (x) << std::endl

	try {
		// Create a new shared memory segment.
		const std::size_t segment_size = align<kAlignment>(process_count * sizeof(shm_vector<bool>)) + align<kAlignment>(max_prime);

#if !defined(NDEBUG) && defined(VERBOSE)
		std::cerr << "Shared memory segment size: " << segment_size << std::endl;
#endif

		boost::interprocess::managed_shared_memory segment(boost::interprocess::create_only, kSharedMemorySegmentName, segment_size);

		// Initialize shared memory allocator.
		const shm_allocator<bool> default_allocator(segment.get_segment_manager());

		// Construct an array of shm_vector<bool> objects in shared memory that
		// use the shared memory allocator.
		shm_vector<bool>* prime_tables = segment.construct<shm_vector<bool>>(kPrimeTableArrayName)[process_count](default_allocator);

		for (std::size_t i = 0; i < process_count; i++) {
			range_sizes[i]  = range_div.quot + (i == 0 ? range_div.rem : 0);
			range_offsets[i] = i * range_sizes[i] + (i > 0 ? range_div.rem : 0);
			prime_tables[i].assign(range_sizes[i], false);
		}

		// Create a semaphore to manage worker processes.
		boost::interprocess::named_semaphore n_done(boost::interprocess::create_only, kSemaphoreName, 0);

		// Perform primality tests on each range of integers.
		for (std::size_t i = 0; i < process_count; i++) {
			char command[max_command_length];
			std::snprintf(command, max_command_length, "./" PACKAGE_NAME "-helper %zu %ju %zu", i, range_offsets[i], range_sizes[i]);
#if !defined(NDEBUG) && defined(VERBOSE)
			std::cerr << "Running '" << command << "'..." << std::endl;
#endif
			// Launch a worker process.
			// Throw a runtime_error exception if the worker process returns
			// a nonzero exit status (hopefully this never happens).
			if (std::system(command))
				throw std::runtime_error(PACKAGE_NAME "-helper");
		}

		// Wait for worker processes to signal this program.
		for (std::size_t i = 0; i < process_count; i++)
			n_done.wait();

		// Write the list of prime numbers to standard output.
		for (std::size_t i = 0; i < process_count; i++) {
			const shm_vector<bool>& prime_table = prime_tables[i];
			for (std::size_t j = 0; j < prime_table.size(); j++) {
				if (prime_table[j]) {
					std::cout << (range_offsets[i] + j) << std::endl;
					if (--prime_count == 0)
						goto exit;
				}
			}
		}

	exit:
		// Destroy shared memory objects.
		segment.destroy_ptr(prime_tables);
	}
	catch (const std::exception& exception) {
		std::cerr << PACKAGE_NAME << ": error: " << exception.what()
		          << std::endl;
		return 1;
	}

	return 0;
}

template<class CharT, class Traits>
void show_usage(std::basic_ostream<CharT, Traits>& out) {
	out << "Usage: " << PACKAGE_NAME << " <number of primes> <number of processes>\n"
	    << "Write the first <number of primes> prime numbers to standard output using an\n"
	    << "algorithm that executes <number of processes> tasks in parallel.\n\n"
	    << "If the specified number of processes is 0, the program uses " << CPU_COUNT << " by default.\n"
	    << "Prime numbers are separated by newlines."
	    << std::endl;
}

// Deletes semaphore and shared memory segment (these resources are not
// automatically released when the process exits otherwise).
void clean_up() {
	boost::interprocess::named_semaphore::remove(kSemaphoreName);
	boost::interprocess::shared_memory_object::remove(kSharedMemorySegmentName);
}
