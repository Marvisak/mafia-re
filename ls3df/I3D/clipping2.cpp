// A file of which only the engine header's statics are left, at 0x10004870-0x10004890 in the original: the header's
// zero vector's $E, the vector (0x100af7d8) and the identity matrix (0x1009b360). Nothing else of it is in the DLL, not
// even a string, so its functions were dropped by /OPT:REF or compiled out on PC (docs/ls3df-files.md). The name is a
// guess that sorts between its neighbours', clipping.cpp and I3d_animation.cpp.
#include "I3D/I3D.h"
