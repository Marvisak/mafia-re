#include "dta_read.h"
#include "dta.h"

// Opens the dta file and saves it
C_rw_data_inter::C_rw_data_inter(char const *file_name, bool &is_valid)
{
    is_valid = false;
    this->ref_count = 0;
    this->file = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
    if (this->file == INVALID_HANDLE_VALUE)
    {
        return;
    }

    uint buffer;
    DWORD bytes_read;
    if (!ReadFile(this->file, &buffer, 4, &bytes_read, NULL) || bytes_read != 4)
    {
        return;
    }

    if (buffer == '0DSI')
    {
        S_HANDLE handle;
        handle.file = this->file;
        handle.key1 = 0;
        handle.key2 = 0;
        strcpy(handle.file_name, file_name);
        handle.file_table.clear();

        v_handle.push_back(handle);
        this->AddRef();
        C_r_f.AddObject();
        is_valid = true;
    }
}

C_rw_data_inter::~C_rw_data_inter()
{
    if (this->file != INVALID_HANDLE_VALUE)
    {
        CloseHandle(this->file);
    }

    for (int i = 0; i < v_handle.size(); i++)
    {
        if (this->file == v_handle[i].file)
        {
            v_handle[i].file_table.clear();
            v_handle.erase(v_handle.begin() + i);
            break;
        }
    }

    for (int j = 0; j < v_file_h.size(); j++)
    {
        S_FILEHANDLE &file_handle = v_file_h[j];
        if (this->file == file_handle.file)
        {
            if (file_handle.tmp_file_name)
            {
                delete file_handle.tmp_file_name;
            }

            if (file_handle.block_content)
            {
                delete file_handle.block_content;
            }

            if (file_handle.wav_header)
            {
                delete file_handle.wav_header;
            }

            file_handle.wav_header = NULL;
            file_handle.block_content = NULL;
            file_handle.is_block_in_memory = false;
            file_handle.file = INVALID_HANDLE_VALUE;

            break;
        }
    }

    C_r_f.ReleaseObject();
}

int C_rw_data_inter::AddRef()
{
    C_m_p.Lock();
    C_r_f.AddRef();
    C_m_p.Unlock();
    return ++this->ref_count;
}

int C_rw_data_inter::Release()
{
    C_m_p.Lock();
    C_r_f.Release();
    uint refs = this->ref_count - 1;
    if (refs < 1)
    {
        delete this;
    }
    C_m_p.Unlock();
    return refs;
}

// Decrypts and reads the header files of the archive, needs 2 keys which are hardcoded in the Game.exe
bool C_rw_data_inter::UnlockPack(uint key1, uint key2)
{
    C_m_p.Lock();

    uint xor_key1 = key1 ^ 0x39475694;
    uint xor_key2 = key2 ^ 0x34985762;
    if (SetFilePointer(this->file, 4, 0, FILE_BEGIN) == INVALID_FILE_SIZE && GetLastError())
    {
        C_m_p.Unlock();
        return false;
    }

    S_DTAHEADER header;
    DWORD bytes_read;
    if (!ReadFile(this->file, &header, sizeof(S_DTAHEADER), &bytes_read, NULL) || bytes_read != sizeof(S_DTAHEADER))
    {
        C_m_p.Unlock();
        return false;
    }

    DeCrypt(reinterpret_cast<uchar *>(&header), sizeof(S_DTAHEADER), xor_key1, xor_key2);
    for (int i = 0; i < v_handle.size(); i++)
    {
        S_HANDLE &handle = v_handle[i];
        if (handle.file == this->file)
        {
            handle.key1 = xor_key1;
            handle.key2 = xor_key2;

            handle.file_table.resize(header.file_table_size / sizeof(S_FILES_IN));

            SetFilePointer(this->file, header.file_table_offset, 0, 0);
            ReadFile(this->file, &handle.file_table[0], header.file_table_size, &bytes_read, NULL);
            DeCrypt(reinterpret_cast<uchar *>(&handle.file_table[0]), header.file_table_size, xor_key1, xor_key2);

            C_m_p.Unlock();
            return true;
        }
    }
    C_m_p.Unlock();
    return false;
}
