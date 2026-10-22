"""Compile variants of a source file with the original toolchain and score one function against the original.

Matching often comes down to choosing between spellings that VC6 compiles differently: an `if` written the other
way round, a temporary assigned instead of constructed, a header's inline function with or without a nested call.
Inlining makes that slow to explore by hand, because a change in one place moves the inline budget of the whole
function. This tool compiles every variant of a spec file in parallel, each with the exact command line the `vc6`
build uses for that source file (target flags, include paths, IJoy's compiler launcher), and prints one score per
variant: the similarity of our instructions to the original's, optionally with registers and stack offsets masked.

A spec is a Python file that defines

    TARGET = "LS3DF"                       # reccmp target id
    SOURCE = "ls3df/I3D/I3d_math.cpp"      # the source file, relative to the repository
    FUNCTION = "0x1002eef0"                # original address, or (part of) the name, as funcdiff takes it
    VARIANTS = [                           # (name, edits): each edit replaces the first occurrence of a text
        ("ge", [("< MRG_ZERO) {", ">= MRG_ZERO) {")]),                           # ... in SOURCE
        ("pleq", [("ls3df/I3D/I3d_math.h", "    S_vector operator-(", "...")]),  # ... or in another file
    ]

A variant can carry a third element, a dict of options: "cl" names another CL.EXE to compile it with (relative to
the repository, e.g. "build/vc6/ijoy/cl/CL.EXE", which loads the C1XX.DLL and c2.dll next to it), "flags" adds
switches after the build's own (the last /O, /G or /Ob wins). They test a front end or a switch the way an edit tests
a spelling. Optionally the spec also defines ORIG_RANGE = (start, end), to compare only that part of the original, and OURS_RETS = (a, b), to
take our instructions from after the a-th `ret` through the b-th (a = 0: from the start). The two together isolate
one exit path of a function. SYMBOL = "?name@@..." names our function in the object where the PDB has no public
symbol for it (a static function). The spec runs as Python, with REPO (the repository path) defined, so it can build
its variants in loops. The unchanged source is always scored as "base".

Edited files are compiled from a temporary directory: SOURCE itself, and other files through an include directory
placed before the project's, so an edited header must be reached through the include path (or SOURCE's folder),
not through a path relative to another header. `__FILE__` changes accordingly.

Our side comes from the object file: the function's COMDAT section, its relocations (relocated operands compare as
ADDR, the original's addresses likewise) and the names its calls go to. Build the vc6 preset (for the command
line) and vc6-pdb (reccmp finds the original function through it) first.

    .venv/Scripts/python tools/variants.py spec.py                  # scores, best first
    .venv/Scripts/python tools/variants.py spec.py --mask rs        # registers and [esp + N] offsets masked
    .venv/Scripts/python tools/variants.py spec.py --calls          # each variant's out-of-line calls
    .venv/Scripts/python tools/variants.py spec.py --show NAME      # unified diff of one variant
    .venv/Scripts/python tools/variants.py spec.py --pdb-build build/agent1   # another PDB tree (see below)

reccmp pairs our function with the original through the PDB build's line numbers, so once the tree's source has
moved lines since `build/vc6-pdb` was built, the base can land on another function ("no matched function", or wild
scores). Rebuild that tree, or pass a private one built from the current source with --pdb-build.
"""

import argparse
import difflib
import json
import logging
import os
import re
import struct
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import capstone
from reccmp.compare import Compare
from reccmp.project.detect import RecCmpProject
from reccmp.types import ImageId

REPO = Path(__file__).resolve().parent.parent
BUILD = REPO / "build" / "vc6"
PDB_BUILD = REPO / "build" / "vc6-pdb"

IMAGE_REL_I386_DIR32 = 0x6
IMAGE_REL_I386_REL32 = 0x14
SENTINEL = 0x7E000000           # written into relocated fields of our code before disassembly
ADDR_RE = re.compile(r"0x7e[0-9a-f]{6}")
MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

# operator codes of MSVC decorated names, for short call names
OPERATORS = {"0": "{ctor}", "1": "{dtor}", "4": "operator=", "D": "operator*", "G": "operator-", "H": "operator+",
             "K": "operator/", "R": "operator()", "A": "operator[]", "X": "operator*=", "Y": "operator+=",
             "Z": "operator-=", "_0": "operator/=", "8": "operator==", "9": "operator!="}


