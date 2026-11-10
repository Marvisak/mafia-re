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
class C_grenade;
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
    // 0x0, the semaphore's model, which DelSemaphore looks for: LoadSemaphores leaves it (PS2's too), so it holds
    // whatever the stack had (the name is ours)
    I3D_frame *model;
    I3D_frame *red;                     // 0x4
    I3D_frame *orange;                  // 0x8
    I3D_frame *green;                   // 0xc
    int *phase;                         // 0x10, C_game's semaphore_phase_x or _z, by the semaphore's direction
};

// The shadows' types, which AddFrameForShadow and DelFrameForShadow take (PS2's E_shadow_type, a global enum; the
// enumerator's name is ours, by value): FRM_SETON passes 0
enum E_shadow_type { SHADOW_TYPE_0 };

// C_game::race6_state, RACING_MISSION6_START's states (the names are ours): the command starts the race at
// RACE6_STATE_NONE and goes on at RACE6_STATE_END
#define RACE6_STATE_NONE 0
#define RACE6_STATE_RUNNING 1           // Tick's, after it has run the race
#define RACE6_STATE_STARTED 2           // SetRace6State(true)
#define RACE6_STATE_END 10

// The bit of C_game::cheats that turns on DoubleKiss, the guns on the player's car (the name is ours): no code of the
// PC's sets it, CheatKey's toggles have no case for it
#define GAMEF_DOUBLEKISS 0x8
// The bit of C_game::cheats that keeps the player and the humans of HUMAN_TYPE_FRIEND from hits (C_human::Hit;
// the name is a guess): no cheat word toggles it
#define GAMEF_INVULNERABLE 0x1
// The bit of C_game::cheats with which a reload takes no ammunition from the reserve (C_human::AI; the name is a
// guess): MUNICAK (Czech munice, ammunition) toggles it
#define GAMEF_INFINITE_AMMO 0x100
// The bit of C_game::cheats with which the Magnum fires grenades (C_human::Shooting; the name is ours): no cheat word
// toggles it
#define GAMEF_GRENADE_GUN 0x4
// The bit of C_game::cheats with which the city's blocks are active farther from the camera (C_cache_base_block::Tick;
// the name is ours): CUMDAL toggles it
#define GAMEF_FAR_BLOCKS 0x200
// The bit of C_game::cheats with which the player steals and hijacks any car, those the garage's profile can't steal
// too (C_player's Can_UseCar and Do_AB_OwnerNULL; the name is ours): CORNUFSE toggles it
#define GAMEF_STEAL_ALL 0x20
// The bit of C_game::cheats with which the player's hands are twice their size (C_player::SetBoxer; the name is
// ours): BOXER toggles it
#define GAMEF_BOXER 0x10
// The bit of C_game::cheats with which Leonardo and Kate stand on the bridges' boat (C_bridge::SetTitanic; the
// name is ours): PARNIK toggles it
#define GAMEF_TITANIC 0x80

// A shot in flight (PS2 S_shoot, PS2's 0x70 bytes; the members' names are ours), which C_game::NewShoot (0x5a84a0)
// adds and TickShoot moves until it hits something or reaches the end of its range
struct S_shoot
{
    S_vector pos;                       // 0x0, where it was shot from
    S_vector dir;                       // 0xc, as far as the level's collision it hits, else its whole range
    float range;                        // 0x18, dir's length
    S_vector curr_pos;                  // 0x1c, where it is now
    float power;                        // 0x28
    C_actor *actor;                     // 0x2c, who shot it
    I3D_frame *frame;                   // 0x30, coll's frame where it is glass or a crash collision, else NULL
    g_collision_header *coll;           // 0x34, the level's collision the line hits, NULL for none
    S_vector hit_pos;                   // 0x38, where it hits coll
    S_vector hit_normal;                // 0x44
    bool hit;                           // 0x50, coll isn't NULL
    int sound;                          // 0x54, played where the shot ends (the material's, or SHOTSOUND_*), -1 for none
    uchar material;                     // 0x58, coll's
};

// The sounds of a shot's hit on an actor (S_shoot::sound; the names are ours): materials.def's "Telo" (body) has the
// first
#define SHOTSOUND_BODY 0x19
#define SHOTSOUND_CAR 0x1a
// The effects' particle of a shot's hit on a human or an animal (materials.def's "Telo" has it too; the name is ours)
#define SHOTPARTICLE_BODY 0
// The effects' particles of C_game::NewExplosion (with big: a car's, the code's at 0x59f920) and of NewSmallFires'
// fires, and the sound table's sound of NewFire (the names are ours)
#define PARTICLE_EXPLOSION_BIG 0x14
#define PARTICLE_EXPLOSION 7
#define PARTICLE_SMALL_FIRE 0x16
#define SOUND_FIRE 0x15
// The effects' particle of the fire a grenade sets, C_grenade::AI's (the name is ours)
#define PARTICLE_GRENADE_FIRE 8
// The sound table's sound of an explosion: the script's EXPLOSION and the seagulls' pass it to NewExplosion (the
// name is ours)
#define SOUND_EXPLOSION 0x12
// C_game::track_textures' index of a bloody footprint, "2bfootprint.bmp", which C_human::cbCollTester gives the
// steps of a human through blood (the name is ours, after the file)
#define TRACK_TEXTURE_BFOOTPRINT 6
// C_game::track_textures' index of the cars' scratches, "skrob.tga" (ScratchAdd's; the name is ours)
#define TRACK_TEXTURE_SCRATCH 9

// The game's record in a saved game (C_game::SaveGameSave's; the names are ours), 0x43 bytes, packed. PS2's has the same
// fields as ints (version 11, 0x48 bytes). The chases' record follows it, then with full the videos' (each an
// S_i_show_savegame and its name) and their fades, then the stacks' items and the using objects' record.
#pragma pack(push, 1)
struct S_game_savegame
{
    uchar version;                      // 0x0, GAME_SAVEGAME_VERSION
    int game_time;                      // 0x1
    int game_frames;                    // 0x5
    int mission_number;                 // 0x9, C_mission's, which SaveGameLoadBefore takes back
    bool timer;                         // 0xd, the HUD shows the clock (INDF_TIMER)
    uint timer_interval;                // 0xe, G_IndicatorsClass::TimerGetInterval's
    uint timer_hour;                    // 0x12, TimerGetTime's
    uint timer_minute;                  // 0x16
    uint timer_second;                  // 0x1a
    int num_inventories;                // 0x1e, inventory_stack's
    int num_characters;                 // 0x22, character_stack's
    int num_floats;                     // 0x26, float_stack's
    int num_i_shows;                    // 0x2a, the videos saved, all but the city's music; 0 without full
    int num_i_show_fades;               // 0x2e, 0 without full
    int next_i_show_id;                 // 0x32, 0 without full
    int city_music;                     // 0x36, -1 without full
    int city_music_old;                 // 0x3a, -1 without full
    bool score_on;                      // 0x3e
    int score;                          // 0x3f
};
// A video's record (the names are ours), 0x19 bytes, packed: its file's name follows it, name_len characters
struct S_i_show_savegame
{
    int id;                             // 0x0
    uint flags;                         // 0x4, ISHOWF_*
    uint name_len;                      // 0x8, the name's length without its 0, 0 for none
    bool playing;                       // 0xc, the video had no event (IShow::GetEvent's 0): it plays again from pos
    uint pos;                           // 0xd, IShow::GetCurrentPos's
    int unknown_11;                     // 0x11, 0
    float volume;                       // 0x15, its volume relative to the music's, at most 1 (0 with the music off)
};
#pragma pack(pop)

