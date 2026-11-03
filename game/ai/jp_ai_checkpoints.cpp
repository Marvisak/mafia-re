// Most likely PS2's jp_ai_checkpoints.cpp (file 52 at 0x4f8490-0x5053fc, docs/game-files.md), between
// jp_ai_actor_history.cpp and jp_ai_entity.cpp as the names sort: ai_wayfind's methods and ai_path's (PS2's other
// classes of the AI's paths). Nothing written yet: the functions C_human.cpp calls are stubs.

#include "ai/jp_ai_main.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x4fbae0
void ai_wayfind::GetPosition(S_vector &pos, uint *node, uint *found, uint mode)
{
    StubLog("ai_wayfind::GetPosition");
}

// STUB: GAME 0x501ff0
bool ai_path::isInConnection(const S_vector &a, const S_vector &b, const S_vector &pos, float width_a, float width_b)
{
    StubLog("ai_path::isInConnection");
    return false;
}

#pragma auto_inline(on)
