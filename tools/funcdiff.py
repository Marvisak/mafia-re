"""Compare a function of the original binary with the rebuild, instruction by instruction.

reccmp gives a similarity score and a unified diff. On large functions that diff loses its alignment
as soon as the code size shifts, and it can't tell register allocation or stack layout differences
from real code differences. This tool reuses reccmp's project setup, function matching, symbol names
and PDB line numbers, and then:

- aligns the two listings with jump targets, stack offsets and registers masked, so moved code and
  allocation differences don't break the alignment;
- classifies every aligned pair: identical, differs only in [esp + N] offsets, differs in registers,
  same jump but to a target that isn't the aligned one, or not aligned at all;
- maps the original's stack slots and registers to ours, from the aligned pairs;
- shows the source line of each rebuilt instruction.

Run it from the repository root after building the vc6-pdb preset (reccmp reads the PDB):

    .venv/Scripts/python tools/funcdiff.py dtaRead                # by name (substring) ...
    .venv/Scripts/python tools/funcdiff.py 0x100025c0             # ... or original address
    .venv/Scripts/python tools/funcdiff.py dtaRead --full         # whole listing, not just changes
    .venv/Scripts/python tools/funcdiff.py --all                  # one line per differing function
"""

import argparse
import difflib
import logging
import re
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from pathlib import Path

from reccmp.compare import Compare
from reccmp.compare.asm.const import JUMP_MNEMONICS
from reccmp.project.detect import RecCmpProject
from reccmp.types import ImageId

REPO = Path(__file__).resolve().parent.parent

REGS32 = ["eax", "ebx", "ecx", "edx", "esi", "edi", "ebp"]
REGS16 = ["ax", "bx", "cx", "dx", "si", "di", "bp"]
REGS8 = ["al", "ah", "bl", "bh", "cl", "ch", "dl", "dh"]
REG_RE = re.compile(r"\b(" + "|".join(REGS32 + REGS16 + REGS8) + r")\b")
# [esp + 0x10], [esp + esi + 0x64], [esp + eax*4 + 0x20]
ESP_RE = re.compile(r"(\[esp(?: \+ [a-z]{2,3}(?:\*\d)?)?) ([+-]) (0x[0-9a-f]+|\d+)\]")
HEX_DISP_RE = re.compile(r"^-?0x[0-9a-f]+$")


@dataclass
class Insn:
    addr: int | None
    text: str
    target: int | None = None  # absolute target of a jump inside the function
    norm: list[str] = field(default_factory=list)  # [exact, stack masked, stack and registers masked]


def reg_class(reg: str) -> str:
    if reg in REGS32:
        return "R"
    if reg in REGS16:
        return "R16"
    return "R8"


def normalize(insn: Insn) -> None:
    text = insn.text
    if insn.target is not None:
        text = text.split()[0] + " <jump>"
    stack = ESP_RE.sub(lambda m: f"{m.group(1)} + S]", text)
    regs = REG_RE.sub(lambda m: reg_class(m.group(1)), stack)
    insn.norm = [text, stack, regs]


def disassemble(sanitizer, data: bytes, start: int) -> list[Insn]:
    """reccmp's sanitizer gives symbolic operands and separates jump tables from code. Jumps come
    out as displacements; turn them back into absolute targets."""
    excerpt = sanitizer.parse_asm(data, start)
    insns = [Insn(addr, text) for addr, text in excerpt]
    end = start + len(data)
    addrs = [i.addr for i in insns]
    for k, insn in enumerate(insns):
        parts = insn.text.split(None, 1)
        if insn.addr is None or len(parts) != 2:
            continue
        mnemonic, op = parts
        if mnemonic in JUMP_MNEMONICS and HEX_DISP_RE.match(op):
            following = next((a for a in addrs[k + 1:] if a is not None), end)
            target = following + int(op, 16)
            if start <= target < end:
                insn.target = target
    for insn in insns:
        normalize(insn)
    return insns


@dataclass
class Result:
    name: str
    orig_addr: int
    recomp_addr: int
    orig: list[Insn]
    ours: list[Insn]
    rows: list[tuple[str, int | None, int | None]]  # (kind, orig index, ours index)
    counts: Counter

    def total(self) -> int:
        return max(len(self.orig), len(self.ours))


