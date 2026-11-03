#pragma once

#include "Main.h"

// A set of the mission's rmorf.bin (PS2 RMRF_S_SETREF), defined in C_rmorf.cpp
struct RMRF_S_SETREF;

// Its scalar deleting destructor, which C_rmorf's destructor calls: the original's copy follows that destructor, its
// first caller, where an inline function of the source file itself goes (a precompiled header's would come later under
// /YX), so the class is the source file's (PS2's file of it too)
// SYNTHETIC: GAME 0x4992e0
// RMRF_S_SETREF::`scalar deleting destructor'

// The mission's missions\<name>\rmorf.bin (PS2 C_rmorf, C_rmorf.cpp), the game's 0x2bc, 8 bytes. Its file also has a
// global one, whose initializer starts the file.
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
