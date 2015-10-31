configure_file(autotest.sh autotest.sh COPYONLY)
configure_file(part1_loc.txt part1_loc.txt COPYONLY)
configure_file(part2_loc.txt part2_loc.txt COPYONLY)

set(SOURCE_NAME apache-25520)
set(src_bc_compiler clang)

add_custom_command(
  OUTPUT  ${SOURCE_NAME}.bc 
  COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} && make
  #DEPENDS ${SOURCE_NAME}.c
  #IMPLICIT_DEPENDS CXX ${SOURCE_NAME}.c
  COMMENT "Building LLVM bitcode ${SOURCE_NAME}.bc"
  VERBATIM
)
set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${SOURCE_NAME}.bc)

add_custom_target(
  apache-25520 ALL
  DEPENDS ${SOURCE_NAME}.bc)
