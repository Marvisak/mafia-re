"""Compile with VC6's automatic precompiled headers (/YX), each source creating its own.

LS3DF.dll was built with /YX, the default of a VC6 DLL project. It shows in the order of each object's functions: the
dynamic initializers of the precompiled headers' statics (the engine header's zero vector, ZeroSVector in I3d_math.h)
and the out-of-line copies of their inline functions don't come where the headers define them. Each comes after the
first of the file's functions at which the file's own source so far outweighs the inline function bodies the
precompiled headers have before that item's definition, and at the end of the file if it never does.

- What counts in the file: function bodies, dead code included, and unused inline and static functions; not comments,
  whitespace, declarations, or the code VC6 inlines into the functions.
- What counts in the headers: inline function bodies (class members included) before the item, by size; not
  declarations, macros, or anything after the item.

So where a file's $E comes measures the header code before the zero vector: where ours comes earlier than the
original's, the original's headers had more inline code before it.

A source that creates the precompiled header gets a little more room than one that uses an existing one. In the VC6
IDE a file used the previous file's when its include lines and switches matched, which isn't known for each file.
This compiler launcher makes every source create its own: it adds /YX with a precompiled header next to the object
and deletes that file before and after the compile, so that a rebuild doesn't use an old one.

    python tools/autopch.py <compiler> <arguments...>
"""
import os
import subprocess
import sys

if len(sys.argv) < 3:
    sys.exit(__doc__)
compiler, args = sys.argv[1], sys.argv[2:]
obj = next((a[3:] for a in args if a.startswith(("/Fo", "-Fo"))), None)
if obj is None:
    sys.exit(subprocess.call([compiler] + args))
pch = obj + ".pch"
if os.path.exists(pch):
    os.remove(pch)
result = subprocess.call([compiler] + args + ["/YX", "/Fp" + pch])
if os.path.exists(pch):
    os.remove(pch)
sys.exit(result)
