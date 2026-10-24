"""Set up a folder to run the rebuilt Game.exe in, and run it there.

Game.exe works from its own folder, which has to hold the game's files, and the unpatched 1.2 executable needs more
than Steam's install has (CONTRIBUTING.md, "Running the rebuilt Game.exe"). This tool builds such a folder in
build/run without changing the game's install:

- the game's files: hard links to the big ones (the archives, the videos, the sounds folder), copies of the rest, so
  what the game writes (log.txt, the saved games) stays in build/run. Nothing is ever written through a link: a file
  the tool replaces is deleted first.
- the retail rw_data.dll (target/rw_data.dll), since Steam's prefers loose files over the archives. With
  --rebuilt-dlls the vc6 build's LS3DF.dll, IJoy.dll and rw_data.dll instead of the originals.
- the two archives Steam lacks, a0.dta and ab.dta, extracted from the third CD's image (--cd3, needed once). They have
  to be in the folder: InitSystem opens every archive there and looks on the drives only for a1.dta.
- the CD the CD check looks for, \\MafiaGame\\a1.dta at the root of some drive: the first CD's image mounted (--cd1;
  Windows mounts it without elevation, until the next restart), whatever drive already has the file, or else a subst
  of build/run/cd (until the next logon). --eject dismounts the image and removes the subst.
- Game.exe: the vc6 build's, built first, or with --orig the original (target/Game.orig.exe), to compare the two.

Then it starts Game.exe there, waits for it to end and prints log.txt, where the engine's dbgPrintf writes: the stubs
of what isn't written yet log their names there. It also reports whether the settings the game reads are in the
registry. Without them the game starts setup.exe (the 1.2 setup), which saves them; --setup runs it directly.

    .venv/Scripts/python tools/rungame.py --cd3 MAFIA_CD_3.iso --cd1 MAFIA_CD_1.iso   # the first time
    .venv/Scripts/python tools/rungame.py                     # build the game and run it
    .venv/Scripts/python tools/rungame.py --orig              # run the original in the same folder
    .venv/Scripts/python tools/rungame.py --setup             # run the 1.2 setup there
    .venv/Scripts/python tools/rungame.py --eject             # dismount the CD, remove the subst
"""

import argparse
import ctypes
import os
import shutil
import subprocess
import sys
import winreg
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
STEAM_GAME = Path(r"C:\Program Files (x86)\Steam\steamapps\common\Mafia\Mafia")
RUN = REPO / "build" / "run"
# The archives the original checks for (the table at 0x637890) but b8.dta, which it doesn't require
ARCHIVES = ["a0.dta", "a1.dta", "a2.dta", "a3.dta", "a4.dta", "a5.dta", "a6.dta", "a7.dta", "a8.dta", "a9.dta",
            "aa.dta", "ab.dta", "ac.dta"]
