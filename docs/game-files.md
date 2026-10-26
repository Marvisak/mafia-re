# Game.exe source files

A map of Game.exe's code by source file, for planning the files of `game/`. It comes from the original's static
initializer table; names come from the PS2 build as each file is worked on. It is a working map: where a boundary or a
name is uncertain, the table says so. Addresses are in `target/Game.orig.exe`. The table names the files without
folders: `game/` sorts them into folders of its own, which `game/CMakeLists.txt` lists.

## Method

- **The marker.** Every one of the game's files includes `<string>`, and with it the template static
  `std::ctype<unsigned short>::id`. VC6 gives each file that includes it a guarded initializer: it tests and sets a guard
  byte all files share (`0x6d8a48`, the COMDAT guard of a class template's static member), then jumps to a helper that
  registers the static's empty destructor helper (`0x404990`, a `ret`) with `atexit`. VC6 emits both after everything
  else in a file, with or without precompiled headers, so the helper's `ret` closes the file's code, but for one
  thing: under `/YX` the precompiled headers' inline functions that never came due in the file follow the marker
  (file 25's, `44b4d0–44c0cf`, in range 26).
- **Reading the table.** `.CRT$XCU` lists every dynamic initializer in link order. 116 of its entries are these markers,
  one per file, from `0x401000` to `0x60fc00`. The game code ends at `0x60fc30`, where LS3DF's import thunks begin.
  - **The C++ library's marker.** One more entry, `0x6101af`, tests the same guard. It is libcp's, the C++ library's: it
    lies past LS3DF's thunks among the library's objects (`operator delete`, the standard exceptions), and its code has
    the library's compiler's shape (`test byte ptr [guard], 1; jne; or byte ptr [guard], 1`, where the game's is `mov
    cl, [guard]; mov al, 1; test al, cl`).
- **The range** runs from the end of the previous file's helper to the end of this one's. Out-of-line inline and
  template functions sit in the range of the first file that has them, since the linker keeps the first copy of a
  COMDAT. The file's other `$E`s (its own globals) come where it defines them, before its functions.
- **Files without a marker.** The Rich header lists 118 objects of the game's own (`Utc12_2_CPP` 9044), so two files
  don't include `<string>`. Their code lies inside one of the ranges below, and only its code can show where. One is
  most likely `C_chunk.cpp` (our name), the chunks' `OpenFile`, which opens range 6.
- **Names.** The PS2 build (`SLUS_206.71`) names its files through `__sinit_<file>.cpp`. Its game files are in
  alphabetical order, case-insensitively, as a VC6 project lists them: `__mem_dbg__.cpp`, `_m_cache.cpp`, the `C_*`
  files, `console.cpp`, the `g_*` files, `game.cpp`, the `jp_ai_*` files, `Load_progress.cpp`, `Main.cpp`,
  `MemoryCard.cpp`, `Mission.cpp`, ... `Winstuff.cpp`; after the engine come the files added later (`g_vid.cpp`,
  `g_inventory.cpp`, the `C_Particle*` files, `_m_cache_ex.cpp`, `g_c_convex.cpp`, `g_garage.cpp`, `g_m_garage.cpp`).
  The PC's order isn't PS2's: `C_mission` (PS2 Mission.cpp) is in file 60, before `Main.cpp` (91), and the menus'
  and load/save files (PS2 g_m_carc.cpp, g_menu.cpp, g_indicators.cpp, g_loadsave.cpp) come after it. So a name in the
  table is the PS2 file of a class in the range, with a `?` where the class doesn't pin the file down.
- **Strings** don't name classes or files here: the game has no `__FILE__` strings and no `Class::Function` messages.

## The map

| # | Range | Size | File | Evidence |
| --- | --- | --- | --- | --- |
| 1 | `401000–40102c` | 0x2c | `__mem_dbg__.cpp`? | no code, only the marker. If the PC's files are in PS2's alphabetical order, this is the first one, the memory debugger a release build compiles out |
| 2 | `401030–404cdc` | 0x3cac | `m_cache.cpp`? | PS2's `C_cache_base_block` (PS2 m_cache.cpp, its helper structs' name), `base_cache` (`63d268`, PS2's name): its initializer starts the file, then PS2's functions in PS2's order, the constructor (`401070`), the destructor (`4010a0`), `Close` (`4011f0`), `PrepareScene` (`4012e0`), `cbDupl` (`401307`) and `Open` (`4013f0`), and the `Tick` GameLoopSingle calls (`4020b0`); stub of `Close`. And the first copies of `C_str`'s out-of-line members (common/C_str.h): the constructor of a `char *` (`403320`), its conversion (`4033c0`), `operator+` (`403450`), the game's wildcard `Match` (`4035d0`, game/C_fstr.h), and the cache items' constructor (`403730`) |
| 3 | `404ce0–40938c` | 0x46ac | `C_actor.cpp`? | the animations' index of PS2 C_actor.cpp, in PS2's order: `compare_anim_index` (`408690`), `InitAnimIndex` (`4086b0`), which InitSystem calls, `DoneAnimIndex` (`408750`), which CloseSystem calls, and `GetAnimIndex` (`408760`); its table `_anim_info_` (`62e488`, 576 animations) is the file's only initialized data, its literals right after it, and `_anim_index_` and `_anim_index_count_` (`63d2cc`, `63d2d0`) end its `.bss`; the caches' item vector's `_Destroy` (`409230`) and `std::copy` (`409260`), the model and sound caches' `Open` (`4087e0`, `408d90`, game/I3D_cache.h) with `C_str::ToUpper` (`408ca0`), the first copies; at its start the first copies of the chunks' `Descend` (`404ce0`) and `Read` (`404d80`) (game/C_chunk.h); written: the table, `compare_anim_index`, `InitAnimIndex` and `DoneAnimIndex` |
| 4 | `409390–40df6c` | 0x4bdc | | |
| 5 | `40df70–411bdc` | 0x3c6c | `C_animations.cpp`? | `C_animations_machine_de_luxe::tracks` (`63d2d8`, PS2 C_animations.cpp's static member): its initializer starts the file (`40df80`) and most of its uses are here; the track's destructor (`40e470`) and assignment (`4116d0`), the cache items' destructor (`410c30`), the caches' loader callback (`410c50`), the items' `insert(P, x)` (`410e80`) and a cache's `Create` (`4104f0`), the first copies |
| 6 | `411be0–41234c` | 0x76c | `C_chunk.cpp` (our name), then `C_blood.cpp` | `C_chunk::OpenFile` (`411be0`), the chunks' one method that isn't inline (LoadPatchFile calls it where it inlines the others), so most likely in a file of its own without the marker; after it the first copies of the chunks' destructor (`411c50`) and `Close` (`411d40`); written: `OpenFile`. Then PS2 C_blood.cpp's functions in PS2's order: `C_blood`'s `Tick` (`411e30`) and `Init` (`411fb0`), `C_blood_manager`'s destructor (`412000`, the copy the linker kept of the code `C_tshift`'s shares), `GameInit` (`412020`), `GameDone` (`412050`), `Tick` (`4120a0`) and `AddBlood` (`412140`), the game's `+0x2d90` calling them; its constructor's copy folded into `C_tshift`'s (`5604f0`); written: the manager's constructor and destructor |
| 7 | `412350–413d8c` | 0x1a3c | | |
| 8 | `413d90–41bb4c` | 0x7dbc | | |
| 9 | `41bb50–42e23c` | 0x126ec | `C_car.cpp`? | `C_car`'s tables (PS2's names), in PS2's order: `LoadMaterials` (`41c7f0`), `LoadParticles` (`41ca80`), `LoadTyres` (`41d480`) and `ConvertTyres` (`41d770`), which InitSystem and `C_game::Init` call, and their vectors' `insert(P, n, x)` (`42d1f0`–`42de10`) and the particles' `size` (`42e140`); most of the uses of the force feedback (`647b28`); written: the four |
| 10 | `42e240–4307cc` | 0x258c | | |
| 11 | `4307d0–430b9c` | 0x3cc | | |
| 12 | `430ba0–43286c` | 0x1ccc | | |
| 13 | `432870–433fdc` | 0x176c | | |
| 14 | `433fe0–43923c` | 0x525c | | |
| 15 | `439240–43bd8c` | 0x2b4c | | |
| 16 | `43bd90–4435ac` | 0x781c | | |
| 17 | `4435b0–443a2c` | 0x47c | | |
| 18 | `443a30–44442c` | 0x9fc | | |
| 19 | `444430–44525c` | 0xe2c | `C_ff_manager.cpp`? | the force feedback, Main.cpp's `ff_manager` (`647b28`), most likely PS2's C_ff_manager (the cars call its methods; PS2's file sits between the cars' and the scripts' as this one does): its constructor (`444630`) and `Reset` (`4446a0`, our name), which InitSystem calls; an empty method CloseSystem calls (`Close`, our name), whose call goes to the linker's folded empty function (`4aabe0`); written: `Reset`, `Close` |
| 20 | `445260–44584c` | 0x5ec | | |
| 21 | `445850–44602c` | 0x7dc | | |
| 22 | `446030–446f1c` | 0xeec | | |
| 23 | `446f20–44856c` | 0x164c | | |
| 24 | `448570–448efc` | 0x98c | | |
| 25 | `448f00–44b4cc` | 0x25cc | `C_effects.cpp` (our name) | the effects of `tables\effects.tbl` (PC only, "Effects.bin" in its messages; PS2's mission has a particle manager instead), `C_mission`'s `+0x6c` (names ours): the constructor (`449080`), the destructor (`449100`), `Load` (`449380`), `Clear` (`44a350`), and the copy of a vector's destructor the linker kept for every vector of a type without one (`44b220`); the first copies of the chunk stack's helpers, its allocator's copy constructor (`44b240`) and `insert` (`44b2a0`), which `Load` calls; after the marker, in range 26, this file's copies of the precompiled headers' inline functions (`44b4d0–44c0cf`, the effects' classes, `game/mission/C_effects.h`); written: the constructor, the destructor, `Load`, `Clear` and the classes `Load` reads |
| 26 | `44b4d0–44c51c` | 0x104c | | starts at `44c0d0` or later: the code before it is file 25's, the copies of the precompiled headers' inline functions that never came due there, which `/YX` puts after a file's marker (the chunks' `RAscend`, `Size` and `ReadString`, the effects' classes; `game/mission/C_effects.h`) |
| 27 | `44c520–459e8c` | 0xd96c | | |
| 28 | `459e90–460a1c` | 0x6b8c | | |
| 29 | `460a20–460a4c` | 0x2c | | |
| 30 | `460a50–47c31c` | 0x1b8cc | `C_program.cpp`? | the scripts (the script language's strings, PS2 C_program.cpp, which is 30th of PS2's files too): `C_program::change_progs` (`63d778`, PS2's static member, which PS2 never fills), whose initializer starts the file; `LoadPatchFile` (`460f20`, PC only, our name), which reads patch\a1.bin to a4.bin into it for InitSystem, its helper that decrypts each string (`461270`), and what frees them (`461310`, `UnloadPatchFiles`, our name), which CloseSystem calls; written: `LoadPatchFile`, its helper and `UnloadPatchFiles` |
| 31 | `47c320–47c53c` | 0x21c | | |
| 32 | `47c540–48192c` | 0x53ec | | |
| 33 | `481930–48268c` | 0xd5c | | |
| 34 | `482690–4826bc` | 0x2c | | |
| 35 | `4826c0–49908c` | 0x169cc | | |
| 36 | `499090–499b3c` | 0xaac | `C_rmorf.cpp` | PS2's file (`missions\%s\rmorf.bin`), before C_shvestky.cpp as on PS2: the initializer of a global `C_rmorf` (`63d7f0`) first, the sets' loader (`4990d0`, PS2 `RMRF_S_SETREF::LoadRAWSet`), the constructor (`499280`) and destructor (`499290`), the sets' scalar deleting destructor (`4992e0`), then the vtable's (`624b50`) `Mission_Start` (`499330`), `Mission_Done` (`499560`) and `Tick` (`4995b0`), PS2's order; the game's `+0x2bc` is one; written: the constructor, the destructor and the sets' destructor; stubs of the virtual functions |
| 37 | `499b40–4a031c` | 0x67dc | `C_shvestky.cpp` | PS2's file, the police: `C_shvestky`'s constructor (`499b40`) and destructor (`499c00`) first, the game's `+0x2da0`; written: the two |
| 38 | `4a0320–4aa2ac` | 0x9f8c | `C_sledovacka.cpp` | PS2's file, the chases: `C_sledovacka`'s constructor first (`4a0320`), the game's `+0x2b18`; its destructor is the linker's folded copy in range 19 (`444670`); the copy loops of its vectors' elements at the end (`4aa14f`–`4aa269`); written: the constructor and the destructor |
| 39 | `4aa2b0–4aa48c` | 0x1dc | | |
| 40 | `4aa490–4bc35c` | 0x11ecc | | |
| 41 | `4bc360–4bd64c` | 0x12ec | | |
| 42 | `4bd650–4d34ac` | 0x15e5c | `C_Vehicle.cpp`? | PS2 C_Vehicle.cpp's tables: the initializers of `v_tyre` (`63d9c4`), `v_materials` (`63d9b0`), `v_particle` (`63d8a0`) and two more vectors (`63d890`, `63d880`, most likely PS2's `v_light_sector` and `v_sound_sector`) start the file, after an empty one (`4bd650`); `SetTyres` (`4c5820`), `CloseTyres` (`4c5b80`), `SetVehicleMaterials` (`4c5c10`), `CloseVehicleMaterials` (`4c5c80`), the particles' setter (`4c5cc0`, PC only, `SetParticles`, our name) and `CloseParticles` (`4c5f80`), in PS2's order; `s_material_conv_tbl` (`63d8b0`) and `scene` (`63d9d4`, which `C_mission::Open` sets); written: the six and the tables |
| 43 | `4d34b0–4df99c` | 0xc4ec | | |
| 44 | `4df9a0–4eac6c` | 0xb2cc | | |
| 45 | `4eac70–4efb3c` | 0x4ecc | | |
| 46 | `4efb40–4f1b4c` | 0x200c | `controls.cpp` (our name) | the player's controls, Main.cpp's `controls` (`647c30`, PC only, names ours): its constructor (`4efb40`), destructor (`4efbd0`), `Close` (`4efc80`), the mouse's scale (`SetMouseScale`, `4efdb0`), `Init` (`4efde0`), which takes IGraph and the joysticks, then the bindings' `ClearControls` (`4f00f0`) and `SetControl` (`4f02b0`), `Update` (`4f0620`, which the menus run) and more of the bindings' functions; the joysticks' vector's `_Ucopy`, `_Ufill` and `_Construct` at the end (`4f1a80`–`4f1b13`). Written: the constructor, the destructor, `Close`, `SetMouseScale`, `Init`, `ClearControls` and `SetControl` |
| 47 | `4f1b50–4f5dbc` | 0x426c | | |
| 48 | `4f5dc0–4f6a2c` | 0xc6c | `console.cpp`? | the console's font (editfont.bmp, fnt_matrix.bin), PS2 console.cpp's functions in PS2's order: the consoles' vector's initializer and destructor first (`4f5dc0`–`4f5e1f`, PS2 `_consoles`, `646b28`), most likely `ConsoleReg` (`4f6590`), `ConsoleUnReg` (`4f67a0`) and `ConsoleRender` (`4f6850`), then `CreateFontTexture` (`4f6920`), `ConsolesInit` (`4f6980`, a tail call of it), which InitSystem calls, and `ConsolesDone` (`4f6990`), which CloseSystem calls; the text drawing before them (`4f6040`) reads the font's characters (`fnt_matrix`, `645728`); `.bss`: `fnt_matrix`, `_consoles`, `fonty` (`646b38`). Written: `CreateFontTexture`, `ConsolesInit`, `ConsolesDone` |
| 49 | `4f6a30–4f6a5c` | 0x2c | | |
| 50 | `4f6a60–4f802c` | 0x15cc | | |
| 51 | `4f8030–4f848c` | 0x45c | | |
| 52 | `4f8490–5053fc` | 0xcf6c | | |
| 53 | `505400–53038c` | 0x2af8c | | |
| 54 | `530390–53857c` | 0x81ec | | |
| 55 | `538580–538c7c` | 0x6fc | | |
| 56 | `538c80–53e4ac` | 0x582c | | |
| 57 | `53e4b0–53ef4c` | 0xa9c | | the first copy of `G_Inventory`'s destructor (`53ea20`, game/actors/g_inventory.h), which `C_game::InventoryStackClear` calls |
| 58 | `53ef50–53ef7c` | 0x2c | | |
| 59 | `53ef80–53efac` | 0x2c | | |
| 60 | `53efb0–54913c` | 0xa18c | `Mission.cpp`? | `C_mission` (PS2 Mission.cpp): its constructor (`53f010`) and destructor (`53f110`) for Main.cpp's `main_mission` (`647df8`), `Init` (`53f230`, the scene, the game and the mission's objects), `UnInit` (`53f3b0`, which CloseSystem and the destructor call), `Close` (`5405e0`, PS2's helpers `DelActors` `540240` and `FrmActBuff_ClearAll` `546b70`), `Open` (`5409d0`), `UpdateSettings` (`545a30`) and `RestoreSettings` (`545ad0`), both PS2's shapes, and the methods of its video options at `+0x90` (`545b20`–`545cb0`, the setters and `GetShadows`); the copies of `C_game`'s inline constructor (`5472f0`) and destructor (`547a60`), which `Init` and `UnInit` call, and of what they call: a vector's constructor (`5479c0`, folded), `C_Debris`'s destructor (`5479e0`, folded with `C_using_object`'s), what `Close` calls of the vectors (the roads' `erase` and `clear`, `5472a0` and `5470f0`, the caches' items' `clear`, `547230`, `C_tshift`'s `erase`, `547a10`), the caches' items' `erase` (`548330`, `UnInit`'s) and `C_Break_plain_glass`'s destructor (`5483d0`); written: the constructor, the destructor, `Init`, `UnInit`, `Close`, the video options' methods and `C_game`'s constructor and destructor (game/mission/game.h); stubs of the rest of what `main` and InitSystem call |
| 61 | `549140–549e9c` | 0xd5c | `g_m_race.cpp`? | `ExecuteRacingMenu` (`549b80`), and the copy of `GM_Menu::OnKey` the menus' vtables point to (`5498f0`, most likely a folded one); stub of `ExecuteRacingMenu` |
| 62 | `549ea0–549ecc` | 0x2c | | |
| 63 | `549ed0–549efc` | 0x2c | | |
| 64 | `549f00–549f2c` | 0x2c | | |
| 65 | `549f30–549f5c` | 0x2c | | |
| 66 | `549f60–549f8c` | 0x2c | | |
| 67 | `549f90–54d31c` | 0x338c | `pathweb.cpp` | PS2's file, which comes after Mission.cpp as this one does: `C_web_node`'s destructor first, then `C_web_path`'s destructor (`54a1e0`) and its virtual functions (`54a3b0`–`54cba0`), PS2's in PS2's order; written: the two destructors and the three empty virtual functions, whose vtable entries are folded copies elsewhere; stubs of the others |
| 68 | `54d320–55997c` | 0xc65c | `racing.cpp`? | `C_racing`'s constructor (`551830`) and destructor (`551a70`); stubs |
| 69 | `559980–559ccc` | 0x34c | | |
| 70 | `559cd0–55cf3c` | 0x326c | `road_web.cpp` | `C_roads` (PS2 road_web.cpp): its constructor first, the destructor (`559d00`), then its virtual functions (`559d40`, `55a210`, `55a620`, `55c890`); written: the constructor and the destructor; stubs of the others |
| 71 | `55cf40–55cf9c` | 0x5c | | |
| 72 | `55cfa0–56040c` | 0x346c | `tools.cpp`? | `C_sound_table` (PS2 tools.cpp): `sound_group` (`646f6c`) and `sound_table` (`646f50`), whose initializers (`55d3c0`, `55d420`) follow those of other globals (`55cfa0–55d3b0`), its constructor (`55edd0`), the destructor (`55ee00`), `FreeTables` (`55ee60`), its virtual functions in PS2's order with `LoadTables` (`55f390`) among them, `ConstructSound` (`55f820`); `C_using_object`'s methods (`55dd60`, `55df10`, the game's `+0x29ec`); the logo videos (`55d460`, PC only, `PlayLogo`, our name); written: `PlayLogo`, the constructor, the destructor, `FreeTables` and `LoadTables`; stubs of the virtual functions |
| 73 | `560410–5605bc` | 0x1ac | `tshift.cpp` | PS2's file, after tools.cpp as on PS2: `S_tshift_elem::Tick` (`560410`), `C_tshift`'s constructor (`5604f0`) and `C_tshift::Tick` (`560510`); the destructor, which PS2 has before `Tick`, is the linker's folded copy at `412000` (C_blood.cpp's); the constructor is also `C_blood_manager`'s, folded; written: the constructor and the destructor |
| 74 | `5605c0–56488c` | 0x42cc | `VE_BreakageGlass.cpp`? | `C_Break_plain_glass`'s methods (`562170`, PS2 VE_BreakageGlass.cpp, which follows tshift.cpp on PS2), the game's `+0x208`, which the cars call; nothing written (its constructor and destructor are the compiler's, game/vehicles/VE_BreakageGlass.h) |
| 75 | `564890–56615c` | 0x18cc | `VE_Debris.cpp`? | `C_Debris`'s methods (`565060`, `565720`, PS2 VE_Debris.cpp), the game's `+0x240`, which the cars call; copy loops of elements of 0x1c and 0x44 bytes (`565013`, `566103`); nothing written (game/vehicles/VE_Debris.h) |
| 76 | `566160–5661ec` | 0x8c | `WinMain.cpp` (our name) | only `WinMain` (`566160`): sets the working directory to the executable's folder and calls `main`; written |
| 77 | `5661f0–566f3c` | 0xd4c | `Winstuff.cpp`? | the startup's Windows helpers: the CD check (`566440`, which Steam's no-CD patch replaces; `a1.dta` in `\MafiaDE`, `\MafiaEN`, `\MafiaSP`, `\MafiaFR`, `\MafiaIT` or the root's `\MafiaGame`), its file test (`566410`) and the loop around it (`5663d0`, "Please insert MAFIA CD 1", also called by `C_mission::Open`), the search of the drives C: to Z: for `\MafiaGame\<file>` (`5662e0`), a `LoadString` wrapper (`5662a0`), the window's icon (`5662c0`) and a message box through IGraph's window (`5661f0`), then the three caches' `AddDir` (`5665a0`, `5667e0`, `566990`, PS2's `C_I3D_cache<T>`, whose only caller is InitSystem) with the first copies of the helpers they need (`C_rep`'s four-argument constructor `566780`, the folders' `insert` `566c60` and its `_Construct` `566f00`) and `std::string::_Grow` (`566b40`); written: the CD check, its file test and the loop (`CheckCD`, `FileExists`, `WaitForCD`), the drive search, the `LoadString` wrapper and the icon (`FindGameFile`, `LoadResString`, `SetWindowIcon`), all names ours, and the `AddDir`s (game/I3D_cache.h), which a dropped stand-in caller puts here |
| 78 | `566f40–5674ec` | 0x5ac | | |
| 79 | `5674f0–567cec` | 0x7fc | | |
| 80 | `567cf0–56ad9c` | 0x30ac | | |
| 81 | `56ada0–56adcc` | 0x2c | | |
| 82 | `56add0–56b17c` | 0x3ac | | |
| 83 | `56b180–58e34c` | 0x231cc | | |
| 84 | `58e350–58f15c` | 0xe0c | | |
| 85 | `58f160–5961fc` | 0x709c | | |
| 86 | `596200–59641c` | 0x21c | | |
| 87 | `596420–5983dc` | 0x1fbc | | |
| 88 | `5983e0–5a075c` | 0x837c | `C_cutscene.cpp` (our name) | the in-engine cutscenes (PC only, names ours; PS2 plays videos, `C_vid`, where the PC's GameLoopSingle ticks this): the keys' interpolation (`598410`, `5984e0`), `Tick` (`599610`, two tracks of keys until a time of 666.0), `Close` (`59c6a0`, which C_mission::Close, C_game::Done, the racing menu and GameLoopSingle call), the object's initializer (`59cc20`) and constructor (`59cc40`, vtable `625ad8`) mid-file; `.bss`: the object (`647900`); stub of `Close` |
| 89 | `5a0760–5bcc0c` | 0x1c4ac | `game.cpp`? | `C_game::Init` (`5a0810`, 0x314c bytes; PS2's takes two bools); the inventory, character and float stacks (PS2's `InventoryStackClear` `5b1da0`, `InventoryStackPush` `5b1e10`, `FloatStackClear` `5b2320`, `CharacterStackClear` `5b2590`); written: the three Clear functions, which the destructor calls (the constructor and the destructor are game/mission/game.h's, their copies in Mission.cpp's range); stubs of `Init` and of `Done` (`5a3c60`) and `DoneIntern` (`5a4d20`), which C_mission::Close calls |
| 90 | `5bcc10–5bcdfc` | 0x1ec | | |
| 91 | `5bce00–5c25ac` | 0x57ac | `Main.cpp` | PS2's file: the global caches' constructors and destructors first, then most likely `GameLoopSingle` (`5be750`, which `main` runs after loading each mission), `main` (`5bebe0`), the out-of-line copy of `common/settings.h`'s `S_settings` constructor (`5bf520`), `InitSystem` (`5bf5a0`) and `InitErrorMessageBox` (`5bfec0`), which are static and so follow their first caller, `CloseSystem` (`5c0080`), then the caches' templates in their vtables' slot order, model, sound, anim (`DelDir` `5c06a0`, `5c10e0`, `5c1aa0`; the items' assignment `5c2540`), and the derived caches' in-class functions (`5c23a0`–`5c2440`) and the tracks' scalar deleting destructor (`5c2520`). Between `GameLoopSingle` and `main` the copy of the in-game menu's inline `OnBeforeCreate` (`5beaf0`, "gamemenu"), the video memory overflow callback (`5beb00`, which saves the settings) and a `bool` function that returns false (`5bebd0`, which `GM_MainMenu::OnBeforeCreate` tests); after `main` and its jump table `GM_Carcyclopedia::OnBeforeCreate`'s copy (`5bf510`). Before `GameLoopSingle` the PC's joystick setup and shutdown (`5bd5f0`, `5bd870`, `InitJoy` and `CloseJoy`, our names) and the scene's callback (`5bd8d0`, which draws the HUD and the menus after the frame). `.bss`: `ff_manager` (`647b28`), `RW_data` (`647be4`, the 14 archives, whose names are the table at `637890`), `joy` (`647c1c`), `controls` (`647c30`), the three caches (`647d80`–`647dd0`), `main_mission` (`647df8`), the drivers (`647ed8`, `647edc`), `igraph`, `setting`, the joystick base (`647f24`). Written: `main`, `InitSystem`, `InitErrorMessageBox`, `CloseSystem`, `InitJoy`, `CloseJoy`, IGraph's error handler (`5beb00`) and the caches' `DelDir` (game/I3D_cache.h); `SceneCallback` and `GameLoopSingle` stubbed, and the caches' other functions |
| 92 | `5c25b0–5c283c` | 0x28c | | |
| 93 | `5c2840–5c503c` | 0x27fc | `g_colls.cpp`? | `g_collision::LoadCollision` (`5c2b70`); its `.bss` most likely holds `gifCollision` (`647f48`); stub |
| 94 | `5c5040–5c9e6c` | 0x4e2c | | |
| 95 | `5c9e70–5cafac` | 0x113c | | |
| 96 | `5cafb0–5d58fc` | 0xa94c | | |
| 97 | `5d5900–5d6e4c` | 0x154c | | |
| 98 | `5d6e50–5d6f7c` | 0x12c | | |
| 99 | `5d6f80–5d767c` | 0x6fc | | |
| 100 | `5d7680–5d87dc` | 0x115c | `g_m_carc.cpp` | `GM_Carcyclopedia` (PS2 g_m_carc.cpp): starts with its `OnCreate` (`5d7680`), its other virtual functions to `5d8450`; stubs |
| 101 | `5d87e0–5dd65c` | 0x4e7c | | |
| 102 | `5dd660–5dfdac` | 0x274c | | |
| 103 | `5dfdb0–5e063c` | 0x88c | | |
| 104 | `5e0640–5e48ec` | 0x42ac | `g_m_func.cpp`? | `GM_MainMenu` (PS2 g_m_func.cpp, the file before the PSX menus' g_m_psx.cpp): its `OnBeforeCreate` (`5e0670`, "mainmenurac" or "mainmenu"), `OnCreate` and `OnClick`; stubs |
| 105 | `5e48f0–5e491c` | 0x2c | | |
| 106 | `5e4920–5e84bc` | 0x3b9c | | `GM_MainMenu::OnChange` (`5e4a50`; stubbed in `g_m_func.cpp`) |
| 107 | `5e84c0–5e8d0c` | 0x84c | `g_m_start.cpp` | `GM_ProfileSelect` (PS2 g_m_start.cpp, the file before g_material.cpp, with `GM_ProfileCreate` and `GM_ProfileDelete`), "profileselect", which `G_LoadSaveClass::Init` runs: `OnCreate` (`5e84c0`), `OnCloseChild` (`5e86a0`), `OnTick` (`5e8760`) and `OnClick` (`5e88d0`), PS2's order; the copies of its inline `OnBeforeCreate` and `OnKey` are in g_loadsave.cpp (`605010`, `605020`); stubs |
| 108 | `5e8d10–5ece6c` | 0x415c | `g_menu.cpp` | `GM_Menu` (PS2 g_menu.cpp): its constructor (`5ea1f0`) and `ExecuteMenu` (`5eba40`); stubs |
| 109 | `5ece70–5f4b5c` | 0x7cec | `g_camera.cpp` | `G_Camera` (PS2 g_camera.cpp), the game's `+0x4c`: its constructor (`5ecf90`), destructor (`5ed000`) and `Reset` (`5ed040`), PS2's order; `.data`: the cameras (`638700`, 17 entries of 0x44 bytes, PS2 `cameraParams`), after the file's other initialized data (`6386e8`–`6386fc`); `.bss`: `needLookFromEyes` (`6bdb10`, PS2's); written: the three and `cameraParams` |
| 110 | `5f4b60–5f6a1c` | 0x1ebc | | |
| 111 | `5f6a20–604e5c` | 0xe43c | `g_indicators.cpp` | `G_IndicatorsClass` (PS2 g_indicators.cpp): `LanguageSelect` (`5f7000`, `tables\lang_xx.def` into `language_primary`, `6c42c0`, and `lang_cz.def` into `language_credits`, `6c4088`); `Create` (`5f70e0`), `Destroy` (`5f7250`) and `ResolutionChanged` (`601520`, the screens and scale for Parhelia's `parheliaIndex`, the dials' vertices through the PC's `SetRectVertices` and `SetDiamondVertices`, `6025f0` and `6026d0`, and the needles' `_SpocitejRafiku2`, `5f7680`), which the profiles' `Init` and `Close` call; ends with its PC-only Parhelia method (`604a80`); `mafiaIndicators` is `6bf980`, constructed at the file's start (constructor `5f6b70`, destructor `5f6a80`); `.data`: the needles (PS2's `tachometrDefSpeed` and `tachometrDefPalivo`, `638bb8`–`638cc0`); its `.bss` is in the reverse of PS2's definition order, as `/Yu` gives (open); written: `LoadFont`, `LanguageSelect`, `Create`, `_SpocitejRafiku2`, `ResolutionChanged`; the rest stubs |
| 112 | `604e60–60f5bc` | 0xa75c | `g_loadsave.cpp` | `G_LoadSaveClass` (PS2 g_loadsave.cpp) and PS2's `G_CarDatabase`: the initializers of `mafiaLoadSave` (`6d4628`) and `mafiaCarDatabase` (`6d4560`, PS2's names) first, then `Init` (`604ee0`), the copies of `GM_ProfileSelect`'s inline `OnBeforeCreate` and `OnKey` (`605010`, `605020`), `Close` (`605030`), `ProfileChange` (`605070`), the constructor (`605140`), `AllDefault` (`6051e0`), `ProfileEnumFiles` (`605260`), `ProfileEnumRelease` (`605530`), `ProfileLoad` (`6057f0`), `ProfileSave` (`6059f0`), `SaveGameEnumFiles` (`605b20`), `SaveGameNewGame` (`605ff0`), `SaveGameLoadHeader` (`606020`), `SaveGameLoad` (`606150`), `LanguageSelect` (`607360`, `tables\textdb_xx.def`), the PC's `SetBinding` (`6073c0`), `DefaultControls` (`607600`, the 60 controls the controls menu's pages name) and `ApplyControls` (`607a60`, names ours), `DefaultSettings` (`607ab0`, the options of the controls menu's Other page and the Audio & Video menu), `G_CarDatabase`'s constructor (`60a2f0`), `Init` (`60a350`, `tables\carindex.def` and `playerindex.def`) and `Close` (`60a420`), `G_Stream` (from `60b050`: the encrypted save files, the keys two dates), `RaceCarsEnable` (`60b610`), `RaceCarsDisable` (`60b630`), `ObjectivesClear` (`60b690`), `ObjectivesRemove` (`60b8a0`), the PC's `GarageClear` (`60b940`, name ours; the garage is in the class on PC), `ExecuteCredits` (`60e710`, `tables\credits.def`), `FreerideSetup` (`60e9d0`), `FreerideSetupAfter` (`60eef0`, Tommy and his car), the destructor (`60ef50`), then the vectors' templates; `.data`: `profileIgnoreSave` (`639320`, PS2's); `.bss`: PS2's `freeride` (`6d4540`); `mafiaLoadSave`'s `+0x4f4` (`6d4b1c`, 1 to 3) is the profile's camera, which `G_Camera::Reset` takes. Written: what InitSystem and CloseSystem reach (`Init`, `Close`, both classes' constructors and destructors, `AllDefault`, `ProfileEnumFiles`, `ProfileEnumRelease`, `SaveGameNewGame`, `SetBinding`, `DefaultControls`, `ApplyControls`, `DefaultSettings`, the car database's `Init` and `Close`, `G_Stream`'s reading, `RaceCarsEnable`, `RaceCarsDisable`, `GarageClear`) and `LanguageSelect`; the rest stubs |
| 113 | `60f5c0–60f82c` | 0x26c | `g_material.cpp` | PS2's file, between g_loadsave.cpp and g_textdb.cpp as on PS2: `LoadMaterialsFromBin` (`60f5c0`, `tables\predmety.def`, `materials.def`, `particles.def` and `voices.def`), which `G_LoadSaveClass::Init` calls, and `UnloadMaterials` (`60f780`), which `Close` calls; `.bss`: the tables and their sizes (`6d4c0c`–`6d4c28`, PS2's `gameVoices`, `gameItemTable`, `gameParticleTable` and `materialList`, in PS2's order); written: all of it |
| 114 | `60f830–60f89c` | 0x6c | | |
| 115 | `60f8a0–60f9fc` | 0x15c | | |
| 116 | `60fa00–60fc2c` | 0x22c | `g_textdb.cpp` | `G_TextDatabase` (PS2 g_textdb.cpp): `textDatabase`'s initializer, the constructor (`60fa40`), the destructor, `LoadDatabase` (`60fa60`), `Unload` (`60fb10`), `GetText` (`60fb40`), `GetTextOrNULL` (`60fba0`); `textDatabase` is `6d8714` |
