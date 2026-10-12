# Visual C++ 6.0 SP5 + Processor Pack, the toolchain the original PC binaries were built with.
# Used by the "vc6" preset in CMakePresets.json, which also puts the tools on PATH
# (every VC6 tool needs MSPDB60.DLL from Common/MSDev98/Bin).

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR X86)

cmake_path(SET VC6_ROOT NORMALIZE "${CMAKE_CURRENT_LIST_DIR}/../3rdparty/msvc600_sp5_vcpp")

set(CMAKE_C_COMPILER "${VC6_ROOT}/VC98/Bin/CL.EXE")
set(CMAKE_CXX_COMPILER "${VC6_ROOT}/VC98/Bin/CL.EXE")
set(CMAKE_RC_COMPILER "${VC6_ROOT}/Common/MSDev98/Bin/RC.EXE")
set(CMAKE_LINKER "${VC6_ROOT}/VC98/Bin/LINK.EXE")
set(CMAKE_AR "${VC6_ROOT}/VC98/Bin/LIB.EXE")

# CMake's MSVC defaults change code generation and link inputs, vc6-flags.cmake replaces them
set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_CURRENT_LIST_DIR}/vc6-flags.cmake")

# No automatic /MD, each target picks its own CRT (/ML is not expressible through this property)
set(CMAKE_MSVC_RUNTIME_LIBRARY "")
