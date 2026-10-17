#pragma once

#define EXPORT __declspec(dllexport) __stdcall
#define EXPORT_CDECL __declspec(dllexport)

typedef unsigned char uchar;
typedef unsigned short ushort;
// unsigned int, as the PS2 build's mangled names have it (Ui; its one Ul is in a Sony library). On PC unsigned int and
// unsigned long compile the same, and no exported name has an integer parameter that would tell them apart.
typedef unsigned int uint;
// 64 bits, as long is in the PS2 build (the EE's long is 64-bit)
typedef unsigned __int64 ulong;