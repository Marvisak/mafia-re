#pragma once

// The functions of rw_data.dll the engine reads its files through (see rw_data/rw_data.h). The engine calls them
// through the import library's thunks, so they aren't declared __declspec(dllimport).

#include <types.h>

struct _FILETIME;

extern "C" {
int __stdcall dtaOpen(const char *file_name, bool force_dta);
int __stdcall dtaOpenWrite(const char *file_name, uint flags);
uint __stdcall dtaRead(int handle, void *buffer, uint len);
int __stdcall dtaWrite(int handle, const void *buffer, uint len);
int __stdcall dtaSeek(int handle, int distance_to_move, int move_method);
bool __stdcall dtaClose(int handle);
bool __stdcall dtaDelete(const char *file_name);
bool __stdcall dtaGetTime(int handle, struct _FILETIME *time, bool force_check_from_file);
}

// The original's thunks follow the CRT's code: the import library came after LIBCMT.LIB, as a default library does
#pragma comment(lib, "rw_data.lib")
