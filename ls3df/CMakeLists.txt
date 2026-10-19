cmake_minimum_required(VERSION 3.25)

project(ls3df)

# The engine. The original has 61 C++ objects and one C object of its own, the sources are added file by file, in the
# original's order. Files with only STUB functions so far are there for the link.
# The folders follow C:\MyProjects\engine8, where the original was built: I3D and IGraph are in its __FILE__ strings,
# the sound files' folder is named here.
add_library(ls3df SHARED
    I3D/I3d_billboard.cpp
    I3D/I3d_camera.cpp
    I3D/I3d_driver.cpp
    I3D/I3d_dummy.cpp
    I3D/I3d_frame.cpp
    I3D/I3d_math.cpp
    I3D/I3d_mesh.cpp
    I3D/I3d_model.cpp
    I3D/I3d_object.cpp
    I3D/I3d_scene.cpp
    I3D/I3d_visual.cpp
    I3D/IBManager.cpp
    I3D/load4ds.cpp
    I3D/render.cpp
    I3D/T_l.cpp
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

# S3's S3TC library, which ITexture.cpp's CompressTexture calls. The original links it prebuilt, compiled by S3 with VC6
# SP3 (the Rich header's Utc12_C 8447 x2), so the build links the same objects, cut out of the original; newer linkers
# take them too.
set(s3tc ${CMAKE_CURRENT_SOURCE_DIR}/S3TC/s3tc.lib)
# Creative's eaxguid.lib, which the EAX GUIDs come from (the Rich header's Utc12_CPP 8799, VC6 SP4): its one object,
# the original's GUIDs laid out as Creative's (eax.h)
set(eaxguid ${CMAKE_CURRENT_SOURCE_DIR}/ISound/eaxguid.lib)

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
    # /YX - automatic precompiled headers, the default of a VC6 DLL project. They decide where the engine header's zero
    #       vector's $E and the out-of-line copies of the headers' inline functions go (see tools/autopch.py).
    set_target_properties(ls3df PROPERTIES CXX_COMPILER_LAUNCHER "${MAFIA_PYTHON};${CMAKE_SOURCE_DIR}/tools/autopch.py")

    # The original's order. Its import directory lists the libraries in the order they were searched: KERNEL32, USER32,
    # GDI32, ole32, d3d8, DINPUT8, DSOUND, WINMM and vorbisfile from the command line, then rw_data and binkw32, which
    # dta.h and IShow.cpp ask for. Its code after the engine has d3d8's thunk, dxerr8, dinput8's thunk and data formats,
    # S3TC, dsound's thunk, the base classes, then vorbisfile's thunks; its constant data dinput8's data formats,
    # dxguid's GUIDs, S3TC's literals, eaxguid's GUIDs, then strmiids'. oleaut32 is for the base classes' wxutil object,
    # whose BSTR helpers nothing calls: /OPT:REF drops them and the import (LNK4089), as in the original, which doesn't
    # import OLEAUT32.
    target_link_libraries(ls3df PRIVATE kernel32 user32 gdi32 ole32 oleaut32 d3d8 dxerr8 dinput8 dxguid ${s3tc}
        ${eaxguid} dsound winmm strmiids strmbase vorbisfile)
    # The original's link time (2003-06-24), in the PE header and the export directory
    postlink(ls3df --timestamp 0x3ef851b2)
    # Temporary, until their callers in I3D code are written: without a reference, /OPT:REF drops these (the names are
    # /Gr's). The buffer managers' m_UnlockAll*Buffers are called by the driver's Render (render.cpp),
    # RenderShadowTexture by the shadows, the visual's SetBoundVolume by the mirrors, the lod's destructor by the lit
    # objects and single meshes, the vertex transforms by the shadows' range (0x10054f60), VertexLightingProcess by the
    # lit objects' lightmaps (0x10021bb0), the camera's UpdateSector by render.cpp; the scene's view, sound and weather
    # functions by render.cpp, its shadow and mirror lists by the shadows and mirrors, MakeReadySectors by I3D code at
    # 0x10045724.
    target_link_options(ls3df PRIVATE "/INCLUDE:?m_UnlockAllIBuffers@C_IBManager@@QAEXXZ"
        "/INCLUDE:?m_UnlockAllVBuffers@C_VBManager@@QAEXXZ"
        "/INCLUDE:?RenderShadowTexture@I3D_driver@@QAE?AW4LS3D_RESULT@@PBMIPBGIPAVITexture@@0@Z"
        "/INCLUDE:?SetBoundVolume@I3D_visual@@QAEXABUI3D_bvolume@@@Z"
        "/INCLUDE:??1lod@@UAE@XZ"
        "/INCLUDE:?TransformVertices@@YIXPAXPBXIABUS_matrix@@2@Z"
        "/INCLUDE:?TransformVerticesBlend@@YIXPAXPBXIABUS_matrix@@2PBM22@Z"
        "/INCLUDE:?VertexLightingProcess@@YIXAAUS_vector@@U1@1PAVI3D_material@@PAVI3D_scene@@IPA_NPAVI3D_frame@@H@Z"
        "/INCLUDE:?UpdateSector@I3D_camera@@QAEXXZ"
        "/INCLUDE:?MakeReadySectors@I3D_scene@@QAEXXZ" "/INCLUDE:?SetViewProjMatrix@I3D_scene@@QAEXABUS_matrix@@@Z"
        "/INCLUDE:?m_UpdateSoundProperties@I3D_scene@@QAEXPAVI3D_sector@@0M@Z"
        "/INCLUDE:?AddShadow@I3D_scene@@QAEXPAVI3D_shadow@@@Z"
        "/INCLUDE:?DeleteShadow@I3D_scene@@QAE?AW4LS3D_RESULT@@PAVI3D_shadow@@@Z"
        "/INCLUDE:?AddMirror@I3D_scene@@QAEXPAVI3D_mirror@@@Z"
        "/INCLUDE:?DeleteMirror@I3D_scene@@QAE?AW4LS3D_RESULT@@PAVI3D_mirror@@@Z"
        "/INCLUDE:?m_UpdateWeatherSystem@I3D_scene@@QAEXXZ" "/INCLUDE:?m_RenderWeatherSystem@I3D_scene@@QAEXXZ")
else()
    # dxerr8.lib uses the CRT's old swprintf
    target_link_libraries(ls3df PRIVATE user32 winmm ole32 oleaut32 d3d8 dsound dinput8 ${s3tc} ${eaxguid} dxguid dxerr8
        strmiids strmbase vorbisfile legacy_stdio_definitions)
endif()

# The import libraries of binkw32.dll and vorbisfile.dll, which nobody has, from DLLs of stubs with their exports
foreach(dll binkw32 vorbisfile)
    add_library(${dll} SHARED imports/${dll}.c)
    target_link_options(${dll} PRIVATE /NOENTRY /NODEFAULTLIB)
endforeach()

# rw_data.lib, which dta.h names, is our rw_data target's import library, and binkw32.lib, which IShow.cpp names, the
# binkw32 target's
add_dependencies(ls3df rw_data binkw32)
target_link_directories(ls3df PRIVATE $<TARGET_LINKER_FILE_DIR:rw_data> $<TARGET_LINKER_FILE_DIR:binkw32>)

vc6_header_dependencies(ls3df)

# DirectShow's base classes, which IShow.cpp's texture renderer derives from: the DirectX 8.1 SDK's
# samples\Multimedia\DirectShow\BaseClasses, unchanged, built as its own project builds its release STRMBASE.lib. The
# original links seven of its objects (renbase, amfilter, ctlutil, mtype, combase, wxutil and wxlist, from 0x10086a20 on),
# compiled with VC6 SP5 without the Processor Pack: the Rich header's Utc12_CPP 8966 x7, and every one of their
# functions in the original compiles to the same bytes with that code generator (C2.DLL 12.00.8966), where the
# Processor Pack's schedules half of them differently.
set(strmbase_sources amextra amfilter amvideo combase cprop ctlutil ddmm dllentry dllsetup mtype outputq pstream
    pullpin refclock renbase schedule seekpt source strmctl sysclock transfrm transip videoctl vtrans winctrl winutil
    wxdebug wxlist wxutil)
list(TRANSFORM strmbase_sources PREPEND BaseClasses/)
list(TRANSFORM strmbase_sources APPEND .cpp)
add_library(strmbase STATIC ${strmbase_sources})
set_target_properties(strmbase PROPERTIES OUTPUT_NAME STRMBASE)
target_include_directories(strmbase PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/BaseClasses)
if (MSVC_VERSION GREATER 1500)
    target_include_directories(strmbase PRIVATE $ENV{WindowsSdkDir}/Include/$ENV{WindowsSDKVersion}/shared)
endif()
target_include_directories(strmbase PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/dx81sdk/include)
# IShow.cpp includes its streams.h
set_property(SOURCE IGraph/IShow.cpp APPEND PROPERTY INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/BaseClasses)
if (MSVC_VERSION EQUAL 1200)
    # The project's release settings (/O2 comes from cmake/vc6-flags.cmake; /Gi and /FD change nothing in the code)
    target_compile_options(strmbase PRIVATE /MT /W3 /GX)
    target_compile_definitions(strmbase PRIVATE NDEBUG WIN32 _MBCS _LIB _WIN32_DCOM WINVER=0x400)
    # And its precompiled header: dllentry.cpp creates it, the others use it
    set(strmbase_pch "${CMAKE_CURRENT_BINARY_DIR}/strmbase.pch")
    file(GLOB strmbase_headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/BaseClasses/*.h")
    set(strmbase_users ${strmbase_sources})
    list(REMOVE_ITEM strmbase_users BaseClasses/dllentry.cpp)
    set_source_files_properties(BaseClasses/dllentry.cpp PROPERTIES COMPILE_OPTIONS "/Ycstreams.h;/Fp${strmbase_pch}"
        OBJECT_OUTPUTS "${strmbase_pch}" OBJECT_DEPENDS "${strmbase_headers}")
    set_source_files_properties(${strmbase_users} PROPERTIES COMPILE_OPTIONS "/Yustreams.h;/Fp${strmbase_pch}"
        OBJECT_DEPENDS "${strmbase_pch};${strmbase_headers}")
    # SP5's driver and front end with SP5's own back end, run from a directory of their own (see ijoy/CMakeLists.txt)
    set(strmbase_cl "${CMAKE_CURRENT_BINARY_DIR}/strmbase_cl")
    file(COPY "${CMAKE_SOURCE_DIR}/3rdparty/msvc600_sp5_vcpp/VC98/Bin/CL.EXE"
        "${CMAKE_SOURCE_DIR}/3rdparty/msvc600_sp5_vcpp/VC98/Bin/C1.DLL"
        "${CMAKE_SOURCE_DIR}/3rdparty/msvc600_sp5_vcpp/VC98/Bin/C1XX.DLL"
        "${CMAKE_SOURCE_DIR}/3rdparty/msvc600_sp5_vcpp/Common/MSDev98/Bin/MSPDB60.DLL"
        "${CMAKE_SOURCE_DIR}/3rdparty/msvc600_sp5/VC98/Bin/C2.DLL" DESTINATION "${strmbase_cl}")
    set_target_properties(strmbase PROPERTIES
        CXX_COMPILER_LAUNCHER "${MAFIA_PYTHON};${CMAKE_SOURCE_DIR}/tools/clswap.py;${strmbase_cl}")
else()
    target_compile_definitions(strmbase PRIVATE NDEBUG WIN32 _LIB)
    # The 2001 code: VC6's for scope (outputq.cpp) and a declaration without a type (ctlutil.h's operator=, which
    # IShow.cpp gets through streams.h too)
    target_compile_options(strmbase PRIVATE /Zc:forScope- /wd4430)
    set_property(SOURCE IGraph/IShow.cpp APPEND PROPERTY COMPILE_OPTIONS /wd4430)
endif()
