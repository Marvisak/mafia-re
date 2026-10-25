#pragma once

// rw_data.dll's own types, which only rw_data.cpp sees: the archive behind C_rw_data, the open files and the archive's
// headers. The interface is rw_data.h's.

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <string.h>
#include <vector>

#include "rw_data.h"

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

// The typo is in the original (PS2 symbol __ct__25C_multithrerad_protectionFv)
class C_multithrerad_protection
{
public:
    CRITICAL_SECTION section;

    C_multithrerad_protection();
    ~C_multithrerad_protection();

    // Guessed names: the PS2 build inlines both, the Xbox build has no names. H&D Deluxe's source (the Insanity engine,
    // whose dta reader has no locking at all) has no such wrapper: its network code, base library and log class call
    // the critical-section functions directly, as a few exits here do (dtaWrite, dtaOpenWrite: their comments say why).
    void Lock()
    {
        EnterCriticalSection(&section);
    }

    void Unlock()
    {
        LeaveCriticalSection(&section);
    }
};

// This class doesn't exist in the PS2 build, so the class and member names are guesses
class C_leak_detector
{
public:
    C_leak_detector();
    ~C_leak_detector();

    void AddRef()
    {
        ++ref_count;
    }

    void Release()
    {
        --ref_count;
    }

    void AddObject()
    {
        ++obj_count;
    }

    void ReleaseObject()
    {
        --obj_count;
    }

    void SetDumpMemoryLeaks(bool dump)
    {
        dump_memory_leaks = dump;
    }

private:
    int ref_count;
    int obj_count;
    bool dump_memory_leaks;
};

// An archive's first dword, "ISD0"
#define DTA_MAGIC 0x30445349
// UnlockPack decrypts the file table with the game's key xored with this
#define DTA_KEY_MASK (((ulong)0x34985762 << 32) | 0x39475694)

struct S_DTAHEADER
{
    uint file_count;
    uint file_table_offset;
    uint file_table_size;
    uint unk;
};

// How much of each file's name the file table keeps: its last characters (OpenFromDTA compares those, then the full
// name in the file's header)
#define DTA_TABLE_NAME_LENGTH 16

// The file table is sorted by hash, OpenFromDTA does a binary search on the whole 32 bit value
struct S_FILES_IN
{
    union
    {
        uint hash;
        struct
        {
            ushort file_name_check_sum;
            ushort file_name_length;
        };
    };
    uint header_offset;
    uint data_offset;
    char file_name[DTA_TABLE_NAME_LENGTH];
};

struct S_FILEHEADER
{
    uint unk1;
    uint unk2;
    ulong timestamp;
    uint size;
    uint block_count;
    uint name_length_flags; // DTA_NAME_LENGTH_MASK, DTA_ENCRYPTED
    uint unk3;
};

// S_FILEHEADER::name_length_flags
#define DTA_NAME_LENGTH_MASK 0x7FFF
#define DTA_ENCRYPTED 0x8000            // the file's blocks are encrypted with the archive's key

// A file in an archive is stored in blocks of DTA_BLOCK_SIZE bytes (the last one shorter), each after a dword that
// holds its compressed size in the low word and the previous block's in the high word, and each starting with its
// type byte. The names are guesses.
#define DTA_BLOCK_SIZE 0x8000
#define DTA_BLOCK_SHIFT 15              // 1 << DTA_BLOCK_SHIFT == DTA_BLOCK_SIZE
#define DTA_BLOCK_HEADER_SIZE 4         // the dword with the sizes
// The block types; any other is stored as it is
#define DTA_BLOCK_COMPRESSED 1          // Decompression
#define DTA_BLOCK_WAV 8                 // and up, in steps of 4 to 32: DPCM sound with a WAV header first (WavDecompression)

struct S_HANDLE
{
    HANDLE file; // 4 bytes of padding follow, key is 8 byte aligned
    __int64 key;
    char file_name[MAX_PATH];
    std::vector<S_FILES_IN> file_table;
}; // sizeof = 0x128

// SYNTHETIC: RWDATA 0x10001380
// S_HANDLE::~S_HANDLE

// SYNTHETIC: RWDATA 0x10005750
// S_HANDLE::operator=

struct S_WAVHEADER
{
    uint riff_chunk_id;
    uint riff_chunk_size;
    uint riff_type;

    uint format_chunk_id;
    uint format_chunk_size;
    ushort format_tag;
    ushort channel_count;
    uint samples_per_second;
    uint bytes_per_second;
    ushort block_align;
    ushort bits_per_sample;

    uint data_chunk_id;
    uint data_chunk_size;
};

struct S_FILEHANDLE
{
    HANDLE file;
    uint flags;
    ulong timestamp;
    uint size;
    uint data_offset;
    uint block_start_file_pointer;
    uint file_pointer;
    uint block_file_pointer;
    uint block_count;
    uint current_block;
    uint compressed_block_size;
    uint previous_compressed_block_size;
    S_WAVHEADER *wav_header;
    uint block_size; // 4 bytes of padding follow, key is 8 byte aligned
    __int64 key;
    bool is_block_in_memory;
    bool encrypted;
    char *tmp_file_name;
    uchar *block_content;
}; // sizeof = 0x58

// S_FILEHANDLE::flags
#define DTA_FROM_FILESYSTEM 0x1
#define DTA_WRITE 0x2
#define DTA_BUFFER 0x4
#define DTA_FAILED_WRITING_TO_BUFFER 0x80

// VTABLE: RWDATA 0x1000b0f8
class C_rw_data_inter : public C_rw_data
{
    virtual ~C_rw_data_inter();

public:
    C_rw_data_inter(const char *file_name, bool &is_valid);

