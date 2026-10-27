// The emitters' parts (PC only, the name is ours: the file at 0x44c0d0-0x44c51c, docs/game-files.md, sorts between
// C_ParticleManager.cpp's and C_person_traffic.cpp's places, and PS2's emitter keeps C_ParticleTypes), which starts with
// the part's destructor. Written so far: nothing; the destructor, which C_effects::Mission_Done runs, is a stub.

#include "mission/C_ParticleType.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x44c0d0
C_ParticleType::~C_ParticleType()
{
    StubLog("C_ParticleType::~C_ParticleType");
}

#pragma auto_inline(on)
