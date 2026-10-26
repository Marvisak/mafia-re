#pragma once

#include "I3D/I3D.h"

#include <set>
#if ENGINE_VERSION < 395
#include <list>
#endif

// The index buffers (PS2 IBManager.cpp, here IBManager.cpp). The engine shares a few large index buffers out in
// blocks: C_IBManager keeps the buffers, each C_IBuffer its free blocks, ordered by size, and the blocks it gave out.
// VBManager.h has the same for vertex buffers. PS2 names the classes, COMPLETE_IB_STRUCT and C_IBManager's
// functions; the rest is named here. Insanity3D (the same lineage's later engine) has a manager of the same design
// (Vb_mgr.h: blocks in a multiset ordered by size), written anew. 1.1's buffers keep their blocks in a std::list and no
// list of the locked buffers: a buffer stays locked until C_IBManager::m_UnlockAllIBuffers unlocks every buffer,
// always with D3DLOCK_NOSYSLOCK.

// A block of a C_IBuffer, free or given out
struct S_IB_block {
    COMPLETE_IB_STRUCT *owner;          // whom it is given out to (cleared when the buffer goes)
    uint index;                         // its first index
    uint size;                          // its number of indices

    S_IB_block(COMPLETE_IB_STRUCT *owner1, uint index1, uint size1) : owner(owner1), index(index1), size(size1) {}
};

// The order of a buffer's free blocks: the smallest first
struct S_IB_block_less {
    bool operator()(S_IB_block *const &b1, S_IB_block *const &b2) const
    {
        return b1->size < b2->size;
    }
};

// The order of the manager's buffers: the least free space first, as of insertion (m_Alloc doesn't reinsert a buffer
// it allocates from)
struct S_IBuffer_less {
    bool operator()(C_IBuffer *const &b1, C_IBuffer *const &b2) const;
};

// The index buffers (PS2 name)
class C_IBManager {
public:
#if ENGINE_VERSION >= 395
    C_list<C_IBuffer *> locked_buffers;
#endif
    std::multiset<C_IBuffer *, S_IBuffer_less> buffers;

    C_IBManager();
    ~C_IBManager();
    bool m_Alloc(uint usage, uint count, COMPLETE_IB_STRUCT &ib);
    void m_Free(COMPLETE_IB_STRUCT &ib);
    void m_UnlockAllIBuffers();
    void m_FreeAllUnusedIBuffers();
    void m_FreeAllIBuffers();
    void m_FreeAllVRAMIBuffers();
};

// One index buffer of 16-bit indices, 0x40 bytes (PS2 name; 1.1's 0x34). It stays locked, in C_IBManager's list,
// until C_IBManager::m_UnlockAllIBuffers.
class C_IBuffer {
public:
    uchar *data;                        // the locked buffer, NULL if unlocked
#if ENGINE_VERSION >= 395
    uint flags;                         // the D3DLOCK_* flags it is locked with
#endif
    uint usage;                         // D3DUSAGE_*
    D3DPOOL pool;
    IDirect3DIndexBuffer8 *ib;
    std::multiset<S_IB_block *, S_IB_block_less> free_blocks;
#if ENGINE_VERSION >= 395
    C_list<S_IB_block *> used_blocks;
#else
    std::list<S_IB_block *> used_blocks;
#endif
    uint num_indices;                   // the buffer's size
    uint num_free;                      // the indices in free blocks
#if ENGINE_VERSION >= 395
    int lock_index;                     // its place in C_IBManager's list of locked buffers, -1 if unlocked
#endif

    C_IBuffer();
    ~C_IBuffer();
    bool m_Init(uint usage, uint count);
    int m_Alloc(uint count, COMPLETE_IB_STRUCT *owner);
    bool m_Free(uint index);

    // Locks the buffer if it isn't, and returns the index's address (names guessed)
    uchar *Lock(int index, uint lock_flags);
    void Unlock();
};

