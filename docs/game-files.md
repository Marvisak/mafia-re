# Game.exe source files

A map of Game.exe's code by source file, for planning the files of `game/`. It comes from the original's static
initializer table; names come from the PS2 build as each file is worked on. It is a working map: where a boundary or a
name is uncertain, the table says so. Addresses are in `target/Game.orig.exe`.

## Method

- **The marker.** Every one of the game's files includes `<string>`, and with it the template static
  `std::ctype<unsigned short>::id`. VC6 gives each file that includes it a guarded initializer: it tests and sets a guard
  byte all files share (`0x6d8a48`, the COMDAT guard of a class template's static member), then jumps to a helper that
  registers the static's empty destructor helper (`0x404990`, a `ret`) with `atexit`. VC6 emits both after everything
  else in a file, with or without precompiled headers, so the helper's `ret` closes the file's code.
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
  don't include `<string>`. Their code lies inside one of the ranges below, and only its code can show where.
- **Names.** The PS2 build (`SLUS_206.71`) names its files through `__sinit_<file>.cpp`. Its game files are in
  alphabetical order, case-insensitively, as a VC6 project lists them: `__mem_dbg__.cpp`, `_m_cache.cpp`, the `C_*`
  files, `console.cpp`, the `g_*` files, `game.cpp`, the `jp_ai_*` files, `Load_progress.cpp`, `Main.cpp`,
  `MemoryCard.cpp`, `Mission.cpp`, ... `Winstuff.cpp`; after the engine come the files added later (`g_vid.cpp`,
  `g_inventory.cpp`, the `C_Particle*` files, `_m_cache_ex.cpp`, `g_c_convex.cpp`, `g_garage.cpp`, `g_m_garage.cpp`).
  The PC's `Main.cpp` is file 91 where PS2's is the 81st, so the PC has about ten more files before it; whether the
  PC's order is alphabetical too isn't checked yet. LS3DF's is.
- **Strings** don't name classes or files here: the game has no `__FILE__` strings and no `Class::Function` messages.

## The map

| # | Range | Size | File | Evidence |
| --- | --- | --- | --- | --- |
| 1 | `401000–40102c` | 0x2c | `__mem_dbg__.cpp`? | no code, only the marker. If the PC's files are in PS2's alphabetical order, this is the first one, the memory debugger a release build compiles out |
| 2 | `401030–404cdc` | 0x3cac | | |
| 3 | `404ce0–40938c` | 0x46ac | | |
| 4 | `409390–40df6c` | 0x4bdc | | |
| 5 | `40df70–411bdc` | 0x3c6c | | |
| 6 | `411be0–41234c` | 0x76c | | |
| 7 | `412350–413d8c` | 0x1a3c | | |
| 8 | `413d90–41bb4c` | 0x7dbc | | |
| 9 | `41bb50–42e23c` | 0x126ec | | |
| 10 | `42e240–4307cc` | 0x258c | | |
| 11 | `4307d0–430b9c` | 0x3cc | | |
| 12 | `430ba0–43286c` | 0x1ccc | | |
| 13 | `432870–433fdc` | 0x176c | | |
| 14 | `433fe0–43923c` | 0x525c | | |
| 15 | `439240–43bd8c` | 0x2b4c | | |
| 16 | `43bd90–4435ac` | 0x781c | | |
| 17 | `4435b0–443a2c` | 0x47c | | |
| 18 | `443a30–44442c` | 0x9fc | | |
| 19 | `444430–44525c` | 0xe2c | | |
| 20 | `445260–44584c` | 0x5ec | | |
| 21 | `445850–44602c` | 0x7dc | | |
| 22 | `446030–446f1c` | 0xeec | | |
| 23 | `446f20–44856c` | 0x164c | | |
| 24 | `448570–448efc` | 0x98c | | |
| 25 | `448f00–44b4cc` | 0x25cc | | |
| 26 | `44b4d0–44c51c` | 0x104c | | |
| 27 | `44c520–459e8c` | 0xd96c | | |
| 28 | `459e90–460a1c` | 0x6b8c | | |
| 29 | `460a20–460a4c` | 0x2c | | |
| 30 | `460a50–47c31c` | 0x1b8cc | | |
| 31 | `47c320–47c53c` | 0x21c | | |
| 32 | `47c540–48192c` | 0x53ec | | |
| 33 | `481930–48268c` | 0xd5c | | |
| 34 | `482690–4826bc` | 0x2c | | |
| 35 | `4826c0–49908c` | 0x169cc | | |
| 36 | `499090–499b3c` | 0xaac | | |
| 37 | `499b40–4a031c` | 0x67dc | | |
| 38 | `4a0320–4aa2ac` | 0x9f8c | | |
| 39 | `4aa2b0–4aa48c` | 0x1dc | | |
| 40 | `4aa490–4bc35c` | 0x11ecc | | |
| 41 | `4bc360–4bd64c` | 0x12ec | | |
| 42 | `4bd650–4d34ac` | 0x15e5c | | |
| 43 | `4d34b0–4df99c` | 0xc4ec | | |
| 44 | `4df9a0–4eac6c` | 0xb2cc | | |
| 45 | `4eac70–4efb3c` | 0x4ecc | | |
| 46 | `4efb40–4f1b4c` | 0x200c | | |
| 47 | `4f1b50–4f5dbc` | 0x426c | | |
| 48 | `4f5dc0–4f6a2c` | 0xc6c | | |
| 49 | `4f6a30–4f6a5c` | 0x2c | | |
| 50 | `4f6a60–4f802c` | 0x15cc | | |
| 51 | `4f8030–4f848c` | 0x45c | | |
| 52 | `4f8490–5053fc` | 0xcf6c | | |
| 53 | `505400–53038c` | 0x2af8c | | |
| 54 | `530390–53857c` | 0x81ec | | |
| 55 | `538580–538c7c` | 0x6fc | | |
| 56 | `538c80–53e4ac` | 0x582c | | |
| 57 | `53e4b0–53ef4c` | 0xa9c | | |
| 58 | `53ef50–53ef7c` | 0x2c | | |
| 59 | `53ef80–53efac` | 0x2c | | |
| 60 | `53efb0–54913c` | 0xa18c | | |
| 61 | `549140–549e9c` | 0xd5c | | |
| 62 | `549ea0–549ecc` | 0x2c | | |
| 63 | `549ed0–549efc` | 0x2c | | |
| 64 | `549f00–549f2c` | 0x2c | | |
| 65 | `549f30–549f5c` | 0x2c | | |
| 66 | `549f60–549f8c` | 0x2c | | |
| 67 | `549f90–54d31c` | 0x338c | | |
| 68 | `54d320–55997c` | 0xc65c | | |
| 69 | `559980–559ccc` | 0x34c | | |
| 70 | `559cd0–55cf3c` | 0x326c | | |
| 71 | `55cf40–55cf9c` | 0x5c | | |
| 72 | `55cfa0–56040c` | 0x346c | | |
| 73 | `560410–5605bc` | 0x1ac | | |
| 74 | `5605c0–56488c` | 0x42cc | | |
| 75 | `564890–56615c` | 0x18cc | | |
| 76 | `566160–5661ec` | 0x8c | `WinMain.cpp` (our name) | only `WinMain` (`566160`): sets the working directory to the executable's folder and calls `main`; written |
| 77 | `5661f0–566f3c` | 0xd4c | ? | the startup's Windows helpers: the CD check (`566440`, which Steam's no-CD patch replaces) and the loop around it (`5663d0`, "Please insert MAFIA CD 1"), the search of the drives C: to Z: for `\MafiaGame\<file>` (`5662e0`), a `LoadString` wrapper (`5662a0`) and the window's icon (`5662c0`) |
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
| 88 | `5983e0–5a075c` | 0x837c | | |
| 89 | `5a0760–5bcc0c` | 0x1c4ac | | |
| 90 | `5bcc10–5bcdfc` | 0x1ec | | |
| 91 | `5bce00–5c25ac` | 0x57ac | `Main.cpp` | PS2's file: the global caches' constructors and destructors first, then most likely `GameLoopSingle` (`5be750`, which `main` runs after loading each mission), `main` (`5bebe0`), the out-of-line copy of `common/settings.h`'s `S_settings` constructor (`5bf520`), `InitSystem` (`5bf5a0`) and `InitErrorMessageBox` (`5bfec0`), which are static and so follow their first caller, `CloseSystem` (`5c0080`), then the cache templates; `main` stubbed |
| 92 | `5c25b0–5c283c` | 0x28c | | |
| 93 | `5c2840–5c503c` | 0x27fc | | |
| 94 | `5c5040–5c9e6c` | 0x4e2c | | |
| 95 | `5c9e70–5cafac` | 0x113c | | |
| 96 | `5cafb0–5d58fc` | 0xa94c | | |
| 97 | `5d5900–5d6e4c` | 0x154c | | |
| 98 | `5d6e50–5d6f7c` | 0x12c | | |
| 99 | `5d6f80–5d767c` | 0x6fc | | |
| 100 | `5d7680–5d87dc` | 0x115c | | |
| 101 | `5d87e0–5dd65c` | 0x4e7c | | |
| 102 | `5dd660–5dfdac` | 0x274c | | |
| 103 | `5dfdb0–5e063c` | 0x88c | | |
| 104 | `5e0640–5e48ec` | 0x42ac | | |
| 105 | `5e48f0–5e491c` | 0x2c | | |
| 106 | `5e4920–5e84bc` | 0x3b9c | | |
| 107 | `5e84c0–5e8d0c` | 0x84c | | |
| 108 | `5e8d10–5ece6c` | 0x415c | | |
| 109 | `5ece70–5f4b5c` | 0x7cec | | |
| 110 | `5f4b60–5f6a1c` | 0x1ebc | | |
| 111 | `5f6a20–604e5c` | 0xe43c | | |
| 112 | `604e60–60f5bc` | 0xa75c | | |
| 113 | `60f5c0–60f82c` | 0x26c | | |
| 114 | `60f830–60f89c` | 0x6c | | |
| 115 | `60f8a0–60f9fc` | 0x15c | | |
| 116 | `60fa00–60fc2c` | 0x22c | | |
