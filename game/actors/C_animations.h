#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

class C_actor;
class C_traffic_element;
class I3D_model;

// The version of the .tck files LoadTrack reads (the name is ours)
#define TCK_VERSION 4

// Tick's result (the names are ours): the track's last 250 ms, which C_human::Update takes as the animation's end, and
// no model
#define ANIMTICK_END 1
#define ANIMTICK_NO_MODEL 2

class I3D_animation_set;

// The actors' animations (PS2 C_animations_machine_de_luxe, C_animations.cpp), 0x128 bytes (the traffic's new, C_human
// has one at 0xbc). It plays a model's animations in two slots, 0 and 1, the new one in the slot the old one doesn't
// use, blending from one to the other, an extended animation over them in slot 2, and moves the actor along the
// tracks (.tck files) the animations have. The members' names are ours, PS2's offsets 0xc0 higher up to 0x2c (its
// class has no matrices).
class C_animations_machine_de_luxe
{
public:
    // Out of line (C_human's constructor and nine others call it)
    C_animations_machine_de_luxe();
    // Empty, out of line (C_traffic_generator::ClearElements calls it before its delete)
    ~C_animations_machine_de_luxe();

    // PS2 names, in PS2's order, the types PS2's but where noted
    // The machine of an actor, a pedestrian or a model: each sets its model and clears the rest as Reset does
    void Init(C_actor *act);
    void Init(C_traffic_element *element);
    void Init(I3D_model *model);
    // Clears the machine's state, which the script's ACTOR_SETPLACEMENT calls
    void Reset();
    // Sets slot dst's time to the same part of its animation as slot src's is of its (the parameters' names are ours)
    void EqualTime(int src, int dst);
    // The active animation's time and length in ms (the parameters' names are guesses)
    void GetActiveAnimTime(uint &time, uint &length);
    void StopAnims();
    // The animation's matrices moved by the matrix (PS2 name), which C_human::Intern_UseRailway passes the
    // human's move into the wagon or out of it
    void RecompileMatrix(S_matrix &m);
    // Starts an animation in the slot the active one doesn't use, blending to it at speed per second (the
    // parameters' names are ours): stop_blended stops it once it is blended in, equal_time takes the old slot's
    // time over (else start, a part of its length, < 0 for none), once plays it once (else looped), use_track
    // moves the actor along the track of its name (load_track loads it, else only a loaded one), scaled by
    // scale, keep_track goes on with the old track while the blend lasts, id is the caller's anim id
    void SetAnim(const char *name, bool stop_blended, bool equal_time, bool once, float speed, float start,
        bool use_track, bool load_track, S_vector *scale, bool keep_track, uint id);
    // The position of the animation's "blnd" frame at its first key (PS2 name; inline, SetAnim's)
    inline S_vector GetAnimBaseDelta(I3D_animation_set *set);
    bool IsActive();
    // The animations' frame: ANIMTICK_* (C_human::Update tests ANIMTICK_END)
    uint Tick(uint time);
    // The extended animation in slot 2, blended to target at speed per second (at once without a speed), looped or
    // played once (the parameters' names are ours), and its end, fading out at speed
    void SetExtendedAnim(I3D_animation_set *set, float target, float speed, bool once);
    void DelExtendedAnim(float speed);
    static bool GetPlacementFromTCK(const char *name, S_vector &pos, S_vector &dir);
    // The track's position at its end, false where it doesn't load (PS2 name; static: the name in ecx, pos in edx),
    // which C_human's car functions call
    static bool GetEndPointOfTCK(const char *name, S_vector &pos);
    // The track's position at its mixing point, 250 ms before its end (PS2 name; the parameters' names and the
    // result's meaning are ours): false without the track or for a shorter one
    static bool GetMixingPointOfTCK(const char *name, S_vector &pos);
    // PS2 name, which C_human::Movement calls while stage_blending is set
    void StageBlendingTick(uint time);

