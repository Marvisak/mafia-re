// The debris (PS2 VE_Debris.cpp; the PC's file is most likely the one at 0x564890-0x56615c, docs/game-files.md).
// Written so far: Init and Done, which C_game's Init and Done call.

#include "vehicles/VE_Debris.h"

#include "I3D/I3D.h"

// The gravity goes through a temporary, as PS2's
// FUNCTION: GAME 0x564890
bool C_Debris::Init(float gravity)
{
    this->gravity = S_vector(0.0f, -gravity, 0.0f);
    debris.clear();
    flashes.clear();
    return true;
}

// FUNCTION: GAME 0x564920
bool C_Debris::Done()
{
    for (int i = debris.size(); i--;)
    {
        debris[i].frame->LinkTo(NULL, 0);
        debris[i].frame->Release();
    }
    debris.clear();
    flashes.clear();
    return true;
}
