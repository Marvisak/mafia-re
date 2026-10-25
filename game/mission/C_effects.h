#pragma once

#include "Main.h"

#include <vector>

#include "C_chunk.h"

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

// A part's subpart, 0x68 bytes
// VTABLE: GAME 0x623ef4
class C_effect_subpart : public C_effect_item
{
public:
    S_vector unknown_c;                 // 0xc
    S_vector unknown_18;                // 0x18
    S_vector unknown_24;                // 0x24
    int unknown_30;                     // 0x30
    int unknown_34;                     // 0x34
    S_vector unknown_38;                // 0x38
    S_vector unknown_44;                // 0x44
    float unknown_50;                   // 0x50
    float unknown_54;                   // 0x54
    float unknown_58;                   // 0x58
    float unknown_5c;                   // 0x5c
    int unknown_60;                     // 0x60
    float unknown_64;                   // 0x64

    C_effect_subpart()
    {
        type = EFFECT_ITEM_SUBPART;
        unknown_34 = 0;
        unknown_30 = 1000;
        unknown_c = S_vector(0.0f, 1.0f, 0.0f);
        unknown_18.Zero();
        unknown_24.Zero();
        unknown_38 = S_vector(1.0f, 1.0f, 1.0f);
        unknown_44.Zero();
        unknown_50 = 1.0f;
        unknown_54 = 0.0f;
        unknown_58 = 0.0f;
        unknown_5c = 0.0f;
        unknown_60 = 1;
        unknown_64 = 9.8f;
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
                    chunk.Read(&unknown_64, sizeof(unknown_64));
                    chunk.Descend();
                    break;
                case 0x6c:
                    chunk.Read(&unknown_38, sizeof(unknown_38));
                    chunk.Read(&unknown_44, sizeof(unknown_44));
                    chunk.Descend();
                    break;
                default:            // CT_EFFECT_PARAMS
                    chunk.Read(&unknown_c, sizeof(unknown_c));
                    chunk.Read(&unknown_18, sizeof(unknown_18));
                    chunk.Read(&unknown_24, sizeof(unknown_24));
                    chunk.Read(&unknown_30, sizeof(unknown_30));
                    chunk.Read(&unknown_34, sizeof(unknown_34));
                    chunk.Read(&unknown_38, sizeof(float));
                    chunk.Read(&unknown_44, sizeof(float));
                    chunk.Read(&unknown_50, sizeof(unknown_50));
                    chunk.Read(&unknown_54, sizeof(unknown_54));
                    chunk.Read(&unknown_58, sizeof(unknown_58));
                    chunk.Read(&unknown_5c, sizeof(unknown_5c));
                    chunk.Read(&unknown_60, sizeof(unknown_60));
                    chunk.Descend();
                    break;
                }
            }
        }
    }
};

// An effect's part, 0x9c bytes
// VTABLE: GAME 0x623ef0
class C_effect_part : public C_effect_item
{
public:
    std::vector<C_effect_subpart *> subparts;   // 0xc
    C_str frame;                        // 0x1c, the name of a frame of the effect's model
    S_vector unknown_20;                // 0x20
    S_vector unknown_2c;                // 0x2c
    S_vector unknown_38;                // 0x38
    S_vector unknown_44;                // 0x44
    float unknown_50;                   // 0x50
    float unknown_54;                   // 0x54
    S_vector unknown_58;                // 0x58
    S_vector unknown_64;                // 0x64
    float unknown_70;                   // 0x70
    float unknown_74;                   // 0x74
    float unknown_78;                   // 0x78
    int unknown_7c;                     // 0x7c
    float unknown_80;                   // 0x80
    float unknown_84;                   // 0x84
    float unknown_88;                   // 0x88
    float unknown_8c;                   // 0x8c
    float unknown_90;                   // 0x90
    float unknown_94;                   // 0x94
    float unknown_98;                   // 0x98

