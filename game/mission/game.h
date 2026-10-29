#pragma once

#include "Main.h"
#include "tools.h"
#include "actors/C_blood.h"
#include "actors/g_inventory.h"
#include "ai/C_shvestky.h"
#include "ai/C_sledovacka.h"
#include "camera/g_camera.h"
#include "collision/g_colls.h"
#include "mission/C_rmorf.h"
#include "vehicles/VE_BreakageGlass.h"
#include "vehicles/VE_Debris.h"

#include <string.h>
#include <vector>

#include "I3D/I3D.h"

class C_mission;
class C_human;
class C_car;
class C_stone;
class C_cartridge;
class C_don_ocas;
class IShow;
class ITexture;
class C_program;
struct S_cache_model_inbb;

// A semaphore of the mission's traffic lights (PS2 S_semaphore, outside C_game; PS2's LoadSemaphores and
// TickSemaphores), 0x14 bytes: its three lights, a model's frames, and the phase of its direction (the members' names
// are ours)
struct S_semaphore
{
    uint unknown_0;                     // 0x0, LoadSemaphores leaves it
    I3D_frame *red;                     // 0x4
    I3D_frame *orange;                  // 0x8
    I3D_frame *green;                   // 0xc
    uint *phase;                        // 0x10, C_game's unknown_19c or unknown_1a0, by the semaphore's direction
};

// The shadows' types, which AddFrameForShadow and DelFrameForShadow take (PS2's E_shadow_type, a global enum; the
// enumerator's name is ours, by value): FRM_SETON passes 0
enum E_shadow_type { SHADOW_TYPE_0 };

