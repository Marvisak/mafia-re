cmake_minimum_required(VERSION 3.29)

project(ijoy)

# The original is a single translation unit
add_library(ijoy SHARED ijoy.cpp)
# The export directory names the DLL
set_target_properties(ijoy PROPERTIES OUTPUT_NAME IJoy)

# The DirectX 8.1 SDK: the Rich header has dinput.lib's import descriptor and null thunk (Implib700 9210), one of its
# data formats (c_dfDIJoystick2, Utc13_C 9178) and dxguid.lib's one object (Utc13_CPP 9178).
# Its headers and libraries come before the compiler's own, which are DirectX 6's. The SDK ships an old basetsd.h,
# so newer compilers need the Windows SDK's headers first.
if (MSVC_VERSION GREATER 1500)
    target_include_directories(ijoy PRIVATE $ENV{WindowsSdkDir}/Include/$ENV{WindowsSDKVersion}/shared)
endif()
target_include_directories(ijoy PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/dx81sdk/include)
target_link_directories(ijoy PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/dx81sdk/lib)

if (MSVC_VERSION EQUAL 1200)
    # Original compile flags (/O2 comes from cmake/vc6-flags.cmake)
    # /ML - static single-threaded CRT (LIBC.LIB): no TLS functions among the imports
    # /GX - C++ exception handling: the constructors' unwind funclets
    target_compile_options(ijoy PRIVATE /ML /GX)

    # Built with the original Visual C++ 6.0 release, apart from the Processor Pack's back end (C2.DLL 13.00.9044,
    # the Rich header's Utc12_2_CPP 9044). The release's front end (C1XX.DLL 12.00.8168) aligns every vtable to 4
    # bytes, where SP5's aligns those of 0x40 bytes and more to 8, which moves IJoy's .rdata, .data and import table.
    # The release's front end works with the Processor Pack's back end (SP5's with the release's back end doesn't
    # compile), and the driver makes no difference (SP5's CL.EXE gives the same file). The launcher runs CL.EXE from a
    # directory with the release's driver and front end and the Processor Pack's back end, copied here.
    set(ijoy_cl "${CMAKE_CURRENT_BINARY_DIR}/cl")
    file(COPY "${CMAKE_SOURCE_DIR}/3rdparty/msvc600/VC98/Bin/CL.EXE" "${CMAKE_SOURCE_DIR}/3rdparty/msvc600/VC98/Bin/C1XX.DLL"
        "${CMAKE_SOURCE_DIR}/3rdparty/msvc600/VC98/Bin/MSPDB60.DLL"
        "${CMAKE_SOURCE_DIR}/3rdparty/msvc600_sp5_vcpp/VC98/Bin/c2.dll" DESTINATION "${ijoy_cl}")
    set_target_properties(ijoy PROPERTIES
        CXX_COMPILER_LAUNCHER "${MAFIA_PYTHON};${CMAKE_SOURCE_DIR}/tools/clswap.py;${ijoy_cl}")

    # The release's linker too: the Rich header has LINK 6.00.8168 and LIBC.LIB objects of build 8168. LINKER_TYPE
    # gives this target its own linker (CMake 3.29, hence the minimum: an older CMake ignores it and links with SP5's),
    # and /LIBPATH comes before the preset's LIB, so LINK takes the release's LIBC.LIB over SP5's. The release's
    # LINK.EXE loads the MSPDB60.DLL next to it, whatever PATH says, so the PDB build's /DEBUG link works too.
    set(CMAKE_CXX_USING_LINKER_VC6RTM "${CMAKE_SOURCE_DIR}/3rdparty/msvc600/VC98/Bin/LINK.EXE")
    set_target_properties(ijoy PROPERTIES LINKER_TYPE VC6RTM)
    target_link_directories(ijoy PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/msvc600/VC98/Lib)
    # kernel32 first fits the original's imports best (the Rich header is the same either way), but they don't match
    # yet: the hint/name entries, the order LINK pulls the import objects in, are the original's, the IAT isn't (the
    # user imports and four or five of the CRT's sit at other slots), which moves every reference to an import slot.
    # In small test links each DLL's IAT follows the order of first reference (rotated by one without the CRT); with
    # the CRT and a second DLL some CRT imports move after the user imports. The library order changes it; linker
    # options, more or reordered references in a leading object, more symbols, WIN32_LEAN_AND_MEAN and /YX don't.
    target_link_libraries(ijoy PRIVATE kernel32 user32 dinput dxguid)
    # No /DEBUG (the original has no debug directory) and no /RELEASE (its checksum is 0).
    # The original's link time (2002-07-17), in the PE header and the export directory
    postlink(ijoy --timestamp 0x3d35e3d1)
else()
    target_link_libraries(ijoy PRIVATE user32 dinput dxguid)
endif()

vc6_header_dependencies(ijoy)
