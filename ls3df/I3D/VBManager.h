#pragma once

#include "I3D/I3D.h"

#include <set>

// The vertex buffers (PS2 VBManager.cpp), the same design as IBManager.h's index buffers, with buffers per vertex
// format and usage. PS2 names the classes, COMPLETE_VB_STRUCT, VERTEX_FORMAT and C_VBManager's functions.

class C_VBuffer;

// A block of one of the vertex buffers, as the engine uses it (PS2 name)
struct COMPLETE_VB_STRUCT {
    C_VBuffer *buffer;                  // NULL if none
    IDirect3DVertexBuffer8 *vb;
    int base;                           // the block's first vertex
};

// A vertex buffer's format (PS2 type; the values aren't known yet)
enum VERTEX_FORMAT {
    VF_UNKNOWN_0,
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
    C_list<C_VBuffer *> locked_buffers;
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

// One vertex buffer, 0x4c bytes (PS2 name). It stays locked, in C_VBManager's list, until
// C_VBManager::m_UnlockAllVBuffers.
class C_VBuffer {
public:
    uchar *data;                        // the locked buffer, NULL if unlocked
    uint flags;                         // as C_IBuffer's (nothing reads it here)
    uint fvf;                           // D3DFVF_*
    VERTEX_FORMAT vertex_format;
    D3DPOOL pool;
    uint usage;                         // D3DUSAGE_*
    uint stride;                        // the size of a vertex
    IDirect3DVertexBuffer8 *vb;
    std::multiset<S_VB_block *, S_VB_block_less> free_blocks;
    C_list<S_VB_block *> used_blocks;
    uint num_vertices;                  // the buffer's size
    uint num_free;                      // the vertices in free blocks
    int lock_index;                     // its place in C_VBManager's list of locked buffers, -1 if unlocked

