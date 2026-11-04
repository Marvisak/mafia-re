#pragma once

#include "Main.h"

#include <vector>

#include "C_chunk.h"

class C_Particle;
class C_ParticleEmitter;
class C_ParticleType;
class I3D_frame;
struct S_particle_subpart;

// The effects the game loads from tables\effects.tbl ("Effects.bin" in its messages), PC only (PS2's mission has a
// particle manager instead), 0x38 bytes. The classes' and the members' names are ours. The table is a tree of chunks:
// effects, their parts and the parts' subparts, each with its name and its parameters, and each class reads its own
// chunks through an inline Load, which the reader of its parent inlines or calls (C_effects::Load inlines the
// effect's, which calls the part's, which inlines the subpart's): written into C_effects::Load, the effect's calls
// would be inline calls still to come for the nested calls before them, which then stay out of line.
// The out-of-line copies of these classes and of C_chunk.h's methods (0x44b4d0-0x44c0cf) come after the end marker
// of C_effects.cpp's range (docs/game-files.md), C_chunk.h's first, then these in this order, but they are that file's:
// C_effects::Load is the first function to construct an effect, so the copy the linker kept of its vtable's scalar
// deleting destructor (0x44bfd0) is C_effects.cpp's. That is /YX, which defers the precompiled headers' inline
// functions that never come due in a file to after its marker: ours, compiled with it, come in the original's order
// (but for the copies the original kept from other files, and the linker's choice among the folded destructors of the
// base and the subparts).

// The table's chunks (the names are ours; the values are the same at every level). Each reader takes the parameters,
// which every effect, part and subpart of the table has, in its switch's default
#define CT_EFFECT 0x64
#define CT_EFFECT_PART 0x65
#define CT_EFFECT_SUBPART 0x66
#define CT_EFFECT_NAME 0x67
#define CT_EFFECT_MODEL 0x68            // the effect's model file
#define CT_EFFECT_FRAME 0x69            // the part's frame of that model
#define CT_EFFECT_PARAMS 0x6a

// C_effect_item::type (the names are ours)
#define EFFECT_ITEM_EFFECT 1
#define EFFECT_ITEM_PART 2
#define EFFECT_ITEM_SUBPART 3

// An effect of the table, by its index (the table's name for it, "dymovnice", Czech for a smoke bomb): the scripts'
// EMITPARTICLE emits it even with the particles option at Low
#define EFFECT_DYMOVNICE 0x5a

// What the table's effects, parts and subparts have in common, deleted through its virtual destructor
// VTABLE: GAME 0x623eec
class C_effect_item
{
public:
    int type;                           // 0x4, EFFECT_ITEM_*
    C_str name;                         // 0x8

    // FUNCTION: GAME 0x44be70
    C_effect_item()
    {
        type = 0;
    }

    // Folded with the subparts' implicit destructor (annotated by name at the end)
    virtual ~C_effect_item() {}
};

// A part's subpart, 0x68 bytes: the values of the emitters' copies (S_particle_subpart, C_ParticleType.h)
// VTABLE: GAME 0x623ef4
class C_effect_subpart : public C_effect_item
{
public:
    S_vector dir;                       // 0xc
    S_vector dir_spread;                // 0x18
    S_vector rot_speed;                 // 0x24
    int duration;                       // 0x30
    int duration_range;                 // 0x34
    S_vector scale;                     // 0x38
    S_vector scale_range;               // 0x44
    float transparency;                 // 0x50
    float transparency_range;           // 0x54
    float speed_change;                 // 0x58
    float speed_change_range;           // 0x5c
    int flags;                          // 0x60, SUBPARTF_*
    float gravity;                      // 0x64

    C_effect_subpart()
    {
        type = EFFECT_ITEM_SUBPART;
        duration_range = 0;
        duration = 1000;
        dir = S_vector(0.0f, 1.0f, 0.0f);
        dir_spread.Zero();
        rot_speed.Zero();
        scale = S_vector(1.0f, 1.0f, 1.0f);
        scale_range.Zero();
        transparency = 1.0f;
        transparency_range = 0.0f;
        speed_change = 0.0f;
        speed_change_range = 0.0f;
        flags = 1;
        gravity = 9.8f;
    }

