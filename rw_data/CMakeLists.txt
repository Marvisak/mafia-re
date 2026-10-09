cmake_minimum_required(VERSION 3.25)

project(rw_data)

# The original is a single translation unit (rw_data.cpp)
add_library(rw_data SHARED rw_data.cpp)

# Original compile flags (/O2 comes from cmake/vc6-flags.cmake)
# /ML - static single-threaded CRT (LIBC.LIB)
# /Gz - every internal function in the binary is __stdcall
if (MSVC_VERSION EQUAL 1200)
    target_compile_options(rw_data PRIVATE /ML /GX /Gz)
    target_link_libraries(rw_data PRIVATE kernel32 user32)
endif()

vc6_header_dependencies(rw_data)