def compare_insns(name, orig_addr, recomp_addr, orig, ours) -> Result:
    sm = difflib.SequenceMatcher(None, [i.norm[2] for i in orig], [i.norm[2] for i in ours], autojunk=False)
    rows = []
    aligned = {}
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "equal":
            for k in range(i2 - i1):
                aligned[i1 + k] = j1 + k
                rows.append(("pair", i1 + k, j1 + k))
        else:
            rows.extend(("-", i, None) for i in range(i1, i2))
            rows.extend(("+", None, j) for j in range(j1, j2))
    orig_index = {i.addr: n for n, i in enumerate(orig) if i.addr is not None}
    ours_index = {i.addr: n for n, i in enumerate(ours) if i.addr is not None}
    counts = Counter()
    for n, (kind, i, j) in enumerate(rows):
        if kind != "pair":
            counts[kind] += 1
            continue
        a, b = orig[i], ours[j]
        if a.norm[0] == b.norm[0]:
            kind = "="
            if a.target is not None or b.target is not None:
                ti = orig_index.get(a.target)
                tj = ours_index.get(b.target)
                if ti is None or tj is None or aligned.get(ti) != tj:
                    kind = "j"
        elif a.norm[1] == b.norm[1]:
            kind = "s"
        else:
            kind = "r"
        counts[kind] += 1
        rows[n] = (kind, i, j)
    return Result(name, orig_addr, recomp_addr, orig, ours, rows, counts)


def slot_and_register_maps(res: Result):
    slots = defaultdict(Counter)
    regs = defaultdict(Counter)
    for kind, i, j in res.rows:
        if kind not in ("s", "r"):
            continue
        a, b = res.orig[i].text, res.ours[j].text
        for ma, mb in zip(ESP_RE.finditer(a), ESP_RE.finditer(b)):
            if ma.group(3) != mb.group(3):
                slots[ma.group(3)][mb.group(3)] += 1
        if kind == "r":
            ra = REG_RE.findall(ESP_RE.sub("", a))
            rb = REG_RE.findall(ESP_RE.sub("", b))
            for x, y in zip(ra, rb):
                if x != y:
                    regs[x][y] += 1
    return slots, regs


def shown(insn: Insn) -> str:
    """Jumps inside the function show their absolute target instead of reccmp's displacement."""
    if insn.target is not None:
        return f"{insn.text.split()[0]} {insn.target:08x}"
    return insn.text


def summary_line(res: Result) -> str:
    c = res.counts
    t = res.total() or 1
    same = c["="]
    return (
        f"{100 * same / t:5.1f}% identical, {100 * (same + c['s']) / t:5.1f}% with stack offsets masked, "
        f"{100 * (same + c['s'] + c['r']) / t:5.1f}% with registers masked | "
        f"stack {c['s']}, registers {c['r']}, jump targets {c['j']}, only orig {c['-']}, only ours {c['+']}"
    )


def print_result(res: Result, comparator, full: bool, context: int) -> None:
    print(f"{res.name}: orig 0x{res.orig_addr:x} ({len(res.orig)} lines), ours 0x{res.recomp_addr:x} ({len(res.ours)} lines)")
    print(summary_line(res))
    print("legend: = identical, s stack offsets differ, r registers differ, j jump goes elsewhere, -/+ only in orig/ours")
    print()

    interesting = [k != "=" for k, _, _ in res.rows]
    show = [full] * len(res.rows)
    if not full:
        for n, flag in enumerate(interesting):
            if flag:
                for m in range(max(0, n - context), min(len(res.rows), n + context + 1)):
                    show[m] = True

    # The PDB only has line numbers where a line starts; carry them forward over the following instructions
    line_of = []
    current = None
    for insn in res.ours:
        if insn.addr is not None:
            ref = comparator._source_ref_of_recomp_addr(insn.addr)  # pylint: disable=protected-access
            if ref:
                current = ref
        line_of.append(current)

    last_src = None
    skipped = False
    for n, (kind, i, j) in enumerate(res.rows):
        if not show[n]:
            skipped = True
            continue
        if skipped:
            print("   ...")
            skipped = False
            last_src = None
        a = res.orig[i] if i is not None else None
        b = res.ours[j] if j is not None else None
        src = ""
        if j is not None and line_of[j] and line_of[j] != last_src:
            src = "  ; " + line_of[j]
            last_src = line_of[j]
        oa = f"{a.addr:08x}" if a is not None and a.addr is not None else " " * 8
        ob = f"{b.addr:08x}" if b is not None and b.addr is not None else " " * 8
        if kind == "-":
            text = shown(a)
        elif kind == "+":
            text = shown(b)
        elif kind == "=" and a.target is None:
            text = a.text
        else:
            text = f"{shown(a):<44} | {shown(b)}"
        print(f" {kind} {oa} {ob}  {text}{src}")

    slots, regs = slot_and_register_maps(res)
    if slots:
        print("\nstack offsets, orig -> ours (offsets are esp-relative at each instruction, so pushes shift them):")
        for off in sorted(slots, key=lambda x: int(x, 16)):
            print(f"  {off:>6} -> " + ", ".join(f"{o} x{n}" for o, n in slots[off].most_common()))
    if regs:
        print("\nregisters, orig -> ours:")
        for r in sorted(regs):
            print(f"  {r:>4} -> " + ", ".join(f"{o} x{n}" for o, n in regs[r].most_common()))


