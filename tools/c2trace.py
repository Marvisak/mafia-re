"""Trace VC6's register allocator: the live ranges it builds for one function, with their ids.

VC6's code generator (SP5+PP's C2.DLL, 13.00.9044; its allocator is `color.c`) splits each variable into live ranges
per region, numbers them from a per-function counter in order of first appearance, and rebuilds them many times; each
rebuild frees the old ranges and takes the new ones' ids from a free list, last freed first. Where it has a choice it
goes by those ids: a loop entry's reloads come out in ascending id, and so do the loads of a sum's two operands. So two
loads in swapped order, with every other instruction the same, come from a different id history, which source that
compiles to the same instructions can still change (the first appearances, a dead initializer the optimizer only drops
late, a dead local, a member call against a direct call of the same function).

This tool compiles one source file with the vc6 build's own command line for it (as tools/variants.py does, compiler
launchers included) under a small debugger, with breakpoints in C2.DLL:

- `0x1071be68`: the counter's reset at the start of each function; `[[esi] + 0x18]` is the function's decorated name.
- `0x10706400`: a live range created; `esi` is the range, `+0x1c` its id, `[esi]` its symbol, whose name is at
  `[[sym] + 0x18]` and frame offset at `+0x24` (once the frame is laid out).
- `0x107068f2`: a live range freed (`ecx`).
- `0x1070cc09` (with --regs): the colouring is done (`color.c`'s driver at `0x1073122d` calls it before freeing its
  arenas). Every live range left is in the table at `0x107ac5b8` (1024 buckets by id, chained through `+0x2c`), and
  `+0x10` points to its register's descriptor (0x58 bytes each from `0x107c4a40`, numbered 1 to 8: eax, ecx, edx, ebx,
  esp, ebp, esi, edi).
- `0x10709a9c` (with --colouring): `0x10709937` has just given one range its register (`ebp` the range, `edi` the
  register's number, `ebx` its class), the cheapest of the ones it may take: the costs at `0x107ac480` (by register)
  add each uncoloured neighbour's preference weights (and 100 times its weight for a neighbour's last register) and
  subtract the range's own (`+0x38`, move-related ranges' registers with weights), and a tie goes to the first in the
  class's list (`0x107b6888` for the integer registers: eax, ecx, edx, esi, edi, ebx, ebp). The allowed registers are
  the bit vector at `+0x20` (sorted chunks of base, next, 32-bit mask).
- `0x10767401` and `0x1076741d` (with --x87): which floats stay on the FPU stack is decided before, and apart from the
  colouring, which takes only the integer registers. `0x1076697d` scores each candidate (a definition earns 2 times its
  block's frequency, a use nothing, an edge leaving the candidate's region costs the successor block's frequency) and
  calls the filter at `0x10767401`, whose loop sees each candidate at `0x1076741d` (`ebx`, its score at `+0xc`) and
  drops those at 0 or less. The best one left is kept on the stack, the ones interfering with it are split and go
  through the filter again.

What-ifs change the allocator's data while it runs, to see whether a decision is the one that matters before looking
for source that gives it: --set NAME=PRIORITY overwrites the priority of every range of a variable (`id:N` one range)
when the colouring's work list is built (`0x10731402`); --fill LINE takes every integer register out of the blocks that
hold code of that source line when the splitter forms its regions (`0x10743bda`, their used registers at `+0x54`; a
block whose set is still empty can't be filled; C2 counts an instruction's line, at `+0x10`, from the line of its
function's name, which the tool finds in the source by the decorated name); --simple splits every range one at a time
instead of by regions (the splitter takes regions only for 3 ranges or more and a count at `0x107ad5b8` of 100 or
more, set below it). --score ADDRESS then scores the traced function against the original at that address, as
tools/variants.py does (--target names the reccmp target if --target-dir doesn't). They combine with each other and
with --regs, --colouring and --x87:

    .venv/Scripts/python tools/c2trace.py ls3df/I3D/visuals/I3d_singlemesh.cpp m_AddFGroups --target-dir ls3df \\
        --set fg=200 --fill 715 --score 0x10055930

The ranges are coloured in the order of a work list (`0x10732af5`, sorted by `0x10708a6e`): by the priority at `+0xc`
descending, then by `+0x44` descending, a full tie in the order the range table is walked (by id) with the later one
first. `0x10707a0b` computes the priority per basic block: a range referenced in the block gains its references there
times the block's pressure (the ranges live in it) times its frequency (2 to the loop depth), one live there without a
reference loses the pressure times the frequency. `+0x40` is the frequency-weighted references, `+0x24` their count.

It groups the events into passes, runs of creations or frees from one call site: `build` is the first build (the
function's constants and global addresses get theirs first, as `const`), `rebuild` the rebuild of a region's ranges
(`0x10744286`, which goes through the old ranges in ascending id). Other sites print their return address. Unnamed
ranges are compiler temporaries, shown by their frame offset.

    .venv/Scripts/python tools/c2trace.py rw_data/rw_data.cpp                         # the functions, with counts
    .venv/Scripts/python tools/c2trace.py rw_data/rw_data.cpp dtaRead                 # every pass of one function
    .venv/Scripts/python tools/c2trace.py rw_data/rw_data.cpp dtaRead --vars file,compressed_block_size
    .venv/Scripts/python tools/c2trace.py rw_data/rw_data.cpp dtaRead --edited my.cpp  # an edited copy instead

With --vars only the passes that create or free those variables' ranges are shown, and only their ids: the history
that decides their relative order. --regs prints instead the ranges the colouring kept, with the register each got,
their priority and their weights; --colouring the colouring's decisions in the order it makes them, each with the
registers the range may take, their costs and its preferences; --x87 each pass of the FPU stack's filter, with every
candidate's score. Only the global allocator's ranges are there: the code generator gives short-lived temporaries (a
load feeding one push) their registers later, the next scratch register after the last one it used. --edited compiles
another file in place of the source, from a temporary directory with the source's folder on the include path, and the
source's command line. A source several targets compile needs
--target-dir, the target's folder in the build tree (`ls3df_1_1` for patch 1.1's engine). Build the vc6 preset first.
Only the 13.00.9044 C2.DLL is traced; plain SP5's (12.00.8966, LS3DF's DirectShow base classes) has other addresses.
"""

