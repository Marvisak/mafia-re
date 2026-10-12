"""Compare a whole rebuilt binary with the original: headers, sections, and the layout and content
of every known object. This covers what reccmp can't: bytes outside functions, the order of
functions and data, and header fields.

Pointers that differ only because their target moved count as equal. Base relocations say which
fields are pointers (in an EXE without them, any differing dword whose target translates counts),
and reccmp's matches say where each target ended up. reccmp works on the
vc6-pdb build, so its addresses are carried over to the byte-exact vc6 build by symbol name
through the two map files.

Build both presets first, then run from the repository root:

    .venv/Scripts/python tools/filediff.py                 # problems only
    .venv/Scripts/python tools/filediff.py --objects       # every object, in the original's order
"""

import argparse
import bisect
import logging
import re
import struct
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from pathlib import Path

from reccmp.compare import Compare
from reccmp.project.detect import RecCmpProject
from reccmp.types import EntityType

REPO = Path(__file__).resolve().parent.parent

COFF_FIELDS = [
    ("Machine", 0, "H"), ("NumberOfSections", 2, "H"), ("TimeDateStamp", 4, "I"),
    ("PointerToSymbolTable", 8, "I"), ("NumberOfSymbols", 12, "I"),
    ("SizeOfOptionalHeader", 16, "H"), ("Characteristics", 18, "H"),
]
OPT_FIELDS = [
    ("Magic", 0, "H"), ("MajorLinkerVersion", 2, "B"), ("MinorLinkerVersion", 3, "B"),
    ("SizeOfCode", 4, "I"), ("SizeOfInitializedData", 8, "I"), ("SizeOfUninitializedData", 12, "I"),
    ("AddressOfEntryPoint", 16, "I"), ("BaseOfCode", 20, "I"), ("BaseOfData", 24, "I"),
    ("ImageBase", 28, "I"), ("SectionAlignment", 32, "I"), ("FileAlignment", 36, "I"),
    ("MajorOperatingSystemVersion", 40, "H"), ("MinorOperatingSystemVersion", 42, "H"),
    ("MajorImageVersion", 44, "H"), ("MinorImageVersion", 46, "H"),
    ("MajorSubsystemVersion", 48, "H"), ("MinorSubsystemVersion", 50, "H"),
    ("Win32VersionValue", 52, "I"), ("SizeOfImage", 56, "I"), ("SizeOfHeaders", 60, "I"),
    ("CheckSum", 64, "I"), ("Subsystem", 68, "H"), ("DllCharacteristics", 70, "H"),
    ("SizeOfStackReserve", 72, "I"), ("SizeOfStackCommit", 76, "I"),
    ("SizeOfHeapReserve", 80, "I"), ("SizeOfHeapCommit", 84, "I"),
    ("LoaderFlags", 88, "I"), ("NumberOfRvaAndSizes", 92, "I"),
]
DIRECTORIES = ["Export", "Import", "Resource", "Exception", "Security", "BaseReloc", "Debug",
               "Architecture", "GlobalPtr", "TLS", "LoadConfig", "BoundImport", "IAT",
               "DelayImport", "COMDescriptor", "Reserved"]


@dataclass
class Section:
    name: str
    vsize: int
    va: int
    rsize: int
    raw: int
    characteristics: int