    // The height a jump lands at, relative to the track's end (the name is ours): PersonAnim's, whose stores address
    // the machine through its own pointer, as an inline method's do (PS2's has the same three stores)
    void SetLandHeight(float height, bool reverse = false)
    {
        land_height = height;
        land_reverse = reverse;
        land_slot = (uchar)anim_index;
    }
    // The height the jump has reached (the name is ours; inline, Tick's)
    inline float GetLandOffset();
    // Both set, the human isn't ready (C_human::IsReady; the name is ours): an inline method, whose object a caller
    // that goes on with the machine keeps in a register (C_human::Do_ThrowGrenade)
    bool IsBlocking() const
    {
        return ext_on && unknown_10c;
    }
    // blend 1 or 0 by blend_speed's sign while stage_blending is set (the name is ours): an inline method, whose
    // object C_human::ZasrateLezeniZAuta_ve2rano keeps in ecx for the GetActiveAnimTime after it (Movement and the
    // note's callback have the same code written out)
    void SetStageBlendingDir()
    {
        if (stage_blending) {
            if (blend_speed > 0.0f)
                blend = 1.0f;
            else
                blend = 0.0f;
        }
    }

    C_actor *actor;                     // 0x0 (PS2 0xc0), Init(C_actor *)'s
    C_traffic_element *element;         // 0x4 (PS2 0xc4), Init(C_traffic_element *)'s
    I3D_model *model;                   // 0x8 (PS2 0xc8), the actor's or the pedestrian's frame
    // The two slots: playing, stopped once the blend to the other slot ends (SetAnim's stop_blended), and played once
    // (I3DANIMOP_ONCE, else looped)
    bool anim0_on;                      // 0xc (PS2 0xcc)
    bool anim0_stop_blended;            // 0xd (PS2 0xcd)
    bool anim0_once;                    // 0xe (PS2 0xce)
    bool anim1_on;                      // 0xf (PS2 0xcf)
    bool anim1_stop_blended;            // 0x10 (PS2 0xd0)
    bool anim1_once;                    // 0x11 (PS2 0xd1)
    bool stage_blending;                // 0x12 (PS2 0xd2), StageBlendingTick runs while it is set
    uchar unknown_13;                   // 0x13
    // Slot 0's blend, slot 1's is 1 - blend, and its change per second, which StageBlendingTick adds up to 1 or down
    // to 0 (C_human::Movement sets it at the end by the speed's sign)
    float blend;                        // 0x14 (PS2 0xd4)
    float blend_speed;                  // 0x18 (PS2 0xd8)
    // 0x1c (PS2 0xdc), the active slot, which C_human::Update keeps and C_human::Hit sets the time of
    uint anim_index;
    // 0x20 (PS2 0xe0), the start, a part of the animation's length, the next SetAnim takes instead of its own, -1 for
    // none (the name is a guess)
    float next_start;
    // The slots' animations, SetAnim's last parameter (C_human's anim ids), -1 for none
    uint anim0_id;                      // 0x24 (PS2 0xe4)
    uint anim1_id;                      // 0x28 (PS2 0xe8)
    int track;                          // 0x2c (PS2 0xec), the track playing, an index into tracks, -1 for none
    // The track's start: the position its keys are relative to, the time into it (unsigned: Tick divides it by the
    // key time with div and compares it with the length with jbe, PS2's divu and sltu) and the model's matrix at its
    // start (without the translation, scaled by SetAnim's vector)
    S_vector track_pos;                 // 0x30
    uint track_time;                    // 0x3c
    S_matrix track_matrix;              // 0x40
    // The track SetAnim left, which Tick goes on with while its blend lasts, -1 for none, and the same of it
    int prev_track;                     // 0x80 (PS2 0xf4)
    float prev_blend;                   // 0x84
    float prev_blend_speed;             // 0x88
    uint prev_track_time;               // 0x8c
    S_vector prev_track_pos;            // 0x90
    S_matrix prev_track_matrix;         // 0x9c
    // The correction from the old track's end to the new one's start, which Tick spreads over the old track's rest
    bool correcting;                    // 0xdc
    float correction_speed;             // 0xe0
    float correction;                   // 0xe4
    S_vector correction_offset;         // 0xe8
    // The extended animation in slot 2 (SetExtendedAnim's): on (with unknown_10c, the human isn't ready), fading in
    // or out, its blend, the blend it fades to, the change per second (negative fading out), played once, and the
    // time it starts fading out at (250 ms before its end, or half of a shorter one)
    bool ext_on;                        // 0xf4 (PS2 0x110), cleared by Reset
    bool ext_fading;                    // 0xf5
    uchar unknown_f6[2];                // 0xf6
    float ext_blend;                    // 0xf8
    float ext_target;                   // 0xfc
    float ext_speed;                    // 0x100
    bool ext_once;                      // 0x104
    uchar unknown_105[3];               // 0x105
    int ext_end;                        // 0x108
    bool unknown_10c;                   // 0x10c, cleared by SetExtendedAnim
    uchar unknown_10d;                  // 0x10d
    // 0x10e (PS2 0x12a), 0xc while the reload's extended animation of the weapons of gun_anim 8 plays (C_human::AI),
    // 0xffff otherwise
    ushort unknown_10e;
    // 0x110 (PS2 0x12c), set by C_human::EineMeineKleineAutoInLuft and _intern_ForceDeath: the machine does nothing
    // while it is set (the name is a guess)
    bool disabled;
    uchar unknown_111[3];               // 0x111
    // 0x114, what C_human::Update moves the track by, the collisions' part of the track's move (SetAnim clears it)
    S_vector track_offset;
    // The jump's landing (SetLandHeight's): the height added to the track's position as the animation of the slot
    // land_slot plays, all of it at its end (240 ms before it), or the other way round, from all of it at its start
    // to none (land_reverse; the name is a guess)
    float land_height;                  // 0x120
    bool land_reverse;                  // 0x124
    uchar land_slot;                    // 0x125
    uchar unknown_126[2];               // 0x126

