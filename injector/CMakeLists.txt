cmake_minimum_required(VERSION 3.25)

project(injector)
project(replacer)

set(CMAKE_CXX_STANDARD 17)

set(SUBHOOK_FORCE_32BIT ON)
set(SUBHOOK_STATIC  ON)
set(SUBHOOK_TESTS  OFF)
add_subdirectory(subhook)

file(GLOB_RECURSE PROJECT_SOURCE_FILES
    ../engine/main.cpp
    ../engine/version.cpp
)

add_executable(injector main.cpp)
add_library(replacer SHARED dll.cpp ${PROJECT_SOURCE_FILES})
target_link_libraries(replacer PRIVATE subhook::subhook)