class PE:
    def __init__(self, path: Path):
        self.path = path
        self.data = open(path, "rb").read()
        d = self.data
        self.pe = struct.unpack_from("<I", d, 0x3C)[0]
        self.coff = {n: struct.unpack_from("<" + f, d, self.pe + 4 + o)[0] for n, o, f in COFF_FIELDS}
        self.opt_off = self.pe + 24
        self.opt = {n: struct.unpack_from("<" + f, d, self.opt_off + o)[0] for n, o, f in OPT_FIELDS}
        self.base = self.opt["ImageBase"]
        self.dirs = [struct.unpack_from("<II", d, self.opt_off + 96 + 8 * i) for i in range(16)]
        self.sections = []
        off = self.opt_off + self.coff["SizeOfOptionalHeader"]
        for _ in range(self.coff["NumberOfSections"]):
            name = d[off:off + 8].rstrip(b"\0").decode("latin-1")
            vsize, va, rsize, raw = struct.unpack_from("<IIII", d, off + 8)
            ch = struct.unpack_from("<I", d, off + 36)[0]
            self.sections.append(Section(name, vsize, va, rsize, raw, ch))
            off += 40
        self.relocs = self._relocs()

    def section_of(self, va: int) -> Section | None:
        rva = va - self.base
        for s in self.sections:
            if s.va <= rva < s.va + max(s.vsize, s.rsize):
                return s
        return None

    def offset(self, va: int) -> int | None:
        s = self.section_of(va)
        if s is None:
            return None
        rva = va - self.base
        if rva - s.va >= s.rsize:
            return None  # uninitialized part
        return s.raw + rva - s.va

    def read(self, va: int, size: int) -> bytes | None:
        """None if va is in no section's raw data; past the end of the raw data the section reads as zeros."""
        o = self.offset(va)
        if o is None:
            return None
        s = self.section_of(va)
        raw_left = s.va + s.rsize - (va - self.base)
        data = self.data[o:o + min(size, raw_left)]
        return data + bytes(size - len(data))

    def u32(self, va: int) -> int:
        return struct.unpack("<I", self.read(va, 4))[0]

    def _relocs(self) -> set[int]:
        rva, size = self.dirs[5]
        out = set()
        if not rva:
            return out
        pos, end = self.base + rva, self.base + rva + size
        while pos < end:
            page, block = struct.unpack("<II", self.read(pos, 8))
            if block == 0:
                break
            entries = self.read(pos + 8, block - 8)
            for (e,) in struct.iter_unpack("<H", entries):
                if e >> 12 == 3:
                    out.add(self.base + page + (e & 0xFFF))
            pos += block
        return out

    def cstring(self, va: int) -> str:
        o = self.offset(va)
        end = self.data.index(b"\0", o)
        return self.data[o:end].decode("latin-1")

    def imports(self) -> dict[str, int]:
        """IAT slot address for every DLL!name."""
        rva, _ = self.dirs[1]
        out = {}
        pos = self.base + rva
        while True:
            oft, _, _, name, ft = struct.unpack("<IIIII", self.read(pos, 20))
            if not ft:
                break
            dll = self.cstring(self.base + name).upper()
            thunks = oft or ft
            i = 0
            while True:
                t = self.u32(self.base + thunks + 4 * i)
                if not t:
                    break
                fn = f"#{t & 0xFFFF}" if t & 0x80000000 else self.cstring(self.base + t + 2)
                out[f"{dll}!{fn}"] = self.base + ft + 4 * i
                i += 1
            pos += 20
        return out

    def export_fields(self) -> tuple[set[int], int | None]:
        """Addresses of the export address table's RVA entries, and of the export timestamp."""
        rva, _ = self.dirs[0]
        if not rva:
            return set(), None
        d = self.read(self.base + rva, 40)
        nfun, _, afun = struct.unpack_from("<III", d, 20)
        return {self.base + afun + 4 * i for i in range(nfun)}, self.base + rva + 4

    def exports(self) -> tuple[int, dict[str, int]]:
        rva, _ = self.dirs[0]
        if not rva:
            return 0, {}
        d = self.read(self.base + rva, 40)
        ts = struct.unpack_from("<I", d, 4)[0]
        nfun, nnames, afun, anames, aords = struct.unpack_from("<IIIII", d, 20)
        out = {}
        for i in range(nnames):
            name = self.cstring(self.base + self.u32(self.base + anames + 4 * i))
            ordinal = struct.unpack("<H", self.read(self.base + aords + 2 * i, 2))[0]
            out[name] = self.base + self.u32(self.base + afun + 4 * ordinal)
        return ts, out


def load_map(path: Path) -> dict[str, int]:
    syms = {}
    for line in open(path, encoding="latin-1"):
        m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s", line)
        if m:
            syms.setdefault(m.group(1), int(m.group(2), 16))
    return syms


@dataclass
class Obj:
    orig: int | None
    ours: int | None
    name: str
    kind: str
    orig_end: int = 0
    ours_end: int = 0
    status: str = ""
    notes: list[str] = field(default_factory=list)
    diffs: list[tuple[int, int, str]] = field(default_factory=list)  # (offset, length, kind)


