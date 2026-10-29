// The effects of tables\effects.tbl, PC only: the file at 0x448f00-0x44b4cc (docs/game-files.md), which starts with
// the initializers of the emitters' free objects, then C_effects' constructor; the name is ours (the file sorts where
// PS2's C_ParticleManager.cpp would, whose class has the functions C_game calls). Written so far: the free objects'
// vectors, the constructor, the destructor, Load and Clear, which C_mission's Init and UnInit run, and with Load the
// effects' classes (C_effects.h), Mission_Start, Mission_Done, ClearEmitters and LoadStaticParticles, which C_game's
// Init and Done run, and CreateSParticle, which LoadStaticParticles runs; CreateParticle, which the scripts call, and
// DestroyParticle are stubs.

#include "mission/C_effects.h"
#include "mission/C_Particle.h"
#include "mission/C_ParticleEmitter.h"
#include "mission/C_ParticleType.h"
#include "mission/Mission.h"

#include <stdio.h>

#include "I3D/I3D.h"
#include "I3D_cache.h"
#include "stub.h"

// The effects.bin chunks (the names are ours): the file's, whose other type the messages call another version, and an
// effect's at a frame
#define CT_EFFECTS_BIN 0x64
#define CT_EFFECTS_BIN_EFFECT 0x3e8

// The free objects of the emitters, each type's (their order in .bss is the names' buckets', tools/bsshash.py)
// GLOBAL: GAME 0x63d718
std::vector<C_Particle *> particle_pool;
// GLOBAL: GAME 0x63d708
std::vector<C_ParticleType *> particle_parts;
// GLOBAL: GAME 0x63d6f8
std::vector<S_particle_subpart *> particle_subparts;
// GLOBAL: GAME 0x63d6e8
std::vector<C_ParticleEmitter *> particle_emitters;

// FUNCTION: GAME 0x449080
C_effects::C_effects()
{
    Load();
    unknown_30 = -1;
}

// FUNCTION: GAME 0x449100
C_effects::~C_effects()
{
    Clear();
}

// The effects loaded if C_mission's Init didn't; true (al holds the flag's 1). The performance counter is read and
// nothing uses it (PC only, as in LoadStaticParticles): the counts are a block's, whose slots the size's temporary
// takes after it
// FUNCTION: GAME 0x449180
bool C_effects::Mission_Start()
{
    {
        LARGE_INTEGER frequency, counter;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&counter);
    }
    unknown_34 = false;
    unknown_30 = -1;
    if (!effects.size())
        Load();
    unknown_34 = true;
    return true;
}

// The emitters in use and every free object deleted (PS2's deletes its emitters the same way)
// FUNCTION: GAME 0x4491d0
void C_effects::Mission_Done()
{
    if (!unknown_34)
        return;
    ClearEmitters();
    unknown_30 = -1;
    for (uint i = 0; i < unknown_20.size(); i++)
        delete unknown_20[i];
    unknown_20.clear();
    C_Particle **particle;
    for (particle = particle_pool.begin(); particle < particle_pool.end(); particle++)
        delete *particle;
    particle_pool.clear();
    C_ParticleType **part;
    for (part = particle_parts.begin(); part < particle_parts.end(); part++)
        delete *part;
    particle_parts.clear();
    S_particle_subpart **subpart;
    for (subpart = particle_subparts.begin(); subpart < particle_subparts.end(); subpart++)
        delete *subpart;
    particle_subparts.clear();
    C_ParticleEmitter **emitter;
    for (emitter = particle_emitters.begin(); emitter < particle_emitters.end(); emitter++)
        delete *emitter;
    particle_emitters.clear();
    unknown_34 = false;
}

// The table is a list of effect chunks; anything else ends it
// FUNCTION: GAME 0x449380
void C_effects::Load()
{
    Clear();
    C_chunk chunk;
    // STRING: GAME 0x632590
    if (!chunk.ROpen("tables\\effects.tbl"))
        return;
    ushort type;
    while (chunk.RAscend(type) && type == CT_EFFECT)
    {
        C_effect *effect = new C_effect;
        effect->Load(chunk);
        chunk.Descend();
        effects.push_back(effect);
    }
    chunk.Close();
}

#pragma auto_inline(off)

// STUB: GAME 0x449690
int C_effects::CreateParticle(int effect, const S_vector &pos, const S_vector &dir, int unknown, I3D_frame *frm)
{
    StubLog("C_effects::CreateParticle");
    return -1;
}

