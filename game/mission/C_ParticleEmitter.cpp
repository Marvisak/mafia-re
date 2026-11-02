// The effects' emitters (named after PS2's C_ParticleEmitter.cpp, which sorts here: the PC's file is most likely the one
// at 0x448570-0x448efc, docs/game-files.md), which starts with the emitter's destructor. Written: the destructor, which
// C_effects::Mission_Done runs, Reset and Init, which C_effects::CreateSParticle runs, and Tick, which
// C_effects::ClientTick runs, with CreateParticle, which Tick runs.

#include "mission/C_ParticleEmitter.h"
#include "mission/C_Particle.h"
#include "mission/C_ParticleType.h"

#include "I3D/I3D.h"

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

// FUNCTION: GAME 0x448610
void C_ParticleEmitter::Reset()
{
    C_ParticleType **type;
    for (type = types.begin(); type < types.end(); type++)
    {
        (*type)->Reset();
        particle_parts.push_back(*type);
    }
    types.clear();
    parent = NULL;
    emitting = true;
    frozen = false;
    local_pos = false;
    local_dir = false;
}

// FUNCTION: GAME 0x448680
void C_ParticleEmitter::Init(I3D_frame *at, const S_vector &p, const S_vector &d, bool v4a, bool v4b,
    const S_vector &v30, const S_vector &v3c)
{
    parent = at;
    dir = d;
    pos = p;
    local_pos = v4a;
    local_dir = v4b;
    offset = v30;
    unknown_3c = v3c;
    I3D_driver *drv = I3DGetDriver();
    ushort n = drv->next_emitor_id;
    drv->next_emitor_id = n + 1;
    id = n;
}

// A free particle or a new one for the part: at the effect's position plus offset (both through the frame's matrix
// where local_pos is set), in the effect's direction where the part has PARTF_DIR (turned by the matrix where
// local_dir is), into the part's list, or back to the free ones if it can't be
// FUNCTION: GAME 0x448710
void C_ParticleEmitter::CreateParticle(C_ParticleType *type, int time)
{
    C_Particle *p;
    if (particle_pool.size())
    {
        p = particle_pool.back();
        particle_pool.pop_back();
    }
    else
        p = new C_Particle;
    S_vector d = type->flags & PARTF_DIR ? dir : S_vector(0.0f, 0.0f, 0.0f);
    S_vector at;
    if (parent && local_pos)
        at = pos * parent->GetMatrix() + offset.RotateByMatrix(parent->GetMatrix());
    else
        at = offset + pos;
    if (parent && local_dir && !dir.IsNull())
        d = dir.RotateByMatrix(parent->GetMatrix());
    if (p->Init(id, at, d, type, time))
        type->particles.push_back(p);
    else
    {
        p->Reset();
        particle_pool.push_back(p);
    }
}

// Each part makes its particles, its burst at once when it starts, or as many as the time gives at the rate, each
// with the time it has already lived; then the older ones live on, and one that has ended goes on with its next
// subpart or back to particle_pool. After its fall_delay the part's rate falls, and once it is gone the part starts
// again after its restart_delay where it repeats. True while a part still works or has particles.
// FUNCTION: GAME 0x448960
bool C_ParticleEmitter::Tick(int time)
{
    if (frozen)
        return true;
    int num_new = 0;
    BOOL alive = FALSE;
    if (!types.size())
        return false;
    for (uint i = 0; i < types.size(); i++)
    {
        C_ParticleType *t = types[i];
        if (emitting)
        {
            if (t->wait > 0)
                t->wait -= time;
            if (!t->run_time && t->burst)
            {
                num_new = t->burst;
                for (int j = 0; j < num_new; j++)
                    CreateParticle(t, 0);
            }
            else if (t->rate > 0.0f && t->wait <= 0)
            {
                t->emit_time += time;
                float elapsed = (float)t->emit_time;
                num_new = (int)(elapsed / t->interval);
                if (num_new)
                {
                    t->emit_time = (int)(elapsed - num_new * t->interval);
                    for (int j = 0; j < num_new; j++)
                        CreateParticle(t, (int)(j * t->interval));
                }
            }
        }
        else
            num_new = 0;
        for (int j = 0; j < (int)t->particles.size() - num_new; j++)
        {
            C_Particle *p = t->particles[j];
            p->Tick(time);
            if (p->time_left > 0)
                continue;
            if (t->subparts.size() - 1 < p->subpart + 1)
            {
                p->Reset();
                particle_pool.push_back(p);
                t->particles.erase(t->particles.begin() + j);
                j--;
            }
            else
                p->SetPhase(p->subpart + 1);
        }
        if (t->wait <= 0)
        {
            t->run_time += time;
            if (t->run_time > t->fall_delay && t->rate > 0.0f)
            {
                t->rate -= time * t->rate_fall * .001f;
                if (t->rate > 0.0f)
                    t->interval = (1.0f / t->rate) * 1000.0f;
            }
            // Negated as the original's test (jp to the store): > gives another
            if (!(t->rate <= 0.0f))
            {
                alive = TRUE;
                continue;
            }
            if (t->forever || t->repeats > 1)
            {
                if (t->repeats > 1)
                    t->repeats--;
                t->rate = t->start_rate;
                t->run_time = 0;
                t->interval = (1.0f / t->rate) * 1000.0f;
                t->wait = t->restart_delay;
                t->emit_time = 0;
                alive = TRUE;
            }
            if (t->particles.size())
                alive = TRUE;
        }
        else
            alive = TRUE;
    }
    return alive;
}