def build_objects(compare: Compare, pdb_map: dict, ours_map: dict, orig: PE, ours: PE) -> list[Obj]:
    # recomp address in the vc6-pdb build -> address in the vc6 build, via the nearest symbol below
    pdb_by_addr = sorted((a, n) for n, a in pdb_map.items() if n in ours_map)
    pdb_addrs = [a for a, _ in pdb_by_addr]

    def to_ours(addr: int) -> int:
        i = bisect.bisect_right(pdb_addrs, addr) - 1
        if i < 0:
            return addr
        a, n = pdb_by_addr[i]
        return ours_map[n] + addr - a

    objs = {}
    for e in compare.get_all():
        t = e.get("type")
        if t in (EntityType.LINE, EntityType.LABEL):
            continue
        kind = EntityType(t).name.lower() if t is not None else "symbol"
        name = e.best_name() or "?"
        if e.orig_addr is not None:
            # reccmp also reports strings it found by scanning the original's data; without a
            # counterpart they only cut other objects short
            if e.recomp_addr is None and kind == "string":
                s = orig.section_of(e.orig_addr)
                if s is not None and s.characteristics & 0x80000000:
                    continue
            ours_addr = to_ours(e.recomp_addr) if e.recomp_addr is not None else None
            objs.setdefault(e.orig_addr, Obj(e.orig_addr, ours_addr, name, kind))
        elif e.recomp_addr is not None:
            # known only from our PDB (EH tables, unannotated CRT): placed by infer_ours_only
            objs.setdefault(("ours", e.recomp_addr), Obj(None, to_ours(e.recomp_addr), name, "ours only"))

    # imports straight from both import tables
    oi, ui = orig.imports(), ours.imports()
    for name, slot in oi.items():
        o = objs.get(slot)
        if o is None or o.ours is None:
            objs[slot] = Obj(slot, ui.get(name), name, "import")

    result = list(objs.values())
    matched_ours = {o.ours for o in result if o.ours is not None}
    # everything else our map knows about, so the extents of our objects end where they should
    for name, addr in ours_map.items():
        if addr not in matched_ours and ours.section_of(addr) is not None:
            result.append(Obj(None, addr, name, "ours only"))
            matched_ours.add(addr)
    return result


def infer_ours_only(objs: list[Obj], orig: PE, ours: PE) -> None:
    """Our map knows every symbol, the original only the matched ones. Place a symbol that only
    our map has into the original at the same offset from the matched object before it (code and
    data that only moved, like the CRT), as long as that doesn't run into the next known object."""
    orig_starts = sorted(o.orig for o in objs if o.orig is not None)
    orig_only = {o.orig: o for o in objs if o.orig is not None and o.ours is None}
    by_section = defaultdict(list)
    for o in objs:
        if o.ours is not None:
            s = ours.section_of(o.ours)
            if s:
                by_section[s.name].append(o)
    for name, lst in by_section.items():
        lst.sort(key=lambda o: o.ours)
        anchor = None
        for o in lst:
            if o.orig is not None:
                anchor = o
                continue
            if anchor is None:
                continue
            guess = anchor.orig + (o.ours - anchor.ours)
            s = orig.section_of(guess)
            i = bisect.bisect_right(orig_starts, anchor.orig)
            nxt = orig_starts[i] if i < len(orig_starts) else None
            if s is None or s.name != name:
                continue
            if guess in orig_only and orig_only[guess].ours is None:
                # the same object, known to reccmp only in the original
                orig_only[guess].ours = o.ours
                o.ours = None
                anchor = orig_only[guess]
            elif nxt is None or guess < nxt:
                o.orig = guess
                o.kind = "inferred"
    objs[:] = [o for o in objs if o.orig is not None or o.ours is not None]


