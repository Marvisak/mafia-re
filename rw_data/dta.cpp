#include "dta.h"

bool dta_first_force = false;
std::vector<struct S_FILEHANDLE> v_file_h;
std::vector<struct S_HANDLE> v_handle;
C_multithread_protection C_m_p;
C_ref_counter C_r_f;

static ushort wav_table[] =
    {
        0x0000, 0x0001, 0x0002, 0x0004, 0x0008, 0x000c, 0x0012, 0x0018, 0x0020, 0x0029, 0x0032, 0x003d,
        0x0049, 0x0055, 0x0063, 0x0072, 0x0082, 0x0092, 0x00a4, 0x00b7, 0x00cb, 0x00df, 0x00f5, 0x010c,
        0x0124, 0x013d, 0x0157, 0x0172, 0x018e, 0x01ab, 0x01c9, 0x01e8, 0x0208, 0x0229, 0x024b, 0x026e,
        0x0292, 0x02b7, 0x02dd, 0x0304, 0x032c, 0x0355, 0x037f, 0x03ab, 0x03d7, 0x0404, 0x0432, 0x0461,
        0x0492, 0x04c3, 0x04f5, 0x0529, 0x055d, 0x0592, 0x05c9, 0x0600, 0x0638, 0x0672, 0x06ac, 0x06e8,
        0x0724, 0x0761, 0x07a0, 0x07df, 0x0820, 0x0861, 0x08a4, 0x08e7, 0x092c, 0x0972, 0x09b8, 0x0a00,
        0x0a48, 0x0a92, 0x0add, 0x0b28, 0x0b75, 0x0bc3, 0x0c12, 0x0c61, 0x0cb2, 0x0d04, 0x0d57, 0x0daa,
        0x0dff, 0x0e55, 0x0eac, 0x0f04, 0x0f5d, 0x0fb7, 0x1012, 0x106d, 0x10ca, 0x1128, 0x1187, 0x11e7,
        0x1248, 0x12aa, 0x130d, 0x1371, 0x13d7, 0x143d, 0x14a4, 0x150c, 0x1575, 0x15df, 0x164a, 0x16b7,
        0x1724, 0x1792, 0x1801, 0x1871, 0x18e3, 0x1955, 0x19c8, 0x1a3d, 0x1ab2, 0x1b28, 0x1ba0, 0x1c18,
        0x1c91, 0x1d0c, 0x1d87, 0x1e04, 0x1e81, 0x1f00, 0x1f7f, 0x2000, 0x0000, 0x0001, 0x0003, 0x0006,
        0x000c, 0x0013, 0x001b, 0x0025, 0x0030, 0x003d, 0x004c, 0x005c, 0x006d, 0x0080, 0x0095, 0x00ab,
        0x00c3, 0x00dc, 0x00f6, 0x0113, 0x0130, 0x014f, 0x0170, 0x0193, 0x01b6, 0x01dc, 0x0203, 0x022b,
        0x0255, 0x0280, 0x02ad, 0x02dc, 0x030c, 0x033d, 0x0370, 0x03a5, 0x03db, 0x0412, 0x044c, 0x0486,
        0x04c2, 0x0500, 0x053f, 0x0580, 0x05c2, 0x0606, 0x064c, 0x0692, 0x06db, 0x0725, 0x0770, 0x07bd,
        0x080c, 0x085c, 0x08ad, 0x0900, 0x0955, 0x09ab, 0x0a02, 0x0a5c, 0x0ab6, 0x0b12, 0x0b70, 0x0bcf,
        0x0c30, 0x0c92, 0x0cf6, 0x0d5b, 0x0dc2, 0x0e2b, 0x0e95, 0x0f00, 0x0f6d, 0x0fdb, 0x104b, 0x10bd,
        0x1130, 0x11a5, 0x121b, 0x1292, 0x130b, 0x1386, 0x1402, 0x1480, 0x14ff, 0x1580, 0x1602, 0x1686,
        0x170b, 0x1792, 0x181b, 0x18a4, 0x1930, 0x19bd, 0x1a4b, 0x1adb, 0x1b6d, 0x1c00, 0x1c94, 0x1d2a,
        0x1dc2, 0x1e5b, 0x1ef6, 0x1f92, 0x2030, 0x20cf, 0x2170, 0x2212, 0x22b6, 0x235b, 0x2402, 0x24aa,
        0x2554, 0x2600, 0x26ad, 0x275b, 0x280b, 0x28bd, 0x2970, 0x2a24, 0x2ada, 0x2b92, 0x2c4b, 0x2d06,
        0x2dc2, 0x2e80, 0x2f3f, 0x3000, 0x0000, 0x0002, 0x0004, 0x0009, 0x0010, 0x0019, 0x0024, 0x0031,
        0x0041, 0x0052, 0x0065, 0x007a, 0x0092, 0x00ab, 0x00c7, 0x00e4, 0x0104, 0x0125, 0x0149, 0x016e,
        0x0196, 0x01bf, 0x01eb, 0x0219, 0x0249, 0x027a, 0x02ae, 0x02e4, 0x031c, 0x0356, 0x0392, 0x03d0,
        0x0410, 0x0452, 0x0496, 0x04dc, 0x0524, 0x056e, 0x05ba, 0x0609, 0x0659, 0x06ab, 0x06ff, 0x0756,
        0x07ae, 0x0809, 0x0865, 0x08c3, 0x0924, 0x0986, 0x09eb, 0x0a52, 0x0aba, 0x0b25, 0x0b92, 0x0c00,
        0x0c71, 0x0ce4, 0x0d59, 0x0dd0, 0x0e48, 0x0ec3, 0x0f40, 0x0fbf, 0x1040, 0x10c3, 0x1148, 0x11cf,
        0x1259, 0x12e4, 0x1371, 0x1400, 0x1491, 0x1525, 0x15ba, 0x1651, 0x16eb, 0x1786, 0x1824, 0x18c3,
        0x1965, 0x1a08, 0x1aae, 0x1b55, 0x1bff, 0x1cab, 0x1d58, 0x1e08, 0x1eba, 0x1f6e, 0x2024, 0x20db,
        0x2195, 0x2251, 0x230f, 0x23cf, 0x2491, 0x2555, 0x261b, 0x26e3, 0x27ae, 0x287a, 0x2948, 0x2a18,
        0x2aeb, 0x2bbf, 0x2c95, 0x2d6e, 0x2e48, 0x2f24, 0x3003, 0x30e3, 0x31c6, 0x32aa, 0x3391, 0x347a,
        0x3564, 0x3651, 0x3740, 0x3830, 0x3923, 0x3a18, 0x3b0f, 0x3c08, 0x3d03, 0x3e00, 0x3eff, 0x4000,
        0x0000, 0x0002, 0x0005, 0x000b, 0x0014, 0x001f, 0x002d, 0x003e, 0x0051, 0x0066, 0x007e, 0x0099,
        0x00b6, 0x00d6, 0x00f8, 0x011d, 0x0145, 0x016e, 0x019b, 0x01ca, 0x01fb, 0x022f, 0x0266, 0x029f,
        0x02db, 0x0319, 0x035a, 0x039d, 0x03e3, 0x042b, 0x0476, 0x04c4, 0x0514, 0x0566, 0x05bb, 0x0613,
        0x066d, 0x06ca, 0x0729, 0x078b, 0x07ef, 0x0856, 0x08bf, 0x092b, 0x099a, 0x0a0b, 0x0a7e, 0x0af4,
        0x0b6d, 0x0be8, 0x0c66, 0x0ce6, 0x0d69, 0x0dee, 0x0e76, 0x0f01, 0x0f8d, 0x101d, 0x10af, 0x1144,
        0x11db, 0x1274, 0x1310, 0x13af, 0x1450, 0x14f4, 0x159b, 0x1643, 0x16ef, 0x179d, 0x184d, 0x1900,
        0x19b6, 0x1a6e, 0x1b29, 0x1be6, 0x1ca6, 0x1d68, 0x1e2d, 0x1ef4, 0x1fbe, 0x208a, 0x2159, 0x222b,
        0x22ff, 0x23d6, 0x24af, 0x258a, 0x2669, 0x2749, 0x282d, 0x2912, 0x29fb, 0x2ae6, 0x2bd3, 0x2cc3,
        0x2db6, 0x2eab, 0x2fa2, 0x309c, 0x3199, 0x3298, 0x339a, 0x349e, 0x35a5, 0x36af, 0x37bb, 0x38c9,
        0x39da, 0x3aee, 0x3c04, 0x3d1c, 0x3e37, 0x3f55, 0x4075, 0x4198, 0x42bd, 0x43e5, 0x4510, 0x463d,
        0x476c, 0x489e, 0x49d3, 0x4b0a, 0x4c43, 0x4d80, 0x4ebe, 0x5000, 0x0000, 0x0002, 0x0006, 0x000d,
        0x0018, 0x0026, 0x0036, 0x004a, 0x0061, 0x007b, 0x0098, 0x00b8, 0x00db, 0x0101, 0x012a, 0x0156,
        0x0186, 0x01b8, 0x01ed, 0x0226, 0x0261, 0x029f, 0x02e1, 0x0326, 0x036d, 0x03b8, 0x0406, 0x0456,
        0x04aa, 0x0501, 0x055b, 0x05b8, 0x0618, 0x067b, 0x06e1, 0x074a, 0x07b6, 0x0825, 0x0898, 0x090d,
        0x0985, 0x0a01, 0x0a7f, 0x0b01, 0x0b85, 0x0c0d, 0x0c98, 0x0d25, 0x0db6, 0x0e4a, 0x0ee1, 0x0f7b,
        0x1018, 0x10b8, 0x115b, 0x1201, 0x12aa, 0x1356, 0x1405, 0x14b8, 0x156d, 0x1625, 0x16e1, 0x179f,
        0x1861, 0x1925, 0x19ed, 0x1ab7, 0x1b85, 0x1c56, 0x1d2a, 0x1e01, 0x1eda, 0x1fb7, 0x2097, 0x217a,
        0x2260, 0x234a, 0x2436, 0x2525, 0x2617, 0x270d, 0x2805, 0x2900, 0x29ff, 0x2b00, 0x2c05, 0x2d0c,
        0x2e17, 0x2f25, 0x3036, 0x3149, 0x3260, 0x337a, 0x3497, 0x35b7, 0x36da, 0x3800, 0x3929, 0x3a55,
        0x3b85, 0x3cb7, 0x3dec, 0x3f25, 0x4060, 0x419e, 0x42e0, 0x4425, 0x456c, 0x46b7, 0x4804, 0x4955,
        0x4aa9, 0x4c00, 0x4d5a, 0x4eb7, 0x5017, 0x517a, 0x52e0, 0x5449, 0x55b5, 0x5724, 0x5896, 0x5a0c,
        0x5b84, 0x5d00, 0x5e7e, 0x6000, 0x0000, 0x0002, 0x0007, 0x000f, 0x001c, 0x002c, 0x003f, 0x0057,
        0x0071, 0x008f, 0x00b1, 0x00d7, 0x00ff, 0x012c, 0x015c, 0x018f, 0x01c7, 0x0201, 0x023f, 0x0281,
        0x02c7, 0x030f, 0x035c, 0x03ac, 0x03ff, 0x0457, 0x04b1, 0x050f, 0x0571, 0x05d7, 0x063f, 0x06ac,
        0x071c, 0x078f, 0x0806, 0x0881, 0x08ff, 0x0981, 0x0a06, 0x0a8f, 0x0b1c, 0x0bac, 0x0c3f, 0x0cd6,
        0x0d71, 0x0e0f, 0x0eb1, 0x0f56, 0x0fff, 0x10ac, 0x115c, 0x120f, 0x12c6, 0x1381, 0x143f, 0x1501,
        0x15c6, 0x168f, 0x175c, 0x182c, 0x18ff, 0x19d6, 0x1ab1, 0x1b8f, 0x1c71, 0x1d56, 0x1e3f, 0x1f2b,
        0x201b, 0x210f, 0x2206, 0x2301, 0x23ff, 0x2501, 0x2606, 0x270f, 0x281b, 0x292b, 0x2a3f, 0x2b56,
        0x2c71, 0x2d8f, 0x2eb1, 0x2fd6, 0x30ff, 0x322b, 0x335b, 0x348f, 0x35c6, 0x3700, 0x383f, 0x3980,
        0x3ac6, 0x3c0f, 0x3d5b, 0x3eab, 0x3ffe, 0x4156, 0x42b0, 0x440e, 0x4570, 0x46d5, 0x483e, 0x49ab,
        0x4b1b, 0x4c8e, 0x4e05, 0x4f80, 0x50fe, 0x5280, 0x5405, 0x558e, 0x571b, 0x58ab, 0x5a3e, 0x5bd5,
        0x5d70, 0x5f0e, 0x60b0, 0x6255, 0x63fe, 0x65aa, 0x675a, 0x690e, 0x6ac5, 0x6c80, 0x6e3e, 0x7000,
        0x0000, 0x0002, 0x0008, 0x0012, 0x0020, 0x0032, 0x0049, 0x0063, 0x0082, 0x00a4, 0x00cb, 0x00f5,
        0x0124, 0x0157, 0x018e, 0x01c9, 0x0208, 0x024b, 0x0292, 0x02dd, 0x032c, 0x037f, 0x03d7, 0x0432,
        0x0492, 0x04f5, 0x055d, 0x05c9, 0x0638, 0x06ac, 0x0724, 0x07a0, 0x0820, 0x08a4, 0x092c, 0x09b8,
        0x0a48, 0x0add, 0x0b75, 0x0c11, 0x0cb2, 0x0d57, 0x0dff, 0x0eac, 0x0f5d, 0x1011, 0x10ca, 0x1187,
        0x1248, 0x130d, 0x13d6, 0x14a4, 0x1575, 0x164a, 0x1724, 0x1801, 0x18e2, 0x19c8, 0x1ab2, 0x1b9f,
        0x1c91, 0x1d87, 0x1e81, 0x1f7f, 0x2081, 0x2187, 0x2291, 0x239f, 0x24b1, 0x25c8, 0x26e2, 0x2801,
        0x2923, 0x2a4a, 0x2b74, 0x2ca3, 0x2dd6, 0x2f0d, 0x3047, 0x3186, 0x32c9, 0x3411, 0x355c, 0x36ab,
        0x37fe, 0x3956, 0x3ab1, 0x3c10, 0x3d74, 0x3edb, 0x4047, 0x41b7, 0x432b, 0x44a2, 0x461e, 0x479e,
        0x4922, 0x4aaa, 0x4c37, 0x4dc7, 0x4f5b, 0x50f3, 0x5290, 0x5430, 0x55d5, 0x577d, 0x592a, 0x5adb,
        0x5c90, 0x5e48, 0x6005, 0x61c6, 0x638b, 0x6554, 0x6722, 0x68f3, 0x6ac8, 0x6ca1, 0x6e7f, 0x7060,
        0x7246, 0x7430, 0x761d, 0x780f, 0x7a05, 0x7bff, 0x7dfd, 0x7fff};

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
            if (fp_block >= current_block >> 1)
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

