// The AI's humans (PS2 jp_ai_entity.cpp, C_entity's; the PC's file is the one at 0x505400-0x53038c,
// docs/game-files.md), whose constructor (0x5071a0) comes after functions of its own. Not written yet: the
// constructor, which C_mission::CreateActor calls, and the cutscene's block and wake, which C_game::EntityBlock calls,
// are stubs.

#include "ai/jp_ai_entity.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x5071a0
C_entity::C_entity()
{
    StubLog("C_entity::C_entity");
}

// STUB: GAME 0x52ff50
void C_entity::SedniNaPrdelASledujKino(bool unknown)
{
    StubLog("C_entity::SedniNaPrdelASledujKino");
}

// STUB: GAME 0x52ffe0
void C_entity::VzbudSeUzSouTitulky()
{
    StubLog("C_entity::VzbudSeUzSouTitulky");
}

#pragma auto_inline(on)
