// The AI's groups, following and fights (PS2 jp_ai_groups.cpp; the PC's file is most likely the one at
// 0x530390-0x53857c, after jp_ai_entity.cpp as the names sort, docs/game-files.md). Written so far: the managers' Init
// and Destroy, which C_game's Init and Done call, PS2's shapes, and the groups' Tick, which C_game::Tick calls; stubs
// of what the script's commands call (GetGroup, LeaveParty, GetParty), of what C_game::Tick calls (the fights' Tick,
// the following's Update) and of the group's ProcessEvents, which the groups' Tick calls. The copies of the implicit
// functions they call (ai_group's operator= and destructor, the vectors' helpers) are compiler-generated.

#include "ai/jp_ai_groups.h"

#include "actors/C_player.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "vehicles/C_car.h"
#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x5310d0
ai_group *ai_groups::GetGroup(uint id)
{
    StubLog("ai_groups::GetGroup");
    return NULL;
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x531130
void ai_groups::Init()
{
    unknown_0 = -1;
    unknown_4 = 0;
    groups.clear();
    free_actors.clear();
}

// FUNCTION: GAME 0x5311a0
void ai_groups::Destroy()
{
    unknown_0 = -1;
    unknown_4 = 0;
    free_actors.clear();
    for (int i = groups.size() - 1; i >= 0; i--) {
        groups[i].members.clear();
        groups[i].unknown_20.clear();
        groups[i].events.clear();
        groups[i].unknown_40.clear();
        groups[i].unknown_50.clear();
    }
    groups.clear();
}

// The player's controls unlocked once the game's time passes unknown_0, the groups' events, then the actors the
// groups let go, from the last: one farther than 150 from the camera is put away, a car hidden and without its dynamic
// collisions first (PS2's the same)
// FUNCTION: GAME 0x531340
void ai_groups::Tick()
{
    uint time = act_mission->game->game_time;
    if (unknown_0 != -1 && unknown_0 < time)
    {
        unknown_0 = -1;
        ((C_player *)act_mission->game->player)->LockControls(false);
    }
    for (uint i = 0; i < groups.size(); i++)
        groups[i].ProcessEvents(time);
    S_vector cam = act_mission->GetScene()->GetActiveCamera()->GetWorldPos();
    for (int j = free_actors.size() - 1; j >= 0; j--)
    {
        S_vector d = free_actors[j]->frame->GetWorldPos() - cam;
        if (d.Square() > 150.0f * 150.0f)
        {
            C_actor *act = free_actors[j];
            if (act->type == ACTOR_CAR)
            {
                act->frame->SetOn(false);
                ((C_car *)act)->SetDynamicCollsOn(false, ((C_car *)act)->unknown_1fac);
                act->Update(1);
            }
            free_actors[j]->SetActState(ACTSTATE_2);
            free_actors.erase(free_actors.begin() + j);
        }
    }
}

#pragma auto_inline(off)

// STUB: GAME 0x530920
void ai_group::ProcessEvents(uint time)
{
    StubLog("ai_group::ProcessEvents");
}

// STUB: GAME 0x5325a0
void ai_follow_manager::Update()
{
    StubLog("ai_follow_manager::Update");
}

#pragma auto_inline(on)

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

#pragma auto_inline(off)

// STUB: GAME 0x533c50
void ai_fight_manager::LeaveParty(C_entity *e, C_actor *act)
{
    StubLog("ai_fight_manager::LeaveParty");
}

// STUB: GAME 0x534db0
ai_fight_followship *ai_fight_manager::GetParty(C_entity *e, C_actor *act, uint *unknown)
{
    StubLog("ai_fight_manager::GetParty");
    return NULL;
}

#pragma auto_inline(on)

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

#pragma auto_inline(off)

// STUB: GAME 0x5350b0
void ai_fight_manager::Tick()
{
    StubLog("ai_fight_manager::Tick");
}

#pragma auto_inline(on)