def short_name(sym: str) -> str:
    """?Magnitude@S_vector@@QBEMXZ -> S_vector::Magnitude, ??0S_vector@@... -> S_vector::S_vector"""
    m = re.match(r"\?\?(_?[0-9A-Z])(\w+)@", sym)
    if m:
        op = OPERATORS.get(m.group(1), "operator?" + m.group(1))
        cls = m.group(2)
        return f"{cls}::{cls if op == '{ctor}' else '~' + cls if op == '{dtor}' else op}"
    m = re.match(r"\?(\w+)@(\w+)@@", sym)
    if m:
        return f"{m.group(2)}::{m.group(1)}"
    m = re.match(r"\?(\w+)@@", sym)
    if m:
        return m.group(1)
    return re.sub(r"@\d+$", "", sym)


# --- the compile command ---------------------------------------------------------------------------------------

def preset_env(name: str) -> dict:
    presets = {p["name"]: p for p in json.loads((REPO / "CMakePresets.json").read_text())["configurePresets"]}
    env, p = {}, presets[name]
    chain = []
    while p:
        chain.append(p)
        p = presets.get(p.get("inherits"))
    for p in reversed(chain):
        env.update(p.get("environment", {}))
    out = dict(os.environ)
    for k, v in env.items():
        v = v.replace("${sourceDir}", str(REPO))
        out[k] = re.sub(r"\$penv\{(\w+)\}", lambda m: os.environ.get(m.group(1), ""), v)
    return out


def compile_command(source: Path) -> str:
    """The vc6 build's command line for one source file, from build.ninja through ninja -t commands."""
    ninja_file = (BUILD / "build.ninja").read_text()
    want = str(source).lower()
    for m in re.finditer(r"^build (\S+\.obj): CXX_COMPILER\S* (\S+)", ninja_file, re.M):
        if m.group(2).replace("$:", ":").lower() == want:
            obj = m.group(1).replace("$:", ":")
            break
    else:
        sys.exit(f"{source} isn't compiled by {BUILD} (configure the vc6 preset)")
    ninja = REPO / ".venv" / "Scripts" / "ninja.exe"
    r = subprocess.run([str(ninja), "-C", str(BUILD), "-t", "commands", "-s", obj], capture_output=True, text=True)
    if r.returncode:
        sys.exit(r.stdout + r.stderr)
    return r.stdout.strip().splitlines()[-1]


def include_dirs(cmd: str) -> list[Path]:
    return [Path(d) for d in re.findall(r"(?:^|\s)[-/]I\s*(\S+)", cmd)]


# --- our side: the function in the object file -----------------------------------------------------------------

def read_function(obj: Path, symbol: str):
    """Code of one function from a COFF object (its own COMDAT section, /O2 implies /Gy), with the relocated
    fields patched to SENTINEL + addend, and the names of the REL32 targets by offset."""
    data = obj.read_bytes()
    _, _, _, symtab, nsyms, opt, _ = struct.unpack_from("<HHIIIHH", data, 0)
    strtab = symtab + nsyms * 18

    names, found, i = {}, None, 0
    while i < nsyms:
        raw_name, value, secnum, _, _, naux = struct.unpack_from("<8sIhHBB", data, symtab + i * 18)
        if raw_name[:4] == b"\0\0\0\0":
            off = strtab + struct.unpack_from("<I", raw_name, 4)[0]
            name = data[off:data.index(b"\0", off)].decode("latin-1")
        else:
            name = raw_name.rstrip(b"\0").decode("latin-1")
        names[i] = name
        if name == symbol and secnum > 0:
            found = (secnum, value)
        i += 1 + naux
    if not found:
        return None, {}
    secnum, value = found
    _, _, _, size, raw, relocs, _, nrel, _, _ = struct.unpack_from("<8sIIIIIIHHI", data, 20 + opt + (secnum - 1) * 40)
    code = bytearray(data[raw:raw + size])
    targets = {}
    for k in range(nrel):
        off, si, typ = struct.unpack_from("<IIH", data, relocs + k * 10)
        if typ == IMAGE_REL_I386_DIR32:
            addend = struct.unpack_from("<I", code, off)[0]
            struct.pack_into("<I", code, off, (SENTINEL + addend) & 0xFFFFFFFF)
        if typ in (IMAGE_REL_I386_DIR32, IMAGE_REL_I386_REL32):
            targets[off - value] = names.get(si, f"sym{si}")
    return bytes(code[value:]), targets


