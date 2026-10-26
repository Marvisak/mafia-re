#pragma once

#include "Main.h"
#include "tools.h"

#include <vector>

// The police (PS2 C_shvestky, C_shvestky.cpp), the game's 0x2da0, 0xb0 bytes. PS2's has the same members, its offsets in
// the comments. Only the constructor and the destructor are written so far.
class C_shvestky
{
public:
    // The road blocks' cars, policemen and enemies (PS2's names), 8 bytes each by the file's code; what they hold isn't
    // known yet
    struct S_road_block_car
    {
        uchar unknown_0[8];
    };
    struct S_road_block_iris
    {
        uchar unknown_0[8];
    };
    struct S_road_block_enemy
    {
        uchar unknown_0[8];
    };

    int unknown_0;                      // 0x0
    S_timer unknown_4;                  // 0x4, PS2's Tick runs TickDoRiciActors at each of its periods
    uchar unknown_c[8];                 // 0xc
    S_timer unknown_14;                 // 0x14
    S_timer unknown_1c;                 // 0x1c
    S_timer unknown_24;                 // 0x24
    uchar unknown_2c[0x10];             // 0x2c
    std::vector<uint> unknown_3c;       // 0x3c (PS2 0x40), 4-byte values (PS2's vector<unsigned int>)
    std::vector<S_road_block_car> road_block_cars;  // 0x4c
    std::vector<S_road_block_iris> road_block_irises;  // 0x5c
    std::vector<S_road_block_enemy> road_block_enemies;  // 0x6c
    std::vector<uint> unknown_7c;       // 0x7c (PS2 0x70)
    std::vector<uint> unknown_8c;       // 0x8c (PS2 0x7c)
    std::vector<ushort> unknown_9c;     // 0x9c (PS2 0x88)
    int unknown_ac;                     // 0xac

    C_shvestky();
    ~C_shvestky();
};
