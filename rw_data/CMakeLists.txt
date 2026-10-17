cmake_minimum_required(VERSION 3.25)

project(rw_data)

# The original is a single translation unit (rw_data.cpp)
add_library(rw_data SHARED rw_data.cpp)

# Original compile flags (/O2 comes from cmake/vc6-flags.cmake)
# /ML - static single-threaded CRT (LIBC.LIB)
# /Gz - every internal function in the binary is __stdcall
# The defaults stay: /O2's /Ob1, as /Ob2 (which H&D Deluxe's dta_read project used) changes the static initializers
# and the C_rw_data_inter constructor, and /GB, as the code has lea [x+x*n] chains but neither npad inside functions
# nor imul by constants, which /G6 gives
if (MSVC_VERSION EQUAL 1200)
    target_compile_options(rw_data PRIVATE /ML /GX /Gz)
    target_link_libraries(rw_data PRIVATE kernel32 user32)
    # The original's link time (2002-07-12), in the PE header and the export directory
    postlink(rw_data --timestamp 0x3d2e1e08)
endif()

vc6_header_dependencies(rw_data)
