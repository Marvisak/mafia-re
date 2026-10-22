"""Run the CL.EXE of another directory in place of the one CMake calls.

CMake has one C++ compiler per build, but a target can name a compiler launcher, which gets the compiler's path
and its arguments. This launcher drops the path and runs CL.EXE from the given directory with the same arguments,
so one target can use other compiler binaries (IJoy.dll's, see ijoy/CMakeLists.txt):

    python tools/clswap.py <directory> [--comp-id <hex>] <compiler> <arguments...>

CL.EXE loads C1XX.DLL and C2.DLL from its own directory, so the directory holds the whole set.

--comp-id writes another compiler's id into the object's @comp.id symbol, which the linker's Rich header lists: for
a compiler that isn't available but whose code another one gives (Steam's Czech setup.exe, see setup/CMakeLists.txt).
"""
import struct
import subprocess
import sys
from pathlib import Path


def set_comp_id(obj: Path, comp_id: int) -> None:
    data = bytearray(obj.read_bytes())
    symbols, count = struct.unpack_from('<II', data, 8)
    for i in range(count):
        o = symbols + 18 * i
        if data[o:o + 8] == b'@comp.id':
            struct.pack_into('<I', data, o + 8, comp_id)
            obj.write_bytes(data)
            return
    sys.exit(f'{obj}: no @comp.id symbol')


if len(sys.argv) < 3:
    sys.exit(__doc__)
args = sys.argv[2:]
comp_id = None
if args[0] == '--comp-id':
    comp_id = int(args[1], 16)
    args = args[2:]
result = subprocess.call([str(Path(sys.argv[1]) / 'CL.EXE')] + args[1:])
if result == 0 and comp_id is not None:
    objects = [a[3:] for a in args[1:] if a[:3] in ('/Fo', '-Fo')]
    if len(objects) != 1:
        sys.exit(f'clswap: --comp-id needs one /Fo<object>, got {objects}')
    set_comp_id(Path(objects[0]), comp_id)
sys.exit(result)
