#pragma once

#include "Main.h"

#include <vector>

class C_ParticleType;
class I3D_frame;

// One particle of the effects' emitters (the PC's own; the name is PS2's C_Particle, after the file, whose class the
// PC's need not be). C_effects' particle_pool keeps the free ones, which C_ParticleType's destructor and Reset give back
// to it. The size isn't known yet.
class C_Particle
{
public:
    I3D_frame *frame;                   // 0x0, the particle's visual, released by the destructor
    C_ParticleType *type;               // 0x4, the emitter's part it belongs to
    int unknown_8;                      // 0x8
    uchar unknown_c[0x30];              // 0xc
    int unknown_3c;                     // 0x3c
    uchar unknown_40[4];                // 0x40
    int unknown_44;                     // 0x44
    uchar unknown_48[0x14];             // 0x48
    int unknown_5c;                     // 0x5c
    int unknown_60;                     // 0x60

    ~C_Particle();
    // Unlinks the visual and clears the particle's state, for particle_pool (the name is ours)
    void Reset();
};

// The free particles (C_effects.cpp's, the name is ours)
extern std::vector<C_Particle *> particle_pool;