# --- comparison ------------------------------------------------------------------------------------------------

def normalize(insns, is_addr, mask: str) -> list[str]:
    out = []
    for mnem, ops in insns:
        ops = ADDR_RE.sub("ADDR", ops)
        ops = re.sub(r"0x[0-9a-f]{6,8}", lambda m: "ADDR" if is_addr(int(m.group(0), 16)) else m.group(0), ops)
        if mnem == "call" or mnem.startswith("j"):
            ops = "X"
        if "s" in mask:
            ops = re.sub(r"\[esp \+ (?:0x[0-9a-f]+|\d+)\]|\[esp\]", "[esp+S]", ops)
        if "r" in mask:
            ops = re.sub(r"(?<![a-z])e(?:ax|bx|cx|dx|si|di|bp)(?![a-z])", "REG", ops)
        out.append(f"{mnem} {ops}".strip())
    return out


def cut_rets(insns, rets):
    if not rets:
        return insns
    pos = [i for i, (mn, _) in enumerate(insns) if mn == "ret"]
    a, b = rets
    return insns[(pos[a - 1] + 1 if a else 0):pos[b - 1] + 1]


def disasm(code: bytes, start: int, calls=None):
    insns, names = [], []
    for i in MD.disasm(code, start):
        insns.append((i.mnemonic, i.op_str))
        if i.mnemonic == "call" and calls is not None:
            names.append(next((short_name(n) for off, n in calls.items() if i.address - start < off < i.address
                               - start + i.size), i.op_str))
    return insns, names


def apply_edits(source: Path, edits):
    """The edited text of SOURCE and of every other file an edit touches (repository-relative path -> text)."""
    texts = {}
    for edit in edits:
        path, old, new = (source, *edit) if len(edit) == 2 else (REPO / edit[0], edit[1], edit[2])
        text = texts.get(path) or path.read_text()
        if old not in text:
            return None, f"edit not found in {path.name}: {old[:70]!r}"
        texts[path] = text.replace(old, new, 1)
    return texts, None


