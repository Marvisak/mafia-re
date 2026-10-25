// The chunk files' one out-of-line method (the file's name is ours). PS2 has every C_chunk method inline; on PC
// LoadPatchFile calls OpenFile, at the start of the range at 0x411be0 (docs/game-files.md), where every other method it
// uses is inline or a copy of one. So OpenFile is defined in a file of its own, most likely one of the two without
// <string>, whose code opens the range: it includes only the chunks' header.
// Open: the original's copies of the destructor (0x411c50) and Close (0x411d40) follow OpenFile, so something in this
// file referenced them out of line, which the linker then dropped.

#include "C_chunk.h"

// FUNCTION: GAME 0x411be0
bool C_chunk::OpenFile(const char *name, bool for_write)
{
    if (open)
        return false;
    file_size = -1;
    if (for_write)
        handle = dtaOpenWrite(name, 0);
    else
        handle = dtaOpen(name, false);
    if (handle != DTA_INVALID_HANDLE)
    {
        write = for_write;
        if (!for_write)
            file_size = dtaSeek(handle, 0, SEEK_END);
        dtaSeek(handle, 0, SEEK_SET);
        open = true;
        return true;
    }
    return false;
}
