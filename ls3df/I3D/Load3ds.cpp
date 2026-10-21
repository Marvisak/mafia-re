// A file of which only the engine header's statics are left, at 0x10060c20-0x10060c40 in the original: the header's
// zero vector's $E, the vector (0x101c3bd8, between Load.cpp's .bss and load4ds.cpp's) and the identity matrix
// (0x1009d298). Nothing else of it is in the DLL, not even a string, so its functions were dropped by /OPT:REF or
// compiled out on PC (docs/ls3df-files.md). The name is a guess: the file sorts between Load.cpp and load4ds.cpp, so it
// starts with Load.
#include "I3D/I3D.h"
