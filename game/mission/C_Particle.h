#pragma once

#include "Main.h"

class I3D_frame;

// One particle of the effects' emitters (the PC's own; the name is PS2's C_Particle, after the file, whose class the
// PC's need not be). C_effects' particle_pool keeps the free ones, which C_ParticleType's destructor gives back to it.
// Only the destructor so far; the size isn't known yet.
class C_Particle
{
public:
    I3D_frame *frame;                   // 0x0, released by the destructor

    ~C_Particle();
};