// STUB: GAME 0x44a240
void C_effects::DestroyParticle(int particle)
{
    StubLog("C_effects::DestroyParticle");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x44a2f0
void C_effects::ClearEmitters()
{
    for (uint i = 0; i < unknown_0.size(); i++)
        delete unknown_0[i].emitter;
    unknown_0.clear();
}

// FUNCTION: GAME 0x44a350
void C_effects::Clear()
{
    for (int i = 0; i < effects.size(); i++)
        delete effects[i];
    effects.clear();
}

// An emitter of the effect at a frame: the effect's model loaded through the cache (Open inlined, I3D_cache.h), and a
// part of the emitter for each of the effect's parts that has subparts and a frame in the model, which goes into the
// scene. The emitter, its parts and their subparts are the free ones where there are any; an emitter without parts goes
// back to them. The model is released either way: the cache keeps its own copy.
// Open: the original compiled S_vector(float, float, float)'s copy before this function, after 0x4496e0 (not written,
// which calls it out of line). Ours compiles it after this function, so VC6 doesn't know that the part's constructor's
// calls of it can't throw: ours sets an EH state after the part's second vector, and its stores and the registers
// around them differ. The inlined Open differs as its other copies do (I3D_cache.h): its match loop's body comes
// after the folders' loop, and the folder's name is converted inline, which the original calls. The registers differ
// throughout: the effect is in ebp (the original's edi), the effect's part and the emitter's part on the stack (the
// original's ebp and ebx).
// FUNCTION: GAME 0x44a3a0
void C_effects::CreateSParticle(int effect, const S_matrix &m, I3D_frame *frm)
{
    if (effects.size() <= (uint)effect)
        return;
    C_effect *e = effects[effect];
    if (!e)
        return;
    if (!e->parts.size())
        return;
    if (!e->model.Size())
        return;
    I3D_model *mod = (I3D_model *)I3DGetDriver()->CreateFrame(FRAME_MODEL);
    if (model_cache.Open(mod, &e->model[0], 0, NULL, NULL, NULL) < I3D_OK)
    {
        mod->Release();
        return;
    }
    C_ParticleEmitter *emitter;
    if (particle_emitters.size())
    {
        emitter = particle_emitters.back();
        particle_emitters.pop_back();
    }
    else
        emitter = new C_ParticleEmitter;
    emitter->Init(frm, m(3), m(1), e->unknown_39, e->unknown_38, e->unknown_1c, e->unknown_28);
    for (uint i = 0; i < e->parts.size(); i++)
    {
        C_effect_part *part = e->parts[i];
        if (part->frame.Size() > 0)
        {
            I3D_frame *f = mod->FindChildFrame(&part->frame[0], ENUMF_VISUAL);
            if (part->subparts.size() && f)
            {
                act_mission->GetScene()->AddFrame(f);
                C_ParticleType *type;
                if (particle_parts.size())
                {
                    type = particle_parts.back();
                    particle_parts.pop_back();
                }
                else
                    type = new C_ParticleType;
                type->Init(frm, f, part->unknown_20, part->unknown_2c, part->unknown_7c, part->unknown_78,
                    part->unknown_80, (int)part->unknown_84, (int)part->unknown_88, (int)part->unknown_8c);
                type->unknown_bc = part->unknown_98;
                type->unknown_68 = part->unknown_58;
                type->unknown_74 = part->unknown_64;
                type->SetFloats2(part->unknown_70, part->unknown_74);
                type->unknown_2c = part->unknown_90;
                type->SetFloats1(part->unknown_50, part->unknown_54);
                type->unknown_48 = part->unknown_38;
                type->unknown_54 = part->unknown_44;
                for (uint j = 0; j < part->subparts.size(); j++)
                {
                    C_effect_subpart *sub = part->subparts[j];
                    S_particle_subpart *subpart;
                    if (particle_subparts.size())
                    {
                        subpart = particle_subparts.back();
                        particle_subparts.pop_back();
                    }
                    else
                        subpart = new S_particle_subpart;
                    subpart->SetValues(sub->unknown_30, sub->unknown_34, sub->unknown_60, sub->unknown_c,
                        sub->unknown_18, sub->unknown_24);
                    subpart->unknown_38 = sub->unknown_38;
                    subpart->unknown_44 = sub->unknown_44;
                    subpart->SetFloats1(sub->unknown_50, sub->unknown_54);
                    subpart->SetFloats2(sub->unknown_58, sub->unknown_5c, sub->unknown_64);
                    type->subparts.push_back(subpart);
                }
                emitter->types.push_back(type);
            }
        }
    }
    if (!emitter->types.size())
    {
        mod->Release();
        emitter->Reset();
        particle_emitters.push_back(emitter);
    }
    else
    {
        unknown_20.push_back(emitter);
        mod->Release();
    }
}

// The mission's effects.bin: the effects started at frames of the scene, each with its matrix (PS2's reads two
// versions of its own file). An effect's frame name is read with one Descend more than its chunks need. The counter is
// read as in Mission_Start, in a block of its own: its slots go to later locals. The error in the effects' chunks skips
// the end's Descend and Close, and shares the end's destructors, the chunk's out of line.
// Open: the inner Size() is inlined, which the original calls; no padding or helper moved it (2026-10-27)
// FUNCTION: GAME 0x44ad30
void C_effects::LoadStaticParticles()
{
    {
        LARGE_INTEGER frequency, counter;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&counter);
    }
    char buf[128];
    // STRING: GAME 0x632620
    sprintf(buf, "missions\\%s\\effects.bin", act_mission->GetMissionName());
    C_str name = buf;
    C_chunk chunk;
    if (!chunk.ROpen(name))
    {
        // STRING: GAME 0x6325f4
        dbgPrintf("Effects.bin - Error: could not open \"%s\"", &name[0]);
        return;
    }
    ushort type = 0;
    chunk.RAscend(type);
    if (type != CT_EFFECTS_BIN)
    {
        // STRING: GAME 0x6325cc
        dbgPrintf("Effects.bin - Error: different versions");
        chunk.Close();
        return;
    }
    C_str frame_name;
    S_matrix m;
    int effect;
    while (chunk.Size())
    {
        chunk.RAscend(type);
        if (type != CT_EFFECTS_BIN_EFFECT)
        {
            // STRING: GAME 0x6325a4
            dbgPrintf("Effects.bin - Error: error in chunks..");
            chunk.Close();
            goto end;
        }
        chunk.Read(&m, sizeof(m));
        chunk.Read(&effect, sizeof(effect));
        I3D_frame *frm;
        if (chunk.Size())
        {
            chunk.RAscend(type);
            chunk.ReadString(frame_name);
            chunk.Descend();
            frm = act_mission->GetScene()->FindFrame(&frame_name[0], ENUMF_ALL);
        }
        else
            frm = NULL;
        chunk.Descend();
        CreateSParticle(effect, m, frm);
    }
    chunk.Descend();
    chunk.Close();
end:
    ;
}