    // The order of the two values outside their places is the original's stores'
    // FUNCTION: GAME 0x44b820
    C_effect_part()
    {
        type = EFFECT_ITEM_PART;
        unknown_90 = 0.0f;
        unknown_20.Zero();
        unknown_2c.Zero();
        unknown_38.Zero();
        unknown_44.Zero();
        unknown_50 = 1.0f;
        unknown_54 = 0.0f;
        unknown_58 = S_vector(1.0f, 1.0f, 1.0f);
        unknown_64.Zero();
        unknown_70 = 1.0f;
        unknown_74 = 0.0f;
        unknown_78 = 0.0f;
        unknown_80 = 0.0f;
        unknown_84 = 0.0f;
        unknown_88 = 0.0f;
        unknown_8c = 0.0f;
        unknown_94 = 0.0f;
        unknown_98 = 0.0f;
        unknown_7c = 0;
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
    // Open: ours sets an EH state after the subpart's base constructor, the original doesn't: VC6 leaves it out when it
    // knows the calls after it can't throw, here S_vector(float, float, float)'s copy, which ours compiles after this
    // copy (with /YX too). So the original compiled that copy earlier, most likely where I3d_math.h's deferred inline
    // functions came due in C_effects.cpp's longer functions (see the file's comment above)
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
                    chunk.Read(&unknown_38, sizeof(unknown_38));
                    chunk.Read(&unknown_44, sizeof(unknown_44));
                    chunk.Descend();
                    break;
                case 0x6c:
                    chunk.Read(&unknown_58, sizeof(unknown_58));
                    chunk.Read(&unknown_64, sizeof(unknown_64));
                    chunk.Descend();
                    break;
                case 0x6d:
                    chunk.Read(&unknown_7c, sizeof(unknown_7c));
                    chunk.Descend();
                    break;
                default:            // CT_EFFECT_PARAMS
                    chunk.Read(&unknown_20, sizeof(unknown_20));
                    chunk.Read(&unknown_2c, sizeof(unknown_2c));
                    chunk.Read(&unknown_50, sizeof(unknown_50));
                    chunk.Read(&unknown_54, sizeof(unknown_54));
                    chunk.Read(&unknown_58, sizeof(float));
                    chunk.Read(&unknown_64, sizeof(float));
                    chunk.Read(&unknown_70, sizeof(unknown_70));
                    chunk.Read(&unknown_74, sizeof(unknown_74));
                    chunk.Read(&unknown_78, sizeof(unknown_78));
                    chunk.Read(&unknown_80, sizeof(unknown_80));
                    chunk.Read(&unknown_84, sizeof(unknown_84));
                    chunk.Read(&unknown_88, sizeof(unknown_88));
                    chunk.Read(&unknown_8c, sizeof(unknown_8c));
                    chunk.Read(&unknown_90, sizeof(unknown_90));
                    chunk.Read(&unknown_94, sizeof(unknown_94));
                    chunk.Read(&unknown_98, sizeof(unknown_98));
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
    S_vector unknown_1c;                // 0x1c
    S_vector unknown_28;                // 0x28
    C_str model;                        // 0x34, the model's file (an .I3D)
    bool unknown_38;                    // 0x38
    bool unknown_39;                    // 0x39

    C_effect()
    {
        type = EFFECT_ITEM_EFFECT;
        unknown_1c.Zero();
        unknown_28.Zero();
        unknown_38 = unknown_39 = false;
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
                    chunk.Read(&unknown_38, sizeof(unknown_38));
                    chunk.Read(&unknown_39, sizeof(unknown_39));
                    chunk.Descend();
                    break;
                default:            // CT_EFFECT_PARAMS
                    chunk.Read(&unknown_1c, sizeof(unknown_1c));
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
    std::vector<void *> unknown_0;      // 0x0
    std::vector<C_effect *> effects;    // 0x10
    std::vector<void *> unknown_20;     // 0x20
    int unknown_30;                     // 0x30
    int unknown_34;                     // 0x34

    C_effects();
    ~C_effects();

    // Reads tables\effects.tbl into effects
    void Load();
    // Deletes the effects
    void Clear();
};

// The classes' scalar deleting destructors; the subparts' is folded with their base's
// SYNTHETIC: GAME 0x44be90
// ??_EC_effect_item@@UAEPAXI@Z

// The item's destructor, folded with the subparts' implicit one: which of the two copies our linker keeps changes with
// the build
// SYNTHETIC: GAME 0x44beb0
// ??1C_effect_subpart@@UAE@XZ

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
// ??1?$vector@US_anim_model@C_mission@@V?$allocator@US_anim_model@C_mission@@@std@@@std@@QAE@XZ

// The pointer vectors' constructor, which the effect's constructor calls out of line, and push_back's insert(P, x),
// which the readers call: folded with those of every other vector of pointers, whose later files' copies the original
// kept
// TEMPLATE: GAME 0x4824c0
// ?insert@?$vector@PAVC_effect@@V?$allocator@PAVC_effect@@@std@@@std@@QAEPAPAVC_effect@@PAPAV3@ABQAV3@@Z

// TEMPLATE: GAME 0x5479c0
// ??0?$vector@PAVC_effect_part@@V?$allocator@PAVC_effect_part@@@std@@@std@@QAE@ABV?$allocator@PAVC_effect_part@@@1@@Z

// The engine's S_vector(float, float, float) (I3d_math.h), which the subparts' constructor calls out of line where the
// parts' Load inlines it: the original kept the copy of the range at 0x401030 (docs/game-files.md)
// LIBRARY: GAME 0x402a30
// ??0S_vector@@QAE@MMM@Z
