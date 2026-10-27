// The effects' emitters (named after PS2's C_ParticleEmitter.cpp, which sorts here: the PC's file is most likely the one
// at 0x448570-0x448efc, docs/game-files.md), which starts with the emitter's destructor. Written so far: the
// destructor, which C_effects::Mission_Done runs.

#include "mission/C_ParticleEmitter.h"
#include "mission/C_ParticleType.h"

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