// C_game::unknown_2edc, RACING_MISSION6_START's states (the names are ours): the command starts the race at
// RACE6_STATE_NONE and goes on at RACE6_STATE_END
#define RACE6_STATE_NONE 0
#define RACE6_STATE_STARTED 2           // SetUnknown2edc(true)
#define RACE6_STATE_END 10

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
        uchar unknown_0[0x20];
    };
    struct S_invisible_actor
    {
        uchar unknown_0[8];
    };
    // Its destructor's copy (0x402090) destroys the vector
    struct S_RoadBlock
    {
        ushort node;                    // 0x0, the roads' node nearest to the road block's dummy (the name is ours)
        bool unknown_2;                 // 0x2
        std::vector<uint> unknown_4;    // 0x4, 4-byte values
    };
    // A telephone, a model of alarm01.i3d (the police's alarm boxes) or PHbox01.i3d (the phone boxes), in the scene or
    // in the base blocks (PS2 name; the members' names are ours)
    struct S_Telephone
    {
        I3D_frame *frame;               // 0x0, the scene's model
        S_cache_model_inbb *cache_model;  // 0x4, or the base blocks' model
        bool phone_box;                 // 0x8
    };
    // A shadow opened from a file, which the shadows of the frames copy (AddShadowModel's; the members' names are ours)
    struct S_shadow_temp
    {
        I3D_shadow *model;              // 0x0
        char *name;                     // 0x4, the file's name without its path, which DoneIntern deletes
    };
    // A frame's shadow, which the shadows' tick sets up for the nearest ones (the members' names are ours)
    struct S_shadow_item
    {
        I3D_frame *frame;               // 0x0
        I3D_shadow *model;              // 0x4, a copy of one of the shadow models (SetupShadow)
        I3D_frame *unknown_8;           // 0x8, turned off while the shadow is on
        bool on;                        // 0xc
        char *name;                     // 0x10, the shadow model's file name, or NULL for the model at unknown_14
        uint unknown_14;                // 0x14, an index into the shadow models
        float dist;                     // 0x18, from the camera, which the tick sorts them by
    };
    // A hydrant's water (the members' names are ours)
    struct S_hydrant
    {
        int particle;                   // 0x0, the effects' particle (C_effects::DestroyParticle's)
        uchar unknown_4[0x10];          // 0x4
        I3D_frame *frame;               // 0x14
    };
    struct S_stop_sounds
    {
        uchar unknown_0[0x14];
    };
    // A sound the game plays (the members' names are ours)
    struct S_sound_play
    {
        I3D_sound *sound;               // 0x0
        uchar unknown_4[0x14];          // 0x4
    };
    struct S_npc_kill_event
    {
        uchar unknown_0[0x28];
    };
    // A driver's model of a car (PS2 AddDriverMan's; the members' names are ours)
    struct S_driver_man
    {
        I3D_model *model;               // 0x0
        uint unknown_4;                 // 0x4
        uint anim;                      // 0x8, the animation Done stops
        uint unknown_c;                 // 0xc
    };
    // A video playing in the mission (the members' names are ours)
    struct S_i_show
    {
        IShow *show;
        uchar unknown_4[8];
        char *unknown_c;                // 0xc, DestroyIShowAll deletes it
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
    // What LightnessClear deletes (PS2's Lightness functions'); what it holds isn't known yet
    struct S_lightness
    {
    };
    // What BoxPlacementDel is given the frame of (PS2's BoxPlacement functions')
    struct S_unknown_2e80
    {
        I3D_frame *frame;               // 0x0
    };
    // A frame's state, which Done puts back (PS2 name, PS2's 0x40 bytes; the members' names are ours)
    struct S_frame_placement
    {
        I3D_frame *frame;               // 0x0
        S_vector pos;                   // 0x4
        S_quat rot;                     // 0x10
        S_vector scale;                 // 0x20
        I3D_frame *parent;              // 0x2c
        bool on;                        // 0x30
    };
    struct S_unknown_1d4
    {
        uchar unknown_0[0x40];
    };
    // 0x58 bytes, of which only the collision object is cleared, by its constructor. PS2's is 0x80, with an I3D_bbox,
    // and clears 0x10 to 0x70. The empty constructor keeps C_game's destructor as it is: with the implicit one VC6
    // inlines less in it.
    struct S_unknown_75c
    {
        I3D_frame *frame;               // 0x0
        tDynamicCollObject coll;        // 0x4
        bool in_collision;              // 0x4c, whether coll is among gifCollision's dynamic objects
        uchar unknown_4d[0xb];          // 0x4d

        S_unknown_75c() {}
    };
    // Cleared through a register of its own after the other members' zero stores (the game's 0x2ec8, PS2 0x12f0): an
    // inlined memset
    struct S_unknown_2ec8
    {
        int unknown_0[4];

        S_unknown_2ec8() { memset(this, 0, sizeof(*this)); }
    };
    struct S_unknown_2a48
    {
        I3D_light *light;               // 0x0
        int unknown_4;                  // 0x4, -1 from Init
        int unknown_8;                  // 0x8
    };
    struct S_unknown_2ac0
    {
        I3D_sound *sound;               // 0x0
        int unknown_4;                  // 0x4, -1 from Init
    };
    struct S_unknown_2fd8
    {
        I3D_frame *frame;               // 0x0
        int unknown_4;                  // 0x4
        int unknown_8;                  // 0x8
        int unknown_c;                  // 0xc
    };
    // An area where the enemies scream (enm_scrm_..., the members' names are ours)
    struct S_unknown_3618
    {
        I3D_frame *area;                // 0x0
        ushort id;                      // 0x4, the hexadecimal number in the name's parentheses
    };
    struct S_unknown_3688
    {
        uchar unknown_0[0x28];
    };

    std::vector<S_frame_placement> frame_placements;  // 0x0 (PS2 0x0, a gif::vector; the name is ours)
    std::vector<I3D_frame *> unknown_10;  // 0x10 (PS2 0xc), turned on again by Done
    std::vector<I3D_frame *> unknown_20;  // 0x20 (PS2 0x18), unlinked and released by Done
    std::vector<S_box_placement_temp> box_placement_temp;  // 0x30 (PS2 0x24)
    // Init has run, until Done; while set, G_LoadSaveClass::LanguageSelect(-1) keeps the loaded texts (PS2 0x58; the name
    // is ours)
    bool initialized;                   // 0x40
    C_mission *mission;                 // 0x44, the mission that created it (PS2 0x5c)
    int unknown_48;                     // 0x48 (PS2 0x60, which PS2's C_mission::Init clears)
    G_Camera camera;                    // 0x4c (PS2 0x70)
    // 0xe4, the player: the mission's last actor of ACTOR_PLAYER (PS2 0x154, which PS2's SetHuman sets; the name is ours)
    C_human *player;
    C_human *unknown_e8;                // 0xe8, the player too after Init
    std::vector<C_actor *> unknown_ec;  // 0xec (PS2 0x158), the mission's actors, the last one first
    std::vector<C_actor *> unknown_fc;  // 0xfc (PS2 0x164), actors, the garage's cars among them
    bool unknown_10c;                   // 0x10c, an actor's state changed (C_actor::SetActState sets it)
    std::vector<uint> unknown_110;      // 0x110 (PS2 0x174)
    std::vector<C_actor *> temporary_actors;  // 0x120 (PS2 0x180, its Temporary functions')
    std::vector<C_actor *> unknown_130;  // 0x130 (PS2 0x18c), the temporary actors being removed (RemoveTemporaryActor's)
    std::vector<C_actor *> unknown_140;  // 0x140 (PS2 0x198, a gif::vector)
    std::vector<S_lightness *> lightness;  // 0x150 (PS2 0x1a4), LightnessClear deletes them
    int unknown_160;                    // 0x160
    bool actor_lightness;               // 0x164, ACTORLIGHTNESSON's (the name is ours)
    std::vector<I3D_frame *> unknown_168;  // 0x168 (PS2 0x1b8), turned on again by Done
    int unknown_178;                    // 0x178
    std::vector<S_invisible_actor> invisible_actors;  // 0x17c (PS2 0x1c8)
    S_timer unknown_18c;                // 0x18c (PS2 0x1d4), the traffic generators' choice
    S_timer unknown_194;                // 0x194 (PS2 0x1dc)
    // 0x19c, the semaphores' phases, one per direction, 0 and 5 from Init (no S_timer: the constructor leaves them):
    // indices into game.cpp's semaphore_lights
    uint unknown_19c;
    uint unknown_1a0;                   // 0x1a0
    std::vector<S_semaphore> semaphores;  // 0x1a4 (PS2 0x1ec)
    std::vector<S_RoadBlock> road_blocks;  // 0x1b4 (PS2 0x1f8)
    std::vector<S_Telephone> telephones;  // 0x1c4 (PS2 0x204)
    std::vector<S_unknown_1d4> unknown_1d4;  // 0x1d4 (PS2 0x210, a gif::vector)
    std::vector<ushort> unknown_1e4;    // 0x1e4 (PS2 0x21c), the base blocks' model sets of the traffic signs
    std::vector<I3D_frame *> taxi_points;  // 0x1f4 (PS2 0x228), the dummies taxi_point*
    int unknown_204;                    // 0x204
    C_Break_plain_glass glass;          // 0x208 (PS2 0x240)
    C_Debris debris;                    // 0x240 (PS2 0x280)
    C_compass compass;                  // 0x26c (PS2 0x2b0)
    std::vector<S_shadow_temp> shadow_temp;  // 0x280 (PS2 0x2c4)
    std::vector<S_shadow_item> shadow_items;  // 0x290 (PS2 0x2d0)
    std::vector<S_shadow_item> shadow_items_2a0;  // 0x2a0 (PS2 0x2dc)
    S_timer unknown_2b0;                // 0x2b0 (PS2 0x2e8), the shadows' tick
    bool unknown_2b8;                   // 0x2b8, which of the two lists the shadows' tick sorts next
    C_rmorf rmorf;                      // 0x2bc (PS2 0x2f4)
    std::vector<S_hydrant> hydrants;    // 0x2c4 (PS2 0x2fc, a gif::vector)
    std::vector<S_stop_sounds> stop_sounds;  // 0x2d4 (PS2 0x308, a gif::vector)
    std::vector<uint> unknown_2e4;      // 0x2e4 (PS2 0x314)
    std::vector<S_sound_play> sound_play;  // 0x2f4 (PS2 0x320)
    S_timer unknown_304;                // 0x304 (PS2 0x32c)
    int unknown_30c;                    // 0x30c
    int unknown_310;                    // 0x310
    std::vector<C_fire *> fires;        // 0x314 (PS2 0x33c), Done deletes them
    // The actors Init creates for the game from 2stone.i3d (PC only) and the cartridges' models, each with a count
    C_stone *stones[32];                // 0x324
    int num_stones;                     // 0x3a4
    C_cartridge *cartridges[100];       // 0x3a8, 2nabojnice.i3d
    int num_cartridges;                 // 0x538
    C_cartridge *cartridges_b[30];      // 0x53c, 2nabojniceb.i3d
    int num_cartridges_b;               // 0x5b4
    I3D_frame *holes[100];              // 0x5b8, copies of the holes' models' dUra_fiRe
    int num_holes;                      // 0x748
    std::vector<I3D_frame *> hole_models;  // 0x74c, a dUra_fiRe of each of the holes' models
    S_unknown_75c unknown_75c[100];     // 0x75c (PS2 0x3b0, 20 of them)
    ITexture *unknown_29bc[10];         // 0x29bc, the blood's, the dirt's and the scratches' textures
    int unknown_29e4;                   // 0x29e4
    int unknown_29e8;                   // 0x29e8
    C_using_object using_object;        // 0x29ec (PS2 0xde0)
    int unknown_2a0c;                   // 0x2a0c
    S_vector unknown_2a10;              // 0x2a10, where the player or else the camera is at Init
    C_don_ocas *don_ocas;               // 0x2a1c
    I3D_sound *unknown_2a20[10];        // 0x2a20
    S_unknown_2a48 unknown_2a48[10];    // 0x2a48
    S_unknown_2ac0 unknown_2ac0[3];     // 0x2ac0
    bool unknown_2ad8;                  // 0x2ad8
    bool unknown_2ad9;                  // 0x2ad9
    std::vector<I3D_frame *> unknown_2adc;  // 0x2adc (PS2 0xe30), frames ClearDiffData deletes from the scene
    std::vector<C_actor *> unknown_2aec;  // 0x2aec (PS2 0xe3c), actors ClearDiffData deletes from the mission
    std::vector<C_program *> unknown_2afc;  // 0x2afc (PS2 0xe48), the game's own scripts (GlobalProgramRun's)
    int game_time;                      // 0x2b0c, the game's time (the name is ours)
    int unknown_2b10;                   // 0x2b10
    uint unknown_2b14;                  // 0x2b14, unsigned (CAMERA_LOCK compares it so)
    C_sledovacka sledovacka;            // 0x2b18 (PS2 0xe60)
    soundArena sound_arena;             // 0x2b3c (PS2's among the 16 entries of 0x30 bytes from 0xe80)
    bool night_mission;                 // 0x2d44, GAME_NIGHTMISSION's (PS2 0x1190; the name is ours)
    // The scripts' events the game fires and the scripts to fire them in (PS2 0x1194 to 0x11b0, which PS2's
    // Timer_TimeOut, PFire_Fire, PHorn_Horn and WFall_Player fire; the names are ours)
    C_program *timeout_program;         // 0x2d48
    int timeout_event;                  // 0x2d4c
    C_program *player_fire_program;     // 0x2d50
    int player_fire_event;              // 0x2d54
    C_program *player_horn_program;     // 0x2d58
    int player_horn_event;              // 0x2d5c
    C_program *player_wfall_program;    // 0x2d60
    int player_wfall_event;             // 0x2d64
    std::vector<S_npc_kill_event> npc_kill_events;  // 0x2d68 (PS2 0x11b4)
    std::vector<S_driver_man> driver_men;  // 0x2d78 (PS2 0x11c0)
    // The steering wheels' animations' lengths, AutoRidicVolant.I3D's and FormuleVolant.I3D's (the names are ours)
    float wheel_anim_time;              // 0x2d88
    float wheel_anim_time_formula;      // 0x2d8c
    C_blood_manager blood_manager;      // 0x2d90 (PS2 0x11d4)
    C_shvestky shvestky;                // 0x2da0 (PS2 0x11e0)
    // What InventoryStackPush, CharacterStackPush and FloatStackPush keep of the player between missions (PS2's
    // functions' names; the members' are ours): copies of inventories, a human's values, floats
    std::vector<G_Inventory *> inventory_stack;  // 0x2e50 (PS2 0x1278)
    std::vector<uint> character_stack;  // 0x2e60 (PS2 0x1284)
    std::vector<float> float_stack;     // 0x2e70 (PS2 0x1290)
    std::vector<S_unknown_2e80 *> unknown_2e80;  // 0x2e80 (PS2 0x129c)
    std::vector<uchar *> unknown_2e90;  // 0x2e90 (PS2 0x12a8), Done deletes them
    uchar unknown_2ea0[0x10];           // 0x2ea0
    int unknown_2eb0;                   // 0x2eb0 (PS2 0x12d4)
    int unknown_2eb4;                   // 0x2eb4
    int unknown_2eb8;                   // 0x2eb8
    int unknown_2ebc;                   // 0x2ebc
    int unknown_2ec0;                   // 0x2ec0
    bool unknown_2ec4;                  // 0x2ec4 (PS2 0x12e8)
    S_unknown_2ec8 unknown_2ec8;        // 0x2ec8 (PS2 0x12f0)
    bool unknown_2ed8;                  // 0x2ed8, set by Done
    int unknown_2edc;                   // 0x2edc, RACE6_STATE_*
    float *unknown_2ee0;                // 0x2ee0, a float register of a script (the race's result most likely)
    std::vector<uint> unknown_2ee4;     // 0x2ee4 (PS2 0x130c)
    char unknown_2ef4[0x20];            // 0x2ef4, a name, empty from Init
    int unknown_2f14;                   // 0x2f14
    std::vector<S_i_show> i_shows;      // 0x2f18 (PS2 0x133c)
    std::vector<S_i_show_fade> i_show_fades;  // 0x2f28 (PS2 0x1348)
    int unknown_2f38;                   // 0x2f38
    int unknown_2f3c;                   // 0x2f3c
    std::vector<S_enemy_talk> enemy_talks;  // 0x2f40 (PS2 0x135c)
    I3D_frame *unknown_2f50;            // 0x2f50
    I3D_frame *unknown_2f54[2];         // 0x2f54
    uchar unknown_2f5c[0x10];           // 0x2f5c
    int unknown_2f6c;                   // 0x2f6c
    int unknown_2f70;                   // 0x2f70
    uchar unknown_2f74[8];              // 0x2f74
    std::vector<S_find_actor> find_actors;  // 0x2f7c (PS2 0x1394)
    std::vector<uint> unknown_2f8c;     // 0x2f8c (PS2 0x13a0)
    bool anim_preload;                  // 0x2f9c, NOANIMPRELOAD clears it (the name is ours)
    std::vector<I3D_frame *> city_music_areas;  // 0x2fa0 (PS2 0x13b0), the areas city_music_*
    int unknown_2fb0;                   // 0x2fb0
    int unknown_2fb4;                   // 0x2fb4
    int unknown_2fb8;                   // 0x2fb8
    int unknown_2fbc;                   // 0x2fbc
    int unknown_2fc0;                   // 0x2fc0
    std::vector<uint> unknown_2fc4;     // 0x2fc4
    int unknown_2fd4;                   // 0x2fd4
    S_unknown_2fd8 unknown_2fd8[100];   // 0x2fd8
    std::vector<S_unknown_3618> unknown_3618;  // 0x3618
    int unknown_3628;                   // 0x3628
    int unknown_362c;                   // 0x362c
    // The scene's weather system's WS_PARAM_ON and WS_PARAM_COUNT when Init started, which Done sets again (the names are
    // ours)
    bool weather_on;                    // 0x3630
    int weather_count;                  // 0x3634, signed (OsefujPocasi divides it so)
    int unknown_3638;                   // 0x3638
    bool unknown_363c;                  // 0x363c, set by C_mission::Open (PS2 0x1538 the same): Init loads more with it
    bool unknown_363d;                  // 0x363d, cleared by C_mission::Open (PS2 0x1539 the same): DoneIntern's
    bool score_on;                      // 0x363e (PS2 ScoreSetOn's)
    int score;                          // 0x3640
    bool unknown_3644;                  // 0x3644
    uchar unknown_3645[0x13];           // 0x3645
    I3D_model *sheet;                   // 0x3658, 9sheet.i3d
    I3D_frame *unknown_365c;            // 0x365c
    I3D_model *money;                   // 0x3660, 9money.i3d
    bool unknown_3664;                  // 0x3664
    bool unknown_3665;                  // 0x3665
    int unknown_3668;                   // 0x3668
    uchar unknown_366c[8];              // 0x366c
    std::vector<uchar *> unknown_3674;  // 0x3674, Done deletes them
    int unknown_3684;                   // 0x3684
    std::vector<S_unknown_3688> death_human_fades;  // 0x3688 (PS2's DeathHumanFade functions')
    // The inter-city's collisions, frames and their arrays, which InterCity_GameInit creates with the city's base blocks
    // (the types are ours)
    tDynamicCollObject **unknown_3698;  // 0x3698
    S_matrix *unknown_369c;             // 0x369c, the collisions' matrices
    S_matrix *unknown_36a0;             // 0x36a0, and their inverses
    int unknown_36a4;                   // 0x36a4, the arrays' count
    I3D_frame **unknown_36a8;           // 0x36a8

    // Inline in the original, as PS2's C_mission::Init shows (it has it inlined): the copy C_mission::Init calls is in
    // Mission.cpp's range. The stores come in the members' order: an initializer list.
    // FUNCTION: GAME 0x5472f0
    C_game() :
        initialized(false),
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
    // class around them, nor an earlier function inlining their clear() change only those three. With the members'
    // types of 2026-10-27 (vectors of distinct pointer types where they were vector<uint>) ours inlines it in 0x2ee4's
    // and 0x2aec's as the original, but also in 0x2fc4's, 0x2f8c's and 0x2e60's, and calls it in 0x2afc's: the
    // instantiations a member's type picks move the decisions. Open too: the EH state
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

    // Sets the game up for the open mission (main calls it after each Open), false if it is set up already. PS2's
    // takes two bools and loads the mission's collisions itself, the PC's main loads them before it.
    bool Init();
    // PS2 names: what C_mission::Close calls first, when a mission ends. Done is false if Init hasn't run.
    bool Done();
    void DoneIntern();
    // A frame of the game, which C_mission::Tick runs once Init has (PS2 name and parameter)
    void Tick(uint time);

    // Empty the stacks, the inventories' copies deleted
    void InventoryStackClear();
    void FloatStackClear();
    void CharacterStackClear();
    // The videos' volumes from the profile's music volume (PS2's sets its one video's)
    void IShowSetVolume();

    // PS2 names, of what Init and Done call
    void ScoreSetOn(bool on);
    void LightnessClear();
    void ChangeActState();
    // A temporary actor to remove: its state ACTSTATE_2, once (PS2 name)
    void RemoveTemporaryActor(C_actor *act);
    // Takes a temporary actor out of the game's lists, unless it is being removed (PS2 name)
    void InternTempActDel(C_actor *act);
    void ClearTemporaryActors();
    void ClearEffects();
    void LoadSemaphores();
    void FreeSemaphores();
    void TickSemaphores();
    void LoadCityMusicAreas();
    void ClearDiffData();
    void BoxPlacementDel(I3D_frame *frm);
    void DestroyIShowAll();
    void LoadRoadBlocks();
    void LoadTaxiPoints();
    void LoadTelephones();
    void CreateSignalMarkIDX();
    // PS2 name: each ACTOR_ENTITY of unknown_ec and unknown_fc, from the last, blocked (C_entity::SedniNaPrdelASledujKino,
    // with the second flag) or woken (VzbudSeUzSouTitulky)
    void EntityBlock(bool block, bool unknown);
    void StopAllSounds();
    void LoadScreamAreas();
    // The weather (PS2 name: Czech for "deal with the weather")
    void OsefujPocasi(bool read, uint param);
    void DelAllHydrants();
    void ClearDeathHumanFade();
    // Takes a death fade's human back (the name is ours)
    void RemoveDeathHumanFade(int index, bool);
    // Takes an actor out of everything that refers to it (PS2 name)
    void InvalidateActor(C_actor *act, bool);
    // A frame going away: its placements and its subtree's taken from the list (PS2 name)
    void InvalidateFrame(I3D_frame *frm);
    // The actor whose id is given, the scripts' FindActorLater's (PS2 name)
    C_actor *GetActorFromGUID(uint id);
    // Runs the game's own script of that name (PS2's is C_mission's): false without one
    bool GlobalProgramRun(const char *name);
    // PS2's C_mission has these two
    void InterCity_GameInit();
    void InterCity_GameDoneInGameDone();
    // Inlined in Init (PS2's are methods; its TickShadows is empty)
    void TickShadows(uint time);
    void SetActiveTrafficP(uint time);
    void PreloadAnims();
    // A shadow's model for the shadows' lists (the name is ours)
    void AddShadowModel(const char *name);
    // Puts a shadow's model on its frame (the name is ours)
    void SetupShadow(S_shadow_item *item);
    // The shadows' order for qsort, by their distance (the name is ours)
    static int __cdecl CompareShadows(const S_shadow_item *a, const S_shadow_item *b);

    // What the scripts call (C_program::Process), PS2 names but where said
    void AddTemporaryActor(C_actor *act);
    void InternTempActAdd(C_actor *act);
    // The traffic generators' cars shown or hidden
    void SetTrafficVisible(bool visible);
    // An explosion of the actor at the position (EXPLOSION's)
    void NewExplosion(C_actor *actor, S_vector &pos, float unknown1, float unknown2, bool unknown3, bool unknown4,
        bool unknown5, int unknown6);
    // A sound of the sound cache played at the frame (the vector by value, PS2's by reference): its number, which
    // Stop3DSound takes, -1 if it fails
    int Play3DSound(I3D_frame *frm, const char *name, I3D_SOUNDTYPE type, S_vector pos, float near_range,
        float far_range, float volume, bool unknown1, float unknown2, bool unknown3);
    void Stop3DSound(int sound, bool fade, float time);
    // A shadow for the frame, type2 one of the shadow models (shadow_temp), type1 choosing the list: its number, 0 if
    // it fails (the human keeps it)
    int AddFrameForShadow(I3D_frame *frm, E_shadow_type type1, E_shadow_type type2, I3D_frame *unknown, char *name);
    void DelFrameForShadow(I3D_frame *frm, E_shadow_type type);
    // The city's music (the IShow of unknown_2fb8) faded out and forgotten, or paused
    void CityMusicOff();
    void CityMusicPaused(bool paused);
    void SetFilmMusic(int music);
    // The film music of unknown_2fc4's index, -1 for none
    int GetFilmMusic(int index);
    // The streams of unknown_2fc4 destroyed, from the last (the name is ours: RECUNLOAD)
    void StreamDestroyAll();
    // ClearDiffData, then the chunk file diff\<name> (the name is ours, after ClearDiffData; PS2's LOADDIFFERENCES does
    // nothing)
    void LoadDiffData(const char *name);
    // The frame's state kept in frame_placements, which Done puts back
    void AddFramePlacement(I3D_frame *frm);
    // SETNPCKILLEVENT's: the script, its value and the name
    void AddNPCKillEvent(C_program *prog, int unknown, char *label);
    void ClearProjectorsForFrame(I3D_frame *frm);
    // What the player keeps between missions (inventory_stack, float_stack, character_stack)
    void InventoryStackPush(C_human *human);
    void InventoryStackPop(C_human *human);
    void FloatStackPush(float f);
    void FloatStackPop(float *f);
    void CharacterStackPush(C_human *human);
    void CharacterStackPop(C_human *human);
    void BoxPlacementAdd(I3D_frame *frm, int unknown);
    void BoxPlacementRecompile();
    void BoxPlacementDestAdd(I3D_frame *frm, int unknown);
    void BoxPlacement_DelInfo(I3D_frame *frm);
    int BoxPlacement_GetBox(int index);
    int BoxPlacement_GetBoxDest(int index);
    void BoxPlacement_MoveBoxToDest(int index);
    void BoxRecompileBoxForCar(C_car *car, bool unknown);
    int BoxPlacementPoPiciNaZkurvenemAute(C_car *car, int unknown);
    void DeathActorAdd(C_actor *act);
    void UseLightCacheSystem();
    void RecomputeLightCache();
    // The videos and the music the scripts play (i_shows): a new one's id, which the others take, -1 if it fails
    int CreateIShow(uint flags);
    // Opens the file in the stream of the id: false if it doesn't
    bool OpenIShow(const char *name, int id);
    void DestroyIShow(int show);
    // The stream of the id, NULL for none
    IShow *GetIShow(int id);
    void IShowStoreVolume(int show, float volume);
    void IShowLoop(int show, bool loop);
    void IShowFadeDownAll(int time);
    void IShowFade(int id, float from, float to, int time);
    bool IsValidTaxiPassenger();
    // Whether the frame is one of the telephones
    bool IsTelephoneExist(I3D_frame *frm);
    void AddHumanTalk(C_human *human, int chanel);
    void DelHumanTalk(C_human *human, int chanel);
    void DeathHumanTalk(C_human *human);
    // Adds the actor to invisible_actors unless it is there
    void AddInvisibleActor(C_actor *act);
    // A particle the cutscene's EMITPARTICLE emitted, which DestroyUnknown2f8cParticles destroys (PC only, the names
    // are ours: unknown_2f8c)
    void AddCutsceneParticle(int particle);
    void DestroyUnknown2f8cParticles();
    // A frame the scripts created (unknown_20, which Done unlinks and releases), and taken from it: false if it isn't
    // there
    void AddCreatedFrameToVector(I3D_frame *frm);
    bool DelCreatedFrameToVector(I3D_frame *frm);
    // The score, which the HUD's bonus counter shows while it is on
    void ScoreSet(int value);
    void SetCameraRotRepair();
    // The cartridges' frames unlinked
    void ClearCartridges();
    // Every global actor of ACTOR_PLAYER or ACTOR_ENTITY set up for a recording (PC only, the name is ours: RECLOAD's
    // and RECLOADFULL's)
    void RecPrepareHumans();

    // The name is ours. C_mission::Open sets both through one load of its game, so through a method: written out, VC6
    // reloads the pointer after the first store.
    void ResetUnknown363c()
    {
        unknown_363c = true;
        unknown_363d = false;
    }
    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours)
    // FUNCTION: GAME 0x47b510
    G_Camera &GetCamera()
    {
        return camera;
    }
    // FUNCTION: GAME 0x47b520
    C_human *GetPlayer() const
    {
        return player;
    }
    // FUNCTION: GAME 0x47b530
    C_using_object &GetUsingObject()
    {
        return using_object;
    }
    // CLEARDIFFERENCES' (PS2's does nothing)
    // FUNCTION: GAME 0x47b540
    void SetUnknown2ad9()
    {
        unknown_2ad9 = true;
    }
    // FUNCTION: GAME 0x47b550
    C_sledovacka *GetSledovacka()
    {
        return &sledovacka;
    }
    // FUNCTION: GAME 0x47b560
    int GetGameTime() const
    {
        return game_time;
    }
    // FUNCTION: GAME 0x47b570
    uint GetUnknown2b14() const
    {
        return unknown_2b14;
    }
    // FUNCTION: GAME 0x47b580
    soundArena &GetSoundArena()
    {
        return sound_arena;
    }
    // FUNCTION: GAME 0x47b590
    void SetNightMission(bool night)
    {
        night_mission = night;
    }
    // FUNCTION: GAME 0x47b5a0
    void SetTimeOutEvent(C_program *prog, int event)
    {
        timeout_program = prog;
        timeout_event = event;
    }
    // FUNCTION: GAME 0x47b5c0
    void SetPlayerFireEvent(C_program *prog, int event)
    {
        player_fire_program = prog;
        player_fire_event = event;
    }
    // FUNCTION: GAME 0x47b5e0
    void SetPlayerHornEvent(C_program *prog, int event)
    {
        player_horn_program = prog;
        player_horn_event = event;
    }
    // FUNCTION: GAME 0x47b600
    void SetPlayerWFallEvent(C_program *prog, int event)
    {
        player_wfall_program = prog;
        player_wfall_event = event;
    }
    // FUNCTION: GAME 0x47b620
    C_shvestky *GetShvestky()
    {
        return &shvestky;
    }
    // FUNCTION: GAME 0x47b630
    void SetUnknown48(int value)
    {
        unknown_48 = value;
    }
    // FUNCTION: GAME 0x47b640
    void SetUnknown2edc(bool start)
    {
        unknown_2edc = start ? RACE6_STATE_STARTED : RACE6_STATE_NONE;
    }
    // FUNCTION: GAME 0x47b660
    int GetUnknown2edc() const
    {
        return unknown_2edc;
    }
    // FUNCTION: GAME 0x47b670
    void SetUnknown2ee0(float *f)
    {
        unknown_2ee0 = f;
    }
    // FUNCTION: GAME 0x47b680
    void SetActorLightness(bool on)
    {
        actor_lightness = on;
    }
    // FUNCTION: GAME 0x47b690
    void SetAnimPreload(bool on)
    {
        anim_preload = on;
    }
    // FUNCTION: GAME 0x47b6a0
    void SetUnknown3638(int value)
    {
        unknown_3638 = value;
    }
    // FUNCTION: GAME 0x47b6b0
    int GetScore() const
    {
        return score;
    }
    // FUNCTION: GAME 0x47b6c0
    void SetUnknown3665(bool value)
    {
        unknown_3665 = value;
    }

    // Init's local struct (PS2's S_hlp, whose cbEnumSectors is PS2's; the other name is ours)
    struct S_hlp
    {
        // The sectors' lights for the vehicles, through the hierarchy
        static void cbEnumSectors(I3D_frame *frm);
        // An animation's .tck
        static void PreloadAnim(const char *name);
        // The frame enumerations' callbacks of the Load functions (PS2 names but cbFindSemaphores's)
        static I3DENUMRET __stdcall cbFindSemaphores(I3D_frame *frm, uint user);
        static I3DENUMRET __stdcall cbCityMusic(I3D_frame *frm, uint user);
        static I3DENUMRET __stdcall cbRoadBlock(I3D_frame *frm, uint user);
        static I3DENUMRET __stdcall cbFindTaxiPoints(I3D_frame *frm, uint user);
        static I3DENUMRET __stdcall cbTelephones(I3D_frame *frm, uint user);
        static I3DENUMRET __stdcall cbScreamArea(I3D_frame *frm, uint user);
        // The frame's placements and its children's taken from the list (PS2 name; PS2's is a gif::vector)
        static void cbFreeFramePlacement(I3D_frame *frm, std::vector<S_frame_placement> &list);
        // The same for InvalidateFrame (PS2 name), which walks only the children that have children of their own
        static void cbDelFrame(I3D_frame *frm, std::vector<S_frame_placement> &list);
    };
};

// The road blocks' destructors, which the constructor's unwinding and the destructor call: the original kept the copies
// of the range at 0x401030 (docs/game-files.md) of their folded groups
// TEMPLATE: GAME 0x4036f0
// ??1?$vector@US_RoadBlock@C_game@@V?$allocator@US_RoadBlock@C_game@@@std@@@std@@QAE@XZ

// SYNTHETIC: GAME 0x402090
// ??1S_RoadBlock@C_game@@QAE@XZ
