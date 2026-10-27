#pragma once

#include "Main.h"
#include "tools.h"

#include <vector>

class C_actor;
class C_car;

// The police (PS2 C_shvestky, C_shvestky.cpp), the game's 0x2da0, 0xb0 bytes. PS2's has the same members, its offsets in
// the comments. Only the constructor, the destructor and GameInit are written so far.
class C_shvestky
{
public:
    // A policeman's record of the people he is after (PS2 name; PS2's DelNasrany is its): only its destructor is known,
    // which frees a vector at 0x70
    struct S_shvestky
    {
        ~S_shvestky();
    };
    // The road blocks' cars, policemen and enemies (PS2's names), 8 bytes each: the actor and whether GameDone leaves it
    // in the game's lists (the members' names are ours)
    struct S_road_block_car
    {
        C_car *car;                     // 0x0
        bool unknown_4;                 // 0x4
    };
    struct S_road_block_iris
    {
        C_actor *actor;                 // 0x0
        bool unknown_4;                 // 0x4
    };
    struct S_road_block_enemy
    {
        C_actor *actor;                 // 0x0
        bool unknown_4;                 // 0x4
    };

    bool unknown_0;                     // 0x0
    uchar unknown_1[3];
    S_timer unknown_4;                  // 0x4, PS2's Tick runs TickDoRiciActors at each of its periods
    float unknown_c;                    // 0xc, 17 from GameInit (PS2's 27)
    float unknown_10;                   // 0x10, 28 from GameInit (PS2's 1000)
    S_timer unknown_14;                 // 0x14
    S_timer unknown_1c;                 // 0x1c
    S_timer unknown_24;                 // 0x24
    int unknown_2c;                     // 0x2c
    int unknown_30;                     // 0x30
    uchar unknown_34[8];                // 0x34
    std::vector<S_shvestky *> unknown_3c;  // 0x3c (PS2 0x40)
    std::vector<S_road_block_car> road_block_cars;  // 0x4c
    std::vector<S_road_block_iris> road_block_irises;  // 0x5c
    std::vector<S_road_block_enemy> road_block_enemies;  // 0x6c
    std::vector<C_actor *> unknown_7c;  // 0x7c (PS2 0x70)
    std::vector<C_car *> unknown_8c;    // 0x8c (PS2 0x7c), the police's temporary cars, on the radar
    std::vector<ushort> unknown_9c;     // 0x9c (PS2 0x88)
    bool unknown_ac;                    // 0xac, which C_game::Init sets
    uchar unknown_ad[3];

    C_shvestky();
    ~C_shvestky();

    // PS2 names: C_game's Init and Done call them
    void GameInit();
    void GameDone();
    // PS2 name (Czech, roughly "tick the actors in the face")
    void TickDoRiciActors(bool);
};