def branch_fixups(a: bytes, b: bytes, oa: int, ua: int, translate) -> tuple[dict[int, int], int, list[int]]:
    """rel32 calls and jumps aren't base relocations. Find them in the original's code and check
    the rebuild's operand at the same offset against the translated target. Returns
    {offset: 4} for operands to skip in the byte comparison, the number of operands that differ
    only because the target moved, and the offsets of operands that point somewhere else."""
    import capstone

    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    skip, moved, wrong = {}, 0, []
    for insn in md.disasm(a, oa):
        k = insn.address - oa
        raw = bytes(insn.bytes)
        if len(raw) < 5 or not (raw[0] in (0xE8, 0xE9) or (raw[0] == 0x0F and 0x80 <= raw[1] <= 0x8F)):
            continue
        op = k + len(raw) - 4
        if b[k:op] != raw[:-4] or op + 4 > len(b):
            continue
        ta = oa + k + len(raw) + struct.unpack_from("<i", a, op)[0]
        tb = ua + k + len(raw) + struct.unpack_from("<i", b, op)[0]
        if ta == tb - (ua - oa) or translate(ta) == tb:
            if a[op:op + 4] != b[op:op + 4]:
                moved += 1
        else:
            wrong.append(op)
        skip[op] = 4
    return skip, moved, wrong


def assign_extents(objs: list[Obj], orig: PE, ours: PE) -> None:
    def extents(side: str, pe: PE):
        starts = defaultdict(list)
        for o in objs:
            a = getattr(o, side)
            if a is not None:
                s = pe.section_of(a)
                if s:
                    starts[s.name].append(a)
        for s in pe.sections:
            lst = sorted(set(starts[s.name]))
            end = pe.base + s.va + (s.vsize or s.rsize)  # not the file padding after the section
            nxt = {a: (lst[i + 1] if i + 1 < len(lst) else end) for i, a in enumerate(lst)}
            for o in objs:
                a = getattr(o, side)
                if a in nxt and pe.section_of(a) is s:
                    setattr(o, side + "_end", nxt[a])

    extents("orig", orig)
    extents("ours", ours)


