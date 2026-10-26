cmake_minimum_required(VERSION 3.25)

project(game)

# The game. The original has 118 C++ objects of its own (the Rich header's Utc12_2_CPP 9044); 116 of them end with the
# initializer of std::ctype<unsigned short>::id, which every file that includes <string> gets, and those mark the files
# in the code (docs/game-files.md). The sources in the original's order, across the folders; most of them aren't written
# yet, as the game is written in the order it runs, and some only hold stubs of what the written code calls.
# The folders are ours, as no binary records the original's paths: actors/ (C_actor, the actors' animations and blood),
# vehicles/ (the cars, their physics and effects), ai/ (the walking paths' and the roads' webs, the police, the chases),
# collision/, input/ (the controls and the force feedback), mission/ (the mission and what it holds: the game, the
# scripts, the effects, rmorf.bin, the city's base blocks, the cutscenes), camera/ (the game's camera), ui/ (the menus, the HUD, the saved games and the
# console); at the top the start (WinMain, main, Winstuff), the chunks, the caches and the tools.
add_executable(game WIN32
    mission/m_cache.cpp
    actors/C_actor.cpp
    actors/C_animations.cpp
    C_chunk.cpp
    actors/C_blood.cpp
    vehicles/C_car.cpp
    input/C_ff_manager.cpp
    mission/C_effects.cpp
    mission/C_program.cpp
    mission/C_rmorf.cpp
    ai/C_shvestky.cpp
    ai/C_sledovacka.cpp
    vehicles/C_Vehicle.cpp
    vehicles/C_Vehicle_sounds.cpp
    input/controls.cpp
    ui/console.cpp
    mission/Mission.cpp
    ui/g_m_race.cpp
    ai/pathweb.cpp
    ui/racing.cpp
    ai/road_web.cpp
    tools.cpp
    mission/tshift.cpp
    WinMain.cpp
    Winstuff.cpp
    mission/C_cutscene.cpp
    mission/game.cpp
    Main.cpp
    collision/g_colls.cpp
    ui/g_m_carc.cpp
    ui/g_m_func.cpp
    ui/g_m_start.cpp
    ui/g_menu.cpp
    camera/g_camera.cpp
    ui/g_indicators.cpp
    ui/g_loadsave.cpp
    g_material.cpp
    ui/g_textdb.cpp
    res/game.rc
)
# The resources and the version information name the executable
set_target_properties(game PROPERTIES OUTPUT_NAME Game)
# A game file includes the game's headers by their folder (ui/g_menu.h; those at the top by name), the engine's by
# theirs (I3D/I3D.h), and IJoy's and rw_data's own
target_include_directories(game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/ls3df ${CMAKE_SOURCE_DIR}/ijoy
    ${CMAKE_SOURCE_DIR}/rw_data)
# The engine's headers include the DirectX 8.1 SDK's, which come before the compiler's own (DirectX 6's). The SDK ships
# an old basetsd.h, so newer compilers need the Windows SDK's headers first.
if (MSVC_VERSION GREATER 1500)
    target_include_directories(game PRIVATE $ENV{WindowsSdkDir}/Include/$ENV{WindowsSDKVersion}/shared)
endif()
target_include_directories(game PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/dx81sdk/include)
# rw_data.lib, which rw_data.h names, is our rw_data target's import library
add_dependencies(game rw_data)
target_link_directories(game PRIVATE $<TARGET_LINKER_FILE_DIR:rw_data>)

if (MSVC_VERSION EQUAL 1200)
    # Original compile flags (/O2 comes from cmake/vc6-flags.cmake)
    # /ML - static single-threaded CRT (LIBC.LIB): no TLS functions among the imports
    # /GX - C++ exception handling: main's EH frame and the unwind funclets after the game code
    # /Gr - __fastcall: free functions take their first two arguments in ecx and edx (InitSystem); main stays __cdecl
    #       (VC6 compiles main __cdecl under any convention), WinMain is WINAPI
    # /GB, the default: the code has lea [x+x*n] chains and no npad inside functions, which /G6 (LS3DF's) gives
    # /Ob1, /O2's default: CheckCD calls FileExists, defined right before it, out of line, where /Ob2 (LS3DF's) inlines
    # it at all six calls
    target_compile_options(game PRIVATE /ML /GX /Gr)
    # /YX - automatic precompiled headers, the default of a VC6 project, as LS3DF's (tools/autopch.py): C_effects.cpp's
    #       copies of its headers' inline functions come after the end marker of its range, as /YX defers the inline
    #       functions of the precompiled headers that never come due in a file (game/mission/C_effects.h)
    set_target_properties(game PROPERTIES CXX_COMPILER_LAUNCHER "${MAFIA_PYTHON};${CMAKE_SOURCE_DIR}/tools/autopch.py")
    # The original's import directory lists KERNEL32, USER32, ADVAPI32, WINMM, LS3DF, IJoy and rw_data, in the order LINK
    # searched them. Its code has LS3DF's thunks right after the game code (0x60fc30), then libcp's objects, then IJoy's and
    # rw_data's thunks (0x6101f2), then LIBC's: so IJoy and rw_data were default libraries, asked for after libcp (which
    # <string> asks for) and before LIBC, by the first object. rw_data comes from rw_data.h's #pragma comment(lib);
    # IJoy has no such request yet, so it is on the command line.
    target_link_libraries(game PRIVATE kernel32 user32 advapi32 winmm ls3df ijoy)
    # No /DEBUG (the original has no debug directory), no relocations (an EXE's default), no checksum.
    # The original's link time (2003-06-30)
    postlink(game --timestamp 0x3f004aa7)
else()
    target_link_libraries(game PRIVATE user32 advapi32 winmm ls3df ijoy)
endif()

vc6_header_dependencies(game ${CMAKE_SOURCE_DIR}/ls3df ${CMAKE_SOURCE_DIR}/ijoy ${CMAKE_SOURCE_DIR}/rw_data)
# RC's dependency output lists only included headers, not the files the script loads, so an icon extracted over the
# placeholder (README) wouldn't rebuild the resources
set_property(SOURCE res/game.rc APPEND PROPERTY OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/res/mafia.ico")