// S_game_savegame::version, the only one SaveGameLoad and SaveGameLoadBefore take (the name is ours)
#define GAME_SAVEGAME_VERSION 10

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
    // Where a box was when it was taken (BoxPlacement_SaveInfo's), which BoxPlacement_RestoreInfo puts it back to (the
    // members' names are ours)
    struct S_box_placement_temp
    {
        I3D_frame *frame;               // 0x0
        I3D_frame *parent;              // 0x4
        S_vector pos;                   // 0x8, the local position
        S_vector dir;                   // 0x14, the local direction
    };
    // An actor Tick wakes (ACTSTATE_ACTIVE to ACTSTATE_INACTIVE) or puts back to ACTSTATE_ACTIVE once it is seen, at
    // most every sixth frame (the members' names are ours)
    struct S_invisible_actor
    {
        C_actor *actor;                 // 0x0
        uint frame;                     // 0x4, game_frames when Tick last changed its state
    };
    // Its destructor's copy (0x402090) destroys the vector
    struct S_RoadBlock
    {
        ushort node;                    // 0x0, the roads' node nearest to the road block's dummy (the name is ours)
        // 0x2, the road block is up: C_shvestky::Tick sets it as a police car comes near and creates the block,
        // and clears it once none is near; C_actor::IsPossibleCollAll tests moves against those up (the name is
        // ours)
        bool active;
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
        I3D_frame *replaced_frame;      // 0x8, turned off while the shadow is on
        bool on;                        // 0xc
        char *name;                     // 0x10, the shadow model's file name, or NULL for the model at model_index
        uint model_index;               // 0x14, an index into the shadow models (shadow_temp)
        float dist;                     // 0x18, from the camera, which the tick sorts them by
    };
    // A hydrant's water (the members' names are ours): TickHydrant stops the particle while the line up from it hits
    // something but an actor other than a car, and destroys it 150 m from the camera
    // AddHydrant's effects' particle and sound table's sound (the names are ours)
#define PARTICLE_HYDRANT 0x57
#define SOUND_HYDRANT 0x147
    struct S_hydrant
    {
        int particle;                   // 0x0, the effects' particle (C_effects::DestroyParticle's)
        bool blocked;                   // 0x4, the particle stopped (C_effects::PauseParticle's false)
        S_vector pos;                   // 0x8
        I3D_frame *frame;               // 0x14
    };
    // A sound PauseAllSounds turned off, which PlayAllSounds turns on again as it was (PS2 name, PS2's 0x24 bytes with
    // the sound's name; the members' names are ours)
    struct S_stop_sounds
    {
        I3D_sound *sound;               // 0x0
        float volume;                   // 0x4
        float volume_out;               // 0x8
        bool loop;                      // 0xc
        uint time;                      // 0x10, its position (GetCurrTime's)
    };
    // A sound the game plays, released once it has stopped (the members' names are ours)
    struct S_sound_play
    {
        I3D_sound *sound;               // 0x0
        int number;                     // 0x4, next_sound_number's, which Play3DSound returns
        float volume;                   // 0x8, the volume a fade in goes to
        float fade;                     // 0xc, the volume's change per ms, in while positive, out to silence otherwise
        bool fading;                    // 0x10, counted by sounds_fading
        uint time;                      // 0x14, the sound's GetCurrTime at the last tick, if it doesn't loop
    };
    // A script's event fired when an NPC of that name is killed (SETNPCKILLEVENT's; the members' names are ours)
    struct S_npc_kill_event
    {
        C_program *program;             // 0x0
        int event;                      // 0x4
        char name[0x20];                // 0x8, the NPC's frame's name, compared without case
    };
    // A driver's model of a car (PS2 AddDriverMan's; the members' names are ours)
    struct S_driver_man
    {
        I3D_model *model;               // 0x0
        C_car *car;                     // 0x4, whose steering the animation's time follows (TickDriverMan)
        uint anim;                      // 0x8, the animation Done stops
        bool formula;                   // 0xc, FormuleVolant.I3D's animation, else AutoRidicVolant.I3D's
    };
    // A video playing in the mission (the members' names are ours)
#define ISHOWF_LOOP 0x1                 // Tick starts it again at its end (IShowLoop's; the name is ours)
// The city's music (TickCityMusic's), which the saved games leave out (the name is ours)
#define ISHOWF_CITY_MUSIC 0x2
    struct S_i_show
    {
        IShow *show;                    // 0x0
        int id;                         // 0x4, the id the IShow functions take
        uint flags;                     // 0x8, ISHOWF_*
        char *name;                     // 0xc, the file's name, which DestroyIShowAll deletes
        float volume;                   // 0x10, its own, which the music's scales (PS2's IShowStoreVolume stores it)
    };
    // A fade of a video's volume (IShowFade's; the members' names are ours)
    struct S_i_show_fade
    {
        int id;                         // 0x0
        float from;                     // 0x4
        float to;                       // 0x8
        short time;                     // 0xc, in ms, signed (Tick compares and converts them so)
        short length;                   // 0xe
    };
    // A human speaking on a dubbing channel (AddHumanTalk's; the members' names are ours)
    struct S_enemy_talk
    {
        C_human *human;                 // 0x0
        int chanel;                     // 0x4, the dubbing's (g_dabing.h)
    };
    // A script's actor register waiting for the actor of an id, which FindActorsVect fills through
    // C_program::FindActorLater (the members' names are ours). PS2 has no insert of the type: nothing adds them there.
    struct S_find_actor
    {
        C_program *program;             // 0x0
        int reg;                        // 0x4
        uint id;                        // 0x8, the actor's (GetActorFromGUID's)
    };

    // The types PS2 doesn't name (the names are ours, but where said; those by an offset are the member's)
    // The lightness of an actor's frame (LightnessAdd's; PS2's Lightness functions', empty there; the members' names are
    // ours), 0x24 bytes: LightnessTick measures the scene's lightness at the actor's frame, offset by pos, and moves the
    // frame's lightness (its visual's, or its subtree's visuals') towards it
    struct S_lightness
    {
        C_actor *actor;                 // 0x0
        I3D_frame *frame;               // 0x4
        ushort flags;                   // 0x8, LIGHTNESSF_*
        S_vector pos;                   // 0xc, from the actor's frame's world position
        float lightness;                // 0x18, the frame's, 1 at first
        float target;                   // 0x1c, the scene's lightness at pos, 1 at first
        uint time;                      // 0x20, the game's game_time when target was measured, 0 at first
    };
    // S_lightness::flags (the name is ours): the lightness goes to the visuals of the frame's subtree (S_hlp::cbLightness),
    // whatever the frame is, and isn't measured for an actor