import argparse
import ctypes
import ctypes.wintypes as W
import hashlib
import msvcrt
import os
import re
import shutil
import struct
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import variants  # noqa: E402

REPO = variants.REPO
C2_REFERENCE = REPO / "3rdparty" / "msvc600_sp5_vcpp" / "VC98" / "Bin" / "C2.DLL"
C2_BASE = 0x10700000
BP_RESET, BP_CREATE, BP_FREE, BP_COLOURED, BP_ASSIGNED = 0x1071BE68, 0x10706400, 0x107068F2, 0x1070CC09, 0x10709A9C
BP_X87_FILTER, BP_X87_CANDIDATE = 0x10767401, 0x1076741D
BP_WORKLIST, BP_SPLIT_REGIONS, BP_POP = 0x10731402, 0x10743BDA, 0x107314F7
SPLIT_GATE, ALL_INTEGER_REGISTERS = 0x107AD5B8, 0b111011110          # eax to edi but esp, by register number
TARGETS = {"ls3df": "LS3DF", "ls3df_1_1": "LS3DF11", "ijoy": "IJOY", "rw_data": "RWDATA", "setup": "SETUP",
           "setup_1_0": "SETUP10", "setup_polish": "SETUPPL", "setup_czech": "SETUPCZ", "game": "GAME"}
WEB_TABLE, REG_DESCRIPTORS, REG_DESCRIPTOR_SIZE = 0x107AC5B8, 0x107C4A40, 0x58
REG_COSTS, CLASS_REGISTERS = 0x107AC480, 0x107B690C
REGS = ("-", "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi")
SITES = {0x1072EA39: "const", 0x107305D6: "build", 0x10744A54: "rebuild", 0x10744488: "rebuild"}

# --- a minimal debugger: int3 breakpoints in 32-bit processes, followed into child processes ---------------------

k32 = ctypes.WinDLL("kernel32", use_last_error=True)
DEBUG_PROCESS = 1
CREATE_UNICODE_ENVIRONMENT = 0x400
CREATE_NO_WINDOW = 0x08000000
STARTF_USESTDHANDLES = 0x100
DBG_CONTINUE = 0x00010002
DBG_EXCEPTION_NOT_HANDLED = 0x80010001
EXCEPTION_EV, CREATE_THREAD_EV, CREATE_PROCESS_EV, EXIT_THREAD_EV, EXIT_PROCESS_EV, LOAD_DLL_EV = range(1, 7)
BREAKPOINTS = (0x80000003, 0x4000001F)      # STATUS_BREAKPOINT, STATUS_WX86_BREAKPOINT
SINGLE_STEPS = (0x80000004, 0x4000001E)     # STATUS_SINGLE_STEP, STATUS_WX86_SINGLE_STEP
WOW64_CONTEXT_FULL = 0x00010007


