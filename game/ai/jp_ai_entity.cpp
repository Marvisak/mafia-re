// The AI's humans (PS2 jp_ai_entity.cpp, C_entity's; the PC's file is the one at 0x505400-0x53038c,
// docs/game-files.md), whose constructor (0x5071a0) comes after functions of its own. Not written yet: the
// constructor, which C_mission::CreateActor calls, the cutscene's block and wake, which C_game::EntityBlock calls, and
// what the script's commands call (the senses', the logs' and the jobs' functions, ForceScript, WeaponChoose,
// AddUsingObject) are stubs.

#include "ai/jp_ai_entity.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x505d20
void ai_sensors::Init(void *owner)
{
    StubLog("ai_sensors::Init");
}

// STUB: GAME 0x506340
void ai_vis_logs::Init(void *owner)
{
    StubLog("ai_vis_logs::Init");
}

// STUB: GAME 0x5071a0
C_entity::C_entity()
{
    StubLog("C_entity::C_entity");
}

// STUB: GAME 0x523f80
void C_entity::ForceScript(bool unknown)
{
    StubLog("C_entity::ForceScript");
}

// STUB: GAME 0x524650
void ai_action_manager::Clear(bool unknown)
{
    StubLog("ai_action_manager::Clear");
}

// STUB: GAME 0x527600
ai_action *ai_action_manager::GetBaseJob()
{
    StubLog("ai_action_manager::GetBaseJob");
    return NULL;
}

// STUB: GAME 0x528370
void C_entity::WeaponChoose(uint id)
{
    StubLog("C_entity::WeaponChoose");
}

// STUB: GAME 0x529100
void C_entity::AddUsingObject()
{
    StubLog("C_entity::AddUsingObject");
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
