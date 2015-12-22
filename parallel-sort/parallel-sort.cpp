/**
 * @file		parallel-sort.cpp
 * A program that implements a parallel merge sort algorithm.
 *
 * @author		Jennifer Yao
 * @date		2015
 * @copyright	All rights reserved.
 */

#include "config.hpp"

#include <cassert>
#include <cinttypes>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#if !defined(NDEBUG) && defined(VERBOSE)
#include <thread>
#endif

class node;

// A helper class. Represents a node in a binary tree.
class node {
public:
	std::unique_ptr<node> left;
	std::unique_ptr<node> right;

	constexpr node() noexcept : left(), right() {}
	node(std::unique_ptr<node>&& left, std::unique_ptr<node>&& right) noexcept : left(std::move(left)), right(std::move(right)) {}

	template<class RandomAccessIterator>
	void parallel_merge_sort(RandomAccessIterator first, RandomAccessIterator last) {
		// If this is a leaf node, sort range using sequential algorithm.
		if (!left && !right) {
#if !defined(NDEBUG) && defined(VERBOSE)
			std::cerr << "parallel_merge_sort[" << std::this_thread::get_id()
			          << "]: Leaf node sorting range of size "
			          << (last - first) << "..."
			          << std::endl;
#endif
			return std::sort(first, last);
		}

		// Sort subrange(s) concurrently.
		RandomAccessIterator middle = first + ((last - first) / 2);
		std::future<void> left_future, right_future;
		auto sort_fn = std::bind(&node::parallel_merge_sort<RandomAccessIterator>,
		                         std::placeholders::_1,
		                         std::placeholders::_2,
		                         std::placeholders::_3);

		if (left && !right) {
			left_future = std::async(std::launch::async, sort_fn, left.get(), first, last);
		}
		else if (!left && right) {
			right_future = std::async(std::launch::async, sort_fn, right.get(), first, last);
		}
		else {
			left_future = std::async(std::launch::async, sort_fn, left.get(), first, middle);
			right_future = std::async(std::launch::async, sort_fn, right.get(), middle, last);
		}
		if (left)
			left_future.wait();
		if (right)
			right_future.wait();

		// Merge sorted subranges.
		if (left && right)
			std::inplace_merge(first, middle, last);
	}

	template<class RandomAccessIterator, class Compare>
	void parallel_merge_sort(RandomAccessIterator first, RandomAccessIterator last, Compare comp) {
		// If this is a leaf node, sort range using sequential algorithm.
		if (!left && !right) {
#if !defined(NDEBUG) && defined(VERBOSE)
			std::cerr << "parallel_merge_sort[" << std::this_thread::get_id()
			          << "]: Leaf node sorting range of size "
			          << (last - first) << "..."
			          << std::endl;
#endif
			return std::sort(first, last, comp);
		}

		// Sort subrange(s) concurrently.
		RandomAccessIterator middle = first + ((last - first) / 2);
		std::future<void> left_future, right_future;
		auto sort_fn = std::bind(&node::parallel_merge_sort<RandomAccessIterator, Compare>,
		                         std::placeholders::_1,
		                         std::placeholders::_2,
		                         std::placeholders::_3,
		                         std::placeholders::_4);

		if (left && !right) {
			left_future = std::async(std::launch::async, sort_fn, left.get(), first, last, comp);
		}
		else if (!left && right) {
			right_future = std::async(std::launch::async, sort_fn, right.get(), first, last, comp);
		}
		else {
			left_future = std::async(std::launch::async, sort_fn, left.get(), first, middle, comp);
			right_future = std::async(std::launch::async, sort_fn, right.get(), middle, last, comp);
		}
		if (left)
			left_future.wait();
		if (right)
			right_future.wait();

		// Merge sorted subranges.
		if (left && right)
			std::inplace_merge(first, middle, last, comp);
	}
};

template<class CharT, class Traits>
void show_usage(std::basic_ostream<CharT, Traits>& out);

template<class CharT, class Traits, class Allocator>
void get_lines(std::basic_istream<CharT, Traits>& in, std::vector<std::basic_string<CharT, Traits, Allocator>>& lines);

std::unique_ptr<node> make_tree(std::size_t n_leaves);

template<class RandomAccessIterator>
void parallel_merge_sort(RandomAccessIterator first, RandomAccessIterator last, std::size_t n_threads);

template<class RandomAccessIterator, class Compare>
void parallel_merge_sort(RandomAccessIterator first, RandomAccessIterator last, Compare comp, std::size_t n_threads);

