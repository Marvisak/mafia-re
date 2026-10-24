"""Which objects and functions an edit changes: every source of a target compiled with and without it.

A header edit made for one function (an inline helper's spelling, an assert, a new accessor) can move others: their
inline budgets, their registers, where /YX puts a file's deferred items. variants.py scores one function; this tool
compiles every source file of the target twice, from the same kind of temporary copy, once as it is and once with the
edits, and compares the two objects section by section (contents and relocations; the debug sections left out, as
.debug$S holds the temporary paths). It prints each object whose sections differ, with the COMDAT symbol of each
differing section, so the functions the edit touches are named; a different list or order of sections (a new or
dropped function, /YX moving one) is reported as such. Only the target's own objects are compiled, not the libraries
built next to it (LS3DF's base classes and import stubs).

A spec is a Python file that defines EDITS, a list of (path, old, new): each replaces the one occurrence of old in the
file at path (relative to the repository). It runs as Python with REPO defined, like a variants spec.

    .venv/Scripts/python tools/editscope.py spec.py                      # LS3DF's sources
    .venv/Scripts/python tools/editscope.py spec.py --target-dir ls3df_1_1

Sources compiled against another file's precompiled header (/Yu, LS3DF's clipper2.cpp) are compiled with a
precompiled header of their own (/Yc) both times, so the edit reaches them; their objects then differ from the
build's, but alike in both compiles. Build the vc6 preset first (for the command lines).
"""

import argparse
import re
import shutil
import struct
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import variants  # noqa: E402

REPO = variants.REPO


def target_sources(target_dir: Path) -> list[Path]:
    """The target's own sources: those whose objects are in <target>/CMakeFiles/<target>.dir (not the libraries built
    next to it, LS3DF's base classes and import stubs)."""
    ninja = (variants.BUILD / "build.ninja").read_text()
    prefix = f"{target_dir}\\CMakeFiles\\{target_dir.name}.dir\\".lower()
    return sorted({Path(m.group(2).replace("$:", ":"))
                   for m in re.finditer(r"^build (\S+\.obj): C(?:XX)?_COMPILER\S* (\S+)", ninja, re.M)
                   if m.group(1).lower().startswith(prefix)})


def sections(data: bytes):
    """(name, COMDAT symbol or None, contents, relocations) of each section of a COFF object."""
    count = struct.unpack_from("<H", data, 2)[0]
    symtab, nsyms = struct.unpack_from("<II", data, 8)
    strtab = symtab + 18 * nsyms

    def long_name(offset):
        return data[strtab + offset:data.index(b"\0", strtab + offset)].decode("latin1")

    out = []
    for i in range(count):
        header = data[20 + 40 * i:60 + 40 * i]
        name = header[:8]
        name = long_name(int(name[1:].rstrip(b"\0"))) if name[:1] == b"/" else name.rstrip(b"\0").decode("latin1")
        size, raw, relocs, _, nrelocs = struct.unpack_from("<IIIIH", header, 16)
        out.append([name, None, data[raw:raw + size] if raw else b"", data[relocs:relocs + 10 * nrelocs]])
    i = 0
    while i < nsyms:
        entry = data[symtab + 18 * i:symtab + 18 * i + 18]
        name = long_name(struct.unpack_from("<I", entry, 4)[0]) if entry[:4] == b"\0\0\0\0" else \
            entry[:8].rstrip(b"\0").decode("latin1")
        section, storage, aux = struct.unpack_from("<h", entry, 12)[0], entry[16], entry[17]
        if 0 < section <= count and out[section - 1][1] is None and \
                (storage == 2 or (storage == 3 and not name.startswith((".", "$")))):
            out[section - 1][1] = name
        i += 1 + aux
    return [tuple(s) for s in out]


def compile_twice(source: Path, target_dir: Path, texts: dict, env: dict):
    cmd = variants.compile_command(source, target_dir)
    objects = []
    for placed in ({}, texts):
        work = Path(tempfile.mkdtemp(prefix="editscope"))
        c = cmd
        m = re.search(r"/Yu(\S+)", c)
        if m:
            c = re.sub(r"/Fp\S+", lambda _: f"/Fp{work / 'own.pch'}", c.replace(m.group(0), f"/Yc{m.group(1)}"))
        obj, error = variants.compile_object(c, env, source, placed, {}, work)
        objects.append(obj.read_bytes() if obj else None)
        shutil.rmtree(work, ignore_errors=True)
        if obj is None:
            return None, error
    return objects, None


def compare(source: Path, target_dir: Path, texts: dict, env: dict) -> list[str]:
    objects, error = compile_twice(source, target_dir, texts, env)
    if objects is None:
        return ["the compile failed: " + error.strip().splitlines()[-1] if error.strip() else "the compile failed"]
    base, edited = sections(objects[0]), sections(objects[1])
    base = [s for s in base if not s[0].startswith(".debug$")]
    edited = [s for s in edited if not s[0].startswith(".debug$")]
    names_base = [(s[0], s[1]) for s in base]
    names_edited = [(s[0], s[1]) for s in edited]
    if names_base != names_edited:
        if sorted(names_base) == sorted(names_edited):
            out = ["  the same sections in another order"]
        else:
            out = ["  other sections"]
        gone = [n for n in names_base if n not in names_edited]
        new = [n for n in names_edited if n not in names_base]
        out += [f"    only before: {variants.short_name(s or '')} ({n})" for n, s in gone]
        out += [f"    only after:  {variants.short_name(s or '')} ({n})" for n, s in new]
        if gone or new:
            return out
        return out + [f"    first moved: {names_base[k]} -> {names_edited[k]}"
                      for k in range(len(names_base)) if names_base[k] != names_edited[k]][:1]
    return [f"  {name:10s} {variants.short_name(symbol) if symbol else ''}"
            for (name, symbol, data, relocs), (_, _, data2, relocs2) in zip(base, edited)
            if data != data2 or relocs != relocs2]


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("spec", help="a file that defines EDITS = [(path, old, new), ...]")
    parser.add_argument("--target-dir", default="ls3df", help="the target's folder in the build tree (default ls3df)")
    parser.add_argument("--jobs", type=int, default=8, help="parallel compiles (default 8)")
    args = parser.parse_args()

    spec = {"REPO": REPO}
    exec(Path(args.spec).read_text(), spec)
    texts = {}
    for path, old, new in spec["EDITS"]:
        real = (REPO / path).resolve()
        text = texts.get(real) or real.read_text()
        if text.count(old) != 1:
            sys.exit(f"{path}: the text to replace occurs {text.count(old)} times: {old[:70]!r}")
        texts[real] = text.replace(old, new)

    target_dir = Path(args.target_dir)
    env = variants.preset_env("vc6")
    sources = target_sources(target_dir)
    changed = 0
    with ThreadPoolExecutor(args.jobs) as pool:
        for source, out in zip(sources, pool.map(lambda s: compare(s, target_dir, texts, env), sources)):
            if out:
                changed += 1
                print(source.relative_to(REPO).as_posix())
                print("\n".join(out))
    print(f"{changed} of {len(sources)} objects change")


if __name__ == "__main__":
    main()
