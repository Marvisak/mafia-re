#pragma once

#include "Main.h"

#include <vector>

// A script (PS2 C_program, C_program.cpp). Only what InitSystem and CloseSystem use so far.
class C_program
{
public:
    // A script replaced by the patch (PS2 C_program::S_prog_change, 16 bytes; the members' names are ours, from PS2's
    // TryChangeProgramData, which compares the first three with the mission's name, the script's and its frame's, and
    // gives the script the source of the first match)
    struct S_prog_change
    {
        char *mission;                  // 0x0
        char *program;                  // 0x4
        char *frame;                    // 0x8
        char *source;                   // 0xc
    };

    // What the patch's files replace, which LoadPatchFile fills (PS2 name; nothing fills PS2's)
    static std::vector<S_prog_change> change_progs;
};

// Loads one of the patch's script files, patch\a1.bin to a4.bin, into C_program::change_progs (PC only, the name is
// ours)
void LoadPatchFile(const char *name);
// Frees what LoadPatchFile loaded, at CloseSystem (PC only, the name is ours)
void UnloadPatchFiles();

// The changes' push_back calls insert out of line, and insert its helpers; the linker folded _Ucopy, _Construct and
// _Ufill with those of every other 16-byte type, whose earlier files' copies the original kept
// TEMPLATE: GAME 0x47be20
// std::vector<C_program::S_prog_change,std::allocator<C_program::S_prog_change> >::size

// TEMPLATE: GAME 0x47be40
// std::vector<C_program::S_prog_change,std::allocator<C_program::S_prog_change> >::insert

// TEMPLATE: GAME 0x498f60
// std::vector<C_program::S_prog_change,std::allocator<C_program::S_prog_change> >::_Ucopy

// TEMPLATE: GAME 0x5bca00
// ?_Construct@std@@YIXPAUS_prog_change@C_program@@ABU23@@Z

// TEMPLATE: GAME 0x5f4af0
// std::vector<C_program::S_prog_change,std::allocator<C_program::S_prog_change> >::_Ufill
