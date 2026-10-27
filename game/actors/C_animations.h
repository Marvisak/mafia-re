#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

// The version of the .tck files LoadTrack reads (the name is ours)
#define TCK_VERSION 4

// The actors' animations (PS2 C_animations_machine_de_luxe, C_animations.cpp). Only what CloseSystem and C_game use so
// far.
class C_animations_machine_de_luxe
{
public:
    // Empty, out of line (C_traffic_generator::ClearElements calls it before its delete)
    ~C_animations_machine_de_luxe();

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
        int unknown_20;                 // 0x20 (PS2 0x50), the length in ms most likely: PS2 compares it with 250
        int unknown_24;                 // 0x24 (PS2 0x54), the time between the keys most likely
        S_vector unknown_28;            // 0x28 (PS2 0x0)
        S_vector unknown_34;            // 0x34 (PS2 0x10)
        bool unknown_40;                // 0x40 (PS2 0x58), whether it has the directions
        std::vector<S_vector> unknown_44;  // 0x44 (PS2 0x20), the positions of the keys
        std::vector<S_vector> unknown_54;  // 0x54 (PS2 0x24), the directions of the keys
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