class STARTUPINFO(ctypes.Structure):
    _fields_ = [("cb", W.DWORD), ("lpReserved", W.LPWSTR), ("lpDesktop", W.LPWSTR), ("lpTitle", W.LPWSTR),
                ("dwX", W.DWORD), ("dwY", W.DWORD), ("dwXSize", W.DWORD), ("dwYSize", W.DWORD),
                ("dwXCountChars", W.DWORD), ("dwYCountChars", W.DWORD), ("dwFillAttribute", W.DWORD),
                ("dwFlags", W.DWORD), ("wShowWindow", W.WORD), ("cbReserved2", W.WORD),
                ("lpReserved2", ctypes.c_void_p), ("hStdInput", W.HANDLE), ("hStdOutput", W.HANDLE),
                ("hStdError", W.HANDLE)]


class PROCESS_INFORMATION(ctypes.Structure):
    _fields_ = [("hProcess", W.HANDLE), ("hThread", W.HANDLE), ("dwProcessId", W.DWORD), ("dwThreadId", W.DWORD)]


class EXCEPTION_RECORD(ctypes.Structure):
    _fields_ = [("ExceptionCode", W.DWORD), ("ExceptionFlags", W.DWORD), ("ExceptionRecord", ctypes.c_void_p),
                ("ExceptionAddress", ctypes.c_void_p), ("NumberParameters", W.DWORD),
                ("ExceptionInformation", ctypes.c_ulonglong * 15)]


class EXCEPTION_DEBUG_INFO(ctypes.Structure):
    _fields_ = [("ExceptionRecord", EXCEPTION_RECORD), ("dwFirstChance", W.DWORD)]


class CREATE_THREAD_DEBUG_INFO(ctypes.Structure):
    _fields_ = [("hThread", W.HANDLE), ("lpThreadLocalBase", ctypes.c_void_p), ("lpStartAddress", ctypes.c_void_p)]


class CREATE_PROCESS_DEBUG_INFO(ctypes.Structure):
    _fields_ = [("hFile", W.HANDLE), ("hProcess", W.HANDLE), ("hThread", W.HANDLE), ("lpBaseOfImage", ctypes.c_void_p),
                ("dwDebugInfoFileOffset", W.DWORD), ("nDebugInfoSize", W.DWORD), ("lpThreadLocalBase", ctypes.c_void_p),
                ("lpStartAddress", ctypes.c_void_p), ("lpImageName", ctypes.c_void_p), ("fUnicode", W.WORD)]


class LOAD_DLL_DEBUG_INFO(ctypes.Structure):
    _fields_ = [("hFile", W.HANDLE), ("lpBaseOfDll", ctypes.c_void_p), ("dwDebugInfoFileOffset", W.DWORD),
                ("nDebugInfoSize", W.DWORD), ("lpImageName", ctypes.c_void_p), ("fUnicode", W.WORD)]


class DEBUG_EVENT_INFO(ctypes.Union):
    _fields_ = [("Exception", EXCEPTION_DEBUG_INFO), ("CreateThread", CREATE_THREAD_DEBUG_INFO),
                ("CreateProcess", CREATE_PROCESS_DEBUG_INFO), ("LoadDll", LOAD_DLL_DEBUG_INFO),
                ("pad", ctypes.c_byte * 200)]


class DEBUG_EVENT(ctypes.Structure):
    _fields_ = [("dwDebugEventCode", W.DWORD), ("dwProcessId", W.DWORD), ("dwThreadId", W.DWORD),
                ("u", DEBUG_EVENT_INFO)]


class WOW64_CONTEXT(ctypes.Structure):
    _fields_ = [("ContextFlags", W.DWORD), ("Dr", W.DWORD * 6), ("FloatSave", ctypes.c_byte * 112),
                ("SegGs", W.DWORD), ("SegFs", W.DWORD), ("SegEs", W.DWORD), ("SegDs", W.DWORD),
                ("Edi", W.DWORD), ("Esi", W.DWORD), ("Ebx", W.DWORD), ("Edx", W.DWORD), ("Ecx", W.DWORD),
                ("Eax", W.DWORD), ("Ebp", W.DWORD), ("Eip", W.DWORD), ("SegCs", W.DWORD), ("EFlags", W.DWORD),
                ("Esp", W.DWORD), ("SegSs", W.DWORD), ("ExtendedRegisters", ctypes.c_byte * 512)]


k32.WaitForDebugEvent.argtypes = [ctypes.POINTER(DEBUG_EVENT), W.DWORD]
k32.ContinueDebugEvent.argtypes = [W.DWORD, W.DWORD, W.DWORD]
k32.ReadProcessMemory.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t,
                                  ctypes.POINTER(ctypes.c_size_t)]
k32.WriteProcessMemory.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t,
                                   ctypes.POINTER(ctypes.c_size_t)]
