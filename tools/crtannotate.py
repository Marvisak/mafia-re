"""Write a target's library_msvc.h: reccmp annotations for the static CRT linked into it.

Both builds link the same CRT objects in the same order, so each CRT function of ours sits in the
original at nearly the same offset as the one before it; only the padding in front of 16-byte aligned
objects can differ while the code before the CRT has another size. Starting from the entry points
(the CRT's startup code), each public CRT function of our map file is looked up within 15 bytes of the
previous offset and accepted when its bytes agree apart from pointer operands, and its calls and jumps to
functions already found go to their original addresses. Those operands then say where the CRT's globals
are in the original, so data addresses come from the original's own code, not from our map plus an offset:
a pointer places the whole contribution of the object it points into (our PDB's section contributions),
which both builds lay out the same, statics included. A source that isn't complete yet links only some
of the original's CRT objects, so after a missing one the next function is further away: a function not
found near the previous offset is looked for in the whole of the original's .text, and taken if it fits
at exactly one place. Identical functions (_memcpy and _memmove, each in its own object) fit at several; the
functions reccmp already matches tell them apart, by which of those places the original calls where ours
calls the twin, and the place left over is the last twin's.

What it writes:
- LIBRARY for every function found, GLOBAL for the globals the CRT code references (including the
  communal ones the map lists as <common>), and STRING for the string literals it references, each
  under the name reccmp gives our copy. Where several public names share an address (__chkstk,
  __alloca_probe), it uses the one reccmp kept from our PDB. Where the CRT objects carry debug info
  (VS2008's, not VC6's), our PDB also names static functions, which get a LIBRARY annotation by that
  name, and static data (GS_ContextRecord), and it gives globals their C names (__security_cookie for
  ___security_cookie, __xc_a for ___xc_a), which are what reccmp matches.
- LIBRARY for the import thunks (jmp [IAT slot]), found in the original through its import table:
  reccmp only recognizes thunks at relocated operands.
- library_msvc.csv with the CRT's float constants, when there are any. reccmp only finds floats at
  relocated operands and an EXE has none, so add the csv to the target's data_sources.

reccmp finds the original's strings, floats and thunks itself at relocated operands, so for an original
with base relocations only the two empty literals get a STRING (its string search skips them), and
neither thunks nor the csv are written. In an EXE without relocations an unannotated literal costs more
than its own line: reccmp numbers its <OFFSETn> placeholders by how many operands it has replaced so
far, named or not, so a literal named on our side only renumbers every placeholder after it.

Without debug info (VC6's CRT) it can't annotate the CRT's static data and labels: there is no name on
either side. In an EXE reccmp then compares their raw addresses, which differ wherever the layout before
them does, so those functions match only once the layout does (_pgmname in __setargv, _unwind_handler in
__abnormal_termination).

A function the target's own annotations (FUNCTION, SYNTHETIC, TEMPLATE, STUB) already place in the original is left
to them: an identical function of the library folded with one of the target's, of which the original kept the
target's copy and our build the library's.

With --library it annotates the objects of another static library instead (LS3DF's STRMBASE, DirectShow's base
classes), the same way: functions by their bytes, globals and vtables by the pointers to them. It writes
library_<name>.h, with no import thunks.

Run it from the repository root after building the target's PDB preset:

    .venv/Scripts/python tools/crtannotate.py SETUP
    .venv/Scripts/python tools/crtannotate.py SETUP10 --out library_msvc_1_0.h
    .venv/Scripts/python tools/crtannotate.py SETUPSTEAM --pdb-build build/vs2008 --out library_msvc_steam.h
    .venv/Scripts/python tools/crtannotate.py LS3DF --library STRMBASE
"""

import argparse
import bisect
import logging
import re
import struct
from collections import defaultdict
from pathlib import Path

import capstone
from reccmp.compare import Compare
from reccmp.cvdump import Cvdump
from reccmp.project.detect import RecCmpProject
from reccmp.types import EntityType, ImageId

REPO = Path(__file__).resolve().parent.parent

HEADER = """\
// reccmp annotations for the static CRT functions and data {name} uses, including the CRT's own internal
// callees, and for its import thunks: without an annotation reccmp can't name a call target in the
// original, so it scores the call as a mismatch even though the code is identical.{floats}
// This file is never compiled, it only tells reccmp where these functions are in the original.
// Generated by tools/crtannotate.py.
"""

