// The vertex buffers (PS2 VBManager.cpp, __FILE__ C:\MyProjects\engine8\I3D\VBManager.cpp). Not written yet: only
// what other files need to link.
#include "I3D/I3D.h"

// GLOBAL: LS3DF 0x101c5014
C_VBManager cVBMan;

// STUB: LS3DF 0x100695e0
bool C_VBManager::m_Alloc(uint fvf, uint usage, uint stride, uint count, COMPLETE_VB_STRUCT &vb, int unknown6,
    bool unknown7)
{
    return unknown_00[2] != 0;
}

// STUB: LS3DF 0x100697c0
void C_VBManager::m_Free(COMPLETE_VB_STRUCT &vb)
{
    unknown_00[4] = 0x5b;
}

// STUB: LS3DF 0x10069820
void C_VBManager::m_FreeAllUnusedVBuffers(void *unknown)
{
    unknown_00[5] = 0x5b;
}

// STUB: LS3DF 0x100698e0
void C_VBManager::m_FreeAllVBuffers()
{
    unknown_00[6] = 0x5b;
}

// STUB: LS3DF 0x10069950
void C_VBManager::m_FreeAllVRAMVBuffers()
{
    unknown_00[3] = 0x5b;
}
