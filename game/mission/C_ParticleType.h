#pragma once

#include "Main.h"

#include <vector>

class C_Particle;
class I3D_frame;

// A subpart's state in an emitter (the name is ours), 0x64 bytes or more, which
// C_ParticleType resets to a subpart's defaults (C_effect_subpart's) and C_effects' particle_subparts keeps when free.
// What it holds isn't known yet.
struct S_particle_subpart
{
    uchar unknown_0[0x64];              // 0x0

    // Declared and empty: deleting one tests the pointer, which a structure without a destructor doesn't
    ~S_particle_subpart() {}
};

// An emitter's part, one of an effect's parts at work (the names are ours, after PS2's C_ParticleEmitter, which keeps a
// vector of C_ParticleType): its subparts and its particles. C_effects' particle_parts keeps the free ones. Only the
// destructor so far, which gives the particles back to particle_pool.
class C_ParticleType
{
public:
    uchar unknown_0[4];                 // 0x0
    std::vector<S_particle_subpart *> subparts;  // 0x4
    I3D_frame *frame;                   // 0x14, released by the destructor
    uchar unknown_18[4];                // 0x18
    std::vector<C_Particle *> particles;  // 0x1c

    ~C_ParticleType();
};
