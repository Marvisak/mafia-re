#include "dta.h"

#include "dta_ops.h"

bool dta_first_force = false;
std::vector<struct S_FILEHANDLE> v_file_h;
std::vector<struct S_HANDLE> v_handle;
C_multithread_protection C_m_p;
C_ref_counter C_r_f;

static int OpenFromDTA(char const *file_name)
{
    char file_name_upper[260];
    strcpy(file_name_upper, file_name);
    strupr(file_name_upper);

    ushort checksum = 0;
    int len = strlen(file_name) + 1;
    do
    {
        checksum += static_cast<uchar>(file_name_upper[--len]);
    } while (len);
    checksum = checksum | strlen(file_name) << 16;

    if (v_handle.empty())
        return -1;

    for (int i = 0; i < v_handle.size(); i++)
    {
        S_HANDLE handle = v_handle[i];
        for (int j = 0; j < handle.file_table.size(); j++)
        {
            S_FILES_IN &field = v_handle[i].file_table[j];
            if (checksum != field.file_name_check_sum)
            {
                continue;
            }

            int str_index = 0;
            if (field.file_name_length > 16)
            {
                str_index = field.file_name_length - 16;
            }

            if (strncmp(field.file_name, &file_name_upper[str_index], 16) != 0)
            {
                continue;
            }

            if (SetFilePointer(handle.file, field.header_offset, NULL, FILE_BEGIN) == INVALID_FILE_SIZE && GetLastError())
            {
                return -1;
            }

            DWORD bytes_read;
            S_FILEHEADER file_header;
            if (!ReadFile(handle.file, &file_header, sizeof(S_FILEHEADER), &bytes_read, NULL) || bytes_read != sizeof(S_FILEHEADER))
            {
                return -1;
            }
            DeCrypt(reinterpret_cast<uchar *>(&file_header), sizeof(S_FILEHEADER), handle.key1, handle.key2);

            char full_file_name[260];
            if (!ReadFile(handle.file, full_file_name, file_header.name_length_flags & 0x7FFF, &bytes_read, NULL) || bytes_read != (file_header.name_length_flags & 0x7FFF))
            {
                return -1;
            }
            DeCrypt(reinterpret_cast<uchar *>(full_file_name), file_header.name_length_flags & 0x7FFF, handle.key1, handle.key2);

            full_file_name[bytes_read] = '\0';
            _strupr(full_file_name);
            if (strcmp(full_file_name, file_name_upper) == 0)
            {
                S_FILEHANDLE file_handle = {0};

                file_handle.tmp_file_name = NULL;
                file_handle.wav_header = NULL;
                file_handle.is_block_in_memory = false;
                file_handle.size = file_header.size;
                file_handle.file = handle.file;
                file_handle.data_offset = field.data_offset;
                file_handle.block_count = file_header.block_count;
                file_handle.timestamp = file_header.timestamp;
                file_handle.current_block = 0;
                file_handle.flags = 0;
                file_handle.block_size = min(0x8000, file_header.size);

                file_handle.key1 = handle.key1;
                file_handle.key2 = handle.key2;
                file_handle.encrypted = (file_header.name_length_flags & 0x8000) != 0;

                if (!ReadFile(handle.file, &file_handle.compressed_block_size, 4, &bytes_read, NULL) || bytes_read != 4)
                {
                    return -1;
                }

                file_handle.compressed_block_size &= 0xFFFF;
                file_handle.previous_compressed_block_size = 0;

                uchar buffer[1 + sizeof(S_WAVHEADER)];
                if (!ReadFile(handle.file, buffer, 1, &bytes_read, NULL) || bytes_read != 1)
                {
                    return -1;
                }
                if (file_handle.encrypted)
                {
                    DeCrypt(buffer, 1, file_handle.key1, file_handle.key2);
                }

                if (buffer[0] >= 8)
                {
                    file_handle.wav_header = new S_WAVHEADER();

                    if (ReadFile(file_handle.file, buffer + 1, sizeof(S_WAVHEADER), &bytes_read, NULL) && bytes_read == sizeof(S_WAVHEADER))
                    {
                        if (file_handle.encrypted)
                        {
                            DeCrypt(buffer, 1 + sizeof(S_WAVHEADER), file_handle.key1, file_handle.key2);
                        }
                        else
                        {
                            // It seems like the wav header is always encrypted no matter if the rest of the archive is
                            DeCrypt(buffer + 1, sizeof(S_WAVHEADER), file_handle.key1, file_handle.key2);
                        }
                        memcpy(file_handle.wav_header, buffer + 1, sizeof(S_WAVHEADER));
                    }
                }

                file_handle.block_content = new uchar[0x8000];

                v_file_h.push_back(file_handle);

                return v_file_h.size() - 1;
            }
        }
    }
    return -1;
}

