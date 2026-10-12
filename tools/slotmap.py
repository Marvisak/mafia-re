"""Print VC6's stack slot assignment for one function from a /FAs assembly listing.

The listing is the only place VC6 names its frame slots: every local, inlined-function local
(`_count$1234`) and compiler temporary (`$T5678`, `tv80`) gets a `name = -offset` line above the
function, and unnamed temporaries show up as `-N+[esp+M]` operands. This tool lists each slot with
its occupants, the number of memory references to it (loads, stores, lea; a read-modify-write
counts as both) and the first and last listing line that touches it. Those are the quantities
VC6's frame layout works from (tools/framelayout.py models it).

Make the listing with the project's compile flags plus /FAs, e.g. for rw_data:

    3rdparty/msvc600_sp5_vcpp/VC98/Bin/CL.EXE /nologo /O2 /ML /GX /Gz /c /FAs /Fa%TEMP%/rw_data.asm \
        /Fo%TEMP%/rw_data.obj /I common rw_data/rw_data.cpp

usage: slotmap.py <listing.asm> <decorated function name>      e.g. _dtaRead@12
"""
import re
import sys
from collections import defaultdict

RMW = ("inc", "dec", "add", "sub", "and", "or", "xor", "shl", "shr", "sar", "neg", "not", "adc", "sbb")


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    text = open(sys.argv[1]).read()
    fn = sys.argv[2]
    m = re.search(r"\n" + re.escape(fn) + r"\s+PROC NEAR(.*?)\n" + re.escape(fn) + r"\s+ENDP", text, re.S)
    if not m:
        print(f"no PROC/ENDP pair for {fn}")
        return 1
    body = m.group(1)
    start = text.find("\n" + fn + "\tPROC NEAR")
    if start < 0:
        start = text.find("\n" + fn + " PROC NEAR")
    pre = text[:start]
    syms = dict(re.findall(r"^(\S+) = (-?\d+)", pre[max(pre.rfind("ENDP"), 0):], re.M))
    fm = re.search(r"mov\s+eax, (\d+)\s*;.*\n\s*call\s+__chkstk", body) or re.search(r"\n\s*sub\s+esp, (\d+)", body)
    frame = int(fm.group(1)) if fm else 0

    refs = defaultdict(lambda: [0, 0, 0, None, None])  # loads, stores, lea, first line, last line
    for ln, line in enumerate(body.split("\n")):
        s = line.strip()
        if not s or s.startswith(";") or s.endswith(":"):
            continue
        parts = s.split(None, 1)
        mn, ops = parts[0], parts[1] if len(parts) > 1 else ""
        for ref in re.finditer(r"(?:([A-Za-z_$?][\w$?@]*)|(-?\d+)\+)\[esp", ops):
            sym = ref.group(1)
            if sym is None:
                sym = "tmp" + ref.group(2)
                syms.setdefault(sym, ref.group(2))
            if sym not in syms:
                continue
            r = refs[sym]
            memdst = ops.split(",")[0].strip().endswith("]")
            if mn == "lea":
                r[2] += 1
            elif mn in ("cmp", "test", "push") or not memdst:
                r[0] += 1
            elif mn in RMW:
                r[0] += 1
                r[1] += 1
            else:
                r[1] += 1
            if r[3] is None:
                r[3] = ln
            r[4] = ln

    rows = sorted((frame + int(syms[sym]), sym, *r) for sym, r in refs.items())
    print(f"{fn}: frame {frame} (0x{frame:x}) bytes; offsets are from the frame bottom (esp after the prologue's sub)")
    last = None
    for pos, sym, loads, stores, lea, first, final in rows:
        name = re.sub(r"\$\d+$", "", sym).lstrip("_")
        marker = "  " if pos == last else "--"
        print(f"{marker} {pos:6d} 0x{pos:04x}  {name:34s} loads {loads:3d} stores {stores:3d} lea {lea:2d}"
              f" total {loads + stores + lea:3d}  lines {first:5d}..{final:5d}")
        last = pos
    return 0


if __name__ == "__main__":
    sys.exit(main())
