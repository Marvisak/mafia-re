"""Re-point by-name annotations of folded functions at the names our PDB keeps now.

The linker folds identical functions (empty ones, return-0s, template copies that don't depend on their type) into
one, and our PDB keeps one public name for the copy. Which one depends on everything linked, so adding functions
anywhere can change it, and a by-name annotation (LIBRARY, TEMPLATE, SYNTHETIC or FUNCTION followed by a name) that
named the old one then matches nothing, silently. This finds those annotations and writes the name reccmp has for our
copy instead:

- for each annotated function of the original that reccmp left unmatched,
- the address our map lists its name at: the map lists every folded name of a copy; a decorated name VC6 cut to 255
  characters with a hash is looked up by its first 247, a plain one (I3D_part_base::`scalar deleting destructor')
  by the qualified name of each decorated one,
- the symbol reccmp has for our function there, if that function is unmatched too.

Several targets can share one annotation (their markers stacked over one name line, as LS3DF's and LS3DF11's), and
their builds can keep different names for a group. Given the targets together, it rewrites the name line where every
target of the block wants the same name, and splits the block into one per name where they don't. A target not given
keeps the name it has.

It prints each change; with --write it rewrites the target's source files. Build the PDB preset first, and rebuild
after writing (a split adds lines, which moves the line numbers of the functions after it). Names that don't fold (an
annotation of a function that's gone, a template not instantiated) are listed as not found, for a look by hand.

    .venv/Scripts/python tools/foldfix.py LS3DF LS3DF11            # what would change
    .venv/Scripts/python tools/foldfix.py LS3DF LS3DF11 --write
"""

import argparse
import logging
import re
from pathlib import Path

import pydemumble
from reccmp.compare import Compare
from reccmp.project.detect import RecCmpProject

REPO = Path(__file__).resolve().parent.parent
MAP_LINE = re.compile(r"\s*([0-9a-f]{4}):([0-9a-f]{8})\s+(\S+)\s+([0-9a-f]{8})")
MARKER = re.compile(r"^\s*//\s*(LIBRARY|TEMPLATE|SYNTHETIC|FUNCTION|STUB):\s*(\w+)\s+(0x[0-9a-fA-F]+)")
ANY_MARKER = re.compile(r"^\s*//\s*[A-Z]+:\s*\w+\s+0x[0-9a-fA-F]+")
CONVENTIONS = re.compile(r"\b__(?:thiscall|stdcall|cdecl|fastcall) ")


def map_addresses(path: Path) -> dict[str, int]:
    """Every public symbol of a map file and its address."""
    out = {}
    for line in path.read_text(errors="replace").splitlines():
        m = MAP_LINE.match(line)
        if m and m.group(1) != "0000":
            out.setdefault(m.group(3), int(m.group(4), 16))
    return out


def qualified_name(symbol: str) -> str | None:
    """The name a decorated symbol has in the PDB, without its type: I3D_part_base::`scalar deleting destructor'."""
    text = pydemumble.demangle(symbol)
    if not text or text == symbol or "(" not in text:
        return None
    text = text[:text.rindex("(")]
    found = list(CONVENTIONS.finditer(text))
    if found:
        text = text[found[-1].end():]
    return text.replace(" dtor'", " destructor'").strip()


def find_changes(project: RecCmpProject, target_id: str) -> tuple[dict[int, tuple[str, str]], list]:
    """{original address: (annotated name, the name our copy has)} for one target, and the names not found."""
    target = project.get(target_id)
    compare = Compare.from_target(target)
    addresses = map_addresses(Path(target.recompiled_path).with_suffix(".map"))
    by_plain = {}
    for symbol, address in addresses.items():
        if symbol.startswith("?"):
            plain = qualified_name(symbol)
            if plain:
                by_plain.setdefault(plain, set()).add(address)

    recomp = {}
    for ent in compare._db.get_all():
        if ent.recomp_addr is not None:
            recomp[ent.recomp_addr] = ent
    changes = {}
    missing = []
    for ent in compare._db.get_all():
        if ent.orig_addr is None or ent.recomp_addr is not None:
            continue
        symbol = ent.get("symbol")
        name = symbol if symbol and symbol.startswith("?") else ent.name
        if not name:
            continue
        if name.startswith("?"):
            address = addresses.get(name)
            if address is None and len(name) == 255:
                # VC6 cuts longer names to 247 characters and a hash of the rest (C4786); the map has them whole
                found = {a for s, a in addresses.items() if s.startswith(name[:247])}
                address = found.pop() if len(found) == 1 else None
        else:
            found = by_plain.get(name, set())
            address = next(iter(found)) if len(found) == 1 else None
            if address is None:
                continue  # not a by-name annotation of a folded function
        ours = recomp.get(address)
        if ours is None or ours.orig_addr is not None or not ours.get("symbol") or ours.get("symbol") == name:
            missing.append((ent.orig_addr, name))
            continue
        changes[ent.orig_addr] = (name, ours.get("symbol"))
    return changes, missing


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("targets", nargs="+", help="reccmp target ids, e.g. LS3DF LS3DF11")
    parser.add_argument("--pdb-build", default=str(REPO / "build" / "vc6-pdb"))
    parser.add_argument("--write", action="store_true", help="rewrite the annotations")
    args = parser.parse_args()

    logging.basicConfig(level=logging.CRITICAL)
    project = RecCmpProject.from_directory(Path(args.pdb_build))
    changes, missing, roots = {}, [], set()
    for target_id in args.targets:
        changes[target_id], target_missing = find_changes(project, target_id)
        missing += [(target_id, addr, name) for addr, name in target_missing]
        roots.update(project.get(target_id).source_paths)

    paths = sorted({p for root in roots for p in Path(root).rglob("*") if p.suffix in (".h", ".cpp")})
    for path in paths:
        # newline="" keeps the file's line ends: "\r" stays on each line, and the lines a split adds get it too
        with open(path, encoding="utf-8", newline="") as f:
            lines = f.read().split("\n")
        out = []
        changed = False
        i = 0
        while i < len(lines):
            # A block: function markers stacked over one name line
            j = i
            while j < len(lines) and MARKER.match(lines[j]):
                j += 1
            if j == i or j >= len(lines) or ANY_MARKER.match(lines[j]) or not lines[j].strip().startswith("//"):
                out.append(lines[i])
                i += 1
                continue
            markers, name_line = lines[i:j], lines[j]
            current = name_line.strip()[2:].strip()
            wanted = []
            for line in markers:
                m = MARKER.match(line)
                old_new = changes.get(m.group(2), {}).get(int(m.group(3), 16))
                wanted.append(old_new[1] if old_new and old_new[0] == current else current)
            names = list(dict.fromkeys(wanted))
            if names == [current]:
                out += markers + [name_line]
            else:
                eol = "\r" if name_line.endswith("\r") else ""
                for k, name in enumerate(names):
                    group = [line for line, w in zip(markers, wanted) if w == name]
                    if k:
                        out.append(eol)
                    out += group + [name_line.replace(current, name)]
                    if name != current:
                        targets = ", ".join(MARKER.match(line).group(2) for line in group)
                        print(f"{path.relative_to(REPO)}:{j + 1}  {MARKER.match(group[0]).group(3)} ({targets})"
                              f"\n    {current}\n -> {name}{'  (split)' if len(names) > 1 else ''}")
                changed = True
            i = j + 1
        if changed and args.write:
            with open(path, "w", encoding="utf-8", newline="") as f:
                f.write("\n".join(out))
    for target_id, addr, name in sorted(missing):
        print(f"not found: {target_id} 0x{addr:x} {name}")


if __name__ == "__main__":
    main()
