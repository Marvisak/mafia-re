// The AI's groups, following and fights (PS2 jp_ai_groups.cpp; the PC's file is most likely the one at
// 0x530390-0x53857c, after jp_ai_entity.cpp as the names sort, docs/game-files.md). Written so far: the managers' Init
// and Destroy, which C_game's Init and Done call, PS2's shapes. The copies of the implicit functions they call
// (ai_group's operator= and destructor, the vectors' helpers) are compiler-generated.

#include "ai/jp_ai_groups.h"

// FUNCTION: GAME 0x531130
void ai_groups::Init()
{
    unknown_0 = -1;
    unknown_4 = 0;
    groups.clear();
    unknown_18.clear();
}

// FUNCTION: GAME 0x5311a0
void ai_groups::Destroy()
{
    unknown_0 = -1;
    unknown_4 = 0;
    unknown_18.clear();
    for (int i = groups.size() - 1; i >= 0; i--) {
        groups[i].members.clear();
        groups[i].unknown_20.clear();
        groups[i].events.clear();
        groups[i].unknown_40.clear();
        groups[i].unknown_50.clear();
    }
    groups.clear();
}

// FUNCTION: GAME 0x532da0
void ai_follow_manager::Init()
{
    unknown_10 = 0;
    groups.clear();
}

// FUNCTION: GAME 0x532e30
void ai_follow_manager::Destroy()
{
    unknown_10 = 0;
    for (int i = groups.size() - 1; i >= 0; i--)
        groups[i].followers.clear();
    groups.clear();
}

// FUNCTION: GAME 0x534ec0
void ai_fight_manager::Init()
{
    parties.clear();
}

// FUNCTION: GAME 0x534f70
void ai_fight_manager::Destroy()
{
    for (int i = parties.size() - 1; i >= 0; i--)
        parties[i].members.clear();
    parties.clear();
}
