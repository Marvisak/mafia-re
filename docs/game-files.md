# Game.exe source files

A map of Game.exe's code by source file, which the files in `game/` follow. Each file's header comment gives its range,
its order and what is left in it. Addresses are in `target/Game.orig.exe`. The table names the files without their
folders; `game/CMakeLists.txt` lists them in this order.

## Method

- **The marker.** Every game file includes `<string>`, which gives it a guarded initializer of the template static
  `std::ctype<unsigned short>::id`. The initializer tests a guard byte all files share (`0x6d8a48`), then jumps to a
  helper that registers an empty destructor helper (`0x404990`) with `atexit`. VC6 emits it after everything else in
  a file, so it closes the file's code. Only the precompiled headers' deferred inline functions (`/YX`) can follow it
  (see file 26).
- **The table.** 116 entries of `.CRT$XCU` are these markers, one per file, from `0x401000` to `0x60fc00`. The game's
  code ends at `0x60fc30`, where LS3DF's import thunks begin. One more entry, `0x6101af`, is the C++ library's own,
  past the thunks.
- **Ranges.** A range runs from the end of the previous file's marker to the end of this one's. Out-of-line inline
  and template functions sit in the range of the first file that has them, since the linker keeps the first copy of a
  COMDAT. A range of 0x2c bytes holds nothing but a marker: a file whose code is gone.
- **Files without a marker.** The Rich header lists 118 objects of the game's own, so two files don't include
  `<string>`. One is most likely `C_chunk.cpp` (range 6); the other hasn't been found.
- **Names.** The game has no `__FILE__` strings and no `Class::Function` messages, so the names come from the PS2
  build's `__sinit_<file>.cpp`. PS2 lists its game files alphabetically, case-insensitively, as a VC6 project does.
  The PC's order as a whole isn't PS2's (`C_mission`, PS2's Mission.cpp, comes before `Main.cpp`, and the menus after
  it), but it is alphabetical in runs: the actors' constructors put PS2's files in order through files 3 to 48 and 78
  to 87. So a PC only file within a run gets a name that sorts between its neighbours'.

## The map

*File*: a `?` marks a name that the code doesn't settle, *(ours)* a name of ours. *Written*: `whole` means every
function of the file has source, matched or not; `part` some of them; `stubs` only stubs of what written code calls;
blank nothing. *Evidence*: what identifies the file; *first* means the range starts with it.