def build_variant(cmd: str, env: dict, source: Path, texts: dict, symbol: str, options: dict):
    work = Path(tempfile.mkdtemp(prefix="variant"))
    inc = work / "inc"
    src = work / source.name
    src.write_text(texts.get(source) or source.read_text())
    dirs = [source.parent] + include_dirs(cmd)
    for path, text in texts.items():
        if path == source:
            continue
        placed = False
        for d in dirs:
            try:
                rel = path.relative_to(d)
            except ValueError:
                continue
            (inc / rel).parent.mkdir(parents=True, exist_ok=True)
            (inc / rel).write_text(text)
            placed = True
        if not placed:
            return None, None, f"{path} isn't under an include directory"
    obj = work / "v.obj"
    c = re.sub(r"/Fo\S+", lambda _: f"/Fo{obj}", cmd)
    c = re.sub(r"/Fd\S+", lambda _: f"/Fd{work}\\", c)
    c = re.sub(re.escape(str(source)) + r"\s*$", "", c, flags=re.I)
    first_i = re.search(r"\s[-/]I", c)
    at = first_i.start() if first_i else len(c)
    c = c[:at] + f' -I"{inc}"' + c[at:] + f' -I"{source.parent}" {options.get("flags", "")} "{src}"'
    if "cl" in options:
        c = re.sub(r"^\S*CL\.EXE", lambda _: str(REPO / options["cl"]), c, flags=re.I)
    r = subprocess.run(c, env=env, cwd=BUILD, capture_output=True, text=True)
    if r.returncode or not obj.exists():
        return None, None, (r.stdout + r.stderr)[-1500:]
    code, calls = read_function(obj, symbol)
    if code is None:
        return None, None, f"{symbol} isn't in the object (inlined everywhere, or renamed?)"
    return code, calls, None


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("spec", help="variants spec file (see the module docstring)")
    parser.add_argument("--mask", default="", help="r: mask registers, s: mask [esp + N] offsets")
    parser.add_argument("--calls", action="store_true", help="list each variant's calls (and the original's)")
    parser.add_argument("--show", metavar="NAME", help="print the diff of one variant against the original")
    parser.add_argument("--context", type=int, default=2, help="context lines for --show (default 2)")
    parser.add_argument("--jobs", type=int, default=os.cpu_count(), help="parallel compiles")
    parser.add_argument("--pdb-build", metavar="DIR", default=str(PDB_BUILD),
                        help="the PDB build tree reccmp finds the original function through (default build/vc6-pdb); "
                        "a private tree built from the edited source once its line numbers have drifted from vc6-pdb's")
    args = parser.parse_args()

    spec = {"REPO": REPO}
    exec(Path(args.spec).read_text(), spec)
    source = (REPO / spec["SOURCE"]).resolve()

    logging.basicConfig(level=logging.ERROR)
    compare = Compare.from_target(RecCmpProject.from_directory(Path(args.pdb_build)).get(spec["TARGET"]))
    sys.path.insert(0, str(REPO / "tools"))
    from funcdiff import find_match
    match = find_match(compare, spec["FUNCTION"])
    symbol = spec.get("SYMBOL") or match.get("symbol")
    start, end = spec.get("ORIG_RANGE") or (match.orig_addr, None)
    if end is None:
        size = match.size(ImageId.ORIG) or match.max_size(ImageId.ORIG)
        end = match.orig_addr + size
    orig_code = compare.orig_bin.read(start, end - start)
    orig, orig_calls = disasm(orig_code, start)
    while orig and orig[-1][0] in ("nop", "int3"):
        orig.pop()
    is_addr = compare.orig_bin.is_valid_vaddr
    orig_norm = normalize(orig, is_addr, args.mask)

    cmd = compile_command(source)
    env = preset_env("vc6")
    variants = [("base", [])] + list(spec["VARIANTS"])
    if args.show:
        variants = [v for v in variants if v[0] == args.show]
        if not variants:
            sys.exit(f"no variant {args.show!r}")

    def run(variant):
        name, edits, options = (*variant, {})[:3]
        texts, err = apply_edits(source, edits)
        if err:
            return name, None, None, err
        code, calls, err = build_variant(cmd, env, source, texts, symbol, options)
        if err:
            return name, None, None, err
        ours, names = disasm(code, 0, calls)
        while ours and ours[-1][0] in ("nop", "int3"):    # the section's padding
            ours.pop()
        return name, cut_rets(ours, spec.get("OURS_RETS")), names, None

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(run, variants))

    print(f"{match.best_name()} (0x{match.orig_addr:x}), {len(orig_norm)} instructions in the original"
          + (f", masked: {args.mask}" if args.mask else ""))
    if args.calls:
        # reccmp's sanitizer names the original's call targets (funcdiff shows them the same way)
        excerpt = compare.function_comparator.orig_sanitize.parse_asm(orig_code, start)
        named = [text.split(None, 1)[1] for _, text in excerpt if text.startswith("call ")]
        print("orig: " + " ".join(re.sub(r"\s*\((FUNCTION|LIBRARY|THUNK|STUB)\)|\(.*\)", "", n) for n in named))
    scored = []
    for name, ours, names, err in results:
        if err:
            print(f"{name}: {err}")
            continue
        ours_norm = normalize(ours, lambda a: False, args.mask)
        ratio = difflib.SequenceMatcher(None, orig_norm, ours_norm, autojunk=False).ratio()
        scored.append((ratio, name, len(ours_norm)))
        if args.calls:
            print(f"{name}: " + " ".join(names))
        if args.show:
            for line in difflib.unified_diff(orig_norm, ours_norm, "orig", name, lineterm="", n=args.context):
                print(line)
    for ratio, name, n in sorted(scored, key=lambda s: -s[0]):
        print(f"{ratio * 100:6.2f}%  {name}  ({n} instructions)")


if __name__ == "__main__":
    main()
