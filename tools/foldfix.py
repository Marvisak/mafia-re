"""Re-point by-name annotations of folded functions at the names our PDB keeps now.

The linker folds identical functions (empty ones, return-0s, template copies that don't depend on their type) into
one, and our PDB keeps one public name for the copy. Which one depends on everything linked, so adding functions
anywhere can change it, and a by-name annotation (LIBRARY, TEMPLATE, SYNTHETIC or FUNCTION with a decorated name)
that named the old one then matches nothing, silently. This finds those annotations and writes the name reccmp has
for our copy instead:

- for each annotated function of the original that reccmp left unmatched and that names a decorated symbol,
- the address our map lists that symbol at (the map lists every folded name of a copy; a name VC6 cut to 255
  characters with a hash is looked up by its first 247),
- the symbol reccmp has for our function there, if that function is unmatched too.

It prints each change; with --write it rewrites the name line under the annotation's marker in the target's source
files. Build the PDB preset first, and rebuild after writing (the annotations' line numbers don't change, only the
names). Names that don't fold (an annotation of a function that's gone, a template not instantiated) are listed as
not found, for a look by hand.

    .venv/Scripts/python tools/foldfix.py LS3DF            # what would change
    .venv/Scripts/python tools/foldfix.py LS3DF --write
"""

import argparse
import logging
import re
from pathlib import Path

from reccmp.compare import Compare
from reccmp.project.detect import RecCmpProject

REPO = Path(__file__).resolve().parent.parent
MAP_LINE = re.compile(r"\s*([0-9a-f]{4}):([0-9a-f]{8})\s+(\S+)\s+([0-9a-f]{8})")
MARKER = re.compile(r"//\s*(LIBRARY|TEMPLATE|SYNTHETIC|FUNCTION|STUB):\s*(\w+)\s+(0x[0-9a-fA-F]+)")


def map_addresses(path: Path) -> dict[str, int]:
    """Every public symbol of a map file and its address."""
    out = {}
    for line in path.read_text(errors="replace").splitlines():
        m = MAP_LINE.match(line)
        if m and m.group(1) != "0000":
            out.setdefault(m.group(3), int(m.group(4), 16))
    return out


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("target", help="reccmp target id, e.g. LS3DF")
    parser.add_argument("--pdb-build", default=str(REPO / "build" / "vc6-pdb"))
    parser.add_argument("--write", action="store_true", help="rewrite the annotations")
    args = parser.parse_args()

    logging.basicConfig(level=logging.CRITICAL)
    project = RecCmpProject.from_directory(Path(args.pdb_build))
    target = project.get(args.target)
    compare = Compare.from_target(target)
    addresses = map_addresses(Path(target.recompiled_path).with_suffix(".map"))

    recomp = {}
    for ent in compare._db.get_all():
        if ent.recomp_addr is not None:
            recomp[ent.recomp_addr] = ent
    changes = {}
    missing = []
    for ent in compare._db.get_all():
        symbol = ent.get("symbol")
        if ent.orig_addr is None or ent.recomp_addr is not None or not symbol or not symbol.startswith("?"):
            continue
        address = addresses.get(symbol)
        if address is None and len(symbol) == 255:
            # VC6 cuts longer names to 247 characters and a hash of the rest (C4786); the map has them whole
            found = {a for s, a in addresses.items() if s.startswith(symbol[:247])}
            address = found.pop() if len(found) == 1 else None
        ours = recomp.get(address)
        if ours is None or ours.orig_addr is not None or not ours.get("symbol") or ours.get("symbol") == symbol:
            missing.append((ent.orig_addr, symbol))
            continue
        changes[ent.orig_addr] = (symbol, ours.get("symbol"))

    for path in sorted(p for root in target.source_paths for p in Path(root).rglob("*") if p.suffix in (".h", ".cpp")):
        lines = path.read_text(encoding="utf-8").split("\n")
        changed = False
        for i, line in enumerate(lines[:-1]):
            m = MARKER.search(line)
            if not m or m.group(2) != args.target or int(m.group(3), 16) not in changes:
                continue
            old, new = changes[int(m.group(3), 16)]
            if lines[i + 1].strip() == "// " + old:
                print(f"{path.relative_to(REPO)}:{i + 2}  {m.group(3)}\n    {old}\n -> {new}")
                lines[i + 1] = lines[i + 1].replace(old, new)
                changed = True
        if changed and args.write:
            path.write_text("\n".join(lines), encoding="utf-8")
    for addr, symbol in sorted(missing):
        print(f"not found: 0x{addr:x} {symbol}")


if __name__ == "__main__":
    main()
