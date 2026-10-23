"""Re-point by-name annotations of numbered compiler names ($E12, the thunks and atexit helpers of a file's dynamic
initializers) at the names our build gives them now.

VC6 numbers a file's `$E`s (and its static guards' `$S`s) from one counter, which every included header that has
something to number moves on (`<stdexcept>`, through `<vector>` or `<list>`, takes one), so a header change can rename a
file's thunks without changing any code. reccmp pairs the n-th annotation of a name (by original address) with our
n-th function of that name (by address), so the annotations then pair with the wrong functions, silently.

This tool finds each annotated function of the original through the initializer table instead: our n-th `.CRT$XCU`
entry is the original's n-th (the same objects in the same order), and an atexit helper is the n-th address its
thunk pushes. A function-local static's helper, which the function registers on its first call, is the n-th address
pushed by the original function reccmp pairs with ours. For every annotation of a numbered name it looks up our function, regroups the annotations by the names
our functions have now, and fills in the earlier functions of a name that the pairing needs (from the table too).
Annotations it can't place are left as they are and listed.

The original's table bounds come from the target's `___xc_a`/`___xc_z` annotations, ours from the map. `$S` guards are
data, without a table: check those by hand (DUMPBIN /SYMBOLS on the objects gives their names).

    .venv/Scripts/python tools/enames.py LS3DF11                 # what would change
    .venv/Scripts/python tools/enames.py LS3DF11 --write
    .venv/Scripts/python tools/enames.py LS3DF --pdb-build build/agent1
"""

import argparse
import collections
import logging
import re
import struct
from pathlib import Path

import capstone
from reccmp.compare import Compare
from reccmp.project.detect import RecCmpProject

REPO = Path(__file__).resolve().parent.parent
NUMBERED = re.compile(r"^\$E\d+$")
MARK = re.compile(r"^// (SYNTHETIC|FUNCTION|STUB|TEMPLATE|LIBRARY): (\w+) (0x[0-9a-fA-F]+)\s*$")


def image(path: Path):
    """read(address, size) over a PE file's sections"""
    data = path.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    count = struct.unpack_from("<H", data, pe + 6)[0]
    optional = struct.unpack_from("<H", data, pe + 20)[0]
    base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
    sections = []
    for i in range(count):
        _, _, va, size, raw = struct.unpack_from("<8sIIII", data, pe + 24 + optional + 40 * i)
        sections.append((base + va, size, raw))

    def read(address: int, size: int) -> bytes:
        for va, length, raw in sections:
            if va <= address < va + length:
                return data[raw + address - va:raw + address - va + size]
        return b""
    return read


def pushes(read, address: int) -> list[int]:
    """the push imm32 operands of a thunk (a short function: the constructor's arguments, the atexit helper)"""
    code = read(address, 0x40)
    return [struct.unpack_from("<I", code, i + 1)[0] for i in range(len(code) - 4) if code[i] == 0x68]


def pushed(read, start: int, size: int) -> list[int]:
    """the push imm32 operands of a function, in order"""
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    return [int(i.op_str, 16) for i in md.disasm(read(start, size), start)
            if i.mnemonic == "push" and i.op_str.startswith("0x")]


def read_lines(path: Path) -> tuple[list[str], str]:
    """a source file's lines and its line end"""
    text = path.read_bytes().decode("utf-8")
    end = "\r\n" if "\r\n" in text else "\n"
    return text.replace("\r\n", "\n").split("\n"), end