def compare_content(o: Obj, orig: PE, ours: PE, translate) -> None:
    osz, usz = o.orig_end - o.orig, o.ours_end - o.ours
    a, b = orig.read(o.orig, osz), ours.read(o.ours, usz)
    if osz != usz:
        o.status = "size"
        o.notes.append(f"size 0x{osz:x} vs 0x{usz:x} ({'+' if usz > osz else '-'}0x{abs(usz - osz):x} in ours)")
    if a is None or b is None:
        if o.status != "size":
            o.status = "bss"
        return
    n = min(len(a), len(b))
    real = moved = wrong = 0
    skip = {}
    s = orig.section_of(o.orig)
    if s is not None and s.characteristics & 0x20000000:  # executable
        skip, moved, wrong_branches = branch_fixups(a[:n], b[:n], o.orig, o.ours, translate)
        wrong = len(wrong_branches)
        o.diffs.extend((off, 4, "branch") for off in wrong_branches)
    orig_rvas, orig_ts = orig.export_fields()
    ours_rvas, ours_ts = ours.export_fields()
    i = 0
    while i < n:
        if i in skip:
            i += skip[i]
            continue
        if o.orig + i == orig_ts and o.ours + i == ours_ts:
            if a[i:i + 4] != b[i:i + 4]:
                o.notes.append("export timestamp (link time)")
            i += 4
            continue
        if a[i] != b[i] and not orig.relocs and not ours.relocs:
            # An EXE has no base relocations to say where the pointers are. Take a differing dword
            # whose original value translates to ours as one, anything else stays a byte difference.
            w = next((w for w in range(max(i - 3, 0), i + 1) if w + 4 <= n
                      and translate(struct.unpack_from("<I", a, w)[0]) == struct.unpack_from("<I", b, w)[0]), None)
            if w is not None:
                moved += 1
                i = w + 4
                continue
        is_rva = o.orig + i in orig_rvas and o.ours + i in ours_rvas
        if (is_rva or (o.orig + i in orig.relocs and o.ours + i in ours.relocs)) and i + 4 <= n:
            va = struct.unpack_from("<I", a, i)[0]
            vb = struct.unpack_from("<I", b, i)[0]
            if is_rva:
                va, vb = va + orig.base, vb + ours.base
            if va != vb:
                if translate(va) == vb:
                    moved += 1
                else:
                    wrong += 1
                    o.diffs.append((i, 4, "pointer"))
            i += 4
            continue
        if a[i] != b[i]:
            real += 1
            if o.diffs and o.diffs[-1][2] == "bytes" and o.diffs[-1][0] + o.diffs[-1][1] == i:
                o.diffs[-1] = (o.diffs[-1][0], o.diffs[-1][1] + 1, "bytes")
            else:
                o.diffs.append((i, 1, "bytes"))
        i += 1
    if real or wrong:
        o.status = o.status or "content"
        if real:
            o.notes.append(f"{real} bytes differ")
        if wrong:
            o.notes.append(f"{wrong} pointers to different targets")
    elif o.status != "size":
        o.status = "moved pointers" if moved else "identical"
    if moved and o.status != "moved pointers":
        o.notes.append(f"{moved} pointers differ only by moved targets")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--target", default="RWDATA")
    parser.add_argument("--pdb-build", default=str(REPO / "build" / "vc6-pdb"), help="build tree reccmp uses")
    parser.add_argument("--ours", help="byte-exact build of the binary (default: same path under build/vc6)")
    parser.add_argument("--objects", action="store_true", help="list every object, not just problems")
    parser.add_argument("--bytes", metavar="ADDR", help="dump the differing bytes of the object at this original address")
    args = parser.parse_args()

    logging.basicConfig(level=logging.ERROR)
    target = RecCmpProject.from_directory(Path(args.pdb_build)).get(args.target)
    compare = Compare.from_target(target)
    pdb_path = Path(target.recompiled_path)
    ours_path = Path(args.ours) if args.ours else Path(str(pdb_path).replace("vc6-pdb", "vc6"))
    orig, ours = PE(Path(target.original_path)), PE(ours_path)
    pdb_map = load_map(pdb_path.with_suffix(".map"))
    ours_map = load_map(ours_path.with_suffix(".map"))

    print(f"original: {orig.path}\nrebuild:  {ours.path}\n")
    same = sum(x == y for x, y in zip(orig.data, ours.data))
    print(f"file size 0x{len(orig.data):x} vs 0x{len(ours.data):x}; "
          f"{same} of {len(orig.data)} bytes identical at the same offset ({100 * same / len(orig.data):.1f}%)\n")

    # headers
    print("== Headers")
    stub = orig.data[:orig.pe] == ours.data[:ours.pe]
    print(f"  DOS header, stub and Rich header (0x0-0x{orig.pe:x}): {'identical' if stub else 'DIFFER'}")
    for n, _, _ in COFF_FIELDS:
        if orig.coff[n] != ours.coff[n]:
            note = "  (link time, VC6 LINK always writes the current time)" if n == "TimeDateStamp" else ""
            print(f"  COFF {n}: 0x{orig.coff[n]:x} vs 0x{ours.coff[n]:x}{note}")
    for n, _, _ in OPT_FIELDS:
        if orig.opt[n] != ours.opt[n]:
            print(f"  Optional {n}: 0x{orig.opt[n]:x} vs 0x{ours.opt[n]:x}")
    for i, (x, y) in enumerate(zip(orig.dirs, ours.dirs)):
        if x != y:
            print(f"  Directory {DIRECTORIES[i]}: rva 0x{x[0]:x} size 0x{x[1]:x} vs rva 0x{y[0]:x} size 0x{y[1]:x}")
    ots, oexp = orig.exports()
    uts, uexp = ours.exports()
    if ots != uts:
        print(f"  Export directory TimeDateStamp: 0x{ots:x} vs 0x{uts:x}  (link time)")
    if set(oexp) != set(uexp):
        print(f"  Export names differ: only orig {sorted(set(oexp) - set(uexp))}, only ours {sorted(set(uexp) - set(oexp))}")
    oi, ui = orig.imports(), ours.imports()
    if list(oi) != list(ui):
        print(f"  Import order or set differs: only orig {sorted(set(oi) - set(ui))}, only ours {sorted(set(ui) - set(oi))}")
    print(f"  base relocations: {len(orig.relocs)} vs {len(ours.relocs)}")

    print("\n== Sections (orig vs ours)")
    for s, t in zip(orig.sections, ours.sections):
        diff = [f"{k} 0x{getattr(s, k):x} vs 0x{getattr(t, k):x}" for k in ("vsize", "va", "rsize", "raw")
                if getattr(s, k) != getattr(t, k)]
        print(f"  {s.name:8} va 0x{s.va:x} vsize 0x{s.vsize:x} raw 0x{s.rsize:x}" + ("  | " + ", ".join(diff) if diff else "  | same"))

    objs = build_objects(compare, pdb_map, ours_map, orig, ours)
    infer_ours_only(objs, orig, ours)
    assign_extents(objs, orig, ours)
    pairs = sorted((o for o in objs if o.orig is not None and o.ours is not None), key=lambda o: o.orig)
    starts = [o.orig for o in pairs]

    def translate(v: int) -> int | None:
        i = bisect.bisect_right(starts, v) - 1
        if i < 0:
            return None
        o = pairs[i]
        if v >= o.orig_end and o.orig_end:
            return None
        return o.ours + v - o.orig

    for o in pairs:
        if o.orig_end and o.ours_end:
            compare_content(o, orig, ours, translate)

    if args.bytes:
        addr = int(args.bytes, 16)
        o = next((p for p in pairs if p.orig <= addr < p.orig_end), None)
        if o is None:
            raise SystemExit(f"no matched object at 0x{addr:x}")
        print(f"{o.name}: orig 0x{o.orig:x}-0x{o.orig_end:x}, ours 0x{o.ours:x}-0x{o.ours_end:x}, {o.status}"
              + (" | " + "; ".join(o.notes) if o.notes else ""))
        for off, length, kind in o.diffs:
            a = orig.read(o.orig + off, length).hex(" ")
            b = ours.read(o.ours + off, length).hex(" ")
            print(f"  +0x{off:04x} {kind:7} orig {o.orig + off:08x}: {a}\n  {'':15} ours {o.ours + off:08x}: {b}")
        return

    for s in orig.sections:
        sec = [o for o in objs if o.orig is not None and orig.section_of(o.orig) is s]
        sec.sort(key=lambda o: o.orig)
        ours_sec = [o for o in objs if o.ours is not None and ours.section_of(o.ours) is not None
                    and ours.section_of(o.ours).name == s.name]
        if not sec and not ours_sec:
            continue
        counts = Counter(o.status or ("orig only" if o.ours is None else "?") for o in sec)
        print(f"\n== {s.name}: {len(sec)} known objects in the original; " + ", ".join(f"{v} {k}" for k, v in counts.most_common()))

        # where the layout shifts: the offset between orig and ours changes
        last_delta = None
        order = [o for o in sec if o.ours is not None]
        out_of_order = 0
        for x, y in zip(order, order[1:]):
            if y.ours < x.ours:
                out_of_order += 1
        if out_of_order:
            print(f"  {out_of_order} objects are in a different order in ours (an object placed before its predecessor)")
        for o in sec:
            delta = (o.ours - o.orig) if o.ours is not None else None
            shift = delta is not None and delta != last_delta
            show = args.objects or o.status not in ("identical", "moved pointers", "bss") or o.ours is None
            if shift and last_delta is not None:
                show = True
            if show:
                d = f"{delta:+#x}" if delta is not None else "  -  "
                ours_s = f"{o.ours:08x}" if o.ours is not None else "--------"
                status = o.status or ("orig only" if o.ours is None else "")
                if shift and last_delta is not None:
                    status = (status + ", " if status else "") + f"layout shifts here (was {last_delta:+#x})"
                size = f"0x{o.orig_end - o.orig:x}" if o.orig_end else "?"
                print(f"  {o.orig:08x} {ours_s} {d:>8}  {size:>7}  {o.kind:8}  {o.name[:60]:60}  {status}"
                      + (" | " + "; ".join(o.notes) if o.notes else ""))
            if delta is not None:
                last_delta = delta
        only_ours = [o for o in ours_sec if o.orig is None]
        if only_ours and args.objects:
            print(f"  symbols in ours without a known original: {len(only_ours)}")
            for o in sorted(only_ours, key=lambda o: o.ours):
                print(f"           {o.ours:08x}            0x{o.ours_end - o.ours:x}  {o.name}")
    print()


if __name__ == "__main__":
    main()