    C_VBuffer();
    ~C_VBuffer();
    bool m_Init(uint fvf, uint usage, uint stride, uint count, VERTEX_FORMAT format);
    int m_Alloc(uint count, COMPLETE_VB_STRUCT *owner);
    bool m_Free(uint index);
};

inline bool S_VBuffer_less::operator()(C_VBuffer *const &b1, C_VBuffer *const &b2) const
{
    return b1->num_free < b2->num_free;
}

// PS2 name
extern C_VBManager cVBMan;

// The multisets' out-of-line functions (VC6's <xtree>), in the original's order. The destructors follow the
// constructors whose unwinding needs them; the rest comes after VBManager.cpp's functions, the buffer tree's before
// the blocks', interrupted by the header's zero vector's $E (0x10069de0). The copies that don't depend on the tree
// are folded with IBManager.cpp's (IBManager.h).
// TEMPLATE: LS3DF 0x10068a00
// ??1?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@QAE@XZ

// TEMPLATE: LS3DF 0x100693f0
// ??1?$multiset@PAVC_VBuffer@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@std@@@std@@QAE@XZ

// TEMPLATE: LS3DF 0x100699d0
// ?erase@?$_Tree@PAVC_VBuffer@@PAV1@U_Kfn@?$multiset@PAVC_VBuffer@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@std@@@std@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@4@@std@@QAE?AViterator@12@V312@@Z

// TEMPLATE: LS3DF 0x10069e00
// ?erase@?$_Tree@PAVC_VBuffer@@PAV1@U_Kfn@?$multiset@PAVC_VBuffer@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@std@@@std@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@4@@std@@QAE?AViterator@12@V312@0@Z

// TEMPLATE: LS3DF 0x10069ed0
// ?_Inc@const_iterator@?$_Tree@PAVC_VBuffer@@PAV1@U_Kfn@?$multiset@PAVC_VBuffer@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@std@@@std@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@4@@std@@QAEXXZ

// TEMPLATE: LS3DF 0x10069f20
// ?insert@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@4@@std@@QAE?AU?$pair@Viterator@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multisedc50274a

// TEMPLATE: LS3DF 0x1006a160
// ?erase@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@4@@std@@QAE?AViterator@12@V312@@Z

// TEMPLATE: LS3DF 0x1006a570
// ?_Inc@const_iterator@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@4@@std@@QAEXXZ

// TEMPLATE: LS3DF 0x1006a5c0
// ?_Erase@?$_Tree@PAVC_VBuffer@@PAV1@U_Kfn@?$multiset@PAVC_VBuffer@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@std@@@std@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@4@@std@@IAEXPAU_Node@12@@Z

// TEMPLATE: LS3DF 0x1006a600
// ?_Insert@?$_Tree@PAVC_VBuffer@@PAV1@U_Kfn@?$multiset@PAVC_VBuffer@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@std@@@std@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@4@@std@@IAE?AViterator@12@PAU_Node@12@0ABQAVC_VBuffer@@@Z

// TEMPLATE: LS3DF 0x1006a870
// ?_Min@?$_Tree@PAVC_VBuffer@@PAV1@U_Kfn@?$multiset@PAVC_VBuffer@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@std@@@std@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@4@@std@@KIPAU_Node@12@PAU312@@Z

// TEMPLATE: LS3DF 0x1006a890
// ?_Erase@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@4@@std@@IAEXPAU_Node@12@@Z

// TEMPLATE: LS3DF 0x1006a8d0
// ?_Insert@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@4@@std@@IAE?AViterator@12@PAU_Node@12@0ABQAUS_VB_block@@@Z

// TEMPLATE: LS3DF 0x1006ab40
// ?_Lrotate@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@4@@std@@IAEXPAU_Node@12@@Z

// TEMPLATE: LS3DF 0x1006aba0
// ?_Min@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@4@@std@@KIPAU_Node@12@PAU312@@Z

// TEMPLATE: LS3DF 0x1006abc0
// ?_Rrotate@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@4@@std@@IAEXPAU_Node@12@@Z

// TEMPLATE: LS3DF 0x1006ac20
// ??Eiterator@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@4@@std@@QAE?AV012@H@Z

// TEMPLATE: LS3DF 0x1006ac80
// ?_Dec@const_iterator@?$_Tree@PAVC_VBuffer@@PAV1@U_Kfn@?$multiset@PAVC_VBuffer@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@std@@@std@@US_VBuffer_less@@V?$allocator@PAVC_VBuffer@@@4@@std@@QAEXXZ

// TEMPLATE: LS3DF 0x1006ace0
// ?_Dec@const_iterator@?$_Tree@PAUS_VB_block@@PAU1@U_Kfn@?$multiset@PAUS_VB_block@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@std@@@std@@US_VB_block_less@@V?$allocator@PAUS_VB_block@@@4@@std@@QAEXXZ

// The trees' empty node and its reference count, after cVBMan in .bss
// GLOBAL: LS3DF 0x101c5034
// std::_Tree<S_VB_block *,S_VB_block *,std::multiset<S_VB_block *,S_VB_block_less,std::allocator<S_VB_block *> >::_Kfn,S_VB_block_less,std::allocator<S_VB_block *> >::_Nilrefs

// GLOBAL: LS3DF 0x101c5038
// std::_Tree<S_VB_block *,S_VB_block *,std::multiset<S_VB_block *,S_VB_block_less,std::allocator<S_VB_block *> >::_Kfn,S_VB_block_less,std::allocator<S_VB_block *> >::_Nil

// GLOBAL: LS3DF 0x101c503c
// std::_Tree<C_VBuffer *,C_VBuffer *,std::multiset<C_VBuffer *,S_VBuffer_less,std::allocator<C_VBuffer *> >::_Kfn,S_VBuffer_less,std::allocator<C_VBuffer *> >::_Nilrefs

// GLOBAL: LS3DF 0x101c5040
// std::_Tree<C_VBuffer *,C_VBuffer *,std::multiset<C_VBuffer *,S_VBuffer_less,std::allocator<C_VBuffer *> >::_Kfn,S_VBuffer_less,std::allocator<C_VBuffer *> >::_Nil