inline bool S_IBuffer_less::operator()(C_IBuffer *const &b1, C_IBuffer *const &b2) const
{
    return b1->num_free < b2->num_free;
}

// PS2 name
extern C_IBManager cIBMan;

// Its initializer's thunk and the destructor it registers: the first $E11 in link order, annotated for T&l.cpp's after
// it, and the second $E9, after the collision file's
// SYNTHETIC: LS3DF 0x1005aad0
// $E11

// SYNTHETIC: LS3DF11 0x10015050
// $E12

// SYNTHETIC: LS3DF11 0x10032bb0
// $E12

// SYNTHETIC: LS3DF11 0x10058180
// $E12

// SYNTHETIC: LS3DF 0x1005aaf0
// $E9

// SYNTHETIC: LS3DF11 0x100581a0
// $E10

#if ENGINE_VERSION >= 395
// A buffer locked read-only is locked again for writing
inline uchar *C_IBuffer::Lock(int index, uint lock_flags)
{
    if (data) {
        if (!(flags & D3DLOCK_READONLY) || lock_flags & D3DLOCK_READONLY)
            return data + index * sizeof(ushort);
        ib->Unlock();
    }
    flags = lock_flags;
    ib->Lock(0, 0, &data, lock_flags);
    if (lock_index < 0) {
        lock_index = cIBMan.locked_buffers.size();
        cIBMan.locked_buffers.push_back(this);
    }
    return data + index * sizeof(ushort);
}
#else
// 1.1's locks only an unlocked buffer, with the caller's flags (BUFFER_LOCK_FLAGS, I3D.h)
inline uchar *C_IBuffer::Lock(int index, uint lock_flags)
{
    if (data)
        return data + index * sizeof(ushort);
    ib->Lock(0, 0, &data, lock_flags);
    return data + index * sizeof(ushort);
}
#endif

// 1.2's locks and unlocks a buffer that wasn't locked read-only once more
inline void C_IBuffer::Unlock()
{
    if (data) {
        ib->Unlock();
#if ENGINE_VERSION >= 395
        if (!(flags & D3DLOCK_READONLY)) {
            ib->Lock(0, 0, &data, 0);
            ib->Unlock();
        }
#endif
        data = NULL;
#if ENGINE_VERSION >= 395
        cIBMan.locked_buffers[cIBMan.locked_buffers.size() - 1]->lock_index = lock_index;
        cIBMan.locked_buffers.erase(lock_index);
        lock_index = -1;
#endif
    }
}

#if ENGINE_VERSION < 395
// Inline in 1.1: the meshes call C_IBuffer::m_Free themselves
inline void C_IBManager::m_Free(COMPLETE_IB_STRUCT &ib)
{
    if (ib.buffer)
        ib.buffer->m_Free(ib.base);
}
#endif

// The multisets' out-of-line functions (VC6's <xtree>), in the original's order. The destructors follow the
// constructors whose unwinding needs them; the rest comes after IBManager.cpp's functions, the buffer tree's before
// the blocks', interrupted by the header's zero vector's $E (0x1005c740). The copies that don't depend on the tree
// (begin, _Construct, the pair's constructor, _Buynode, C_list::reserve) are folded with VBManager.cpp's, and which of
// the names our PDB keeps changes from one build to the next: annotate the one it has. In both files operator++(int)
// and the buffer tree's _Inc don't come where the original's do yet.
// TEMPLATE: LS3DF 0x1005ab00
// TEMPLATE: LS3DF11 0x100581b0
// ??1?$multiset@PAUS_IB_block@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@std@@@std@@QAE@XZ

// TEMPLATE: LS3DF 0x1005b4d0
// TEMPLATE: LS3DF11 0x10058bd0
// ??1?$multiset@PAVC_IBuffer@@US_IBuffer_less@@V?$allocator@PAVC_IBuffer@@@std@@@std@@QAE@XZ

// TEMPLATE: LS3DF 0x1005bb20
// ?reserve@?$C_list@PAUS_IB_block@@@@QAEXH@Z

