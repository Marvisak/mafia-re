#pragma once

#include "Main.h"

// A set read from the mission's rmorf.bin (PS2 RMRF_S_SETREF, C_rmorf.cpp), 0xc bytes, a list; the names of the members
// are ours
struct RMRF_S_SETREF
{
    int count;                          // 0x0
    uint *data;                         // 0x4, an array
    RMRF_S_SETREF *next;                // 0x8

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
// global one (0x63d7f0), whose initializer starts the file. Only the constructor and the destructor are written so far.
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
