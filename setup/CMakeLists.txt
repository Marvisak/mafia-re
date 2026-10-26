cmake_minimum_required(VERSION 3.25)

project(setup)

# The target is the 1.2 setup: Visual C++ 6.0 SP5 + Processor Pack, DirectX 8.0 SDK.
# SETUP_STEAM builds Steam's 2010 rebuild instead: Visual C++ 2008 SP1, the DirectX 8.1 SDK (3rdparty/dx81sdk).
# The top-level CMakeLists.txt adds this directory a second time with SETUP_1_0 set, for 1.0's setup as target
# setup_1_0: the same toolchain, SDK, libraries and link (its Rich header is 1.2's but for one import less).
# It adds it again with SETUP_LANGUAGE set for the language versions Steam ships that are builds of their own, as
# target setup_<language>, built with res/<language>.rc: Russian (1.2's code), Polish and Czech (older code, the Czech
# one without optimization). Steam's German, French, Italian and Spanish ones are its 2010 rebuild localized after the
# link (below).
# The sources keep the code's versions apart by SETUP_DATE (version.h), which a build of older code sets.
option(SETUP_STEAM "Build Steam's 2010 rebuild of setup.exe instead of the 1.2 one" OFF)

if (SETUP_1_0)
	set(setup setup_1_0)
elseif (SETUP_LANGUAGE)
	set(setup setup_${SETUP_LANGUAGE})
else()
	set(setup setup)
endif()

if (SETUP_LANGUAGE)
	set(resources res/${SETUP_LANGUAGE}.rc)
else()
	set(resources res/setup.rc)
endif()

add_executable(${setup} WIN32 main.cpp hw.cpp ${resources})
set_target_properties(${setup} PROPERTIES OUTPUT_NAME setup)

if (SETUP_1_0)
	target_compile_definitions(${setup} PRIVATE SETUP_DATE=SETUP_DATE_1_0)
elseif (SETUP_LANGUAGE STREQUAL "polish")
	target_compile_definitions(${setup} PRIVATE SETUP_DATE=SETUP_DATE_POLISH)
elseif (SETUP_LANGUAGE STREQUAL "czech")
	target_compile_definitions(${setup} PRIVATE SETUP_DATE=SETUP_DATE_CZECH)
endif()

if (SETUP_STEAM)
	target_compile_definitions(${setup} PRIVATE SETUP_STEAM)
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
	target_include_directories(${setup} PRIVATE
		$ENV{WindowsSdkDir}/Include/$ENV{WindowsSDKVersion}/shared
	)
endif()

# /I comes before INCLUDE, so VC6 takes the SDK's ddraw.h over its own DirectX 6 one
if (NOT MSVC_VERSION EQUAL 1500)
	target_include_directories(${setup} PRIVATE ${dxsdk_include})
endif()
target_link_directories(${setup} PRIVATE ${dxsdk_lib})

