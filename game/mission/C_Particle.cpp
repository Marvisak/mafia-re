// The effects' particles (named after PS2's C_Particle.cpp, which sorts here: the PC's file is most likely the one at
// 0x446f20-0x44856c, docs/game-files.md). Written so far: the particle's destructor, which C_effects::Mission_Done
// runs, and Reset, which C_ParticleType's destructor and Reset run.

#include "mission/C_Particle.h"

#include "I3D/I3D.h"

// FUNCTION: GAME 0x447150
C_Particle::~C_Particle()
{
    if (frame)
        frame->Release();
}

// FUNCTION: GAME 0x447160
void C_Particle::Reset()
{
    if (frame)
        frame->LinkTo(NULL, 0);
    unknown_8 = 0;
    type = NULL;
    unknown_3c = 0;
    unknown_60 = 0;
    unknown_44 = 0;
    unknown_5c = 0;
}
