"""Copy files out of the game's archives (the .dta files), and show a menu with its texts.

Much of what the code doesn't name is in the game's data: the menus (tables\\menu\\<name>.mnu, the name being what a
menu's OnBeforeCreate returns) and their texts (tables\\textdb_en.def) named the video options of the settings, the
labels of the Audio & Video menu. The archives are encrypted, so the tool reads them as the game does: a small 32-bit
helper (tools/dtadump.cpp, built with the repository's VC6 into build/dtadump) loads the vc6 build's rw_data.dll,
which is the retail one byte for byte, opens the archives with dtaCreate, unlocks each with the key Game.exe's
InitSystem passes and reads the files through dtaOpen and dtaRead. rw_data can't list an archive's files, so they go
by name, as the code names them (Game.exe's strings: "tables\\menu\\%s.mnu", "tables\\textdb_xx.def", ...).

The archives are taken from build/run, which tools/rungame.py sets up with all of them, or from the game's install
(--game-dir; Steam's lacks a0.dta and ab.dta).

    .venv/Scripts/python tools/dtadump.py tables/textdb_en.def tables/menu/optionsa.mnu     # into build/dta
    .venv/Scripts/python tools/dtadump.py --menu optionsa mainmenu        # menus' items with their texts

The formats as far as read:
- a menu (.mnu): "Menu", a version (1), the item count, a dword, then 32 bytes per item: its id (what the menu's code
  passes to address it), its type as four characters stored backwards (butt, stxt, slid, bsel, wind, line, ...), its
  position and size as four floats, the id of its text and a dword of flags.
- the text database (textdb_xx.def): the count of texts, a dword, then an id and an offset into the file per text, each
  text a zero-terminated string (Windows-1252 for English).
"""

import argparse
import os
import struct
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
VC6 = REPO / "3rdparty" / "msvc600_sp5_vcpp"
HELPER_SOURCE = REPO / "tools" / "dtadump.cpp"
HELPER = REPO / "build" / "dtadump" / "dtadump.exe"
OUT = REPO / "build" / "dta"
RUN = REPO / "build" / "run"
STEAM_GAME = Path(r"C:\Program Files (x86)\Steam\steamapps\common\Mafia\Mafia")
RW_DATA = [REPO / "build" / "vc6" / "rw_data" / "rw_data.dll", REPO / "target" / "rw_data.dll"]


def build_helper():
    """Builds the helper with VC6, the vc6 preset's environment, when it is missing or older than its source."""
    if HELPER.exists() and HELPER.stat().st_mtime >= HELPER_SOURCE.stat().st_mtime:
        return
    HELPER.parent.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    env["PATH"] = os.pathsep.join([str(VC6 / "VC98" / "Bin"), str(VC6 / "Common" / "MSDev98" / "Bin"), env["PATH"]])
    env["INCLUDE"] = str(VC6 / "VC98" / "Include")
    env["LIB"] = str(VC6 / "VC98" / "Lib")
    # By its path: Windows looks a program up on this process's PATH, not the one passed
    result = subprocess.run([str(VC6 / "VC98" / "Bin" / "CL.EXE"), "/nologo", "/O1", "/GX", str(HELPER_SOURCE), f"/Fe{HELPER}",
                             f"/Fo{HELPER.with_suffix('.obj')}"], cwd=HELPER.parent, env=env, capture_output=True,
                            text=True, errors="replace")
    if result.returncode:
        sys.exit("building the helper failed:\n" + result.stdout + result.stderr)


def game_dir(given: Path | None) -> Path:
    if given:
        return given
    return RUN if (RUN / "a8.dta").exists() else STEAM_GAME


def dump(names: list[str], game: Path, out: Path, quiet: bool = False) -> bool:
    build_helper()
    rw_data = next((p for p in RW_DATA if p.exists()), None)
    if not rw_data:
        sys.exit("no rw_data.dll: build the vc6 preset")
    if not any(game.glob("*.dta")):
        sys.exit(f"no archives in {game}; pass --game-dir")
    result = subprocess.run([str(HELPER), str(rw_data), str(out.resolve())] + names, cwd=game,
                            capture_output=True, text=True, errors="replace")
    if not quiet or result.returncode:
        print(result.stdout.rstrip())
    return result.returncode == 0


def read_texts(path: Path) -> dict[int, str]:
    data = path.read_bytes()
    texts = {}
    for i in range(struct.unpack_from("<I", data, 0)[0]):
        text_id, offset = struct.unpack_from("<II", data, 8 + i * 8)
        texts[text_id] = data[offset:data.index(b"\0", offset)].decode("cp1252", "replace")
    return texts


def show_menu(path: Path, texts: dict[int, str]):
    data = path.read_bytes()
    count = struct.unpack_from("<I", data, 8)[0]
    print(f"{path.stem}: {count} items")
    print("    id  type       x      y      w      h  text")
    for k in range(count):
        offset = 0x10 + k * 32
        if offset + 32 > len(data):
            break
        item_id, kind, x, y, w, h, text_id, flags = struct.unpack_from("<I4s4fII", data, offset)
        text = f"{text_id:#x} {texts.get(text_id, '')!r}" if text_id else ""
        print(f"  {item_id:4x}  {kind[::-1].decode('ascii', 'replace')}  {x:5.0f}  {y:5.0f}  {w:5.0f}  {h:5.0f}  {text}")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("names", nargs="*", help="files in the archives, as the code names them (tables\\load.def)")
    ap.add_argument("--menu", nargs="+", metavar="NAME", help="menus to show with their texts (optionsa)")
    ap.add_argument("--game-dir", type=Path, help="the folder with the archives (default: build/run, else Steam's)")
    ap.add_argument("--out", type=Path, default=OUT, help="where the files go (default: build/dta)")
    args = ap.parse_args()
    if not args.names and not args.menu:
        ap.error("name files to copy out, or menus to show")

    game = game_dir(args.game_dir)
    ok = True
    if args.names:
        # The archives name their files with backslashes; slashes are easier to type in a shell
        ok = dump([name.replace("/", "\\") for name in args.names], game, args.out)
    if args.menu:
        files = ["tables\\textdb_en.def"] + [f"tables\\menu\\{name}.mnu" for name in args.menu]
        ok = dump(files, game, args.out, quiet=True) and ok
        texts = read_texts(args.out / "textdb_en.def")
        for name in args.menu:
            path = args.out / f"{name}.mnu"
            if path.exists():
                show_menu(path, texts)
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
