// The humans' blood (PS2 C_blood.cpp; the PC's file is the second one in the range 0x411be0-0x41234c, after C_chunk.cpp,
// docs/game-files.md): C_blood's Tick (0x411e30) and Init (0x411fb0), then C_blood_manager's destructor, GameInit
// (0x412020), GameDone (0x412050), Tick (0x4120a0) and AddBlood (0x412140), PS2's order. Written so far: the manager's
// constructor and destructor, which the game's constructor and destructor call, and its GameInit and GameDone.

#include "actors/C_blood.h"

#include "stub.h"

// A blood stain (PS2 C_blood), 0x24 bytes (AddBlood's new). Only its destructor so far, empty, for which GameDone's
// delete tests the pointer; the class is here rather than in the header, which game.h includes, until more of it is
// written.
class C_blood
{
public:
    ~C_blood()
    {
    }

    uchar unknown_0[0x24];
};

// The same code as C_tshift's constructor, whose copy the linker kept (0x5604f0): the marker is mission/tshift.h's
C_blood_manager::C_blood_manager()
{
}

// The same code as C_tshift's destructor; the original kept this copy (0x412000), ours keeps either, so the marker is by
// name, actors/C_blood.h's
C_blood_manager::~C_blood_manager()
{
}

// FUNCTION: GAME 0x412020
void C_blood_manager::GameInit()
{
    bloods.clear();
}

// From the last one, as PS2's
// FUNCTION: GAME 0x412050
void C_blood_manager::GameDone()
{
    for (int i = bloods.size(); i--;)
        delete bloods[i];
    bloods.clear();
}


