cmake_minimum_required(VERSION 3.25)

project(rw_data)

add_library(rw_data SHARED dta.cpp dta_read.cpp main.cpp )

# This is just to try to match the original compile flags
if (MSVC_VERSION EQUAL 1200)
    target_compile_options(rw_data PRIVATE /ML /GX)
endif()
