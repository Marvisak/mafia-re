#pragma once

// rw_data.dll's interface: the archives and the files read and written through them, which the engine and the game
// use. The DLL's own types are rw_data_inter.h's. This header includes nothing but types.h: windows.h and the STL would
// change what the engine's files compile to (VC6's <vector> brings <new>, whose operator delete is throw(), which
// changes their EH states).

#include <types.h>

struct _FILETIME;

// dtaOpenWrite's flags
#define DTA_TEMPORARY 0x1
#define DTA_FILE_BUFFER 0x100
#define DTA_CREATE_FILE 0x10000
#define DTA_CHANGE_ATTRIBUTES 0x100000

// What dtaOpen and dtaOpenWrite return when they fail (the name is a guess)
#define DTA_INVALID_HANDLE -1

// An archive dtaCreate opened, C_rw_data_inter. The vtable of the base class has 4 entries, 3 of them are _purecall.
// VTABLE: RWDATA 0x1000b108
class C_rw_data
{
public:
    // FUNCTION: RWDATA 0x10001330
    virtual ~C_rw_data() {}

    virtual int AddRef() = 0;
    virtual int Release() = 0;
    // Decrypts the archive's file table. The key is a single signed 64 bit value (PS2:
    // UnlockPack__15C_rw_data_interFl, long is 64 bit on the EE), Game.exe pushes it as two dwords.
    virtual bool UnlockPack(__int64 key) = 0;
};
// A typedef for the return type: VC6 ignores a __declspec(dllexport) after a '*' (C4518)
typedef C_rw_data *LPC_rw_data;

// SYNTHETIC: RWDATA 0x10001340
// C_rw_data::`scalar deleting destructor'

// The engine and the game call these through the import library's thunks, so they aren't declared
// __declspec(dllimport)
extern "C"
{
    LPC_rw_data EXPORT dtaCreate(const char *file_name);
    // dtaOpen looks into the archives before the folders
    void EXPORT dtaSetDtaFirstForce();
    int EXPORT dtaOpen(const char *file_name, bool force_dta);
    int EXPORT dtaOpenWrite(const char *file_name, uint flags);
    uint EXPORT dtaRead(int handle, void *buffer, uint len);
    int EXPORT dtaWrite(int handle, const void *buffer, uint len);
    // move_method is SetFilePointer's: FILE_BEGIN, FILE_CURRENT or FILE_END (SEEK_SET, SEEK_CUR and SEEK_END have the
    // same values)
    int EXPORT dtaSeek(int handle, int distance_to_move, int move_method);
    bool EXPORT dtaClose(int handle);
    bool EXPORT dtaDelete(const char *file_name);
    bool EXPORT dtaGetTime(int handle, struct _FILETIME *time, bool force_check_from_file);
    // Neither of these is imported by Game.exe, but they are part of the export table
    uint EXPORT dtaBin2Text(char *buffer, uint len);
    void EXPORT dtaDumpMemoryLeaks(bool dump);
}

// The engine and the game link the import library as a default library: their thunks of it follow the CRT's code
// (LS3DF's after LIBCMT's, Game.exe's after the C++ library's). CMake defines rw_data_EXPORTS for the DLL's own build,
// which must not ask for it.
#ifndef rw_data_EXPORTS
#pragma comment(lib, "rw_data.lib")
#endif
