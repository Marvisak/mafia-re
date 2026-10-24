"""Name the original's stack slots: for each of our function's variables, the offset it has in the original's frame.

funcdiff maps the original's [esp + N] offsets to ours, but our side is only numbers too. VC6's /FAcs listing names
every frame reference (`_pitch$76313[esp+24]`, `$T1234[esp+8]`, `-12+[esp+20]` for an unnamed temporary) and gives
each instruction's offset in its function. This tool compiles the source with the vc6 build's own command line and
/FAcs, takes funcdiff's aligned instruction pairs (and, in a run funcdiff left unaligned, the instructions that differ
only in their numbers: it masks esp offsets, not ebp's), finds each of our instructions in the listing by its offset,
and pairs the listing's named references with the original's operands. It prints each variable's offset in our frame
(the listing's `_name$ = -12`) and the offset(s) its references have in the original's, frame-relative as in the
listing: each side's esp offsets are taken relative to its own pushes and frame, from a walk of its instructions
(another frame size, or registers saved in another place, then don't shift the offsets).

    .venv/Scripts/python tools/slotnames.py ls3df/IGraph/IShow.cpp DoRenderSample --target LS3DF
    .venv/Scripts/python tools/slotnames.py rw_data/rw_data.cpp dtaRead

A variable with one original offset sits elsewhere or in place (same); one with several shares slots the original
gives to different variables, or the alignment paired it badly at a few references (the counts show which). The
function's decorated name is found by comparing the listing's code with ours (--symbol names it instead). Build the
vc6 and vc6-pdb presets first: funcdiff reads the PDB, the listing takes the vc6 build's command line.
"""

import argparse
import logging
import re
import shutil
import subprocess
import sys
import tempfile
from collections import Counter, defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import funcdiff  # noqa: E402
import variants  # noqa: E402
from reccmp.compare import Compare  # noqa: E402
from reccmp.project.detect import RecCmpProject  # noqa: E402
from reccmp.types import ImageId  # noqa: E402

REPO = variants.REPO
TARGET_DIRS = {"LS3DF": "ls3df", "LS3DF11": "ls3df_1_1", "IJOY": "ijoy", "RWDATA": "rw_data", "SETUP": "setup",
               "SETUP10": "setup_1_0", "SETUPPL": "setup_polish", "SETUPCZ": "setup_czech", "GAME": "game"}

OFFSET_LINE = re.compile(r"^  ([0-9a-f]{5})\t((?:[0-9a-f]{2} ?)+)\t*(.*)$")
MORE_BYTES = re.compile(r"^\t((?:[0-9a-f]{2} ?)+)\t+(.*)$")
LISTING_REF = re.compile(r"([^\s,\[]*)\[(esp|ebp)([^\]]*)\]")
OUR_REF = re.compile(r"\[(esp|ebp)([^\]]*)\]")


def listing(source: Path, target_dir: str) -> str:
    cmd = variants.compile_command(source, Path(target_dir))
    work = Path(tempfile.mkdtemp(prefix="slotnames"))
    try:
        c = re.sub(r"/Fo\S+", lambda _: f"/Fo{work / 'x.obj'}", cmd)
        c = re.sub(r"/Fd\S+", lambda _: f"/Fd{work}\\", c)
        c = re.sub(r" [-/]c ", lambda _: f" /FAcs /Fa{work / 'x.cod'} -c ", c, count=1)
        r = subprocess.run(c, env=variants.preset_env("vc6"), cwd=variants.BUILD, capture_output=True, text=True,
                           shell=True)
        if not (work / "x.cod").exists():
            sys.exit(f"the compile failed:\n{c}\n{r.stdout}{r.stderr}")
        return (work / "x.cod").read_text(errors="replace")
    finally:
        shutil.rmtree(work, ignore_errors=True)


