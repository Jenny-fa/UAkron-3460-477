/**
 * @file		shared_memory.hpp
 * An internal header.
 *
 * @author		Jennifer Yao
 * @date		2015
 * @copyright	All rights reserved.
 */

#ifndef SHARED_MEMORY_HPP
#define SHARED_MEMORY_HPP

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>

// NOTE: On Cygwin, Boost.Interprocess throws an exception if
// managed_shared_memory objects are created with a size that is not a
// multiple of 512 bytes for some reason.
#define kAlignment 512

#define kSharedMemorySegmentName PACKAGE_NAME ".prime-tables"
#define kSemaphoreName PACKAGE_NAME ".helper-count"
#define kPrimeTableArrayName "prime-tables"

template<class T>
using shm_allocator = boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager>;

template<class T>
using shm_vector = boost::interprocess::vector<T, shm_allocator<T>>;

/**
 * Returns the given object size rounded up to the nearest specified
 * alignment boundary.
 * @tparam Alignment The desired alignment requirement for @p n.
 * @param  n         An object size in bytes.
 * @return           The object size @p n rounded up to the nearest alignment
 *                   boundary @p Alignment.
 * @post             The return value is a multiple of @p Alignment.
 * @post             The return value is never less than @p n.
 */
template<std::size_t Alignment>
constexpr std::size_t align(std::size_t n) noexcept {
	return n + (Alignment - 1) & ~(Alignment - 1);
}

#endif // SHARED_MEMORY_HPP