LIBRARY_HEADER = """\
// reccmp annotations for the functions and data of the static library {library} that {name} links: without
// an annotation reccmp can't name a call target in the original, so it scores the call as a mismatch even
// though the code is identical.
// This file is never compiled, it only tells reccmp where these functions are in the original.
// Generated by tools/crtannotate.py --library {library}.
"""

MAP_LINE = re.compile(r"^\s*([0-9a-f]{4}):([0-9a-f]{8})\s+(\S+)\s+([0-9a-f]{8})\s+(?:f\s+)?(?:i\s+)?(\S*)\s*$")

# The empty literals, which reccmp's string search never finds, relocated or not
EMPTY_STRINGS = {"??_C@_01A@?$AA?$AA@", "??_C@_13A@?$AA?$AA?$AA?$AA@"}

# The object of static data known only from our PDB
STATIC_DATA = "<static>"

# reccmp's name of a vtable: X::`vftable', or X::`vftable'{for `Y'} for one of multiple inheritance
VTABLE_NAME = re.compile(r"^(.+)::`vftable'(?:\{for `(.+)'\})?$")

# A function annotation of the target's own code
OWN_MARKER = re.compile(r"//\s*(FUNCTION|TEMPLATE|SYNTHETIC|STUB):\s*(\w+)\s+(0x[0-9a-fA-F]+)")


class PE:
    def __init__(self, path: Path):
        self.data = path.read_bytes()
        pe = struct.unpack_from("<I", self.data, 0x3C)[0]
        nsec = struct.unpack_from("<H", self.data, pe + 6)[0]
        opt = pe + 24
        self.base = struct.unpack_from("<I", self.data, opt + 28)[0]
        self.entry = self.base + struct.unpack_from("<I", self.data, opt + 16)[0]
        self.size = struct.unpack_from("<I", self.data, opt + 56)[0]
        optsz = struct.unpack_from("<H", self.data, pe + 20)[0]
        self.sections = []
        for k in range(nsec):
            s = opt + optsz + 40 * k
            name = self.data[s:s + 8].rstrip(b"\0").decode()
            vs, va, rs, rp = struct.unpack_from("<IIII", self.data, s + 8)
            self.sections.append((name, self.base + va, vs, rp, rs))
        self.import_dir = struct.unpack_from("<I", self.data, opt + 96 + 8)[0]
        self.relocated = struct.unpack_from("<I", self.data, opt + 96 + 5 * 8 + 4)[0] != 0  # base relocations

    def imports(self) -> dict[tuple[str, str], int]:
        """(lowercase DLL name, function name) -> address of its IAT slot."""
        def cstr(rva):  # byte by byte: a name can end right at the end of its section
            out = b""
            while (c := self.read(self.base + rva + len(out), 1)) and c != b"\0":
                out += c
            return out.decode()

        slots = {}
        desc = self.base + self.import_dir
        while True:
            lookup, _, _, name, iat = struct.unpack("<IIIII", self.read(desc, 20))
            if not iat:
                return slots
            dll = cstr(name).lower()
            k = 0
            while entry := struct.unpack("<I", self.read(self.base + (lookup or iat) + 4 * k, 4))[0]:
                func = f"#{entry & 0xFFFF}" if entry & 0x80000000 else cstr(entry + 2)  # by ordinal or by name
                slots[(dll, func)] = self.base + iat + 4 * k
                k += 1
            desc += 20

    def section(self, name: str):
        return next(s for s in self.sections if s[0] == name)

    def read(self, addr: int, n: int) -> bytes | None:
        for _, va, vs, rp, rs in self.sections:
            if va <= addr and addr + n <= va + max(vs, rs):
                off = addr - va
                return self.data[rp + off:rp + off + n].ljust(n, b"\0")
        return None

    def contains(self, addr: int) -> bool:
        return self.base <= addr < self.base + self.size


def parse_map(path: Path, statics: bool = False) -> list[tuple[int, str, str]]:
    """Public symbols (address, name, lib:object) of a map file, sorted by address, or its static ones."""
    symbols = []
    in_statics = False
    for line in path.read_text(errors="replace").splitlines():
        if "Static symbols" in line:
            in_statics = True
        m = MAP_LINE.match(line)
        if m and m.group(1) != "0000" and in_statics == statics:
            symbols.append((int(m.group(4), 16), m.group(3), m.group(5)))
    return sorted(symbols)


