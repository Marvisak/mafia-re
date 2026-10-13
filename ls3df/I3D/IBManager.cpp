// The index buffers (PS2 IBManager.cpp, __FILE__ C:\MyProjects\engine8\I3D\IBManager.cpp). Not written yet: only
// what other files need to link.
#include "I3D/I3D.h"

// GLOBAL: LS3DF 0x101b50f8
C_IBManager cIBMan;

// STUB: LS3DF 0x1005b6c0
bool C_IBManager::m_Alloc(uint usage, uint count, COMPLETE_IB_STRUCT &ib)
{
    return unknown_00[0] != 0;
}

// STUB: LS3DF 0x1005b940
void C_IBManager::m_Free(COMPLETE_IB_STRUCT &ib)
{
    unknown_00[2] = 0;
}

// STUB: LS3DF 0x1005b9d0
void C_IBManager::m_FreeAllUnusedIBuffers()
{
    unknown_00[3] = 0;
}

// STUB: LS3DF 0x1005ba30
void C_IBManager::m_FreeAllIBuffers()
{
    unknown_00[4] = 0;
}

// STUB: LS3DF 0x1005baa0
void C_IBManager::m_FreeAllVRAMIBuffers()
{
    unknown_00[1] = 0;
}