static int OpenFromFileSystem(const char *file_name)
{
    S_FILEHANDLE file_handle = {0};

    HANDLE file = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    file_handle.file = file;
    if (file == INVALID_HANDLE_VALUE)
    {
        return -1;
    }

    uint size = SetFilePointer(file, 0, NULL, FILE_END);
    file_handle.size = size;
    if (size == INVALID_FILE_SIZE && GetLastError())
    {
        return -1;
    }

    file_handle.is_block_in_memory = false;
    file_handle.block_count = 1;
    file_handle.current_block = 0;
    file_handle.flags = DTA_FROM_FILESYSTEM;
    file_handle.wav_header = NULL;
    file_handle.block_size = min(0x8000, size);
    file_handle.block_content = new uchar[0x8000];
    file_handle.compressed_block_size = 0;
    file_handle.previous_compressed_block_size = 0;
    file_handle.tmp_file_name = NULL;
    v_file_h.push_back(file_handle);

    return v_file_h.size() - 1;
}

LPC_rw_data_inter EXPORT dtaCreate(const char *file_name)
{
    C_m_p.Lock();
    bool is_valid_dta_file = false;
    C_rw_data_inter *rw_data_inter = new C_rw_data_inter(file_name, is_valid_dta_file);
    if (rw_data_inter && is_valid_dta_file)
    {
        C_m_p.Unlock();
        return rw_data_inter;
    }

    C_m_p.Unlock();
    if (rw_data_inter)
    {
        rw_data_inter->Release();
    }
    return NULL;
}

// The binary seems like it was patched in order to remove this function (there are some instruction after retn, that enable dta_first_force)
void EXPORT dtaSetDtaFirstForce()
{
}

int EXPORT dtaOpen(const char *file_name, bool force_dta)
{
    C_m_p.Lock();
    int handle = -1;
    if (force_dta || dta_first_force)
    {
        handle = OpenFromDTA(file_name);
        if (handle == -1)
        {
            handle = OpenFromFileSystem(file_name);
        }
    }
    else
    {
        handle = OpenFromFileSystem(file_name);
        if (handle == -1)
        {
            handle = OpenFromDTA(file_name);
        }
    }

    C_m_p.Unlock();
    return handle;
}

