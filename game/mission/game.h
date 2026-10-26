#pragma once

#include "Main.h"
#include "tools.h"
#include "actors/C_blood.h"
#include "actors/g_inventory.h"
#include "ai/C_shvestky.h"
#include "ai/C_sledovacka.h"
#include "camera/g_camera.h"
#include "mission/C_rmorf.h"
#include "vehicles/VE_BreakageGlass.h"
#include "vehicles/VE_Debris.h"

#include <string.h>
#include <vector>

class C_mission;
class IShow;

// A semaphore of the mission's traffic lights (PS2 S_semaphore, outside C_game; PS2's LoadSemaphores and
// TickSemaphores), 0x14 bytes by the game's code; what it holds isn't known yet
struct S_semaphore
{
    uchar unknown_0[0x14];
};

// The game of the loaded mission (PS2 game.cpp), 0x36ac bytes, the size C_mission::Init creates it with. PS2's Init
// takes two bools and loads the mission's collisions itself; the PC's takes none, main loads them before it.
// PS2's has most of the members in the same order (its offsets in the comments) and a few of its own; PS2's constructor,
// inlined in C_mission::Init, and its destructor, inlined in UnInit, give their types. Members without a name are named
// by their offset; the names of the others are ours, after their types. The sizes of the vectors' elements are those the
// game's code divides by; the element types whose PC layout isn't known yet are empty.
class C_game
{
public:
    // PS2's names of the vectors' elements
    struct S_box_placement_temp
    {
    };
    struct S_invisible_actor
    {
        uchar unknown_0[8];
    };
    // Its destructor's copy (0x402090) destroys the vector
    struct S_RoadBlock
    {
        int unknown_0;                  // 0x0
        std::vector<uint> unknown_4;    // 0x4, 4-byte values
    };
    struct S_Telephone
    {
        uchar unknown_0[0xc];
    };
    struct S_shadow_temp
    {
        uchar unknown_0[8];
    };
    struct S_shadow_item
    {
        uchar unknown_0[0x1c];
    };
    struct S_hydrant
    {
        uchar unknown_0[0x18];
    };
    struct S_stop_sounds
    {
        uchar unknown_0[0x14];
    };
    struct S_sound_play
    {
        uchar unknown_0[0x18];
    };
    struct S_npc_kill_event
    {
        uchar unknown_0[0x28];
    };
    struct S_driver_man
    {
        uchar unknown_0[0x10];
    };
    // A video playing in the mission (the members' names are ours)
    struct S_i_show
    {
        IShow *show;
        uchar unknown_4[0xc];
        float volume;                   // 0x10, its own, which the music's scales (PS2's IShowStoreVolume stores it)
    };
    struct S_i_show_fade
    {
        uchar unknown_0[0x10];
    };
    struct S_enemy_talk
    {
        uchar unknown_0[8];
    };
    struct S_find_actor
    {
        uchar unknown_0[0xc];
    };

    // The types PS2 doesn't name (the names are ours), by the member's offset
    struct S_unknown_0
    {
    };
    struct S_unknown_1d4
    {
        uchar unknown_0[0x40];
    };
    // Its destructor is declared: the constructor's unwinding calls it, an empty function (the folded 0x404990)
    struct S_unknown_26c
    {
        int unknown_0;                  // 0x0
        uchar unknown_4[0x10];          // 0x4

        S_unknown_26c() : unknown_0(0) {}
        ~S_unknown_26c() {}
    };
    // 0x58 bytes, of which the constructor clears the first 0x48. PS2's is 0x80, with an I3D_bbox, and clears 0x10 to 0x70.
    struct S_unknown_760
    {
        uchar unknown_0[0x48];          // 0x0
        uchar unknown_48[0x10];         // 0x48

        S_unknown_760() { memset(unknown_0, 0, sizeof(unknown_0)); }
    };
    // Cleared through a register of its own after the other members' zero stores (the game's 0x2ec8, PS2 0x12f0): an
    // inlined memset
    struct S_unknown_2ec8
    {
        int unknown_0[4];

