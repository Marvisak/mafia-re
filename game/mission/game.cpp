// The game of the loaded mission (PS2 game.cpp; the PC's file is the one at 0x5a0760-0x5bcc0c, docs/game-files.md).
// Written so far: the stacks' Clear functions, which the destructor calls (the constructor and the destructor are
// game.h's); Init, which main calls, is a stub.

#include "mission/game.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x5a0810
void C_game::Init()
{
    StubLog("C_game::Init");
}

#pragma auto_inline(on)

// From the last copy to the first
// FUNCTION: GAME 0x5b1da0
void C_game::InventoryStackClear()
{
    for (int i = inventory_stack.size(); i--; )
        delete inventory_stack[i];
    inventory_stack.clear();
}

// FUNCTION: GAME 0x5b2320
void C_game::FloatStackClear()
{
    float_stack.clear();
}

// FUNCTION: GAME 0x5b2590
void C_game::CharacterStackClear()
{
    character_stack.clear();
}
