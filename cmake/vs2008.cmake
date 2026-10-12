# Visual C++ 2008 SP1 with the Windows SDK 6.0A, the toolchain of Steam's 2010 rebuild of setup.exe.
# Used by the "vs2008" preset in CMakePresets.json, which also puts the tools on PATH (cl needs mspdb80.dll
# from VC/bin) and sets INCLUDE and LIB.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR X86)

cmake_path(SET VS2008_ROOT NORMALIZE "${CMAKE_CURRENT_LIST_DIR}/../3rdparty/msvc900_sp1")
set(VS2008_SDK "${VS2008_ROOT}/Microsoft SDKs/Windows/v6.0A")

set(CMAKE_C_COMPILER "${VS2008_ROOT}/VC/bin/cl.exe")
set(CMAKE_CXX_COMPILER "${VS2008_ROOT}/VC/bin/cl.exe")
set(CMAKE_RC_COMPILER "${VS2008_SDK}/Bin/RC.Exe")
set(CMAKE_LINKER "${VS2008_ROOT}/VC/bin/link.exe")
set(CMAKE_AR "${VS2008_ROOT}/VC/bin/lib.exe")
set(CMAKE_MT "${VS2008_SDK}/Bin/mt.exe")

# The same reset of CMake's MSVC defaults as for VC6: every target states its own flags
set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_CURRENT_LIST_DIR}/vc6-flags.cmake")
set(CMAKE_MSVC_RUNTIME_LIBRARY "")
