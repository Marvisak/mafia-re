cmake_minimum_required(VERSION 3.25)

project(ls3df)

# The engine. The original has 61 C++ objects and one C object of its own, the sources are added file by file.
add_library(ls3df SHARED Globals.cpp)
# The export directory names the DLL
set_target_properties(ls3df PROPERTIES OUTPUT_NAME LS3DF)

# The DirectX 8.1 SDK: its d3d8.lib, dinput8.lib and dsound.lib import objects (Implib700 9210) and its dxguid.lib
# and dinput8.lib data objects (Utc13 9178) are in the Rich header. Its headers come before the compiler's own, which
# are DirectX 6's. The SDK ships an old basetsd.h, so newer compilers need the Windows SDK's headers first.
if (MSVC_VERSION GREATER 1500)
    target_include_directories(ls3df PRIVATE $ENV{WindowsSdkDir}/Include/$ENV{WindowsSDKVersion}/shared)
endif()
target_include_directories(ls3df PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/dx81sdk/include)
target_link_directories(ls3df PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/dx81sdk/lib)

if (MSVC_VERSION EQUAL 1200)
    # Original compile flags (/O2 comes from cmake/vc6-flags.cmake)
    # /MT - static multithreaded CRT (LIBCMT.LIB): TLS functions among the imports
    # /GX - C++ exception handling: the unwind funclets after the CRT
    # /Gr - __fastcall: free functions take their first two arguments in ecx and edx (LoadAnimation5ds, dbgPrintGuid,
    #       ErrorHandler); the API is declared __stdcall (the exported math) or __cdecl (the undecorated exports)
    # /G6 - Pentium Pro scheduling: npad alignment inside functions and imul by constants
    target_compile_options(ls3df PRIVATE /MT /GX /Gr /G6)

    # The original also imports GDI32, WINMM, ole32, d3d8, DINPUT8, DSOUND, binkw32, vorbisfile and rw_data. Each
    # library is added with the first function that needs it: their order is still to be worked out.
    target_link_libraries(ls3df PRIVATE kernel32 user32)
    # The original's link time (2003-06-24), in the PE header and the export directory
    postlink(ls3df --timestamp 0x3ef851b2)
else()
    target_link_libraries(ls3df PRIVATE user32)
endif()

vc6_header_dependencies(ls3df)
