"""Localize Steam's 2010 setup.exe the way Steam's German, French, Italian and Spanish ones were made.

Those four are the English link with other resources, written after it through Windows' UpdateResource: their
.rsrc is laid out as EndUpdateResource lays it out (the same as mt.exe's, which also uses it), and their checksum is
still the English one, which EndUpdateResource doesn't recompute. So the build does the same to a copy of the
English setup.exe:

    python tools/localize.py build/vs2008/setup/setup.exe build/vs2008/setup/german.res build/vs2008/setup_german/setup.exe

- Every resource of the .res replaces the exe's of the same type and name, in every language: a dialog of another
  language (Spanish) replaces the English one.
- The DLGINIT goes: none of the four has it.
- The linker's map is copied next to the output, for tools/filediff.py.
"""

import ctypes
import shutil
import struct
import sys
from ctypes import wintypes
from pathlib import Path

RT_DLGINIT = 240

kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
kernel32.BeginUpdateResourceW.restype = wintypes.HANDLE
kernel32.BeginUpdateResourceW.argtypes = [wintypes.LPCWSTR, wintypes.BOOL]
kernel32.UpdateResourceW.restype = wintypes.BOOL
kernel32.UpdateResourceW.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_void_p, wintypes.WORD,
                                     ctypes.c_void_p, wintypes.DWORD]
kernel32.EndUpdateResourceW.restype = wintypes.BOOL
kernel32.EndUpdateResourceW.argtypes = [wintypes.HANDLE, wintypes.BOOL]


def read_res(path: Path) -> list[tuple[int, int, int, bytes]]:
    """The resources of a .res file: (type, name, language, data). Only numeric types and names."""
    d = path.read_bytes()
    out = []
    o = 0
    while o < len(d):
        size, header = struct.unpack_from("<II", d, o)
        h = o + 8
        ids = []
        for _ in range(2):
            if struct.unpack_from("<H", d, h)[0] != 0xFFFF:
                raise ValueError(f"{path}: resource with a string type or name at 0x{o:x}")
            ids.append(struct.unpack_from("<H", d, h + 2)[0])
            h += 4
        _, _, lang = struct.unpack_from("<IHH", d, h)
        if ids[0]:  # the first entry is an empty one
            out.append((ids[0], ids[1], lang, d[o + header:o + header + size]))
        o = (o + header + size + 3) & ~3
    return out


def languages(exe: Path, typ: int, name: int) -> list[int]:
    """The languages the exe has a resource of this type and name in."""
    found = []

    def callback(module, t, n, lang, param):
        found.append(lang)
        return True

    proc = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HMODULE, ctypes.c_void_p, ctypes.c_void_p, wintypes.WORD,
                              ctypes.c_void_p)(callback)
    module = kernel32.LoadLibraryExW(str(exe), None, 0x2)  # LOAD_LIBRARY_AS_DATAFILE
    try:
        kernel32.EnumResourceLanguagesW(module, ctypes.c_void_p(typ), ctypes.c_void_p(name), proc, None)
    finally:
        kernel32.FreeLibrary(module)
    return found


def update(handle, typ: int, name: int, lang: int, data: bytes | None) -> None:
    buf = ctypes.create_string_buffer(data, len(data)) if data is not None else None
    if not kernel32.UpdateResourceW(handle, ctypes.c_void_p(typ), ctypes.c_void_p(name), lang, buf,
                                    len(data) if data is not None else 0):
        raise ctypes.WinError(ctypes.get_last_error())


def main() -> None:
    exe, res, out = (Path(a) for a in sys.argv[1:4])
    kernel32.LoadLibraryExW.restype = wintypes.HMODULE
    kernel32.LoadLibraryExW.argtypes = [wintypes.LPCWSTR, wintypes.HANDLE, wintypes.DWORD]
    kernel32.EnumResourceLanguagesW.argtypes = [wintypes.HMODULE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p,
                                                ctypes.c_void_p]
    kernel32.FreeLibrary.argtypes = [wintypes.HMODULE]
    new = read_res(res)
    # What to remove, decided on the English exe before it changes
    removals = [(RT_DLGINIT, name, lang) for name in (101,) for lang in languages(exe, RT_DLGINIT, name)]
    for typ, name, lang, _ in new:
        removals += [(typ, name, other) for other in languages(exe, typ, name) if other != lang]

    out.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(exe, out)
    if exe.with_suffix(".map").exists():
        shutil.copyfile(exe.with_suffix(".map"), out.with_suffix(".map"))
    handle = kernel32.BeginUpdateResourceW(str(out), False)
    if not handle:
        raise ctypes.WinError(ctypes.get_last_error())
    for typ, name, lang in removals:
        update(handle, typ, name, lang, None)
    for typ, name, lang, data in new:
        update(handle, typ, name, lang, data)
    if not kernel32.EndUpdateResourceW(handle, False):
        raise ctypes.WinError(ctypes.get_last_error())


if __name__ == "__main__":
    main()
