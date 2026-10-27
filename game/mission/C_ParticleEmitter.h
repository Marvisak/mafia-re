#pragma once

#include "Main.h"

#include <vector>

class C_ParticleType;

// An effect at work (the name is PS2's C_ParticleEmitter, after the file; the PC's is its own): its parts, one for each
// of the effect's. C_effects' particle_emitters keeps the free ones, its unknown_0 and unknown_20 the ones in use. Only
// the destructor so far.
class C_ParticleEmitter
{
public:
    uchar unknown_0[4];                 // 0x0
    std::vector<C_ParticleType *> types;  // 0x4

    ~C_ParticleEmitter();
};