def procedures(text: str) -> dict:
    """name -> (symbols {name: value}, instructions {offset: text}, code bytes)"""
    out = {}
    for m in re.finditer(r"^(\S+)\s+PROC NEAR.*?$(.*?)^\1\s+ENDP", text, re.M | re.S):
        name, body = m.group(1), m.group(2)
        head = text[:m.start()]
        head = head[max(head.rfind("ENDP"), head.rfind("SEGMENT")):]
        symbols = {k: int(v) for k, v in re.findall(r"^(\S+) = (-?\d+)$", head, re.M)}
        insns, code, current = {}, bytearray(), None
        for line in body.splitlines():
            if mo := OFFSET_LINE.match(line):
                current = int(mo.group(1), 16)
                code += bytes.fromhex(mo.group(2))
                insns[current] = mo.group(3).strip()
            elif (mc := MORE_BYTES.match(line)) and current is not None:
                code += bytes.fromhex(mc.group(1))
                insns[current] = (insns[current] + " " + mc.group(2)).strip()
        out[name] = (symbols, insns, bytes(code))
    return out


def displacement(inner: str) -> int:
    """The constant part of what follows the base register in a bracket: '+24', ' + 0x18', ' + eax*4 - 8'."""
    total = 0
    for sign, num in re.findall(r"([+-])\s*(0x[0-9a-f]+|\d+)(?![\w*])", inner):
        total += int(num, 0) if sign == "+" else -int(num, 0)
    return total


def frame_size(insns_text: list[str]) -> int:
    """The prologue's sub esp, N, or the N a large frame moves into eax for __chkstk."""
    for k, text in enumerate(insns_text[:8]):
        if m := re.match(r"sub\s+esp,\s*(0x[0-9a-f]+|\d+)", text):
            return int(m.group(1), 0)
        if (m := re.match(r"mov\s+eax,\s*(0x[0-9a-f]+|\d+)$", text)) and k + 1 < len(insns_text) \
                and re.search(r"chkstk|alloca_probe", insns_text[k + 1]):
            return int(m.group(1), 0)
    return 0


def esp_depths(insns_text: list[str]) -> list[int]:
    """The bytes below the return address before each instruction, in layout order: pushes, pops and esp
    adjustments. A call is taken to pop nothing, and an epilogue (the pops and adds before a ret) is undone after
    it, as the code that follows still has the frame. So the depths are off where a callee pops its arguments or
    a jump lands, by the same amount in two functions of the same shape, which is all a comparison of the two
    needs."""
    depths, depth, epilogue = [], 0, 0
    for text in insns_text:
        depths.append(depth)
        op = text.split()[0] if text else ""
        step = 0
        if op == "push":
            step = 4
        elif op == "pop":
            step = -4
        elif m := re.match(r"(sub|add)\s+esp,\s*(0x[0-9a-f]+|\d+)$", text):
            step = int(m.group(2), 0) * (1 if m.group(1) == "sub" else -1)
        elif re.search(r"chkstk|alloca_probe", text) and len(depths) > 1 \
                and (m := re.match(r"mov\s+eax,\s*(0x[0-9a-f]+|\d+)$", insns_text[len(depths) - 2])):
            step = int(m.group(1), 0)
        depth += step
        if op.startswith("ret"):
            depth -= epilogue
            epilogue = 0
        elif step < 0:
            epilogue += step
        else:
            epilogue = 0
    return depths


