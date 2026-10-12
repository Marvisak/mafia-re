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
	# /G6 - Pentium Pro scheduling: SetText's push 0x400 comes before the lea of its buffer
	target_compile_options(setup PRIVATE /ML /Gz /G6)

	# The original was linked with /DEBUG: it ends with a CodeView record naming its PDB, and /DEBUG's
	# default /OPT:NOREF kept every import thunk and all of dxguid.lib's GUIDs. /OPT:NOREF also overrides
	# the PDB build's /OPT:REF.
	# d3d8.lib isn't listed: hw.cpp names it with #pragma comment(lib), so LINK searches it after LIBC, as the
	# thunk order shows.
	# dxguid before comctl32: the Rich header lists the tools in reverse order of first use, and the original has
	# COMCTL32.LIB's objects (no tool id) ahead of dxguid's.
	target_link_options(setup PRIVATE /DEBUG /OPT:NOREF /INCREMENTAL:NO)
	target_link_libraries(setup PRIVATE kernel32 user32 advapi32 ddraw dxguid comctl32)
	# The original's link time (2002-11-07) and CodeView record: its PDB, linked into twice (age 2)
	postlink(setup --timestamp 0x3dca7e48 --codeview-signature 0x3dca7530 --codeview-age 2
		"--codeview-path=C:\\Mafia\\_src\\setup\\Release\\setup.pdb")
elseif (MSVC_VERSION EQUAL 1500)
	# Steam's setup.exe: Visual C++ 2008 SP1, its default Release settings (/O2 /Oi /GL /Gy /EHsc /W3 /Zi, MSVCR90.dll)
	# and what its header shows: /LTCG, /DEBUG (an RSDS record), /RELEASE (a checksum), /DYNAMICBASE /NXCOMPAT.
	# The manifest is the linker's default one, which mt.exe embeds after the link (as resource 1, language 1033), as
	# a Release build in the IDE does: mt.exe rewrites the resource section sorted by type, as the original's is.
	# CMake's link step runs the toolchain's mt.exe like that.
	target_compile_options(setup PRIVATE /O2 /Oi /GL /GS /EHsc /Gy /W3 /Zi /MD)
	target_compile_definitions(setup PRIVATE WIN32 NDEBUG _WINDOWS)
	# /PDBALTPATH (undocumented in VS2008) records the original's PDB path in the CodeView record, which VS2008 puts in
	# .rdata: a path of another length moves everything after it. The PDB is still written to the build tree.
	target_link_options(setup PRIVATE /LTCG /DEBUG /RELEASE /OPT:REF /OPT:ICF /DYNAMICBASE /NXCOMPAT /MACHINE:X86
		"/PDBALTPATH:D:\\zdrojaky\\setup\\Release\\setup.pdb")
	# The system libraries first, as a VS2008 project's inherited defaults come: the import descriptors are in the
	# order the libraries are first used, and each DLL's imports in the order they are resolved.
	# d3d8.lib comes from hw.cpp's #pragma comment(lib), after MSVCRT, as the thunk order shows
	target_link_libraries(setup PRIVATE kernel32 user32 advapi32 comctl32 dxguid ddraw)
	# The original's link time (2010-04-07) and its PDB's GUID and age; /RELEASE's checksum is computed again after
	postlink(setup --timestamp 0x4bbc50b1 --codeview-guid 30b9c62b279e3745966e7ab4e7290a90 --codeview-age 1 --checksum)
else()
	target_link_libraries(setup PRIVATE comctl32 d3d8 ddraw dxguid)
endif()

vc6_header_dependencies(setup)
# RC's dependency output lists only included headers, not the files the script loads, so a splash or icon extracted
# over the placeholders (README) wouldn't rebuild the resources
set_property(SOURCE res/setup.rc APPEND PROPERTY OBJECT_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/res/splash.bmp" "${CMAKE_CURRENT_SOURCE_DIR}/res/mafia.ico")
