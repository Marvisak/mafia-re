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

It groups the events into passes, runs of creations or frees from one call site: `build` is the first build (the
function's constants and global addresses get theirs first, as `const`), `rebuild` the rebuild of a region's ranges
(`0x10744286`, which goes through the old ranges in ascending id). Other sites print their return address. Unnamed
ranges are compiler temporaries, shown by their frame offset.

    .venv/Scripts/python tools/c2trace.py rw_data/rw_data.cpp                         # the functions, with counts
    .venv/Scripts/python tools/c2trace.py rw_data/rw_data.cpp dtaRead                 # every pass of one function
    .venv/Scripts/python tools/c2trace.py rw_data/rw_data.cpp dtaRead --vars file,compressed_block_size
    .venv/Scripts/python tools/c2trace.py rw_data/rw_data.cpp dtaRead --edited my.cpp  # an edited copy instead

With --vars only the passes that create or free those variables' ranges are shown, and only their ids: the history
that decides their relative order. --edited compiles another file in place of the source, from a temporary directory
with the source's folder on the include path, and the source's command line. Build the vc6 preset first.
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
BP_RESET, BP_CREATE, BP_FREE = 0x1071BE68, 0x10706400, 0x107068F2
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
        self.functions = []      # [name, [events]]; an event is (kind, id, symbol name or label, site)

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
        self.functions.append([mem.cstr(mem.u32(mem.u32(ctx.Esi) + 0x18)) or "?", []])

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


def command(source: Path, edited: Path | None, work: Path) -> str:
    cmd = variants.compile_command(source)
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
    ap.add_argument("--edited", help="compile this file in place of the source")
    args = ap.parse_args()

    source = (REPO / args.source).resolve()
    work = Path(tempfile.mkdtemp(prefix="c2trace"))
    cmd = command(source, Path(args.edited).resolve() if args.edited else None, work)
    reference = hashlib.sha256(C2_REFERENCE.read_bytes()).digest()

    def is_c2(path):
        p = Path(path.removeprefix("\\\\?\\"))
        return p.name.lower() == "c2.dll" and hashlib.sha256(p.read_bytes()).digest() == reference

    trace = Trace()
    with open(work / "cl.txt", "w+") as output:
        traced = debug(cmd, str(variants.BUILD), variants.preset_env("vc6"),
                       {BP_RESET: trace.on_reset, BP_CREATE: trace.on_create, BP_FREE: trace.on_free}, is_c2, output)
        output.seek(0)
        log = output.read()
    compiled = (work / "trace.obj").exists()
    shutil.rmtree(work, ignore_errors=True)
    if not compiled:
        sys.exit(f"the compile failed:\n{cmd}\n{log}")
    if not traced:
        sys.exit(f"{args.source} isn't compiled with the 13.00.9044 C2.DLL")

    if not args.function:
        for name, events in trace.functions:
            if events:
                print(f"{sum(1 for e in events if e[0] == 'new'):5d} ranges  {name}")
        return
    found = [(n, e) for n, e in trace.functions if args.function in n]
    if not found:
        sys.exit(f"no function matching {args.function!r}; run without one for the list")
    name, events = max(found, key=lambda f: len(f[1]))
    news = [e for e in events if e[0] == "new"]
    print(f"{name}: {len(news)} ranges created, {len(events) - len(news)} freed, ids up to "
          f"{max((e[1] for e in news), default=0)}")
    want = {v.strip() for v in args.vars.split(",")} if args.vars else None
    for i, ((kind, label), items) in enumerate(passes(events)):
        if want:
            items = [(w, n) for w, n in items if n in want]
            if not items:
                continue
        print(f"{i:4d} {kind:4s} {label:8s} " + " ".join(f"{w}:{n}" for w, n in items))


if __name__ == "__main__":
    main()
