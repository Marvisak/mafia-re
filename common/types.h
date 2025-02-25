#pragma once

#define EXPORT __declspec(dllexport) __stdcall
#define EXPORT_CDECL __declspec(dllexport)

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned __int64 ulong;