    // A loaded track, 0x64 bytes (PS2's, 0x60 bytes, keeps its two lists as arrays; the names are ours), as its
    // implicit destructor and assignment show (0x40e470, 0x4116d0): the name's 32 bytes one by one, the two lists by
    // their own assignments.
    // Open: our assignment comes from Main.cpp, where VC6 calls the first list's empty _Destroy out of line; the
    // original's copy is the animations' file's, which inlines it.
    // PS2's members are in another order, its offsets in the comments; their roles come from PS2's GetPlacementFromTCK,
    // GetEndPointOfTCK and GetMixingPointOfTCK.
    struct S_track
    {
        char name[32];                  // 0x0, the file's (PS2 0x30), which LoadTrack finds it by
        // The length in ms and the time between the keys: PS2's GetMixingPointOfTCK takes the key 250 ms before the end
        // of a track longer than 250, the key count less 250 / key_time
        int length;                     // 0x20 (PS2 0x50)
        int key_time;                   // 0x24 (PS2 0x54)
        // Its start and its end: SetAnim's correction goes from the old track's end to the new one's start
        S_vector start_pos;             // 0x28 (PS2 0x0)
        S_vector end_pos;               // 0x34 (PS2 0x10)
        bool has_directions;            // 0x40 (PS2 0x58)
        std::vector<S_vector> positions;    // 0x44 (PS2 0x20), the keys'
        std::vector<S_vector> directions;   // 0x54 (PS2 0x24), the keys'
    };

    // The tracks every actor shares
    static std::vector<S_track> tracks;
    // How many of them C_game::Init preloaded, which ClearNoBaseTracks keeps (PS2 name). Open: the original's .bss has
    // tracks first (0x63d2d8, its destructor's guard byte before it), where VC6's hash puts base_track first
    static int base_track;

