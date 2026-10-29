#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

class C_Particle;
class I3D_frame;

// A subpart's values in an emitter (the name is ours), 0x64 bytes: a copy of a C_effect_subpart's (C_effects.h, whose
// members' names are the comments'), which Reset sets to the subpart's defaults. C_effects' particle_subparts keeps the
// free ones. The names are ours, and so are the setters', each a group of the values that CreateSParticle copies
// together: its floats go through the FPU, as through an inline function's parameters, and its first three vectors
// through temporaries, as by-value parameters.
struct S_particle_subpart
{
    int unknown_0;                      // 0x0, unknown_30's (1000)
    int unknown_4;                      // 0x4, unknown_34's
    int unknown_8;                      // 0x8, unknown_60's (1)
    S_vector unknown_c;                 // 0xc
    S_vector unknown_18;                // 0x18
    S_vector unknown_24;                // 0x24
    uchar unknown_30[8];                // 0x30
    S_vector unknown_38;                // 0x38
    S_vector unknown_44;                // 0x44
    float unknown_50;                   // 0x50
    float unknown_54;                   // 0x54
    float unknown_58;                   // 0x58
    float unknown_5c;                   // 0x5c
    float unknown_60;                   // 0x60, unknown_64's (9.8)

    // Reset's values, written out: CreateSParticle inlines it, which a call of Reset would leave out of line
    S_particle_subpart()
    {
        unknown_0 = 0;
        unknown_4 = 0;
        unknown_8 = 1;
        unknown_38 = S_vector(1.0f, 1.0f, 1.0f);
        unknown_44 = S_vector(0.0f, 0.0f, 0.0f);
        unknown_50 = 0.0f;
        unknown_54 = 0.0f;
        unknown_58 = 0.0f;
        unknown_5c = 0.0f;
        unknown_60 = 9.8f;
    }

    // Declared and empty: deleting one tests the pointer, which a structure without a destructor doesn't
    ~S_particle_subpart() {}

    // The defaults, which C_ParticleType::Reset inlines
    void Reset()
    {
        unknown_0 = 0;
        unknown_4 = 0;
        unknown_8 = 1;
        unknown_38 = S_vector(1.0f, 1.0f, 1.0f);
        unknown_44 = S_vector(0.0f, 0.0f, 0.0f);
        unknown_50 = 0.0f;
        unknown_54 = 0.0f;
        unknown_58 = 0.0f;
        unknown_5c = 0.0f;
        unknown_60 = 9.8f;
    }

    void SetValues(int v0, int v4, int v8, S_vector vc, S_vector v18, S_vector v24)
    {
        unknown_0 = v0;
        unknown_4 = v4;
        unknown_8 = v8;
        unknown_c = vc;
        unknown_18 = v18;
        unknown_24 = v24;
    }

    void SetFloats1(float f50, float f54)
    {
        unknown_50 = f50;
        unknown_54 = f54;
    }

    void SetFloats2(float f58, float f5c, float f60)
    {
        unknown_58 = f58;
        unknown_5c = f5c;
        unknown_60 = f60;
    }
};

// An emitter's part, one of an effect's parts at work (the names are ours, after PS2's C_ParticleEmitter, which keeps a
// vector of C_ParticleType), 0xc0 bytes: the values of a C_effect_part (C_effects.h, whose members' names are the
// comments'), its subparts' and its particles. C_effects' particle_parts keeps the free ones. The destructor and Reset
// give the subparts back to particle_subparts and the particles to particle_pool.
class C_ParticleType
{
public:
    uchar unknown_0[4];                 // 0x0
    std::vector<S_particle_subpart *> subparts;  // 0x4
    I3D_frame *frame;                   // 0x14, the part's frame of the effect's model, a reference of its own
    I3D_frame *parent;                  // 0x18, the frame the effect is at (LoadStaticParticles')
    std::vector<C_Particle *> particles;  // 0x1c
    int unknown_2c;                     // 0x2c, unknown_90's
    S_vector unknown_30;                // 0x30, unknown_20's
    S_vector unknown_3c;                // 0x3c, unknown_2c's
    S_vector unknown_48;                // 0x48, unknown_38's
    S_vector unknown_54;                // 0x54, unknown_44's
    float unknown_60;                   // 0x60, unknown_50's
    float unknown_64;                   // 0x64, unknown_54's
    S_vector unknown_68;                // 0x68, unknown_58's
    S_vector unknown_74;                // 0x74, unknown_64's
    float unknown_80;                   // 0x80, unknown_70's
    float unknown_84;                   // 0x84, unknown_74's
    int unknown_88;                     // 0x88, unknown_7c's
    float unknown_8c;                   // 0x8c, unknown_78's
    float unknown_90;                   // 0x90, unknown_78's
    float unknown_94;                   // 0x94, unknown_80's
    float unknown_98;                   // 0x98, 1000 / unknown_78, 0 for 0
    float unknown_9c;                   // 0x9c, 1000 / unknown_80, 0 for 0
    int unknown_a0;                     // 0xa0, unknown_84's, an integer
    int unknown_a4;                     // 0xa4
    int unknown_a8;                     // 0xa8, unknown_8c's, an integer
    int unknown_ac;                     // 0xac, -1 from Init
    int unknown_b0;                     // 0xb0, unknown_88's, an integer
    int unknown_b4;                     // 0xb4
    bool unknown_b8;                    // 0xb8, unknown_b0 is -1
    int unknown_bc;                     // 0xbc, unknown_98's (flags)

    C_ParticleType() : frame(NULL)
    {
        unknown_60 = 0.0f;
        unknown_64 = 0.0f;
        unknown_68 = S_vector(1.0f, 1.0f, 1.0f);
        unknown_74 = S_vector(0.0f, 0.0f, 0.0f);
        unknown_88 = 0;
        unknown_8c = 0.0f;
        unknown_90 = 0.0f;
        unknown_94 = 0.0f;
        unknown_98 = 0.0f;
        unknown_9c = 0.0f;
        unknown_a8 = 0;
        unknown_ac = 0;
        unknown_b0 = 0;
        unknown_b4 = 0;
        unknown_b8 = false;
        unknown_bc = 0;
    }

    ~C_ParticleType();
    // The part's values, its frame (with a reference) and the effect's frame
    void Init(I3D_frame *at, I3D_frame *frm, const S_vector &v30, const S_vector &v3c, int v88, float v8c,
        float v94, int va0, int vb0, int va8);
    // Gives the subparts and the particles back and clears the values, for particle_parts (the name is ours)
    void Reset();

    void SetFloats1(float f60, float f64)
    {
        unknown_60 = f60;
        unknown_64 = f64;
    }

    void SetFloats2(float f80, float f84)
    {
        unknown_80 = f80;
        unknown_84 = f84;
    }
};

// The free parts and subparts (C_effects.cpp's, the names are ours)
extern std::vector<C_ParticleType *> particle_parts;
extern std::vector<S_particle_subpart *> particle_subparts;
