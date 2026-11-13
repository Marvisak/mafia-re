// The humans' blood (PS2 C_blood.cpp; the PC's file is the second one in the range 0x411be0-0x41234c, after C_chunk.cpp,
// docs/game-files.md): C_blood's Tick (0x411e30) and Init (0x411fb0), then C_blood_manager's destructor, GameInit
// (0x412020), GameDone (0x412050), Tick (0x4120a0) and AddBlood (0x412140), PS2's order. A drop of blood falls from
// where a hit's blood comes out until it hits the level, where it leaves a track (C_game::CreateTrackEffect).

#include "actors/C_blood.h"

#include "actors/C_actor.h"
#include "collision/g_c_glob.h"
#include "mission/Mission.h"
#include "mission/game.h"

// A drop of blood (PS2 C_blood), 0x24 bytes (AddBlood's new; the members' names are ours, after Init's parameters).
// The class is here rather than in the header, which game.h includes: the manager only keeps pointers.
class C_blood
{
public:
    C_blood();
    ~C_blood()
    {
    }

    // The drop by the time passed in seconds: false once it is gone (PS2 name)
    bool Tick(float time);
    // PS2 name and types
    void Init(S_vector &pos, S_vector &dir, float size, int texture, bool grow);

    S_vector pos;                       // 0x0
    S_vector dir;                       // 0xc, the velocity, in m/s
    float size;                         // 0x18, the track's size, scaled at random
    int texture;                        // 0x1c, the track's index into C_game::track_textures
    bool grow;                          // 0x20, the track grows (CreateTrackEffect's)
};

// Empty, defined out of class as PS2's: AddBlood calls it, and the linker folded it with the other functions of its
// code (mov eax, ecx; ret) at 0x605560, so it has no address of its own and no marker
C_blood::C_blood()
{
}

// The drop falls (10 m/s^2) and is gone once it rises faster than 20 m/s, or where its move hits the level: a track
// there, half a metre off the surface, turned and sized at random (PS2's the same). The original's bytes; reccmp's
// 99.10% takes the flags' 0x600000 for an address in the original's code. The game through GetGame(), which VC6 loads
// before the angle's rnd() (the member read directly, after it).
// FUNCTION: GAME 0x411e30
bool C_blood::Tick(float time)
{
    dir.y -= 10.0f * time;
    if (dir.y > 20.0f)
        return false;
    S_vector move = dir * time;
    S_vector normal, hit;
    if (gifCollision.TestLineHStatic(pos, move, &hit, &normal, COLF2_UNKNOWN_200000 | COLF2_UNKNOWN_400000))
    {
        S_vector track_pos = hit + normal * .5f;
        S_vector track_normal = -normal;
        float s = rnd() * .3f * size + .5f;
        S_vector track_size(s, s, 1.0f);
        act_mission->GetGame()->CreateTrackEffect(track_pos, track_normal, rnd() * 2.0f * 3.1415927f, track_size, .3f,
            texture, true, 1.0f, true, grow, NULL);
        return false;
    }
    pos += move;
    return true;
}

// FUNCTION: GAME 0x411fb0
void C_blood::Init(S_vector &pos, S_vector &dir, float size, int texture, bool grow)
{
    this->pos = pos;
    this->dir = dir;
    this->size = size;
    this->texture = texture;
    this->grow = grow;
}

// The same code as C_tshift's constructor, whose copy the linker kept (0x5604f0): the marker is mission/tshift.h's
C_blood_manager::C_blood_manager()
{
}

// The same code as C_tshift's destructor; the original kept this copy (0x412000), ours keeps either, so the marker is by
// name, actors/C_blood.h's
C_blood_manager::~C_blood_manager()
{
}

// FUNCTION: GAME 0x412020
void C_blood_manager::GameInit()
{
    bloods.clear();
}

// From the last one, as PS2's
// FUNCTION: GAME 0x412050
void C_blood_manager::GameDone()
{
    for (int i = bloods.size(); i--;)
        delete bloods[i];
    bloods.clear();
}

// A drop whose Tick ends is deleted, the last one taken into its place (PS2's the same)
// FUNCTION: GAME 0x4120a0
void C_blood_manager::Tick(uint time)
{
    float t = time * .001f;
    for (uint i = 0; i < bloods.size();)
    {
        if (!bloods[i]->Tick(t))
        {
            delete bloods[i];
            bloods[i] = bloods.back();
            bloods.pop_back();
        }
        else
            i++;
    }
}

// FUNCTION: GAME 0x412140
void C_blood_manager::AddBlood(S_vector &pos, S_vector &dir, float size, int texture, bool grow)
{
    C_blood *blood = new C_blood;
    if (blood)
    {
        blood->Init(pos, dir, size, texture, grow);
        bloods.push_back(blood);
    }
}