    // Reads the subpart's chunks: its name, two chunks of values and its parameters (which have only the first value of
    // each vector 0x6c has)
    void Load(C_chunk &chunk)
    {
        while (chunk.Size())
        {
            ushort t;
            if (chunk.RAscend(t))
            {
                switch (t)
                {
                case CT_EFFECT_NAME:
                    chunk.ReadString(name);
                    break;
                case 0x6b:
                    chunk.Read(&gravity, sizeof(gravity));
                    chunk.Descend();
                    break;
                case 0x6c:
                    chunk.Read(&scale, sizeof(scale));
                    chunk.Read(&scale_range, sizeof(scale_range));
                    chunk.Descend();
                    break;
                default:            // CT_EFFECT_PARAMS
                    chunk.Read(&dir, sizeof(dir));
                    chunk.Read(&dir_spread, sizeof(dir_spread));
                    chunk.Read(&rot_speed, sizeof(rot_speed));
                    chunk.Read(&duration, sizeof(duration));
                    chunk.Read(&duration_range, sizeof(duration_range));
                    chunk.Read(&scale, sizeof(float));
                    chunk.Read(&scale_range, sizeof(float));
                    chunk.Read(&transparency, sizeof(transparency));
                    chunk.Read(&transparency_range, sizeof(transparency_range));
                    chunk.Read(&speed_change, sizeof(speed_change));
                    chunk.Read(&speed_change_range, sizeof(speed_change_range));
                    chunk.Read(&flags, sizeof(flags));
                    chunk.Descend();
                    break;
                }
            }
        }
    }
};

// An effect's part, 0x9c bytes: the values of the emitters' parts (C_ParticleType, C_ParticleType.h), but unknown_94
// VTABLE: GAME 0x623ef0
class C_effect_part : public C_effect_item
{
public:
    std::vector<C_effect_subpart *> subparts;   // 0xc
    C_str frame;                        // 0x1c, the name of a frame of the effect's model
    S_vector offset;                    // 0x20
    S_vector offset_spread;             // 0x2c
    S_vector dir;                       // 0x38
    S_vector dir_spread;                // 0x44
    float speed;                        // 0x50
    float speed_spread;                 // 0x54
    S_vector scale;                     // 0x58
    S_vector scale_spread;              // 0x64
    float transparency;                 // 0x70
    float transparency_spread;          // 0x74
    float rate;                         // 0x78
    int burst;                          // 0x7c
    float rate_fall;                    // 0x80
    float fall_delay;                   // 0x84, the emitter's part takes it as an integer, as the next two
    float repeats;                      // 0x88
    float restart_delay;                // 0x8c
    int blend;                          // 0x90, an integer: the emitter's part takes it as one
    float unknown_94;                   // 0x94, if above 0, CreateParticle's fourth argument above 0 must be it
    int flags;                          // 0x98, PARTF_*

    // The order of the two values outside their places is the original's stores'
    // FUNCTION: GAME 0x44b820
    C_effect_part()
    {
        type = EFFECT_ITEM_PART;
        blend = 0;
        offset.Zero();
        offset_spread.Zero();
        dir.Zero();
        dir_spread.Zero();
        speed = 1.0f;
        speed_spread = 0.0f;
        scale = S_vector(1.0f, 1.0f, 1.0f);
        scale_spread.Zero();
        transparency = 1.0f;
        transparency_spread = 0.0f;
        rate = 0.0f;
        rate_fall = 0.0f;
        fall_delay = 0.0f;
        repeats = 0.0f;
        restart_delay = 0.0f;
        unknown_94 = 0.0f;
        flags = 0;
        burst = 0;
    }

    // FUNCTION: GAME 0x44bef0
    virtual ~C_effect_part()
    {
        while (subparts.size())
        {
            delete subparts.back();
            subparts.pop_back();
        }
    }