| # | Range | Size | File | Written | Evidence |
| --- | --- | --- | --- | --- | --- |
| 1 | `401000–40102c` | 0x2c | `__mem_dbg__.cpp`? | whole | no code, only the marker: the first file in PS2's order, the memory debugger a release build compiles out. As the first object its default-library requests give the link's order (libcp, IJoy, rw_data, then LIBC) |
| 2 | `401030–404cdc` | 0x3cac | `m_cache.cpp`? | whole | `C_cache_base_block`, the city's base blocks (PS2's m_cache.cpp names its helper structs); `base_cache`'s initializer starts it. Then the first copies of `C_str`'s members and the caches' templates |
| 3 | `404ce0–40938c` | 0x46ac | `C_actor.cpp` | whole | PS2's file (`InitAnimIndex`): the actors' cutscene functions (PC only) first, the constructor `405d20` (vtable `623248`) |
| 4 | `409390–40df6c` | 0x4bdc | `C_airplane.cpp` | part | constructor `409390` (vtable `623318`) first |
| 5 | `40df70–411bdc` | 0x3c6c | `C_animations.cpp` | whole | `C_animations_machine_de_luxe`; the initializer of `tracks` first |
| 6 | `411be0–41234c` | 0x76c | `C_chunk.cpp` (ours), then `C_blood.cpp` | whole, part | `C_chunk::OpenFile` (`411be0`), the chunks' one method that isn't inline, in a file without a marker; then PS2's C_blood.cpp from `C_blood::Tick` (`411e30`) |
| 7 | `412350–413d8c` | 0x1a3c | `C_bottle.cpp` | whole | constructor `412350` (vtable `6233f0`) first |
| 8 | `413d90–41bb4c` | 0x7dbc | `C_dyn_object.cpp` (ours) | stubs | a PC only actor (type 9): constructor `418930` (vtable `6234f8`) after the vector math it starts with |
| 9 | `41bb50–42e23c` | 0x126ec | `C_car.cpp`? | part | the cars' tables (PS2's names) and `C_car`'s constructor `41bb80` (vtables `623618`, `6236b0`) |
| 10 | `42e240–4307cc` | 0x258c | `C_car_ex.cpp` | stubs | constructor `42e240` (vtable `6237d8`) first |
| 11 | `4307d0–430b9c` | 0x3cc | `C_cartridge.cpp` | whole | the cartridge cases: constructor `4307d0` (vtable `623880`, a `C_bottle`) first |
| 12 | `430ba0–43286c` | 0x1ccc | `C_crash_obj.cpp` | stubs | the pieces crashes break off: constructor `431b40` (vtable `623918`) after the helpers that create them |
| 13 | `432870–433fdc` | 0x176c | `C_detector.cpp` | whole | constructor `432880`; vtables `6239b0` and `623a48` (a `C_program` as its second base) |
| 14 | `433fe0–43923c` | 0x525c | `C_dog.cpp` | part | constructor `434030` (vtable `623a70`) |
| 15 | `439240–43bd8c` | 0x2b4c | `C_door.cpp` | part | constructor `439290` (vtable `623b20`) |
| 16 | `43bd90–4435ac` | 0x781c | `C_driver.cpp` | part | the cars' AI driver: constructor `43bd90` first |
| 17 | `4435b0–443a2c` | 0x47c | `C_drop_in_weapon.cpp` | part | the weapons put down: constructor `4435b0` first |
| 18 | `443a30–44442c` | 0x9fc |  |  |  |
| 19 | `444430–44525c` | 0xe2c | `C_ff_manager.cpp`? | part | the force feedback, Main.cpp's `ff_manager`: constructor `444630` |
| 20 | `445260–44584c` | 0x5ec | `C_grenade.cpp` | whole | constructor `445260` (vtable `623d70`) first |
| 21 | `445850–44602c` | 0x7dc | `C_irenka.cpp` | part | constructor `445850` (vtable `623e08`) first |
| 22 | `446030–446f1c` | 0xeec | `C_movement.cpp`? |  | PS2's file in this place if the order holds: quaternions, the frames' world matrices and the humans' movement |
| 23 | `446f20–44856c` | 0x164c | `C_Particle.cpp`? | whole | the effects' particles (the class's name is PS2's) |
| 24 | `448570–448efc` | 0x98c | `C_ParticleEmitter.cpp`? | whole | the effects' emitters: the destructor `448570` first |
| 25 | `448f00–44b4cc` | 0x25cc | `C_effects.cpp` (ours) | whole | the effects of `tables\effects.tbl` (PC only), where PS2's C_ParticleManager.cpp sorts: the initializers of the free lists first |
| 26 | `44b4d0–44c51c` | 0x104c | `C_ParticleType.cpp` (ours) | whole | the emitters' parts (PC only), from `44c0d0`. `44b4d0–44c0cf` is file 25's: the copies of the precompiled headers' inline functions that `/YX` puts after a file's marker |
| 27 | `44c520–459e8c` | 0xd96c | `C_person_traffic.cpp` | part | the pedestrians' `C_traffic_generator`: constructor `44d410` (vtable `623fa0`) |
| 28 | `459e90–460a1c` | 0x6b8c | `C_PhThings.cpp` | stubs | `C_PhThing`, the base of the public physics and of `C_box_new`: constructor `459e90` (vtable `624088`) first |
| 29 | `460a20–460a4c` | 0x2c |  |  |  |
| 30 | `460a50–47c31c` | 0x1b8cc | `C_program.cpp`? | part | the scripts: `change_progs`' initializer first, the constructor `460ad0` (vtable `624858`), the interpreter `Process` (`46d3b0`). `47a070–47c31c` are `Process`'s out-of-line copies of the inline functions it calls, in header order |
| 31 | `47c320–47c53c` | 0x21c | `C_public_physics.cpp` | part | constructor `47c3a0` (vtable `6248f8`) |
| 32 | `47c540–48192c` | 0x53ec | `C_pumper.cpp` | stubs | the petrol pump attendants: constructor `47c540` (vtable `624990`) first |
| 33 | `481930–48268c` | 0xd5c |  |  |  |
| 34 | `482690–4826bc` | 0x2c |  |  |  |
| 35 | `4826c0–49908c` | 0x169cc | `C_railway.cpp`? | part | constructor `4865b0` (vtable `624a70`) after functions of its own; or PS2's C_rail_traffic.cpp |
| 36 | `499090–499b3c` | 0xaac | `C_rmorf.cpp` | whole | `missions\%s\rmorf.bin`: the initializer of a global `C_rmorf` first |
| 37 | `499b40–4a031c` | 0x67dc | `C_shvestky.cpp` | part | the police: constructor `499b40` first |
| 38 | `4a0320–4aa2ac` | 0x9f8c | `C_sledovacka.cpp` | whole | the chases: constructor `4a0320` first |
| 39 | `4aa2b0–4aa48c` | 0x1dc | `C_stone.cpp` (ours) | whole | the stones (PC only, a `C_bottle`): constructor `4aa2b0` (vtable `624c00`) first |
| 40 | `4aa490–4bc35c` | 0x11ecc | `C_trf_c.cpp` | part | the traffic's `C_traffic_car`: constructor `4aa7a0` (vtable `624c98`) after the initializers of six vectors |
| 41 | `4bc360–4bd64c` | 0x12ec | `C_turnout.cpp` | part | constructor `4bc360` (vtable `624d58`) first |
| 42 | `4bd650–4d34ac` | 0x15e5c | `C_Vehicle.cpp`? | part | PS2's C_Vehicle.cpp tables (`v_tyre`, `v_materials`, `v_particle`), whose initializers come first |
| 43 | `4d34b0–4df99c` | 0xc4ec |  |  |  |
| 44 | `4df9a0–4eac6c` | 0xb2cc |  |  |  |
| 45 | `4eac70–4efb3c` | 0x4ecc | `C_Vehicle_sounds.cpp` | part | `C_Vehicle`'s sounds, in PS2's order from `InitSounds` (`4eac70`) |
| 46 | `4efb40–4f1b4c` | 0x200c | `controls.cpp` (ours) | whole | the player's controls, Main.cpp's `controls` (PC only): constructor `4efb40` first |
| 47 | `4f1b50–4f5dbc` | 0x426c | `C_wagon.cpp` | part | constructor `4f1b50` (vtable `624e78`) first |
| 48 | `4f5dc0–4f6a2c` | 0xc6c | `console.cpp`? | whole | the console and its font (editfont.bmp, fnt_matrix.bin), PS2's functions in PS2's order |
| 49 | `4f6a30–4f6a5c` | 0x2c |  |  |  |
| 50 | `4f6a60–4f802c` | 0x15cc | `jp_ai_actor_history.cpp`? | stubs | `ai_logs`, whose PS2 functions fit its size |
| 51 | `4f8030–4f848c` | 0x45c | `jp_ai_checkbuild.cpp`? |  | PS2's order |
| 52 | `4f8490–5053fc` | 0xcf6c | `jp_ai_checkpoints.cpp`? | stubs | `ai_wayfind` and `ai_path` |
| 53 | `505400–53038c` | 0x2af8c | `jp_ai_entity.cpp` | stubs | the AI's humans, `C_entity`: constructor `5071a0` (vtable `625090`) |
| 54 | `530390–53857c` | 0x81ec | `jp_ai_groups.cpp`? | part | the AI's groups, following and fights |
| 55 | `538580–538c7c` | 0x6fc | `jp_ai_main.cpp`? | whole | only `AIManager`'s initializer, constructor and destructor |
| 56 | `538c80–53e4ac` | 0x582c | `jp_ai_prislusnici.cpp`? | part | the police officers' board (`ai_police_nastenka`), in PS2's order |
| 57 | `53e4b0–53ef4c` | 0xa9c | `jp_ai_segments.cpp`? | whole | the AI's segments (`check_seg`, `check_seg_clusters`), in PS2's order |
| 58 | `53ef50–53ef7c` | 0x2c |  |  |  |
| 59 | `53ef80–53efac` | 0x2c |  |  |  |
| 60 | `53efb0–54913c` | 0xa18c | `Mission.cpp`? | whole | `C_mission`: constructor `53f010`, `Open` (`5409d0`), `OpenBin` (`541030`) |
| 61 | `549140–549e9c` | 0xd5c | `g_m_race.cpp`? | stubs | `ExecuteRacingMenu` (`549b80`) and mission 6's race. PS2 doesn't fit the name: its `ExecuteRacingMenu` isn't in its g_m_race.cpp (see 106) |
| 62 | `549ea0–549ecc` | 0x2c |  |  |  |
| 63 | `549ed0–549efc` | 0x2c |  |  |  |
| 64 | `549f00–549f2c` | 0x2c |  |  |  |
| 65 | `549f30–549f5c` | 0x2c |  |  |  |
| 66 | `549f60–549f8c` | 0x2c |  |  |  |
| 67 | `549f90–54d31c` | 0x338c | `pathweb.cpp` | part | PS2's file, after Mission.cpp as here: `C_web_node`'s and `C_web_path`'s destructors first |
| 68 | `54d320–55997c` | 0xc65c | `racing.cpp`? | part | `C_racing`'s constructor `551830` and destructor `551a70` |
| 69 | `559980–559ccc` | 0x34c |  |  |  |
| 70 | `559cd0–55cf3c` | 0x326c | `road_web.cpp` | part | `C_roads` (vtable `625370`): the constructor first |
| 71 | `55cf40–55cf9c` | 0x5c |  |  |  |
| 72 | `55cfa0–56040c` | 0x346c | `tools.cpp` | whole | PS2's file: the direction tables, the logo videos, `C_fire`, `C_using_object`, `C_sound_table` and `soundArena` |
| 73 | `560410–5605bc` | 0x1ac | `tshift.cpp` | whole | PS2's file, after tools.cpp as on PS2 |
| 74 | `5605c0–56488c` | 0x42cc | `VE_BreakageGlass.cpp`? | part | `C_Break_plain_glass`, `C_game`'s `+0x208` |
| 75 | `564890–56615c` | 0x18cc | `VE_Debris.cpp`? | part | `C_Debris`, `C_game`'s `+0x240` |
| 76 | `566160–5661ec` | 0x8c | `WinMain.cpp` (ours) | whole | only `WinMain` (`566160`) |
| 77 | `5661f0–566f3c` | 0xd4c | `Winstuff.cpp`? | whole | the start-up's Windows helpers: the CD check (`566440`), the drive search, then the caches' `AddDir` |
| 78 | `566f40–5674ec` | 0x5ac | `C_actor_shot.cpp` | part | constructor `566f40` (vtable `625458`) first |
| 79 | `5674f0–567cec` | 0x7fc | `C_box_new.cpp` | part | constructor `5674f0` (vtable `6254f0`, a `C_PhThing`) first |
| 80 | `567cf0–56ad9c` | 0x30ac | `C_bridge.cpp` | stubs | constructor `567d10` (vtable `625588`) |
| 81 | `56ada0–56adcc` | 0x2c |  |  |  |
| 82 | `56add0–56b17c` | 0x3ac | `c_clock.cpp` | whole | `C_clock`: constructor `56add0` (vtable `625648`) first |
| 83 | `56b180–58e34c` | 0x231cc | `C_human.cpp` | whole | `C_human`, the base of `C_player` and `C_entity`: constructor `56efd0` |
| 84 | `58e350–58f15c` | 0xe0c | `C_scatter.cpp` (ours) | part | a PC only actor (type 0x1d): constructor `58e350` (vtable `6257d0`) first |
| 85 | `58f160–5961fc` | 0x709c | `C_player.cpp` | whole | `C_player`: constructor `58f170` (vtable `625878`) |
| 86 | `596200–59641c` | 0x21c | `c_racecam.cpp` | part | constructor `596200` (vtable `625950`) first |
| 87 | `596420–5983dc` | 0x1fbc | `C_rail_generator.cpp` | part | constructor `596620` (vtable `6259e8`) |
| 88 | `5983e0–5a075c` | 0x837c | `C_recorder.cpp` (ours) | whole | the in-engine cutscenes (PC only; `C_recorder` in `LoadTCK`'s messages), then the seagulls (`C_seagull`, PC only) |
| 89 | `5a0760–5bcc0c` | 0x1c4ac | `game.cpp`? | whole | `C_game` |
| 90 | `5bcc10–5bcdfc` | 0x1ec | `Load_progress.cpp`? | whole | only `cbLoadScene` (`5bcc10`), a mission's loading screen |
| 91 | `5bce00–5c25ac` | 0x57ac | `Main.cpp` | whole | PS2's file: the globals' initializers, `main` and `InitSystem` (`5bf5a0`) |
| 92 | `5c25b0–5c283c` | 0x28c | `C_cutscene_records.cpp` (ours) | whole | an actor's cutscene records (PC only) |
| 93 | `5c2840–5c503c` | 0x27fc | `g_c_glob.cpp` | whole | the first of PS2's five collision files, which are files 93 to 97 in PS2's order: `gifCollision`'s initializer first, `LoadCollision` |
| 94 | `5c5040–5c9e6c` | 0x4e2c | `g_c_line.cpp` | whole | `g_collision`'s line tests |
| 95 | `5c9e70–5cafac` | 0x113c | `g_c_linebs.cpp`? | stubs | by its place: `FindCollsByFrame` (`5ca5c0`) |
| 96 | `5cafb0–5d58fc` | 0xa94c | `g_c_sph.cpp` | whole | PS2's file: `g_collision`'s sphere tests in PS2's order, `SphereInDynamics` first (PS2's `SphereInAACapsule` missing, the PC's sliding test after `_TestSphereB`) |
| 97 | `5d5900–5d6e4c` | 0x154c | `g_colls.cpp` | whole | the tests' wrappers: PS2's g_colls.cpp has exactly this range's functions |
| 98 | `5d6e50–5d6f7c` | 0x12c | `g_lines.cpp` (ours) | whole | the lines drawn over each frame (PC only) |
| 99 | `5d6f80–5d767c` | 0x6fc | `C_ShotSkeleton.cpp` (ours) | whole | `C_ShotSkeleton`, the bones the shots test against (PS2's class) |
| 100 | `5d7680–5d87dc` | 0x115c | `g_m_carc.cpp` | whole | `GM_Carcyclopedia`, the car encyclopedia |
| 101 | `5d87e0–5dd65c` | 0x4e7c | `g_m_comps.cpp` | whole | the menus' components: the factories of 29 classes first |
| 102 | `5dd660–5dfdac` | 0x274c | `g_m_comps2.cpp`? | whole | more components (the combo box, the list box, the scroll bar, the text input) |
| 103 | `5dfdb0–5e063c` | 0x88c | `g_m_compsm.cpp`? | whole | the PC's grid component |
| 104 | `5e0640–5e48ec` | 0x42ac | `g_m_func.cpp` | whole | the game's menus, from the main menu's quit question (`5e0640`) |
| 105 | `5e48f0–5e491c` | 0x2c |  |  |  |
| 106 | `5e4920–5e84bc` | 0x3b9c |  | stubs | the racing menu (vtable `625264`) and `GM_SelectCarManufacturer`: most likely PS2's g_m_race.cpp, whose first function is `GM_Racing::OnClick` too. The stubs are at g_m_func.cpp's end |
| 107 | `5e84c0–5e8d0c` | 0x84c | `g_m_start.cpp` | whole | the profiles' menu, `GM_ProfileSelect` |
| 108 | `5e8d10–5ece6c` | 0x415c | `g_menu.cpp` | whole | `GM_Menu`: `menuScreenScale`'s initializer first |
| 109 | `5ece70–5f4b5c` | 0x7cec | `g_camera.cpp` | whole | `G_Camera` |
| 110 | `5f4b60–5f6a1c` | 0x1ebc | `g_dabing.cpp`? | whole | the dubbing |
| 111 | `5f6a20–604e5c` | 0xe43c | `g_indicators.cpp` | whole | `G_IndicatorsClass`, the HUD |
| 112 | `604e60–60f5bc` | 0xa75c | `g_loadsave.cpp` | whole | `G_LoadSaveClass`, with the inventory and the garage, which PS2 has in g_inventory.cpp and g_garage.cpp |
| 113 | `60f5c0–60f82c` | 0x26c | `g_material.cpp` | whole | the tables `predmety.def`, `materials.def`, `particles.def` and `voices.def` |
| 114 | `60f830–60f89c` | 0x6c |  |  |  |
| 115 | `60f8a0–60f9fc` | 0x15c |  |  |  |
| 116 | `60fa00–60fc2c` | 0x22c | `g_textdb.cpp` | whole | `G_TextDatabase` |

## Open questions

- **Unnamed ranges.** The second file without a marker, and the files of the ranges that have no name yet.
- **`.bss` under `/Yu`.** The uninitialized globals of `g_indicators.cpp` and of the menu components' factories
  (files 101 and 102) lie in the reverse of their definition order. That is what another file's precompiled header
  (`/Yu`) gives, where ours are compiled with `/YX` and laid out by VC6's hash. Which file's header they used, and
  through which include, isn't known.
