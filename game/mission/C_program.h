#pragma once

#include "Main.h"

#include <vector>

class C_chunk;
class I3D_frame;

// A script (PS2 C_program, C_program.cpp), 0x68 bytes. PS2's has the same members at offsets 4 lower (its vtable is
// last), and 4 more lower after each vector (PS2's are 12 bytes). Only what InitSystem, CloseSystem and
// C_mission::OpenBin use so far.
// VTABLE: GAME 0x624858
class C_program
{
public:
    // A compiled command (PS2 C_program::S_instruction), PS2's 8 bytes; what it holds isn't known yet
    struct S_instruction
    {
        uchar unknown_0[8];
    };

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
    // through it (PS2's too)
    virtual ~C_program();

    // Reads the script from its chunk of the mission's scene2.bin (PS2 name)
    bool LoadSourceCode(C_chunk *chunk);
    // Gives the script the source of the last of change_progs that names its mission, its name and its actor's frame
    // (PS2 name): a change without a name or a frame fits only a script without one
    void TryChangeProgramData();
    // Replaces the source with a copy of src, NULL for none (PS2 name)
    void SetSourceCode(char *src);

    char *name;                         // 0x4, what S_prog_change::program names (the names are ours)
    char *source;                       // 0x8
    std::vector<S_instruction> instructions;  // 0xc
    uchar unknown_1c[0x14];             // 0x1c
    std::vector<ushort> unknown_30;     // 0x30 (PS2's vector<unsigned short>)
    uchar unknown_40[4];                // 0x40
    // Whose frame S_prog_change::frame names (the name is ours, from TryChangeProgramData, which reads the actor's frame)
    C_actor *actor;                     // 0x44
    // The registers, each an array and its size, as PS2's GetFloatReg, GetFrameReg and GetActorReg read them (the
    // names are ours)
    float *float_regs;                  // 0x48
    int num_float_regs;                 // 0x4c
    I3D_frame **frame_regs;             // 0x50
    int num_frame_regs;                 // 0x54
    C_actor **actor_regs;               // 0x58
    int num_actor_regs;                 // 0x5c
    float unknown_60;                   // 0x60, 0.5 at first
    uchar unknown_64[4];                // 0x64
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
