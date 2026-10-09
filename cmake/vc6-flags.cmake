# Loaded through CMAKE_USER_MAKE_RULES_OVERRIDE, after CMake's own MSVC defaults.
# Those defaults (/DWIN32 /D_WINDOWS /EHsc, /Ob2 /DNDEBUG, /machine:X86, a list of default import
# libraries) would change the generated code and the link inputs, so start from nothing and let
# every target state its flags.

# Release is the byte-exact build. RelWithDebInfo is the same code plus a PDB for reccmp, /Zi
# comes from CMAKE_MSVC_DEBUG_INFORMATION_FORMAT. Only .rdata addresses differ between the two
# (the debug directory), so RelWithDebInfo output never matches the original file.
foreach(lang C CXX)
    set(CMAKE_${lang}_FLAGS_INIT "")
    set(CMAKE_${lang}_FLAGS_RELEASE_INIT "/O2")
    set(CMAKE_${lang}_FLAGS_RELWITHDEBINFO_INIT "/O2")
    set(CMAKE_${lang}_STANDARD_LIBRARIES_INIT "")
endforeach()

# /MAP doesn't change the image, the map file is what rebuilt functions are compared by.
# /DEBUG switches LINK to /OPT:NOREF and incremental linking, so put the release defaults back.
foreach(type EXE SHARED MODULE)
    set(CMAKE_${type}_LINKER_FLAGS_INIT "/MAP")
    set(CMAKE_${type}_LINKER_FLAGS_RELEASE_INIT "")
    set(CMAKE_${type}_LINKER_FLAGS_RELWITHDEBINFO_INIT "/DEBUG /OPT:REF /INCREMENTAL:NO")
endforeach()
