"""Run the CL.EXE of another directory in place of the one CMake calls.

CMake has one C++ compiler per build, but a target can name a compiler launcher, which gets the compiler's path
and its arguments. This launcher drops the path and runs CL.EXE from the given directory with the same arguments,
so one target can use other compiler binaries (IJoy.dll's, see ijoy/CMakeLists.txt):

    python tools/clswap.py <directory> <compiler> <arguments...>

CL.EXE loads C1XX.DLL and C2.DLL from its own directory, so the directory holds the whole set.
"""
import subprocess
import sys
from pathlib import Path

if len(sys.argv) < 3:
    sys.exit(__doc__)
sys.exit(subprocess.call([str(Path(sys.argv[1]) / 'CL.EXE')] + sys.argv[3:]))
