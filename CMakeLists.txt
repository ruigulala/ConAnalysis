cmake_minimum_required(VERSION 2.8)
project (ConAnalysis)
set(CMAKE_BUILD_TYPE Release)

find_package(LLVM CONFIG REQUIRED)

#configure_file(${CMAKE_SOURCE_DIR}/include/ConAnal/ConAnalysis.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(lib/ConAnal)
add_subdirectory(TESTS/freebsd-cve-3527a)

# enable_testing
enable_testing()

# test freebsd-cve-3527a 
add_test(freebsd-cve-3527a.test "${PROJECT_BINARY_DIR}/TESTS/freebsd-cve-3527a/autotest.sh" "freebsd-cve-3527a")
set_tests_properties (freebsd-cve-3527a.test
  PROPERTIES PASS_REGULAR_EXPRESSION "[ 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 19 ];[ 19 20 21 22 23 ];[ 19 ]"
  )