    // Reads the part's chunks: its subparts, its name, its frame's, two pairs of vectors, a value and its parameters
    // (which have only the first value of each vector 0x6c has).
    // No EH state after the subpart's base constructor: VC6 knows that the calls after it can't throw, here
    // S_vector(float, float, float)'s copy, which C_effects::CreateSParticle compiled before this copy (it calls the
    // constructor out of line)
    // FUNCTION: GAME 0x44b8e0
    void Load(C_chunk &chunk)
    {
        while (chunk.Size())
        {
            ushort t;
            if (chunk.RAscend(t))
            {
                switch (t)
                {
                case CT_EFFECT_SUBPART:
                {
                    C_effect_subpart *subpart = new C_effect_subpart;
                    subpart->Load(chunk);
                    subparts.push_back(subpart);
                    chunk.Descend();
                    break;
                }
                case CT_EFFECT_NAME:
                    chunk.ReadString(name);
                    break;
                case CT_EFFECT_FRAME:
                    chunk.ReadString(frame);
                    break;
                case 0x6b:
                    chunk.Read(&dir, sizeof(dir));
                    chunk.Read(&dir_spread, sizeof(dir_spread));
                    chunk.Descend();
                    break;
                case 0x6c:
                    chunk.Read(&scale, sizeof(scale));
                    chunk.Read(&scale_spread, sizeof(scale_spread));
                    chunk.Descend();
                    break;
                case 0x6d:
                    chunk.Read(&burst, sizeof(burst));
                    chunk.Descend();
                    break;
                default:            // CT_EFFECT_PARAMS
                    chunk.Read(&offset, sizeof(offset));
                    chunk.Read(&offset_spread, sizeof(offset_spread));
                    chunk.Read(&speed, sizeof(speed));
                    chunk.Read(&speed_spread, sizeof(speed_spread));
                    chunk.Read(&scale, sizeof(float));
                    chunk.Read(&scale_spread, sizeof(float));
                    chunk.Read(&transparency, sizeof(transparency));
                    chunk.Read(&transparency_spread, sizeof(transparency_spread));
                    chunk.Read(&rate, sizeof(rate));
                    chunk.Read(&rate_fall, sizeof(rate_fall));
                    chunk.Read(&fall_delay, sizeof(fall_delay));
                    chunk.Read(&repeats, sizeof(repeats));
                    chunk.Read(&restart_delay, sizeof(restart_delay));
                    chunk.Read(&blend, sizeof(blend));
                    chunk.Read(&unknown_94, sizeof(unknown_94));
                    chunk.Read(&flags, sizeof(flags));
                    chunk.Descend();
                    break;
                }
            }
        }
    }
};

// One of the table's effects, 0x3c bytes
// VTABLE: GAME 0x623ee8
class C_effect : public C_effect_item
{
public:
    std::vector<C_effect_part *> parts; // 0xc
    S_vector offset;                    // 0x1c, the particles' offset from the effect's position (C_ParticleEmitter's)
    S_vector unknown_28;                // 0x28, null in all of the table's effects; the emitter keeps it, unread
    C_str model;                        // 0x34, the model's file (an .I3D)
    bool local_dir;                     // 0x38, the emitter's direction turned by its frame's matrix
    bool local_pos;                     // 0x39, its position and offset turned by it

    C_effect()
    {
        type = EFFECT_ITEM_EFFECT;
        offset.Zero();
        unknown_28.Zero();
        local_dir = local_pos = false;
    }

    // FUNCTION: GAME 0x44bff0
    virtual ~C_effect()
    {
        while (parts.size())
        {
            delete parts.back();
            parts.pop_back();
        }
    }

    // Reads the effect's chunks: its parts, its name, its model's, its parameters and two flags
    void Load(C_chunk &chunk)
    {
        while (chunk.Size())
        {
            ushort t;
            if (chunk.RAscend(t))
            {
                switch (t)
                {
                case CT_EFFECT_PART:
                {
                    C_effect_part *part = new C_effect_part;
                    part->Load(chunk);
                    parts.push_back(part);
                    chunk.Descend();
                    break;
                }
                case CT_EFFECT_NAME:
                    chunk.ReadString(name);
                    break;
                case CT_EFFECT_MODEL:
                    chunk.ReadString(model);
                    break;
                case 0x6e:
                    chunk.Read(&local_dir, sizeof(local_dir));
                    chunk.Read(&local_pos, sizeof(local_pos));
                    chunk.Descend();
                    break;
                default:            // CT_EFFECT_PARAMS
                    chunk.Read(&offset, sizeof(offset));
                    chunk.Read(&unknown_28, sizeof(unknown_28));
                    chunk.Descend();
                    break;
                }
            }
        }
    }
};

