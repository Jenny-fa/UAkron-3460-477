cmake_minimum_required(VERSION 3.0)

include(CheckCXXCompilerFlag)
include(CheckCXXSymbolExists)
include(ProcessorCount)

# Add a project.
project(distributed-prime-numbers VERSION 1.0 LANGUAGES CXX)

# Set variables.
set(PACKAGE_NAME ${PROJECT_NAME})
set(PACKAGE_TARNAME ${PACKAGE_NAME})
set(PACKAGE_VERSION ${PROJECT_VERSION})
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "jy38@zips.uakron.edu")
set(PACKAGE_URL "https://github.com/Jenny-fa/UAkron-3460-477")

# Add options.
option(VERBOSE "Set to enable verbose output for non-release build configurations." OFF)

# Enable testing.
# enable_testing()

# Run platform checks.
processorcount(CPU_COUNT)
if(CPU_COUNT EQUAL 0)
  set(CPU_COUNT 1)
endif()
check_cxx_compiler_flag(-std=c++11 CXX_COMPILER_HAS_STDCXX11_FLAG)
if(CXX_COMPILER_HAS_STDCXX11_FLAG)
	set(CMAKE_REQUIRED_FLAGS -std=c++11)
endif()
find_package(Boost 1.57.0 REQUIRED)
check_cxx_symbol_exists(std::snprintf cstdio HAVE_STD_SNPRINTF)

# Set compiler and linker flags.
if(CXX_COMPILER_HAS_STDCXX11_FLAG)
	add_compile_options(-std=c++11)
endif()
if(VERBOSE)
	add_definitions(-DVERBOSE)
endif()
include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR} ${BOOST_INCLUDE_DIRS})

# Add the executable targets.
add_executable(distributed-prime-numbers distributed-prime-numbers.cpp)
add_executable(distributed-prime-numbers-helper distributed-prime-numbers-helper.cpp)

# Generate the configuration header.
configure_file(config.hpp.in config.hpp)

# Add 'check' target.
# if(NOT TARGET check)
# 	add_custom_target(check COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target test)
# endif()

# Add subdirectories.
# add_subdirectory(test EXCLUDE_FROM_ALL)
