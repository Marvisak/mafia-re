// The effects' emitters (named after PS2's C_ParticleEmitter.cpp, which sorts here: the PC's file is most likely the one
// at 0x448570-0x448efc, docs/game-files.md), which starts with the emitter's destructor. Written: the destructor, which
// C_effects::Mission_Done runs, Reset and Init, which C_effects::CreateSParticle runs, and Tick, which
// C_effects::ClientTick runs; CreateParticle, which Tick runs, is a stub.

#include "mission/C_ParticleEmitter.h"
#include "mission/C_Particle.h"
#include "mission/C_ParticleType.h"

#include "I3D/I3D.h"

#include "stub.h"

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
    unknown_48 = true;
    unknown_49 = false;
    unknown_4a = false;
    unknown_4b = false;
}

// FUNCTION: GAME 0x448680
void C_ParticleEmitter::Init(I3D_frame *at, const S_vector &p, const S_vector &d, bool v4a, bool v4b,
    const S_vector &v30, const S_vector &v3c)
{
    parent = at;
    dir = d;
    pos = p;
    unknown_4a = v4a;
    unknown_4b = v4b;
    unknown_30 = v30;
    unknown_3c = v3c;
    I3D_driver *drv = I3DGetDriver();
    ushort n = drv->unknown_58;
    drv->unknown_58 = n + 1;
    id = n;
}

// Each part makes its particles, all of them at once without a rate (unknown_88), or as many as the time gives at the
// rate, each with the time it has already lived; then the older ones live on, and one that has ended goes on with its
// next subpart or back to particle_pool. After its delay the part's rate falls, and once it is gone the part starts
// again where it repeats. True while a part still works or has particles.
// FUNCTION: GAME 0x448960
bool C_ParticleEmitter::Tick(int time)
{
    if (unknown_49)
        return true;
    int num_new = 0;
    BOOL alive = FALSE;
    if (!types.size())
        return false;
    for (uint i = 0; i < types.size(); i++)
    {
        C_ParticleType *t = types[i];
        if (unknown_48)
        {
            if (t->unknown_ac > 0)
                t->unknown_ac -= time;
            if (!t->unknown_a4 && t->unknown_88)
            {
                num_new = t->unknown_88;
                for (int j = 0; j < num_new; j++)
                    CreateParticle(t, 0);
            }
            else if (t->unknown_8c > 0.0f && t->unknown_ac <= 0)
            {
                t->unknown_b4 += time;
                float elapsed = (float)t->unknown_b4;
                num_new = (int)(elapsed / t->unknown_98);
                if (num_new)
                {
                    t->unknown_b4 = (int)(elapsed - num_new * t->unknown_98);
                    for (int j = 0; j < num_new; j++)
                        CreateParticle(t, (int)(j * t->unknown_98));
                }
            }
        }
        else
            num_new = 0;
        for (int j = 0; j < (int)t->particles.size() - num_new; j++)
        {
            C_Particle *p = t->particles[j];
            p->Tick(time);
            if (p->unknown_3c > 0)
                continue;
            if (t->subparts.size() - 1 < p->unknown_8 + 1)
            {
                p->Reset();
                particle_pool.push_back(p);
                t->particles.erase(t->particles.begin() + j);
                j--;
            }
            else
                p->SetPhase(p->unknown_8 + 1);
        }
        if (t->unknown_ac <= 0)
        {
            t->unknown_a4 += time;
            if (t->unknown_a4 > t->unknown_a0 && t->unknown_8c > 0.0f)
            {
                t->unknown_8c -= time * t->unknown_94 * .001f;
                if (t->unknown_8c > 0.0f)
                    t->unknown_98 = (1.0f / t->unknown_8c) * 1000.0f;
            }
            // Negated as the original's test (jp to the store): > gives another
            if (!(t->unknown_8c <= 0.0f))
            {
                alive = TRUE;
                continue;
            }
            if (t->unknown_b8 || t->unknown_b0 > 1)
            {
                if (t->unknown_b0 > 1)
                    t->unknown_b0--;
                t->unknown_8c = t->unknown_90;
                t->unknown_a4 = 0;
                t->unknown_98 = (1.0f / t->unknown_8c) * 1000.0f;
                t->unknown_ac = t->unknown_a8;
                t->unknown_b4 = 0;
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

#pragma auto_inline(off)

// STUB: GAME 0x448710
void C_ParticleEmitter::CreateParticle(C_ParticleType *type, int time)
{
    StubLog("C_ParticleEmitter::CreateParticle");
}

#pragma auto_inline(on)
