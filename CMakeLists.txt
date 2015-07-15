cmake_minimum_required(VERSION 2.8)
project (ConAnalysis)
set(CMAKE_BUILD_TYPE Release)

find_package(LLVM CONFIG REQUIRED)

#configure_file(${CMAKE_SOURCE_DIR}/include/ConAnal/ConAnalysis.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(lib/ConAnal)
