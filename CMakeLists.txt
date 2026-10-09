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

add_subdirectory(rw_data)

# setup.exe was built with Visual C++ 2008, it is not part of the VC6 build
if (NOT MSVC_VERSION EQUAL 1200)
    add_subdirectory(setup)
endif()

# reccmp reads the recompiled binaries and their PDBs from reccmp-build.yml in the build directory
if (MSVC_VERSION EQUAL 1200 AND CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/reccmp-build.yml" CONTENT
"project: ${CMAKE_SOURCE_DIR}
targets:
  RWDATA:
    path: $<TARGET_FILE:rw_data>
    pdb: $<TARGET_PDB_FILE:rw_data>
")
endif()
