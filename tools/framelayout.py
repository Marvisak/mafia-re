"""Predict a function's VC6 /O2 stack-frame layout from its /FAs listing and compare it with the actual one.

usage: framelayout.py <listing.asm> <function> [--infl name=+n ...] [--force-alone name ...]
                      [--hidden auto|tails|none] [--debug]

The model: each spilled local's weight is its number of memory references in the
code before tail merging; variables are processed heaviest first (ties: earliest last reference first) and each
takes the free slot with the smallest creator weight >= its own (ties: the most recently created), else a new slot;
freeness is precise liveness on the final code. Frames over 128 bytes are sorted: the slots (key = sum of the
members' weights; byte cells key = references * 4) ordered by (key desc, creator weight asc, creation) go through
VC6's quicksort; arrays sit above everything.

Tail merging hides references, and the tool counts two kinds itself (--hidden auto): an exit tail entered by jumps
from n source lines that are an `if` whose body ends in a `return` (read from the source file the listing names, so
a jump that only skips code doesn't count) had n copies, and so had a block of plain reloads entered from n lines.
Branch ends merged after a `jmp` and the copies' temporaries aren't counted: `--infl` adds the pre-merge extras you
know of (e.g. `--infl _current_block$=+8`), and `--force-alone` keeps a local in a slot of its own. `--debug` lists
every block with several entries and their source lines. The tool prints the actual slot table from the listing, the
predicted one, and the differences, which point at the variable whose count or lifetime is off.
"""
import re, sys, argparse
from collections import defaultdict

SRC = [None, None]
RMW = ("inc", "dec", "add", "sub", "and", "or", "xor", "shl", "shr", "sar", "neg", "not", "adc", "sbb")

def parse(path, func):
    text = open(path).read()
    fm0 = re.search(r"^; File (.+)$", text, re.M)
    SRC[0] = fm0.group(1).strip() if fm0 else None; SRC[1] = path
    m = re.search(r"\n" + re.escape(func) + r"\s+PROC NEAR(.*?)\n" + re.escape(func) + r"\s+ENDP", text, re.S)
    if not m: raise SystemExit(f"{func} not found in {path}")
    start = text.find(func + "\tPROC NEAR")
    if start < 0: start = text.find(func + " PROC NEAR")
    pre = text[:start]
    syms = dict(re.findall(r"^(\S+) = (-?\d+)", pre[max(pre.rfind("ENDP"), 0):], re.M))
    body = m.group(1)
    fm = re.search(r"mov\s+eax, (\d+)\s*;.*\n\s*call\s+__chkstk", body) or re.search(r"\n\s*sub\s+esp, (\d+)", body)
    frame = int(fm.group(1)) if fm else 0
    ins, labels, pending = [], {}, []
    line = None
    for raw in body.split("\n"):
        s = raw.strip()
        if s.startswith("; Line "): line = int(s[7:].split()[0]); continue
        mm = re.match(r"; (\d+)\s+: ", s)
        if mm: line = int(mm.group(1)); continue
        if not s or s.startswith(";"): continue
        if s.endswith(":"): pending.append(s[:-1]); continue
        mn, _, ops = s.partition("\t"); mn = mn.strip(); ops = ops.strip()
        if mn == "npad": continue
        uses, defs, lea, byte, arr, wide = set(), set(), set(), set(), set(), set()
        for m2 in re.finditer(r"(?:([A-Za-z_$?][\w$?@]*)|(-?\d+)\+)\[esp([^\]]*)\]", ops):
            sym = m2.group(1) if m2.group(1) else "tmp" + m2.group(2)
            if sym not in syms:
                if m2.group(1): continue
                syms[sym] = m2.group(2)
            if int(syms[sym]) >= 0: continue          # a parameter, not a local
            if re.search(r"\+e[a-z]{2}\+", "+" + m2.group(3)): arr.add(sym)
            memdst = ops.split(",")[0].strip().endswith("]")
            if "BYTE PTR " + (m2.group(1) or "") not in ops: wide.add(sym)
            elif m2.group(1): byte.add(sym)
            if mn == "lea": lea.add(sym); uses.add(sym); defs.add(sym)
            elif mn.startswith("f") and not mn.startswith(("fst", "fist", "fnst", "fbstp")): uses.add(sym)   # fld/fadd/fmul/fcomp m32 read
            elif mn.startswith(("fst", "fist", "fbstp")): defs.add(sym)
            elif mn in ("cmp", "test", "push") or not memdst: uses.add(sym)
            elif mn in RMW: uses.add(sym); defs.add(sym)
            else: defs.add(sym)
        tgt = ops.replace("SHORT", "").strip() if mn.startswith("j") else None
        d = dict(idx=len(ins), mn=mn, uses=uses, defs=defs, lea=lea, byte=byte, wide=wide, arr=arr, tgt=tgt, line=line, labels=list(pending), ops=ops)
        for l in pending: labels[l] = len(ins)
        pending = []
        ins.append(d)
    for d in ins:
        if d["mn"] == "ret": d["succ"] = []
        elif d["mn"] == "jmp": d["succ"] = [labels[d["tgt"]]] if d["tgt"] in labels else []   # an EH funclet's jmp to ___CxxFrameHandler leaves the function
        elif d["tgt"]: d["succ"] = [d["idx"] + 1] + ([labels[d["tgt"]]] if d["tgt"] in labels else [])
        else: d["succ"] = [d["idx"] + 1] if d["idx"] + 1 < len(ins) else []
    return ins, syms, frame