k32.FlushInstructionCache.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.c_size_t]
k32.Wow64GetThreadContext.argtypes = [W.HANDLE, ctypes.POINTER(WOW64_CONTEXT)]
k32.Wow64SetThreadContext.argtypes = [W.HANDLE, ctypes.POINTER(WOW64_CONTEXT)]
k32.CreateProcessW.argtypes = [W.LPCWSTR, W.LPWSTR, ctypes.c_void_p, ctypes.c_void_p, W.BOOL, W.DWORD, ctypes.c_void_p,
                               W.LPCWSTR, ctypes.POINTER(STARTUPINFO), ctypes.POINTER(PROCESS_INFORMATION)]
k32.GetFinalPathNameByHandleW.argtypes = [W.HANDLE, W.LPWSTR, W.DWORD, W.DWORD]
k32.CloseHandle.argtypes = [W.HANDLE]


class Memory:
    def __init__(self, process):
        self.process = process

    def read(self, addr, n):
        buf, got = ctypes.create_string_buffer(n), ctypes.c_size_t()
        if not k32.ReadProcessMemory(self.process, ctypes.c_void_p(addr), buf, n, ctypes.byref(got)):
            return b""
        return buf.raw[:got.value]

    def u32(self, addr):
        b = self.read(addr, 4) if addr else b""
        return struct.unpack("<I", b)[0] if len(b) == 4 else 0

    def i32(self, addr):
        return struct.unpack("<i", struct.pack("<I", self.u32(addr)))[0]

    def cstr(self, addr, n=256):
        b = self.read(addr, n).split(b"\0")[0] if addr else b""
        return b.decode("latin1") if b and all(32 <= c < 127 for c in b) else None

    def write(self, addr, data):
        got = ctypes.c_size_t()
        k32.WriteProcessMemory(self.process, ctypes.c_void_p(addr), data, len(data), ctypes.byref(got))
        k32.FlushInstructionCache(self.process, ctypes.c_void_p(addr), len(data))


def handle_path(h):
    buf = ctypes.create_unicode_buffer(1024)
    k32.GetFinalPathNameByHandleW(h, buf, 1024, 0)
    return buf.value


def debug(cmdline, cwd, env, breakpoints, dll_matches, output):
    """Run cmdline with its child processes under the debugger, without a console, their output going to the open
    file output. breakpoints maps addresses at C2_BASE to callbacks (context, memory); they are set in every process
    that loads a DLL for which dll_matches(path) holds. Returns whether one did."""
    si, pi = STARTUPINFO(), PROCESS_INFORMATION()
    si.cb = ctypes.sizeof(si)
    si.dwFlags = STARTF_USESTDHANDLES
    handle = msvcrt.get_osfhandle(output.fileno())
    os.set_handle_inheritable(handle, True)
    si.hStdOutput = si.hStdError = handle
    block = ctypes.create_unicode_buffer("".join(f"{k}={v}\0" for k, v in env.items()) + "\0")
    flags = DEBUG_PROCESS | CREATE_UNICODE_ENVIRONMENT | CREATE_NO_WINDOW
    if not k32.CreateProcessW(None, ctypes.create_unicode_buffer(cmdline), None, None, True, flags, block, cwd,
                              ctypes.byref(si), ctypes.byref(pi)):
        raise OSError(ctypes.get_last_error(), "CreateProcess failed: " + cmdline)
    memories, threads, armed, pending = {}, {pi.dwThreadId: pi.hThread}, {}, {}
    traced = False
    ev = DEBUG_EVENT()
    while True:
        k32.WaitForDebugEvent(ctypes.byref(ev), 0xFFFFFFFF)
        code, pid, tid = ev.dwDebugEventCode, ev.dwProcessId, ev.dwThreadId
        status = DBG_CONTINUE
        if code == CREATE_PROCESS_EV:
            info = ev.u.CreateProcess
            memories[pid] = Memory(info.hProcess)
            threads[tid] = info.hThread
            if info.hFile:
                k32.CloseHandle(info.hFile)
        elif code == CREATE_THREAD_EV:
            threads[tid] = ev.u.CreateThread.hThread
        elif code == LOAD_DLL_EV:
            info = ev.u.LoadDll
            path = handle_path(info.hFile) if info.hFile else ""
            if info.hFile:
                k32.CloseHandle(info.hFile)
            if path and dll_matches(path):
                traced = True
                mem = memories[pid]
                for addr, callback in breakpoints.items():
                    a = addr - C2_BASE + info.lpBaseOfDll
                    armed[(pid, a)] = (mem.read(a, 1), callback)
                    mem.write(a, b"\xcc")
        elif code == EXCEPTION_EV:
            rec = ev.u.Exception.ExceptionRecord
            addr = rec.ExceptionAddress or 0
            if rec.ExceptionCode in BREAKPOINTS:
                if (pid, addr) in armed:
                    original, callback = armed[(pid, addr)]
                    ctx = WOW64_CONTEXT()
                    ctx.ContextFlags = WOW64_CONTEXT_FULL
                    k32.Wow64GetThreadContext(threads[tid], ctypes.byref(ctx))
                    ctx.Eip = addr
                    callback(ctx, memories[pid])
                    memories[pid].write(addr, original)
                    ctx.EFlags |= 0x100
                    k32.Wow64SetThreadContext(threads[tid], ctypes.byref(ctx))
                    pending[tid] = (pid, addr)
            elif rec.ExceptionCode in SINGLE_STEPS and tid in pending:
                p, a = pending.pop(tid)
                memories[p].write(a, b"\xcc")
            else:
                status = DBG_EXCEPTION_NOT_HANDLED
        elif code == EXIT_PROCESS_EV and pid == pi.dwProcessId:
            k32.ContinueDebugEvent(pid, tid, DBG_CONTINUE)
            break
        k32.ContinueDebugEvent(pid, tid, status)
    k32.CloseHandle(pi.hProcess)
    k32.CloseHandle(pi.hThread)
    return traced


