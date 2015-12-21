cmake_minimum_required(VERSION 3.0)

include(CheckCXXCompilerFlag)
include(ProcessorCount)
include(CheckTypeSize)

# Add a project.
project(convolution VERSION 1.0 LANGUAGES CXX)

# Set variables.
set(PACKAGE_NAME "${PROJECT_NAME}")
set(PACKAGE_TARNAME "${PACKAGE_NAME}")
set(PACKAGE_VERSION "${PROJECT_VERSION}")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "jy38@zips.uakron.edu")
set(PACKAGE_URL "https://github.com/Jenny-fa/UAkron-3460-477")

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
find_package(JPEG REQUIRED)
find_package(Boost 1.57.0 REQUIRED)
check_type_size(ptrdiff_t SIZEOF_PTRDIFF_T LANGUAGE CXX)
check_type_size(long SIZEOF_LONG BUILTIN_TYPES_ONLY LANGUAGE CXX)
check_type_size("long long" SIZEOF_LONG_LONG BUILTIN_TYPES_ONLY LANGUAGE CXX)
if(SIZEOF_PTRDIFF_T EQUAL SIZEOF_LONG)
	set(PTRDIFF_T_SUFFIX "L")
elseif(SIZEOF_PTRDIFF_T EQUAL SIZEOF_LONG_LONG)
	set(PTRDIFF_T_SUFFIX "LL")
endif()

# Set compiler and linker flags.
if(CXX_COMPILER_HAS_STDCXX11_FLAG)
	add_compile_options(-std=c++11)
endif()
include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR} ${JPEG_INCLUDE_DIR} ${Boost_INCLUDE_DIRS})

# Add the executable target.
add_executable(convolution convolution.cpp)
target_link_libraries(convolution ${JPEG_LIBRARIES})

# Generate the configuration header.
configure_file(config.hpp.in config.hpp)

# Add 'check' target.
if(NOT TARGET check)
	add_custom_target(check COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target test)
endif()

# Add subdirectories.
# add_subdirectory(test EXCLUDE_FROM_ALL)