int main(int argc, char* argv[]) {
	if (argc != 3) {
		show_usage(std::cerr);
		return 1;
	}

	// Parse command-line arguments.
	char* thread_count_end;

	const std::intmax_t thread_count = std::strtoimax(argv[2], &thread_count_end, 10);

	if (thread_count_end == argv[2]) {
		std::cerr << PACKAGE_NAME << ": Invalid number of threads."
		          << std::endl;
		return 1;
	}
	if (thread_count < 0) {
		std::cerr << PACKAGE_NAME
		          << ": The number of threads must be non-negative."
		          << std::endl;
		return 1;
	}

	std::vector<std::string> lines;

	// Read the input file.
	if (std::strcmp(argv[1], "-") == 0) {
		get_lines(std::cin, lines);
	}
	else {
		std::ifstream in(argv[1]);
		if (!in) {
			std::cerr << PACKAGE_NAME << ": Could not read " << argv[1] << "."
			          << std::endl;
			return 1;
		}
		get_lines(in, lines);
	}

	// If the input file is empty, do nothing and exit.
	if (lines.size() == 0)
		return 0;

	if (thread_count > lines.size()) {
		std::cerr << PACKAGE_NAME
		          << ": The number of threads must not exceed the number of lines."
		          << std::endl;
		return 1;
	}

	// Perform the parallel merge sort operation.
	parallel_merge_sort(lines.begin(), lines.end(), thread_count);

	// Write the sorted lines to standard output.
	for (const std::string& line : lines)
		std::cout << line << std::endl;

	return 0;
}

template<class CharT, class Traits>
void show_usage(std::basic_ostream<CharT, Traits>& out) {
	out << "Usage: " << PACKAGE_NAME << " <input file> <number of threads>\n"
	    << "Sort the lines in <input file> using a merge sort algorithm that executes\n"
	    << "<number of threads> tasks in parallel, and write the result to standard\n"
	    << "output.\n\n"
	    << "If <input file> is -, the program reads from standard input.\n\n"
	    << "If the specified number of threads is 0, the program uses " << CPU_COUNT << " by default."
	    << std::endl;
}

template<class CharT, class Traits, class Allocator>
void get_lines(std::basic_istream<CharT, Traits>& in, std::vector<std::basic_string<CharT, Traits, Allocator>>& lines) {
	std::basic_string<CharT, Traits, Allocator> line;
	while (std::getline(in, line))
		lines.push_back(line);
}

// Given the number of leaf nodes, constructs a more-or-less balanced binary
// tree from bottom-up.
// Precondition: n_leaves != 0.
std::unique_ptr<node> make_tree(std::size_t n_leaves) {
	std::vector<std::unique_ptr<node>> nodes;

	for (std::size_t i = 0; i < n_leaves; i++)
		nodes.emplace_back(new node);

	bool reverse = false;

	while (nodes.size() > 1) {
		std::vector<std::unique_ptr<node>> new_nodes;
		if (!reverse) {
			for (auto first = nodes.begin(), last = nodes.end(); first < last; first += 2) {
				std::unique_ptr<node> left = std::move(*first);
				std::unique_ptr<node> right = first + 1 < last ? std::move(*(first + 1)) : nullptr;
				new_nodes.emplace_back(new node(std::move(left), std::move(right)));
			}
		}
		else {
			for (auto first = nodes.rbegin(), last = nodes.rend(); first < last; first += 2) {
				std::unique_ptr<node> right = std::move(*first);
				std::unique_ptr<node> left = first + 1 < last ? std::move(*(first + 1)) : nullptr;
				new_nodes.emplace_back(new node(std::move(left), std::move(right)));
			}
		}
		reverse = !reverse;
		nodes = std::move(new_nodes);
	}

	assert(nodes.size() == 1);

	return std::move(nodes[0]);
}

template<class RandomAccessIterator>
void parallel_merge_sort(RandomAccessIterator first, RandomAccessIterator last, std::size_t n_threads) {
	if (n_threads == 0)
		n_threads = std::min(SIZE_C(CPU_COUNT), static_cast<std::size_t>(last - first));
	std::unique_ptr<node> head = make_tree(n_threads);
	head->parallel_merge_sort(first, last);
}

template<class RandomAccessIterator, class Compare>
void parallel_merge_sort(RandomAccessIterator first, RandomAccessIterator last, Compare comp, std::size_t n_threads) {
	if (n_threads == 0)
		n_threads = std::min(SIZE_C(CPU_COUNT), static_cast<std::size_t>(last - first));
	std::unique_ptr<node> head = make_tree(n_threads);
	head->parallel_merge_sort(first, last, comp);
}
