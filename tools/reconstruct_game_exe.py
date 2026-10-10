"""Reconstruct the original, unprotected Mafia v1.2 Game.exe.

No unprotected copy of Game.exe survives. Three copies of the same build (PE timestamp 0x3F004AA7,
2003-06-30) do, and each one is damaged in a different place:

* Steam (target/Game.exe): a SafeDisc unwrap, the same file as the scene no-CD exe. Its code and
  import table are restored, but:
  - about 2,100 padding gaps between functions hold junk;
  - the import hints are zeroed;
  - the header is damaged: a VLV block over the DOS stub and Rich header, a fake timestamp, no IAT
    directory, and SafeDisc's SizeOfCode;
  - three code patches: no-CD, HKLM -> HKCU, and the missing-archive check NOPed.
* CD 2 of the 2003 re-release (MafiaGame\\Game.exe): the same build under a second protector, probably
  SecuROM. It has clean padding, the original hints and the DOS stub/Rich header, but its import calls
  are redirected and about 20 KB of CRT startup code is encrypted.
* Patch 1.2 (Patch\\Game.exe): the same build under SafeDisc 2.80.011. Its code and data are
  encrypted, but the header and section table are intact. It is used only for cross-checks.

Every output byte comes from one of the copies by an explicit rule, or from a VC6 LINK convention
checked against LS3DF/rw_data/IJoy/INet. Any difference that no rule explains aborts the script. The
only bytes that remain unverified are padding gaps inside the CRT code that CD 2 encrypted; they are
listed at the end.

Usage (7-Zip must be installed):
    python tools/reconstruct_game_exe.py [--steam target/Game.exe] [--cd2 target/Game.cd2.exe]
        [--patch target/mafia_patch_1.2_eng.exe] [--out target/Game.orig.exe]
"""

import argparse
import glob
import hashlib
import os
import shutil
import struct
import subprocess
import sys
import tempfile

BASE = 0x400000
PE = 0x100                       # e_lfanew of the original (Steam and patch agree)
OPT = PE + 24
SECTIONS = OPT + 0xE0
FILE_SIZE = 0x23F000
TEXT, TEXT_VEND, TEXT_RAW_END = 0x1000, 0x1000 + 0x2212AA, 0x223000   # file offset == RVA up to .rsrc
# Steam and the patch exe end .text with a 24-byte byte-loop strlen at 0x622292 that nothing calls. It sits
# after the .text$x unwind funclets, which VC6 links last, and CD 2 has zeros there. So it was most likely
# appended by SafeDisc, and the linked .text ended at 0x622292 (inference, not proof).
TEXT_VEND_LINKED = 0x1000 + 0x221292
SAFEDISC_APPENDIX = bytes.fromhex('8b4c24048a118d410184d274078a104084d275f92bc148c3')
# SafeDisc also appended 31 bytes ("GetSysteminfo" + data, plaintext in the patch exe) after the import
# names and grew .rdata from 0xAECC (CD 2's section table) to 0xAEEB.
RDATA_VEND_LINKED = 0x223000 + 0xAECC
RDATA, RDATA_END = 0x223000, 0x22E000
DATA, DATA_END = 0x22E000, 0x23E000
RSRC, RSRC_END = 0x23E000, 0x23F000
IMPORT_DIR, IMPORT_DIR_SIZE = 0x22D050, 0xA0
IAT_LO, IAT_HI = 0x223000, 0x22322C
TIMESTAMP = 0x3F004AA7
CRT_ENCRYPTED = (0x210000, 0x215000)   # CD 2 encrypted this CRT startup code; only Steam has it

# Patches in the Steam .text: file offset -> (original bytes, Steam bytes)
KNOWN_PATCHES = {
    0x166440: (bytes.fromhex('81ec9001'), bytes.fromhex('33c040c3')),          # CD check: sub esp,190h -> return 1
    0x1BEC62: (bytes.fromhex('02'), bytes.fromhex('01')),                      # RegOpenKeyA: HKEY_LOCAL_MACHINE -> HKEY_CURRENT_USER
    0x1BF616: (bytes.fromhex('0f8512030000'), bytes.fromhex('909090909090')),  # jnz to the "archive missing" error
}


def u16(d, o): return struct.unpack_from('<H', d, o)[0]
def u32(d, o): return struct.unpack_from('<I', d, o)[0]
def align(x): return (x + 0xFFF) & ~0xFFF


