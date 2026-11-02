#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

class C_ParticleType;
class I3D_frame;

// An effect at work (the name is PS2's C_ParticleEmitter, after the file; the PC's is its own), 0x4c bytes: its parts,
// one for each of the effect's parts that has subparts and a frame in the model, and the effect's values (C_effect's,
// C_effects.h, whose members have the same names). C_effects' particle_emitters keeps the free ones, its in_use and
// static_emitters the ones in use. The members' names are ours.
class C_ParticleEmitter
{
public:
    I3D_frame *parent;                  // 0x0, the frame the effect is at (LoadStaticParticles')
    std::vector<C_ParticleType *> types;  // 0x4
    ushort id;                          // 0x14, the driver's count of emitters
    S_vector pos;                       // 0x18, the effect's matrix's position
    S_vector dir;                       // 0x24, and its second row
    S_vector offset;                    // 0x30, the particles' offset from pos
    S_vector unknown_3c;                // 0x3c, C_effect::unknown_28's
    bool emitting;                      // 0x48, the parts make new particles (Tick): true from the constructor and Reset
    bool frozen;                        // 0x49, Tick does nothing and the emitter stays alive
    bool local_pos;                     // 0x4a, pos and offset are turned by the frame's matrix
    bool local_dir;                     // 0x4b, dir is turned by it

    C_ParticleEmitter() : parent(NULL)
    {
        emitting = true;
        frozen = false;
        local_pos = false;
        local_dir = false;
    }

    ~C_ParticleEmitter();
    // Gives the parts back to particle_parts and clears the emitter, for particle_emitters (the name is ours)
    void Reset();
    // The effect's frame, matrix and values, and a number of the driver's
    void Init(I3D_frame *at, const S_vector &p, const S_vector &d, bool v4a, bool v4b, const S_vector &v30,
        const S_vector &v3c);
    // A particle of the part, which has already lived time (PS2's name; its PS2 parameters are a type's data, a matrix
    // and two integers)
    void CreateParticle(C_ParticleType *type, int time);
    // The effect by the time passed: false once it has ended (PS2's name)
    bool Tick(int time);
};

// The free emitters (C_effects.cpp's, the name is ours)
extern std::vector<C_ParticleEmitter *> particle_emitters;
