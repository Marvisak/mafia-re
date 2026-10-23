#pragma once

#include "I3D/I3D.h"

#include <set>
#if ENGINE_VERSION < 395
#include <list>
#endif

// The vertex buffers (PS2 VBManager.cpp), the same design as IBManager.h's index buffers, with buffers per vertex
// format and usage. PS2 names the classes, COMPLETE_VB_STRUCT, VERTEX_FORMAT and C_VBManager's functions.
// 1.1's buffers keep their blocks in a std::list and no list of the locked buffers: a buffer stays locked until
// C_VBManager::m_UnlockAllVBuffers unlocks every buffer, always with D3DLOCK_NOSYSLOCK.

// The bytes a buffer holds at least (Insanity3D's name for the same constant of its buffer manager, which computes
// the size the same way)
#define RB_MANAGER_MEM_SIZE 0x20000

// A vertex buffer's format (PS2 type; the names are guesses, 2 and 3 aren't known)
enum VERTEX_FORMAT {
    VF_LVERTEX,                         // LVERTEX: IGraph's, the objects' lods with hardware T&L
    VF_LVERTEX2,                        // LVERTEX2
    VF_TLVERTEX = 4,                    // TLVERTEX: the objects' lods without hardware T&L
    VF_TLVERTEX2,                       // TLVERTEX2
    VF_VERTEX,                          // I3D_vertex: the meshes'
};

// A block of a C_VBuffer, free or given out
struct S_VB_block {
    COMPLETE_VB_STRUCT *owner;          // whom it is given out to (cleared when the buffer goes)
    uint index;                         // its first vertex
    uint size;                          // its number of vertices

    S_VB_block(COMPLETE_VB_STRUCT *owner1, uint index1, uint size1) : owner(owner1), index(index1), size(size1) {}
};

// The order of a buffer's free blocks: the smallest first
struct S_VB_block_less {
    bool operator()(S_VB_block *const &b1, S_VB_block *const &b2) const
    {
        return b1->size < b2->size;
    }
};

// The order of the manager's buffers: the least free space first, as of insertion
struct S_VBuffer_less {
    bool operator()(C_VBuffer *const &b1, C_VBuffer *const &b2) const;
};

// The vertex buffers (PS2 name)
class C_VBManager {
public:
#if ENGINE_VERSION >= 395
    C_list<C_VBuffer *> locked_buffers;
#endif
    std::multiset<C_VBuffer *, S_VBuffer_less> buffers;

    C_VBManager();
    ~C_VBManager();
    bool m_Alloc(uint fvf, uint usage, uint stride, uint count, COMPLETE_VB_STRUCT &vb, VERTEX_FORMAT format,
        bool exclusive);
    void m_Free(COMPLETE_VB_STRUCT &vb);
    void m_UnlockAllVBuffers();
    void m_FreeAllUnusedVBuffers(C_VBuffer *buffer);
    void m_FreeAllVBuffers();
    void m_FreeAllVRAMVBuffers();
};

// One vertex buffer, 0x4c bytes (PS2 name; 1.1's 0x40). It stays locked, in C_VBManager's list, until
// C_VBManager::m_UnlockAllVBuffers.
class C_VBuffer {
public:
    uchar *data;                        // the locked buffer, NULL if unlocked
#if ENGINE_VERSION >= 395
    uint flags;                         // the D3DLOCK_* flags it is locked with
#endif
    uint fvf;                           // D3DFVF_*
    VERTEX_FORMAT vertex_format;
    D3DPOOL pool;
    uint usage;                         // D3DUSAGE_*
    uint stride;                        // the size of a vertex
    IDirect3DVertexBuffer8 *vb;
    std::multiset<S_VB_block *, S_VB_block_less> free_blocks;
#if ENGINE_VERSION >= 395
    C_list<S_VB_block *> used_blocks;
#else
    std::list<S_VB_block *> used_blocks;
#endif
    uint num_vertices;                  // the buffer's size
    uint num_free;                      // the vertices in free blocks
#if ENGINE_VERSION >= 395
    int lock_index;                     // its place in C_VBManager's list of locked buffers, -1 if unlocked
#endif

    C_VBuffer();
    ~C_VBuffer();
    bool m_Init(uint fvf, uint usage, uint stride, uint count, VERTEX_FORMAT format);
    int m_Alloc(uint count, COMPLETE_VB_STRUCT *owner);
    bool m_Free(uint index);

    // Locks the buffer if it isn't, and returns the vertex's address (names guessed)
    uchar *Lock(int index, uint lock_flags);
    void Unlock();
};

inline bool S_VBuffer_less::operator()(C_VBuffer *const &b1, C_VBuffer *const &b2) const
{
    return b1->num_free < b2->num_free;
}

// PS2 name
extern C_VBManager cVBMan;

// The destructor its initializer's thunk registers: the third $E9 in link order, after IBManager.cpp's
// SYNTHETIC: LS3DF 0x100688a0
// $E9

