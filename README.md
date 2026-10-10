# Mafia-RE

Decompilation of *Mafia: The City of Lost Heaven* (Illusion Softworks, 2002), PC version 1.2: the last official patch, and
the version Steam ships.

The goal is a byte-exact decompilation: source code that, built with the compilers and linkers the original binaries were
built with, produces files identical to the originals.

## Progress

| Binary | Status |
| --- | --- |
| `rw_data.dll` | Fully decompiled. Most functions already compile to the original instructions; the rest, and the data layout, are still being matched. |
| `setup.exe` | The target is the 1.2 setup, built with Visual C++ 6.0 like the game. The existing decompilation was made from Steam's 2010 rebuild (Visual Studio 2008) and still has to be redone against it. |
| `LS3DF.dll` (engine), `Game.exe` | Not started. These are the two big tasks ahead, and both will take a long time. |
| `IJoy.dll`, `INet.dll` | Not started. |

## Building

You need Windows, Git, Python 3 and CMake 3.25 or newer. Building doesn't need the game, but comparing a build with
the original binaries does: bring your own copy of version 1.2.

1. Clone the two Visual C++ 6.0 toolchains into the repository root (git ignores them):
   ```bash
   git -c core.autocrlf=false clone --depth 1 https://github.com/archaic-msvc/msvc600_sp5_vcpp.git
   git -c core.autocrlf=false clone --depth 1 https://github.com/archaic-msvc/msvc600.git
   ```
2. Install the Python tools (they include Ninja):
   ```bash
   python -m venv .venv && .venv/Scripts/python -m pip install -r requirements.txt
   ```
3. Build with Visual C++ 6.0, the toolchain the game was built with. The output goes to `build/vc6`:
   ```bash
   cmake --preset vc6 && cmake --build --preset vc6
   ```
4. `setup.exe` doesn't build with Visual C++ 6.0 yet: its source still follows Steam's 2010 rebuild. Build it with an
   installed Visual Studio from an x86 developer prompt, with the DirectX 8.1 SDK installed (`DXSDK_DIR` set). The
   output goes to `build/msvc`, and it works but doesn't match the original:
   ```bash
   cmake --preset msvc && cmake --build --preset msvc
   ```

[CONTRIBUTING.md](CONTRIBUTING.md) describes how to compare a build with the original binaries.

### setup.exe splash and icon

`setup/res/splash.bmp` and `setup/res/mafia.ico` are placeholders. The originals are the game's artwork and aren't
distributed with this repository. The setup builds and works with the placeholders, but shows a grey box instead of the
splash, and a byte-exact build needs the originals. Extract them from your own `setup.exe` (version 1.2, which is
`Patch\setup.exe` in the official 1.2 patch) over the placeholders:

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

## Contributing

Any contributions are welcome, all information for contributors is in [CONTRIBUTING.md](CONTRIBUTING.md).