int EXPORT dtaOpenWrite(const char *file_name, uint flags)
{
    C_m_p.Lock();
    int file_flags = flags & DTA_TEMPORARY ? FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_HIDDEN : FILE_ATTRIBUTE_NORMAL;
    int disposition = flags & DTA_CREATE_FILE ? 2 : 4;

    HANDLE file;
    char tmp_file_name[512];
    if (flags & DTA_FILE_BUFFER)
    {
        strcpy(tmp_file_name, file_name);
        strcat(tmp_file_name, ".tmp");

        if (disposition == 4)
        {
            CopyFileA(file_name, tmp_file_name, false);
        }
        else
        {
            DeleteFileA(tmp_file_name);
        }

        file = CreateFileA(tmp_file_name, GENERIC_WRITE, NULL, NULL, disposition, file_flags, NULL);
        if (file == INVALID_HANDLE_VALUE)
        {
            if ((flags & DTA_CHANGE_ATTRIBUTES) == 0)
            {
                C_m_p.Unlock();
                return -1;
            }

            DWORD attributes = GetFileAttributesA(tmp_file_name);
            if (attributes == -1)
            {
                C_m_p.Unlock();
                return -1;
            }
            attributes &= -FILE_ATTRIBUTE_HIDDEN;
            SetFileAttributesA(tmp_file_name, attributes);
            file = CreateFileA(tmp_file_name, GENERIC_WRITE, NULL, NULL, disposition, file_flags, NULL);
            if (file == INVALID_HANDLE_VALUE)
            {
                C_m_p.Unlock();
                return -1;
            }
        }
    }
    else
    {
        file = CreateFileA(file_name, GENERIC_WRITE, NULL, NULL, disposition, file_flags, NULL);
        if (file == INVALID_HANDLE_VALUE)
        {
            if ((flags & DTA_CHANGE_ATTRIBUTES) == 0)
            {
                C_m_p.Unlock();
                return -1;
            }

            DWORD attributes = GetFileAttributesA(file_name);
            if (attributes == -1)
            {
                C_m_p.Unlock();
                return -1;
            }
            attributes &= -FILE_ATTRIBUTE_HIDDEN;
            SetFileAttributesA(file_name, attributes);
            file = CreateFileA(file_name, GENERIC_WRITE, NULL, NULL, disposition, file_flags, NULL);
            if (file == INVALID_HANDLE_VALUE)
            {
                C_m_p.Unlock();
                return -1;
            }
        }
    }

    S_FILEHANDLE file_handle = {0};
    file_handle.file = file;
    file_handle.is_block_in_memory = false;
    file_handle.block_count = 0;
    file_handle.current_block = 0;
    file_handle.flags = DTA_FROM_FILESYSTEM | DTA_WRITE;
    file_handle.tmp_file_name = NULL;
    if (flags & DTA_FILE_BUFFER)
    {
        file_handle.flags = DTA_FROM_FILESYSTEM | DTA_WRITE | DTA_BUFFER;
        file_handle.tmp_file_name = new char[strlen(tmp_file_name) + 1];
        if (!file_handle.tmp_file_name)
        {
            C_m_p.Unlock();
            return -1;
        }
        strcpy(file_handle.tmp_file_name, tmp_file_name);
    }
    file_handle.wav_header = NULL;
    file_handle.block_size = 0x8000;
    file_handle.block_content = new uchar[0x8000];
    if (!file_handle.block_content)
    {
        C_m_p.Unlock();
        return -1;
    }
    file_handle.compressed_block_size = 0;
    file_handle.previous_compressed_block_size = 0;

    v_file_h.push_back(file_handle);
    C_m_p.Unlock();
    return v_file_h.size() - 1;
}

