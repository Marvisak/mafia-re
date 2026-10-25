#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

// The actors' animations (PS2 C_animations_machine_de_luxe, C_animations.cpp). Only what CloseSystem uses so far.
class C_animations_machine_de_luxe
{
public:
    // A loaded track, 0x64 bytes (PS2's, 0x60 bytes, keeps its two lists as arrays; the names are ours), as its
    // implicit destructor and assignment show (0x40e470, 0x4116d0): the name's 32 bytes one by one, the two lists by
    // their own assignments.
    // Open: our assignment comes from Main.cpp, where VC6 calls the first list's empty _Destroy out of line; the
    // original's copy is the animations' file's, which inlines it.
    struct S_track
    {
        char name[32];                  // 0x0
        int unknown_20;                 // 0x20
        int unknown_24;                 // 0x24
        S_vector unknown_28;            // 0x28
        S_vector unknown_34;            // 0x34
        bool unknown_40;                // 0x40
        std::vector<S_vector> unknown_44;  // 0x44
        std::vector<S_vector> unknown_54;  // 0x54
    };

    // The tracks every actor shares
    static std::vector<S_track> tracks;

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
