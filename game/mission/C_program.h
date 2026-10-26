#pragma once

#include "Main.h"

#include <vector>

class C_chunk;

// A script (PS2 C_program, C_program.cpp), 0x68 bytes. Only what InitSystem, CloseSystem and C_mission::OpenBin use so
// far.
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

    C_program();
    // Virtual, the vtable's first function (0x624858, ten functions as PS2's): C_mission::Close deletes its scripts
    // through it (PS2's too). Not written yet.
    virtual ~C_program();

    // Reads the script from its chunk of the mission's scene2.bin (PS2 name)
    void LoadSourceCode(C_chunk *chunk);

    uchar unknown_4[0x64];
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