# Steam's install has the others; the third CD has these in MafiaGame\
CD3_ARCHIVES = ["a0.dta", "ab.dta"]
# Placed by the tool, not taken from the install (an IDA database of someone's isn't the game's either)
NOT_FROM_INSTALL = {"game.exe", "log.txt", "rw_data.dll", "ls3df.dll", "ijoy.dll"}
REBUILT_DLLS = {"LS3DF.dll": "ls3df", "IJoy.dll": "ijoy", "rw_data.dll": "rw_data"}
# Where a 32-bit program without a manifest reads HKLM\Software\Illusion Softworks\Mafia: the 32-bit view, or UAC's
# virtual store when the setup that wrote it wasn't elevated
SETTINGS_KEYS = [
    (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\Illusion Softworks\Mafia", winreg.KEY_WOW64_32KEY),
    (winreg.HKEY_CURRENT_USER, r"Software\Classes\VirtualStore\MACHINE\SOFTWARE\WOW6432Node\Illusion Softworks\Mafia", 0),
]


def place(src: Path, dst: Path, link: bool):
    """Puts src at dst, a hard link or a copy. An existing dst is deleted first, which only removes a link."""
    if dst.exists():
        dst.unlink()
    dst.parent.mkdir(parents=True, exist_ok=True)
    if link:
        try:
            os.link(src, dst)
            return
        except OSError:
            pass
    shutil.copy2(src, dst)


def populate(game: Path, run: Path):
    """The install's files, linked or copied. A link is made again when the install's file has another size (an
    update replaced it); a copy is made only once, as the game may have changed it since (the saved games)."""
    for src in sorted(game.rglob("*")):
        rel = src.relative_to(game)
        if src.is_dir():
            continue
        if len(rel.parts) == 1 and (rel.name.lower() in NOT_FROM_INSTALL or rel.suffix.lower() == ".i64"):
            continue
        dst = run / rel
        size = src.stat().st_size
        # The saved games are copied whatever their size: the game writes them
        link = rel.parts[0].lower() == "sounds" or (len(rel.parts) == 1 and size >= 1 << 20)
        if dst.exists() and (not link or dst.stat().st_size == size):
            continue
        place(src, dst, link)


def find(folder: Path, name: str) -> Path | None:
    """A file of the folder by its name, in any case (the install has A1.dta and a2.dta)."""
    for p in folder.iterdir():
        if p.name.lower() == name.lower():
            return p
    return None


def seven_zip() -> str:
    return shutil.which("7z") or r"C:\Program Files\7-Zip\7z.exe"


def subst_drives() -> dict[str, Path]:
    out = subprocess.run(["subst"], capture_output=True, text=True, errors="replace").stdout
    drives = {}
    for line in out.splitlines():
        if "=>" in line:
            drive, path = line.split("=>", 1)
            drives[drive.strip()[0].upper()] = Path(path.strip())
    return drives


def powershell(command: str) -> str:
    return subprocess.run(["powershell", "-NoProfile", "-Command", command], capture_output=True, text=True,
                          errors="replace", check=True).stdout.strip()


def drive_with_cd() -> str | None:
    """A drive that has \\MafiaGame\\a1.dta at its root, where the CD check finds it."""
    # No "no disk" message boxes for empty drives
    ctypes.windll.kernel32.SetErrorMode(1)
    used = ctypes.windll.kernel32.GetLogicalDrives()
    for letter in "DEFGHIJKLMNOPQRSTUVWXYZ":
        # Not network drives (DRIVE_REMOTE), which can take long to answer
        if used & (1 << (ord(letter) - ord("A"))) and ctypes.windll.kernel32.GetDriveTypeW(f"{letter}:\\") != 4:
            if os.path.exists(f"{letter}:\\MafiaGame\\a1.dta"):
                return letter
    return None


def mount(image: Path) -> str:
    """Mounts a CD image (unless it is already) and returns its drive letter."""
    path = str(image.resolve()).replace("'", "''")
    return powershell(f"$i = Get-DiskImage -ImagePath '{path}'; if (-not $i.Attached) {{ $i = Mount-DiskImage "
                      f"-ImagePath '{path}' -PassThru }}; ($i | Get-Volume).DriveLetter")


def eject(run: Path):
    for letter, path in subst_drives().items():
        if path.resolve() == (run / "cd").resolve():
            subprocess.run(["subst", f"{letter}:", "/d"], check=True)
            print(f"removed the subst {letter}:")
    # The mounted images with the CD's file
    out = powershell("Get-Volume | Where-Object { $_.DriveLetter -and (Test-Path \"$($_.DriveLetter):\\MafiaGame\\a1.dta\") }"
                     " | ForEach-Object { $i = Get-DiskImage -Volume $_ -ErrorAction SilentlyContinue; if ($i) {"
                     " Dismount-DiskImage -ImagePath $i.ImagePath | Out-Null; $i.ImagePath } }")
    for line in out.splitlines():
        print(f"dismounted {line}")


def cd_drive(run: Path, image: Path | None) -> str:
    """The drive the CD check will find: the mounted image, one that has the file already, or a subst of build/run/cd."""
    if image:
        return mount(image)
    letter = drive_with_cd()
    if letter:
        return letter
    cd = run / "cd"
    a1 = cd / "MafiaGame" / "a1.dta"
    if not a1.exists():
        # The CD check only looks for the file (FindFirstFile), but a link costs nothing
        src = find(run, "a1.dta")
        if src:
            place(src, a1, link=True)
        else:
            a1.parent.mkdir(parents=True, exist_ok=True)
            a1.touch()
    for letter, path in subst_drives().items():
        if path.resolve() == cd.resolve():
            return letter
    used = ctypes.windll.kernel32.GetLogicalDrives()
    for letter in "ZYXWVUTSRQPONMLKJIHGFED":
        if not used & (1 << (ord(letter) - ord("A"))):
            subprocess.run(["subst", f"{letter}:", str(cd)], check=True)
            return letter
    sys.exit("no free drive letter for the CD")


def settings_found() -> list[str]:
    found = []
    for root, path, view in SETTINGS_KEYS:
        try:
            with winreg.OpenKey(root, path, 0, winreg.KEY_READ | view) as key:
                value, _ = winreg.QueryValueEx(key, "LS3D_setup")
        except OSError:
            continue
        # LoadSettings wants the whole 0x3f bytes and the magic 0x14
        if len(value) == 0x3F and value[0] == 0x14:
            found.append(("HKLM\\" if root == winreg.HKEY_LOCAL_MACHINE else "HKCU\\") + path)
    return found


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--game-dir", type=Path, default=STEAM_GAME, help="the game's install (default: Steam's)")
    ap.add_argument("--cd3", type=Path, help="the third CD's image, for a0.dta and ab.dta")
    ap.add_argument("--cd1", type=Path, help="the first CD's image, mounted for the CD check")
    ap.add_argument("--orig", action="store_true", help="run the original (target/Game.orig.exe)")
    ap.add_argument("--rebuilt-dlls", action="store_true", help="the vc6 build's LS3DF, IJoy and rw_data")
    ap.add_argument("--no-build", action="store_true", help="run the vc6 build's Game.exe as it is")
    ap.add_argument("--no-run", action="store_true", help="only set the folder up")
    ap.add_argument("--setup", action="store_true", help="run the 1.2 setup in the folder")
    ap.add_argument("--eject", action="store_true", help="dismount the CD's image, remove the subst")
    args = ap.parse_args()

    if args.eject:
        eject(RUN)
        return

    game = args.game_dir
    if not (game / "LS3DF.dll").exists():
        sys.exit(f"{game} isn't the game's folder (no LS3DF.dll); pass --game-dir")
    RUN.mkdir(parents=True, exist_ok=True)
    populate(game, RUN)

    if args.cd3:
        missing = [n for n in CD3_ARCHIVES if not find(RUN, n)]
        if missing:
            print(f"extracting {', '.join(missing)} from {args.cd3} ...")
            subprocess.run([seven_zip(), "e", "-y", f"-o{RUN}", str(args.cd3)] + [f"MafiaGame\\{n}" for n in missing],
                           check=True, stdout=subprocess.DEVNULL)

    # The DLLs, every time, so the choice of --rebuilt-dlls doesn't stick
    for name, folder in REBUILT_DLLS.items():
        if args.rebuilt_dlls:
            src = REPO / "build" / "vc6" / folder / name
        elif name == "rw_data.dll":
            src = REPO / "target" / name
            if not src.exists():
                # The vc6 build's is the retail one byte for byte
                src = REPO / "build" / "vc6" / folder / name
        else:
            src = find(game, name)
        if not src or not src.exists():
            sys.exit(f"no {name} to put into {RUN}")
        place(src, RUN / name, link=False)

    missing = [n for n in ARCHIVES if not find(RUN, n)]
    letter = cd_drive(RUN, args.cd1)
    settings = settings_found()
    print(f"folder:   {RUN}")
    print(f"CD:       {letter}:\\MafiaGame\\a1.dta")
    print(f"archives: {'all there' if not missing else 'missing ' + ', '.join(missing) + ' (--cd3)'}")
    print(f"settings: {', '.join(settings) if settings else 'none, the game starts setup.exe'}")

    if args.setup:
        subprocess.run([str(find(RUN, "setup.exe"))], cwd=RUN)
        return

    if args.orig:
        exe = REPO / "target" / "Game.orig.exe"
        if missing:
            print("warning: the original stops with error #3075 without every archive")
    else:
        if not args.no_build:
            build = subprocess.run(["cmake", "--build", "--preset", "vc6", "--target", "game"], cwd=REPO,
                                   capture_output=True, text=True, errors="replace")
            if build.returncode:
                sys.exit(build.stdout + build.stderr)
        exe = REPO / "build" / "vc6" / "game" / "Game.exe"
    place(exe, RUN / "Game.exe", link=False)
    if args.no_run:
        return

    log = RUN / "log.txt"
    if log.exists():
        log.unlink()
    print(f"running {exe.relative_to(REPO)} ...")
    code = subprocess.run([str(RUN / "Game.exe")], cwd=RUN).returncode
    print(f"exit code {code}")
    if log.exists():
        print("log.txt:")
        print(log.read_text(errors="replace").rstrip())
    else:
        print("no log.txt: LS3DF.dll wasn't loaded, or nothing was logged")


if __name__ == "__main__":
    main()