// TEMPLATE: LS3DF 0x1005bb80
// ?begin@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@4@@std@@QAE?AViterator@12@XZ

// TEMPLATE: LS3DF11 0x10066c50
// ?begin@?$_Tree@PAVC_IBuffer@@PAV1@U_Kfn@?$multiset@PAVC_IBuffer@@US_IBuffer_less@@V?$allocator@PAVC_IBuffer@@@std@@@std@@US_IBuffer_less@@V?$allocator@PAVC_IBuffer@@@4@@std@@QAE?AViterator@12@XZ

// TEMPLATE: LS3DF 0x1005bb90
// TEMPLATE: LS3DF11 0x10059550
// ?erase@?$_Tree@PAVC_IBuffer@@PAV1@U_Kfn@?$multiset@PAVC_IBuffer@@US_IBuffer_less@@V?$allocator@PAVC_IBuffer@@@std@@@std@@US_IBuffer_less@@V?$allocator@PAVC_IBuffer@@@4@@std@@QAE?AViterator@12@V312@@Z

// TEMPLATE: LS3DF 0x1005bfa0
// TEMPLATE: LS3DF11 0x10059960
// ?erase@?$_Tree@PAVC_IBuffer@@PAV1@U_Kfn@?$multiset@PAVC_IBuffer@@US_IBuffer_less@@V?$allocator@PAVC_IBuffer@@@std@@@std@@US_IBuffer_less@@V?$allocator@PAVC_IBuffer@@@4@@std@@QAE?AViterator@12@V312@0@Z

// TEMPLATE: LS3DF 0x1005c070
// TEMPLATE: LS3DF11 0x10059a30
// ?_Inc@const_iterator@?$_Tree@PAVC_IBuffer@@PAV1@U_Kfn@?$multiset@PAVC_IBuffer@@US_IBuffer_less@@V?$allocator@PAVC_IBuffer@@@std@@@std@@US_IBuffer_less@@V?$allocator@PAVC_IBuffer@@@4@@std@@QAEXXZ

// TEMPLATE: LS3DF 0x1005c0c0
// ??0?$_Tree@PAUS_IB_block@@PAU1@U_Kfn@?$multiset@PAUS_IB_block@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@std@@@std@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@4@@std@@QAE@ABUS_IB_block_less@@_NABV?$allocator@PAUS_IB_block@@@1@@Z

// TEMPLATE: LS3DF 0x1005c0f0
// ?insert@?$_Tree@PAUS_IB_block@@PAU1@U_Kfn@?$multiset@PAUS_IB_block@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@std@@@std@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@4@@std@@QAE?AU?$pair@Viterator@?$_Tree@PAUS_IB_block@@PAU1@U_Kfn@?$multisefcbf76eb

// TEMPLATE: LS3DF 0x1005c330
// TEMPLATE: LS3DF11 0x100590f0
// ?erase@?$_Tree@PAUS_IB_block@@PAU1@U_Kfn@?$multiset@PAUS_IB_block@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@std@@@std@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@4@@std@@QAE?AViterator@12@V312@@Z

// TEMPLATE: LS3DF 0x1005c760
// TEMPLATE: LS3DF11 0x10059500
// ?_Inc@const_iterator@?$_Tree@PAUS_IB_block@@PAU1@U_Kfn@?$multiset@PAUS_IB_block@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@std@@@std@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@4@@std@@QAEXXZ

// TEMPLATE: LS3DF 0x1005c7b0
// TEMPLATE: LS3DF11 0x10059e60
// ?_Erase@?$_Tree@PAVC_IBuffer@@PAV1@U_Kfn@?$multiset@PAVC_IBuffer@@US_IBuffer_less@@V?$allocator@PAVC_IBuffer@@@std@@@std@@US_IBuffer_less@@V?$allocator@PAVC_IBuffer@@@4@@std@@IAEXPAU_Node@12@@Z

