"""Disassemble a function of the PS2 build (`target/SLUS_206.71`) by name.

The PS2 ELF has a full symbol table (every function with a size), so the function to read comes from a name and
the bytes from the ELF; Capstone (MIPS64, little-endian) disassembles them. R5900's multimedia instructions and
a few others come out as `.byte`, which never matters for the game code this reads. Names are CodeWarrior's
(`GetChildFrame__9I3D_frameFPCcUi`), see the project notes for the mangling.

    .venv/Scripts/python tools/ps2fn.py GetChildFrame            # every symbol containing the text, then the code
    .venv/Scripts/python tools/ps2fn.py GetChildFrame__9I3D_frameFPCcUi --exact
    .venv/Scripts/python tools/ps2fn.py --list I3D_shadow        # only the matching symbols (name, address, size)

Branch targets and `jal` destinations are resolved to symbol names where one exists, and calls to a symbol print its
name, so the structure of a function (loops, which helper is a call and which is inlined) reads off the listing. Data
references through `lui`/`addiu` pairs aren't resolved.
"""

import argparse
import struct
import sys
from pathlib import Path

import capstone

REPO = Path(__file__).resolve().parent.parent
ELF = REPO / "target" / "SLUS_206.71"


def read_elf(path):
    data = path.read_bytes()
    assert data[:4] == b"\x7fELF"
    e_shoff = struct.unpack_from("<I", data, 0x20)[0]
    e_shentsize, e_shnum, e_shstrndx = struct.unpack_from("<HHH", data, 0x2E)
    sections = []
    for i in range(e_shnum):
        off = e_shoff + i * e_shentsize
        name, typ, flags, addr, offset, size, link, info, align, entsize = (
            struct.unpack_from("<IIIIIIIIII", data, off)
        )
        sections.append(
            dict(
                name=name,
                type=typ,
                addr=addr,
                offset=offset,
                size=size,
                link=link,
                entsize=entsize,
            )
        )
    shstr = sections[e_shstrndx]
    for s in sections:
        n = shstr["offset"] + s["name"]
        s["name"] = data[n : data.index(b"\0", n)].decode()
    symtab = next(s for s in sections if s["type"] == 2)
    strtab = sections[symtab["link"]]
    symbols = []
    for i in range(symtab["size"] // 16):
        st_name, st_value, st_size, st_info, st_other, st_shndx = struct.unpack_from(
            "<IIIBBH", data, symtab["offset"] + i * 16
        )
        n = strtab["offset"] + st_name
        name = data[n : data.index(b"\0", n)].decode(errors="replace")
        symbols.append((name, st_value, st_size, st_info & 0xF, st_info >> 4))
    return data, sections, symbols


def bytes_at(data, sections, addr, size):
    for s in sections:
        if s["type"] == 1 and s["addr"] <= addr < s["addr"] + s["size"]:
            o = s["offset"] + addr - s["addr"]
            return data[o : o + size]
    raise KeyError(f"no section holds {addr:#x}")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("name", help="symbol name or a part of it")
    ap.add_argument(
        "--exact", action="store_true", help="the whole name, not a substring"
    )
    ap.add_argument(
        "--list", action="store_true", help="list the matching symbols only"
    )
    ap.add_argument("--elf", default=str(ELF))
    args = ap.parse_args()

    data, sections, symbols = read_elf(Path(args.elf))
    by_addr = {}
    for name, value, size, typ, bind in symbols:
        if typ == 2 and value:  # STT_FUNC
            by_addr.setdefault(value, name)
    matches = [
        s
        for s in symbols
        if s[3] == 2
        and (s[0] == args.name if args.exact else args.name.lower() in s[0].lower())
    ]
    if not matches:
        print("no function symbol matches", args.name)
        return 1
    for name, value, size, typ, bind in matches:
        print(f"{value:#010x} {size:6d}  {'local ' if bind == 0 else 'global'}  {name}")
    if args.list or len(matches) > 1 and not args.exact:
        if len(matches) > 1 and not args.list:
            print("\nseveral match; pass the whole name with --exact")
        return 0

    name, addr, size, _, _ = matches[0]
    code = bytes_at(data, sections, addr, size)
    md = capstone.Cs(
        capstone.CS_ARCH_MIPS, capstone.CS_MODE_MIPS64 | capstone.CS_MODE_LITTLE_ENDIAN
    )
    md.skipdata = True
    print(f"\n{name} ({addr:#x}, {size} bytes)")
    for insn in md.disasm(code, addr):
        text = f"{insn.mnemonic} {insn.op_str}".strip()
        for tok in insn.op_str.replace(",", " ").split():
            if tok.startswith("0x"):
                try:
                    v = int(tok, 16)
                except ValueError:
                    continue
                if v in by_addr:
                    text += f"    ; {by_addr[v]}"
                elif addr <= v < addr + size:
                    text += f"    ; +{v - addr:#x}"
        print(f"  {insn.address:#x}  {text}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
