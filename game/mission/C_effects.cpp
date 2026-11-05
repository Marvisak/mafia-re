// The effects of tables\effects.tbl, PC only: the file at 0x448f00-0x44b4cc (docs/game-files.md), which starts with
// the initializers of the emitters' free objects, then C_effects' constructor; the name is ours (the file sorts where
// PS2's C_ParticleManager.cpp would, whose class has the functions C_game calls). Written so far: the free objects'
// vectors, the constructor, the destructor, Load and Clear, which C_mission's Init and UnInit run, and with Load the
// effects' classes (C_effects.h), Mission_Start, Mission_Done, ClearEmitters and LoadStaticParticles, which C_game's
// Init and Done run, CreateSParticle, which LoadStaticParticles runs, and ClientTick, which C_game::Tick calls;
// CreateParticle, which the scripts call, DestroyParticle, and IsParticleAlive and PauseParticle, which C_game::Tick
// calls, are stubs.

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
    last_guid = -1;
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
    started = false;
    last_guid = -1;
    if (!effects.size())
        Load();
    started = true;
    return true;
}

// The emitters in use and every free object deleted (PS2's deletes its emitters the same way)
// FUNCTION: GAME 0x4491d0
void C_effects::Mission_Done()
{
    if (!started)
        return;
    ClearEmitters();
    last_guid = -1;
    for (uint i = 0; i < static_emitters.size(); i++)
        delete static_emitters[i];
    static_emitters.clear();
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
    started = false;
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

// STUB: GAME 0x449690
int C_effects::CreateParticle(int effect, const S_vector &pos, const S_vector &dir, int unknown, I3D_frame *frm)
{
    StubLog("C_effects::CreateParticle");
    return -1;
}

// The static emitters, then, once a mission has started, those in use: one whose Tick ends is reset and goes back to
// the free ones (PS2's particle manager's the same, with one counter). The counter's decrement in the erase's argument
// comes before the erase's copy loop, as the original's; a statement of its own after it comes after.
// FUNCTION: GAME 0x44a0d0
void C_effects::ClientTick(int time)
{
    uint i;
    for (i = 0; i < static_emitters.size(); i++)
        static_emitters[i]->Tick(time);
    if (last_guid == -1)
        return;
    for (i = 0; i < in_use.size(); i++)
    {
        if (!in_use[i].emitter->Tick(time))
        {
            in_use[i].emitter->Reset();
            particle_emitters.push_back(in_use[i].emitter);
            in_use.erase(in_use.begin() + i--);
        }
    }
}

// STUB: GAME 0x44a240
void C_effects::DestroyParticle(int particle)
{
    StubLog("C_effects::DestroyParticle");
}

// STUB: GAME 0x44a2b0
bool C_effects::IsParticleAlive(int particle)
{
    StubLog("C_effects::IsParticleAlive");
    return false;
}

// STUB: GAME 0x44a2d0
void C_effects::PauseParticle(int particle, bool emit)
{
    StubLog("C_effects::PauseParticle");
}

// FUNCTION: GAME 0x44a2f0
void C_effects::ClearEmitters()
{
    for (uint i = 0; i < in_use.size(); i++)
        delete in_use[i].emitter;
    in_use.clear();
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
    emitter->Init(frm, m(3), m(1), e->local_pos, e->local_dir, e->offset, e->unknown_28);
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
                type->Init(frm, f, part->offset, part->offset_spread, part->burst, part->rate,
                    part->rate_fall, (int)part->fall_delay, (int)part->repeats, (int)part->restart_delay);
                type->flags = part->flags;
                type->scale = part->scale;
                type->scale_spread = part->scale_spread;
                type->SetTransparency(part->transparency, part->transparency_spread);
                type->blend = part->blend;
                type->SetSpeed(part->speed, part->speed_spread);
                type->dir = part->dir;
                type->dir_spread = part->dir_spread;
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
                    subpart->SetValues(sub->duration, sub->duration_range, sub->flags, sub->dir,
                        sub->dir_spread, sub->rot_speed);
                    subpart->scale = sub->scale;
                    subpart->scale_range = sub->scale_range;
                    subpart->SetTransparency(sub->transparency, sub->transparency_range);
                    subpart->SetMotion(sub->speed_change, sub->speed_change_range, sub->gravity);
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
        static_emitters.push_back(emitter);
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