def table(read, start: int, end: int) -> list[int]:
    return list(struct.unpack(f"<{(end - start) // 4}I", read(start, end - start)))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("target", help="reccmp target id, e.g. LS3DF11")
    parser.add_argument("--pdb-build", default=str(REPO / "build" / "vc6-pdb"))
    parser.add_argument("--write", action="store_true", help="rewrite the annotations")
    args = parser.parse_args()
    logging.basicConfig(level=logging.CRITICAL)
    target = RecCmpProject.from_directory(Path(args.pdb_build)).get(args.target)

    # our numbered functions, from the map
    ours = {}
    symbols = {}
    for line in Path(target.recompiled_path).with_suffix(".map").read_text().splitlines():
        p = line.split()
        if len(p) >= 4 and re.match(r"000\d:", p[0]) and re.match(r"^[0-9a-f]{8}$", p[2]):
            symbols[p[1]] = int(p[2], 16)
            name = p[1][1:] if p[1].startswith("_$") else p[1]
            if NUMBERED.match(name) and "f" in p[3:-1]:
                ours[int(p[2], 16)] = name
    by_name = collections.defaultdict(list)
    for address, name in sorted(ours.items()):
        by_name[name].append(address)

    # the annotation blocks: markers stacked over a numbered name line
    paths = sorted({p for root in target.source_paths for p in Path(root).rglob("*") if p.suffix in (".h", ".cpp")})
    texts = {}
    blocks = []
    bounds = {}
    for path in paths:
        lines, line_end = read_lines(path)
        texts[path] = lines, line_end
        for i, line in enumerate(lines):
            m = re.match(rf"^// GLOBAL: {args.target} (0x[0-9a-fA-F]+)$", lines[i - 1]) if i else None
            if m and line in ("// ___xc_a", "// ___xc_z"):
                bounds[line[3:]] = int(m.group(1), 16)
            if not (line.startswith("// ") and NUMBERED.match(line[3:].strip())):
                continue
            j = i
            marks = []
            while j > 0 and MARK.match(lines[j - 1]):
                j -= 1
                m = MARK.match(lines[j])
                marks.insert(0, (m.group(1), m.group(2), int(m.group(3), 16)))
            if marks:
                blocks.append({"path": path, "start": j, "end": i, "marks": marks, "name": line[3:].strip()})

    # original address <-> ours, through the initializer tables
    our_read = image(Path(target.recompiled_path))
    orig_read = image(Path(target.original_path))
    our_table = table(our_read, symbols["___xc_a"], symbols["___xc_z"])
    orig_table = table(orig_read, bounds["___xc_a"], bounds["___xc_z"])
    if len(our_table) != len(orig_table):
        raise SystemExit(f"initializer tables differ in length: ours {len(our_table)}, the original's {len(orig_table)}")
    to_ours, to_orig = {}, {}
    for o, g in zip(our_table, orig_table):
        if o and g:
            to_ours[g], to_orig[o] = o, g
            for po, pg in zip(pushes(our_read, o), pushes(orig_read, g)):
                if po in ours:
                    to_ours[pg], to_orig[po] = po, pg

    # the helpers of function-local statics, through the function that registers them
    rest = [a for a in ours if a not in to_orig]
    if rest:
        compare = Compare.from_target(target)
        matched = sorted((e.recomp_addr, e.orig_addr) for e in compare._db.get_all()
                         if e.recomp_addr is not None and e.orig_addr is not None)
        for helper in rest:
            for k, (ra, oa) in enumerate(matched):
                end = matched[k + 1][0] if k + 1 < len(matched) else ra + 0x4000
                if end - ra > 0x8000:
                    continue
                ops = pushed(our_read, ra, end - ra)
                if helper in ops:
                    j = ops.index(helper)
                    orig_ops = pushed(orig_read, oa, end - ra + 0x400)
                    if j < len(orig_ops):
                        to_ours[orig_ops[j]], to_orig[helper] = helper, orig_ops[j]
                    break

    # where each annotation points now
    placed = collections.defaultdict(dict)       # our name: {rank: (original address, kind)}
    moves = []
    unplaced = []
    for block in blocks:
        for kind, target_id, address in block["marks"]:
            if target_id != args.target:
                continue
            our = to_ours.get(address)
            if our is None:
                unplaced.append((block, address))
                continue
            name = ours[our]
            rank = by_name[name].index(our)
            placed[name][rank] = (address, kind)
            moves.append((block, kind, address, name, rank))
    fills = []
    for name, ranks in placed.items():
        for rank in range(max(ranks)):
            if rank not in ranks:
                address = to_orig.get(by_name[name][rank])
                if address is None:
                    raise SystemExit(f"{name} #{rank} (ours {by_name[name][rank]:#x}) has no original address")
                kind = next(iter(ranks.values()))[1]
                ranks[rank] = (address, kind)
                fills.append((name, kind, address, rank))

    changes = [(b, kind, a, new, rank) for b, kind, a, new, rank in moves if new != b["name"]]
    for b, kind, a, new, rank in changes:
        print(f"{b['path'].relative_to(REPO)}:{b['end'] + 1}  {a:#x}  {b['name']} -> {new} (#{rank})")
    for name, kind, a, rank in fills:
        print(f"  needs {name} #{rank}: {a:#x}")
    for b, a in unplaced:
        print(f"  not an initializer: {b['path'].relative_to(REPO)}:{b['end'] + 1}  {a:#x}  {b['name']}")
    if not args.write:
        return

    edits = collections.defaultdict(list)
    for b, kind, a, new, rank in changes:
        edits[b["path"]].append((b, kind, a, new))
    for path, entries in edits.items():
        lines, line_end = texts[path]
        for b, kind, a, new in sorted(entries, key=lambda e: -e[0]["end"]):
            mine = [x for x in lines[b["start"]:b["end"]] if MARK.match(x).group(2) == args.target
                    and int(MARK.match(x).group(3), 16) == a]
            others = [x for x in lines[b["start"]:b["end"]] if x not in mine]
            if not others:
                lines[b["end"]] = "// " + new
            else:
                lines[b["start"]:b["end"]] = others
                end = b["start"] + len(others)
                # a block of its own after the old one, in a run of blocks without blank lines between them too
                after = lines[end + 1] if end + 1 < len(lines) else ""
                lines[end + 1:end + 1] = ([mine[0], "// " + new] if MARK.match(after) else ["", mine[0], "// " + new])
        path.write_bytes(line_end.join(lines).encode("utf-8"))
    for name, kind, a, rank in sorted(fills, key=lambda f: (f[0], -f[3])):
        for path in paths:
            lines, line_end = read_lines(path)
            at = [i for i in range(1, len(lines)) if lines[i] == "// " + name and MARK.match(lines[i - 1])
                  and MARK.match(lines[i - 1]).group(2) == args.target]
            if at:
                i = at[0] - 1
                while i > 0 and MARK.match(lines[i - 1]):
                    i -= 1
                run = i > 0 and NUMBERED.match(lines[i - 1][3:].strip())      # in a run without blank lines
                lines[i:i] = [f"// {kind}: {args.target} {a:#010x}", "// " + name] + ([] if run else [""])
                path.write_bytes(line_end.join(lines).encode("utf-8"))
                break
    print("written")


if __name__ == "__main__":
    main()