def sizes(syms, frame):
    """object sizes: the distance to the next local above (the frame's symbols are sorted by offset)"""
    offs = sorted((int(v), k) for k, v in syms.items() if int(v) < 0)
    out = {}
    for i, (o, k) in enumerate(offs):
        nxt = next((o2 for o2, _ in offs[i + 1:] if o2 != o), 0)     # symbols sharing an offset share the size
        out[k] = nxt - o
    return out

def analyse(ins):
    n = len(ins)
    live_in = [set() for _ in range(n)]; live_out = [set() for _ in range(n)]
    changed = True
    while changed:
        changed = False
        for d in reversed(ins):
            out = set()
            for s in d["succ"]: out |= live_in[s]
            inn = d["uses"] | (out - d["defs"])
            if out != live_out[d["idx"]] or inn != live_in[d["idx"]]:
                live_out[d["idx"]], live_in[d["idx"]] = out, inn; changed = True
    refs, last, addr, bytes_, arrays, wides = defaultdict(int), {}, set(), set(), set(), set()
    occ = defaultdict(set)
    for d in ins:
        for v in d["lea"]: addr.add(v)
        for v in d["lea"]: refs[v] += 1; last[v] = d["idx"]
        for v in d["uses"] - d["lea"]: refs[v] += 1; last[v] = d["idx"]
        for v in d["defs"] - d["lea"]: refs[v] += 1; last[v] = d["idx"]
        bytes_ |= d["byte"]; wides |= d["wide"]; arrays |= d["arr"]
        for v in live_in[d["idx"]]: occ[v].add(d["idx"])
        for v in d["defs"]:
            if v in live_out[d["idx"]]: occ[v].add(d["idx"] + 0.5)
    for v in list(addr):                           # address-taken and never accessed directly: an array (or struct)
        if all(v not in (d["uses"] | d["defs"]) - d["lea"] for d in ins): arrays.add(v)
    bytes_ -= arrays | wides                       # a byte local is only ever accessed as a byte
    return dict(refs), last, addr, bytes_, arrays, occ

_src_cache = {}
def failure_site(line):
    """True when source line `line` is an `if` whose body ends with a return (so a jump from it into a shared exit is
    a merged copy of that body, not a skip over it)."""
    if not SRC[0] or line is None: return False
    if "lines" not in _src_cache:
        _src_cache["lines"] = []
        import os, glob
        cands = [SRC[0], os.path.join(os.path.dirname(SRC[1] or ""), SRC[0])] + glob.glob(os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "*", os.path.basename(SRC[0])))
        for c in cands:
            try: _src_cache["lines"] = open(c, encoding="utf-8", errors="replace").read().splitlines(); break
            except OSError: continue
    L = _src_cache["lines"]
    if line < 1 or line > len(L): return False
    text = L[line - 1]
    if not re.search(r"\bif\s*\(", text): return False
    # the body: braces starting on this line or the next, else the single statement that follows
    i = line - 1
    rest = text[text.index("if"):]
    depth = rest.count("(") - rest.count(")")
    while depth > 0 and i + 1 < len(L):              # condition continues on following lines
        i += 1; depth += L[i].count("(") - L[i].count(")")
    after = L[i][L[i].rfind(")") + 1:].strip() if i == line - 1 else L[i].strip()
    j = i
    if not after:
        j = i + 1
        while j < len(L) and not L[j].strip(): j += 1
        after = L[j].strip() if j < len(L) else ""
    if after.startswith("{"):
        d = 0; last = ""
        for k in range(j, len(L)):
            t = L[k]
            for ch in t:
                if ch == "{": d += 1
                elif ch == "}":
                    d -= 1
                    if d == 0: return last.startswith("return")
            st = t.strip()
            if st and st not in ("{", "}") and not st.startswith("//"): last = st
        return False
    return after.startswith("return")