# --- the trace ----------------------------------------------------------------------------------------------------

class Trace:
    def __init__(self):
        # [name, [events], [ranges], [decisions], [filter passes]]: an event is (kind, id, symbol name or label,
        # site), a range left after the colouring (id, symbol, register, priority, weight, references, +0x44), a
        # decision (id, symbol, priority, weight, register, class, allowed registers, costs, preferences), a pass of the
        # FPU stack's filter a list of candidates (id, symbol, score)
        self.functions = []

    def symbol(self, mem, sym):
        name = mem.cstr(mem.u32(mem.u32(sym) + 0x18)) if sym else None
        if name:
            return name[1:] if name.startswith("_") else name
        kind = mem.u32(sym + 4) & 0xff
        if kind == 0xd:
            return "const"
        off = mem.i32(sym + 0x24)
        return f"temp{off:+d}" if off else "temp"

    def on_reset(self, ctx, mem):
        self.functions.append([mem.cstr(mem.u32(mem.u32(ctx.Esi) + 0x18)) or "?", [], [], [], []])

    def on_create(self, ctx, mem):
        if self.functions:
            web = ctx.Esi
            site = mem.u32(ctx.Esp + 8)      # after the function's push esi; push edi
            self.functions[-1][1].append(("new", mem.u32(web + 0x1c), self.symbol(mem, mem.u32(web)), site))

    def on_free(self, ctx, mem):
        if self.functions:
            web = ctx.Ecx
            site = mem.u32(ctx.Esp)
            self.functions[-1][1].append(("free", mem.u32(web + 0x1c), self.symbol(mem, mem.u32(web)), site))

    def on_coloured(self, ctx, mem):
        if not self.functions:
            return
        base = ctx.Eip - (BP_COLOURED - C2_BASE)
        table = mem.read(base + WEB_TABLE - C2_BASE, 4096)
        descriptors = base + REG_DESCRIPTORS - C2_BASE
        ranges = self.functions[-1][2]
        ranges.clear()
        for bucket in struct.unpack("<1024I", table):
            web = bucket
            while web:
                fields = struct.unpack("<18i", mem.read(web, 0x48))
                n, rest = divmod((fields[4] & 0xffffffff) - descriptors, REG_DESCRIPTOR_SIZE)
                reg = REGS[n] if fields[4] and not rest and 0 < n < len(REGS) else "-"
                ranges.append((fields[7], self.symbol(mem, fields[0] & 0xffffffff), reg, fields[3], fields[16],
                               fields[9], fields[17]))
                web = fields[11] & 0xffffffff
        ranges.sort()

    @staticmethod
    def bits(mem, field):
        vector = mem.u32(field)
        out, chunk = [], mem.u32(vector) if vector else 0
        while chunk:
            first, mask = mem.u32(chunk), mem.u32(chunk + 8)
            out += [first + k for k in range(32) if mask >> k & 1]
            chunk = mem.u32(chunk + 4)
        return out

    def on_assigned(self, ctx, mem):
        if not self.functions:
            return
        base = ctx.Eip - (BP_ASSIGNED - C2_BASE)
        web, reg, cls = ctx.Ebp, ctx.Edi, ctx.Ebx
        regs = []
        listed = mem.u32(base + CLASS_REGISTERS - C2_BASE + 4 * cls)
        while r := mem.u32(listed + 4 * len(regs)):
            regs.append(r)
        allowed = set(self.bits(mem, web + 0x20))
        costs = {r: mem.i32(base + REG_COSTS - C2_BASE + 4 * r) for r in regs}
        prefs, p = [], mem.u32(web + 0x38)
        while p:
            prefs.append((mem.u32(mem.u32(p + 4) + 0x1c), mem.i32(p + 8)))
            p = mem.u32(p)
        self.functions[-1][3].append((mem.u32(web + 0x1c), self.symbol(mem, mem.u32(web)), mem.i32(web + 0xc),
                                      mem.i32(web + 0x40), reg, cls, [r for r in regs if r in allowed], costs, prefs))

    def on_x87_filter(self, ctx, mem):
        if self.functions:
            self.functions[-1][4].append([])

    def on_x87_candidate(self, ctx, mem):
        if self.functions and self.functions[-1][4]:
            web = ctx.Ebx
            self.functions[-1][4][-1].append((mem.u32(web + 0x1c), self.symbol(mem, mem.u32(web)), mem.i32(web + 0xc)))