void DeCrypt(uchar *buffer, uint len, uint key1, uint key2)
{
    uint keys[2] = {key1, key2};
    uchar *key = reinterpret_cast<uchar *>(keys);

    for (uint i = 0; i < len; i++)
    {
        buffer[i] = (~((~buffer[i]) ^ key[i % sizeof(keys)]));
    }
}

int OpenFromDTA(char const *file_name)
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

int OpenFromFileSystem(const char *file_name)
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

int Decompression(uchar *buffer, uchar *dest, uint len)
{
    uchar *start = dest;

    uint position = 1;
    while (position < len)
    {
        ushort value = (buffer[position] << 8) | (buffer[position + 1]);
        position += 2;

        if (!value)
        {
            uchar segment = min(len - position, 16);
            memcpy(dest, buffer + position, segment);
            position += segment;
            dest += segment;
        }
        else
        {
            for (int i = 0; i < 16 && position < len; ++i, value <<= 1)
            {
                if (value & 0x8000)
                {
                    uint offset = (buffer[position] << 4) | (buffer[position + 1] >> 4);
                    uint length = buffer[position + 1] & 0xF;

                    if (offset == 0)
                    {
                        length = ((length << 8) | buffer[position + 2]) + 16;
                        memset(dest, buffer[position + 3], length);
                        position += 4;
                        dest += length;
                    }
                    else
                    {
                        length += 3;
                        if (length > offset)
                        {
                            while (length--)
                                *dest++ = *(dest - offset);
                        }
                        else
                        {
                            memcpy(dest, dest - offset, length);
                            dest += length;
                        }
                        position += 2;
                    }
                }
                else
                {
                    *dest++ = buffer[position++];
                }
            }
        }
    }

    return dest - start;
}

