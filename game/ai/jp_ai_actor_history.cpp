// What the AI's entities remember of the other characters (PS2 ai_logs, jp_ai_actor_history.cpp; the PC's file is
// most likely the one at 0x4f6a60-0x4f802c, docs/game-files.md, which PS2's ai_logs functions fit in size). The class
// is in jp_ai_entity.h, whose C_entity's member it is. Only a stub so far: GetLog, which ai_fight_manager::Tick calls.

#include "ai/jp_ai_entity.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x4f77f0
ai_character_log *ai_logs::GetLog(C_actor *act)
{
    StubLog("ai_logs::GetLog");
    return NULL;
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x4f7850
ai_character_log *ai_logs::GetMaxAngryCharacter()
{
    StubLog("ai_logs::GetMaxAngryCharacter");
    return NULL;
}

#pragma auto_inline(on)