// TEMPLATE: LS3DF 0x1005c7f0
// TEMPLATE: LS3DF11 0x10059ea0
// ?_Insert@?$_Tree@PAVC_IBuffer@@PAV1@U_Kfn@?$multiset@PAVC_IBuffer@@US_IBuffer_less@@V?$allocator@PAVC_IBuffer@@@std@@@std@@US_IBuffer_less@@V?$allocator@PAVC_IBuffer@@@4@@std@@IAE?AViterator@12@PAU_Node@12@0ABQAVC_IBuffer@@@Z

// TEMPLATE: LS3DF 0x1005ca60
// TEMPLATE: LS3DF11 0x1005a110
// ?_Min@?$_Tree@PAVC_IBuffer@@PAV1@U_Kfn@?$multiset@PAVC_IBuffer@@US_IBuffer_less@@V?$allocator@PAVC_IBuffer@@@std@@@std@@US_IBuffer_less@@V?$allocator@PAVC_IBuffer@@@4@@std@@KIPAU_Node@12@PAU312@@Z

// TEMPLATE: LS3DF 0x1005ca80
// TEMPLATE: LS3DF11 0x10059a80
// ?_Erase@?$_Tree@PAUS_IB_block@@PAU1@U_Kfn@?$multiset@PAUS_IB_block@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@std@@@std@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@4@@std@@IAEXPAU_Node@12@@Z

// TEMPLATE: LS3DF 0x1005cac0
// TEMPLATE: LS3DF11 0x10059ac0
// ?_Init@?$_Tree@PAUS_IB_block@@PAU1@U_Kfn@?$multiset@PAUS_IB_block@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@std@@@std@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@4@@std@@IAEXXZ

// TEMPLATE: LS3DF 0x1005cb50
// TEMPLATE: LS3DF11 0x10059b50
// ?_Insert@?$_Tree@PAUS_IB_block@@PAU1@U_Kfn@?$multiset@PAUS_IB_block@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@std@@@std@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@4@@std@@IAE?AViterator@12@PAU_Node@12@0ABQAUS_IB_block@@@Z

// TEMPLATE: LS3DF 0x1005cdc0
// ?_Lrotate@?$_Tree@PAUS_IB_block@@PAU1@U_Kfn@?$multiset@PAUS_IB_block@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@std@@@std@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@4@@std@@IAEXPAU_Node@12@@Z

// TEMPLATE: LS3DF 0x1005ce20
// TEMPLATE: LS3DF11 0x10059de0
// ?_Min@?$_Tree@PAUS_IB_block@@PAU1@U_Kfn@?$multiset@PAUS_IB_block@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@std@@@std@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@4@@std@@KIPAU_Node@12@PAU312@@Z

// TEMPLATE: LS3DF 0x1005ce40
// ?_Rrotate@?$_Tree@PAUS_IB_block@@PAU1@U_Kfn@?$multiset@PAUS_IB_block@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@std@@@std@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@4@@std@@IAEXPAU_Node@12@@Z

// TEMPLATE: LS3DF 0x1005cea0
// TEMPLATE: LS3DF11 0x10059e00
// ??Eiterator@?$_Tree@PAUS_IB_block@@PAU1@U_Kfn@?$multiset@PAUS_IB_block@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@std@@@std@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@4@@std@@QAE?AV012@H@Z

// 1.1's buffer tree's operator++(int), out of line where ~C_IBManager calls it
// TEMPLATE: LS3DF11 0x1005a130
// ??Eiterator@?$_Tree@PAVC_IBuffer@@PAV1@U_Kfn@?$multiset@PAVC_IBuffer@@US_IBuffer_less@@V?$allocator@PAVC_IBuffer@@@std@@@std@@US_IBuffer_less@@V?$allocator@PAVC_IBuffer@@@4@@std@@QAE?AV012@H@Z

