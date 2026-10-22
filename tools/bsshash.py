"""Predict the order VC6 gives a translation unit's uninitialized globals in .bss.

VC6 (SP5 + Processor Pack) doesn't lay out .bss in declaration order. It keeps the uninitialized
globals of a translation unit in a hash table with 1024 buckets and allocates them bucket by bucket,
from bucket 0 up:

    h = 0
    for each character c:  h = 4*h + (h >> 4) + c     (unsigned 32-bit)
    bucket = (h ^ (h >> 16)) & 0x3FF

- Key: the identifier for plain and `static` globals (type and linkage don't matter), the full
  decorated name for class static members (?count@C_thing@@2HA) and function-local statics
  (?x@?1??use@@YAHH@Z@4HA). Namespace members follow neither form.
- Within one bucket, the global declared last comes first. The first declaration counts, so an
  extern in a header decides it, not the definition.
- Zero-initialized globals (= 0, = false) aren't hashed. They come after all hashed ones, in
  declaration order, like initialized globals in .data.
- Alignment: 1-byte objects 1, 2-byte 2, 3 to 0x3f bytes 4, 0x40 bytes and more 8, or the type's
  own alignment if that is larger (double, __int64).

A function compiled without optimization lays out the locals of each scope by the same hash: the
bucket mod 16, the lowest at the top of the frame (Steam's Czech setup.exe, setup/main.cpp).

Give the names in declaration order:

    .venv/Scripts/python tools/bsshash.py v_file_h v_handle C_m_p C_l_d unreferenced_buffer
"""

import argparse


def bucket(key: str) -> int:
    h = 0
    for c in key.encode("ascii"):
        h = (4 * h + (h >> 4) + c) & 0xFFFFFFFF
    return (h ^ (h >> 16)) & 0x3FF


def main() -> None:
    parser = argparse.ArgumentParser(description="Predict VC6's .bss order of uninitialized globals.")
    parser.add_argument("names", nargs="+", help="identifiers (decorated names for class statics and local "
                                                 "statics), in declaration order")
    args = parser.parse_args()
    order = sorted(enumerate(args.names), key=lambda item: (bucket(item[1]), -item[0]))
    for _, name in order:
        print(f"{bucket(name):5}  {name}")


if __name__ == "__main__":
    main()
