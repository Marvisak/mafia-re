// A file of which only the engine header's statics are left, at 0x10061990-0x100619b0 in the original: the header's
// zero vector's $E, the vector (0x101c3c24) and the identity matrix (0x1009d358). Nothing else of it is in the DLL, not
// even a string, so its functions were dropped by /OPT:REF or compiled out on PC (docs/ls3df-files.md). The name is a
// guess that sorts between its neighbours', Load5ds.cpp and RadixSort.cpp.
#include "I3D/I3D.h"