def extract(archive, member, work):
    seven = shutil.which('7z') or r'C:\Program Files\7-Zip\7z.exe'
    out_dir = tempfile.mkdtemp(dir=work)
    subprocess.run([seven, 'e', '-y', f'-o{out_dir}', archive, member], check=True, stdout=subprocess.DEVNULL)
    return open(os.path.join(out_dir, os.path.basename(member)), 'rb').read()


def load(path, member, work):
    if path.lower().endswith('.iso') or (member == 'Patch\\Game.exe' and os.path.getsize(path) > 10_000_000):
        return extract(path, member, work)
    return open(path, 'rb').read()


def diff_groups(a, b, lo, hi, join=2):
    groups = []
    for i in range(lo, hi):
        if a[i] != b[i]:
            if groups and i - groups[-1][1] <= join:
                groups[-1][1] = i
            else:
                groups.append([i, i])
    return [(x, y + 1) for x, y in groups]


def import_operand(steam, lo, hi):
    """Start of a Steam instruction with an IAT memory operand that covers [lo, hi), or None."""
    for j in range(hi - 6, lo + 1):
        if steam[j] in (0xFF, 0x8B) and steam[j + 1] & 0xC7 == 0x05:
            if IAT_LO <= u32(steam, j + 2) - BASE < IAT_HI and j <= lo and hi <= j + 6:
                return j
        if steam[j] == 0xA1 and IAT_LO <= u32(steam, j + 1) - BASE < IAT_HI and j <= lo and hi <= j + 5:
            return j
    return None