def pairs(res):
    """funcdiff's aligned pairs, and the instructions of a replaced run paired in order where they differ only in
    their numbers (funcdiff masks esp offsets, not ebp's, so an ebp frame's moved slots come out unaligned)."""
    def shape(text):
        return re.sub(r"\s[+-]\s(?:0x[0-9a-f]+|\d+)|0x[0-9a-f]+|\d+", "#", text)

    out, gone, new = [], [], []

    def flush():
        if len(gone) == len(new) and all(shape(res.orig[i].text) == shape(res.ours[j].text) for i, j in zip(gone, new)):
            out.extend(zip(gone, new))
        gone.clear()
        new.clear()

    for kind, i, j in res.rows:
        if kind == "-":
            if new:
                flush()
            gone.append(i)
        elif kind == "+":
            new.append(j)
        else:
            flush()
            out.append((i, j))
    flush()
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("source", help="the function's source file, relative to the repository")
    parser.add_argument("function", help="original address or (part of) the function name, as funcdiff takes it")
    parser.add_argument("--target", default="RWDATA", help="reccmp target id (default RWDATA)")
    parser.add_argument("--build", default=str(REPO / "build" / "vc6-pdb"), help="build tree with reccmp-build.yml")
    parser.add_argument("--target-dir", help="the source's target folder in the build tree (from --target if omitted)")
    parser.add_argument("--symbol", help="our function's decorated name in the listing")
    args = parser.parse_args()

    logging.basicConfig(level=logging.ERROR)
    compare = Compare.from_target(RecCmpProject.from_directory(Path(args.build)).get(args.target))
    match = funcdiff.find_match(compare, args.function)
    res = funcdiff.run_match(compare, match)
    ours_code = compare.recomp_bin.read(match.recomp_addr, match.size(ImageId.RECOMP))

    procs = procedures(listing((REPO / args.source).resolve(), args.target_dir or TARGET_DIRS.get(args.target, "")))
    if args.symbol:
        if args.symbol not in procs:
            sys.exit(f"{args.symbol} isn't in the listing")
        symbol = args.symbol
    else:
        same_size = [n for n, (_, _, code) in procs.items() if len(code) == len(ours_code)]
        if not same_size:
            sys.exit("no function of the listing has our function's size; name it with --symbol")
        # relocated fields are zero in the listing, so count the bytes that agree
        symbol = max(same_size, key=lambda n: sum(a == b for a, b in zip(procs[n][2], ours_code)))
    symbols, insns, _ = procs[symbol]

    ebp_frame = any(re.match(r"mov\s+ebp,\s*esp", t) for t in list(insns.values())[:4])
    f_ours = frame_size([i.text for i in res.ours])
    f_orig = frame_size([i.text for i in res.orig])
    depth_ours = esp_depths([i.text for i in res.ours])
    depth_orig = esp_depths([i.text for i in res.orig])
    found = defaultdict(Counter)
    for i, j in pairs(res):
        if res.ours[j].addr is None:
            continue
        text = insns.get(res.ours[j].addr - res.recomp_addr)
        if text is None:
            continue
        named = LISTING_REF.findall(text)
        ours_refs = OUR_REF.findall(res.ours[j].text)
        orig_refs = OUR_REF.findall(res.orig[i].text)
        if not named or not (len(named) == len(ours_refs) == len(orig_refs)):
            continue
        for (prefix, base, _), (base_u, inner_u), (base_o, inner_o) in zip(named, ours_refs, orig_refs):
            if base != base_u or base_u != base_o:
                continue
            if prefix in symbols:
                name, value = prefix, symbols[prefix]
            elif re.fullmatch(r"-?\d+\+", prefix):
                value = int(prefix[:-1])
                name = f"({value})"
            else:
                continue                # a reference the listing gives no name (an outgoing argument)
            delta = displacement(inner_o) - displacement(inner_u)
            if base == "esp":
                delta -= depth_orig[i] - depth_ours[j]
            found[(value, name)][value + delta] += 1

    print(f"{res.name}: {symbol}")
    print(f"frame {f_orig:#x} in the original, {f_ours:#x} ours" + (", ebp-based" if ebp_frame else ""))
    print("   ours  original  variable")
    for (value, name), offsets in sorted(found.items()):
        shown = ", ".join(("=" if off == value else f"{off}") + (f" x{n}" if len(offsets) > 1 else "")
                          for off, n in offsets.most_common())
        print(f"{value:7d}  {shown:8s}  {name}")


if __name__ == "__main__":
    main()
