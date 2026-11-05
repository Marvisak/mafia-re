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
                chunk.Read(&t.length, sizeof(t.length));
                chunk.Read(&t.key_time, sizeof(t.key_time));
                int num;
                chunk.Read(&num, sizeof(num));
                if (num)
                {
                    t.positions.resize(num, S_vector(0.0f, 0.0f, 0.0f));
                    chunk.Read(t.positions.begin(), num * sizeof(S_vector));
                }
                chunk.Read(&num, sizeof(num));
                if (num)
                {
                    t.directions.resize(num, S_vector(0.0f, 0.0f, 0.0f));
                    chunk.Read(t.directions.begin(), num * sizeof(S_vector));
                }
                t.has_directions = num > 0;
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

// STUB: GAME 0x40e7a0
void C_animations_machine_de_luxe::Reset()
{
    StubLog("C_animations_machine_de_luxe::Reset");
}

// STUB: GAME 0x40e010
C_animations_machine_de_luxe::C_animations_machine_de_luxe()
{
    StubLog("C_animations_machine_de_luxe::C_animations_machine_de_luxe");
}

// STUB: GAME 0x40e660
void C_animations_machine_de_luxe::Init(C_actor *act)
{
    StubLog("C_animations_machine_de_luxe::Init");
}

// STUB: GAME 0x40e890
void C_animations_machine_de_luxe::GetActiveAnimTime(uint &time, uint &length)
{
    StubLog("C_animations_machine_de_luxe::GetActiveAnimTime");
    time = length = 0;
}

// STUB: GAME 0x40e8e0
void C_animations_machine_de_luxe::StopAnims()
{
    StubLog("C_animations_machine_de_luxe::StopAnims");
}

// STUB: GAME 0x40e950
void C_animations_machine_de_luxe::SetAnim(const char *name, bool b1, bool b2, bool b3, float f1, float f2, bool b4,
    bool b5, S_vector *v, bool b6, uint u)
{
    StubLog("C_animations_machine_de_luxe::SetAnim");
}

// STUB: GAME 0x40f170
bool C_animations_machine_de_luxe::IsActive()
{
    StubLog("C_animations_machine_de_luxe::IsActive");
    return false;
}

// STUB: GAME 0x40f1b0
uint C_animations_machine_de_luxe::Tick(uint time)
{
    StubLog("C_animations_machine_de_luxe::Tick");
    return 0;
}

// STUB: GAME 0x410060
void C_animations_machine_de_luxe::SetExtendedAnim(I3D_animation_set *set, float f1, float f2, bool b)
{
    StubLog("C_animations_machine_de_luxe::SetExtendedAnim");
}

// STUB: GAME 0x410150
void C_animations_machine_de_luxe::DelExtendedAnim(float f)
{
    StubLog("C_animations_machine_de_luxe::DelExtendedAnim");
}

// STUB: GAME 0x4101d0
bool C_animations_machine_de_luxe::GetPlacementFromTCK(const char *name, S_vector &pos, S_vector &dir)
{
    StubLog("C_animations_machine_de_luxe::GetPlacementFromTCK");
    return false;
}

// STUB: GAME 0x40e900
void C_animations_machine_de_luxe::RecompileMatrix(S_matrix &m)
{
    StubLog("C_animations_machine_de_luxe::RecompileMatrix");
}

// STUB: GAME 0x4103d0
void C_animations_machine_de_luxe::StageBlendingTick(uint time)
{
    StubLog("C_animations_machine_de_luxe::StageBlendingTick");
}

// STUB: GAME 0x410310
bool C_animations_machine_de_luxe::GetEndPointOfTCK(const char *name, S_vector &pos)
{
    StubLog("C_animations_machine_de_luxe::GetEndPointOfTCK");
    return false;
}

// STUB: GAME 0x410350
bool C_animations_machine_de_luxe::GetMixingPointOfTCK(const char *name, S_vector &pos)
{
    StubLog("C_animations_machine_de_luxe::GetMixingPointOfTCK");
    return false;
}
