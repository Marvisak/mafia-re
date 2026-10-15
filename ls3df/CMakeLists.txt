cmake_minimum_required(VERSION 3.25)

project(ls3df)

# The engine. The original has 61 C++ objects and one C object of its own, the sources are added file by file, in the
# original's order. Files with only STUB functions so far are there for the link.
# The folders follow C:\MyProjects\engine8, where the original was built: I3D and IGraph are in its __FILE__ strings,
# the sound files' folder is named here.
add_library(ls3df SHARED
    I3D/I3d_driver.cpp
    I3D/I3d_frame.cpp
    I3D/I3d_math.cpp
    I3D/IBManager.cpp
    I3D/VBManager.cpp
    IGraph/cpuid.c
    IGraph/Globals.cpp
    IGraph/IGraph.cpp
    IGraph/IShow.cpp
    IGraph/IShowStream.cpp
    IGraph/ITexture.cpp
    ISound/IS_3Dlistener.cpp
    ISound/IS_3Dsound.cpp
    ISound/IS_driver.cpp
    ISound/IS_sound.cpp
    ISound/IS_source.cpp
    S3TC/s3tc.c
)
# The export directory names the DLL
set_target_properties(ls3df PROPERTIES OUTPUT_NAME LS3DF)
# Every file includes ls3df.h, and a header from another folder by its folder (I3D/I3D.h)
target_include_directories(ls3df PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

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
    # /Ob2 - inline any suitable function, not only inline ones: ISND_3Dlistener::Release inlines the destructor, a
    #        plain function of the same file (I3D code calls it out of line)
    target_compile_options(ls3df PRIVATE /MT /GX /G6 /Ob2)
    # /Gr is a per-file option: IShow.cpp is __cdecl by default (/Gd), as its helpers take their arguments on the stack,
    # and so do vorbisfile's callbacks, which libvorbis' header declares without a calling convention
    get_target_property(ls3df_sources ls3df SOURCES)
    list(REMOVE_ITEM ls3df_sources IGraph/IShow.cpp)
    set_source_files_properties(${ls3df_sources} PROPERTIES COMPILE_OPTIONS /Gr)
    set_source_files_properties(IGraph/IShow.cpp PROPERTIES COMPILE_OPTIONS /Gd)

    # The original also imports GDI32, d3d8, DINPUT8, binkw32 and vorbisfile. Each library is added with the first
    # function that needs it: their order is still to be worked out. rw_data comes from dta.h's #pragma.
    target_link_libraries(ls3df PRIVATE gdi32 kernel32 user32 winmm ole32 d3d8 dsound dinput8 dxguid dxerr8 strmiids
        binkw32 vorbisfile)
    # The original's link time (2003-06-24), in the PE header and the export directory
    postlink(ls3df --timestamp 0x3ef851b2)
    # Temporary, until their callers in I3D and IGraph code are written: without a reference, /OPT:REF drops these
    # (the names are /Gr's; Make4X4's other callers are in camera code, the buffer managers' m_UnlockAll*Buffers
    # caller is the driver's Render, render.cpp, FindFrameFast_'s are the frames', RenderShadowTexture's the shadows', the
    # frames' are the scenes', models' and visuals')
    target_link_options(ls3df PRIVATE "/INCLUDE:?UpdateAllTerminatedBuffs@@YIXXZ" "/INCLUDE:??1ISND_3Dlistener@@QAE@XZ"
        "/INCLUDE:?dbgPrintGuid@@YIXPBX@Z" "/INCLUDE:?BBoxVertices@@YIPBUS_vector@@ABUI3D_bbox@@@Z"
        "/INCLUDE:?ErrorHandler@@YIXW4LS3D_RESULT@@W4E_SEVERITY@@PBD@Z" "/INCLUDE:?Make4X4@S_matrix@@QAEAAU1@ABU1@0@Z"
        "/INCLUDE:?UnloadManagedTextures@@YIXXZ" "/INCLUDE:?m_UnlockAllIBuffers@C_IBManager@@QAEXXZ"
        "/INCLUDE:?m_UnlockAllVBuffers@C_VBManager@@QAEXXZ" "/INCLUDE:?FindFrameFast_@I3D_driver@@QAEPAVI3D_frame@@PBDPAV2@@Z"
        "/INCLUDE:?RenderShadowTexture@I3D_driver@@QAE?AW4LS3D_RESULT@@PBMIPBGIPAVITexture@@0@Z"
        "/INCLUDE:?AddChild@I3D_frame@@QAEPAV1@PAV1@@Z" "/INCLUDE:?DeleteFromOwner@I3D_frame@@QAEXXZ"
        "/INCLUDE:?DiscardHRBoundVol@I3D_frame@@QAEXXZ" "/INCLUDE:?GetChildFrame@I3D_frame@@QAEPAV1@PBDI@Z"
        "/INCLUDE:?ResetModelName@I3D_frame@@QAE?AW4LS3D_RESULT@@XZ"
        "/INCLUDE:?SetBoundVolume@I3D_frame@@QAEXABUI3D_bbox@@@Z"
        "/INCLUDE:?SetModelName@I3D_frame@@QAE?AW4LS3D_RESULT@@PBD@Z")
else()
    # dxerr8.lib uses the CRT's old swprintf
    target_link_libraries(ls3df PRIVATE user32 winmm ole32 d3d8 dsound dinput8 dxguid dxerr8 strmiids binkw32 vorbisfile
        legacy_stdio_definitions)
endif()

# The import libraries of binkw32.dll and vorbisfile.dll, which nobody has, from DLLs of stubs with their exports
foreach(dll binkw32 vorbisfile)
    add_library(${dll} SHARED imports/${dll}.c)
    target_link_options(${dll} PRIVATE /NOENTRY /NODEFAULTLIB)
endforeach()

# rw_data.lib, which dta.h names, is our rw_data target's import library
add_dependencies(ls3df rw_data)
target_link_directories(ls3df PRIVATE $<TARGET_LINKER_FILE_DIR:rw_data>)

vc6_header_dependencies(ls3df)
