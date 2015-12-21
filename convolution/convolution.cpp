/**
 * @file		convolution.cpp
 * A program that implements a parallelized version of the convolution
 * algorithm on an image file.
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

#include <boost/mpl/vector.hpp>
#include <boost/gil/extension/io/jpeg_io.hpp>
#include <boost/gil/extension/numeric/kernel.hpp>
#include <boost/gil/extension/numeric/convolve.hpp>

template<class PixelAccum, class Kernel>
struct convolve_fixed_fn : boost::gil::binary_operation_obj<convolve_fixed_fn<PixelAccum, Kernel>> {
	convolve_fixed_fn(const Kernel& kernel, boost::gil::convolve_boundary_option option) : kernel(kernel), option(option) {}

	const Kernel& kernel;
	boost::gil::convolve_boundary_option option;

	template<typename View1, typename View2>
	void apply_compatible(const View1& src, const View2& dst) const {
		boost::gil::convolve_rows_fixed<PixelAccum>(src, kernel, dst, option);
		boost::gil::convolve_cols_fixed<PixelAccum>(src, kernel, dst, option);
	}
};

template<class CharT, class Traits>
void show_usage(std::basic_ostream<CharT, Traits>& out);

int main(int argc, char* argv[]) {
	if (argc != 4) {
		show_usage(std::cerr);
		return 1;
	}

	// Parse command-line arguments.
	char* thread_count_end;

	std::intmax_t thread_count = std::strtoimax(argv[3], &thread_count_end, 10);

	if (thread_count_end == argv[3]) {
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

	// Read the input image.
	boost::gil::rgb8_image_t image;

	try {
		boost::gil::jpeg_read_image(argv[1], image);
	}
	catch (const std::ios_base::failure& exception) {
		std::cerr << PACKAGE_NAME << ": Could not read " << argv[1] << "."
		          << std::endl;
		return 1;
	}

	if (thread_count == 0)
		thread_count = std::min(PTRDIFF_C(CPU_COUNT), image.height());

	const boost::gil::rgb8_view_t image_view = boost::gil::view(image);
	const boost::gil::rgb8c_view_t const_image_view = boost::gil::const_view(image);

	// Create the kernel (radius-1 Gaussian blur).
	const double gaussian_1[] = {
		0.00022923296, 0.0059770769, 0.060597949,
		0.24173197,    0.38292751,   0.24173197,
		0.060597949,   0.0059770769, 0.00022923296
	};
	boost::gil::kernel_1d_fixed<double, 9> kernel(gaussian_1, 4);

	// Divide the input image into horizontal slices, one for each thread.
	const auto slice_div = std::div(image.height(), thread_count);

	std::vector<boost::gil::rgb8c_view_t> src_slices;
	std::vector<boost::gil::rgb8_view_t> dst_slices;
	src_slices.reserve(thread_count);
	dst_slices.reserve(thread_count);

	std::vector<std::future<void>> convolve_futures(thread_count);

	for (std::size_t i = 0; i < thread_count; i++) {
		typedef typename boost::gil::rgb8_view_t::point_t point_t;
		const std::ptrdiff_t slice_height = slice_div.quot + (i == 0 ? slice_div.rem : 0);
		const std::ptrdiff_t slice_y = i * slice_height + (i > 0 ? slice_div.rem : 0);
		const point_t size(image.width(), slice_height);
		const point_t offset(0, slice_y);
		src_slices.push_back(boost::gil::subimage_view(const_image_view, offset, size));
		dst_slices.push_back(boost::gil::subimage_view(image_view, offset, size));
	}

	// Perform the convolution operations on each slice.
	for (std::size_t i = 0; i < thread_count; i++) {
		convolve_futures[i] = std::async(std::launch::async,
		                                 convolve_fixed_fn<boost::gil::rgb32f_pixel_t, boost::gil::kernel_1d_fixed<double, 9>>(kernel, boost::gil::convolve_option_extend_constant),
		                                 src_slices[i],
		                                 dst_slices[i]);
	}

	for (std::future<void>& convolve_future : convolve_futures)
		convolve_future.wait();

	// Write the output image.
	try {
		boost::gil::jpeg_write_view(argv[2], image_view);
	}
	catch (const std::ios_base::failure& exception) {
		std::cerr << PACKAGE_NAME << ": Could not write " << argv[2] << "."
		          << std::endl;
		return 1;
	}

	return 0;
}

template<class CharT, class Traits>
void show_usage(std::basic_ostream<CharT, Traits>& out) {
	out << "Usage: " << PACKAGE_NAME << " <input file> <output file> <number of threads>\n"
	    << "Apply a very basic Gaussian blur effect on the image <input file> using a\n"
	    << "convolution algorithm that executes <number of threads> tasks in parallel,\n"
	    << "and write the result to <output file>.\n\n"
	    << "If the specified number of threads is 0, the program uses " << CPU_COUNT << " by default.\n\n"
	    << "NOTE: The input file must be a color JPEG image."
	    << std::endl;
}
