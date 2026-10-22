// The index buffers (PS2 IBManager.cpp, __FILE__ C:\MyProjects\engine8\I3D\IBManager.cpp), at 0x1005aad0-0x1005c760
// in the original, then the multisets' out-of-line functions up to 0x1005d010. PS2 has C_IBuffer's constructor,
// destructor and m_Init and C_IBManager's functions; the PC's C_IBuffer also shares its buffer out (m_Alloc, m_Free).
// The header's zero vector (0x101b50ec) has its $E at 0x1005c740, among the multisets' functions (ours comes earlier
// among them: see tools/autopch.py).
#include "I3D/render/IBManager.h"
#include "IGraph/IGraph.h"

#include <dxerr8.h>

// IGraph.cpp
extern IDirect3DDevice8 *m_pD3DDevice8;

// Logs a failed DirectX call with the original's __FILE__; the line is passed in
#define DX_ERROR(hr, line) dbgPrintf("!! Error detected: %s, File: %s, Line: %i !!", DXGetErrorString8(hr), \
    "C:\\MyProjects\\engine8\\I3D\\IBManager.cpp", line)

// GLOBAL: LS3DF 0x101b50f8
C_IBManager cIBMan;

// Inlined into C_IBManager::m_Alloc, its out-of-line copy dropped; the multiset's destructor its unwinding needs
// follows it (0x1005ab00)
C_IBuffer::C_IBuffer()
{
    data = NULL;
    flags = 0;
    usage = 0;
    ib = NULL;
    num_indices = 0;
    num_free = 0;
    pool = D3DPOOL_MANAGED;
    lock_index = -1;
    used_blocks.SetGrowSize(256);
    used_blocks.reserve(256);
}

// Clears the COMPLETE_IB_STRUCT of every block still given out
// Not matching yet: the original's inlined ~multiset calls its last begin() out of line (a little inline budget short).
// An empty destructor declared in S_IB_block gives that (and the registers), with either loop's delete going
// through it, but then VC6 tests the pointer before each delete, which the original doesn't. An inline call of about
// 4 dead statements at the start or the end gives it too; zero-cost inline calls, Unlock() for the if (data) block and
// the loops' spellings don't.
// FUNCTION: LS3DF 0x1005abf0
C_IBuffer::~C_IBuffer()
{
    S_IB_block **p = &used_blocks[0];
    for (int i = used_blocks.size(); i--; p++) {
        memset((*p)->owner, 0, sizeof(COMPLETE_IB_STRUCT));
        delete *p;
    }
    std::multiset<S_IB_block *, S_IB_block_less>::iterator it = free_blocks.begin();
    while (it != free_blocks.end()) {
        delete *it;
        it++;
    }
    if (data) {
        ib->Unlock();
        if (!(flags & D3DLOCK_READONLY)) {
            ib->Lock(0, 0, &data, 0);
            ib->Unlock();
        }
        data = NULL;
        cIBMan.locked_buffers[cIBMan.locked_buffers.size() - 1]->lock_index = lock_index;
        cIBMan.locked_buffers.erase(lock_index);
        lock_index = -1;
    }
    if (ib)
        ib->Release();
    if (pI3DDriver)
        pI3DDriver->m_RecomputeUsedVRAM();
}

// Creates the buffer, at least 0x10000 indices, in the managed pool if it is static and processed in software, and
// locks it. Out of video memory it evicts managed resources and tries again, then tries the managed pool.
// FUNCTION: LS3DF 0x1005ae00
bool C_IBuffer::m_Init(uint usage, uint count)
{
    this->usage = usage;
    if (count < 0x10000)
        count = 0x10000;
    num_indices = count;
    num_free = count;
    if (!(usage & D3DUSAGE_DYNAMIC) && (usage & D3DUSAGE_SOFTWAREPROCESSING))
        pool = D3DPOOL_MANAGED;
    else
        pool = D3DPOOL_DEFAULT;
    HRESULT hr = m_pD3DDevice8->CreateIndexBuffer(num_indices * 2, this->usage, D3DFMT_INDEX16, pool, &ib);
    if (FAILED(hr)) {
        m_pD3DDevice8->ResourceManagerDiscardBytes(0);
        hr = m_pD3DDevice8->CreateIndexBuffer(num_indices * 2, this->usage, D3DFMT_INDEX16, pool, &ib);
        if (FAILED(hr)) {
            pool = D3DPOOL_MANAGED;
            hr = m_pD3DDevice8->CreateIndexBuffer(num_indices * 2, this->usage, D3DFMT_INDEX16, pool, &ib);
            if (FAILED(hr)) {
                DX_ERROR(hr, 129);
                ib = NULL;
                ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                return false;
            }
        }
    }
    free_blocks.insert(new S_IB_block(NULL, 0, num_indices));
    pI3DDriver->m_RecomputeUsedVRAM();
    ib->Lock(0, 0, &data, this->usage & D3DUSAGE_DYNAMIC ? D3DLOCK_DISCARD : 0);
    lock_index = cIBMan.locked_buffers.size();
    cIBMan.locked_buffers.push_back(this);
    return true;
}

// Gives out count indices from the smallest free block that has room, and returns the first one's index, or -1
// FUNCTION: LS3DF 0x1005aff0
int C_IBuffer::m_Alloc(uint count, COMPLETE_IB_STRUCT *owner)
{
    std::multiset<S_IB_block *, S_IB_block_less>::iterator it;
    for (it = free_blocks.begin(); it != free_blocks.end(); it++) {
        S_IB_block *block = *it;
        int rest = block->size - count;
        if (rest >= 0) {
            block->owner = owner;
            used_blocks.push_back(block);
            free_blocks.erase(it);
            block->size = count;
            num_free -= count;
            if (rest > 0)
                free_blocks.insert(new S_IB_block(NULL, block->index + count, rest));
            return block->index;
        }
    }
    return -1;
}

