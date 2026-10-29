#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

class C_ParticleType;
class I3D_frame;

// An effect at work (the name is PS2's C_ParticleEmitter, after the file; the PC's is its own), 0x4c bytes: its parts,
// one for each of the effect's parts that has subparts and a frame in the model, and the effect's values (C_effect's,
// C_effects.h, whose members' names are the comments'). C_effects' particle_emitters keeps the free ones, its unknown_0
// and unknown_20 the ones in use. The members' names are ours.
class C_ParticleEmitter
{
public:
    I3D_frame *parent;                  // 0x0, the frame the effect is at (LoadStaticParticles')
    std::vector<C_ParticleType *> types;  // 0x4
    ushort id;                          // 0x14, the driver's count of emitters
    S_vector pos;                       // 0x18, the effect's matrix's position
    S_vector dir;                       // 0x24, and its second row
    S_vector unknown_30;                // 0x30, unknown_1c's
    S_vector unknown_3c;                // 0x3c, unknown_28's
    bool unknown_48;                    // 0x48, true from the constructor and Reset
    bool unknown_49;                    // 0x49
    bool unknown_4a;                    // 0x4a, unknown_39's
    bool unknown_4b;                    // 0x4b, unknown_38's

    C_ParticleEmitter() : parent(NULL)
    {
        unknown_48 = true;
        unknown_49 = false;
        unknown_4a = false;
        unknown_4b = false;
    }

    ~C_ParticleEmitter();
    // Gives the parts back to particle_parts and clears the emitter, for particle_emitters (the name is ours)
    void Reset();
    // The effect's frame, matrix and values, and a number of the driver's
    void Init(I3D_frame *at, const S_vector &p, const S_vector &d, bool v4a, bool v4b, const S_vector &v30,
        const S_vector &v3c);
};

// The free emitters (C_effects.cpp's, the name is ours)
extern std::vector<C_ParticleEmitter *> particle_emitters;