if (MSVC_VERSION EQUAL 1200)
	# Original compile flags (/O2 comes from cmake/vc6-flags.cmake)
	# /ML - static single-threaded CRT (LIBC.LIB)
	# /Gz - every function in the binary is __stdcall
	# /G6 - Pentium Pro scheduling, the only switch that puts SetText's push 0x400 before the lea of its buffer
	target_compile_options(${setup} PRIVATE /ML /Gz /G6)
	if (SETUP_LANGUAGE STREQUAL "czech")
		# The Czech one was built without optimization but with the rest of /O2: inline expansion, intrinsics, string
		# pooling (its literals in reverse order of use) and function-level linking. That is what VC6's Standard Edition,
		# which has no optimizer, makes of a Release build. /Od comes after the global /O2 (warning D4025) and turns /Gf
		# off with it. Its code generator is plain SP5's (the Rich header's 8966): SP5+PP's CL and C1XX with plain SP5's
		# C2.DLL, as LS3DF's base classes (ls3df/CMakeLists.txt).
		target_compile_options(${setup} PRIVATE /Od /Ob1 /Oi /Gf /Gy)
		set(czech_cl "${CMAKE_CURRENT_BINARY_DIR}/czech_cl")
		file(COPY "${CMAKE_SOURCE_DIR}/3rdparty/msvc600_sp5_vcpp/VC98/Bin/CL.EXE"
			"${CMAKE_SOURCE_DIR}/3rdparty/msvc600_sp5_vcpp/VC98/Bin/C1.DLL"
			"${CMAKE_SOURCE_DIR}/3rdparty/msvc600_sp5_vcpp/VC98/Bin/C1XX.DLL"
			"${CMAKE_SOURCE_DIR}/3rdparty/msvc600_sp5/VC98/Bin/C2.DLL" DESTINATION "${czech_cl}")
		set_target_properties(${setup} PROPERTIES
			CXX_COMPILER_LAUNCHER "${MAFIA_PYTHON};${CMAKE_SOURCE_DIR}/tools/clswap.py;${czech_cl};--comp-id;00162306")
	endif()

	# The original was linked with /DEBUG: it ends with a CodeView record naming its PDB, and /DEBUG's
	# default /OPT:NOREF kept every import thunk and all of dxguid.lib's GUIDs. /OPT:NOREF also overrides
	# the PDB build's /OPT:REF.
	# d3d8.lib isn't listed: hw.cpp names it with #pragma comment(lib), so LINK searches it after LIBC, as the
	# thunk order shows.
	# dxguid before comctl32: the Rich header lists the tools in reverse order of first use, and the original has
	# COMCTL32.LIB's objects (no tool id) ahead of dxguid's.
	target_link_options(${setup} PRIVATE /DEBUG /OPT:NOREF /INCREMENTAL:NO)
	target_link_libraries(${setup} PRIVATE kernel32 user32 advapi32 ddraw dxguid comctl32)
	if (SETUP_1_0)
		# 1.0's link time (2002-07-28) and CodeView record: the same PDB path, linked into five times (age 5)
		postlink(${setup} --timestamp 0x3d433684 --codeview-signature 0x3d432d1e --codeview-age 5
			"--codeview-path=C:\\Mafia\\_src\\setup\\Release\\setup.pdb")
	elseif (SETUP_LANGUAGE STREQUAL "russian")
		# The Russian one's link time (2003-01-23) and CodeView record: another machine's tree, linked into twice.
		# That machine's LINK sorted the imports with an older qsort than today's msvcrt.dll has (as IJoy's did), so
		# on current Windows each DLL's imports come in another order than the original's. Linked on Windows 2000, whose
		# msvcrt.dll has that qsort, the build is identical; 1.0's, 1.2's and the Polish one then differ in their imports.
		postlink(${setup} --timestamp 0x3e3018ce --codeview-signature 0x3e3004ad --codeview-age 2
			"--codeview-path=D:\\mafia\\_SRC\\setup\\Release\\setup.pdb")
	elseif (SETUP_LANGUAGE STREQUAL "polish")
		# The Polish one's link time (2002-10-30) and CodeView record: 1.2's tree, a new PDB (age 1)
		postlink(${setup} --timestamp 0x3dbfc39e --codeview-signature 0x3dbfc39e --codeview-age 1
			"--codeview-path=C:\\Mafia\\_src\\setup\\Release\\setup.pdb")
	elseif (SETUP_LANGUAGE STREQUAL "czech")
		# The Czech one's link time (2002-08-23) and CodeView record: a copy of the project on the Russian one's machine,
		# a new PDB (age 1). As the Russian one's, its imports come in another order on current Windows and in the
		# original's on Windows 2000.
		postlink(${setup} --timestamp 0x3d664bce --codeview-signature 0x3d664bce --codeview-age 1
			"--codeview-path=D:\\mafia\\_src\\setup_cz\\Release\\setup.pdb")
	else()
		# The original's link time (2002-11-07) and CodeView record: its PDB, linked into twice (age 2)
		postlink(${setup} --timestamp 0x3dca7e48 --codeview-signature 0x3dca7530 --codeview-age 2
			"--codeview-path=C:\\Mafia\\_src\\setup\\Release\\setup.pdb")
	endif()