// SYNTHETIC: LS3DF11 0x10065b80
// $E10

#if ENGINE_VERSION >= 395
// A buffer locked read-only is locked again for writing
// FUNCTION: LS3DF 0x1000af80
inline uchar *C_VBuffer::Lock(int index, uint lock_flags)
{
    if (data) {
        if (!(flags & D3DLOCK_READONLY) || lock_flags & D3DLOCK_READONLY)
            return data + stride * index;
        vb->Unlock();
    }
    flags = lock_flags;
    vb->Lock(0, 0, &data, lock_flags);
    if (lock_index < 0) {
        lock_index = cVBMan.locked_buffers.size();
        cVBMan.locked_buffers.push_back(this);
    }
    return data + stride * index;
}
#else
// 1.1's locks only an unlocked buffer, with the caller's flags (BUFFER_LOCK_FLAGS, I3D.h)
// FUNCTION: LS3DF11 0x1000ac10
inline uchar *C_VBuffer::Lock(int index, uint lock_flags)
{
    if (data)
        return data + stride * index;
    vb->Lock(0, 0, &data, lock_flags);
    return data + stride * index;
}
#endif

// FUNCTION: LS3DF 0x1000b060
// FUNCTION: LS3DF11 0x1000ac40
inline void C_VBuffer::Unlock()
{
    if (data) {
        vb->Unlock();
        data = NULL;
#if ENGINE_VERSION >= 395
        cVBMan.locked_buffers[cVBMan.locked_buffers.size() - 1]->lock_index = lock_index;
        cVBMan.locked_buffers.erase(lock_index);
        lock_index = -1;
#endif
    }
}

#if ENGINE_VERSION < 395
// Inline in 1.1: the scene's destructor and the meshes call C_VBuffer::m_Free themselves
inline void C_VBManager::m_Free(COMPLETE_VB_STRUCT &vb)
{
    if (vb.buffer)
        vb.buffer->m_Free(vb.base);
}
#endif

// The multisets' out-of-line functions (VC6's <xtree>), in the original's order. The destructors follow the
// constructors whose unwinding needs them; the rest comes after VBManager.cpp's functions, the buffer tree's before
// the blocks', interrupted by the header's zero vector's $E (0x10069de0). The copies that don't depend on the tree
// are folded with IBManager.cpp's (IBManager.h).
// TEMPLATE: LS3DF 0x10068a00
// TEMPLATE: LS3DF11 0x10065b90
// ??1?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@QAE@XZ

// TEMPLATE: LS3DF 0x100693f0
// TEMPLATE: LS3DF11 0x100665f0
// ??1?$multiset@PAVC_VBuffer@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@std@@@std@@QAE@XZ

// TEMPLATE: LS3DF 0x100699d0
// TEMPLATE: LS3DF11 0x100670f0
// ?erase@?$_Tree@PAVC_VBuffer@@PAV1@U_Kfn@?$multiset@PAVC_VBuffer@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@std@@@std@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@4@@std@@QAE?AViterator@12@V312@@Z

// TEMPLATE: LS3DF 0x10069e00
// TEMPLATE: LS3DF11 0x10067500
// ?erase@?$_Tree@PAVC_VBuffer@@PAV1@U_Kfn@?$multiset@PAVC_VBuffer@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@std@@@std@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@4@@std@@QAE?AViterator@12@V312@0@Z

// TEMPLATE: LS3DF 0x10069ed0
// TEMPLATE: LS3DF11 0x100675d0
// ?_Inc@const_iterator@?$_Tree@PAVC_VBuffer@@PAV1@U_Kfn@?$multiset@PAVC_VBuffer@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@std@@@std@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@4@@std@@QAEXXZ

// TEMPLATE: LS3DF 0x10069f20
// ?insert@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@4@@std@@QAE?AU?$pair@Viterator@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multisedc50274a

// TEMPLATE: LS3DF 0x1006a160
// TEMPLATE: LS3DF11 0x10066c60
// ?erase@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@4@@std@@QAE?AViterator@12@V312@@Z

// TEMPLATE: LS3DF 0x1006a570
// TEMPLATE: LS3DF11 0x10067070
// ?_Inc@const_iterator@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@4@@std@@QAEXXZ

// TEMPLATE: LS3DF 0x1006a5c0
// TEMPLATE: LS3DF11 0x100679e0
// ?_Erase@?$_Tree@PAVC_VBuffer@@PAV1@U_Kfn@?$multiset@PAVC_VBuffer@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@std@@@std@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@4@@std@@IAEXPAU_Node@12@@Z

// TEMPLATE: LS3DF 0x1006a600
// TEMPLATE: LS3DF11 0x10067a20
// ?_Insert@?$_Tree@PAVC_VBuffer@@PAV1@U_Kfn@?$multiset@PAVC_VBuffer@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@std@@@std@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@4@@std@@IAE?AViterator@12@PAU_Node@12@0ABQAVC_VBuffer@@@Z

