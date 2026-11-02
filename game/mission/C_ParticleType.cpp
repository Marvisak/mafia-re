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
    scale = S_vector(1.0f, 1.0f, 1.0f);
    flags = 0;
    burst = 0;
    rate = 0.0f;
    rate_fall = 0.0f;
    start_rate = 0.0f;
    interval = 0.0f;
    fall_delay = 0;
    fall_interval = 0.0f;
    forever = false;
    emit_time = 0;
    wait = 0;
    repeats = 0;
    restart_delay = 0;
    speed = 0.0f;
    speed_spread = 0.0f;
    scale_spread = S_vector(0.0f, 0.0f, 0.0f);
}

// The rates are per second of times in milliseconds
// FUNCTION: GAME 0x44c3e0
void C_ParticleType::Init(I3D_frame *at, I3D_frame *frm, const S_vector &v30, const S_vector &v3c, int v88,
    float v8c, float v94, int va0, int vb0, int va8)
{
    parent = at;
    frame = frm;
    frm->m_RefCount++;
    offset = v30;
    offset_spread = v3c;
    burst = v88;
    rate = v8c;
    start_rate = v8c;
    rate_fall = v94;
    if (v8c != 0.0f)
        interval = (1.0f / v8c) * 1000.0f;
    else
        interval = 0.0f;
    if (v94 != 0.0f)
        fall_interval = (1.0f / v94) * 1000.0f;
    else
        fall_interval = 0.0f;
    fall_delay = va0;
    forever = vb0 == -1;
    repeats = vb0;
    run_time = 0;
    restart_delay = va8;
    wait = -1;
}