// I have no idea how this monstrosity came to existence
// It seems like this function was patched like 80 different times and more and more features were piled up on it
// Maybe it's just because I am terrible at reversing, but I doubt the compiler would butcher it that much
uint EXPORT dtaRead(int handle, uchar *buffer, uint len)
{
    C_m_p.Lock();
    if (!buffer || v_file_h.size() <= handle)
    {
        C_m_p.Unlock();
        return -1;
    }

    uchar file_buffer[0x8001] = {0};

    S_FILEHANDLE &file_handle = v_file_h[handle];
    if (file_handle.file == INVALID_HANDLE_VALUE || (file_handle.flags & DTA_WRITE) != 0)
    {
        C_m_p.Unlock();
        return -1;
    }

    uint block_start_file_pointer = file_handle.block_start_file_pointer;
    uint file_pointer = file_handle.file_pointer;
    uint block_file_pointer = file_handle.block_file_pointer;
    uint current_block = file_handle.current_block;
    uint compressed_block_size = file_handle.compressed_block_size;
    uint previous_compressed_block_size = file_handle.previous_compressed_block_size;
    uint block_size = file_handle.block_size;

    if (len > file_handle.size - file_pointer)
    {
        len = file_handle.size - file_pointer;
    }

    DWORD bytes_read;
    if (file_handle.flags & DTA_FROM_FILESYSTEM)
    {
        if (!file_handle.is_block_in_memory || block_start_file_pointer > file_pointer || block_start_file_pointer + block_size < file_pointer)
        {
            block_start_file_pointer = file_pointer >> 15 << 15;
            if (SetFilePointer(file_handle.file, block_start_file_pointer, NULL, FILE_BEGIN) == INVALID_FILE_SIZE && GetLastError())
            {
                C_m_p.Unlock();
                return -1;
            }

            block_size = min(0x8000, file_handle.size - block_start_file_pointer);

            if (!ReadFile(file_handle.file, file_handle.block_content, block_size, &bytes_read, NULL) || bytes_read != block_size)
            {
                C_m_p.Unlock();
                return -1;
            }

            file_handle.is_block_in_memory = true;
            block_file_pointer = file_pointer - block_start_file_pointer;
        }

        uint remaining_block_size = block_size - block_file_pointer;
        if (remaining_block_size >= len)
        {
            memcpy(buffer, file_handle.block_content + block_file_pointer, len);
            block_file_pointer += len;
            file_pointer += len;
            if (block_file_pointer == block_size && file_pointer < file_handle.size)
            {
                block_start_file_pointer += block_size;
                file_handle.is_block_in_memory = false;
            }
        }
        else
        {
            memcpy(buffer, file_handle.block_content + block_file_pointer, remaining_block_size);

            uint remaining_bytes = len - remaining_block_size;
            file_handle.is_block_in_memory = false;
            file_pointer += remaining_block_size;
            block_file_pointer = 0;
            block_start_file_pointer += block_size;
            if (SetFilePointer(file_handle.file, block_start_file_pointer, NULL, FILE_BEGIN) == INVALID_FILE_SIZE && GetLastError())
            {
                C_m_p.Unlock();
                return -1;
            }
            block_size = min(0x8000, file_handle.size - block_start_file_pointer);

            // This block is the best example of how terrible this function is
            // First it does the regular block stuff, even adds an insane optimization by aligning the address to 4 bytes
            // But if the caller wants more than a single extra block, it skips the whole block bullshit and just reads it straight into the buffer
            // Absolute Cinema
            if (remaining_bytes < block_size)
            {
                if (remaining_bytes != 0 && ReadFile(file_handle.file, file_handle.block_content, block_size, &bytes_read, NULL) && bytes_read == block_size)
                {
                    // Insane optimization
                    memcpy(buffer + remaining_block_size, file_handle.block_content, 4 * (remaining_bytes >> 2));

                    uchar *end_of_buffer = buffer + 4 * (remaining_bytes >> 2) + remaining_block_size;
                    memcpy(end_of_buffer, file_handle.block_content + 4 * (remaining_bytes >> 2), remaining_bytes % 4);

                    block_file_pointer = remaining_bytes;
                    file_pointer += remaining_bytes;
                    file_handle.is_block_in_memory = true;
                    len = remaining_bytes + remaining_block_size;
                }
            }
            else if (ReadFile(file_handle.file, buffer + remaining_block_size, remaining_bytes, &bytes_read, NULL) && bytes_read == remaining_bytes)
            {
                file_pointer += remaining_bytes;

                len = remaining_bytes + remaining_block_size;
            }
        }
    }
    else
    {
        uint fp_block = file_pointer / 0x8000;
        if (fp_block > current_block)
        {
            do
            {
                block_start_file_pointer += compressed_block_size + 4;
                ++current_block;
                if (SetFilePointer(file_handle.file, block_start_file_pointer + file_handle.data_offset, NULL, FILE_BEGIN) == INVALID_FILE_SIZE && GetLastError())
                {
                    C_m_p.Unlock();
                    return -1;
                }

                if (!ReadFile(file_handle.file, &compressed_block_size, 4, &bytes_read, NULL) || bytes_read != 4)
                {
                    C_m_p.Unlock();
                    return -1;
                }

                previous_compressed_block_size = compressed_block_size >> 16;
                compressed_block_size &= 0xFFFF;
            } while (current_block != fp_block);
            block_file_pointer = file_pointer - (current_block * 0x8000);
            file_handle.is_block_in_memory = false;
        }

        if (fp_block < current_block)
        {
            if (fp_block >= current_block / 2)
            {
                if (current_block == fp_block)
                {
                    block_file_pointer = file_pointer - (current_block / 0x8000);
                    file_handle.is_block_in_memory = false;
                }
                else
                {
                    do
                    {
                        block_start_file_pointer -= 4 + previous_compressed_block_size;
                        --current_block;
                        if (SetFilePointer(file_handle.file, block_start_file_pointer + file_handle.data_offset, NULL, FILE_BEGIN) == INVALID_FILE_SIZE && GetLastError())
                        {
                            C_m_p.Unlock();
                            return -1;
                        }

                        if (!ReadFile(file_handle.file, &compressed_block_size, 4, &bytes_read, NULL) || bytes_read != 4)
                        {
                            C_m_p.Unlock();
                            return -1;
                        }

                        previous_compressed_block_size = compressed_block_size >> 16;
                        compressed_block_size &= 0xFFFF;
                    } while (current_block != fp_block);
                }
            }
            else
            {
                if (SetFilePointer(file_handle.file, file_handle.data_offset, NULL, FILE_BEGIN) == INVALID_FILE_SIZE && GetLastError())
                {
                    C_m_p.Unlock();
                    return -1;
                }

                block_start_file_pointer = 0;
                current_block = 0;
                if (!ReadFile(file_handle.file, &compressed_block_size, 4, &bytes_read, NULL) || bytes_read != 4)
                {
                    C_m_p.Unlock();
                    return -1;
                }

                previous_compressed_block_size = compressed_block_size >> 16;
                compressed_block_size &= 0xFFFF;
                if (fp_block == 0)
                {
                    file_handle.is_block_in_memory = false;
                }
                else
                {
                    while (true)
                    {
                        block_start_file_pointer += compressed_block_size + 4;
                        ++current_block;
                        if (SetFilePointer(file_handle.file, block_start_file_pointer + file_handle.data_offset, NULL, FILE_BEGIN) == INVALID_FILE_SIZE && GetLastError())
                            break;
                        if (!ReadFile(file_handle.file, &compressed_block_size, 4, &bytes_read, NULL) || bytes_read != 4)
                            break;
                        previous_compressed_block_size = compressed_block_size >> 16;
                        compressed_block_size &= 0xFFFF;
                        if (current_block == fp_block)
                        {
                            file_handle.is_block_in_memory = false;
                            break;
                        }
                    }
                }
            }
        }

        if (!file_handle.is_block_in_memory)
        {
            if (SetFilePointer(file_handle.file, block_start_file_pointer + file_handle.data_offset + 4, NULL, FILE_BEGIN) == INVALID_FILE_SIZE && GetLastError())
            {
                C_m_p.Unlock();
                return -1;
            }

            if (!ReadFile(file_handle.file, file_buffer, compressed_block_size, &bytes_read, NULL) || bytes_read != compressed_block_size)
            {
                C_m_p.Unlock();
                return -1;
            }

            if (file_handle.encrypted)
            {
                DeCrypt(file_buffer, compressed_block_size, file_handle.key1, file_handle.key2);
            }

            if (file_buffer[0] == 1)
            {
                block_size = Decompression(file_buffer, file_handle.block_content, compressed_block_size);
            }
            else if (file_buffer[0] < 8)
            {
                block_size = compressed_block_size - 1;
                memcpy(file_handle.block_content, file_buffer + 1, block_size);
            }
            else
            {
                if (current_block == 0 && !file_handle.encrypted)
                {
                    DeCrypt(file_buffer + 1, sizeof(S_WAVHEADER), file_handle.key1, file_handle.key2);
                }
                block_size = WavDecompression(file_buffer, file_handle.block_content, compressed_block_size, file_handle.wav_header);
            }

            file_handle.is_block_in_memory = true;
        }

        uint remaining_block_size = block_size - block_file_pointer;
        if (remaining_block_size >= len)
        {
            memcpy(buffer, file_handle.block_content + block_file_pointer, len);
            block_file_pointer += len;
            file_pointer += len;

            if (block_file_pointer == block_size && current_block + 1 < file_handle.block_count)
            {
                current_block++;
                block_start_file_pointer += compressed_block_size + 4;
                if (SetFilePointer(file_handle.file, block_start_file_pointer + file_handle.data_offset, NULL, FILE_BEGIN) == INVALID_FILE_SIZE && GetLastError())
                {
                    C_m_p.Unlock();
                    return -1;
                }

                if (!ReadFile(file_handle.file, &compressed_block_size, 4, &bytes_read, NULL) || bytes_read != 4)
                {
                    C_m_p.Unlock();
                    return -1;
                }
                block_file_pointer = 0;
                previous_compressed_block_size = compressed_block_size >> 16;
                compressed_block_size &= 0xFFFF;
                file_handle.is_block_in_memory = false;
            }
        }
        else
        {
            memcpy(buffer, file_handle.block_content + block_file_pointer, remaining_block_size);

            uint remaining_bytes = len - remaining_block_size;
            len = block_size - block_file_pointer;
            block_start_file_pointer += compressed_block_size + 4;
            file_pointer += remaining_block_size;
            block_file_pointer = 0;
            if (SetFilePointer(file_handle.file, file_handle.data_offset + block_start_file_pointer, NULL, FILE_BEGIN) == INVALID_FILE_SIZE && GetLastError())
            {
                C_m_p.Unlock();
                return -1;
            }

            int remaining_blocks = remaining_bytes / 0x8000;

            int relative_fp = remaining_bytes % 0x8000;
            for (int i = 0; i < remaining_blocks; ++i)
            {
                if (!ReadFile(file_handle.file, &compressed_block_size, 4, &bytes_read, NULL) || bytes_read != 4)
                {
                    file_handle.is_block_in_memory = false;
                    C_m_p.Unlock();
                    return -1;
                }

                compressed_block_size &= 0xFFFF;
                if (!ReadFile(file_handle.file, file_buffer, compressed_block_size, &bytes_read, NULL) || bytes_read != compressed_block_size)
                {
                    file_handle.is_block_in_memory = false;
                    C_m_p.Unlock();
                    return -1;
                }

                current_block++;
                if (file_handle.encrypted)
                {
                    DeCrypt(file_buffer, compressed_block_size, file_handle.key1, file_handle.key2);
                }

                if (file_buffer[0] == 1)
                {
                    block_size = Decompression(file_buffer, buffer + len, compressed_block_size);
                }
                else if (file_buffer[0] < 8)
                {
                    block_size = compressed_block_size - 1;
                    memcpy(buffer + len, file_buffer + 1, block_size);
                }
                else
                {
                    if (current_block == 0 && !file_handle.encrypted)
                    {
                        DeCrypt(file_buffer + 1, sizeof(S_WAVHEADER), file_handle.key1, file_handle.key2);
                    }
                    block_size = WavDecompression(file_buffer, buffer + len, compressed_block_size, file_handle.wav_header);
                }

                len += block_size;
                file_pointer += block_size;
                block_start_file_pointer += compressed_block_size + 4;
            }

            if (!ReadFile(file_handle.file, &compressed_block_size, 4, &bytes_read, NULL) || bytes_read != 4)
            {
                file_handle.is_block_in_memory = false;
                C_m_p.Unlock();
                return -1;
            }

            previous_compressed_block_size = compressed_block_size >> 16;
            compressed_block_size &= 0xFFFF;

            ++current_block;
            if (relative_fp != 0)
            {
                if (!ReadFile(file_handle.file, file_buffer, compressed_block_size, &bytes_read, NULL) || bytes_read != compressed_block_size)
                {
                    file_handle.is_block_in_memory = false;
                    C_m_p.Unlock();
                    return -1;
                }

                if (file_handle.encrypted)
                {
                    DeCrypt(file_buffer, compressed_block_size, file_handle.key1, file_handle.key2);
                }

                if (file_buffer[0] == 1)
                {
                    block_size = Decompression(file_buffer, file_handle.block_content, compressed_block_size);
                }
                else if (file_buffer[0] < 8)
                {
                    block_size = compressed_block_size - 1;
                    memcpy(file_handle.block_content, file_buffer + 1, block_size);
                }
                else
                {
                    if (current_block == 0 && !file_handle.encrypted)
                    {
                        DeCrypt(file_buffer + 1, sizeof(S_WAVHEADER), file_handle.key1, file_handle.key2);
                    }
                    block_size = WavDecompression(file_buffer, file_handle.block_content, compressed_block_size, file_handle.wav_header);
                }

                memcpy(buffer + len, file_handle.block_content, 4 * (relative_fp >> 2));

                uchar *end_of_buffer = buffer + 4 * (relative_fp >> 2) + remaining_block_size;
                memcpy(end_of_buffer, file_handle.block_content + 4 * (relative_fp >> 2), relative_fp % 4);

                block_file_pointer = relative_fp;
                file_pointer += relative_fp;
                len += relative_fp;
            }
            else
            {
                file_handle.is_block_in_memory = false;
            }
        }
    }

    file_handle.block_start_file_pointer = block_start_file_pointer;
    file_handle.file_pointer = file_pointer;
    file_handle.block_file_pointer = block_file_pointer;
    file_handle.current_block = current_block;
    file_handle.compressed_block_size = compressed_block_size;
    file_handle.previous_compressed_block_size = previous_compressed_block_size;
    file_handle.block_size = block_size;
    C_m_p.Unlock();
    return len;
}