        S_unknown_2ec8() { memset(this, 0, sizeof(*this)); }
    };
    struct S_unknown_3618
    {
        uchar unknown_0[8];
    };
    struct S_unknown_3688
    {
        uchar unknown_0[0x28];
    };

    std::vector<S_unknown_0> unknown_0;     // 0x0 (PS2 0x0, a gif::vector)
    std::vector<uint> unknown_10;       // 0x10 (PS2 0xc), 4-byte values (PS2's vector<unsigned int>), as all those below
    std::vector<uint> unknown_20;       // 0x20 (PS2 0x18)
    std::vector<S_box_placement_temp> box_placement_temp;  // 0x30 (PS2 0x24)
    // While set, G_LoadSaveClass::LanguageSelect(-1) keeps the loaded texts (PS2 0x58)
    bool unknown_40;                    // 0x40
    C_mission *mission;                 // 0x44, the mission that created it (PS2 0x5c)
    int unknown_48;                     // 0x48 (PS2 0x60, which PS2's C_mission::Init clears)
    G_Camera camera;                    // 0x4c (PS2 0x70)
    std::vector<uint> unknown_ec;       // 0xec (PS2 0x158)
    std::vector<uint> unknown_fc;       // 0xfc (PS2 0x164)
    int unknown_10c;                    // 0x10c
    std::vector<uint> unknown_110;      // 0x110 (PS2 0x174)
    std::vector<uint> unknown_120;      // 0x120 (PS2 0x180)
    std::vector<uint> unknown_130;      // 0x130 (PS2 0x18c)
    std::vector<uint> unknown_140;      // 0x140 (PS2 0x198, a gif::vector)
    std::vector<uint> unknown_150;      // 0x150 (PS2 0x1a4)
    uchar unknown_160[8];               // 0x160
    std::vector<uint> unknown_168;      // 0x168 (PS2 0x1b8)
    int unknown_178;                    // 0x178
    std::vector<S_invisible_actor> invisible_actors;  // 0x17c (PS2 0x1c8)
    S_timer unknown_18c;                // 0x18c (PS2 0x1d4)
    S_timer unknown_194;                // 0x194 (PS2 0x1dc)
    uchar unknown_19c[8];               // 0x19c
    std::vector<S_semaphore> semaphores;  // 0x1a4 (PS2 0x1ec)
    std::vector<S_RoadBlock> road_blocks;  // 0x1b4 (PS2 0x1f8)
    std::vector<S_Telephone> telephones;  // 0x1c4 (PS2 0x204)
    std::vector<S_unknown_1d4> unknown_1d4;  // 0x1d4 (PS2 0x210, a gif::vector)
    std::vector<ushort> unknown_1e4;    // 0x1e4 (PS2 0x21c)
    std::vector<uint> unknown_1f4;      // 0x1f4 (PS2 0x228)
    int unknown_204;                    // 0x204
    C_Break_plain_glass glass;          // 0x208 (PS2 0x240)
    C_Debris debris;                    // 0x240 (PS2 0x280)
    S_unknown_26c unknown_26c;          // 0x26c (PS2 0x2b0)
    std::vector<S_shadow_temp> shadow_temp;  // 0x280 (PS2 0x2c4)
    std::vector<S_shadow_item> shadow_items;  // 0x290 (PS2 0x2d0)
    std::vector<S_shadow_item> shadow_items_2a0;  // 0x2a0 (PS2 0x2dc)
    S_timer unknown_2b0;                // 0x2b0 (PS2 0x2e8)
    int unknown_2b8;                    // 0x2b8
    C_rmorf rmorf;                      // 0x2bc (PS2 0x2f4)
    std::vector<S_hydrant> hydrants;    // 0x2c4 (PS2 0x2fc, a gif::vector)
    std::vector<S_stop_sounds> stop_sounds;  // 0x2d4 (PS2 0x308, a gif::vector)
    std::vector<uint> unknown_2e4;      // 0x2e4 (PS2 0x314)
    std::vector<S_sound_play> sound_play;  // 0x2f4 (PS2 0x320)
    S_timer unknown_304;                // 0x304 (PS2 0x32c)
    uchar unknown_30c[8];               // 0x30c
    std::vector<uint> unknown_314;      // 0x314 (PS2 0x33c)
    uchar unknown_324[0x428];           // 0x324
    std::vector<uint> unknown_74c;      // 0x74c
    int unknown_75c;                    // 0x75c
    S_unknown_760 unknown_760[100];     // 0x760 (PS2 0x3b0, 20 of them)
    uchar unknown_29c0[0x2c];           // 0x29c0
    C_using_object using_object;        // 0x29ec (PS2 0xde0)
    uchar unknown_2a0c[0xd0];           // 0x2a0c, the rest of the using object's or the game's own
    std::vector<uint> unknown_2adc;     // 0x2adc (PS2 0xe30)
    std::vector<uint> unknown_2aec;     // 0x2aec (PS2 0xe3c)
    std::vector<uint> unknown_2afc;     // 0x2afc (PS2 0xe48)
    uchar unknown_2b0c[0xc];            // 0x2b0c
    C_sledovacka sledovacka;            // 0x2b18 (PS2 0xe60)
    uchar unknown_2b3c[0x22c];          // 0x2b3c (PS2's 16 entries of 0x30 bytes from 0xe80 among them)
    std::vector<S_npc_kill_event> npc_kill_events;  // 0x2d68 (PS2 0x11b4)
    std::vector<S_driver_man> driver_men;  // 0x2d78 (PS2 0x11c0)
    uchar unknown_2d88[8];              // 0x2d88
    C_blood_manager blood_manager;      // 0x2d90 (PS2 0x11d4)
    C_shvestky shvestky;                // 0x2da0 (PS2 0x11e0)
    // What InventoryStackPush, CharacterStackPush and FloatStackPush keep of the player between missions (PS2's
    // functions' names; the members' are ours): copies of inventories, a human's values, floats
    std::vector<G_Inventory *> inventory_stack;  // 0x2e50 (PS2 0x1278)
    std::vector<uint> character_stack;  // 0x2e60 (PS2 0x1284)
    std::vector<float> float_stack;     // 0x2e70 (PS2 0x1290)
    std::vector<uint> unknown_2e80;     // 0x2e80 (PS2 0x129c)
    std::vector<uint> unknown_2e90;     // 0x2e90 (PS2 0x12a8)
    uchar unknown_2ea0[0x10];           // 0x2ea0
    int unknown_2eb0;                   // 0x2eb0 (PS2 0x12d4)
    int unknown_2eb4;                   // 0x2eb4
    int unknown_2eb8;                   // 0x2eb8
    int unknown_2ebc;                   // 0x2ebc
    int unknown_2ec0;                   // 0x2ec0
    bool unknown_2ec4;                  // 0x2ec4 (PS2 0x12e8)
    S_unknown_2ec8 unknown_2ec8;        // 0x2ec8 (PS2 0x12f0)
    uchar unknown_2ed8[0xc];            // 0x2ed8
    std::vector<uint> unknown_2ee4;     // 0x2ee4 (PS2 0x130c)
    uchar unknown_2ef4[0x24];           // 0x2ef4
    std::vector<S_i_show> i_shows;      // 0x2f18 (PS2 0x133c)
    std::vector<S_i_show_fade> i_show_fades;  // 0x2f28 (PS2 0x1348)
    uchar unknown_2f38[8];              // 0x2f38
    std::vector<S_enemy_talk> enemy_talks;  // 0x2f40 (PS2 0x135c)
    uchar unknown_2f50[0x2c];           // 0x2f50
    std::vector<S_find_actor> find_actors;  // 0x2f7c (PS2 0x1394)
    std::vector<uint> unknown_2f8c;     // 0x2f8c (PS2 0x13a0)
    int unknown_2f9c;                   // 0x2f9c
    std::vector<uint> unknown_2fa0;     // 0x2fa0 (PS2 0x13b0)
    uchar unknown_2fb0[0x14];           // 0x2fb0
    std::vector<uint> unknown_2fc4;     // 0x2fc4
    uchar unknown_2fd4[0x644];          // 0x2fd4
    std::vector<S_unknown_3618> unknown_3618;  // 0x3618
    uchar unknown_3628[0x14];           // 0x3628
    bool unknown_363c;                  // 0x363c, set by C_mission::Open (PS2 0x1538 the same)
    bool unknown_363d;                  // 0x363d, cleared by C_mission::Open (PS2 0x1539 the same)
    uchar unknown_363e[0x36];           // 0x363e
    std::vector<uint> unknown_3674;     // 0x3674
    int unknown_3684;                   // 0x3684
    std::vector<S_unknown_3688> unknown_3688;  // 0x3688
    uchar unknown_3698[0x14];           // 0x3698

