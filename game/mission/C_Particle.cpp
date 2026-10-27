// The effects' particles (named after PS2's C_Particle.cpp, which sorts here: the PC's file is most likely the one at
// 0x446f20-0x44856c, docs/game-files.md). Written so far: the particle's destructor, which C_effects::Mission_Done
// runs.

#include "mission/C_Particle.h"

#include "I3D/I3D.h"

// FUNCTION: GAME 0x447150
C_Particle::~C_Particle()
{
    if (frame)
        frame->Release();
}
