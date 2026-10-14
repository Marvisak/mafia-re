// The vertex buffers (PS2 VBManager.cpp, __FILE__ C:\MyProjects\engine8\I3D\VBManager.cpp), at 0x10068880-0x10069e00
// in the original, then the multisets' out-of-line functions up to 0x1006ad40. The same code as IBManager.cpp, which
// it was probably copied from or to. The header's zero vector (0x101c5008) has its $E at 0x10069de0 and isn't here
// yet.
#include "I3D/VBManager.h"
#include "IGraph/IGraph.h"

#include <dxerr8.h>

// IGraph.cpp
extern IDirect3DDevice8 *m_pD3DDevice8;

// GLOBAL: LS3DF 0x101c5014
C_VBManager cVBMan;

// Out of line in the original, where C_VBManager::m_Alloc calls it; ours is inlined there, which costs m_Alloc most of
// its score. A little more source tips it over VC6's /Ob2 limit for functions not declared inline (two more
// assignments VC6 merges away are enough), so the original's had something these statements don't show.
// FUNCTION: LS3DF 0x100688b0
C_VBuffer::C_VBuffer()
{
    data = NULL;
    flags = 0;
    fvf = 0;
    usage = 0;
    vb = NULL;
    num_vertices = 0;
    num_free = 0;
    pool = D3DPOOL_MANAGED;
    vertex_format = VF_UNKNOWN_0;
    lock_index = -1;
    used_blocks.SetGrowSize(256);
    used_blocks.reserve(256);
}

// Clears the COMPLETE_VB_STRUCT of every block still given out
// FUNCTION: LS3DF 0x10068af0
C_VBuffer::~C_VBuffer()
{
    S_VB_block **p = &used_blocks[0];
    for (int i = used_blocks.size(); i--; p++) {
        memset((*p)->owner, 0, sizeof(COMPLETE_VB_STRUCT));
        delete *p;
    }
    std::multiset<S_VB_block *, S_VB_block_less>::iterator it = free_blocks.begin();
    while (it != free_blocks.end()) {
        delete *it;
        it++;
    }
    if (data) {
        vb->Unlock();
        data = NULL;
        cVBMan.locked_buffers[cVBMan.locked_buffers.size() - 1]->lock_index = lock_index;
        cVBMan.locked_buffers.erase(lock_index);
        lock_index = -1;
    }
    if (vb)
        vb->Release();
    if (pI3DDriver)
        pI3DDriver->m_RecomputeUsedVRAM();
}

// Creates the buffer, at least 128 KB and count vertices, in the managed pool if it is static and processed in
// software, and locks it. Out of video memory it evicts managed resources and tries again, then tries the managed
// pool.
// FUNCTION: LS3DF 0x10068cf0
bool C_VBuffer::m_Init(uint fvf, uint usage, uint stride, uint count, VERTEX_FORMAT format)
{
    this->fvf = fvf;
    vertex_format = format;
    this->usage = usage;
    this->stride = stride;
    uint num = max(0x20000 / stride, count);
    num_vertices = num;
    num_free = num;
    if (!(usage & D3DUSAGE_DYNAMIC) && (usage & D3DUSAGE_SOFTWAREPROCESSING))
        pool = D3DPOOL_MANAGED;
    else
        pool = D3DPOOL_DEFAULT;
    HRESULT hr = m_pD3DDevice8->CreateVertexBuffer(num_vertices * stride, this->usage, this->fvf, pool, &vb);
    if (FAILED(hr)) {
        m_pD3DDevice8->ResourceManagerDiscardBytes(0);
        hr = m_pD3DDevice8->CreateVertexBuffer(num_vertices * stride, this->usage, this->fvf, pool, &vb);
        if (FAILED(hr)) {
            pool = D3DPOOL_MANAGED;
            hr = m_pD3DDevice8->CreateVertexBuffer(num_vertices * stride, this->usage, this->fvf, pool, &vb);
            if (FAILED(hr)) {
                dbgPrintf("!! Error detected: %s, File: %s, Line: %i !!, Bytes allocated: %i", DXGetErrorString8(hr),
                    "C:\\MyProjects\\engine8\\I3D\\VBManager.cpp", 134, num_vertices * stride);
                vb = NULL;
                ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                return false;
            }
        }
    }
    pI3DDriver->m_RecomputeUsedVRAM();
    free_blocks.insert(new S_VB_block(NULL, 0, num_vertices));
    // A dynamic buffer is locked with D3DLOCK_DISCARD
    vb->Lock(0, 0, &data, (this->usage & D3DUSAGE_DYNAMIC) << 4);
    lock_index = cVBMan.locked_buffers.size();
    cVBMan.locked_buffers.push_back(this);
    return true;
}

// Gives out count vertices from the smallest free block that has room, and returns the first one's index, or -1
// FUNCTION: LS3DF 0x10068f10
int C_VBuffer::m_Alloc(uint count, COMPLETE_VB_STRUCT *owner)
{
    std::multiset<S_VB_block *, S_VB_block_less>::iterator it;
    for (it = free_blocks.begin(); it != free_blocks.end(); it++) {
        S_VB_block *block = *it;
        int rest = block->size - count;
        if (rest >= 0) {
            block->owner = owner;
            used_blocks.push_back(block);
            free_blocks.erase(it);
            block->size = count;
            num_free -= count;
            if (rest > 0)
                free_blocks.insert(new S_VB_block(NULL, block->index + count, rest));
            return block->index;
        }
    }
    return -1;
}

