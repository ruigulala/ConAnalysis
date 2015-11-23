cmake_minimum_required(VERSION 2.8)
project (ConAnalysis)
set(CMAKE_BUILD_TYPE Release)

find_package(LLVM CONFIG REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(lib/ConAnal)
add_subdirectory(lib/DOL)
add_subdirectory(TESTS/libsafe-cve-1125)
add_subdirectory(TESTS/kernel-CVE-2004-1235)
add_subdirectory(TESTS/libvirt-cve-1447)
add_subdirectory(TESTS/apache-25520)
#add_subdirectory(TESTS/freebsd-cve-3527a)
#add_subdirectory(TESTS/freebsd-osvdb-58543)

# enable_testing
enable_testing()

#define a macro to simplify adding tests
macro (do_test arg result)
  add_test(${arg}.test ${PROJECT_BINARY_DIR}/TESTS/${arg}/autotest.sh ${arg})
  set_tests_properties (${arg}.test
    PROPERTIES PASS_REGULAR_EXPRESSION ${result}
  )
endmacro (do_test)

# test libsafe-cve-1125
add_test(libsafe-cve-1125.test "${PROJECT_BINARY_DIR}/TESTS/libsafe-cve-1125/autotest.sh" "libsafe-cve-1125")
set_tests_properties (libsafe-cve-1125.test
  PROPERTIES PASS_REGULAR_EXPRESSION ""
  )

# test kernel-CVE-2004-1235
add_test(kernel-CVE-2004-1235.test "${PROJECT_BINARY_DIR}/TESTS/kernel-CVE-2004-1235/autotest.sh" "kernel-CVE-2004-1235")
set_tests_properties (kernel-CVE-2004-1235.test
  PROPERTIES PASS_REGULAR_EXPRESSION ""
  )

# test libvirt-cve-1447 
add_test(libvirt-cve-1447.test "${PROJECT_BINARY_DIR}/TESTS/libvirt-cve-1447/autotest.sh" "libvirt-cve-1447")
set_tests_properties (libvirt-cve-1447.test
  PROPERTIES PASS_REGULAR_EXPRESSION ""
  )

# test apache-25520 
add_test(apache-25520.test "${PROJECT_BINARY_DIR}/TESTS/apache-25520/autotest.sh" "apache-25520")
set_tests_properties (apache-25520.test
  PROPERTIES PASS_REGULAR_EXPRESSION ""
  )

# test freebsd-cve-3527a 
#add_test(freebsd-cve-3527a.test "${PROJECT_BINARY_DIR}/TESTS/freebsd-cve-3527a/autotest.sh" "freebsd-cve-3527a")
#set_tests_properties (freebsd-cve-3527a.test
  #PROPERTIES PASS_REGULAR_EXPRESSION "\\[ 13 14 15 16 17 \\]\n\\[ 1 2 3 4 5 6 7 8 9 10 13 \\]\n\\[ 13 \\]"
  #)

# test freebsd-osvdb-58543 
#add_test(freebsd-osvdb-58543.test "${PROJECT_BINARY_DIR}/TESTS/freebsd-osvdb-58543/autotest.sh" "freebsd-osvdb-58543")
#set_tests_properties (freebsd-osvdb-58543.test
  #PROPERTIES PASS_REGULAR_EXPRESSION "\\[ 2 3 vp 8 9 11 \\]\n\\[ 7 8 9 10 11 \\]\n\\[ 8 9 11 \\]"
  #)

#do_test(freebsd-cve-3527a "\n[ 13 14 15 16 17 ]\n[ 1 2 3 4 5 6 7 8 9 10 13 ]\n[ 13 ]")