def library_hints(lib_dir):
    """Hints that the VC6 import libraries record, {(DLL, name): hint}."""
    hints = {}
    for lib in glob.glob(os.path.join(lib_dir, '*.LIB')):
        if os.path.basename(lib).upper() not in ('KERNEL32.LIB', 'USER32.LIB', 'ADVAPI32.LIB', 'WINMM.LIB'):
            continue
        d = open(lib, 'rb').read()
        i = 8
        while i < len(d):
            size = int(d[i + 48:i + 58])
            m = d[i + 60:i + 60 + size]
            if len(m) >= 20 and m[:4] == b'\0\0\xff\xff':          # short import member
                sym, dll = m[20:].split(b'\0')[:2]
                hints[(dll.decode().upper(), sym.decode().lstrip('_').split('@')[0])] = u16(m, 16)
            i += 60 + size + (size & 1)
    return hints


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--steam', default='target/Game.exe')
    ap.add_argument('--cd2', default='target/Game.cd2.exe', help="CD 2's MafiaGame\\Game.exe, or the CD 2 image")
    ap.add_argument('--patch', default='target/mafia_patch_1.2_eng.exe', help='1.2 patch SFX or its extracted Game.exe')
    ap.add_argument('--libs', default='msvc600_sp5_vcpp/VC98/Lib')
    ap.add_argument('--out', default='target/Game.orig.exe')
    args = ap.parse_args()

    with tempfile.TemporaryDirectory() as work:
        steam = open(args.steam, 'rb').read()
        cd2 = load(args.cd2, 'MafiaGame\\Game.exe', work)
        patch = load(args.patch, 'Patch\\Game.exe', work)

    assert len(steam) == FILE_SIZE and steam[0x40:0x44] == b'VLV\0', 'unexpected Steam Game.exe'
    assert u32(cd2, u32(cd2, 0x3C) + 8) == TIMESTAMP and u32(patch, PE + 8) == TIMESTAMP, 'not the 1.2 build'
    out = bytearray(steam)
    log = {}
    unverified = []

    # --- Header: Steam's, minus the damage. ---
    assert cd2[0x40:0x100] == patch[0x40:0x100]
    out[0x40:0x100] = cd2[0x40:0x100]                                  # DOS stub + Rich header
    struct.pack_into('<I', out, PE + 8, TIMESTAMP)
    nsec = u16(out, PE + 6)
    secs = [struct.unpack_from('<8sIIIIIIHHI', out, SECTIONS + 40 * k) for k in range(nsec)]
    assert out[SECTIONS:SECTIONS + 40 * nsec] == patch[SECTIONS:SECTIONS + 40 * nsec], 'section table differs from the patch exe'
    # VC6 LINK conventions, checked against LS3DF, rw_data, IJoy and INet
    struct.pack_into('<I', out, OPT + 4, sum(align(s[1]) for s in secs if s[9] & 0x20))   # SizeOfCode
    struct.pack_into('<I', out, OPT + 8, sum(align(s[1]) for s in secs if s[9] & 0x40))   # SizeOfInitializedData
    struct.pack_into('<I', out, OPT + 56, align(secs[-1][2] + secs[-1][1]))              # SizeOfImage
    struct.pack_into('<II', out, OPT + 96 + 8 * 12, IAT_LO, IAT_HI - IAT_LO)              # IAT directory
    for off in range(OPT, OPT + 0xE0):
        if off in range(OPT + 4, OPT + 12) or off in range(OPT + 16, OPT + 20) or off in range(OPT + 56, OPT + 60) \
                or off in range(OPT + 96 + 8, OPT + 96 + 16) or off in range(OPT + 96 + 96, OPT + 96 + 104):
            continue                                                   # fields SafeDisc rewrote
        assert out[off] == patch[off], f'optional header byte {off:#x} differs from the patch exe'
    assert not any(out[SECTIONS + 40 * nsec:0x1000]), 'header padding not empty'

    # --- .text: CD 2's, plus Steam's where CD 2's protector changed it. ---
    for lo, hi in diff_groups(cd2, steam, TEXT, TEXT_VEND_LINKED):
        if lo in KNOWN_PATCHES:
            orig, patched = KNOWN_PATCHES[lo]
            assert cd2[lo:lo + len(orig)] == orig and steam[lo:lo + len(patched)] == patched and hi <= lo + len(orig)
            out[lo:hi] = cd2[lo:hi]
            log['Steam/no-CD patch undone'] = log.get('Steam/no-CD patch undone', 0) + 1
            continue
        j = import_operand(steam, lo, hi)
        if j is not None:
            out[j:j + 6] = steam[j:j + 6]
            log['import reference redirected by CD 2 protector'] = log.get('import reference redirected by CD 2 protector', 0) + 1
            continue
        fill = cd2[lo]
        if fill in (0x90, 0xCC) and all(x == fill for x in cd2[lo:hi]):
            end = hi
            while cd2[end] == fill:
                end += 1
            assert end % 16 == 0, f'padding at {lo + BASE:#x} does not end at a function boundary'
            out[lo:hi] = cd2[lo:hi]
            log['junk in padding (SafeDisc unwrap)'] = log.get('junk in padding (SafeDisc unwrap)', 0) + 1
            continue
        if hi - lo > 64 and CRT_ENCRYPTED[0] <= lo and hi <= CRT_ENCRYPTED[1]:
            out[lo:hi] = steam[lo:hi]
            log['CRT block encrypted on CD 2 (bytes)'] = log.get('CRT block encrypted on CD 2 (bytes)', 0) + hi - lo
            unverified += suspicious_gaps(steam, lo, hi)
            continue
        sys.exit(f'unexplained .text difference at {lo + BASE:#x}: cd2 {cd2[lo:hi].hex()} steam {steam[lo:hi].hex()}')
    assert steam[TEXT_VEND_LINKED:TEXT_VEND] == SAFEDISC_APPENDIX and not any(cd2[TEXT_VEND_LINKED:TEXT_RAW_END])
    out[TEXT_VEND_LINKED:TEXT_RAW_END] = bytes(TEXT_RAW_END - TEXT_VEND_LINKED)   # raw padding (Steam: strlen + 0x10s)
    struct.pack_into('<I', out, SECTIONS + 8, TEXT_VEND_LINKED - TEXT)            # .text VirtualSize
    log['SafeDisc strlen appendix removed, .text VirtualSize 0x221292 (inferred)'] = 1

    # --- .rdata: CD 2's, with the import descriptors from Steam/patch. ---
    descriptors = range(IMPORT_DIR, IMPORT_DIR + IMPORT_DIR_SIZE)
    assert steam[IMPORT_DIR:IMPORT_DIR + IMPORT_DIR_SIZE] == patch[IMPORT_DIR:IMPORT_DIR + IMPORT_DIR_SIZE]
    hint_fields = {}
    hints = library_hints(args.libs)
    for d in range(IMPORT_DIR, IMPORT_DIR + IMPORT_DIR_SIZE - 20, 20):
        oft, name = u32(steam, d), u32(steam, d + 12)
        dll = steam[name:steam.index(b'\0', name)].decode().upper()
        while u32(steam, oft):
            e = u32(steam, oft)
            fn = steam[e + 2:steam.index(b'\0', e + 2)].decode()
            hint_fields[e] = hints.get((dll, fn))
            oft += 4
    cd2_pe = u32(cd2, 0x3C)
    cd2_secs = {struct.unpack_from('<8s', cd2, s)[0].rstrip(b'\0'): u32(cd2, s + 8)
                for s in range(cd2_pe + 24 + u16(cd2, cd2_pe + 20), cd2_pe + 24 + u16(cd2, cd2_pe + 20) + 40 * u16(cd2, cd2_pe + 6), 40)}
    rdata_vend = u32(out, SECTIONS + 40 + 8) + RDATA
    assert cd2_secs[b'.rdata'] + RDATA == RDATA_VEND_LINKED and not any(cd2[RDATA_VEND_LINKED:RDATA_END])
    assert steam[RDATA_VEND_LINKED:rdata_vend] == patch[RDATA_VEND_LINKED:rdata_vend]
    out[RDATA_VEND_LINKED:RDATA_END] = bytes(RDATA_END - RDATA_VEND_LINKED)
    struct.pack_into('<I', out, SECTIONS + 40 + 8, RDATA_VEND_LINKED - RDATA)     # .rdata VirtualSize
    log['SafeDisc .rdata appendix removed, .rdata VirtualSize 0xAECC (CD 2)'] = 1
    for lo, hi in diff_groups(cd2, steam, RDATA, RDATA_VEND_LINKED, join=1):
        if lo in descriptors and hi - 1 in descriptors:
            continue                                                   # rebuilt below
        e = lo if lo in hint_fields else lo - 1
        assert e in hint_fields and hi <= e + 2, f'unexplained .rdata difference at {lo + BASE:#x}'
        assert hint_fields[e] in (None, u16(cd2, e)), f'CD 2 hint at {e + BASE:#x} disagrees with the import library'
        out[lo:hi] = cd2[lo:hi]
        log['import hint zeroed by Steam unwrap'] = log.get('import hint zeroed by Steam unwrap', 0) + 1
    out[IMPORT_DIR:IMPORT_DIR + IMPORT_DIR_SIZE] = steam[IMPORT_DIR:IMPORT_DIR + IMPORT_DIR_SIZE]
    for d in range(IMPORT_DIR, IMPORT_DIR + IMPORT_DIR_SIZE - 20, 20):
        struct.pack_into('<II', out, d + 4, 0, 0)                      # TimeDateStamp/ForwarderChain: VC6 writes 0, SafeDisc -1

    # --- .data and .rsrc ---
    assert cd2[DATA:DATA_END] == steam[DATA:DATA_END], '.data differs'
    assert steam[RSRC:RSRC_END] == patch[RSRC:RSRC_END], '.rsrc differs'

    with open(args.out, 'wb') as f:
        f.write(out)
    for k, v in log.items():
        print(f'{v:6}  {k}')
    print(f'wrote {args.out}  md5 {hashlib.md5(out).hexdigest()}  sha1 {hashlib.sha1(out).hexdigest()}')
    if unverified:
        print('unverified padding (Steam bytes, CD 2 encrypted):')
        for va, gap in unverified:
            print(f'  {va:#x}  {gap.hex(" ")}')


_call_targets = None


def suspicious_gaps(steam, lo, hi):
    """Padding gaps inside [lo, hi) that are not clean 0x90/0xCC fill: gap after ret up to a called, aligned address."""
    global _call_targets
    if _call_targets is None:
        _call_targets = {i + 5 + struct.unpack_from('<i', steam, i + 1)[0]
                         for i in range(TEXT, TEXT_VEND - 5) if steam[i] == 0xE8}
    found = []
    for t in range(lo + 16 - lo % 16, hi, 16):
        if t not in _call_targets:
            continue
        for k in range(1, 16):
            if steam[t - k] == 0xC3 or (steam[t - k - 2] == 0xC2 and steam[t - k] == 0):
                gap = steam[t - k + 1:t]
                if gap and not all(x == 0x90 for x in gap) and not all(x == 0xCC for x in gap):
                    found.append((t - k + 1 + BASE, gap))
                break
    return found


if __name__ == '__main__':
    main()
