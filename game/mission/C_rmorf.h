#pragma once

#include "Main.h"

class I3D_visual;

// The most regions and levels of a set's weights (the names are ours, by Tick's arrays)
#define RMORF_MAX_REGIONS 16
#define RMORF_MAX_LEVELS 8

// A set's mode (RMRF_S_SETREF::mode; the names are ours, by what Tick does)
#define RMORF_MODE_SMOOTH 1             // the weights move along a half cosine, not linearly
#define RMORF_MODE_CYCLE 0x80           // each level takes the previous one's weight, the first the last one's

// A set read from the mission's rmorf.bin (PS2 RMRF_S_SETREF, C_rmorf.cpp), a list, 0x430 bytes: visuals whose morph
// weights Tick moves, the same in each, from the weights to the weights plus the deltas over a period. The names of the
// members are ours.
struct RMRF_S_SETREF
{
    uint count;                         // 0x0, of the visuals
    I3D_visual **data;                  // 0x4, the visuals, an array
    RMRF_S_SETREF *next;                // 0x8
    uint num_levels;                    // 0xc
    uint num_regions;                   // 0x10
    uint time;                          // 0x14, in the period
    uint period;                        // 0x18, period_base and a random part of period_rand, each period
    float weights[RMORF_MAX_REGIONS][RMORF_MAX_LEVELS];  // 0x1c, at the period's start
    float deltas[RMORF_MAX_REGIONS][RMORF_MAX_LEVELS];  // 0x21c, by the period's end
    uint mode;                          // 0x41c, RMORF_MODE_*
    uint period_base;                   // 0x420
    uint period_rand;                   // 0x424
    // How many levels the random mode turns on each period: count_base and a random part of count_rand
    uint count_base;                    // 0x428
    uint count_rand;                    // 0x42c

    // Reads a set from data into a new set at the list's head, returning what follows it (PS2 name)
    static char *LoadRAWSet(RMRF_S_SETREF **list, char *data);

    // In the class: C_rmorf's destructor has it inlined, and its scalar deleting destructor calls itself for the list's
    // next set. The original's copy of that follows C_rmorf's destructor, ours comes at the file's end: under /YX a
    // precompiled header's inline function comes due once the file's own source outweighs the headers' inline code
    // before it, and the file's code before the constructor isn't written yet
    ~RMRF_S_SETREF()
    {
        if (data)
            delete[] data;
        data = NULL;
        count = 0;
        if (next)
            delete next;
        next = NULL;
    }
};

// SYNTHETIC: GAME 0x4992e0
// RMRF_S_SETREF::`scalar deleting destructor'

// The mission's missions\<name>\rmorf.bin (PS2 C_rmorf, C_rmorf.cpp), the game's 0x2bc, 8 bytes. Its file also has a
// global one (0x63d7f0), whose initializer starts the file. All but the sets' loader are written.
// VTABLE: GAME 0x624b50
class C_rmorf
{
public:
    RMRF_S_SETREF *sets;                // 0x4 (PS2 0x0, before its vtable)

    C_rmorf();
    // Not virtual: the vtable doesn't have it
    ~C_rmorf();

    // PS2's virtual functions, in PS2's order
    virtual void Tick(int time);
    virtual void Mission_Start();
    virtual void Mission_Done();
};

// The std::string helpers fstring's constructor calls where it inlines assign, and the empty string c_str returns
// without a buffer: the first copies are C_rmorf.cpp's
// TEMPLATE: GAME 0x4999b0
// std::basic_string<char,std::char_traits<char>,std::allocator<char> >::_Tidy

// TEMPLATE: GAME 0x499a00
// std::basic_string<char,std::char_traits<char>,std::allocator<char> >::_Copy

// GLOBAL: GAME 0x624b5c
// `std::basic_string<char,std::char_traits<char>,std::allocator<char> >::_Nullstr'::`2'::_C