int EXPORT dtaWrite(int handle, const uchar *buffer, uint len)
{
    C_m_p.Lock();
    if (handle >= v_file_h.size())
    {
        C_m_p.Unlock();
        return -1;
    }

    S_FILEHANDLE &file_handle = v_file_h[handle];
    if (file_handle.file == INVALID_HANDLE_VALUE || (file_handle.flags & DTA_WRITE) == 0)
    {
        C_m_p.Unlock();
        return -1;
    }

    if (!buffer)
    {
        if (file_handle.flags & DTA_BUFFER)
        {
            file_handle.flags |= DTA_FAILED_WRITING_TO_BUFFER;
        }
        C_m_p.Unlock();
        return -1;
    }

    DWORD bytes_written = 0;
    if (!WriteFile(file_handle.file, buffer, len, &bytes_written, NULL) || bytes_written != len)
    {
        if (file_handle.flags & DTA_BUFFER)
        {
            file_handle.flags |= DTA_FAILED_WRITING_TO_BUFFER;
        }
        C_m_p.Unlock();
        return -1;
    }

    C_m_p.Unlock();
    return len;
}

int EXPORT dtaSeek(int handle, int distance_to_move, int move_method)
{
    C_m_p.Lock();
    if (handle >= v_file_h.size())
    {
        C_m_p.Unlock();
        return -1;
    }

    S_FILEHANDLE &file_handle = v_file_h[handle];

    if (file_handle.file == INVALID_HANDLE_VALUE)
    {
        C_m_p.Unlock();
        return -1;
    }

    if (file_handle.flags & DTA_WRITE)
    {
        if (move_method != 0 && move_method != 1 && move_method != 2)
        {
            C_m_p.Unlock();
            return -1;
        }

        file_handle.file_pointer = SetFilePointer(file_handle.file, distance_to_move, NULL, move_method);
        if (file_handle.file_pointer == INVALID_FILE_SIZE)
        {
            C_m_p.Unlock();
            return -1;
        }
    }
    else
    {
        if (move_method == 0)
        {
            if (distance_to_move < 0 || distance_to_move > file_handle.size)
            {
                C_m_p.Unlock();
                return -1;
            }
            file_handle.file_pointer = distance_to_move;
        }
        else if (move_method == 1)
        {
            int new_file_pointer = file_handle.file_pointer + distance_to_move;
            if (new_file_pointer > file_handle.size || new_file_pointer < 0)
            {
                C_m_p.Unlock();
                return -1;
            }
            file_handle.file_pointer = new_file_pointer;
        }
        else if (move_method == 2)
        {
            int new_file_pointer = file_handle.size + distance_to_move;
            if (new_file_pointer < 0 || distance_to_move > 1)
            {
                C_m_p.Unlock();
                return -1;
            }
            file_handle.file_pointer = new_file_pointer;
        }
    }

    file_handle.block_file_pointer = file_handle.file_pointer % 0x8000;
    C_m_p.Unlock();
    return file_handle.file_pointer;
}