def run_match(compare: Compare, match) -> Result:
    fc = compare.function_comparator
    orig_size = match.size(ImageId.ORIG)
    recomp_size = match.size(ImageId.RECOMP)
    if orig_size is None:
        # reccmp has no size for the original's functions and disassembles min(our size, distance to the
        # next known entity), which cuts off the end of an original that is longer than ours. Take the
        # whole distance instead; the padding after the function only adds a few unmatched lines.
        orig_max = match.max_size(ImageId.ORIG)
        orig_size = orig_max if orig_max is not None else recomp_size
    orig = disassemble(fc.orig_sanitize, compare.orig_bin.read(match.orig_addr, orig_size), match.orig_addr)
    while orig and orig[-1].text.strip() in ("nop", "int3"):  # the linker's padding up to the next function
        orig.pop()
    ours = disassemble(fc.recomp_sanitize, compare.recomp_bin.read(match.recomp_addr, recomp_size), match.recomp_addr)
    return compare_insns(match.best_name() or hex(match.orig_addr), match.orig_addr, match.recomp_addr, orig, ours)


def find_match(compare: Compare, query: str):
    functions = list(compare.get_functions())
    if re.fullmatch(r"(0x)?[0-9a-fA-F]+", query) and len(query) > 4:
        addr = int(query, 16)
        hits = [f for f in functions if f.orig_addr == addr]
    else:
        q = query.lower()
        hits = [f for f in functions if (f.best_name() or "").lower() == q]
        if not hits:
            hits = [f for f in functions if q in (f.best_name() or "").lower()]
    if len(hits) != 1:
        if not hits:
            sys.exit(f"no matched function for {query!r}")
        print(f"{query!r} matches several functions:")
        for f in hits:
            print(f"  0x{f.orig_addr:x}  {f.best_name()}")
        sys.exit(1)
    return hits[0]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("function", nargs="?", help="original address or (part of) the function name")
    parser.add_argument("--target", default="RWDATA", help="reccmp target id (default RWDATA)")
    parser.add_argument("--build", default=str(REPO / "build" / "vc6-pdb"), help="build tree with reccmp-build.yml")
    parser.add_argument("--full", action="store_true", help="print the whole listing, not just the changes")
    parser.add_argument("--context", type=int, default=3, help="lines of context around changes (default 3)")
    parser.add_argument("--all", action="store_true", help="one summary line per function that isn't identical")
    parser.add_argument("--lib", action="store_true", help="with --all: include LIBRARY (CRT) functions")
    args = parser.parse_args()
    if not args.function and not args.all:
        parser.error("give a function or --all")

    logging.basicConfig(level=logging.ERROR)
    project = RecCmpProject.from_directory(Path(args.build))
    compare = Compare.from_target(project.get(args.target))

    if args.all:
        for match in sorted(compare.get_functions(), key=lambda m: m.orig_addr):
            if match.get("library") and not args.lib:
                continue
            res = run_match(compare, match)
            if res.counts["="] == res.total() and len(res.orig) == len(res.ours):
                continue
            print(f"0x{res.orig_addr:x} {res.name}")
            print("    " + summary_line(res))
        return

    res = run_match(compare, find_match(compare, args.function))
    print_result(res, compare.function_comparator, args.full, args.context)


if __name__ == "__main__":
    main()
