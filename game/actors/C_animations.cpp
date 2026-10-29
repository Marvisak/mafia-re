// The actors' animations (PS2 C_animations.cpp, which has C_animations_machine_de_luxe; the PC's file is most likely
// the one at 0x40df70-0x411bdc, which starts with the tracks' initializer, docs/game-files.md). Written so far: the
// tracks' LoadTrack and ClearNoBaseTracks, which C_game's Init and Done call. Reset, which the script calls, is a stub.
// Open: the tracks' destructor (0x40dfb0, registered by 0x40dfa0) tests and sets a guard byte (0x63d2d4) of its own.

#include "actors/C_animations.h"

#include <stdio.h>
#include <string.h>

#include "C_chunk.h"
#include "stub.h"

// GLOBAL: GAME 0x63d2d8
std::vector<C_animations_machine_de_luxe::S_track> C_animations_machine_de_luxe::tracks;
// GLOBAL: GAME 0x63d2e8
int C_animations_machine_de_luxe::base_tracks;

// Empty: the linker folded it with the other empty functions of its kind (0x4aabe0), where C_traffic_generator::
// ClearElements calls it (PS2's is in this file, its place on PC isn't known)
C_animations_machine_de_luxe::~C_animations_machine_de_luxe()
{
}

// The tracks are searched from the last one. The failures share the chunk's destructor and return -1 at the end, PS2's
// too, and the track has a block of its own: the original destroys it before it counts the tracks. The first five reads
// are inlined, the budget leaves the others and the second vector's constructor out of line, as the original.
// FUNCTION: GAME 0x40e020
int C_animations_machine_de_luxe::LoadTrack(const char *name)
{
    for (int i = tracks.size(); i--;)
    {
        if (!_strcmpi(name, tracks[i].name))
            return i;
    }
    char path[128];
    // STRING: GAME 0x632280
    sprintf(path, "anims\\%s", name);
    C_chunk chunk;
    if (chunk.OpenFile(path, false))
    {
        int version;
        chunk.Read(&version, sizeof(version));
        if (version == TCK_VERSION)
        {
            {
                S_track t;
                strcpy(t.name, name);
                chunk.Read(&t.unknown_28, sizeof(t.unknown_28));
                chunk.Read(&t.unknown_34, sizeof(t.unknown_34));
                chunk.Read(&t.unknown_20, sizeof(t.unknown_20));
                chunk.Read(&t.unknown_24, sizeof(t.unknown_24));
                int num;
                chunk.Read(&num, sizeof(num));
                if (num)
                {
                    t.unknown_44.resize(num, S_vector(0.0f, 0.0f, 0.0f));
                    chunk.Read(t.unknown_44.begin(), num * sizeof(S_vector));
                }
                chunk.Read(&num, sizeof(num));
                if (num)
                {
                    t.unknown_54.resize(num, S_vector(0.0f, 0.0f, 0.0f));
                    chunk.Read(t.unknown_54.begin(), num * sizeof(S_vector));
                }
                t.unknown_40 = num > 0;
                tracks.push_back(t);
                chunk.Close();
            }
            return tracks.size() - 1;
        }
        chunk.Close();
    }
    return -1;
}

// The tracks after the base ones from the last one, or all of them before C_game::Init has counted the base ones
// FUNCTION: GAME 0x40e510
void C_animations_machine_de_luxe::ClearNoBaseTracks()
{
    if (base_tracks > 0)
    {
        while (tracks.size() > base_tracks)
            tracks.pop_back();
    }
    else
        ClearTracks();
}

#pragma auto_inline(off)

// STUB: GAME 0x40e7a0
void C_animations_machine_de_luxe::Reset()
{
    StubLog("C_animations_machine_de_luxe::Reset");
}

#pragma auto_inline(on)