#define LIGHTNESSF_SUBTREE 0x1
    // A box the scripts placed (FUCKINGBOX_ADD, BoxPlacementAdd's; PS2's BoxPlacement functions'), 0x98 bytes, PS2's
    // 0xc0 (the members' names are ours)
    struct S_box_placement
    {
        I3D_frame *frame;               // 0x0
        // 0x4, its "Pick up box" (GAMEITEM_SPECIAL_ACTION, USING_ACTION_PICK_UP_BOX), which BoxPlacementRecompile adds
        // while the box can be picked up
        C_using_object::S_using_object *using_object;
        // 0x8, DYNCOLL_UNKNOWN_40, of the frame's bound volume: in the collisions unless the box lies on an actor
        tDynamicCollObject coll;
        S_matrix inv_matrix;            // 0x50, the inverse of the frame's world matrix, coll's
#define BOXID_NONE 121212               // S_box_placement::id: a box put down, which goes to no destination (the name is ours)
        int id;                         // 0x90, the id of its destinations (S_box_dest::id), the scripts' register
        // 0x94, the box can be picked up: cleared while it lies on a car whose special dynamic collisions are off
        bool on;
    };
    // A destination of the boxes (FUCKINGBOX_ADD_DEST, BoxPlacementDestAdd's; the names are ours)
    struct S_box_dest
    {
        I3D_frame *frame;               // 0x0
        int id;                         // 0x4, the boxes' (S_box_placement::id)
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
    // A projector of the tracks CreateTrackEffect leaves (the blood, the holes' marks; the name is ours), 0x58 bytes,
    // of which only the collision object is cleared, by its constructor. PS2's is 0x80, with an I3D_bbox, and clears
    // 0x10 to 0x70. The empty constructor keeps C_game's destructor as it is: with the implicit one VC6 inlines less in it.
    struct S_track_projector
    {
        I3D_frame *frame;               // 0x0
        tDynamicCollObject coll;        // 0x4
        bool in_collision;              // 0x4c, whether coll is among gifCollision's dynamic objects
        bool growing;                   // 0x4d, Tick scales the projector up to scale 1, then clears it
        uchar unknown_4e[2];            // 0x4e
        float scale;                    // 0x50, of the size, 0.1 to 1 while growing
        // 0x54, the frame the track is on (CreateTrackEffect's frm, a shot's glass or crash collision's), whose tracks
        // ClearProjectorsForFrame turns off (the name is ours)
        I3D_frame *hit_frame;

        S_track_projector() {}
    };
    // A shot's light (after PS2's AddShootLight and ShootLightTick; the members' names are ours)
    struct S_shoot_light
    {
        I3D_light *light;               // 0x0
        int time;                       // 0x4, -1 from Init: the time left, after which the light goes off (free at 0 or less)
#define SHOOTLIGHTF_FLICKER 0x1         // S_shoot_light::flags: the light flickers, and fades in its last 500 ms (ours)
        ushort flags;                   // 0x8, SHOOTLIGHTF_*
    };
    // A sound turned on once its time is up (the names are ours): Init gives each of the three sound 0xbf
    struct S_delayed_sound
    {
        I3D_sound *sound;               // 0x0
        int time;                       // 0x4, -1 from Init: the time left until the sound is turned on
    };
    // A scratch on a car, a projector of skrob.tga (after PS2's Scratch functions, ScratchAdd's and
    // ScratchDeleteFromCar's, 0x5b8780 and 0x5b8c20; the names are ours)
    struct S_scratch
    {
        I3D_frame *frame;               // 0x0, the projector
        C_car *car;                     // 0x4, whose scratches ScratchDeleteFromCar turns off
        I3D_frame *car_frame;           // 0x8, the car's frame the projector projects on (ScratchRecompile's)
        // 0xc, the hits near it since it was taken (ScratchGetFreeIdx's), up to five of which make it more opaque
        int hits;
    };
    // An area where the enemies scream (PS2 name; enm_scrm_..., the members' names are ours)
    struct S_scream_area
    {
        I3D_area *area;                 // 0x0
        ushort id;                      // 0x4, the hexadecimal number in the name's parentheses
    };
    // An item's blur (ItemBlurTick's; PS2's ItemBlur functions', whose list nothing of the PC's or PS2's fills, PS2's
    // elements 0x30 bytes; the names are ours): copies of the item's frame trailing behind it
    struct S_item_blur
    {
        C_actor *actor;                 // 0x0, whose frame's movement the blur follows (InvalidateActor's)
        I3D_frame *frame;               // 0x4, whose position and rotation the copies take
        I3D_frame **frames;             // 0x8, the copies and their array, which ItemBlurDelItem releases and deletes
        int num_frames;                 // 0xc
        S_vector last_pos;              // 0x10, the actor's frame's position in the camera's space at the last tick
        float speed;                    // 0x1c, its speed there, per second, averaged over the ticks
        bool on;                        // 0x20, the copies are placed (ItemBlurTick unlinks them as it clears it)
        float spread;                   // 0x24, the most the copies spread over
    };

    std::vector<S_frame_placement> frame_placements;  // 0x0 (PS2 0x0, a gif::vector; the name is ours)
    // The frames turned off for the game, which Done turns on again, and the frames the scripts created
    // (AddCreatedFrameToVector's, PS2 name), which Done unlinks and releases (the names are ours)
    std::vector<I3D_frame *> hidden_frames;  // 0x10 (PS2 0xc)
    std::vector<I3D_frame *> created_frames;  // 0x20 (PS2 0x18)
    std::vector<S_box_placement_temp> box_placement_temp;  // 0x30 (PS2 0x24)
    // Init has run, until Done; while set, G_LoadSaveClass::LanguageSelect(-1) keeps the loaded texts (PS2 0x58; the name
    // is ours)
    bool initialized;                   // 0x40
    C_mission *mission;                 // 0x44, the mission that created it (PS2 0x5c)
    // 0x48 (PS2 0x60, which PS2's C_mission::Init clears), how the mission ends (the name is ours): 1 when it ends
    // (ENDOFMISSION's) or the failure menu's exit, 2 its GAMEFAILED_RESULT_4FC, 3 its retry or a saved game loaded
    int mission_end;
    G_Camera camera;                    // 0x4c (PS2 0x70)
    // 0xe4, the player: the mission's last actor of ACTOR_PLAYER (PS2 0x150, which PS2's Init sets; the name is ours)
    C_human *player;
    // 0xe8 (PS2 0x154), the human the camera follows: the player after Init, then SetHuman's (PS2 name, 0x5a07e0, which
    // takes a player or an entity and gives the camera him too; the name is ours)
    C_human *camera_human;
    // The actors Tick ticks, those in ACTSTATE_ACTIVE (PS2's AddNewActorToActive adds to it), the last one first, and
    // those in ACTSTATE_INACTIVE (ACT_SETSTATE INACTIVE), the garage's cars among them: ChangeActState moves them
    // between the two (the names are ours)
    std::vector<C_actor *> active_actors;  // 0xec (PS2 0x158)
    std::vector<C_actor *> inactive_actors;  // 0xfc (PS2 0x164)
    bool act_state_changed;             // 0x10c, an actor's state changed (C_actor::SetActState's; the name is ours)
    // 0x110 (PS2 0x174), the humans TickDeadHumanForTick ticks (PS2's AddDeadHumanForTick adds to it; the name is ours)
    std::vector<uint> dead_humans;
    std::vector<C_actor *> temporary_actors;  // 0x120 (PS2 0x180, its Temporary functions')
    // 0x130 (PS2 0x18c), the temporary actors being removed (RemoveTemporaryActor's; the name is ours)
    std::vector<C_actor *> removed_actors;
    // 0x140 (PS2 0x198, a gif::vector), the actors InternMissionInit carried over from the last mission (the name
    // is ours): Done leaves out the cars in the garage and deletes the others from the mission, and a saved game
    // that isn't full keeps them
    std::vector<C_actor *> carried_actors;
    std::vector<S_lightness *> lightness;  // 0x150 (PS2 0x1a4), LightnessClear deletes them
    int lightness_next;                 // 0x160, the entry LightnessTick (0x5b57e0) checks next (the name is ours)
    bool actor_lightness;               // 0x164, ACTORLIGHTNESSON's (the name is ours)
    // The lights LightCacheTick turns on near the camera (PS2's LightCache functions'), which Done turns on again, and
    // the time left until it checks them again (the names are ours)
    std::vector<I3D_light *> light_cache;  // 0x168 (PS2 0x1b8)
    int light_cache_time;               // 0x178
    std::vector<S_invisible_actor> invisible_actors;  // 0x17c (PS2 0x1c8)
    // The periods of the traffic generators' choice and of the semaphores' phases (the names are ours)
    S_timer traffic_timer;              // 0x18c (PS2 0x1d4)
    S_timer semaphore_timer;            // 0x194 (PS2 0x1dc)
    // 0x19c, the semaphores' phases, of those facing along x and along z, 0 and 5 from Init (no S_timer: the
    // constructor leaves them): indices into game.cpp's semaphore_lights, signed (Tick wraps them so; the names are
    // ours)
    int semaphore_phase_x;
    int semaphore_phase_z;              // 0x1a0
    std::vector<S_semaphore> semaphores;  // 0x1a4 (PS2 0x1ec)
    std::vector<S_RoadBlock> road_blocks;  // 0x1b4 (PS2 0x1f8)
    std::vector<S_Telephone> telephones;  // 0x1c4 (PS2 0x204)
    std::vector<S_shoot> shoots;        // 0x1d4 (PS2 0x210, a gif::vector<S_shoot, 8>), TickShoot's
    // 0x1e4 (PS2 0x21c), the base blocks' model sets of the traffic signs (after PS2's CreateSignalMarkIDX; the name is
    // ours)
    std::vector<ushort> signal_mark_idx;
    std::vector<I3D_frame *> taxi_points;  // 0x1f4 (PS2 0x228), the dummies taxi_point*
    // 0x204, the taxi's passenger: IsValidTaxiPassenger (0x5b6d10) wants him alive in a car, the player's if the player
    // is in one, and InvalidateActor clears it (the name is ours)
    C_human *taxi_passenger;
    C_Break_plain_glass glass;          // 0x208 (PS2 0x240)
    C_Debris debris;                    // 0x240 (PS2 0x280)
    C_compass compass;                  // 0x26c (PS2 0x2b0)
    std::vector<S_shadow_temp> shadow_temp;  // 0x280 (PS2 0x2c4)
    // The humans' shadows and the cars' (TickShadows sorts the first with the player's frame first, the second with
    // his car's), the shadows' tick's period, and which of the two lists it sorts next, the humans' while set (the
    // names but shadow_items are ours)
    std::vector<S_shadow_item> shadow_items;  // 0x290 (PS2 0x2d0)
    std::vector<S_shadow_item> car_shadow_items;  // 0x2a0 (PS2 0x2dc)
    S_timer shadow_timer;               // 0x2b0 (PS2 0x2e8)
    bool human_shadows_next;            // 0x2b8
    C_rmorf rmorf;                      // 0x2bc (PS2 0x2f4)
    std::vector<S_hydrant> hydrants;    // 0x2c4 (PS2 0x2fc, a gif::vector)
    std::vector<S_stop_sounds> stop_sounds;  // 0x2d4 (PS2 0x308, a gif::vector)
    // 0x2e4 (PS2 0x314), the ids of the videos PauseAllSounds paused, which PlayAllSounds plays again (S_i_show::id's,
    // signed: a vector of another type converts each id into a temporary; the name is ours)
    std::vector<int> paused_i_shows;
    std::vector<S_sound_play> sound_play;  // 0x2f4 (PS2 0x320)
    // TickEffects' period while no sound fades, the number Play3DSound (0x5aca70) gives its next sound, and the count
    // of sound_play's sounds fading (the names are ours)
    S_timer sound_play_timer;           // 0x304 (PS2 0x32c)
    int next_sound_number;              // 0x30c
    int sounds_fading;                  // 0x310
    std::vector<C_fire *> fires;        // 0x314 (PS2 0x33c), Done deletes them
    // The actors Init creates for the game from 2stone.i3d (PC only) and the cartridges' models, each with a count
#define MAX_STONES 32                   // stones' size (the name is ours)
    C_stone *stones[MAX_STONES];        // 0x324
    int num_stones;                     // 0x3a4
    C_cartridge *cartridges[100];       // 0x3a8, 2nabojnice.i3d
    int num_cartridges;                 // 0x538
    C_cartridge *cartridges_b[30];      // 0x53c, 2nabojniceb.i3d
    int num_cartridges_b;               // 0x5b4
    I3D_frame *holes[100];              // 0x5b8, copies of the holes' models' dUra_fiRe
    int num_holes;                      // 0x748
    std::vector<I3D_frame *> hole_models;  // 0x74c, a dUra_fiRe of each of the holes' models
    // The tracks' projectors, the textures of the tracks (CreateTrackEffect's index: the blood's, the dirt's and the
    // scratches'), the projector CreateTrackEffect takes next (it wraps at 100), and the count of the projectors Tick
    // grows to their full size (the names are ours)
    S_track_projector track_projectors[100];  // 0x75c (PS2 0x3b0, 20 of them)
    ITexture *track_textures[10];       // 0x29bc
    int track_projector_next;           // 0x29e4
    int growing_projectors;             // 0x29e8
    C_using_object using_object;        // 0x29ec (PS2 0xde0)
    // How fast player_pos moved in the last frame, per second, and where the player or else the camera is (Init's and
    // each Tick's; the names are ours)
    float player_speed;                 // 0x2a0c
    S_vector player_pos;                // 0x2a10
    C_don_ocas *don_ocas;               // 0x2a1c
    // The shots' sounds, played where they end, and their lights (the names are ours)
    I3D_sound *shoot_sounds[10];        // 0x2a20
    S_shoot_light shoot_lights[10];     // 0x2a48
    S_delayed_sound delayed_sounds[3];  // 0x2ac0 (the name is ours)
    // 0x2ad8, the player's failure (C_human sets it, C_car clears it, the racing menu reads it): Tick runs the failure
    // menu once the screen has faded out (the name is ours)
    bool game_failed;
    // 0x2ad9, CLEARDIFFERENCES' (and the racing menu's): Tick clears it and calls ClearDiffData (the name is ours)
    bool clear_diff_data;
    // What the diff data (LoadDiffData's chunk file) added, which ClearDiffData deletes (the names are ours): frames
    // from the scene, actors from the mission, and scripts, the game's own (GlobalProgramRun's)
    std::vector<I3D_frame *> diff_frames;  // 0x2adc (PS2 0xe30)
    std::vector<C_actor *> diff_actors;  // 0x2aec (PS2 0xe3c)
    std::vector<C_program *> diff_programs;  // 0x2afc (PS2 0xe48)
    int game_time;                      // 0x2b0c, the game's time (the name is ours)
    // The frames Tick counts (the names are ours): the game's, cleared by Init, which 0x5b4920 reads and 0x5b4d10
    // writes (PS2's SaveGameSave and SaveGameLoad most likely), and those since Init, which the camera's turns in the
    // first frames and CAMERA_LOCK compare, unsigned (CAMERA_LOCK compares it so)
    int game_frames;                    // 0x2b10
    uint init_frames;                   // 0x2b14
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
    // 0x2e4c (PS2 0x1274), set by Init: the first push onto the stacks after it empties them all first (the name
    // is ours)
    bool stacks_clear;
    uchar unknown_2e4d[3];
    // What InventoryStackPush, CharacterStackPush and FloatStackPush keep of the player between missions (PS2's
    // functions' names; the members' are ours): copies of inventories, a human's properties, floats
    std::vector<G_Inventory *> inventory_stack;  // 0x2e50 (PS2 0x1278)
    std::vector<float> character_stack;  // 0x2e60 (PS2 0x1284)
    std::vector<float> float_stack;     // 0x2e70 (PS2 0x1290)
    // The boxes the scripts place and their destinations (PS2's BoxPlacementAdd, BoxPlacement_GetBox, and
    // BoxPlacementDestAdd, BoxPlacement_GetBoxDest, 0x5b2a00 to 0x5b4547, the names are ours): Done takes the boxes out
    // and deletes the destinations
    std::vector<S_box_placement *> boxes;  // 0x2e80 (PS2 0x129c)
    std::vector<S_box_dest *> box_dests;  // 0x2e90 (PS2 0x12a8)
    // 0x2ea0 (PS2 0x12c0), the "Drop box" (GAMEITEM_SPECIAL_ACTION, USING_ACTION_DROP_BOX) at the destination of the box
    // the player carries (BoxPlacementDestSetOU's and BoxPlacementReposeDestPlayer's; the name is ours)
    C_using_object::S_using_object dest_object;
    bool game_done;                     // 0x2ed8, set by Done, cleared by Init (C_human reads it; the name is ours)
    // RACING_MISSION6_START's race (the names are ours): its state, RACE6_STATE_*, and the float register of the script
    // Tick writes the race's result into
    int race6_state;                    // 0x2edc
    float *race6_result;                // 0x2ee0
    // 0x2ee4 (PS2 0x130c), the actors DeathActorProcess processes (PS2's DeathActorAdd adds to it; the name is ours)
    std::vector<uint> death_actors;
    // The cheats (CheatKey takes the keys typed; the names are ours): the letters typed so far,
    // upper case and complemented as the table of the cheats' words (0x637000: BOXER, PARNIK, JIZDA, KREFJEZIVOT,
    // MUNICAK, POLIFUCK, CORNUFSE, ROZMAZ, CUMDAL), empty from Init, and the bits the words toggle (BOXER 0x10,
    // CORNUFSE 0x20, PARNIK 0x80, MUNICAK 0x100, CUMDAL 0x200), which the game reads with a shift and a test,
    // bitfields most likely
    char cheat_keys[0x20];              // 0x2ef4
    uint cheats;                        // 0x2f14
    std::vector<S_i_show> i_shows;      // 0x2f18 (PS2 0x133c)
    std::vector<S_i_show_fade> i_show_fades;  // 0x2f28 (PS2 0x1348)
    int next_i_show_id;                 // 0x2f38, the id CreateIShow gives next (the name is ours)
    int unknown_2f3c;                   // 0x2f3c
    std::vector<S_enemy_talk> enemy_talks;  // 0x2f40 (PS2 0x135c)
    // DoubleKiss's guns on the player's car (PS2 0x1368 to 0x1390; the names are ours). Nothing of the PC's creates
    // the frames: Init clears kiss_frame and unknown_2f54, Done releases them.
    I3D_frame *kiss_frame;              // 0x2f50, linked to the car, turned to the target
    I3D_frame *unknown_2f54[2];         // 0x2f54
    I3D_frame *kiss_guns[2];            // 0x2f5c, where the shots start
    I3D_frame *kiss_cartridges[2];      // 0x2f64, where the cartridges fly out
    C_car *kiss_car;                    // 0x2f6c, the car kiss_frame is linked to
    C_actor *kiss_target;               // 0x2f70
    int kiss_shoot_time;                // 0x2f74, the time until the guns shoot again
    int kiss_find_time;                 // 0x2f78, the time until the target is aimed at or looked for again
    std::vector<S_find_actor> find_actors;  // 0x2f7c (PS2 0x1394)
    // 0x2f8c (PS2 0x13a0), the effects' particles EMITPARTICLE created while a recording plays (AddCutsceneParticle's),
    // which RECUNLOAD destroys (the name is ours)
    std::vector<int> cutscene_particles;
    bool anim_preload;                  // 0x2f9c, NOANIMPRELOAD clears it (the name is ours)
    std::vector<I3D_frame *> city_music_areas;  // 0x2fa0 (PS2 0x13b0), the areas city_music_*
    // The city's music (TickCityMusic's; the names are ours): the time left until the areas are checked again, the
    // area of the music playing, the IShow ids of that music and of the music fading out (-1 for none), and
    // CityMusicPaused's flag, while which the areas aren't checked
    int city_music_time;                // 0x2fb0
    I3D_frame *city_music_area;         // 0x2fb4
    int city_music;                     // 0x2fb8
    int city_music_old;                 // 0x2fbc
    bool city_music_paused;             // 0x2fc0
    uchar unknown_2fc1[3];              // 0x2fc1
    // 0x2fc4, the film music's streams, IShow ids: GetFilmMusic's index, StreamDestroyAll's (the name is ours)
    std::vector<int> film_music_streams;
    // 0x2fd4, 7000 from Init: the time before the player's death fades to the menu (the name is ours)
    int death_fade_time;
    S_scratch scratches[100];           // 0x2fd8 (the name is ours)
    // The scream areas (PS2's LoadScreamAreas'), the ids of those the camera is in, ORed (1 in none, which C_human
    // reads), and the time left until TickScreamAreas runs again (the names are ours)
    std::vector<S_scream_area> scream_areas;  // 0x3618
    ushort camera_scream_areas;         // 0x3628
    uchar unknown_362a[2];
    int scream_areas_time;              // 0x362c
    // The scene's weather system's WS_PARAM_ON and WS_PARAM_COUNT when Init started, which Done sets again (the names are
    // ours)
    bool weather_on;                    // 0x3630
    int weather_count;                  // 0x3634, signed (OsefujPocasi divides it so)
    // 0x3638, WEATHER_PREPAREBUFFER's count of the weather's particles, which Tick sets the weather to and clears, -1
    // once it has turned it off (the name is ours)
    int weather_buffer;
    // What Init loads and DoneIntern releases once per mission (the textures, projectors, stones, cartridges, holes,
    // the shots' sounds and lights, the sheet and the money; the names are ours): C_mission::Open sets intern_load and
    // clears intern_loaded (PS2 0x1538 and 0x1539 the same), Init loads with the first and swaps them, DoneIntern
    // releases with the second
    bool intern_load;                   // 0x363c
    bool intern_loaded;                 // 0x363d
    bool score_on;                      // 0x363e (PS2 ScoreSetOn's)
    int score;                          // 0x3640
    // SetCameraRotRepair's (PS2 name): the camera's rotation, which Tick sets again in the seventh frame since Init
    // while the flag is set, and clears it (the names are ours)
    bool camera_rot_repair;             // 0x3644
    uchar unknown_3645[3];              // 0x3645
    S_quat camera_rot;                  // 0x3648
    I3D_model *sheet;                   // 0x3658, 9sheet.i3d
    I3D_frame *unknown_365c;            // 0x365c
    I3D_model *money;                   // 0x3660, 9money.i3d
    // 0x3664, set by Init: the cheat KREFJEZIVOT heals the player while it is set, and clears it (CheatKey; the name
    // is ours)
    bool heal_cheat;
    // 0x3665, the mission passed (ENDOFMISSION's with its first value, CHANGE_MISSION's), cleared by Init: the garage's
    // GarageGameDone lets the player choose the cars then (the name is ours)
    bool mission_passed;
    // The cars' sounds (TickCarSoundManager's; the names are ours): on while positive, the time left until the next
    // tick, and which of the two periods runs, 200 ms while set, else 800 ms
    // The value of C_actor::SoundsChange that turns a car's sounds on, any other turns them off (C_car::SoundsChange;
    // the name is ours)
#define SOUNDS_ON 0x7b
    int car_sounds;                     // 0x3668
    int car_sounds_time;                // 0x366c
    bool car_sounds_short;              // 0x3670
    uchar unknown_3671[3];              // 0x3671
    // 0x3674, spare buffers for the shadows' names, of at least SHADOW_NAME_SIZE bytes: DelFrameForShadow adds the name's,
    // AddFrameForShadow takes the last for a name that fits, Done deletes them (the names are ours)
#define SHADOW_NAME_SIZE 32
    std::vector<char *> free_shadow_names;
    bool item_blur;                     // 0x3684, ItemBlurTick runs while it is set (the name is ours)
    uchar unknown_3685[3];              // 0x3685
    std::vector<S_item_blur> item_blurs;  // 0x3688 (PS2 0x156c, its ItemBlur functions'; the name is ours)
    // The inter-city's collisions, frames and their arrays, which InterCity_GameInit creates with the city's base blocks
    // (the names and types are ours)
    tDynamicCollObject **inter_city_colls;  // 0x3698
    S_matrix *inter_city_matrices;      // 0x369c, the collisions' matrices
    S_matrix *inter_city_inv_matrices;  // 0x36a0, and their inverses
    int num_inter_city;                 // 0x36a4, the arrays' count
    I3D_frame **inter_city_frames;      // 0x36a8

    // Inline in the original, as PS2's C_mission::Init shows (it has it inlined): the copy C_mission::Init calls is in
    // Mission.cpp's range. The stores come in the members' order: an initializer list.
    // FUNCTION: GAME 0x5472f0
    C_game() :
        initialized(false)
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
    // The game's data of a saved game (PS2 names; G_LoadSaveClass::SaveGameLoad's): SaveGameLoadBefore takes the
    // mission's number from it before the game's Init, SaveGameLoad the rest after it; false for another version
    bool SaveGameLoad(void **buf);
    bool SaveGameLoadBefore(void **buf);
    // A temporary actor to remove: its state ACTSTATE_NONE, once (PS2 name)
    void RemoveTemporaryActor(C_actor *act);
    // Takes a temporary actor out of the game's lists, unless it is being removed (PS2 name)
    void InternTempActDel(C_actor *act);
    void ClearTemporaryActors();
    // The players take their controls again, from the profile's bindings the options changed (PS2 name): only once the
    // game is initialized
    void UpdateControls();
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
    // PS2 name: each ACTOR_ENTITY of active_actors and inactive_actors, from the last, blocked
    // (C_entity::SedniNaPrdelASledujKino, with the second flag) or woken (VzbudSeUzSouTitulky)
    void EntityBlock(bool block, bool unknown);
    void StopAllSounds();
    void LoadScreamAreas();
    // The weather (PS2 name: Czech for "deal with the weather")
    void OsefujPocasi(bool read, uint param);
    void DelAllHydrants();
    // The items' blurs ended, and one taken away, the last one put in its place with remove (PS2 names)
    void ItemBlurDone();
    void ItemBlurDelItem(int index, bool remove);
    // Takes an actor out of everything that refers to it (PS2 name)
    void InvalidateActor(C_actor *act, bool keep_regs);
    // A frame going away: its placements and its subtree's taken from the list (PS2 name)
    void InvalidateFrame(I3D_frame *frm);
    // The actor whose id is given, the scripts' FindActorLater's (PS2 name)
    C_actor *GetActorFromGUID(uint id);
    // The saved game's record of the game, its size and the record (PS2 names), with full of every actor
    uint SaveGameGetSize(bool full);
    bool SaveGameSave(void **buf, bool full);
    // Runs the game's own script of that name (PS2's is C_mission's): false without one
    bool GlobalProgramRun(const char *name);
    // PS2's C_mission has these two
    void InterCity_GameInit();
    void InterCity_GameDoneInGameDone();
    // Inlined in Init and Tick (PS2's are methods; its TickShadows is empty)
    void TickShadows(uint time);
    void SetActiveTrafficP(uint time);
    // Inlined in Tick (PS2's are methods): the fades of sound_play's sounds, the shots' lights, the lights near the
    // camera turned on, the hydrants, the city's music by the camera's area, the cars' steering wheels
    void TickEffects(uint time);
    void ShootLightTick(uint time);
    void LightCacheTick(uint time);
    void TickHydrant(uint time);
    void TickCityMusic(uint time);
    void TickDriverMan();
    // PS2 names, of what Tick calls
    void FindActorsVect();
    void TickDeadHumanForTick(uint time);
    // Adds a dead human to those TickDeadHumanForTick ticks (PS2 name), which C_human::_intern_ThrowMeFromCar calls
    void AddDeadHumanForTick(C_human *human);
    void DeathActorProcess();
    void _RemoveTemporaryActor(C_actor *act);
    void TickShoot(uint time);
    // A shot from pos along dir, which TickShoot moves (PS2 name; the vectors by value, PS2's by reference; the
    // parameters' names are ours): count shots (a shotgun's), the effects' particle on frm (the muzzle's); true
    bool NewShoot(C_actor *actor, S_vector pos, S_vector dir, float power, int particle, I3D_frame *frm, int count);
    // A cartridge thrown out at the frame, a shotgun's shell with shell, or dropped along dir with drop (the
    // revolver's cylinder emptied; PS2 name; the parameters' names are ours)
    void NewCartidge(I3D_frame *&frm, S_vector &dir, bool shell, bool drop);
    // A track on a surface (a shot's, on the hit level's frame or NULL): the texture of track_textures' index, at pos
    // facing normal, turned by angle; fitted to the surface (fit), with a collision for the steps' blood (collision),
    // growing (grow) (PS2 name and types; the parameters' names are ours): the projector, NULL if there is none
    I3D_frame *CreateTrackEffect(S_vector &pos, S_vector &normal, float angle, S_vector &size, float size_rnd,
        int texture, bool fit, float alpha, bool collision, bool grow, I3D_frame *frm);
    // The same turned by a direction instead of an angle (PS2's other overload): human_notify_callback's steps
    I3D_frame *CreateTrackEffect(S_vector &pos, S_vector &normal, S_vector &dir, S_vector &size, float size_rnd,
        int texture, bool fit, float alpha, bool collision, bool grow, I3D_frame *frm);
    // CreateTrackEffect's helpers (the names are ours): the two axes perpendicular to a normal, the next projector, and
    // the track fitted to the surface (inline, game.cpp)
    void GetTangents(const S_vector &normal, S_vector &u, S_vector &v);
    void NextTrackProjector();
    void FitTrack(S_vector &pos, S_vector &normal);
    // A grenade of the type from pos, the model frm or the type's own, thrown along dir where thrown is set (PS2 name;
    // the parameters' names are ours): the grenade, NULL if it fails
#define NEWGRENADE_GRENADE 0            // the types human_notify_callback's throw gives (the names are ours)
#define NEWGRENADE_MOLOTOV 3
#define NEWGRENADE_SHOT 1               // C_human::Shooting's, the Magnum's with GAMEF_GRENADE_GUN
#define NEWGRENADE_INCENDIARY 2         // a fire after the grenade's fuse, which no caller gives (the name is ours)
    C_grenade *NewGrenade(int type, S_vector &pos, S_vector &dir, bool thrown, I3D_frame *frm);
    // A shot's light at pos (PS2 name; the parameters' names are ours), SHOOTLIGHTF_*: C_human::Shooting's, 7 m for
    // 50 ms; none if the ten are on
    void AddShootLight(S_vector &pos, float range, int time, ushort flags);
    // A shot at a level's collision of control signals (the shot's coll, hit position, start, direction and shooter;
    // PS2 name, PS2's takes the collision as a uint *)
    void DestroyControlSignal(g_collision_header *coll, S_vector &pos, S_vector &from, S_vector &dir, C_human *human);
    void ItemBlurTick(uint time);
    void TickScreamAreas();
    void TickCarSoundManager();
    void LightnessTick(uint time);
    void DoubleKiss(uint time);
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
    // An explosion of the actor at the position (EXPLOSION's, a car's, a crash object's; the parameters' names are
    // ours): with damage the actors, the pedestrians, the glass and the crash collisions within range are hit (power
    // at the centre), big takes the larger particle, smoke throws the smoke trails, sound is the sound table's
    void NewExplosion(C_actor *actor, S_vector &pos, float range, float power, bool damage, bool big, bool smoke,
        int sound);
    // A piece thrown from an explosion at pos, one of the stones, at min_speed to max_speed (PS2 name; the parameters'
    // names are ours): NewExplosion's smoke trails
    void NewSmokeTrail(S_vector &pos, float min_speed, float max_speed);
    // A sound of the sound cache played at the frame (the vector by value, PS2's by reference): its number, which
    // Stop3DSound takes, -1 if it fails. With fade_in it starts silent and fades in by fade a ms (S_sound_play's).
    int Play3DSound(I3D_frame *frm, const char *name, I3D_SOUNDTYPE type, S_vector pos, float near_range,
        float far_range, float volume, bool fade_in, float fade, bool loop);
    // Stops it, fading out by speed a ms with fade, else at once
    void Stop3DSound(int sound, bool fade, float speed);
    // The sound played as it is set up, released once it ends (PS2 name, 0x5accb0): its number, which
    // Stop3DSound takes
    int Play3DSound(I3D_sound *sound);
    // The player fired a shot (PS2 name, 0x5b0680)
    void PFire_Fire();
    // The player sounded the horn: the script's event of it fired (PS2 name, 0x5b06a0)
    void PHorn_Horn();
    // The player fell into a death collision: the script's event of it fired (PS2 name, 0x5b06c0)
    void WFall_Player();
    // A shadow for the frame, type2 one of the shadow models (shadow_temp), type1 choosing the list (the cars' if not
    // 0), replaced_frame the frame it turns off: the shadow (an I3D_shadow *), 0 if it fails (the human keeps it)
    int AddFrameForShadow(I3D_frame *frm, E_shadow_type type1, E_shadow_type type2, I3D_frame *replaced_frame,
        char *name);
    // The frame's shadow taken away: false if it has none
    bool DelFrameForShadow(I3D_frame *frm, E_shadow_type type);
    // The frame's shadow given another shadow model (PS2 name): false if it has none
    bool SetFrameForShadow(I3D_frame *frm, E_shadow_type type1, E_shadow_type type2);
    // A moving collision gone: the entities that keep it forget it (PS2 name, PS2's takes a uint *)
    void InvalidateColl(tDynamicCollObject *coll);
    // The actor's lightness taken at its frame, offset by pos, and forgotten (PS2 names; the parameters' names
    // are ours): C_human::Death has its "base" frame's taken away
    void LightnessAdd(C_actor *act, I3D_frame *frm, ushort flags, S_vector &pos);
    void LightnessDel(I3D_frame *frm);
    // The city's music (the IShow of city_music) faded out and forgotten, or paused
    void CityMusicOff();
    void CityMusicPaused(bool paused);
    void SetFilmMusic(int music);
    // The film music of film_music_streams' index, -1 for none
    int GetFilmMusic(int index);
    // The streams of film_music_streams destroyed, from the last (the name is ours: RECUNLOAD)
    void StreamDestroyAll();
    // ClearDiffData, then the chunk file diff\<name> (the name is ours, after ClearDiffData; PS2's LOADDIFFERENCES does
    // nothing)
    void LoadDiffData(const char *name);
    // The frame's state kept in frame_placements, which Done puts back
    void AddFramePlacement(I3D_frame *frm);
    // SETNPCKILLEVENT's: the script, its value and the name
    void AddNPCKillEvent(C_program *prog, int event, char *name);
    void ClearProjectorsForFrame(I3D_frame *frm);
    // What the player keeps between missions (inventory_stack, float_stack, character_stack)
    void InventoryStackPush(C_human *human);
    void InventoryStackPop(C_human *human);
    void FloatStackPush(float f);
    void FloatStackPop(float *f);
    void CharacterStackPush(C_human *human);
    void CharacterStackPop(C_human *human);
    // PS2 names: what the scripts' FUCKINGBOX_ commands, C_human's and C_player's carrying of the boxes call
    void BoxPlacementAdd(I3D_frame *frm, int id);
    void BoxPlacementRecompile();
    void BoxPlacementDestAdd(I3D_frame *frm, int id);
    void BoxPlacementReposeDestPlayer();
    void BoxPlacement_SaveInfo(I3D_frame *frm);
    // The last destination of the id, its index in box_dests, -1 for none (the name is ours): inline, as
    // BoxPlacementDestSetOU shows, which compares the destinations with its copy of the box's id, not with the
    // id it returns
    int FindBoxDest(int id)
    {
        int i;
        for (i = box_dests.size(); i--; )
            if (id == box_dests[i]->id)
                break;
        return i;
    }
    // The box the player takes (on) or puts down (PS2 name): taken, its index (C_human::box_index)
    int BoxPlacementDestSetOU(bool on, I3D_frame *frm);
    // The destination the human takes the box of the index to (PS2 name)
    I3D_frame *BoxPlacementGetEnemyDest(C_human *human, int id);
    // Where the box was taken from: false if it isn't one of the boxes (PS2 name)
    bool BoxPlacement_GetStartPoint(I3D_frame *frm, S_vector &pos);
    // The box put down at its destination dest (PS2 name)
    void BoxPlacementGetNextDest(I3D_frame *frm, I3D_frame *dest);
    // The box put back where it was taken from (PS2 name)
    void BoxPlacement_RestoreInfo(I3D_frame *frm);
    void BoxPlacement_DelInfo(I3D_frame *frm);
    int BoxPlacement_GetBox(int id);
    int BoxPlacement_GetBoxDest(int id);
    void BoxPlacement_MoveBoxToDest(int id);
    void BoxRecompileBoxForCar(C_car *car, bool on);
    int BoxPlacementPoPiciNaZkurvenemAute(C_car *car, int id);
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
    // A particle the cutscene's EMITPARTICLE emitted, which DestroyCutsceneParticles destroys, RECUNLOAD's (PC only,
    // the names are ours: cutscene_particles)
    void AddCutsceneParticle(int particle);
    void DestroyCutsceneParticles();
    // A frame the scripts created (created_frames, which Done unlinks and releases), and taken from it: false for none,
    // a frame there already, or one that isn't there
    bool AddCreatedFrameToVector(I3D_frame *frm);
    bool DelCreatedFrameToVector(I3D_frame *frm);
    // The score, which the HUD's bonus counter shows while it is on
    void ScoreSet(int value);
    void SetCameraRotRepair();
    // The cartridges' frames unlinked
    void ClearCartridges();
    // Every global actor of ACTOR_PLAYER or ACTOR_ENTITY set up for a recording (PC only, the name is ours: RECLOAD's
    // and RECLOADFULL's)
    void RecPrepareHumans();

    // The file's functions nothing written calls yet (stubs; PS2 names but where said, Unknown_<address> for those
    // whose role isn't settled)
    void SetHuman(C_actor *act);
    bool CheatKey(char key);
    // count small fires around pos (PS2 name), which the code at 0x4452bb calls
    void NewSmallFires(S_vector &pos, int count);
    // A fire at pos in frm's space, burning time ms, that hurts within range by damage if hurts is set (C_fire; PS2
    // name, the parameters' names are ours), which the code at 0x431bdb (a crash object's) and 0x4452bb calls
    void NewFire(S_vector &pos, I3D_frame *frm, int time, float range, float damage, bool hurts);
    void ClearFrameFromFireHole(I3D_frame *frm);
    void DelFramePlacement(I3D_frame *frm);
    void Timer_TimeOut();
    void Do_NPCKillEvent(char *label);
    void AddDriverMan(I3D_model *model, C_car *car, int anim, bool formula);
    void DelDriverMan(I3D_model *model);
    void CreateTrackEffect_Grenade(S_vector &pos);
    void DelSemaphore(I3D_frame *frm);
    void DelTelephones(I3D_frame *frm);
    void PauseAllSounds();
    void PlayAllSounds(bool i_shows_too);
    void ScratchAdd(C_car *car, I3D_frame *frm, uint part_type, const S_vector &pos, const S_vector &force);
    void ScratchRecompile(C_car *car, I3D_frame *frm);
    void ScratchDeleteFromCar(C_car *car);
    int ScratchGetFreeIdx(C_car *car, I3D_frame *frm, const S_vector &pos);
    void DelDeadHumanForTick(C_human *human);
    void AddHydrant(I3D_frame *frm);

    // The name is ours. C_mission::Open sets both through one load of its game, so through a method: written out, VC6
    // reloads the pointer after the first store.
    void ResetIntern()
    {
        intern_load = true;
        intern_loaded = false;
    }
    // A bit of the cheats (0x40, which no cheat word toggles), with which C_shvestky::Tick does nothing (the name is
    // ours): through an accessor, as VC6 reads it with a shift, where the expression written out becomes a test of the
    // byte
    bool GetCheatsBit6() const
    {
        return (cheats >> 6) & 1;
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
    void SetClearDiffData()
    {
        clear_diff_data = true;
    }
    // FUNCTION: GAME 0x47b550
    C_sledovacka *GetSledovacka()
    {
        return &sledovacka;
    }
    // GameLoopSingle's loop test (the name is ours)
    bool IsInitialized() const
    {
        return initialized;
    }
    // FUNCTION: GAME 0x47b560
    int GetGameTime() const
    {
        return game_time;
    }
    // FUNCTION: GAME 0x47b570
    uint GetInitFrames() const
    {
        return init_frames;
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
    void SetMissionEnd(int value)
    {
        mission_end = value;
    }
    // FUNCTION: GAME 0x47b640
    void SetRace6State(bool start)
    {
        race6_state = start ? RACE6_STATE_STARTED : RACE6_STATE_NONE;
    }
    // FUNCTION: GAME 0x47b660
    int GetRace6State() const
    {
        return race6_state;
    }
    // FUNCTION: GAME 0x47b670
    void SetRace6Result(float *f)
    {
        race6_result = f;
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
    void SetWeatherBuffer(int value)
    {
        weather_buffer = value;
    }
    // FUNCTION: GAME 0x47b6b0
    int GetScore() const
    {
        return score;
    }
    // FUNCTION: GAME 0x47b6c0
    void SetMissionPassed(bool value)
    {
        mission_passed = value;
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
        // Stubs: LightnessTick's and PauseAllSounds' walks (the names are ours) and the lights of a subtree (PS2 name)
        static void cbLightness(I3D_frame *frm, float lightness);
        static void cbLight(I3D_frame *frm, std::vector<I3D_light *> &lights);
        static void cbPauseSounds(I3D_frame *frm, std::vector<S_stop_sounds> &list);
        // The frame's placements and its children's taken from the list (PS2 name; PS2's is a gif::vector)
        static void cbFreeFramePlacement(I3D_frame *frm, std::vector<S_frame_placement> &list);
        // The same for InvalidateFrame (PS2 name), which walks only the children that have children of their own
        static void cbDelFrame(I3D_frame *frm, std::vector<S_frame_placement> &list);
    };
};

// The road blocks' destructors, which the constructor's unwinding and the destructor call: the original kept the copies
// of the range at 0x401030 (docs/game-files.md) of their folded groups
// TEMPLATE: GAME 0x4036f0
// ??1?$vector@US_cache_model_set@@V?$allocator@US_cache_model_set@@@std@@@std@@QAE@XZ

// SYNTHETIC: GAME 0x402090
// ??1S_RoadBlock@C_game@@QAE@XZ

// The vectors' functions Tick calls out of line (folded groups: the names are those of the copies our linker keeps)
// TEMPLATE: GAME 0x5470c0
// std::vector<C_game::S_sound_play,std::allocator<C_game::S_sound_play> >::size

// TEMPLATE: GAME 0x5bc050
// std::vector<C_game::S_sound_play,std::allocator<C_game::S_sound_play> >::erase

// TEMPLATE: GAME 0x5bc900
// ?copy@std@@YIPAUS_GameItem@@PAU2@00@Z

// The templates and compiler-generated functions after the file's last function, in the original's order (folded
// groups: the names are those of the copies our linker keeps). The original's 0x5bc8a0 is a std::copy no type of
// ours instantiates yet, and its 0x5bcbc0 S_RoadBlock's scalar deleting destructor, which Done calls with 0 to
// destroy the road blocks, where ours destroys them inline (Done's inline budget)
// TEMPLATE: GAME 0x5bbbf0
// ?size@?$vector@US_frame_placement@C_game@@V?$allocator@US_frame_placement@C_game@@@std@@@std@@QBEIXZ

// TEMPLATE: GAME 0x5bbc20
// ?clear@?$vector@US_item_blur@C_game@@V?$allocator@US_item_blur@C_game@@@std@@@std@@QAEXXZ

// TEMPLATE: GAME 0x5bbc50
// ?back@?$vector@US_itm@?$C_I3D_cache@VI3D_model@@@@V?$allocator@US_itm@?$C_I3D_cache@VI3D_model@@@@@std@@@std@@QAEAAUS_itm@?$C_I3D_cache@VI3D_model@@@@XZ

// TEMPLATE: GAME 0x5bbc60
// ?push_back@?$vector@US_itm@?$C_I3D_cache@VI3D_model@@@@V?$allocator@US_itm@?$C_I3D_cache@VI3D_model@@@@@std@@@std@@QAEXABUS_itm@?$C_I3D_cache@VI3D_model@@@@@Z

// TEMPLATE: GAME 0x5bbde0
// ?erase@?$vector@US_Telephone@C_game@@V?$allocator@US_Telephone@C_game@@@std@@@std@@QAEPAUS_Telephone@C_game@@PAU34@0@Z

// TEMPLATE: GAME 0x5bbe30
// ?insert@?$vector@US_shoot@@V?$allocator@US_shoot@@@std@@@std@@QAEPAUS_shoot@@PAU3@ABU3@@Z

// TEMPLATE: GAME 0x5bc0a0
// ?erase@?$vector@US_i_show@C_game@@V?$allocator@US_i_show@C_game@@@std@@@std@@QAEPAUS_i_show@C_game@@PAU34@0@Z

// TEMPLATE: GAME 0x5bc0f0
// ?insert@?$vector@US_i_show@C_game@@V?$allocator@US_i_show@C_game@@@std@@@std@@QAEXPAUS_i_show@C_game@@IABU34@@Z

// TEMPLATE: GAME 0x5bc3f0
// ?insert@?$vector@US_RoadBlock@C_game@@V?$allocator@US_RoadBlock@C_game@@@std@@@std@@QAEXPAUS_RoadBlock@C_game@@IABU34@@Z

// TEMPLATE: GAME 0x5bc700
// ?_Ucopy@?$vector@US_frame_placement@C_game@@V?$allocator@US_frame_placement@C_game@@@std@@@std@@IAEPAUS_frame_placement@C_game@@PBU34@0PAU34@@Z

// TEMPLATE: GAME 0x5bc740
// ?_Ucopy@?$vector@US_box_placement_temp@C_game@@V?$allocator@US_box_placement_temp@C_game@@@std@@@std@@IAEPAUS_box_placement_temp@C_game@@PBU34@0PAU34@@Z

// TEMPLATE: GAME 0x5bc840
// ?copy@std@@YIPAUS_frame_placement@C_game@@PAU23@00@Z

// TEMPLATE: GAME 0x5bc870
// ?copy@std@@YIPAUS_box_placement_temp@C_game@@PAU23@00@Z

// TEMPLATE: GAME 0x5bc8d0
// ?copy@std@@YIPAUS_npc_kill_event@C_game@@PAU23@00@Z

// TEMPLATE: GAME 0x5bc940
// ?_Construct@std@@YIXPAUS_RoadBlock@C_game@@ABU23@@Z

// TEMPLATE: GAME 0x5bc9e0
// ?_Construct@std@@YIXPAUS_shoot@@ABU2@@Z

// SYNTHETIC: GAME 0x5bca20
// ??4S_RoadBlock@C_game@@QAEAAU01@ABU01@@Z

// SYNTHETIC: GAME 0x5bcbc0
// ??_GS_RoadBlock@C_game@@QAEPAXI@Z