bool EXPORT dtaClose(int handle)
{
    C_m_p.Lock();

    if (v_file_h.empty() || v_file_h.size() <= handle)
    {
        C_m_p.Unlock();
        return false;
    }

    S_FILEHANDLE &file_handle = v_file_h[handle];
    if (file_handle.file != INVALID_HANDLE_VALUE)
    {
        if (file_handle.flags & DTA_FROM_FILESYSTEM)
        {
            if (!CloseHandle(file_handle.file))
            {
                C_m_p.Unlock();
                return false;
            }

            if (file_handle.flags & DTA_BUFFER)
            {
                if ((file_handle.flags & DTA_FAILED_WRITING_TO_BUFFER) == 0)
                {
                    char file_name[512];
                    strcpy(file_name, file_handle.tmp_file_name);
                    file_name[strlen(file_name) - 4] = '\0';
                    if (!DeleteFileA(file_name))
                    {
                        DWORD file_attributes = GetFileAttributesA(file_name);
                        if (file_attributes != INVALID_FILE_SIZE)
                        {
                            file_attributes &= ~FILE_ATTRIBUTE_READONLY;
                            SetFileAttributesA(file_name, file_attributes);
                            DeleteFileA(file_name);
                        }
                    }
                    MoveFileA(file_handle.tmp_file_name, file_name);
                }
                else
                {
                    DeleteFileA(file_handle.tmp_file_name);
                }
            }
        }

        if (file_handle.tmp_file_name)
        {
            delete[] file_handle.tmp_file_name;
        }

        if (file_handle.block_content)
        {
            delete[] file_handle.block_content;
        }

        if (file_handle.wav_header)
        {
            delete file_handle.wav_header;
        }

        file_handle.wav_header = NULL;
        file_handle.block_content = NULL;
        file_handle.is_block_in_memory = false;
        file_handle.file = INVALID_HANDLE_VALUE;
    }

    C_m_p.Unlock();
    return true;
}

bool EXPORT dtaDelete(const char *file_name)
{
    return (DeleteFileA(file_name) != 0) ? true : false;
}

bool EXPORT dtaGetTime(int handle, ulong *time, bool force_check_from_file)
{
    C_m_p.Lock();

    if (v_file_h.empty() || v_file_h.size() >= handle)
    {
        C_m_p.Unlock();
        return false;
    }

    S_FILEHANDLE file_handle = v_file_h[handle];
    if (file_handle.file == INVALID_HANDLE_VALUE)
    {
        C_m_p.Unlock();
        return false;
    }

    if (file_handle.flags & DTA_FROM_FILESYSTEM || force_check_from_file)
    {
        bool success = GetFileTime(file_handle.file, NULL, NULL, reinterpret_cast<LPFILETIME>(time));
        C_m_p.Unlock();
        return success;
    }
    else
    {
        *time = file_handle.timestamp;
        C_m_p.Unlock();
        return true;
    }
}
