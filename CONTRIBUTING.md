# Contributing

Thank you for even considering contributing

If you have no experience with reverse engineering or don't know what tools to use, I kindly ask you to look at different resources first.

## Goal

The goal of the project is byte-exact decompilation: every binary has to rebuild, with the compiler and linker it was originally built with, into a file identical to the original. A function is done when it compiles to exactly the original instructions, not when it behaves the same. Rewriting working code until the compiler produces the same bytes is most of the work.

## Toolchains

Each binary is built with its original toolchain:

- `rw_data.dll`, `LS3DF.dll`, `Game.exe` and `setup.exe`: Visual C++ 6.0 Service Pack 5 with the Processor Pack. Not Service Pack 6, its compiler and runtime library produce different code.
- `IJoy.dll`: compiled with the same compiler, but linked with the linker and runtime library of the original Visual C++ 6.0 release.

So most of C++ features everybody is used to and comfortable with are off the table (like range-based for loops, `auto` or `nullptr`).

## Building and comparing

1. Clone the two Visual C++ 6.0 toolchains into the repository root (git ignores them):
   ```bash
   git -c core.autocrlf=false clone --depth 1 https://github.com/archaic-msvc/msvc600_sp5_vcpp.git
   git -c core.autocrlf=false clone --depth 1 https://github.com/archaic-msvc/msvc600.git
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
   - `Game.exe`: no unprotected copy survives, and Steam's is a SafeDisc unwrap. `tools/reconstruct_game_exe.py` rebuilds the original as `target/Game.orig.exe` from Steam's `Game.exe`, CD 2's `MafiaGame\Game.exe` (saved as `target/Game.cd2.exe`) and the patch exe.
4. Build:
   ```bash
   cmake --preset vc6 && cmake --build --preset vc6              # the byte-exact build, in build/vc6
   cmake --preset vc6-pdb && cmake --build --preset vc6-pdb      # the same code with PDBs, for reccmp
   ```
5. Compare with the original:
   ```bash
   cd build/vc6-pdb
   ../../.venv/Scripts/reccmp-reccmp --target RWDATA --nolib                  # score of every function
   ../../.venv/Scripts/reccmp-reccmp --target RWDATA --verbose 0x10001850     # diff of one function
   cd ../..
   .venv/Scripts/python tools/funcdiff.py dtaRead                             # what kind of differences are left
   .venv/Scripts/python tools/filediff.py                                     # the whole file: headers, layout, data
   .venv/Scripts/python tools/progress.py                                     # redraw the README's progress cards
   ```

## Debug Symbols

The USA PS2 version was compiled with debug symbols enabled, but sadly only very basic debug symbols so only the function names and global variable names are known. This is still an incredible help so I very highly recommend using them. The game wasn't compiled with RTTI so this makes finding methods much easier.

Most of the function names are written in English and those are fairly descriptive, but a big chunk of the names are written in Czech language - while those are absolutely hilarious if you understand the language, they are also absolutely unrelated to what the function does, but you should still keep the original names where possible.

## Coding Style

Ideally the goal is to mirror the coding style of the original source code, which is a bit of a problem because the game doesn't respect any. You can find basically every single naming convention in here, but it seems like most of the game sticks to some weird mixture of snake case and Hungarian notation, but you can find plenty of exceptions.