def hidden_refs(ins, reload_mode="edges", debug=False):
    """References tail merging removed: an exit tail (a run from a label to `ret`) entered by jumps from k distinct
    source lines (+1 for a fallthrough entry) had k (+1) copies; a block of plain loads that falls through into a
    labelled block, entered by k edges, had k copies (reload_mode "edges"), or one ("none")."""
    extra = {}
    n = len(ins)
    preds = [[] for _ in range(n)]
    for d in ins:
        for t in d["succ"]: preds[t].append(d["idx"])
    label_at = {d["idx"] for d in ins if d["labels"]}
    def run_from(i, stop_at_label=True):
        """straight-line run from i: through fallthroughs, stopping after ret/jmp or before a later label if asked"""
        out = []
        while True:
            d = ins[i]; out.append(d)
            if d["mn"] == "ret" or d["tgt"] is not None: return out       # a branch ends the straight run
            if i + 1 >= n or (stop_at_label and i + 1 in label_at): return out
            i += 1
    for i in sorted(label_at):
        jump_preds = [p for p in preds[i] if ins[p]["tgt"] is not None]
        fall = [p for p in preds[i] if ins[p]["tgt"] is None]
        if len(preds[i]) < 2: continue
        run = run_from(i, stop_at_label=False)
        if debug:
            print(f"  join {ins[i]['labels']} line {ins[i]['line']}: jumps from lines {sorted(ins[p]['line'] for p in jump_preds)}"
                  f"{' + fallthrough from line %s' % ins[fall[0]]['line'] if fall else ''}; run ends with {run[-1]['mn']} ({len(run)} instructions)")
        if run[-1]["mn"] == "ret":
            # exit tail: count the straight run up to ret, but references of blocks after a later label that is
            # itself a merge target belong to both groups (nested merges); count them per entering group
            lines = {ins[p]["line"] for p in jump_preds if failure_site(ins[p]["line"])}
            more = len(lines) - (0 if fall else 1)
            if debug: print(f"     exit tail: failure sites {sorted(lines)} -> {more} hidden copies")
            if more > 0:
                for d in run:
                    for v in d["uses"] | d["defs"]:
                        extra[v] = extra.get(v, 0) + more * (2 if (v in d["uses"] and v in d["defs"] and v not in d["lea"]) else 1)
            continue
        blk = run_from(i, stop_at_label=True)
        if (reload_mode == "edges" and all(d["mn"] == "mov" and not d["ops"].split(",")[0].strip().endswith("]") and (d["uses"] - d["defs"]) for d in blk)
                and blk[-1]["mn"] != "jmp" and blk[-1]["idx"] + 1 in label_at):
            lines = {ins[p]["line"] for p in jump_preds}
            more = len(lines) - (0 if fall else 1)
            if debug: print(f"     reload block: {more} hidden copies")
            for d in blk:
                for v in d["uses"]:
                    extra[v] = extra.get(v, 0) + max(more, 0)
    return extra

def c2sort(a, key):
    a = list(a)
    def qs(lo, hi):
        if hi - lo + 1 < 2: return
        p = (lo + hi + 1) // 2
        a[p], a[lo] = a[lo], a[p]; pv = key(a[lo]); store = lo
        for i in range(lo + 1, hi + 1):
            if key(a[i]) < pv:
                store += 1; a[i], a[store] = a[store], a[i]
        a[store], a[lo] = a[lo], a[store]
        qs(lo, store - 2); qs(store + 1, hi)
    qs(0, len(a) - 1)
    return a