def is_crt(obj: str) -> bool:
    # The CRT's uninitialized C globals are communal, VC6's C++ globals never are. MSVCRT: the static part
    # of the DLL CRT's import library (startup code, security cookie). LIBCP/LIBCPMT: the C++ library
    # (std::_Lockit, which the STL's trees lock with).
    return obj == "<common>" or obj.upper().startswith(
        ("LIBC:", "LIBCMT:", "LIBCP:", "LIBCPMT:", "MSVCRT:", "OLDNAMES:"))


def rel32_operand(code: bytes, w: int) -> bool:
    """Whether the dword at w follows the opcode of a call, jmp or conditional jump with a rel32 operand. Without
    this test any small dword passes for one, its target landing inside both images: CPosPassThru's AddRef and
    Release differ only in a vtable displacement (call [ecx+4], call [ecx+8]), which with the ret after it read as
    a rel32, so each fit at the other's place."""
    return w >= 1 and code[w - 1] in (0xE8, 0xE9) or w >= 2 and code[w - 2] == 0x0F and 0x80 <= code[w - 1] <= 0x8F


def compare(ours: PE, orig: PE, a: int, b: int, n: int, limit: int) -> list[tuple[int, int]] | None:
    """Compare n bytes at our a and the original's b, with pointer operands reaching up to limit
    (padding trimmed from n can include an operand's last zero byte). Returns the differing dwords as
    (our value, original value) pairs, absolute or as rel32 targets, or None if other bytes differ."""
    x, y = ours.read(a, limit), orig.read(b, limit)
    if x is None or y is None:
        return None
    pairs = []
    i = 0
    while i < n:
        if x[i] == y[i]:
            i += 1
            continue
        for w in range(max(i - 3, 0), i + 1):
            if w + 4 > limit:
                return None
            vx, vy = struct.unpack_from("<I", x, w)[0], struct.unpack_from("<I", y, w)[0]
            if ours.contains(vx) and orig.contains(vy):
                pairs.append((vx, vy))
                break
            tx, ty = (a + w + 4 + vx) & 0xFFFFFFFF, (b + w + 4 + vy) & 0xFFFFFFFF
            if rel32_operand(x, w) and ours.contains(tx) and orig.contains(ty):
                pairs.append((tx, ty))
                break
        else:
            return None
        i = w + 4
    return pairs


def candidate_sizes(ours: PE, a: int, n: int) -> list[int]:
    """The sizes to compare a function at our a by: n, the range up to the next symbol, then the shorter
    prefixes that end after a ret or jmp, longest first. The range can end with bytes that aren't the
    function's, which the original needn't have after it: the next object's leading data (exsup3's
    "VC20XC00" marker), padding inside it, or static functions the original links elsewhere."""
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    ends = {insn.address + insn.size - a for insn in md.disasm(ours.read(a, n), a)
            if insn.mnemonic in ("ret", "jmp")}
    return [n] + sorted((e for e in ends if e < n), reverse=True)


def call_targets(pe: PE, a: int, n: int) -> set[int]:
    """The targets of the direct calls and jumps in n bytes at a."""
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    return {int(insn.op_str, 16) for insn in md.disasm(pe.read(a, n) or b"", a)
            if insn.mnemonic in ("call", "jmp") and insn.op_str.startswith("0x")}


