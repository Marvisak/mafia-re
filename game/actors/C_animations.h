#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

class C_actor;

// The version of the .tck files LoadTrack reads (the name is ours)
#define TCK_VERSION 4

class I3D_animation_set;

// The actors' animations (PS2 C_animations_machine_de_luxe, C_animations.cpp), 0x128 bytes (the traffic's new, C_human
// has one at 0xbc). Only what CloseSystem, C_game and the script use so far.
class C_animations_machine_de_luxe
{
public:
    // Out of line (C_human's constructor and nine others call it)
    C_animations_machine_de_luxe();
    // Empty, out of line (C_traffic_generator::ClearElements calls it before its delete)
    ~C_animations_machine_de_luxe();

    // PS2 names, in PS2's order, the types PS2's but where noted
    void Init(C_actor *act);
    // Clears the machine's state, which the script's ACTOR_SETPLACEMENT calls
    void Reset();
    // The active animation's time and length in ms (the parameters' names are guesses)
    void GetActiveAnimTime(uint &time, uint &length);
    void StopAnims();
    void SetAnim(const char *name, bool b1, bool b2, bool b3, float f1, float f2, bool b4, bool b5, S_vector *v, bool b6,
        uint u);
    bool IsActive();
    // The animations' frame: flags, the end of the animation in bit 1 (C_human::Update tests it)
    uint Tick(uint time);
    void SetExtendedAnim(I3D_animation_set *set, float f1, float f2, bool b);
    void DelExtendedAnim(float f);
    static bool GetPlacementFromTCK(const char *name, S_vector &pos, S_vector &dir);

    // The height a jump lands at, relative to the track's end (the name is ours): PersonAnim's, whose stores address
    // the machine through its own pointer, as an inline method's do (PS2's has the same three stores)
    void SetLandHeight(float height)
    {
        unknown_120 = height;
        unknown_124 = false;
        unknown_125 = (uchar)anim_index;
    }
    // Both set, the human isn't ready (C_human::IsReady; the name is ours): an inline method, whose object a caller
    // that goes on with the machine keeps in a register (C_human::Do_ThrowGrenade)
    bool IsBlocking() const
    {
        return unknown_f4 && unknown_10c;
    }

    uchar unknown_0[0x1c];              // 0x0
    // 0x1c (PS2 0xdc), the active animation of the model, which C_human::Update keeps and C_human::Hit sets the time of
    // (the name is ours)
    uint anim_index;
    uchar unknown_20[0xc];              // 0x20
    int track;                          // 0x2c, the track playing, an index into tracks, -1 for none (the name is ours)
    uchar unknown_30[0x50];             // 0x30
    int unknown_80;                     // 0x80, -1 for none
    uchar unknown_84[0x70];             // 0x84
    // Both set, the human isn't ready (C_human::IsReady)
    bool unknown_f4;                    // 0xf4, cleared by Reset
    uchar unknown_f5[0x17];             // 0xf5
    bool unknown_10c;                   // 0x10c
    uchar unknown_10d[7];               // 0x10d
    S_vector unknown_114;               // 0x114, which C_human::Update moves by what the collisions take off the track's move
    float unknown_120;                  // 0x120
    bool unknown_124;                   // 0x124
    uchar unknown_125;                  // 0x125
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
        S_vector unknown_28;            // 0x28 (PS2 0x0)
        S_vector unknown_34;            // 0x34 (PS2 0x10)
        bool has_directions;            // 0x40 (PS2 0x58)
        std::vector<S_vector> positions;    // 0x44 (PS2 0x20), the keys'
        std::vector<S_vector> directions;   // 0x54 (PS2 0x24), the keys'
    };

    // The tracks every actor shares
    static std::vector<S_track> tracks;
    // How many of them C_game::Init preloaded, which ClearNoBaseTracks keeps (the name is ours)
    static int base_tracks;

    // Loads a track, a .tck file of the anims folder, unless it is loaded: its index, -1 if it doesn't load (PS2 name)
    static int LoadTrack(const char *name);
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

// TEMPLATE: GAME 0x411900
// std::vector<S_vector,std::allocator<S_vector> >::operator=
