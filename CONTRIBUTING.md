# Contributing

Thank you for even considering contributing

If you have no experience with reverse engineering or don't know what tools to use, I kindly ask you to look at different resources first.

## Goal

The goal of the project is byte-exact decompilation: every binary has to rebuild, with the compiler and linker it was originally built with, into a file identical to the original. A function is done when it compiles to exactly the original instructions, not when it behaves the same. Rewriting working code until the compiler produces the same bytes is most of the work.

## Toolchains

Each binary is built with its original toolchain:

- `rw_data.dll`, `LS3DF.dll`, `Game.exe` and `setup.exe`: Visual C++ 6.0 Service Pack 5 with the Processor Pack. Not Service Pack 6, its compiler and runtime library produce different code.
- `IJoy.dll`: the original Visual C++ 6.0 release (compiler front end, linker and runtime library) with the Processor Pack's code generator. The build copies that mix of compiler files into the build tree and runs it through `tools/clswap.py`.
- `LS3DF.dll` was compiled with the compiler's automatic precompiled headers (`/YX`), which decide where some functions end up in each object. The build runs the compiler through `tools/autopch.py`, which gives every source file a precompiled header of its own.
- `LS3DF.dll` also contains DirectShow's base classes, the DirectX 8.1 SDK's sample code in `ls3df/BaseClasses/`, unchanged. They were built as a library of their own, by their own project's settings, with Service Pack 5's code generator without the Processor Pack.
- `setup.exe` also needs the headers and libraries of the DirectX 8.0 SDK, `IJoy.dll` and `LS3DF.dll` those of the DirectX 8.1 SDK.

All of them are git submodules in `3rdparty/`.

So most of C++ features everybody is used to and comfortable with are off the table (like range-based for loops, `auto` or `nullptr`).

## Building and comparing

1. Clone the repository with its submodules, keeping git from changing their line endings (in an existing clone: `git -c core.autocrlf=false submodule update --init`):
   ```bash
   git -c core.autocrlf=false clone --recurse-submodules https://github.com/Marvisak/mafia-re.git
   ```
2. Install the Python tools (Ninja and [reccmp](https://github.com/isledecomp/reccmp)):
   ```bash
   python -m venv .venv && .venv/Scripts/python -m pip install -r requirements.txt
   ```
3. Put the original binaries into `target/` and let reccmp find them:
   ```bash
   .venv/Scripts/reccmp-project detect --search-path target
   ```
   `target/` is ignored by git. The targets are the retail 1.2 files:
   - `LS3DF.dll` and `IJoy.dll`: Steam's copies are identical to retail.
   - `rw_data.dll`: the retail copy from CD 2. Steam's has a one-byte patch at file offset `0x4720`.
   - `setup.exe`: `Patch\setup.exe` from the official 1.2 patch, `mafia_patch_1.2_eng.exe`.
   - `Setup.1.0.exe`, the secondary target `SETUP10`: the `setup.exe` of the original 1.0 release (linked 2002-07-28), saved under that name; `reccmp-project.yml` has its SHA-256. The `vc6` and `vc6-pdb` builds build it too, as `setup_1_0`.
   - `setup.steam.exe`, the secondary target `SETUPSTEAM`: Steam's 2010 `setup.exe`, at the top of the game's folder. The `vs2008` build builds it.
   - `setup.<language>.exe` for `german`, `french`, `italian`, `spanish`, `russian`, `polish` and `czech`: the same `setup.exe` of Steam's version in that language (the game's language setting in Steam), saved under that name. The `vs2008` build makes the first four as `setup_<language>/setup.exe`, the `vc6` builds the others as `setup_russian`, `setup_polish` and `setup_czech`. The Polish and Czech ones are the secondary targets `SETUPPL` and `SETUPCZ`; the others have the code of `SETUP` or `SETUPSTEAM` and no reccmp targets of their own (step 5).
   - `LS3DF.1.1.dll`, the secondary target `LS3DF11`: `Patch\LS3DF.dll` from the official 1.1 patch, `mafiapatchv1.1en.exe`, saved under that name. The `vc6` and `vc6-pdb` builds build it too, as `ls3df_1_1`.
   - `Game.exe`: no unprotected copy survives, and Steam's is a SafeDisc unwrap. `tools/reconstruct_game_exe.py` rebuilds the original as `target/Game.orig.exe` from Steam's `Game.exe`, CD 2's `MafiaGame\Game.exe` (saved as `target/Game.cd2.exe`) and the patch exe.
4. Build:
   ```bash
   cmake --preset vc6 && cmake --build --preset vc6              # the byte-exact build, in build/vc6
   cmake --preset vc6-pdb && cmake --build --preset vc6-pdb      # the same code with PDBs, for reccmp
   cmake --preset vs2008 && cmake --build --preset vs2008        # Steam's 2010 setup.exe, Visual C++ 2008 SP1
   ```
   The linker stamps its output with the time of the link and the path of the PDB. The `vc6` and `vs2008` builds then write the originals' values into it (`tools/postlink.py`), so a finished binary can come out identical to the original; their PDBs no longer pair with them in a debugger. The `vc6-pdb` build keeps the linker's.
5. Compare with the original. The reccmp targets are `RWDATA`, `SETUP`, `SETUP10`, `SETUPCZ`, `SETUPPL`, `IJOY`, `LS3DF`, `LS3DF11` and `GAME` (run from `build/vc6-pdb`) and `SETUPSTEAM`, Steam's 2010 `setup.exe` (run from `build/vs2008`); funcdiff and filediff take the same names with `--target`:
   ```bash
   cd build/vc6-pdb
   ../../.venv/Scripts/reccmp-reccmp --target RWDATA --nolib                  # score of every function
   ../../.venv/Scripts/reccmp-reccmp --target RWDATA --verbose 0x10001850     # diff of one function
   cd ../..
   .venv/Scripts/python tools/funcdiff.py dtaRead                             # what kind of differences are left
   .venv/Scripts/python tools/filediff.py                                     # the whole file: headers, layout, data
   .venv/Scripts/python tools/progress.py                                     # redraw the README's progress cards
   ```
   Steam's language setups have the code of `SETUP` (Russian) or `SETUPSTEAM` (the others), so filediff compares them with that target and the original named by `--orig`:
   ```bash
   .venv/Scripts/python tools/filediff.py --target SETUP --orig target/setup.russian.exe --ours build/vc6/setup_russian/setup.exe
   .venv/Scripts/python tools/filediff.py --target SETUPSTEAM --pdb-build build/vs2008 --orig target/setup.german.exe --ours build/vs2008/setup_german/setup.exe
   ```

## Running the rebuilt Game.exe

`Game.exe` is written in the order the game runs, so a build can be tried in the game itself. It works from its own
folder, so copy `build/vc6/game/Game.exe` into the game's folder (Steam's is `steamapps\common\Mafia\Mafia`) under
another name and start it there. It is the unpatched 1.2 executable, so it needs what that one does, where Steam's
patched copy doesn't:

