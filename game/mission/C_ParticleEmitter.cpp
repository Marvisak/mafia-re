// The effects' emitters (named after PS2's C_ParticleEmitter.cpp, which sorts here: the PC's file is most likely the one
// at 0x448570-0x448efc, docs/game-files.md), which starts with the emitter's destructor. Written: the destructor, which
// C_effects::Mission_Done runs, and Reset and Init, which C_effects::CreateSParticle runs.

#include "mission/C_ParticleEmitter.h"
#include "mission/C_ParticleType.h"

#include "I3D/I3D.h"

// From the last part, as C_effect's destructor deletes its parts
// FUNCTION: GAME 0x448570
C_ParticleEmitter::~C_ParticleEmitter()
{
    while (types.size())
    {
        delete types.back();
        types.pop_back();
    }
}

// FUNCTION: GAME 0x448610
void C_ParticleEmitter::Reset()
{
    C_ParticleType **type;
    for (type = types.begin(); type < types.end(); type++)
    {
        (*type)->Reset();
        particle_parts.push_back(*type);
    }
    types.clear();
    parent = NULL;
    unknown_48 = true;
    unknown_49 = false;
    unknown_4a = false;
    unknown_4b = false;
}

// FUNCTION: GAME 0x448680
void C_ParticleEmitter::Init(I3D_frame *at, const S_vector &p, const S_vector &d, bool v4a, bool v4b,
    const S_vector &v30, const S_vector &v3c)
{
    parent = at;
    dir = d;
    pos = p;
    unknown_4a = v4a;
    unknown_4b = v4b;
    unknown_30 = v30;
    unknown_3c = v3c;
    I3D_driver *drv = I3DGetDriver();
    ushort n = drv->unknown_58;
    drv->unknown_58 = n + 1;
    id = n;
}
