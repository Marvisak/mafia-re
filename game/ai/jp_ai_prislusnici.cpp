// The police's officers (PS2 jp_ai_prislusnici.cpp; the PC's file is most likely the one at 0x538c80-0x53e4ac,
// docs/game-files.md), in PS2's order. Written so far: the shift's start and end, which C_game's Init and Done call,
// and the frames' Clear and Init, which they end with (tail calls); a stub of AkceDokoncenaPlukovniku, which the
// shift's end calls for each action.

#include "ai/jp_ai_prislusnici.h"

#include "I3D/I3D.h"

#include "stub.h"

// FUNCTION: GAME 0x53ba50
void ai_police_nastenka::ZacatekSmeny()
{
    actions.clear();
    unknown_20 = 0;
    unknown_24 = 0xfffffff;
    unknown_2c = false;
    unknown_28 = 1.0f;
    unknown_30 = 0;
    Init();
}

#pragma auto_inline(off)

// STUB: GAME 0x53c550
void ai_police_nastenka::AkceDokoncenaPlukovniku(ai_police_action *action)
{
    StubLog("ai_police_nastenka::AkceDokoncenaPlukovniku");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x53d750
void ai_police_nastenka::KonecSmeny()
{
    for (int i = actions.size() - 1; i >= 0; i--)
        AkceDokoncenaPlukovniku(&actions[i]);
    Clear();
}

// FUNCTION: GAME 0x53d8c0
void ai_animitem_manager::Clear()
{
    for (int i = items.size() - 1; i >= 0; i--) {
        items[i].frame->LinkTo(NULL, 0);
        items[i].frame->SetOn(false);
        items[i].frame->Release();
        items[i].frame = NULL;
        items.erase(&items[i]);
    }
    items.clear();
}

// FUNCTION: GAME 0x53d9c0
void ai_animitem_manager::Init()
{
    items.clear();
}
