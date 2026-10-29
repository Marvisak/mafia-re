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

    C_chunk() : open(false), handle(DTA_INVALID_HANDLE), write(false) {}

    // Opens a file for reading or writing; the only method that isn't inline (C_chunk.cpp)
    bool OpenFile(const char *name, bool for_write);

    // Opens a file for reading (Insanity3D's name). C_effects::Load opens its table through it: the call is one of the
    // inline calls still to come for the chunk's constructor, which then leaves the stack's allocator copy out of line
    bool ROpen(const char *name)
    {
        return OpenFile(name, false);
    }

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
    // FUNCTION: GAME 0x411d40
    bool Close()
    {
        if (open)
        {
            if (write)
                while (stack.size())
                    Descend();
            dtaClose(handle);
            handle = DTA_INVALID_HANDLE;
            open = false;
            return true;
        }
        return false;
    }

    // Enters the next chunk: reads its header and returns its type. The read's success returns from a block of its own,
    // so the copy (C_effects.cpp's) sends both failures to the return false after it.
    // Open: the copy calls _Destroy out of line, which the original inlines with _Ufill and _Ucopy left out of line,
    // and no padding gives that mix. Under /YX the stack's insert(P, x), C_effects.cpp's copy in ours, inlines it and
    // the original's (an earlier file's) doesn't: which deferred copy inlines it seems to depend on the object
    // FUNCTION: GAME 0x44b4d0
    bool RAscend(ushort &type)
    {
        if (!open)
            return false;
        S_chunk_header header;
        if (dtaRead(handle, &header, sizeof(header)) == sizeof(header))
        {
            stack.push_back(dtaSeek(handle, 0, SEEK_CUR) + header.size - sizeof(header));
            type = header.type;
            return true;
        }
        return false;
    }

    // What is left of the current chunk
    // FUNCTION: GAME 0x44b6a0
    int Size() const
    {
        if (!open)
            return 0;
        int pos = dtaSeek(handle, 0, SEEK_CUR);
        if (pos == -1)
            return 0;
        return stack.back() - pos;
    }

    // Reads the current chunk, at most 999 bytes of it, into a string and leaves the chunk (PC only, the name is
    // Insanity3D's, whose ReadString also ascends into the chunk first). The bytes include the string's end. The
    // chunk's end is read before the seek, so it's a local (stack.back() in the expression reads it after the call)
    // FUNCTION: GAME 0x44b6d0
    void ReadString(C_str &s)
    {
        if (open)
        {
            char buf[1000];
            int end = stack.back();
            Read(buf, Min(999, end - dtaSeek(handle, 0, SEEK_CUR)));
            s = buf;
            Descend();
        }
    }

    // The file's handle, DTA_INVALID_HANDLE if none is open (the name is ours): C_mission::OpenBin hands it to the
    // lightmaps, which read their data from the file themselves
    // FUNCTION: GAME 0x548480
    int GetHandle() const
    {
        return open ? handle : DTA_INVALID_HANDLE;
    }

    float ReadFloat()
    {
        float value;
        Read(&value, sizeof(value));
        Descend();
        return value;
    }

    // Reads the current chunk's dword and leaves the chunk (the name is ours)
    // FUNCTION: GAME 0x548490
    int ReadDword()
    {
        int value;
        Read(&value, sizeof(value));
        Descend();
        return value;
    }
};

// The stack's out-of-line functions: push_back's insert(P, x), which the parts' Load calls, and the insert(P, n, x)
// it calls, which RAscend's copies call where they inline the first. The original kept insert(P, x) from the range at
// 0x401030, ours is C_effects.cpp's, which inlines _Destroy (see RAscend), and the original copies the value into a
// stack slot for the element's construction
// TEMPLATE: GAME 0x403740
// ?insert@?$vector@HV?$C_chunk_allocator@H@@@std@@QAEPAHPAHABH@Z

// VTABLE: GAME 0x6233dc
// class C_chunk_allocator<int>

// TEMPLATE: GAME 0x410c00
// C_chunk_allocator<int>::allocate

// TEMPLATE: GAME 0x410c20
// C_chunk_allocator<int>::deallocate

// TEMPLATE: GAME 0x44b1f0
// std::vector<int,C_chunk_allocator<int> >::~vector<int,C_chunk_allocator<int> >

// SYNTHETIC: GAME 0x44b240
// C_chunk_allocator<int>::C_chunk_allocator<int>

// TEMPLATE: GAME 0x44b2a0
// ?insert@?$vector@HV?$C_chunk_allocator@H@@@std@@QAEXPAHIABH@Z

// TEMPLATE: GAME 0x598390
// ?size@?$vector@PAVC_program@@V?$allocator@PAVC_program@@@std@@@std@@QBEIXZ