class C_effects
{
public:
    // An emitter in use and its number (PS2's C_ParticleInUse is 16 bytes; the members' names are ours)
    struct S_particle_in_use
    {
        C_ParticleEmitter *emitter;     // 0x0
        int guid;                       // 0x4, CreateParticle's number for it (after PS2's FindGuid, which finds it)
    };

    std::vector<S_particle_in_use> in_use;  // 0x0, CreateParticle's emitters
    std::vector<C_effect *> effects;    // 0x10
    std::vector<C_ParticleEmitter *> static_emitters;  // 0x20, LoadStaticParticles' (CreateSParticle's)
    int last_guid;                      // 0x30, CreateParticle's last; -1 from the constructor, Mission_Start and Done
    // 0x34, set by Mission_Start: Mission_Done does nothing without it, and clears it (PS2's particle manager the same)
    bool started;
    uchar unknown_35[3];                // 0x35

    C_effects();
    ~C_effects();

    // Reads tables\effects.tbl into effects
    void Load();
    // Deletes the effects
    void Clear();
    // PS2's particle manager's (C_ParticleManager), which C_game's Init and Done call: the mission's effects.bin, and
    // the effects loaded if they aren't, and freed
    void LoadStaticParticles();
    bool Mission_Start();
    void Mission_Done();
    // An emitter of the effect at the position and the direction, at the frame (PS2's particle manager's; PS2 has the
    // frame fourth and two bools): its particle's number, which DestroyParticle takes, -1 for an effect not loaded
    int CreateParticle(int effect, const S_vector &pos, const S_vector &dir, int unknown, I3D_frame *frm);
    void DestroyParticle(int particle);
    // PS2's particle manager's names, which C_game::Tick calls: the emitters by the time passed, whether a particle's
    // emitter is still at work, and whether it makes new particles (the flag is C_ParticleEmitter::emitting's)
    void ClientTick(int time);
    bool IsParticleAlive(int particle);
    void PauseParticle(int particle, bool emit);
    // PS2's names: the emitters in use deleted, an effect started at a frame (with the matrix) for LoadStaticParticles
    void ClearEmitters();
    void CreateSParticle(int effect, const S_matrix &m, I3D_frame *frm);
};

// The classes' scalar deleting destructors; the subparts' is folded with their base's
// SYNTHETIC: GAME 0x44be90
// ??_EC_effect_subpart@@UAEPAXI@Z

// The item's destructor, folded with the subparts' implicit one: which of the two copies our linker keeps changes with
// the build
// SYNTHETIC: GAME 0x44beb0
// ??1C_effect_item@@UAE@XZ

// SYNTHETIC: GAME 0x44bed0
// C_effect_part::`scalar deleting destructor'

// SYNTHETIC: GAME 0x44bfd0
// C_effect::`scalar deleting destructor'

// The stack's pop_back, which the inlined Descend calls out of line in the parts' Load
// TEMPLATE: GAME 0x44be40
// std::vector<int,C_chunk_allocator<int> >::erase

// The destructor of a vector whose elements have none, which the linker folded for every such vector; the original
// kept this file's copy. C_mission's destructor calls it out of line for its models.
// TEMPLATE: GAME 0x44b220
// ??1?$vector@US_particle_in_use@C_effects@@V?$allocator@US_particle_in_use@C_effects@@@std@@@std@@QAE@XZ

// The pointer vectors' constructor, which the effect's constructor calls out of line, and push_back's insert(P, x),
// which the readers call: folded with those of every other vector of pointers, whose later files' copies the original
// kept
// TEMPLATE: GAME 0x4824c0
// ?insert@?$vector@PAVC_ParticleEmitter@@V?$allocator@PAVC_ParticleEmitter@@@std@@@std@@QAEPAPAVC_ParticleEmitter@@PAPAV3@ABQAV3@@Z

// TEMPLATE: GAME 0x5479c0
// ??0?$vector@Uai_anim_item@@V?$allocator@Uai_anim_item@@@std@@@std@@QAE@ABV?$allocator@Uai_anim_item@@@1@@Z

// The engine's S_vector(float, float, float) (I3d_math.h), which the subparts' constructor calls out of line where the
// parts' Load inlines it: the original kept the copy of the range at 0x401030 (docs/game-files.md)
// LIBRARY: GAME 0x402a30
// ??0S_vector@@QAE@MMM@Z