- The game's CD: a folder `MafiaGame` with `a1.dta` in it at the root of some drive. The first CD's image mounted, or
  a `subst` drive, will do.
- All of the game's archives, in the game's folder. Steam's install lacks `a0.dta` and `ab.dta`, which are on the third
  CD.
- The settings the 1.2 setup (`Patch\setup.exe` in the patch) saves. Steam's setup saves them where only Steam's
  `Game.exe` looks.

`tools/rungame.py` sets all of that up in `build/run`, without changing the game's install (hard links to its big
files, copies of the rest), builds the game, runs it there and prints `log.txt`, where the stubs of what isn't written
yet log their names. `--orig` runs the original instead, to compare:

```bash
.venv/Scripts/python tools/rungame.py --cd3 MAFIA_CD_3.iso --cd1 MAFIA_CD_1.iso   # the first time
.venv/Scripts/python tools/rungame.py                                             # after that
```

## Debug Symbols

The USA PS2 version was compiled with debug symbols enabled, but sadly only very basic debug symbols so only the function names and global variable names are known. This is still an incredible help so I very highly recommend using them. The game wasn't compiled with RTTI so this makes finding methods much easier.

Most of the function names are written in English and those are fairly descriptive, but a big chunk of the names are written in Czech language - while those are absolutely hilarious if you understand the language, they are also absolutely unrelated to what the function does, but you should still keep the original names where possible.

## Coding Style

Ideally the goal is to mirror the coding style of the original source code, which is a bit of a problem because the game doesn't respect any. You can find basically every single naming convention in here, so use the original's names wherever something shows them: the PS2 symbols first, then the original's own messages (`"!! m_RefCount  ITexture this:0x%x"`), then Insanity3D. Even the engine mixes conventions. Its core has snake case members (Insanity3D's, and messages like `m_numRectangles:%d < pVI->rect:%d`, whose `rect` has no prefix at all), while the parts written for Mafia (lightmaps, morphs, `IShow`, `ITexture`, IGraph's globals) use `m_` names with `p` for pointers but no other type prefix (`m_numRegions`, `m_RefCount`). A class whose own messages show such a name gets the style for all its members: `I3D_frame`'s `m_pOwner` (in `DeleteFromOwner`'s message) gave its others `m_pParent`, `m_FrmFlags`, `m_LocalMatrix`. So there is no Hungarian notation to follow everywhere: a name nothing shows is a guess in the style of the names around it.

Use C-style casts (`(uint)x`, `(LPARAM)buffer`), not `static_cast` and friends: the later engine of the same lineage, [Insanity3D](https://github.com/mice777/Insanity3D), uses them almost everywhere. Name an engine header after its module, in the casing of the original's source files (`I3D/I3d_math.h` next to `I3d_math.cpp`, `IGraph/IGraph.h` next to `IGraph.cpp`), and include it by its folder (`#include "I3D/I3D.h"`).