class WhatIf:
    """Changes the allocator's data for one function: priorities, full blocks, the split's kind."""

    def __init__(self, trace, function, priorities, lines, simple, text):
        self.trace, self.function = trace, function
        self.priorities, self.source_lines, self.simple = priorities, lines, simple
        self.text, self.lines, self.start = text, None, 0
        self.done, self.gate = [], None

    def active(self):
        return self.trace.functions and self.function in self.trace.functions[-1][0]

    def relative_lines(self):
        """C2 numbers an instruction's line from the line of its function's name, so --fill's source lines are
        counted from the function's definition, found in the source by its decorated name."""
        body = self.trace.functions[-1][0][1:].split("@@")[0]     # name@Class... or ?0Class for a constructor
        special, body = (body[:2], body[2:]) if body.startswith("?") else (None, body)
        parts = body.split("@")
        if special in ("?0", "?1"):
            parts = [("~" if special == "?1" else "") + parts[0]] + parts
        elif special:
            self.done.append(f"--fill can't place {self.trace.functions[-1][0]} in the source")
            return set()
        name = "::".join(reversed(parts))
        pattern = re.compile(r"(^|[\s*&])" + re.escape(name) + r"\s*\(")
        for number, line in enumerate(self.text.splitlines(), 1):
            if pattern.search(line) and not line.rstrip().endswith(";"):
                self.done.append(f"{name} defined at line {number}")
                self.start = number
                return {n - number for n in self.source_lines}
        self.done.append(f"{name}'s definition not found in the source")
        return set()

    def on_worklist(self, ctx, mem):
        if not self.active():
            return
        base = ctx.Eip - (BP_WORKLIST - C2_BASE)
        for bucket in struct.unpack("<1024I", mem.read(base + WEB_TABLE - C2_BASE, 4096)):
            web = bucket
            while web:
                wid, sym = mem.u32(web + 0x1c), self.trace.symbol(mem, mem.u32(web))
                value = self.priorities.get(f"id:{wid}", self.priorities.get(sym))
                if value is not None:
                    self.done.append(f"{wid}:{sym} priority {mem.i32(web + 0xc)} -> {value}")
                    mem.write(web + 0xc, struct.pack("<i", value))
                web = mem.u32(web + 0x2c)

    @staticmethod
    def block_lines(mem, block):
        lines, insn, end = set(), mem.u32(block + 0x1c), mem.u32(block + 0x20)
        for _ in range(4000):
            if not insn or insn == end:
                break
            if mem.read(insn + 9, 1)[0] & 1 and mem.u32(insn + 0x10) & 0xffff:
                lines.add(mem.u32(insn + 0x10) & 0xffff)
            insn = mem.u32(insn)
        return lines

    def on_split_regions(self, ctx, mem):
        if not self.active():
            return
        if self.simple:
            gate = ctx.Eip - (BP_SPLIT_REGIONS - C2_BASE) + SPLIT_GATE - C2_BASE
            if self.gate is None:
                self.gate = (gate, mem.read(gate, 4))
            mem.write(gate, struct.pack("<i", 0))
            self.done.append("a split made one range at a time")
        if not self.source_lines:
            return
        if self.lines is None:
            self.lines = self.relative_lines()
        block = mem.u32(mem.u32(ctx.Ecx + 8))
        while block:
            lines = self.block_lines(mem, block)
            if lines & self.lines:
                shown = ",".join(str(n + self.start) for n in sorted(lines))
                chunk = mem.u32(mem.u32(block + 0x54)) if mem.u32(block + 0x54) else 0
                if chunk and mem.u32(chunk) == 0:
                    mem.write(chunk + 8, struct.pack("<I", mem.u32(chunk + 8) | ALL_INTEGER_REGISTERS))
                    self.done.append(f"block of lines {shown} filled")
                else:
                    self.done.append(f"block of lines {shown} left (no register used yet)")
            block = mem.u32(block)

    def on_pop(self, ctx, mem):
        if self.gate:
            mem.write(*self.gate)
            self.gate = None


