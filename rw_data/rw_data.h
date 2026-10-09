#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <string.h>
#include <vector>
#include <types.h>

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif

// The typo is in the original (PS2 symbol __ct__25C_multithrerad_protectionFv)
class C_multithrerad_protection
{
public:
    CRITICAL_SECTION section;

    C_multithrerad_protection();
    ~C_multithrerad_protection();

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
class C_ref_counter
{
public:
    C_ref_counter();
    ~C_ref_counter();

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

struct S_DTAHEADER
{
    uint file_count;
    uint file_table_offset;
    uint file_table_size;
    uint unk;
};

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
    char file_name[16];
};

struct S_FILEHEADER
{
    uint unk1;
    uint unk2;
    ulong timestamp;
    uint size;
    uint block_count;
    uint name_length_flags; // low 15 bits = name length, bit 15 = encrypted
    uint unk3;
};

struct S_HANDLE
{
    HANDLE file; // 4 bytes of padding follow, key is 8 byte aligned
    ulong key;
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
    ulong key;
    bool is_block_in_memory;
    bool encrypted;
    char *tmp_file_name;
    uchar *block_content;
}; // sizeof = 0x58

#define DTA_FROM_FILESYSTEM 0x1
#define DTA_WRITE 0x2
#define DTA_BUFFER 0x4
#define DTA_FAILED_WRITING_TO_BUFFER 0x80

#define DTA_TEMPORARY 0x1
#define DTA_FILE_BUFFER 0x100
#define DTA_CREATE_FILE 0x10000
#define DTA_CHANGE_ATTRIBUTES 0x100000

// The vtable of the base class has 4 entries, 3 of them are _purecall
// VTABLE: RWDATA 0x1000b108
class C_rw_data
{
public:
    // FUNCTION: RWDATA 0x10001330
    virtual ~C_rw_data() {}

    virtual int AddRef() = 0;
    virtual int Release() = 0;
    virtual bool UnlockPack(ulong key) = 0;
};

// SYNTHETIC: RWDATA 0x10001340
// C_rw_data::`scalar deleting destructor'

// VTABLE: RWDATA 0x1000b0f8
class C_rw_data_inter : public C_rw_data
{
    virtual ~C_rw_data_inter();

public:
    C_rw_data_inter(const char *file_name, bool &is_valid);

    virtual int AddRef();
    virtual int Release();
    // The key is a single 64 bit value (PS2: UnlockPack__15C_rw_data_interFl), Game.exe pushes it as two dwords
    virtual bool UnlockPack(ulong key);

private:
    int ref_count;
    HANDLE file;
};
typedef class C_rw_data_inter *LPC_rw_data_inter;

// SYNTHETIC: RWDATA 0x10001360
// C_rw_data_inter::`scalar deleting destructor'

extern "C"
{
    LPC_rw_data_inter EXPORT dtaCreate(const char *file_name);
    void EXPORT dtaSetDtaFirstForce();
    int EXPORT dtaOpen(const char *file_name, bool force_dta);
    int EXPORT dtaOpenWrite(const char *file_name, uint flags);
    uint EXPORT dtaRead(int handle, uchar *buffer, uint len);
    int EXPORT dtaWrite(int handle, const uchar *buffer, uint len);
    int EXPORT dtaSeek(int handle, int distance_to_move, int move_method);
    bool EXPORT dtaClose(int handle);
    bool EXPORT dtaDelete(const char *file_name);
    bool EXPORT dtaGetTime(int handle, ulong *time, bool force_check_from_file);
    // Neither of these is imported by Game.exe, but they are part of the export table
    uint EXPORT dtaBin2Text(char *buffer, uint len);
    void EXPORT dtaDumpMemoryLeaks(bool dump);
}

// The STL code the compiler emitted into rw_data.cpp. The original orders it differently from
// the rebuild, the order follows the first use of each instantiation in the source.

// TEMPLATE: RWDATA 0x10004730
// ?size@?$vector@US_FILEHANDLE@@V?$allocator@US_FILEHANDLE@@@std@@@std@@QBEIXZ

// TEMPLATE: RWDATA 0x10004760
// ?insert@?$vector@IV?$allocator@I@std@@@std@@QAEXPAIIABI@Z

// TEMPLATE: RWDATA 0x10004970
// ?insert@?$vector@US_FILES_IN@@V?$allocator@US_FILES_IN@@@std@@@std@@QAEXPAUS_FILES_IN@@IABU3@@Z

// Out-of-line in the original (with std::copy inlined), inlined in the rebuild
// TEMPLATE: RWDATA 0x10004c90
// ?erase@?$vector@US_FILES_IN@@V?$allocator@US_FILES_IN@@@std@@@std@@QAEPAUS_FILES_IN@@PAU3@0@Z

// TEMPLATE: RWDATA 0x10004ce0
// ?insert@?$vector@US_FILEHANDLE@@V?$allocator@US_FILEHANDLE@@@std@@@std@@QAEPAUS_FILEHANDLE@@PAU3@ABU3@@Z

// Folded with vector<S_FILES_IN>::_Destroy
// TEMPLATE: RWDATA 0x10004ef0
// ?_Destroy@?$vector@US_FILEHANDLE@@V?$allocator@US_FILEHANDLE@@@std@@@std@@IAEXPAUS_FILEHANDLE@@0@Z

// Out-of-line in the original, inlined in the rebuild
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

// Placement delete from <new>
// LIBRARY: RWDATA 0x10005740
// ??3@YAXPAX0@Z

// TEMPLATE: RWDATA 0x100059f0
// ?_Ucopy@?$vector@US_FILES_IN@@V?$allocator@US_FILES_IN@@@std@@@std@@IAEPAUS_FILES_IN@@PBU3@0PAU3@@Z

// TEMPLATE: RWDATA 0x10005a30
// ?_Construct@std@@YGXPAUS_FILES_IN@@ABU2@@Z

// TEMPLATE: RWDATA 0x10005a50
// ??4?$vector@US_FILES_IN@@V?$allocator@US_FILES_IN@@@std@@@std@@QAEAAV01@ABV01@@Z