// I have no idea how audio decompression works, so ignore the simple variable names
int WavDecompression(uchar *buffer, uchar *dest, uint len, S_WAVHEADER *wav_header)
{
    int decompressed_size = 0;
    int position = 1;

    int dpcm_type = 0;
    uchar block_type = buffer[0];
    switch (block_type)
    {
    case 12:
        dpcm_type = 1;
        break;
    case 16:
        dpcm_type = 2;
        break;
    case 20:
        dpcm_type = 3;
        break;
    case 24:
        dpcm_type = 4;
        break;
    case 28:
        dpcm_type = 5;
        break;
    case 32:
        dpcm_type = 6;
        break;
    }

    S_WAVHEADER *block_header = reinterpret_cast<S_WAVHEADER *>(buffer + 1);
    if (block_header->riff_chunk_id == wav_header->riff_chunk_id && block_header->format_chunk_id == wav_header->format_chunk_id && block_header->data_chunk_id == wav_header->data_chunk_id)
    {
        memcpy(dest, buffer + 1, sizeof(S_WAVHEADER));
        position = sizeof(S_WAVHEADER) + 1;
        decompressed_size = sizeof(S_WAVHEADER);
    }

    if (position >= len)
        return decompressed_size;

    if (wav_header->block_align / wav_header->channel_count != 2)
    {
        // This branch is never taken, like ever, I have no idea why this is even here, just made reversing this shit harder
        return Decompression(&buffer[position], &dest[decompressed_size], len - position);
    }

    int wav_table_index = dpcm_type * 128;
    if (wav_header->channel_count == 2)
    {
        // Another sequel in the series of "branches that are never taken but for some reason are in the code"
        for (int i = 4; i != 0; --i)
        {
            dest[decompressed_size++] = buffer[position++];
        }
        if (position >= len)
        {
            return decompressed_size;
        }
        ushort value = *reinterpret_cast<ushort *>(&dest[decompressed_size - 4]);
        ushort value2 = *reinterpret_cast<ushort *>(&dest[decompressed_size - 2]);
        for (; position < len; position += 2)
        {
            uchar buffer_value = buffer[position];
            ushort wav_value = wav_table[wav_table_index + (buffer_value & 0x7F)];
            value += static_cast<char>(buffer_value) >= 0 ? wav_value : -wav_value;
            *reinterpret_cast<ushort *>(&dest[decompressed_size]) = value;

            uchar buffer_value2 = buffer[position + 1];
            ushort wav_value2 = wav_table[wav_table_index + (buffer_value2 & 0x7F)];
            value2 += static_cast<char>(buffer_value2) >= 0 ? wav_value2 : -wav_value2;
            *reinterpret_cast<ushort *>(&dest[decompressed_size + 2]) = value2;

            decompressed_size += 4;
        }

        return decompressed_size;
    }

    for (int i = 2; i != 0; --i)
    {
        dest[decompressed_size++] = buffer[position++];
    }

    if (position >= len)
    {
        return decompressed_size;
    }

    ushort value = *reinterpret_cast<ushort *>(&dest[decompressed_size - 2]);
    for (; position < len; ++position)
    {
        uchar buffer_value = buffer[position];
        ushort wav_value = wav_table[wav_table_index + (buffer_value & 0x7F)];
        value += static_cast<char>(buffer_value) >= 0 ? wav_value : -wav_value;
        *reinterpret_cast<ushort *>(&dest[decompressed_size]) = value;

        decompressed_size += 2;
    }

    return decompressed_size;
}

bool EXPORT dtaClose(int handle)
{
    C_m_p.Lock();

    if (v_file_h.empty() || v_file_h.size() <= handle)
    {
        C_m_p.Unlock();
        return false;
    }

    S_FILEHANDLE file_handle = v_file_h[handle];
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
