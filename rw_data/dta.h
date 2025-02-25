#pragma once

#include "main.h"

#define DTA_FROM_FILESYSTEM 0x1
#define DTA_WRITE 0x2
#define DTA_BUFFER 0x4
#define DTA_FAILED_WRITING_TO_BUFFER 0x80

#define DTA_TEMPORARY 0x1
#define DTA_FILE_BUFFER 0x100
#define DTA_CREATE_FILE 0x10000
#define DTA_CHANGE_ATTRIBUTES 0x100000

class C_rw_data
{
protected:
    virtual ~C_rw_data() {};
};

class C_rw_data_inter : public C_rw_data
{
    virtual ~C_rw_data_inter();

public:
    C_rw_data_inter(char const* file_name, bool& is_valid);

    // In H&D there are smart pointers, but I cannot be really sure at this point if they are used even here
    virtual int AddRef();
    virtual int Release();
    virtual bool UnlockPack(uint key1, uint key2);

private:
    int ref_count;
    HANDLE file;
};
typedef class C_rw_data_inter* LPC_rw_data_inter;

// There is also dtaBin2Text, but that function isn't even imported by the main executable so it's utterly useless
extern "C"
{
    LPC_rw_data_inter EXPORT dtaCreate(const char *file_name);
    void EXPORT dtaSetDtaFirstForce();
    int EXPORT dtaOpen(const char *file_name, bool force_dta);
    int EXPORT dtaOpenWrite(const char *file_name, uint flags);
    uint EXPORT dtaRead(int handle, uchar *buffer, uint byte_count);
    int EXPORT dtaWrite(int handle, const uchar *buffer, uint len);
    int EXPORT dtaSeek(int handle, int distance_to_move, int move_method);
    bool EXPORT dtaClose(int handle);
    bool EXPORT dtaDelete(const char *file_name);
    bool EXPORT dtaGetTime(int handle, ulong *time, bool force_check_from_file);
}