// Takes back the block that starts at index, joined with the free blocks next to it
// FUNCTION: LS3DF 0x1005b1b0
bool C_IBuffer::m_Free(uint index)
{
    S_IB_block **p = &used_blocks[0];
    for (int i = 0; i < used_blocks.size(); i++, p++) {
        S_IB_block *block = *p;
        if (block->index == index) {
            num_free += block->size;
            used_blocks.erase(i);
            uint start = block->index;
            uint size = block->size;
            uchar joined = 0;                   // 1: joined with the next block, 2: with the previous one
            // ++it, where the other loops have it++: each gives the original's inlining
            std::multiset<S_IB_block *, S_IB_block_less>::iterator it;
        restart:
            for (it = free_blocks.begin(); it != free_blocks.end(); ++it) {
                S_IB_block *b = *it;
                if (b->index == start + size) {
                    size += b->size;
                    free_blocks.erase(it);
                    delete b;
                    joined |= 1;
                    if (joined != 3)
                        goto restart;
                    break;
                }
                if (b->index + b->size == start) {
                    start = b->index;
                    size += b->size;
                    free_blocks.erase(it);
                    delete b;
                    joined |= 2;
                    if (joined != 3)
                        goto restart;
                    break;
                }
            }
            if (joined > 0) {
                block->index = start;
                block->size = size;
            }
            free_blocks.insert(block);
            return true;
        }
    }
    return false;
}

// FUNCTION: LS3DF 0x1005b390
C_IBManager::C_IBManager()
{
    locked_buffers.reserve(256);
    locked_buffers.SetGrowSize(256);
}

// FUNCTION: LS3DF 0x1005b5c0
C_IBManager::~C_IBManager()
{
    std::multiset<C_IBuffer *, S_IBuffer_less>::iterator it;
    for (it = buffers.begin(); it != buffers.end(); it++)
        delete *it;
    buffers.clear();
}

// Gives out count indices of a buffer of the usage, from the first that has room, or from a new one
// FUNCTION: LS3DF 0x1005b6c0
bool C_IBManager::m_Alloc(uint usage, uint count, COMPLETE_IB_STRUCT &ib)
{
    std::multiset<C_IBuffer *, S_IBuffer_less>::iterator it;
    for (it = buffers.begin(); it != buffers.end(); it++) {
        C_IBuffer *buffer = *it;
        if (buffer->usage == usage && buffer->num_free >= count) {
            int base = buffer->m_Alloc(count, &ib);
            if (base != -1) {
                ib.buffer = buffer;
                ib.base = base;
                ib.ib = buffer->ib;
                return true;
            }
        }
    }
    C_IBuffer *buffer = new C_IBuffer;
    if (!buffer->m_Init(usage, count)) {
        delete buffer;
        return false;
    }
    buffers.insert(buffer);
    ib.buffer = buffer;
    ib.base = buffer->m_Alloc(count, &ib);
    ib.ib = buffer->ib;
    return true;
}

// FUNCTION: LS3DF 0x1005b940
void C_IBManager::m_Free(COMPLETE_IB_STRUCT &ib)
{
    if (ib.buffer)
        ib.buffer->m_Free(ib.base);
}

// FUNCTION: LS3DF 0x1005b960
void C_IBManager::m_UnlockAllIBuffers()
{
    C_IBuffer **p = &locked_buffers[0];
    for (int i = locked_buffers.size(); i--; p++) {
        (*p)->ib->Unlock();
        if (!((*p)->flags & 0x10)) {
            (*p)->ib->Lock(0, 0, &(*p)->data, 0);
            (*p)->ib->Unlock();
        }
        (*p)->data = NULL;
        (*p)->lock_index = -1;
    }
    locked_buffers.reset();
}

// Deletes the buffers that have nothing given out
// FUNCTION: LS3DF 0x1005b9d0
void C_IBManager::m_FreeAllUnusedIBuffers()
{
    std::multiset<C_IBuffer *, S_IBuffer_less>::iterator it;
restart:
    for (it = buffers.begin(); it != buffers.end(); it++) {
        if ((*it)->num_free == (*it)->num_indices) {
            delete *it;
            buffers.erase(it);
            goto restart;
        }
    }
}

// FUNCTION: LS3DF 0x1005ba30
void C_IBManager::m_FreeAllIBuffers()
{
    std::multiset<C_IBuffer *, S_IBuffer_less>::iterator it;
restart:
    for (it = buffers.begin(); it != buffers.end(); it++) {
        if ((*it)->used_blocks.size() || (*it)->free_blocks.size() != 1)
            dbgPrintf("C_IBManager::m_FreeAllIBManager() - WARNING! Releasing unreleased IndexBuffer !!! Some of meshes "
                "are still unreleased ?!");
        delete *it;
        buffers.erase(it);
        goto restart;
    }
}

// Deletes the buffers in the default pool, which a reset device loses
// FUNCTION: LS3DF 0x1005baa0
void C_IBManager::m_FreeAllVRAMIBuffers()
{
    std::multiset<C_IBuffer *, S_IBuffer_less>::iterator it;
restart:
    for (it = buffers.begin(); it != buffers.end(); it++) {
        if ((*it)->pool == D3DPOOL_DEFAULT) {
            delete *it;
            buffers.erase(it);
            goto restart;
        }
    }
}
