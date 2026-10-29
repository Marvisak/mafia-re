// The emitters' parts (PC only, the name is ours: the file at 0x44c0d0-0x44c51c, docs/game-files.md, sorts between
// C_ParticleManager.cpp's and C_person_traffic.cpp's places, and PS2's emitter keeps C_ParticleTypes), which starts with
// the part's destructor. Written: the destructor, which C_effects::Mission_Done and the emitter's destructor run, Reset,
// which the emitter's Reset runs, and Init, which C_effects::CreateSParticle runs.

#include "mission/C_ParticleType.h"
#include "mission/C_Particle.h"

#include "I3D/I3D.h"

// The subparts deleted, the particles given back to particle_pool, from the last
// FUNCTION: GAME 0x44c0d0
C_ParticleType::~C_ParticleType()
{
    while (subparts.size())
    {
        delete subparts.back();
        subparts.pop_back();
    }
    while (particles.size())
    {
        particles.back()->Reset();
        particle_pool.push_back(particles.back());
        particles.pop_back();
    }
    if (frame)
        frame->Release();
}

// FUNCTION: GAME 0x44c200
void C_ParticleType::Reset()
{
    S_particle_subpart **subpart;
    for (subpart = subparts.begin(); subpart < subparts.end(); subpart++)
    {
        (*subpart)->Reset();
        particle_subparts.push_back(*subpart);
    }
    subparts.clear();
    C_Particle **particle;
    for (particle = particles.begin(); particle < particles.end(); particle++)
    {
        (*particle)->Reset();
        particle_pool.push_back(*particle);
    }
    particles.clear();
    if (frame)
        frame->Release();
    frame = NULL;
    unknown_68 = S_vector(1.0f, 1.0f, 1.0f);
    unknown_bc = 0;
    unknown_88 = 0;
    unknown_8c = 0.0f;
    unknown_94 = 0.0f;
    unknown_90 = 0.0f;
    unknown_98 = 0.0f;
    unknown_a0 = 0;
    unknown_9c = 0.0f;
    unknown_b8 = false;
    unknown_b4 = 0;
    unknown_ac = 0;
    unknown_b0 = 0;
    unknown_a8 = 0;
    unknown_60 = 0.0f;
    unknown_64 = 0.0f;
    unknown_74 = S_vector(0.0f, 0.0f, 0.0f);
}

// The rates are per second of times in milliseconds
// FUNCTION: GAME 0x44c3e0
void C_ParticleType::Init(I3D_frame *at, I3D_frame *frm, const S_vector &v30, const S_vector &v3c, int v88,
    float v8c, float v94, int va0, int vb0, int va8)
{
    parent = at;
    frame = frm;
    frm->m_RefCount++;
    unknown_30 = v30;
    unknown_3c = v3c;
    unknown_88 = v88;
    unknown_8c = v8c;
    unknown_90 = v8c;
    unknown_94 = v94;
    if (v8c != 0.0f)
        unknown_98 = (1.0f / v8c) * 1000.0f;
    else
        unknown_98 = 0.0f;
    if (v94 != 0.0f)
        unknown_9c = (1.0f / v94) * 1000.0f;
    else
        unknown_9c = 0.0f;
    unknown_a0 = va0;
    unknown_b8 = vb0 == -1;
    unknown_b0 = vb0;
    unknown_a4 = 0;
    unknown_a8 = va8;
    unknown_ac = -1;
}
