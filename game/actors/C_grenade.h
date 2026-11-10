#pragma once

#include "actors/C_bottle.h"

// A grenade's saved game, after the bottle's (the name is ours), 0x1d bytes: SaveGameSave writes the members in this
// order
#pragma pack(push, 1)
struct S_grenade_savegame
{
    uchar version;                      // 0x0, GRENADE_SAVEGAME_VERSION
    bool timer;                         // 0x1
    int fuse_time;                      // 0x2
    bool impact;                        // 0x6
    bool explode;                       // 0x7
    float explosion_radius;             // 0x8
    float explosion_power;              // 0xc
    bool fire;                          // 0x10
    int fire_time;                      // 0x11
    float fire_unknown1;                // 0x15
    float fire_unknown2;                // 0x19
};
#pragma pack(pop)

// The version of the grenade's saved game, the only one SaveGameLoad takes (the name is ours)
#define GRENADE_SAVEGAME_VERSION 1

// The grenades, the Molotov cocktails and the cheat Magnum's shots (PS2 C_grenade, C_grenade.cpp), ACTOR_GRENADE, 0x160
// bytes (PS2's 0x1c0): a bottle's physics, which C_game::NewGrenade creates and throws. AI sets it off once its fuse
// has burnt down (timer) or it has hit something (impact): an explosion (explode) or a fire (fire) where it is, then it
// leaves the game.
// VTABLE: GAME 0x623d70
class C_grenade : public C_bottle
{
public:
    C_grenade();
    virtual ~C_grenade();
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual void NetDirect(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual bool SaveData(C_chunk *chunk);
    virtual bool LoadData(C_chunk *chunk);
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();
    // A hit counts where it goes off at the first one; a removed grenade takes no collision or hit (false and true)
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);

    // The fire it sets where it goes off, with NewFire's time and floats (the name is ours). Its copy is in game.cpp's
    // range, among the hydrants' functions (a deferred item of the precompiled header): C_game::NewGrenade inlines
    // it for type 2 and calls it for the Molotov cocktail.
    // FUNCTION: GAME 0x5b9b60
    void SetFire(bool on, int time, float unknown1, float unknown2)
    {
        fire = on;
        fire_time = time;
        fire_unknown1 = unknown1;
        fire_unknown2 = unknown2;
    }

    // The fuse (the names are ours): AI counts fuse_time (ms) down while timer is set, and sets the grenade off below 0
    bool timer;                         // 0x138
    uchar unknown_139[3];               // 0x139
    int fuse_time;                      // 0x13c
    // 0x140, it goes off at its first hit (the bottle's bounces above 0), which Collision and Hit count (the name is
    // ours)
    bool impact;
    // 0x141, it explodes (C_game::NewExplosion) where it goes off, with the radius and the power (NewGrenade's 15 and
    // 400; the names are ours, after NewExplosion's use of them)
    bool explode;
    uchar unknown_142[2];               // 0x142
    float explosion_radius;             // 0x144
    float explosion_power;              // 0x148
    // 0x14c, it sets a fire (C_game::Unknown_5abe10, PS2's NewFire) where it goes off, with NewFire's time and its two
    // floats (NewGrenade's 5000, 2.5 and 50; the names are ours): SetFire's
    bool fire;
    uchar unknown_14d[3];               // 0x14d
    int fire_time;                      // 0x150
    float fire_unknown1;                // 0x154
    float fire_unknown2;                // 0x158
    bool unknown_15c;                   // 0x15c, true from the constructor (PS2's 0x1b4 the same)
    uchar unknown_15d[3];               // 0x15d
};

// SYNTHETIC: GAME 0x445290
// C_grenade::`scalar deleting destructor'