    virtual int AddRef();
    virtual int Release();
    virtual bool UnlockPack(__int64 key);

private:
    uint ref_count; // Release compares it unsigned (cmp 1, jae)
    HANDLE file;
};

// SYNTHETIC: RWDATA 0x10001360
// C_rw_data_inter::`scalar deleting destructor'

// Dynamic initializers of the globals in rw_data.cpp, one group of four per global, in source order:
// a thunk in the .CRT$XCU table (call constructor, jmp to the atexit registration), the constructor, the
// atexit registration and the destructor. reccmp matches the thunks and constructors itself through the
// table (as $CRT_CPP_Initializer(n)), so only the other two are annotated.

// v_file_h
// SYNTHETIC: RWDATA 0x10001030
// $E4

// SYNTHETIC: RWDATA 0x10001040
// $E3

// v_handle
// SYNTHETIC: RWDATA 0x10001090
// $E9

// SYNTHETIC: RWDATA 0x100010a0
// $E8

// C_m_p
// SYNTHETIC: RWDATA 0x10001110
// $E14

// SYNTHETIC: RWDATA 0x10001120
// $E13

// C_l_d
// SYNTHETIC: RWDATA 0x10001150
// $E19

// SYNTHETIC: RWDATA 0x10001160
// $E18

// The STL code the compiler emitted into rw_data.cpp, in the order VC6 instantiates it: in rounds,
// first what the source calls directly, then what those functions call, and so on; within a round by class and
// member declaration order.

// TEMPLATE: RWDATA 0x10004730
// ?size@?$vector@US_FILEHANDLE@@V?$allocator@US_FILEHANDLE@@@std@@@std@@QBEIXZ

// TEMPLATE: RWDATA 0x10004760
// ?insert@?$vector@IV?$allocator@I@std@@@std@@QAEXPAIIABI@Z

// TEMPLATE: RWDATA 0x10004970
// ?insert@?$vector@US_FILES_IN@@V?$allocator@US_FILES_IN@@@std@@@std@@QAEXPAUS_FILES_IN@@IABU3@@Z

// Out of line (with std::copy inlined), called from the resize inlined into UnlockPack and ~C_rw_data_inter.
// Our build only emits it out of line since UnlockPack indexes v_handle directly.
// TEMPLATE: RWDATA 0x10004c90
// ?erase@?$vector@US_FILES_IN@@V?$allocator@US_FILES_IN@@@std@@@std@@QAEPAUS_FILES_IN@@PAU3@0@Z

// TEMPLATE: RWDATA 0x10004ce0
// ?insert@?$vector@US_FILEHANDLE@@V?$allocator@US_FILEHANDLE@@@std@@@std@@QAEPAUS_FILEHANDLE@@PAU3@ABU3@@Z

// Folded with vector<S_FILES_IN>::_Destroy
// TEMPLATE: RWDATA 0x10004ef0
// ?_Destroy@?$vector@US_FILEHANDLE@@V?$allocator@US_FILEHANDLE@@@std@@@std@@IAEXPAUS_FILEHANDLE@@0@Z

// Out of line, called from the v_handle.erase inlined into ~C_rw_data_inter. Our build only emits it out of
// line since ~C_rw_data_inter indexes v_file_h directly.
// TEMPLATE: RWDATA 0x10004f00
// ?_Destroy@?$vector@US_HANDLE@@V?$allocator@US_HANDLE@@@std@@@std@@IAEXPAUS_HANDLE@@0@Z

// TEMPLATE: RWDATA 0x10004f30
// ?insert@?$vector@US_FILEHANDLE@@V?$allocator@US_FILEHANDLE@@@std@@@std@@QAEXPAUS_FILEHANDLE@@IABU3@@Z

// TEMPLATE: RWDATA 0x10005230
// ?insert@?$vector@US_HANDLE@@V?$allocator@US_HANDLE@@@std@@@std@@QAEXPAUS_HANDLE@@IABU3@@Z

// TEMPLATE: RWDATA 0x100055c0
// ?_Ucopy@?$vector@US_FILEHANDLE@@V?$allocator@US_FILEHANDLE@@@std@@@std@@IAEPAUS_FILEHANDLE@@PBU3@0PAU3@@Z

// TEMPLATE: RWDATA 0x10005600
// ?_Ufill@?$vector@US_FILEHANDLE@@V?$allocator@US_FILEHANDLE@@@std@@@std@@IAEXPAUS_FILEHANDLE@@IABU3@@Z

// TEMPLATE: RWDATA 0x10005630
// ?_Construct@std@@YGXPAUS_FILEHANDLE@@ABU2@@Z

// TEMPLATE: RWDATA 0x10005650
// ?_Construct@std@@YGXPAUS_HANDLE@@ABU2@@Z

// Placement delete, <new>'s inline one: the unwinding of the STL's placement news calls it (the funclet at 0x1000a880),
// so rw_data.cpp emits it out of line among the templates. No library has an object of it, so it is this file's code.
// FUNCTION: RWDATA 0x10005740
// ??3@YAXPAX0@Z

// TEMPLATE: RWDATA 0x100059f0
// ?_Ucopy@?$vector@US_FILES_IN@@V?$allocator@US_FILES_IN@@@std@@@std@@IAEPAUS_FILES_IN@@PBU3@0PAU3@@Z

// TEMPLATE: RWDATA 0x10005a30
// ?_Construct@std@@YGXPAUS_FILES_IN@@ABU2@@Z

// TEMPLATE: RWDATA 0x10005a50
// ??4?$vector@US_FILES_IN@@V?$allocator@US_FILES_IN@@@std@@@std@@QAEAAV01@ABV01@@Z
