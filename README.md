# Mafia-RE

Decompilation of *Mafia: The City of Lost Heaven* (Illusion Softworks, 2002), PC version 1.2: the last official patch, and
the version Steam ships.

The goal is a byte-exact decompilation: source code that, built with the compilers and linkers the original binaries were
built with, produces files identical to the originals.

## Progress

<img src="docs/progress/Game.svg" width="50%"><img src="docs/progress/LS3DF.svg" width="50%">
<img src="docs/progress/rw_data.svg" width="50%"><img src="docs/progress/IJoy.svg" width="50%">
<img src="docs/progress/setup.svg" width="50%"><img src="docs/progress/setup.steam.svg" width="50%">

The cards count each binary's own functions, without the C runtime linked into it. *Implemented* is how many of them
have source, *accuracy* is [reccmp](https://github.com/isledecomp/reccmp)'s average score for those, and the bar is the
score summed over all of them. The function counts of binaries that haven't been started are estimates.
*setup.exe (Steam 2010)* is Steam's rebuild of the setup, built from the same source with Visual C++ 2008.

| Binary | Status |
| --- | --- |
| `rw_data.dll` | Done. The Visual C++ 6.0 build is identical to the original, byte for byte. |
| `setup.exe` | Fully decompiled. With the original splash and icon ([below](#setupexe-splash-and-icon)), the Visual C++ 6.0 build is identical to the 1.2 setup. The same source, with Steam's changes behind `SETUP_STEAM`, builds Steam's 2010 rebuild with Visual C++ 2008 SP1: every function matches, and the file differs only in one size field of its relocation section, and so in its checksum. That size depends on code in the 2010 source that the linker discarded, which hasn't been recovered. |
| `LS3DF.dll` (engine) | Every source file is written: the 3D engine (frames and visuals, meshes, objects and lightmapped objects, models, animation, cameras, lights, particles, portals and sectors, shadows and projectors, collision, the scene loaders, the renderer, the driver with its vertex and index buffers, and the math), the graphics layer (the Direct3D device, textures, and video through Bink and DirectShow) and the sound driver. About nine in ten of its functions compile to the original instructions. The others differ mostly in register allocation and in what the compiler inlines, and until they match the file differs from the original too, its code shifted by their sizes. The libraries it links are in place: S3's texture compressor comes from the original's own objects, and the DirectX 8.1 SDK's DirectShow base classes build from the SDK's source to the original's bytes. |
| `Game.exe` | Started. The build is in place: the entry point compiles to the original's instructions, the C runtime is linked as in the original, and the resources are the original's apart from the icon ([below](#gameexe-icon)). The rest is written in the order the game runs, so that the rebuilt executable gets further with each step: first through the start-up to the main menu. With about 5,000 functions it is the biggest task ahead. No unprotected copy of the 1.2 executable survives, so the reference it is compared with is rebuilt from the retail copies ([CONTRIBUTING.md](CONTRIBUTING.md)). |
| `IJoy.dll` | Done. Every function compiles to the original instructions, and linked on Windows 2000 the build is identical to the original. On current Windows the order of the imports within each DLL differs, because the linker sorts them with the system's `msvcrt.dll`, whose sort has changed since. It is built with the original Visual C++ 6.0 release and the Processor Pack's code generator, as the original was. |

## Building

You need Windows, Git, Python 3 and CMake 3.29 or newer. Building doesn't need the game, but comparing a build with
the original binaries does: bring your own copy of version 1.2.

1. Clone the repository with its submodules, keeping git from changing their line endings:
   ```bash
   git -c core.autocrlf=false clone --recurse-submodules https://github.com/Marvisak/mafia-re.git
   ```
   In a clone made without them, run `git -c core.autocrlf=false submodule update --init`. The submodules in
   `3rdparty/` are the build tools the original binaries were made with:
   - `msvc600_sp5_vcpp`: Visual C++ 6.0 with Service Pack 5 and the Processor Pack, from
     [archaic-msvc](https://github.com/archaic-msvc/msvc600_sp5_vcpp).
   - `msvc600`: the original Visual C++ 6.0 release, which builds `IJoy.dll` (with the Processor Pack's code
     generator from `msvc600_sp5_vcpp`), also from
     [archaic-msvc](https://github.com/archaic-msvc/msvc600).
   - `msvc600_sp5`: Visual C++ 6.0 with Service Pack 5 but without the Processor Pack, whose code generator builds the
     DirectShow base classes in `LS3DF.dll`, also from [archaic-msvc](https://github.com/archaic-msvc/msvc600_sp5).
   - `msvc900_sp1`: Visual C++ 2008 SP1 with the Windows SDK 6.0A, which Steam's 2010 `setup.exe` was built with, from
     [Marvisak/msvc900_sp1](https://github.com/Marvisak/msvc900_sp1).
   - `dx8sdk`: the headers and libraries of the DirectX 8.0 SDK, which `setup.exe` was built with, from
     [anominy/dx8sdk](https://github.com/anominy/dx8sdk).
   - `dx81sdk`: the headers and libraries of the DirectX 8.1 SDK, which Steam's 2010 `setup.exe`, `IJoy.dll` and
     `LS3DF.dll` were built with, from [Marvisak/dx81sdk](https://github.com/Marvisak/dx81sdk).
2. Install the Python tools (they include Ninja):
   ```bash
   python -m venv .venv && .venv/Scripts/python -m pip install -r requirements.txt
   ```
3. Build with Visual C++ 6.0, the toolchain the game was built with. The output goes to `build/vc6`:
   ```bash
   cmake --preset vc6 && cmake --build --preset vc6
   ```
4. Optionally, build with an installed Visual Studio from an x86 developer prompt. The output goes to `build/msvc`, and
   it works but doesn't match the original:
   ```bash
   cmake --preset msvc && cmake --build --preset msvc
   ```
   Add `-DSETUP_STEAM=ON` to the first command to build Steam's 2010 `setup.exe` instead of the 1.2 one.
5. Optionally, build Steam's 2010 `setup.exe` with Visual C++ 2008 SP1, the toolchain it was built with. The output
   goes to `build/vs2008`:
   ```bash
   cmake --preset vs2008 && cmake --build --preset vs2008
   ```

[CONTRIBUTING.md](CONTRIBUTING.md) describes how to compare a build with the original binaries.

### setup.exe splash and icon

`setup/res/splash.bmp` and `setup/res/mafia.ico` are placeholders. The originals are the game's artwork and aren't
distributed with this repository. The setup builds and works with the placeholders, but shows a grey box instead of the
splash, and a byte-exact build needs the originals. Extract them over the placeholders from your own `setup.exe`:
version 1.2's (`Patch\setup.exe` in the official 1.2 patch), or Steam's (the one at the top of the game's folder),
which has the same artwork:

- The splash, with [7-Zip](https://www.7-zip.org/):
  ```bash
  7z e -osetup/res path/to/setup.exe '.rsrc\1029\BITMAP\108.bmp' && mv setup/res/108.bmp setup/res/splash.bmp
  ```
- The icon: save icon group 1 (32×32 and 16×16 in one file) as `setup/res/mafia.ico` with a resource editor such as
  Resource Hacker. 7-Zip exports the two sizes as separate files, which won't do.

Don't commit the extracted files. Git shows them as modified; this hides them:

```bash
git update-index --skip-worktree setup/res/splash.bmp setup/res/mafia.ico
```

### Game.exe icon

`game/res/mafia.ico` is a placeholder too: `Game.exe`'s icon is the game's artwork. Its 32×32 image isn't the setup's, so
it needs extracting from `Game.exe` itself: save icon group 1 (32×32 and 16×16 in one file) as `game/res/mafia.ico`
with a resource editor, from Steam's `Game.exe` (in the game's folder) or from the reconstructed one. Hide it from git
the same way:

```bash
git update-index --skip-worktree game/res/mafia.ico
```

## Contributing

Any contributions are welcome, all information for contributors is in [CONTRIBUTING.md](CONTRIBUTING.md).
