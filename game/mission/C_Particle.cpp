// The effects' particles (named after PS2's C_Particle.cpp, which sorts here: the PC's file is most likely the one at
// 0x446f20-0x44856c, docs/game-files.md). Written so far: the particle's destructor, which C_effects::Mission_Done
// runs, and Reset, which C_ParticleType's destructor and Reset run; Init, which the emitter's CreateParticle runs (it
// fails, so the particle goes back to particle_pool), and SetPhase and Tick, which the emitter's Tick runs, are stubs
// (a particle ends at once).

#include "mission/C_Particle.h"

#include "I3D/I3D.h"
#include "stub.h"

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

#pragma auto_inline(off)

// STUB: GAME 0x447190
bool C_Particle::Init(ushort id, const S_vector &pos, const S_vector &dir, C_ParticleType *type, int time)
{
    StubLog("C_Particle::Init");
    return false;
}

// STUB: GAME 0x4476c0
void C_Particle::SetPhase(int phase)
{
    StubLog("C_Particle::SetPhase");
    unknown_8 = phase;
}

// STUB: GAME 0x447c20
void C_Particle::Tick(int time)
{
    StubLog("C_Particle::Tick");
    unknown_3c = 0;
}

#pragma auto_inline(on)