elseif (MSVC_VERSION EQUAL 1500)
	# Steam's setup.exe: Visual C++ 2008 SP1, its default Release settings (/O2 /Oi /GL /Gy /EHsc /W3 /Zi, MSVCR90.dll)
	# and what its header shows: /LTCG, /DEBUG (an RSDS record), /RELEASE (a checksum), /DYNAMICBASE /NXCOMPAT.
	# The manifest is the linker's default one, which mt.exe embeds after the link (as resource 1, language 1033), as
	# a Release build in the IDE does: mt.exe rewrites the resource section sorted by type and id, 4-byte aligned, as
	# the original's is. CMake's link step runs the toolchain's mt.exe like that. Compiled in through setup.rc, the
	# manifest would keep the .res order, 8-byte aligned, under the script's language.
	target_compile_options(${setup} PRIVATE /O2 /Oi /GL /GS /EHsc /Gy /W3 /Zi /MD)
	target_compile_definitions(${setup} PRIVATE WIN32 NDEBUG _WINDOWS)
	# /PDBALTPATH (undocumented in VS2008) records the original's PDB path in the CodeView record, which VS2008 puts in
	# .rdata: a path of another length moves everything after it. The PDB is still written to the build tree.
	target_link_options(${setup} PRIVATE /LTCG /DEBUG /RELEASE /OPT:REF /OPT:ICF /DYNAMICBASE /NXCOMPAT /MACHINE:X86
		"/PDBALTPATH:D:\\zdrojaky\\setup\\Release\\setup.pdb")
	# The system libraries first, as a VS2008 project's inherited defaults come (the whole default list gives the same
	# file): the import descriptors are in the order the libraries are first used, and each DLL's imports in the order
	# they are resolved. dxguid before ddraw for the Rich header.
	# d3d8.lib comes from hw.cpp's #pragma comment(lib), after MSVCRT, as the thunk order shows
	target_link_libraries(${setup} PRIVATE kernel32 user32 advapi32 comctl32 dxguid ddraw)
	# .reloc's VirtualSize (0x5ba) counts code the linker discarded: VS2008's linker reserves 2 bytes for every absolute
	# address in its input, also in code it then drops. /LTCG compiles a separate copy of each function it inlines (even
	# one called only from dead code, `if (0) f();`) and the linker drops it unreferenced. Unreferenced functions don't
	# count, /LTCG drops them first; unreferenced data does, VS2008 keeps it. The nine addresses in main.cpp's four
	# dialog helpers, inlined everywhere, give the original's size.
	# The original's link time (2010-04-07) and its PDB's GUID and age; /RELEASE's checksum is computed again after
	postlink(${setup} --timestamp 0x4bbc50b1 --codeview-guid 30b9c62b279e3745966e7ab4e7290a90 --codeview-age 1 --checksum)
else()
	target_link_libraries(${setup} PRIVATE comctl32 d3d8 ddraw dxguid)
endif()

vc6_header_dependencies(${setup})
# RC's dependency output lists only included headers, not the files the script loads, so a changed splash or icon
# wouldn't rebuild the resources
set_property(SOURCE ${resources} APPEND PROPERTY OBJECT_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/res/splash.bmp" "${CMAKE_CURRENT_SOURCE_DIR}/res/mafia.ico")

# Steam's German, French, Italian and Spanish setup.exe are its 2010 rebuild with other resources, written after the
# link through UpdateResource (tools/localize.py): setup_<language>/setup.exe, from this build's setup.exe and
# res/<language>.rc
if (SETUP_STEAM)
	foreach(language german french italian spanish)
		set(res "${CMAKE_CURRENT_BINARY_DIR}/${language}.res")
		add_custom_command(OUTPUT "${res}"
			COMMAND "${CMAKE_RC_COMPILER}" "/fo${res}" "${CMAKE_CURRENT_SOURCE_DIR}/res/${language}.rc"
			DEPENDS res/${language}.rc res/resource.h
			VERBATIM)
		set(localized "${CMAKE_BINARY_DIR}/setup_${language}/setup.exe")
		add_custom_command(OUTPUT "${localized}"
			COMMAND "${MAFIA_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/localize.py" "$<TARGET_FILE:${setup}>" "${res}"
				"${localized}"
			DEPENDS ${setup} "${res}" "${CMAKE_SOURCE_DIR}/tools/localize.py"
			VERBATIM)
		add_custom_target(setup_${language} ALL DEPENDS "${localized}")
	endforeach()
endif()