    // Loads a track, a .tck file of the anims folder, unless it is loaded: its index, -1 if it doesn't load (PS2 name)
    static int LoadTrack(const char *name);
    // A loaded track's index, -1 if it isn't loaded (PS2 name)
    static int GetTrack(const char *name);
    // Frees the tracks loaded after the base ones (PS2 name)
    static void ClearNoBaseTracks();

    // Inline: CloseSystem clears the tracks itself (PS2 calls its ClearTracks, which frees each track's arrays first)
    static void ClearTracks()
    {
        tracks.clear();
    }
};

// The tracks' compiler-generated functions. The clear that CloseSystem inlines destroys each track through its scalar
// deleting destructor with 0, as VC6 does where its inline budget runs out between the vector's helpers and the
// destructor.
// SYNTHETIC: GAME 0x40e470
// C_animations_machine_de_luxe::S_track::~S_track

// SYNTHETIC: GAME 0x4116d0
// C_animations_machine_de_luxe::S_track::operator=

// SYNTHETIC: GAME 0x5c2520
// C_animations_machine_de_luxe::S_track::`scalar deleting destructor'

// What LoadTrack and ClearNoBaseTracks call out of line, this file's copies: the chunk's stack's constructor, the
// lists' resize and assignment, the tracks' push_back's insert and size
// TEMPLATE: GAME 0x410910
// std::vector<S_vector,std::allocator<S_vector> >::resize

// TEMPLATE: GAME 0x410ba0
// std::vector<C_animations_machine_de_luxe::S_track,std::allocator<C_animations_machine_de_luxe::S_track> >::size

// TEMPLATE: GAME 0x410bd0
// std::vector<int,C_chunk_allocator<int> >::vector<int,C_chunk_allocator<int> >

// TEMPLATE: GAME 0x410c70
// ?insert@?$vector@US_track@C_animations_machine_de_luxe@@V?$allocator@US_track@C_animations_machine_de_luxe@@@std@@@std@@QAEPAUS_track@C_animations_machine_de_luxe@@PAU34@ABU34@@Z

// The tracks' vector's helpers, which its insert calls
// TEMPLATE: GAME 0x410e50
// ?_Destroy@?$vector@US_track@C_animations_machine_de_luxe@@V?$allocator@US_track@C_animations_machine_de_luxe@@@std@@@std@@IAEXPAUS_track@C_animations_machine_de_luxe@@0@Z

// TEMPLATE: GAME 0x411080
// ?_Ucopy@?$vector@US_track@C_animations_machine_de_luxe@@V?$allocator@US_track@C_animations_machine_de_luxe@@@std@@@std@@IAEPAUS_track@C_animations_machine_de_luxe@@PBU34@0PAU34@@Z

// TEMPLATE: GAME 0x4110c0
// ?_Ufill@?$vector@US_track@C_animations_machine_de_luxe@@V?$allocator@US_track@C_animations_machine_de_luxe@@@std@@@std@@IAEXPAUS_track@C_animations_machine_de_luxe@@IABU34@@Z

// TEMPLATE: GAME 0x411120
// ?fill@std@@YIXPAUS_track@C_animations_machine_de_luxe@@0ABU23@@Z

// TEMPLATE: GAME 0x411340
// ?copy_backward@std@@YIPAUS_track@C_animations_machine_de_luxe@@PAU23@00@Z

// TEMPLATE: GAME 0x411550
// ?_Construct@std@@YIXPAUS_track@C_animations_machine_de_luxe@@ABU23@@Z

// TEMPLATE: GAME 0x411900
// std::vector<S_vector,std::allocator<S_vector> >::operator=

// The keys' vectors' assignment's helpers
// TEMPLATE: GAME 0x411b60
// ?capacity@?$vector@US_vector@@V?$allocator@US_vector@@@std@@@std@@QBEIXZ

// TEMPLATE: GAME 0x411b80
// ?copy@std@@YIPAUS_find_actor@C_game@@PAU23@00@Z
