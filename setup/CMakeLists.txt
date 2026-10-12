cmake_minimum_required(VERSION 3.25)

project(setup)

# The target is the 1.2 setup: Visual C++ 6.0 SP5 + Processor Pack, DirectX 8.0 SDK.
# SETUP_STEAM builds Steam's 2010 rebuild instead: Visual C++ 2008 SP1, the DirectX 8.1 SDK (3rdparty/dx81sdk).
# The sources keep the rebuild's differences behind the macro of the same name.
option(SETUP_STEAM "Build Steam's 2010 rebuild of setup.exe instead of the 1.2 one" OFF)

add_executable(setup WIN32 main.cpp hw.cpp res/setup.rc)

if (SETUP_STEAM)
	target_compile_definitions(setup PRIVATE SETUP_STEAM)
	set(dxsdk_include ${CMAKE_SOURCE_DIR}/3rdparty/dx81sdk/include)
	set(dxsdk_lib ${CMAKE_SOURCE_DIR}/3rdparty/dx81sdk/lib)
else()
	# Direct3DCreate8(120) and the SDK's libraries in the Rich header
	set(dxsdk_include ${CMAKE_SOURCE_DIR}/3rdparty/dx8sdk/inc/dxsdk)
	set(dxsdk_lib ${CMAKE_SOURCE_DIR}/3rdparty/dx8sdk/lib/x86/dxsdk)
endif()

# The DirectX SDKs ship an old basetsd.h, so newer compilers need the Windows SDK's headers first. The vs2008
# preset puts the DirectX 8.1 SDK at the end of INCLUDE for that.
if (MSVC_VERSION GREATER 1500)
	target_include_directories(setup PRIVATE
		$ENV{WindowsSdkDir}/Include/$ENV{WindowsSDKVersion}/shared
	)
endif()

# /I comes before INCLUDE, so VC6 takes the SDK's ddraw.h over its own DirectX 6 one
if (NOT MSVC_VERSION EQUAL 1500)
	target_include_directories(setup PRIVATE ${dxsdk_include})
endif()
target_link_directories(setup PRIVATE ${dxsdk_lib})

if (MSVC_VERSION EQUAL 1200)
	# Original compile flags (/O2 comes from cmake/vc6-flags.cmake)
	# /ML - static single-threaded CRT (LIBC.LIB)
	# /Gz - every function in the binary is __stdcall
	target_compile_options(setup PRIVATE /ML /Gz)

	# The original was linked with /DEBUG: it ends with a CodeView record naming its PDB, and /DEBUG's
	# default /OPT:NOREF kept every import thunk and all of dxguid.lib's GUIDs. /OPT:NOREF also overrides
	# the PDB build's /OPT:REF.
	# d3d8.lib isn't listed: hw.cpp names it with #pragma comment(lib), so LINK searches it after LIBC.
	target_link_options(setup PRIVATE /DEBUG /OPT:NOREF /INCREMENTAL:NO)
	target_link_libraries(setup PRIVATE kernel32 user32 advapi32 ddraw comctl32 dxguid)
elseif (MSVC_VERSION EQUAL 1500)
	# Steam's setup.exe: Visual C++ 2008 SP1, its default Release settings (/O2 /Oi /GL /Gy /EHsc /W3 /Zi, MSVCR90.dll)
	# and what its header shows: /LTCG, /DEBUG (an RSDS record), /RELEASE (a checksum), /DYNAMICBASE /NXCOMPAT.
	# The manifest is a resource in setup.rc, so the linker doesn't make one.
	target_compile_options(setup PRIVATE /O2 /Oi /GL /GS /EHsc /Gy /W3 /Zi /MD)
	target_compile_definitions(setup PRIVATE WIN32 NDEBUG _WINDOWS)
	target_link_options(setup PRIVATE /LTCG /DEBUG /RELEASE /OPT:REF /OPT:ICF /DYNAMICBASE /NXCOMPAT /MACHINE:X86
		/MANIFEST:NO)
	target_link_libraries(setup PRIVATE comctl32 d3d8 ddraw dxguid kernel32 user32 advapi32)
else()
	target_link_libraries(setup PRIVATE comctl32 d3d8 ddraw dxguid)
endif()

vc6_header_dependencies(setup)
