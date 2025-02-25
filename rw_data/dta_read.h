#pragma once

#include "main.h"

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
    S_WAVHEADER *wav_header;
    uint block_size;
    int unk;
    uint key1;
    uint key2;
    bool is_block_in_memory;
    bool encrypted;
    ushort unk2;
    char *tmp_file_name;
    uchar *block_content;
    int unk3;
};

class C_rw_data
{
protected:
    virtual ~C_rw_data() {};
};

class C_rw_data_inter : public C_rw_data
{
    virtual ~C_rw_data_inter();

public:
    C_rw_data_inter(char const *file_name, bool &is_valid);

    // In H&D there are smart pointers, but I cannot be really sure at this point if they are used even here
    virtual int AddRef();
    virtual int Release();
    virtual bool UnlockPack(uint key1, uint key2);

private:
    int ref_count;
    HANDLE file;
};
typedef class C_rw_data_inter *LPC_rw_data_inter;
