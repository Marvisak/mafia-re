// The game of the loaded mission (PS2 game.cpp; the PC's file is the one at 0x5a0760-0x5bcc0c, docs/game-files.md).
// Written so far: the stacks' Clear functions, which the destructor calls (the constructor and the destructor are
// game.h's), and IShowSetVolume, which the profiles call; Init, which main calls, and Done and DoneIntern, which
// C_mission::Close calls, are stubs.

#include "mission/game.h"
#include "ui/g_loadsave.h"

#include "IGraph/IGraph.h"
#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x5a0810
void C_game::Init()
{
    StubLog("C_game::Init");
}

// STUB: GAME 0x5a3c60
void C_game::Done()
{
    StubLog("C_game::Done");
}

// STUB: GAME 0x5a4d20
void C_game::DoneIntern()
{
    StubLog("C_game::DoneIntern");
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

// From the last video to the first
// FUNCTION: GAME 0x5b6600
void C_game::IShowSetVolume()
{
    for (int i = i_shows.size(); i--; )
        i_shows[i].show->SetVolume(mafiaLoadSave.settings.music_volume * i_shows[i].volume);
}