// TEMPLATE: LS3DF 0x1006a870
// TEMPLATE: LS3DF11 0x10067c90
// ?_Min@?$_Tree@PAVC_VBuffer@@PAV1@U_Kfn@?$multiset@PAVC_VBuffer@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@std@@@std@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@4@@std@@KIPAU_Node@12@PAU312@@Z

// TEMPLATE: LS3DF 0x1006a890
// TEMPLATE: LS3DF11 0x10067620
// ?_Erase@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@4@@std@@IAEXPAU_Node@12@@Z

// TEMPLATE: LS3DF 0x1006a8d0
// TEMPLATE: LS3DF11 0x100676f0
// ?_Insert@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@4@@std@@IAE?AViterator@12@PAU_Node@12@0ABQAUS_VB_block@@@Z

// TEMPLATE: LS3DF 0x1006ab40
// ?_Lrotate@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@4@@std@@IAEXPAU_Node@12@@Z

// TEMPLATE: LS3DF 0x1006aba0
// TEMPLATE: LS3DF11 0x10067960
// ?_Min@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@4@@std@@KIPAU_Node@12@PAU312@@Z

// TEMPLATE: LS3DF 0x1006abc0
// ?_Rrotate@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@4@@std@@IAEXPAU_Node@12@@Z

// TEMPLATE: LS3DF 0x1006ac20
// TEMPLATE: LS3DF11 0x10067980
// ??Eiterator@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@4@@std@@QAE?AV012@H@Z

// 1.1's buffer tree's operator++(int), out of line where ~C_VBManager calls it
// TEMPLATE: LS3DF11 0x10067cb0
// ??Eiterator@?$_Tree@PAVC_VBuffer@@PAV1@U_Kfn@?$multiset@PAVC_VBuffer@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@std@@@std@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@4@@std@@QAE?AV012@H@Z

// TEMPLATE: LS3DF 0x1006ac80
// TEMPLATE: LS3DF11 0x10067d70
// ?_Dec@const_iterator@?$_Tree@PAVC_VBuffer@@PAV1@U_Kfn@?$multiset@PAVC_VBuffer@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@std@@@std@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@4@@std@@QAEXXZ

// TEMPLATE: LS3DF 0x1006ace0
// TEMPLATE: LS3DF11 0x10067d10
// ?_Dec@const_iterator@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@4@@std@@QAEXXZ

// The trees' empty node and its reference count, after cVBMan in .bss
// GLOBAL: LS3DF 0x101c5034
// GLOBAL: LS3DF11 0x101c2134
// std::_Tree<S_VB_block *,S_VB_block *,std::multiset<S_VB_block *,S_VB_block_less,std::allocator<S_VB_block *> >::_Kfn,S_VB_block_less,std::allocator<S_VB_block *> >::_Nilrefs

// GLOBAL: LS3DF 0x101c5038
// GLOBAL: LS3DF11 0x101c2138
// std::_Tree<S_VB_block *,S_VB_block *,std::multiset<S_VB_block *,S_VB_block_less,std::allocator<S_VB_block *> >::_Kfn,S_VB_block_less,std::allocator<S_VB_block *> >::_Nil

// GLOBAL: LS3DF 0x101c503c
// GLOBAL: LS3DF11 0x101c213c
// std::_Tree<C_VBuffer *,C_VBuffer *,std::multiset<C_VBuffer *,S_VBuffer_less,std::allocator<C_VBuffer *> >::_Kfn,S_VBuffer_less,std::allocator<C_VBuffer *> >::_Nilrefs

// GLOBAL: LS3DF 0x101c5040
// GLOBAL: LS3DF11 0x101c2140
// std::_Tree<C_VBuffer *,C_VBuffer *,std::multiset<C_VBuffer *,S_VBuffer_less,std::allocator<C_VBuffer *> >::_Kfn,S_VBuffer_less,std::allocator<C_VBuffer *> >::_Nil

// 1.1's: its C_VBuffer constructor is inlined into C_VBManager::m_Alloc, which calls the block tree's _Init, and its
// std::list of blocks has these out of line, folded with IBManager.h's (both lists of pointers)
// TEMPLATE: LS3DF11 0x10067660
// ?_Init@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@4@@std@@IAEXXZ

// TEMPLATE: LS3DF11 0x10066ba0
// ??1?$list@PAUS_IB_block@@V?$allocator@PAUS_IB_block@@@std@@@std@@QAE@XZ

// TEMPLATE: LS3DF11 0x10066bf0
// ?erase@?$list@PAUS_VB_block@@V?$allocator@PAUS_VB_block@@@std@@@std@@QAE?AViterator@12@V312@@Z

// TEMPLATE: LS3DF11 0x100670c0
// ?_Buynode@?$list@PAUS_VB_block@@V?$allocator@PAUS_VB_block@@@std@@@std@@IAEPAU_Node@12@PAU312@0@Z