def predict(refs, last, addr, bytes_, arrays, occ, infl, force_alone):
    w = {v: refs[v] + infl.get(v, 0) for v in refs}
    dwords = [v for v in refs if v not in bytes_ and v not in arrays]
    order = sorted(dwords, key=lambda v: (-w[v], last[v]))
    slots = []
    for v in order:
        best = None
        if v not in addr and v not in force_alone:
            for s in slots:
                if s["w"] < w[v] or any(m in addr or m in force_alone or (occ[v] & occ[m]) for m in s["members"]): continue
                if best is None or s["w"] < best["w"] or (s["w"] == best["w"] and s["created"] > best["created"]): best = s
        if best is None: slots.append(dict(creator=v, members=[v], w=w[v], created=len(slots), sum=w[v], byte=False))
        else: best["members"].append(v); best["sum"] += w[v]
    cells = []                                       # byte locals share cells by the same rules; key = references * 4
    for v in sorted(bytes_, key=lambda v: (-w[v], last[v])):
        best = None
        for s in cells:
            if s["w"] < w[v] * 4 or any(occ[v] & occ[m] for m in s["members"]): continue
            if best is None or s["w"] < best["w"] or (s["w"] == best["w"] and s["created"] > best["created"]): best = s
        if best is None: cells.append(dict(creator=v, members=[v], w=w[v] * 4, created=len(slots) + len(cells), sum=w[v] * 4, byte=True))
        else: best["members"].append(v); best["sum"] += w[v] * 4
    lst = sorted(cells + slots, key=lambda s: (-s["sum"], s["w"], s["created"]))
    return c2sort(lst, key=lambda s: -s["sum"]), w

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("listing"); ap.add_argument("function")
    ap.add_argument("--infl", nargs="*", default=[], help="pre-merge extras, name=+n (symbol as in the listing, e.g. _size$=+1)")
    ap.add_argument("--force-alone", nargs="*", default=[], help="symbols that are known to take a slot of their own")
    ap.add_argument("--debug", action="store_true", help="print every multi-entry block with its source lines")
    ap.add_argument("--hidden", default="auto", choices=["auto", "tails", "none"], help="count references hidden by tail merging: merged exit tails and reload blocks (auto), exit tails only, or nothing")
    a = ap.parse_args()
    ins, syms, frame = parse(a.listing, a.function)
    refs, last, addr, bytes_, arrays, occ = analyse(ins)
    size = sizes(syms, frame)
    for v in list(refs):                                            # structs and arrays: address-taken or indexed
        if v in arrays or (v in addr and size.get(v, 4) > 4): arrays.add(v); bytes_.discard(v)
        else: size[v] = 1 if v in bytes_ else 4
    infl = {}
    if a.hidden != "none":
        infl.update(hidden_refs(ins, "edges" if a.hidden == "auto" else "none", a.debug))
    for x in a.infl:
        k, v = x.split("="); infl[k] = infl.get(k, 0) + int(v)
    if infl: print("hidden references counted:", {k: v for k, v in sorted(infl.items())})
    # actual layout
    byoff = defaultdict(list)
    for v in refs:
        off = frame + int(syms[v])
        byoff[off].append(v)
    print(f"frame {frame} bytes; address-taken {sorted(addr)}; byte locals {sorted(bytes_)}; arrays {sorted(arrays)}")
    print("actual (bottom-up):")
    for off in sorted(byoff):
        print(f"  {off:6d}  " + ", ".join(f"{v}({refs[v]})" for v in byoff[off]))
    srt, w = predict(refs, last, addr, bytes_, arrays, occ, infl, set(a.force_alone))
    print("predicted (bottom-up, sorted only if the frame is over 128 bytes):")
    big = frame > 128
    seq = srt if big else sorted(srt, key=lambda s: s["created"])
    actual_groups = set(tuple(sorted(vs)) for vs in byoff.values())
    for s in seq:
        g = tuple(sorted(s["members"]))
        flag = "" if g in actual_groups else "   <-- not a group in the actual layout"
        print(f"  key {s['sum']:3d}  " + ", ".join(f"{v}({w[v]})" for v in s["members"]) + flag)
    bigslots = []                                      # structs and arrays share by the same rules, then sit above the scalars
    for v in sorted([v for v in refs if v in arrays], key=lambda v: (-w[v], last[v])):
        best = None
        for b in bigslots:
            if b["w"] < w[v] or b["size"] < size.get(v, 4) or any(occ[v] & occ[m] for m in b["members"]): continue
            if best is None or b["w"] < best["w"] or (b["w"] == best["w"] and b["created"] > best["created"]): best = b
        if best is None: bigslots.append(dict(members=[v], w=w[v], sum=w[v], size=size.get(v, 4), created=len(bigslots)))
        else: best["members"].append(v); best["sum"] += w[v]
    bigslots.sort(key=lambda b: (-(b["sum"] / b["size"]), b["created"]))
    big = [b["members"] for b in bigslots]
    for b in bigslots:
        print(f"  key {b['sum'] * 4 / b['size']:5.2f}  " + ", ".join(f"{v}({w[v]})" for v in b["members"]) + f" [{b['size']} bytes, above the scalars]")
    pred_groups = set(tuple(sorted(s["members"])) for s in seq) | set(tuple(sorted(m)) for m in big)
    missing = [g for g in actual_groups if g not in pred_groups]
    if missing: print("actual groups the model does not produce:", missing)
    if big:
        act_order = [tuple(sorted(byoff[o])) for o in sorted(byoff)]
        pred_order = [tuple(sorted(s["members"])) for s in seq] + [tuple(sorted(m)) for m in big]
        same = sum(1 for x, y in zip(act_order, pred_order) if x == y)
        print(f"order: {same}/{len(act_order)} positions agree")

if __name__ == "__main__":
    main()