// TEMPLATE: LS3DF 0x1005cf00
// ??0?$pair@Viterator@?$_Tree@PAVC_IBuffer@@PAV1@U_Kfn@?$multiset@PAVC_IBuffer@@US_IBuffer_less@@V?$allocator@PAVC_IBuffer@@@std@@@std@@US_IBuffer_less@@V?$allocator@PAVC_IBuffer@@@4@@std@@_N@std@@QAE@ABViterator@?$_Tree@PAVC_IBuffer@@PAV1@U_Kfn@?$ma0ee93cf

// TEMPLATE: LS3DF11 0x1005a190
// ??0?$pair@Viterator@?$_Tree@PAVC_VBuffer@@PAV1@U_Kfn@?$multiset@PAVC_VBuffer@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@std@@@std@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@4@@std@@_N@std@@QAE@ABViterator@?$_Tree@PAVC_VBuffer@@PAV1@U_Kfn@?$m9d95d36c

// TEMPLATE: LS3DF 0x1005cf20
// ?_Buynode@?$_Tree@PAVC_VBuffer@@PAV1@U_Kfn@?$multiset@PAVC_VBuffer@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@std@@@std@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@4@@std@@IAEPAU_Node@12@PAU312@W4_Redbl@12@@Z

// TEMPLATE: LS3DF 0x1005cf40
// TEMPLATE: LS3DF11 0x1005a210
// ?_Dec@const_iterator@?$_Tree@PAVC_IBuffer@@PAV1@U_Kfn@?$multiset@PAVC_IBuffer@@US_IBuffer_less@@V?$allocator@PAVC_IBuffer@@@std@@@std@@US_IBuffer_less@@V?$allocator@PAVC_IBuffer@@@4@@std@@QAEXXZ

// TEMPLATE: LS3DF 0x1005cfa0
// TEMPLATE: LS3DF11 0x1005a1b0
// ?_Dec@const_iterator@?$_Tree@PAUS_IB_block@@PAU1@U_Kfn@?$multiset@PAUS_IB_block@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@std@@@std@@US_IB_block_less@@V?$allocator@PAUS_IB_block@@@4@@std@@QAEXXZ

// TEMPLATE: LS3DF 0x1005d000
// ?_Construct@std@@YIXPAPAUS_VB_block@@ABQAU2@@Z

// TEMPLATE: LS3DF11 0x1005a270
// ?_Construct@std@@YIXPAPAVC_VBuffer@@ABQAV2@@Z

// The trees' empty node and its reference count, after cIBMan in .bss
// GLOBAL: LS3DF 0x101b5118
// GLOBAL: LS3DF11 0x101b2228
// std::_Tree<S_IB_block *,S_IB_block *,std::multiset<S_IB_block *,S_IB_block_less,std::allocator<S_IB_block *> >::_Kfn,S_IB_block_less,std::allocator<S_IB_block *> >::_Nilrefs

// GLOBAL: LS3DF 0x101b511c
// GLOBAL: LS3DF11 0x101b222c
// std::_Tree<S_IB_block *,S_IB_block *,std::multiset<S_IB_block *,S_IB_block_less,std::allocator<S_IB_block *> >::_Kfn,S_IB_block_less,std::allocator<S_IB_block *> >::_Nil

// GLOBAL: LS3DF 0x101b5120
// GLOBAL: LS3DF11 0x101b2230
// std::_Tree<C_IBuffer *,C_IBuffer *,std::multiset<C_IBuffer *,S_IBuffer_less,std::allocator<C_IBuffer *> >::_Kfn,S_IBuffer_less,std::allocator<C_IBuffer *> >::_Nilrefs

// GLOBAL: LS3DF 0x101b5124
// GLOBAL: LS3DF11 0x101b2234
// std::_Tree<C_IBuffer *,C_IBuffer *,std::multiset<C_IBuffer *,S_IBuffer_less,std::allocator<C_IBuffer *> >::_Kfn,S_IBuffer_less,std::allocator<C_IBuffer *> >::_Nil