def pointer_operands(ours: PE, orig: PE, a: int, b: int, n: int) -> list[tuple[int, int]]:
    """(our value, original value) for every displacement or immediate in our n bytes at a that is an
    address in our image, read at the same offset of the original's copy at b. Unlike compare(), this
    also finds the pointers that happen to be equal in both builds."""
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    pairs = []
    for insn in md.disasm(ours.read(a, n), a):
        for offset, size in ((insn.disp_offset, insn.disp_size), (insn.imm_offset, insn.imm_size)):
            if size != 4:
                continue
            k = insn.address - a + offset
            vx = struct.unpack("<I", ours.read(a + k, 4))[0]
            if ours.contains(vx):
                pairs.append((vx, struct.unpack("<I", orig.read(b + k, 4))[0]))
    return pairs


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("target", help="reccmp target id, e.g. SETUP")
    parser.add_argument("--pdb-build", default=str(REPO / "build" / "vc6-pdb"))
    parser.add_argument("--out", help="file name in the target's first source root (default library_msvc.h, "
                                      "with --library library_<name>.h)")
    parser.add_argument("--library", help="annotate this static library's objects (e.g. STRMBASE), not the CRT")
    args = parser.parse_args()
    if args.library:
        prefix = args.library.upper() + ":"
        in_library = lambda obj: obj.upper().startswith(prefix)
        args.out = args.out or f"library_{args.library.lower()}.h"
    else:
        in_library = is_crt
        args.out = args.out or "library_msvc.h"

    logging.basicConfig(level=logging.ERROR)
    target = RecCmpProject.from_directory(Path(args.pdb_build)).get(args.target)
    ours, orig = PE(Path(target.recompiled_path)), PE(Path(target.original_path))
    symbols = parse_map(Path(target.recompiled_path).with_suffix(".map"))
    statics = parse_map(Path(target.recompiled_path).with_suffix(".map"), statics=True)
    _, text_start, text_size, _, _ = ours.section(".text")
    text_end = text_start + text_size

    # Several public names can share an address (__chkstk and __alloca_probe). reccmp keeps the first of
    # them from our PDB, and only that one can match. Read from the PDB, not from reccmp's matched
    # entities: once annotated, an initializer is renamed from the tables (pre_cpp_init becomes
    # $CRT_CPP_Initializer), and a second run would annotate that name.
    reccmp = Compare.from_target(target)
    kept = {}
    for node in reccmp.cvdump_analysis.nodes:
        if node.addr is not None and node.addr not in kept and (node.decorated_name or node.name()):
            kept[node.addr] = node.decorated_name or node.name()
    # A global matches by the name reccmp gives ours, the first PDB name at its address. Where the CRT carries
    # debug info (VS2008's) that is the C name, ___security_cookie's is __security_cookie; VC6's PDB has only
    # the public names, the same as the map's.
    var_names = {}
    for node in reccmp.cvdump_analysis.nodes:
        if node.addr is not None and node.addr not in var_names and node.name():
            var_names[node.addr] = node.name()
    # A string matches by its name, which reccmp makes from our copy's text (escaped, in quotes). An
    # annotation's literal goes through literal_eval and gets the same escaping, so that name is the literal.
    string_names = {e.recomp_addr: e.name for e in reccmp.get_all()
                    if e.recomp_addr is not None and e.get("type") in (EntityType.STRING, EntityType.WIDECHAR)}
    # Static functions only have the name reccmp gives them from our PDB, which knows them only where the
    # CRT objects carry debug info (VS2008's, not VC6's). Those are annotated by that name.
    by_name = {kept[s[0]] for s in statics if kept.get(s[0])}
    code_symbols = sorted(symbols + [(addr, kept[addr], obj) for addr, _, obj in statics if kept.get(addr)])
    # Static data (GS_ContextRecord, startinfo) is only in the PDB: VS2008's map lists static functions alone.
    # A static is visible only in its own object, so one the CRT code below refers to is the CRT's.
    data_symbols = sorted(symbols + [(n.addr, n.name(), STATIC_DATA) for n in reccmp.cvdump_analysis.nodes
                                     if n.addr is not None and n.decorated_name is None and n.name()
                                     and n.node_type == EntityType.DATA and not text_start <= n.addr < text_end])
    # Each object's data contributions, from our PDB. An object's data is laid out the same in both builds, so
    # a pointer anywhere into a contribution places all of it, its statics included, which VC6's CRT has no
    # names for. Credited to the public global before it instead, a pointer past that global's own object gave it
    # another object's address (LS3DF's __ctype got one in sbheap's data, which cost two __old_sbh functions).
    cv = Cvdump(str(target.recompiled_pdb)).modules().section_contributions().run()
    modules = {m.id: m for m in cv.modules}
    contribs = []
    for c in cv.sizerefs:
        m = modules.get(c.module)
        if m is None or not m.obj or not 1 <= c.section <= len(ours.sections):
            continue
        start = ours.sections[c.section - 1][1] + c.offset
        if not text_start <= start < text_end:
            obj = f"{Path(m.lib).stem.upper()}:{Path(m.obj).name}" if m.lib else Path(m.obj).name
            contribs.append((start, start + c.size, obj))
    contribs.sort()
    contrib_starts = [c[0] for c in contribs]

    def contribution(addr: int) -> tuple[int, int, str] | None:
        k = bisect.bisect_right(contrib_starts, addr) - 1
        return contribs[k] if k >= 0 and addr < contribs[k][1] else None

    _, orig_text_start, orig_text_size, _, _ = orig.section(".text")
    orig_text = orig.read(orig_text_start, orig_text_size)

    delta = orig.entry - ours.entry
    found = {}  # original address -> (our address, names)
    failed = []
    ambiguous = []  # (our address, name, the places it fits in the original, size)
    # our symbol -> original address -> votes, kept apart for operands pointing exactly at the symbol
    exact = defaultdict(lambda: defaultdict(int))
    inside = defaultdict(lambda: defaultdict(int))
    placed = defaultdict(lambda: defaultdict(int))  # contribution -> its original address -> votes

    in_orig = {}  # our address of a function found -> its original address

    def fits(a: int, b: int, size: int, limit: int) -> bool:
        # The bytes agree apart from pointers, and a call or jump to a function already found goes to its original
        # address: adjustor thunks differ only in the function they jump to (CBaseInputPin's AddRef and Release)
        pairs = compare(ours, orig, a, b, size, limit)
        return pairs is not None and all(in_orig.get(vx, vy) == vy for vx, vy in pairs)

    def accept(addr: int, orig_addr: int, name: str, size: int) -> None:
        found.setdefault(orig_addr, (addr, []))[1].append(name)
        in_orig[addr] = orig_addr
        for vx, vy in pointer_operands(ours, orig, addr, orig_addr, size):
            sym = max((s for s in data_symbols if s[0] <= vx), default=None)  # the operand may point into it
            if sym:
                (exact if vx == sym[0] else inside)[sym][vy - (vx - sym[0])] += 1
            c = contribution(vx)
            if c and in_library(c[2]):
                placed[c][vy - (vx - c[0])] += 1

    for k, (addr, name, obj) in enumerate(code_symbols):
        # Import thunks (MSVCRT:MSVCR90.dll) are left to the thunk search below, or to reccmp itself in a
        # relocated original: they differ only in their pointer, so any of them would pass for any other here
        if not (text_start <= addr < text_end) or not in_library(obj) or obj.lower().endswith(".dll"):
            continue
        end = min(next((s[0] for s in code_symbols[k + 1:] if s[0] > addr), text_end), text_end)
        code = ours.read(addr, end - addr)
        sizes = candidate_sizes(ours, addr, len(code.rstrip(b"\x00\xcc\x90")) or len(code))  # without padding
        match = next(((size, shift) for size in sizes for shift in sorted(range(-15, 16), key=abs)
                      if fits(addr, addr + delta + shift, size, len(code))), None)
        if match:
            size, shift = match
            delta += shift
        else:
            # The objects before this one differ from the original's (an incomplete source links fewer): look
            # in the whole of the original's .text. A function's first byte is an opcode, never part of a
            # pointer, so it has to agree.
            def search(sizes: list[int], limit: int) -> tuple[list[int], int]:
                for size in sizes:
                    hits = [orig_text_start + p for p in range(orig_text_size - size) if orig_text[p] == code[0]
                            and fits(addr, orig_text_start + p, size, limit)]
                    if hits:
                        return hits, size
                return [], 0

            hits, size = search(sizes, len(code))
            # Several hits: the function is short (strchr up to the entry point inside it) or has twins that
            # differ only in a call (_lseek and _write call their _lk versions). The rest of its object, from
            # the original too, tells them apart.
            obj_end = min(next((s[0] for s in code_symbols[k + 1:] if s[0] > addr and s[2] != obj), text_end),
                          text_end)
            if len(hits) > 1 and obj_end > end:
                whole = ours.read(addr, obj_end - addr)
                hits, size = search(candidate_sizes(ours, addr, len(whole.rstrip(b"\x00\xcc\x90")) or len(whole)),
                                    len(whole))
                size = min(size, len(code))
            if len(hits) > 1:
                ambiguous.append((addr, name, hits, size))
                continue
            if not hits:
                failed.append((addr, name))
                continue
            delta = hits[0] - addr
        accept(addr, addr + delta, name, size)

    # Twins, identical functions in their own objects (_memcpy and _memmove): the functions reccmp matched tell
    # them apart, by which of the places the original calls where ours calls the twin (LS3DF's ReadCharBuffer
    # calls _memmove). Once the others have their place, the one left is a twin's too.
    if ambiguous:
        matched = sorted((e.orig_addr, e.recomp_addr, e.size(ImageId.RECOMP)) for e in reccmp.get_all()
                         if e.get("type") == EntityType.FUNCTION and e.orig_addr is not None
                         and e.recomp_addr is not None and e.size(ImageId.RECOMP))
        calls = []  # (our callees, the original's callees) of each matched function
        for i, (orig_addr, recomp_addr, size) in enumerate(matched):
            end = matched[i + 1][0] if i + 1 < len(matched) else orig_addr + size
            calls.append((call_targets(ours, recomp_addr, size),
                          call_targets(orig, orig_addr, min(size, end - orig_addr))))
        while ambiguous:
            resolved = []
            for twin in ambiguous:
                addr, name, hits, size = twin
                left = [h for h in hits if h not in found]
                votes = {h: sum(addr in ours_calls and h in orig_calls for ours_calls, orig_calls in calls)
                         for h in left}
                best = sorted(left, key=votes.get, reverse=True)
                if len(left) == 1 or (best and votes[best[0]] > votes[best[1]]):
                    accept(addr, best[0], name, size)
                    resolved.append(twin)
            if not resolved:
                break
            ambiguous = [twin for twin in ambiguous if twin not in resolved]
        failed += [(addr, name) for addr, name, _, _ in ambiguous]

    functions =[(a, kept[ours_addr] if kept.get(ours_addr) in names else names[0])
                 for a, (ours_addr, names) in found.items()]

    # Import thunks, jmp [IAT slot]: the original's is the one jumping through its slot for the same import
    no_thunks = orig.relocated or args.library
    our_imports = {} if no_thunks else {slot: key for key, slot in ours.imports().items()}
    orig_imports = {} if no_thunks else orig.imports()
    orig_slots = set(orig_imports.values())

    def in_thunk_run(pos: int) -> bool:
        """Whether the jmp at pos has another jmp [IAT slot] right before or after it."""
        return any(orig_text[p:p + 2] == b"\xff\x25" and struct.unpack_from("<I", orig_text, p + 2)[0] in orig_slots
                   for p in (pos - 6, pos + 6) if 0 <= p <= len(orig_text) - 6)

    thunks, no_thunk = [], []
    for addr, name, obj in [] if no_thunks else symbols:
        code = ours.read(addr, 6)
        if not (text_start <= addr < text_end) or not obj.lower().endswith(".dll") or code[:2] != b"\xff\x25":
            continue
        slot = orig_imports.get(our_imports.get(struct.unpack_from("<I", code, 2)[0]))
        jmp = b"\xff\x25" + struct.pack("<I", slot) if slot else None
        hits = [m.start() for m in re.finditer(re.escape(jmp), orig_text)] if jmp else []
        if not hits:
            no_thunk.append(name)
            continue
        # A CRT function can end in a tail call through the same slot (getSystemCP's jmp [GetOEMCP]). The
        # linker puts the thunks one after another, so take a jump among other thunks, the nearest to ours.
        pos = min(hits, key=lambda p: (not in_thunk_run(p), abs(orig_text_start + p - (addr + delta))))
        thunks.append((orig_text_start + pos, name))

    globals_, strings, floats, conflicts, outvoted, displaced = [], [], [], [], [], []
    rank = {}  # global name -> (pointers exactly at it, votes), to settle two globals given one address
    for sym in set(exact) | set(inside):
        addr, name, obj = sym
        if not (in_library(obj) or obj == STATIC_DATA and not args.library) or text_start <= addr < text_end:
            continue
        c = contribution(addr)
        cand = ({a + addr - c[0]: v for a, v in placed[c].items()} if c in placed
                else exact.get(sym) or inside[sym])
        if len(cand) > 1 and (c in placed or sym in exact):
            # A loop's end pointer, one past an array, points where the next object or variable starts in our
            # build (__NMSG_WRITE's message table before _iob, ___initstdio's _iob before _pctype). The real
            # references outnumber those.
            best = max(cand, key=cand.get)
            if 2 * cand[best] > sum(cand.values()):
                outvoted.append((name, best, dict(cand)))
                cand = {best: cand[best]}
        if len(cand) > 1:
            conflicts.append((name, dict(cand)))
        elif name.startswith("__real@"):
            if not orig.relocated:
                floats.append((next(iter(cand)), int(name[7])))  # __real@8@... is a double
        elif name.startswith("??_C@"):
            if (not orig.relocated or name in EMPTY_STRINGS) and addr in string_names:
                strings.append((next(iter(cand)), string_names[addr]))
        else:
            global_name = var_names.get(addr, name)
            globals_.append((next(iter(cand)), global_name))
            rank[global_name] = (sym in exact, sum(cand.values()))

    # Two globals given one address: a pointer into a global resolves to whatever is at that address in the
    # original (LS3DF's ___lc_collate_cp got __cflush's, which _stbuf, _openfile and _getbuf increment). Pointers
    # exactly at a global outrank pointers into one, then more of them win; the other is left out.
    by_addr = defaultdict(list)
    for addr, name in globals_:
        by_addr[addr].append(name)
    for addr, names in by_addr.items():
        if len(names) > 1:
            keep = max(names, key=lambda n: rank[n])
            displaced += [(n, keep, addr) for n in names if n != keep]
    globals_ = [(addr, name) for addr, name in globals_ if name not in {n for n, _, _ in displaced}]

    out = Path(target.source_paths[0]) / args.out
    csv = out.with_suffix(".csv")
    # A copy the original kept in the target's own code, which our build kept in the library (LS3DF11's folded
    # I3D_part_base::`scalar deleting destructor', which our build kept as CRendererInputPin's), is annotated there
    claimed = set()
    for root in target.source_paths:
        for path in Path(root).rglob("*"):
            if path.suffix in (".h", ".cpp", ".c") and path.resolve() != out.resolve():
                for m in OWN_MARKER.finditer(path.read_text(encoding="latin-1")):
                    if m.group(2) == args.target:
                        claimed.add(int(m.group(3), 16))
    skipped = [(addr, name) for addr, name in functions if addr in claimed]
    functions = [(addr, name) for addr, name in functions if addr not in claimed]
    if args.library:
        text = LIBRARY_HEADER.format(name=Path(target.original_path).name, library=args.library)
    else:
        text = HEADER.format(name=Path(target.original_path).name,
                             floats=f"\n// The float constants are in {csv.name}." if floats else "")
    for addr, name in sorted(functions + thunks):
        symbol = "" if name.startswith("?") or name in by_name else " SYMBOL"
        text += f"\n// LIBRARY: {args.target} {addr:#x}{symbol}\n// {name}\n"
    for addr, name in sorted(globals_):
        # A C++ library's vtables (the base classes'): reccmp reads the class from a comment line, and the base
        # a vtable of multiple inheritance is for from the marker
        vtable = VTABLE_NAME.match(name)
        if vtable:
            base = f" {vtable.group(2)}" if vtable.group(2) else ""
            text += f"\n// VTABLE: {args.target} {addr:#x}{base}\n// class {vtable.group(1)}\n"
        else:
            text += f"\n// GLOBAL: {args.target} {addr:#x}\n// {name}\n"
    for addr, value in sorted(strings):
        text += f"\n// STRING: {args.target} {addr:#x}\n// {value}\n"
    out.write_text(text, encoding="ascii", newline="\r\n")
    if floats:
        rows = "".join(f"{addr:#x}|float|{size}\n" for addr, size in sorted(floats))
        csv.write_text("address|type|size\n" + rows, encoding="ascii", newline="\r\n")

    print(f"{len(functions)} functions, {len(thunks)} import thunks, {len(globals_)} globals, {len(strings)} strings, "
          f"{len(floats)} floats"
          f" -> {out.relative_to(REPO).as_posix()}")
    for addr, name in failed:
        print(f"  not found: {name} (ours 0x{addr:x})")
    for addr, name in skipped:
        print(f"  left to the target's own annotation at 0x{addr:x}: {name}")
    for name in no_thunk:
        print(f"  no thunk in the original: {name}")
    for name, best, cand in outvoted:
        print(f"  {name} at 0x{best:x}, outvoting " + ", ".join(f"0x{a:x} ({v}x)" for a, v in cand.items() if a != best))
    for name, cand in conflicts:
        print(f"  conflicting addresses for {name}: " + ", ".join(f"0x{a:x} ({v}x)" for a, v in cand.items()))
    for name, keep, addr in displaced:
        print(f"  {name} left out: 0x{addr:x} is {keep}'s")


if __name__ == "__main__":
    main()