// Takes back the block that starts at index, joined with the free blocks next to it, and deletes the buffer if
// nothing is given out any more
// FUNCTION: LS3DF 0x100690d0
bool C_VBuffer::m_Free(uint index)
{
    S_VB_block **p = &used_blocks[0];
    for (int i = 0; i < used_blocks.size(); i++, p++) {
        S_VB_block *block = *p;
        if (block->index == index) {
            num_free += block->size;
            used_blocks.erase(i);
            uint start = block->index;
            uint size = block->size;
            uchar joined = 0;                   // 1: joined with the next block, 2: with the previous one
            // ++it, where the other loops have it++: each gives the original's inlining
            std::multiset<S_VB_block *, S_VB_block_less>::iterator it;
        restart:
            for (it = free_blocks.begin(); it != free_blocks.end(); ++it) {
                S_VB_block *b = *it;
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
            cVBMan.m_FreeAllUnusedVBuffers(this);
            return true;
        }
    }
    return false;
}

// FUNCTION: LS3DF 0x100692b0
C_VBManager::C_VBManager()
{
    locked_buffers.reserve(256);
    locked_buffers.SetGrowSize(256);
}

// FUNCTION: LS3DF 0x100694e0
C_VBManager::~C_VBManager()
{
    std::multiset<C_VBuffer *, S_VBuffer_less>::iterator it;
    for (it = buffers.begin(); it != buffers.end(); it++)
        delete *it;
    buffers.clear();
}

// Gives out count vertices of a buffer of the format and usage, from the first that has room, or from a new one,
// which is the only way if exclusive is set
// FUNCTION: LS3DF 0x100695e0
bool C_VBManager::m_Alloc(uint fvf, uint usage, uint stride, uint count, COMPLETE_VB_STRUCT &vb, VERTEX_FORMAT format,
    bool exclusive)
{
    if (!exclusive) {
        std::multiset<C_VBuffer *, S_VBuffer_less>::iterator it;
        for (it = buffers.begin(); it != buffers.end(); it++) {
            C_VBuffer *buffer = *it;
            if (buffer->fvf == fvf && buffer->usage == usage && buffer->num_free >= count) {
                int base = buffer->m_Alloc(count, &vb);
                if (base != -1) {
                    vb.buffer = buffer;
                    vb.base = base;
                    vb.vb = buffer->vb;
                    return true;
                }
            }
        }
    }
    C_VBuffer *buffer = new C_VBuffer;
    if (!buffer->m_Init(fvf, usage, stride, count, format)) {
        delete buffer;
        return false;
    }
    buffers.insert(buffer);
    vb.buffer = buffer;
    vb.base = buffer->m_Alloc(count, &vb);
    vb.vb = buffer->vb;
    return true;
}

// FUNCTION: LS3DF 0x100697c0
void C_VBManager::m_Free(COMPLETE_VB_STRUCT &vb)
{
    if (vb.buffer)
        vb.buffer->m_Free(vb.base);
}

// FUNCTION: LS3DF 0x100697e0
void C_VBManager::m_UnlockAllVBuffers()
{
    C_VBuffer **p = &locked_buffers[0];
    for (int i = locked_buffers.size(); i--; p++) {
        (*p)->vb->Unlock();
        (*p)->data = NULL;
        (*p)->lock_index = -1;
    }
    locked_buffers.reset();
}

// Deletes the buffer if it has nothing given out, or without one every buffer that has nothing given out
// FUNCTION: LS3DF 0x10069820
void C_VBManager::m_FreeAllUnusedVBuffers(C_VBuffer *buffer)
{
    std::multiset<C_VBuffer *, S_VBuffer_less>::iterator it, it1;
    if (buffer) {
        if (buffer->num_free == buffer->num_vertices) {
            for (it1 = buffers.begin(); it1 != buffers.end(); it1++) {
                if (*it1 == buffer) {
                    buffers.erase(it1);
                    break;
                }
            }
            delete buffer;
        }
    } else {
    restart:
        for (it = buffers.begin(); it != buffers.end(); it++) {
            if ((*it)->num_free == (*it)->num_vertices) {
                delete *it;
                buffers.erase(it);
                goto restart;
            }
        }
    }
}

// FUNCTION: LS3DF 0x100698e0
void C_VBManager::m_FreeAllVBuffers()
{
    std::multiset<C_VBuffer *, S_VBuffer_less>::iterator it;
restart:
    for (it = buffers.begin(); it != buffers.end(); it++) {
        if ((*it)->used_blocks.size() || (*it)->free_blocks.size() != 1)
            dbgPrintf("C_VBManager::m_FreeAllVBManager() - WARNING! Releasing unreleased VertexBuffer !!! Some of meshes "
                "are still unreleased ?!");
        delete *it;
        buffers.erase(it);
        goto restart;
    }
}

// Deletes the buffers in the default pool, which a reset device loses
// FUNCTION: LS3DF 0x10069950
void C_VBManager::m_FreeAllVRAMVBuffers()
{
    std::multiset<C_VBuffer *, S_VBuffer_less>::iterator it;
restart:
    for (it = buffers.begin(); it != buffers.end(); it++) {
        if ((*it)->pool == D3DPOOL_DEFAULT) {
            delete *it;
            buffers.erase(it);
            goto restart;
        }
    }
}
