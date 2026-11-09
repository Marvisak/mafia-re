cmake_minimum_required(VERSION 3.25)

project(game)

# The game. The original has 118 C++ objects of its own (the Rich header's Utc12_2_CPP 9044); 116 of them end with the
# initializer of std::ctype<unsigned short>::id, which every file that includes <string> gets, and those mark the files
# in the code (docs/game-files.md). The sources in the original's order, across the folders; most of them aren't written
# yet, as the game is written in the order it runs, and some only hold stubs of what the written code calls.
# The folders are ours, as no binary records the original's paths: actors/ (C_actor, the actors' animations and blood,
# the humans, the animals and the objects), vehicles/ (the cars, their physics and effects, the plane, the rail
# vehicles and their turnouts), ai/ (the walking paths' and the roads' webs, the police, the chases, the AI's humans,
# the pedestrians), collision/, input/ (the controls and the force feedback), mission/ (the mission and what it holds:
# the game, the scripts, the effects, rmorf.bin, the city's base blocks, the cutscenes), camera/ (the game's camera, the
# race cameras), ui/ (the menus, the HUD, the saved games and the console); at the top the start (WinMain, main,
# Winstuff), the chunks, the caches and the tools.
add_executable(game WIN32
    __mem_dbg__.cpp
    mission/m_cache.cpp
    actors/C_actor.cpp
    vehicles/C_airplane.cpp
    actors/C_animations.cpp
    C_chunk.cpp
    actors/C_blood.cpp
    actors/C_bottle.cpp
    actors/C_dyn_object.cpp
    vehicles/C_car.cpp
    vehicles/C_car_ex.cpp
    actors/C_cartridge.cpp
    actors/C_crash_obj.cpp
    actors/C_detector.cpp
    actors/C_dog.cpp
    actors/C_door.cpp
    ai/C_driver.cpp
    actors/C_drop_in_weapon.cpp
    input/C_ff_manager.cpp
    actors/C_grenade.cpp
    actors/C_irenka.cpp
    mission/C_Particle.cpp
    mission/C_ParticleEmitter.cpp
    mission/C_effects.cpp
    mission/C_ParticleType.cpp
    ai/C_person_traffic.cpp
    actors/C_PhThings.cpp
    mission/C_program.cpp
    actors/C_public_physics.cpp
    actors/C_pumper.cpp
    vehicles/C_railway.cpp
    mission/C_rmorf.cpp
    ai/C_shvestky.cpp
    ai/C_sledovacka.cpp
    actors/C_stone.cpp
    vehicles/C_trf_c.cpp
    vehicles/C_turnout.cpp
    vehicles/C_Vehicle.cpp
    vehicles/C_Vehicle_sounds.cpp
    input/controls.cpp
    vehicles/C_wagon.cpp
    ui/console.cpp
    ai/jp_ai_actor_history.cpp
    ai/jp_ai_checkpoints.cpp
    ai/jp_ai_entity.cpp
    ai/jp_ai_groups.cpp
    ai/jp_ai_main.cpp
    ai/jp_ai_prislusnici.cpp
    ai/jp_ai_segments.cpp
    mission/Mission.cpp
    ui/g_m_race.cpp
    ai/pathweb.cpp
    ui/racing.cpp
    ai/road_web.cpp
    tools.cpp
    mission/tshift.cpp
    vehicles/VE_BreakageGlass.cpp
    vehicles/VE_Debris.cpp
    WinMain.cpp
    Winstuff.cpp
    actors/C_actor_shot.cpp
    actors/C_box_new.cpp
    actors/C_bridge.cpp
    actors/c_clock.cpp
    actors/C_human.cpp
    actors/C_scatter.cpp
    actors/C_player.cpp
    camera/c_racecam.cpp
    vehicles/C_rail_generator.cpp
    mission/C_recorder.cpp
    mission/game.cpp
    ui/Load_progress.cpp
    Main.cpp
    mission/C_cutscene_records.cpp
    collision/g_c_glob.cpp
    collision/g_c_line.cpp
    collision/g_c_linebs.cpp
    collision/g_c_sph.cpp
    collision/g_colls.cpp
    g_lines.cpp
    actors/C_ShotSkeleton.cpp
    ui/g_m_carc.cpp
    ui/g_m_comps.cpp
    ui/g_m_comps2.cpp
    ui/g_m_compsm.cpp
    ui/g_m_func.cpp
    ui/g_m_start.cpp
    ui/g_menu.cpp
    camera/g_camera.cpp
    ui/g_dabing.cpp
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
# rw_data.lib and IJoy.lib, which rw_data.h and ijoy.h name, are our rw_data and IJoy targets' import libraries
add_dependencies(game rw_data ijoy)
target_link_directories(game PRIVATE $<TARGET_LINKER_FILE_DIR:rw_data> $<TARGET_LINKER_FILE_DIR:ijoy>)

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
    # <string> asks for) and before LIBC, by the first object (__mem_dbg__.cpp), through ijoy.h's and rw_data.h's
    # #pragma comment(lib).
    target_link_libraries(game PRIVATE kernel32 user32 advapi32 winmm ls3df)
    # Written functions whose callers aren't written yet, which /OPT:REF would drop: the scripts' InvalidateActorReg and
    # FindActorLater (C_program.cpp), and the mission's AddTimeTransparentObject and four FrmActBuff functions
    # (Mission.cpp)
    # The animations' Init of a pedestrian and of a model (C_animations.cpp)
    target_link_options(game PRIVATE /INCLUDE:?Init@C_animations_machine_de_luxe@@QAEXPAVC_traffic_element@@@Z
        /INCLUDE:?Init@C_animations_machine_de_luxe@@QAEXPAVI3D_model@@@Z)
    # The base blocks' FindModel and DestroyFrame, which C_game::DestroyControlSignal calls (m_cache.cpp)
    target_link_options(game PRIVATE /INCLUDE:?FindModel@C_cache_base_block@@QAEPAVI3D_frame@@GG@Z
        /INCLUDE:?DestroyFrame@C_cache_base_block@@QAE_NPAUS_cache_model_inbb@@@Z)
    target_link_options(game PRIVATE /INCLUDE:?InvalidateActorReg@C_program@@QAEXPAVC_actor@@@Z
        /INCLUDE:?FindActorLater@C_program@@QAEXHI@Z
        /INCLUDE:?AddTimeTransparentObject@C_mission@@QAEXPAVI3D_object@@F@Z
        /INCLUDE:?FrmActBuff_StoreModel@C_mission@@QAEXPAVI3D_model@@@Z
        /INCLUDE:?FrmActBuff_GetModel@C_mission@@QAEPAVI3D_model@@PBD@Z
        /INCLUDE:?FrmActBuff_StoreActorSound@C_mission@@QAEXPAVC_actor@@HPAVI3D_sound@@@Z
        /INCLUDE:?FrmActBuff_GetActorSound@C_mission@@QAEPAVI3D_sound@@PAVC_actor@@H@Z)
    # C_human.cpp's functions that nothing written calls yet (the AI's files, the script's commands not written yet)
    target_link_options(game PRIVATE
        /INCLUDE:?GetBlob@ai_blob_log@@QAEPAUai_blob@@I@Z
        /INCLUDE:?ThrowFromCarTruck@C_human@@QAEXUS_vector@@M@Z
        /INCLUDE:?Do_ShockStart@C_human@@QAEXH@Z
        /INCLUDE:?Do_ShockDown@C_human@@QAEXH@Z
        /INCLUDE:?Do_ShockUp@C_human@@QAEXXZ
        /INCLUDE:?Do_ShockStumble@C_human@@QAEXXZ
        /INCLUDE:?EineMeineKleineAutoInLuft@C_human@@QAEXPAVC_car@@@Z
        /INCLUDE:?Do_CornerBreath@C_human@@QAE_NW4E_corner_view@1@@Z
        /INCLUDE:?Do_CornerView@C_human@@QAE_NW4E_corner_view@1@@Z
        /INCLUDE:?Do_CornerFire@C_human@@QAE_NW4E_corner_view@1@@Z
        /INCLUDE:?Do_CornerFireAimed@C_human@@QAE_NW4E_corner_view@1@@Z
        /INCLUDE:?Force_CanWorking@C_human@@QAEXXZ
        /INCLUDE:?Turn_Turn@C_human@@QAEXXZ)
    # C_player.cpp's functions that nothing written calls yet: the car's InvalidateCarDamage
    target_link_options(game PRIVATE /INCLUDE:?InvalidateCarDamage@C_player@@QAEXPAVC_car@@@Z)
    # The collisions' TestDBox, which only a vehicle's code calls (g_colls.cpp), and with it _TestDBox and its tests
    # (g_c_glob.cpp)
    target_link_options(game PRIVATE /INCLUDE:?TestDBox@g_collision@@QAE_NABUI3D_bbox@@ABUS_matrix@@1ABUS_vector@@22II@Z)
    # game.cpp's functions that nothing written calls yet (the racing's, the scripts', the effects', the vehicles'),
    # with the functions only they call (C_bridge::SetTitanic, C_fire, DabingPauseSounds)
    target_link_options(game PRIVATE
        /INCLUDE:?SetHuman@C_game@@QAEXPAVC_actor@@@Z
        /INCLUDE:?CheatKey@C_game@@QAE_ND@Z
        /INCLUDE:?NewSmallFires@C_game@@QAEXAAUS_vector@@H@Z
        /INCLUDE:?NewFire@C_game@@QAEXAAUS_vector@@PAVI3D_frame@@HMM_N@Z
        /INCLUDE:?ClearFrameFromFireHole@C_game@@QAEXPAVI3D_frame@@@Z
        /INCLUDE:?DelFramePlacement@C_game@@QAEXPAVI3D_frame@@@Z
        /INCLUDE:?Timer_TimeOut@C_game@@QAEXXZ
        /INCLUDE:?Do_NPCKillEvent@C_game@@QAEXPAD@Z
        /INCLUDE:?AddDriverMan@C_game@@QAEXPAVI3D_model@@PAVC_car@@H_N@Z
        /INCLUDE:?DelDriverMan@C_game@@QAEXPAVI3D_model@@@Z
        /INCLUDE:?DelSemaphore@C_game@@QAEXPAVI3D_frame@@@Z
        /INCLUDE:?DelTelephones@C_game@@QAEXPAVI3D_frame@@@Z
        /INCLUDE:?PauseAllSounds@C_game@@QAEXXZ
        /INCLUDE:?PlayAllSounds@C_game@@QAEX_N@Z
        /INCLUDE:?ScratchAdd@C_game@@QAEXPAVC_car@@PAVI3D_frame@@IABUS_vector@@2@Z
        /INCLUDE:?ScratchRecompile@C_game@@QAEXPAVC_car@@PAVI3D_frame@@@Z
        /INCLUDE:?ScratchGetFreeIdx@C_game@@QAEHPAVC_car@@PAVI3D_frame@@ABUS_vector@@@Z
        /INCLUDE:?AddHydrant@C_game@@QAEXPAVI3D_frame@@@Z)
    # C_actor.cpp's functions that nothing written calls yet (Main.cpp's, the AI's, the animations'), and the records'
    # walk (C_cutscene_records.cpp), which the recorder's saves call
    target_link_options(game PRIVATE
        /INCLUDE:?InternalPlacementReset@C_actor@@QAEXXZ
        /INCLUDE:?IsPossibleCollAll@C_actor@@QAE_NUS_vector@@0MAAU2@PAPAV1@@Z
        /INCLUDE:?AddFrameForPossibleColl@C_actor@@SIXPAVI3D_frame@@M@Z
        /INCLUDE:?DelFrameForPossibleColl@C_actor@@SIXPAVI3D_frame@@@Z
        /INCLUDE:?RotateDir@@YIXAAUS_vector@@M@Z
        /INCLUDE:?GetRecordsSize@C_cutscene_records@@QAEHXZ)
    # g_loadsave.cpp's functions that nothing written calls yet: the inventory's, the car database's and the replay's
    # the menus and the races call, the saved games' list the load menu reads
    target_link_options(game PRIVATE
        /INCLUDE:?EnemyToList@G_Inventory@@QAEXPAV?$vector@PAUS_GameItem@@V?$allocator@PAUS_GameItem@@@std@@@std@@@Z
        /INCLUDE:?FreerideGetState@G_LoadSaveClass@@QAEIXZ
        /INCLUDE:?GetAmmoByID@G_Inventory@@QAEIIPAH@Z
        /INCLUDE:?GetCarClass@G_CarDatabase@@QAEII@Z
        /INCLUDE:?GetCarsByClass@G_CarDatabase@@QAEIIPAV?$vector@IV?$allocator@I@std@@@std@@PAVG_EnabledCars@@@Z
        /INCLUDE:?GetCarsByManufacturer@G_CarDatabase@@QAEIIPAV?$vector@IV?$allocator@I@std@@@std@@PAVG_EnabledCars@@@Z
        /INCLUDE:?GetPlayersByCivilMask@G_CarDatabase@@QAEIIPAV?$vector@PADV?$allocator@PAD@std@@@std@@@Z
        /INCLUDE:?GetScreenShotIdx@G_LoadSaveClass@@QAEII@Z
        /INCLUDE:?GetShadowByModel@G_CarDatabase@@QAEPBDPBD@Z
        /INCLUDE:?GetWreckageByModel@G_CarDatabase@@QAEPBDPBD@Z
        /INCLUDE:?HaveItems@G_Inventory@@QAEIPAV?$vector@US_GameItem@@V?$allocator@US_GameItem@@@std@@@std@@I@Z
        /INCLUDE:?IsEnabledForChampionship@G_CarDatabase@@QAE_NII@Z
        /INCLUDE:?IsEnabledForFreeride@G_CarDatabase@@QAE_NII@Z
        /INCLUDE:?IsEnabledForRace@G_CarDatabase@@QAE_NII@Z
        /INCLUDE:?RaceCarsAddWin@G_LoadSaveClass@@QAEXI@Z
        /INCLUDE:?RaceCarsGetEnabled@G_LoadSaveClass@@QAEIXZ
        /INCLUDE:?RemoveByPtr@G_Inventory@@QAEXPAUS_GameItem@@@Z
        /INCLUDE:?ReplayLoad@G_LoadSaveClass@@QAE_NI@Z
        /INCLUDE:?ReplaySet@G_LoadSaveClass@@QAEX_N0@Z
        /INCLUDE:?ReplayTick@G_LoadSaveClass@@QAEXAAH@Z
        /INCLUDE:?SaveGameEnumFiles@G_LoadSaveClass@@QAEPAV?$vector@UG_SaveGame_FileHead@@V?$allocator@UG_SaveGame_FileHead@@@std@@@std@@XZ
        /INCLUDE:?SaveGameLoadHeader@G_LoadSaveClass@@QAEXIPAUG_SaveGame_FileHead@@PAUG_SaveGame_Info@@@Z)
    # g_camera.cpp's functions that nothing written calls yet: the race cameras'
    # (c_racecam.cpp), the racing's (racing.cpp) and the AI's (jp_ai_entity.cpp)
    target_link_options(game PRIVATE
        /INCLUDE:?CumNaNe@G_Camera@@QAEXPAVC_human@@0@Z
        /INCLUDE:?AddRaceCamera@G_Camera@@QAEXPAUG_RaceCamera@@@Z
        /INCLUDE:?RemoveRaceCamera@G_Camera@@QAEXPAUG_RaceCamera@@@Z
        /INCLUDE:?InitRaceCameras@G_Camera@@QAEXXZ)
    # No /DEBUG (the original has no debug directory), no relocations (an EXE's default), no checksum.
    # The original's link time (2003-06-30)
    postlink(game --timestamp 0x3f004aa7)
else()
    target_link_libraries(game PRIVATE user32 advapi32 winmm ls3df ijoy)
endif()

vc6_header_dependencies(game ${CMAKE_SOURCE_DIR}/ls3df ${CMAKE_SOURCE_DIR}/ijoy ${CMAKE_SOURCE_DIR}/rw_data)
# RC's dependency output lists only included headers, not the files the script loads, so a changed icon wouldn't
# rebuild the resources
set_property(SOURCE res/game.rc APPEND PROPERTY OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/res/mafia.ico")
