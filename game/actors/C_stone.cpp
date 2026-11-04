// The stones (PC only; the file is the one at 0x4aa2b0-0x4aa48c, between C_sledovacka.cpp and C_trf_c.cpp, its name
// ours, docs/game-files.md), which starts with the constructor: a bottle's physics, the frame ticked with it, and out
// of the game once the stone rests or falls too fast.

#include "actors/C_stone.h"

#include "mission/Mission.h"
#include "mission/game.h"

// FUNCTION: GAME 0x4aa2b0
C_stone::C_stone()
{
    use_coll = false;
    unknown_6d = false;
    type = ACTOR_STONE;
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x4aa300
C_stone::~C_stone()
{
}

// C_cartridge's code, which the linker folded it with
// FUNCTION: GAME 0x430830 FOLDED
bool C_stone::Init(I3D_frame *frm)
{
    if (!frm)
        return false;
    return C_bottle::Init(frm) ? true : false;
}

// The bottle's step, and the stone out of the game where the bottle's stops it
// FUNCTION: GAME 0x4aa310
void C_stone::AI(uint time)
{
    C_bottle::AI(time);
    if (speed <= MRG_ZERO && fall_speed <= MRG_ZERO || fall_speed > 50.0f)
    {
        act_mission->game->InvalidateActor(this, false);
        act_mission->game->RemoveTemporaryActor(this);
    }
}

// FUNCTION: GAME 0x4aa380
void C_stone::Update(uint time)
{
    C_bottle::Update(time);
    frame->Tick(time);
}

// The bottle's, with a stone's radius and axis and no states
// FUNCTION: GAME 0x4aa3a0
void C_stone::GameInit()
{
    C_bottle::GameInit();
    axis = S_vector(0.0f, 1.0f, 0.0f);
    set_states = false;
    radius = .1f;
    unknown_65 = true;
}

// The bottle's, then the frame turned off, unlinked and released
// FUNCTION: GAME 0x4aa400
void C_stone::GameDone()
{
    C_bottle::GameDone();
    if (frame)
    {
        frame->SetOn(false);
        frame->LinkTo(NULL, 0);
        frame->Release();
    }
}

// The linker's folded copy of the out-of-line kind (0x4aa430)
int C_stone::SaveGameGetSize()
{
    return 0;
}
