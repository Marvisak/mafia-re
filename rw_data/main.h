#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <vector>
#include <types.h>

class C_multithread_protection
{
public:
    CRITICAL_SECTION section;

    C_multithread_protection();
    ~C_multithread_protection();

    void Lock()
    {
        EnterCriticalSection(&section);
    }

    void Unlock()
    {
        LeaveCriticalSection(&section);
    }
};

class C_ref_counter
{
public:
    C_ref_counter();
    ~C_ref_counter();

    void AddRef()
    {
        this->ref_count++;
    }

    void Release()
    {
        --this->ref_count;
    }

    void AddObject()
    {
        ++this->obj_count;
    }

    void ReleaseObject()
    {
        --this->obj_count;
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

struct S_FILES_IN
{
    ushort file_name_check_sum;
    ushort file_name_length;
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
    ulong name_length_flags;
};

struct S_HANDLE
{
    HANDLE file;
    int unk;
    uint key1;
    uint key2;
    char file_name[264];
    std::vector<S_FILES_IN> file_table;
};

struct S_WAVHEADER
{
    uint riff_chunk_id;
    uint riff_chunk_size;
    uint riff_type;

    uint format_chunk_id;
    uint format_chunk_size2;
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
    S_WAVHEADER* wav_header;
    uint block_size;
    int unk;
    uint key1;
    uint key2;
    bool is_block_in_memory;
    bool encrypted;
    ushort unk2;
    char* tmp_file_name;
    uchar* block_content;
    int unk3;
};


extern std::vector<S_FILEHANDLE> v_file_h;
extern std::vector<S_HANDLE> v_handle;
extern C_multithread_protection C_m_p;
extern C_ref_counter C_r_f;
