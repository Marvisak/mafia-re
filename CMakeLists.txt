cmake_minimum_required(VERSION 3.25)

project(mafia)

include_directories(common)

# VC6 has no /showIncludes, so Ninja can't tell which headers a source includes.
# Make every source of the target depend on all project headers instead.
function(vc6_header_dependencies target)
    if (MSVC_VERSION EQUAL 1200)
        file(GLOB_RECURSE headers CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_SOURCE_DIR}/common/*.h")
        get_target_property(sources ${target} SOURCES)
        set_source_files_properties(${sources} PROPERTIES OBJECT_DEPENDS "${headers}")
    endif()
endfunction()

# The byte-exact builds (vc6, vs2008) get the original's link-time metadata after the link: the timestamps and the
# CodeView record, which the linker takes from the clock and the build tree (tools/postlink.py). The vc6-pdb build
# keeps the linker's, which match its PDB.
find_program(MAFIA_PYTHON NAMES python python3 HINTS "${CMAKE_SOURCE_DIR}/.venv/Scripts" REQUIRED)
function(postlink target)
    if ((MSVC_VERSION EQUAL 1200 OR MSVC_VERSION EQUAL 1500) AND CMAKE_BUILD_TYPE STREQUAL "Release")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND "${MAFIA_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/postlink.py" "$<TARGET_FILE:${target}>" ${ARGN}
            VERBATIM)
    endif()
endfunction()

# VC6 has no /FS: /Zi compiles of one target share its vc60.pdb, and parallel ones fail with C1033.
# Only the PDB build compiles with /Zi, so only it runs one compile at a time.
if (MSVC_VERSION EQUAL 1200 AND CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set_property(GLOBAL APPEND PROPERTY JOB_POOLS vc6_compile=1)
    set(CMAKE_JOB_POOL_COMPILE vc6_compile)
endif()

# Visual C++ 2008 builds only Steam's 2010 setup.exe
if (NOT MSVC_VERSION EQUAL 1500)
    add_subdirectory(rw_data)
endif()
add_subdirectory(setup)

# reccmp reads the recompiled binaries and their PDBs from reccmp-build.yml in the build directory
if (MSVC_VERSION EQUAL 1200 AND CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/reccmp-build.yml" CONTENT
"project: ${CMAKE_SOURCE_DIR}
targets:
  RWDATA:
    path: $<TARGET_FILE:rw_data>
    pdb: $<TARGET_PDB_FILE:rw_data>
  SETUP:
    path: $<TARGET_FILE:setup>
    pdb: $<TARGET_PDB_FILE:setup>
")
elseif (MSVC_VERSION EQUAL 1500)
    file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/reccmp-build.yml" CONTENT
"project: ${CMAKE_SOURCE_DIR}
targets:
  SETUPSTEAM:
    path: $<TARGET_FILE:setup>
    pdb: $<TARGET_PDB_FILE:setup>
")
endif()