    // Inline in the original, as PS2's C_mission::Init shows (it has it inlined): the copy C_mission::Init calls is in
    // Mission.cpp's range. The stores come in the members' order: an initializer list.
    // FUNCTION: GAME 0x5472f0
    C_game() :
        unknown_40(false),
        unknown_2eb0(0),
        unknown_2eb8(0),
        unknown_2ebc(0),
        unknown_2ec0(0),
        unknown_2ec4(false)
    {
    }

    // Inline in the original too, as PS2's C_mission::UnInit shows (it has it inlined): the copy C_mission::UnInit calls
    // is in Mission.cpp's range. The stacks emptied, then the members destroyed: the vectors from 0x3688 to 0x2d68 with
    // their ~vector inlined and _Destroy called (an empty ret 8), those from 0x2adc down with ~vector called, as the
    // original. Open: the original also inlines _Destroy (nothing left of it) in 0x2ee4's, 0x2afc's and 0x2aec's. Not
    // the element types (scalars, pointers, char's specialization, classes with destructors), padding (any amount
    // inlines 0x2adc's ~vector first), fewer pending inline calls (20 fewer flip the first three), a struct or base
    // class around them, nor an earlier function inlining their clear() change only those three. Open too: the EH state
    // stores before some of the out-of-line ~vector calls, which the original has none of. They come and go with
    // Mission.cpp's other functions (AddActor, DelActors and FrmActBuff_ClearAll added several), though the copy is
    // compiled after them, at the object's end.
    // FUNCTION: GAME 0x547a60
    ~C_game()
    {
        InventoryStackClear();
        CharacterStackClear();
        FloatStackClear();
    }

    void Init();
    // PS2 names: what C_mission::Close calls first, when a mission ends
    void Done();
    void DoneIntern();

    // Empty the stacks, the inventories' copies deleted
    void InventoryStackClear();
    void FloatStackClear();
    void CharacterStackClear();
    // The videos' volumes from the profile's music volume (PS2's sets its one video's)
    void IShowSetVolume();

    // The name is ours. C_mission::Open sets both through one load of its game, so through a method: written out, VC6
    // reloads the pointer after the first store.
    void ResetUnknown363c()
    {
        unknown_363c = true;
        unknown_363d = false;
    }
};

// The road blocks' destructors, which the constructor's unwinding and the destructor call: the original kept the copies
// of the range at 0x401030 (docs/game-files.md) of their folded groups
// TEMPLATE: GAME 0x4036f0
// ??1?$vector@US_RoadBlock@C_game@@V?$allocator@US_RoadBlock@C_game@@@std@@@std@@QAE@XZ

// SYNTHETIC: GAME 0x402090
// ??1S_RoadBlock@C_game@@QAE@XZ