def score(obj: Path, target: str, address: str, decorated: str) -> str:
    """The traced object's function against the original, as tools/variants.py scores a variant."""
    import difflib
    import logging
    from funcdiff import find_match
    from reccmp.compare import Compare
    from reccmp.project.detect import RecCmpProject
    from reccmp.types import ImageId
    logging.basicConfig(level=logging.ERROR)
    compare = Compare.from_target(RecCmpProject.from_directory(variants.PDB_BUILD).get(target))
    match = find_match(compare, address)
    size = match.size(ImageId.ORIG) or match.max_size(ImageId.ORIG)
    orig, _ = variants.disasm(compare.orig_bin.read(match.orig_addr, size), match.orig_addr)
    symbol = match.get("symbol") or decorated            # a static function has no public symbol in the PDB
    code, calls = variants.read_function(obj, symbol)
    if code is None:
        return f"{symbol} is not in the object"
    ours, _ = variants.disasm(code, 0, calls)
    for insns in (orig, ours):
        while insns and insns[-1][0] in ("nop", "int3"):
            insns.pop()
    a = variants.normalize(orig, compare.orig_bin.is_valid_vaddr, "")
    b = variants.normalize(ours, lambda x: False, "")
    ratio = difflib.SequenceMatcher(None, a, b, autojunk=False).ratio()
    return f"{ratio * 100:.2f}% against {match.best_name()} (0x{match.orig_addr:x}), {len(b)} instructions"


def command(source: Path, edited: Path | None, work: Path, target_dir: str) -> str:
    cmd = variants.compile_command(source, Path(target_dir))
    cmd = re.sub(r"/Fo\S+", lambda _: f"/Fo{work / 'trace.obj'}", cmd)
    cmd = re.sub(r"/Fd\S+", lambda _: f"/Fd{work}\\", cmd)
    if edited:
        copy = work / source.name
        copy.write_text(edited.read_text())
        cmd = re.sub(re.escape(str(source)) + r"\s*$", "", cmd, flags=re.I) + f' -I"{source.parent}" "{copy}"'
    return cmd


