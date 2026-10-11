"""Compile a C++ snippet with the project's VC6 flags and print its stack slot map.

The quick way to test a VC6 rule: write a small function, compile it with the same compiler and
switches as rw_data (/O2 /ML /GX /Gz), and read the frame layout off the /FAs listing. The
frame layout rules in framelayout.py were found this way; the snippet below is the
template they used, with four register hogs (h1..h4 own ebx/ebp/esi/edi across every call) so
every other local is spilled, and calls to an external function between uses so each use is a
separate memory reference.

usage: vc6probe.py <snippet.cpp> [function]     function defaults to ?f@@YGHH@Z (f with /Gz)

A snippet is the body of `int __stdcall f(int n)` plus any helpers; it may use
    extern "C" int __stdcall ext(int); extern int g[1024]; extern unsigned char gb[64];
Example (hosts a1..a3 die before q, which then takes the free slot of the smallest weight not
below its own):

    int a1 = g[10], a2 = g[11], a3 = g[12];
    g[20] = a1; g[21] = a2; g[22] = a3; ext(2);
    g[23] = a1; g[24] = a2; ext(3);
    g[25] = a1; ext(4);
    int q = g[13]; ext(5); g[29] = q;
"""
import os
import re
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VC6 = os.path.join(ROOT, "msvc600_sp5_vcpp")
HEADER = '''extern "C" int __stdcall ext(int);
extern "C" int __stdcall ext2(int *);
extern int g[1024];
extern unsigned char gb[64];
'''
HOG_DECL = "    int h1 = g[1], h2 = g[2], h3 = g[3], h4 = g[4];\n"


def hog_uses(t):
    return "".join(f"    g[{t + x}] = h1; g[{t + x + 1}] = h2; g[{t + x + 2}] = h3; g[{t + x + 3}] = h4;\n"
                   for x in range(0, 40, 4))


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    body = open(sys.argv[1]).read()
    fn = sys.argv[2] if len(sys.argv) > 2 else "?f@@YGHH@Z"
    helpers, lines = [], []
    for line in body.split("\n"):   # top-level helpers (not indented) go before f
        (lines if line.startswith(" ") or not line.strip() else helpers).append(line)
    src = (HEADER + "\n".join(helpers) + "\nint __stdcall f(int n)\n{\n" + HOG_DECL + hog_uses(100)
           + "    ext(1);\n" + "\n".join(lines) + "\n    ext(99);\n" + hog_uses(200) + "    return 0;\n}\n")
    work = tempfile.mkdtemp(prefix="vc6probe")
    cpp = os.path.join(work, "probe.cpp")
    asm = os.path.join(work, "probe.asm")
    open(cpp, "w").write(src)
    env = dict(os.environ)
    env["PATH"] = os.path.join(VC6, "VC98", "Bin") + ";" + os.path.join(VC6, "Common", "MSDev98", "Bin") + ";" + env["PATH"]
    env["INCLUDE"] = os.path.join(VC6, "VC98", "Include")
    env["LIB"] = os.path.join(VC6, "VC98", "Lib")
    r = subprocess.run([os.path.join(VC6, "VC98", "Bin", "CL.EXE"), "/nologo", "/O2", "/ML", "/GX", "/Gz", "/c",
                        "/FAs", "/Fa" + asm, "/Fo" + os.path.join(work, "probe.obj"), cpp],
                       capture_output=True, text=True, env=env)
    if r.returncode != 0:
        print(r.stdout, r.stderr)
        return 1
    print(f"listing: {asm}")
    return subprocess.call([sys.executable, os.path.join(ROOT, "tools", "slotmap.py"), asm, fn])


if __name__ == "__main__":
    sys.exit(main())
