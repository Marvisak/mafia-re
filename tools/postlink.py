"""Write the original's link-time metadata into a rebuilt PE file.

The linker stamps every output with values of the moment and place it ran: the PE header's
TimeDateStamp (VC6 LINK and VS2008 link always write the current time), the export directory's and
the debug directory's TimeDateStamp, and the CodeView record of a /DEBUG link (the PDB's signature or
GUID, its age, which goes up every time the linker reuses the PDB, and its path). None of them comes
from the source, so the byte-exact builds set the original's after the link:

    python tools/postlink.py setup.exe --timestamp 0x3dca7e48 \\
        --codeview-signature 0x3dca7530 --codeview-age 2 \\
        --codeview-path "C:\\Mafia\\_src\\setup\\Release\\setup.pdb"

- --timestamp sets the PE header's, the export directory's and every debug directory entry's.
- --codeview-signature (NB10, VC6) or --codeview-guid (RSDS, VS2008), --codeview-age and
  --codeview-path rewrite the CodeView record. An NB10 record after the last section (VC6) may change
  length, which changes the file size and the debug directory entry's SizeOfData; a record inside a
  section (VS2008 puts it in .rdata) must keep its length, so link with a path of the same length
  (VS2008's /PDBALTPATH).
- --checksum recomputes the PE checksum (/RELEASE) once everything else is written.

The PDB keeps the linker's values, so a debugger no longer pairs the patched file with it. reccmp
reads the PDB named in reccmp-build.yml and doesn't check the pairing.
"""

import argparse
import struct
import sys
from pathlib import Path

DEBUG_TYPE_CODEVIEW = 2


class PE:
    def __init__(self, data: bytearray):
        self.data = data
        self.pe = struct.unpack_from("<I", data, 0x3C)[0]
        if data[self.pe:self.pe + 4] != b"PE\0\0":
            raise ValueError("not a PE file")
        self.optional = self.pe + 24
        if struct.unpack_from("<H", data, self.optional)[0] != 0x10B:
            raise ValueError("not a PE32 file")
        count = struct.unpack_from("<H", data, self.pe + 6)[0]
        optional_size = struct.unpack_from("<H", data, self.pe + 20)[0]
        self.sections = []
        for i in range(count):
            header = self.optional + optional_size + 40 * i
            virtual_size, rva, raw_size, raw = struct.unpack_from("<IIII", data, header + 8)
            self.sections.append((rva, max(virtual_size, raw_size), raw))

    def offset(self, rva: int) -> int:
        for start, size, raw in self.sections:
            if start <= rva < start + size:
                return raw + rva - start
        raise ValueError(f"RVA {rva:#x} is in no section")

    def directory(self, index: int) -> tuple[int, int]:
        return struct.unpack_from("<II", self.data, self.optional + 96 + 8 * index)

    def end_of_sections(self) -> int:
        return max(raw + size for _, size, raw in self.sections)


def checksum(data: bytes, field: int) -> int:
    """The PE checksum: 16-bit one's-complement-style sum of the file without the checksum field,
    plus the file length."""
    padded = data + b"\0" * (len(data) & 1)
    total = 0
    for i in range(0, len(padded), 2):
        if field <= i < field + 4:
            continue
        total += padded[i] | padded[i + 1] << 8
        total = (total & 0xFFFF) + (total >> 16)
    total = (total & 0xFFFF) + (total >> 16)
    return (total + len(data)) & 0xFFFFFFFF


def patch(path: Path, args: argparse.Namespace) -> None:
    data = bytearray(path.read_bytes())
    pe = PE(data)

    if args.timestamp is not None:
        struct.pack_into("<I", data, pe.pe + 8, args.timestamp)
        export_rva, export_size = pe.directory(0)
        if export_size:
            struct.pack_into("<I", data, pe.offset(export_rva) + 4, args.timestamp)

    debug_rva, debug_size = pe.directory(6)
    entries = [pe.offset(debug_rva) + 28 * i for i in range(debug_size // 28)] if debug_size else []
    if args.timestamp is not None:
        for entry in entries:
            struct.pack_into("<I", data, entry + 4, args.timestamp)

    codeview = any(v is not None for v in (args.codeview_signature, args.codeview_guid,
                                           args.codeview_age, args.codeview_path))
    if codeview:
        entry = next((e for e in entries if struct.unpack_from("<I", data, e + 12)[0] == DEBUG_TYPE_CODEVIEW), None)
        if entry is None:
            sys.exit(f"{path}: no CodeView debug directory entry")
        size, rva, raw = struct.unpack_from("<III", data, entry + 16)
        record = bytes(data[raw:raw + size])
        if record[:4] == b"NB10":
            if args.codeview_guid is not None:
                sys.exit(f"{path}: an NB10 record has a signature, not a GUID")
            signature, age = struct.unpack_from("<II", record, 8)
            head = b"NB10" + struct.pack("<III", 0,
                                         signature if args.codeview_signature is None else args.codeview_signature,
                                         age if args.codeview_age is None else args.codeview_age)
            old_path = record[16:].split(b"\0")[0]
        elif record[:4] == b"RSDS":
            if args.codeview_signature is not None:
                sys.exit(f"{path}: an RSDS record has a GUID, not a signature")
            guid = record[4:20] if args.codeview_guid is None else bytes.fromhex(args.codeview_guid)
            if len(guid) != 16:
                sys.exit("--codeview-guid takes 16 bytes in file order (32 hex digits)")
            age = struct.unpack_from("<I", record, 20)[0]
            head = b"RSDS" + guid + struct.pack("<I", age if args.codeview_age is None else args.codeview_age)
            old_path = record[24:].split(b"\0")[0]
        else:
            sys.exit(f"{path}: unknown CodeView record {record[:4]!r}")
        new_path = old_path if args.codeview_path is None else args.codeview_path.encode("ascii")
        new_record = head + new_path + b"\0"

        if rva == 0 and raw >= pe.end_of_sections():
            # VC6: the record follows the sections and ends the file, so it can change length
            if raw + size != len(data):
                sys.exit(f"{path}: data follows the CodeView record")
            data[raw:] = new_record
            struct.pack_into("<I", data, entry + 16, len(new_record))
        elif len(new_record) == size:
            data[raw:raw + size] = new_record
        else:
            sys.exit(f"{path}: the CodeView record is inside a section and would change length "
                     f"({size:#x} -> {len(new_record):#x}); link with a path of the same length")

    if args.checksum:
        field = pe.optional + 64
        struct.pack_into("<I", data, field, checksum(bytes(data), field))

    path.write_bytes(data)


def number(text: str) -> int:
    return int(text, 0)


def main() -> None:
    parser = argparse.ArgumentParser(description="Write the original's link-time metadata into a rebuilt PE file.")
    parser.add_argument("file", type=Path)
    parser.add_argument("--timestamp", type=number, help="PE header, export and debug directory TimeDateStamp")
    parser.add_argument("--codeview-signature", type=number, help="NB10 record: the PDB's signature")
    parser.add_argument("--codeview-guid", help="RSDS record: the PDB's GUID, 32 hex digits in file order")
    parser.add_argument("--codeview-age", type=number, help="the PDB's age")
    parser.add_argument("--codeview-path", help="the PDB's path")
    parser.add_argument("--checksum", action="store_true", help="recompute the PE checksum")
    args = parser.parse_args()
    patch(args.file, args)


if __name__ == "__main__":
    main()
