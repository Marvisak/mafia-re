#pragma once

// The chunk files (PS2 C_chunk; the header's name is ours): a chunk is a 6-byte header, its type and its size with the
// header, followed by its data, which can be chunks again. PS2's methods are inline, their copies in C_animations.cpp's
// range, and PS2's stack of open chunks is an array of 256 with the file's size first; the PC's is a vector.
// LoadPatchFile shows which methods are inline on PC: all but OpenFile (C_chunk.cpp), the destructor included, which
// its budget leaves out of line, as the fourth Read and the second and third Read's size(). Read has PS2's statements,
// the stack test as a block of its own and the read's test written out, which cost the budget the original spent.
// Read, Descend and Close end in a return false their success skips, so their copies send every failure to one block
// after the normal exit. The original kept the copies of Descend and Read at the start of the range at 0x404ce0, of
// the destructor and Close (0x411d40) after OpenFile (docs/game-files.md); the methods are in address order here.

#include <vector>

#include "I3D/I3d_math.h"
#include "rw_data.h"

// The stack's allocator (the name is ours): std::allocator with allocate and deallocate virtual, so the vector calls
// them through the vtable (0x6233dc). The empty base sits at +4, over the vector's first pointer: the implicit copy
// constructor, which the vector's default argument calls, copies its byte there before the vector clears it.
template<class T>
class C_chunk_allocator : public std::allocator<T> {
public:
    virtual pointer allocate(size_type n, const void *)
    {
        return (pointer)operator new(n * sizeof(T));
    }

    virtual void deallocate(void *p, size_type)
    {
        operator delete(p);
    }
};

#pragma pack(push, 2)
// A chunk's header, as the file has it (PS2 reads the size unaligned; the names are ours)
struct S_chunk_header {
    ushort type;
    int size;                           // with the header
};
#pragma pack(pop)

class C_chunk {
public:
    std::vector<int, C_chunk_allocator<int> > stack;  // 0x0, the file positions where the open chunks end
    bool open;                          // 0x10
    int handle;                         // 0x14
    bool write;                         // 0x18
    int file_size;                      // 0x1c, read files only (PS2's is the stack's first entry)

    C_chunk() : open(false), handle(-1), write(false) {}

    // Opens a file for reading or writing; the only method that isn't inline (C_chunk.cpp)
    bool OpenFile(const char *name, bool for_write);

    // Leaves the current chunk: to its end when reading, writing its size into its header when writing. The chunk's
    // start is a local: VC6 reads back() again for each use
    // FUNCTION: GAME 0x404ce0
    bool Descend()
    {
        if (open)
        {
            if (!write)
                dtaSeek(handle, stack.back(), SEEK_SET);
            else
            {
                int pos = dtaSeek(handle, 0, SEEK_CUR);
                int start = stack.back();
                int size = pos - start;
                dtaSeek(handle, start + 2, SEEK_SET);
                dtaWrite(handle, &size, sizeof(size));
                dtaSeek(handle, pos, SEEK_SET);
            }
            stack.pop_back();
            return true;
        }
        return false;
    }

    // Reads len bytes, if the current chunk has that many left
    // FUNCTION: GAME 0x404d80
    bool Read(void *buffer, int len)
    {
        if (!open)
            return false;
        if (!write)
        {
            if (stack.size())
            {
                if (len > Max(0, stack.back() - dtaSeek(handle, 0, SEEK_CUR)))
                    return false;
            }
            if ((int)dtaRead(handle, buffer, len) < len)
                return false;
            return true;
        }
        return false;
    }

    // FUNCTION: GAME 0x411c50
    ~C_chunk()
    {
        Close();
    }

    // Closes the open chunks of a written file, then the file
    bool Close()
    {
        if (open)
        {
            if (write)
                while (stack.size())
                    Descend();
            dtaClose(handle);
            handle = -1;
            open = false;
            return true;
        }
        return false;
    }

    // Enters the next chunk: reads its header and returns its type
    bool RAscend(ushort &type)
    {
        if (!open)
            return false;
        S_chunk_header header;
        if (dtaRead(handle, &header, sizeof(header)) != sizeof(header))
            return false;
        stack.push_back(dtaSeek(handle, 0, SEEK_CUR) + header.size - sizeof(header));
        type = header.type;
        return true;
    }

    // What is left of the current chunk
    int Size() const
    {
        if (!open)
            return 0;
        int pos = dtaSeek(handle, 0, SEEK_CUR);
        if (pos == -1)
            return 0;
        return stack.back() - pos;
    }
};

// The stack's out-of-line functions
// VTABLE: GAME 0x6233dc
// class C_chunk_allocator<int>

// TEMPLATE: GAME 0x410c00
// C_chunk_allocator<int>::allocate

// TEMPLATE: GAME 0x410c20
// C_chunk_allocator<int>::deallocate

// SYNTHETIC: GAME 0x44b240
// C_chunk_allocator<int>::C_chunk_allocator<int>

// TEMPLATE: GAME 0x44b2a0
// std::vector<int,C_chunk_allocator<int> >::insert

// TEMPLATE: GAME 0x598390
// std::vector<int,C_chunk_allocator<int> >::size
