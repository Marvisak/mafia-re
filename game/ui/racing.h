#pragma once

#include "Main.h"

class C_car;

// Where an opponent is in the race (the name is ours): what the race camera reads
struct S_opponent_place
{
    uchar unknown_0[0x1c];
    int gate;                           // 0x1c (PS2 0x20), the number of the gate the opponent passed last (ours)
};

// A car in the race (PS2 C_opponent). Only what the race camera and the scripts read.
class C_opponent
{
public:
    uchar unknown_0[0x20];
    // 0x20, the driver's index in tables\playerindex.def (G_CarDatabase::GetPlayerModel's)
    uint unknown_20;
    uchar unknown_24[4];
    C_car *car;                         // 0x28 (PS2 0x134)
    uchar unknown_2c[0x28];             // 0x2c
    S_opponent_place *place;            // 0x54 (PS2 0x13c; the name is ours)
    uchar unknown_58[8];
    int unknown_60;                     // 0x60, what RACING_CHANGE_MODEL looks the opponent up by

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours)
    // FUNCTION: GAME 0x47a370
    int GetUnknown60() const
    {
        return unknown_60;
    }
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
    // The race's cars (the names are ours), two counts of them: RACING_CHANGE_MODEL walks unknown_298 of them (most
    // likely PS2's GetOpponentCount's, 0xac0), the race camera num_opponents
    C_opponent *opponents[0x18];        // 0x238 (PS2 0xa3c)
    int unknown_298;                    // 0x298
    uint num_opponents;                 // 0x29c (PS2 0xac8)
    uchar unknown_2a0[0x3e8];           // 0x2a0

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours)
    // FUNCTION: GAME 0x47a380
    C_opponent **GetOpponents()
    {
        return opponents;
    }
    // FUNCTION: GAME 0x47a390
    int GetUnknown298() const
    {
        return unknown_298;
    }
};

extern C_racing *CRacing;

// The championship's menu, with CRacing created (PS2's file isn't known, perhaps g_m_race.cpp)
void ExecuteRacingMenu();
// The racing mission's (mission 6's) race, which its script starts (PS2 name, after ExecuteRacingMenu as on the PC;
// the parameters' meaning isn't known)
void RacingMisse06(uint a, uint b, uint c, uint d);