def passes(events):
    out = []
    for kind, wid, name, site in events:
        label = SITES.get(site, f"{site:08x}")
        if not out or out[-1][0] != (kind, label):
            out.append([(kind, label), []])
        out[-1][1].append((wid, name))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("source", help="the source file, as the vc6 build compiles it")
    ap.add_argument("function", nargs="?", help="(part of) the function's decorated name; omitted: list them")
    ap.add_argument("--vars", help="comma-separated variable names: show only their ranges' history")
    ap.add_argument("--regs", action="store_true", help="the ranges left after the colouring, with their registers")
    ap.add_argument("--colouring", action="store_true", help="the colouring's decisions, in the order it makes them")
    ap.add_argument("--x87", action="store_true", help="the FPU stack's candidates, with their scores, pass by pass")
    ap.add_argument("--set", action="append", default=[], metavar="NAME=PRIORITY",
                    help="what-if: a variable's ranges' priority in the colouring (id:N one range; not the x87 "
                    "candidates); repeatable")
    ap.add_argument("--fill", action="append", type=int, default=[], metavar="LINE",
                    help="what-if: no free register in the blocks of this source line when the splitter runs")
    ap.add_argument("--simple", action="store_true", help="what-if: split ranges one at a time, never by regions")
    ap.add_argument("--score", metavar="ADDRESS", help="score the traced function against the original's at ADDRESS")
    ap.add_argument("--target", help="the reccmp target for --score (default: from --target-dir)")
    ap.add_argument("--edited", help="compile this file in place of the source")
    ap.add_argument("--target-dir", default="", help="for a source several targets compile, the target's folder in "
                    "the build tree whose command line to use (ls3df_1_1, setup_czech)")
    args = ap.parse_args()

    source = (REPO / args.source).resolve()
    work = Path(tempfile.mkdtemp(prefix="c2trace"))
    cmd = command(source, Path(args.edited).resolve() if args.edited else None, work, args.target_dir)
    reference = hashlib.sha256(C2_REFERENCE.read_bytes()).digest()

    def is_c2(path):
        p = Path(path.removeprefix("\\\\?\\"))
        return p.name.lower() == "c2.dll" and hashlib.sha256(p.read_bytes()).digest() == reference

    whatif_wanted = args.set or args.fill or args.simple
    if (whatif_wanted or args.score) and not args.function:
        sys.exit("what-ifs and --score need the function")
    if args.score and not (args.target or TARGETS.get(args.target_dir)):
        sys.exit("--score needs --target (or a --target-dir it knows)")
    trace = Trace()
    breakpoints = {BP_RESET: trace.on_reset, BP_CREATE: trace.on_create, BP_FREE: trace.on_free}
    if whatif_wanted:
        try:
            priorities = {k: int(v) for k, v in (a.split("=", 1) for a in args.set)}
        except ValueError:
            sys.exit("--set takes NAME=PRIORITY")
        whatif = WhatIf(trace, args.function, priorities, args.fill, args.simple,
                        Path(args.edited or source).read_text(errors="replace"))
        breakpoints[BP_WORKLIST] = whatif.on_worklist
        breakpoints[BP_SPLIT_REGIONS] = whatif.on_split_regions
        breakpoints[BP_POP] = whatif.on_pop
    if args.regs:
        breakpoints[BP_COLOURED] = trace.on_coloured
    if args.colouring:
        breakpoints[BP_ASSIGNED] = trace.on_assigned
    if args.x87:
        breakpoints[BP_X87_FILTER] = trace.on_x87_filter
        breakpoints[BP_X87_CANDIDATE] = trace.on_x87_candidate
    with open(work / "cl.txt", "w+") as output:
        traced = debug(cmd, str(variants.BUILD), variants.preset_env("vc6"), breakpoints, is_c2, output)
        output.seek(0)
        log = output.read()
    compiled = (work / "trace.obj").exists()
    chosen = max((f for f in trace.functions if args.function and args.function in f[0]), key=lambda f: len(f[1]),
                 default=None)
    scored = score(work / "trace.obj", args.target or TARGETS[args.target_dir], args.score, chosen[0]) \
        if compiled and traced and args.score and chosen else None
    shutil.rmtree(work, ignore_errors=True)
    if not compiled:
        sys.exit(f"the compile failed:\n{cmd}\n{log}")
    if not traced:
        sys.exit(f"{args.source} isn't compiled with the 13.00.9044 C2.DLL")
    if whatif_wanted:
        counts = {}
        for change in whatif.done:
            counts[change] = counts.get(change, 0) + 1
        print("what-if: " + ("; ".join(c + (f" (x{n})" if n > 1 else "") for c, n in counts.items()) or
                             "nothing matched"))
    if scored:
        print(scored)
    if (whatif_wanted or scored) and not (args.regs or args.colouring or args.x87 or args.vars):
        return

    if not args.function:
        for name, events, *_ in trace.functions:
            if events:
                print(f"{sum(1 for e in events if e[0] == 'new'):5d} ranges  {name}")
        return
    found = [f for f in trace.functions if args.function in f[0]]
    if not found:
        sys.exit(f"no function matching {args.function!r}; run without one for the list")
    name, events, ranges, decisions, filters = max(found, key=lambda f: len(f[1]))
    want = {v.strip() for v in args.vars.split(",")} if args.vars else None
    if args.regs:
        print(f"{name}: {len(ranges)} ranges after the colouring")
        print("  id  variable                  reg   priority weight   refs  +0x44")
        for wid, sym, reg, prio, weight, refs, f44 in ranges:
            if not want or sym in want:
                print(f"{wid:4d}  {sym:24s}  {reg:5s} {prio:6d} {weight:6d} {refs:6d} {f44:6d}")
        return
    if args.colouring:
        print(f"{name}: {len(decisions)} decisions, in order (class 0 is the integer registers)")
        for wid, sym, prio, weight, reg, cls, allowed, costs, prefs in decisions:
            if want and sym not in want:
                continue

            def rn(r):
                return REGS[r] if cls == 0 and r < len(REGS) else str(r)

            line = (f"{wid:4d}  {sym:24s} priority {prio:5d} weight {weight:4d}  -> {rn(reg):4s}"
                    + (f" (class {cls})" if cls else "") + f"  may take {','.join(rn(r) for r in allowed)}")
            if any(costs.values()):
                line += "  costs " + " ".join(f"{rn(r)}={c}" for r, c in costs.items() if c)
            if prefs:
                line += "  prefers " + " ".join(f"{rn(r)}:{w}" for r, w in prefs)
            print(line)
        return
    if args.x87:
        print(f"{name}: {len(filters)} passes of the FPU stack's filter (a score above 0 stays a candidate)")
        for i, candidates in enumerate(filters):
            shown = [c for c in candidates if not want or c[1] in want]
            if shown:
                print(f"{i:4d}  " + "  ".join(f"{wid}:{sym} {score}" for wid, sym, score in shown))
        return
    news = [e for e in events if e[0] == "new"]
    print(f"{name}: {len(news)} ranges created, {len(events) - len(news)} freed, ids up to "
          f"{max((e[1] for e in news), default=0)}")
    for i, ((kind, label), items) in enumerate(passes(events)):
        if want:
            items = [(w, n) for w, n in items if n in want]
            if not items:
                continue
        print(f"{i:4d} {kind:4s} {label:8s} " + " ".join(f"{w}:{n}" for w, n in items))


if __name__ == "__main__":
    main()
