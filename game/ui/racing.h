#pragma once

#include "Main.h"

class C_car;

// Where an opponent is in the race (the name is ours): what the race camera reads
struct S_opponent_place
{
    uchar unknown_0[0x1c];
    int gate;                           // 0x1c (PS2 0x20), the number of the gate the opponent passed last (ours)
};

// A car in the race (PS2 C_opponent). Only what the race camera reads.
class C_opponent
{
public:
    uchar unknown_0[0x28];
    C_car *car;                         // 0x28 (PS2 0x134)
    uchar unknown_2c[0x28];             // 0x2c
    S_opponent_place *place;            // 0x54 (PS2 0x13c; the name is ours)
};

// The racing championship (PS2 C_racing, racing.cpp), 0x688 bytes, which exists while its menu runs. Only what main
// and the race camera use so far.
class C_racing
{
public:
    C_racing();
    ~C_racing();

    // PS2 names
    void GameInit();
    void GameDone();

    uchar unknown_0;
    bool unknown_1;                     // 0x1, C_game::Done deletes the racing when it is set
    uchar unknown_2[0x236];
    // The race's cars (the names are ours)
    C_opponent *opponents[0x19];        // 0x238 (PS2 0xa3c)
    uint num_opponents;                 // 0x29c (PS2 0xac8)
    uchar unknown_2a0[0x3e8];           // 0x2a0
};

extern C_racing *CRacing;

// The championship's menu, with CRacing created (PS2's file isn't known, perhaps g_m_race.cpp)
void ExecuteRacingMenu();
