// The police (PS2 C_shvestky.cpp; the PC's file is the one at 0x499b40-0x4a031c, docs/game-files.md), which starts with
// the constructor and the destructor. Written so far: those two, which the game's constructor and destructor call,
// GameInit and GameDone, which C_game's Init and Done call, and Tick, which C_game::Tick calls, with PS2's TestOverRed
// and CreateShvestkyTest, which the PC inlines in it; the records' destructor, what Tick calls of the file's
// (TickDoRiciActors, DobreMiPokuril_MuzeJitDom, VasekSelChlastatNaOslavu, the road blocks' CreateRoadBlock) and what
// the script's commands call are stubs.

#include "ai/C_shvestky.h"
#include "actors/C_actor.h"
#include "actors/C_human.h"
#include "ai/C_person_traffic.h"
#include "ai/jp_ai_entity.h"
#include "ai/jp_ai_main.h"
#include "ai/road_web.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "ui/g_indicators.h"
#include "ui/g_m_func.h"
#include "vehicles/C_car.h"
#include "vehicles/C_trf_c.h"

#include "stub.h"

#include <stdio.h>

// FUNCTION: GAME 0x499b40
C_shvestky::C_shvestky()
{
}

// FUNCTION: GAME 0x499c00
C_shvestky::~C_shvestky()
{
}

// PS2's order, with the PC's values for unknown_c and unknown_10. The periods are milliseconds most likely; the timers
// are set through S_timer::Reset, whose inline calls leave the first clear's _Destroy out of line, as the original.
// FUNCTION: GAME 0x499cd0
void C_shvestky::GameInit()
{
    unknown_0 = false;
    unknown_4.Reset(330);
    unknown_c = 17.0f;
    unknown_10 = 28.0f;
    unknown_3c.clear();
    unknown_1c.Reset(217);
    unknown_24.Reset(4013);
    unknown_2c = 0;
    unknown_30 = 0;
    road_block_cars.clear();
    road_block_irises.clear();
    unknown_14.Reset(913);
    unknown_7c.clear();
}

// The police's records deleted, the next mission's actors kept out of its lists, its temporary cars taken from the game
// and the radar, the road blocks' cars, policemen and enemies ended and released but those the next mission keeps
// (PS2's order)
// FUNCTION: GAME 0x499dc0
void C_shvestky::GameDone()
{
    int i, j;
    for (i = unknown_3c.size(); i--;)
        delete unknown_3c[i];
    unknown_3c.clear();
    for (j = 0; j < 2; j++)
    {
        C_actor *act = NewMissionActors[j];
        if (!act)
            continue;
        for (i = unknown_7c.size(); i--;)
        {
            if (unknown_7c[i] == act)
            {
                unknown_7c[i] = unknown_7c.back();
                unknown_7c.pop_back();
                break;
            }
        }
    }
    for (i = unknown_8c.size(); i--;)
    {
        if (NewMissionActors[0] == unknown_8c[i])
            act_mission->game->InternTempActDel(unknown_8c[i]);
        else
        {
            act_mission->game->InvalidateActor(unknown_8c[i], false);
            mafiaIndicators.RadarRemoveCar(unknown_8c[i]);
            act_mission->game->RemoveTemporaryActor(unknown_8c[i]);
        }
    }
    unknown_8c.clear();
    TickDoRiciActors(true);
    for (i = road_block_cars.size(); i--;)
    {
        for (j = C_actor::global_actors.size(); j--;)
        {
            if (C_actor::global_actors[j]->type == ACTOR_TRAFFIC_CAR)
                ((C_traffic_car *)C_actor::global_actors[j])->VypraskeeeKaaaru(road_block_cars[i].car);
        }
        if (road_block_cars[i].car)
        {
            char name[32];
            // STRING: GAME 0x634974
            sprintf(name, "road_bl_C_%i", i);
            road_block_cars[i].car->frame->SetName(name);
        }
        if (!road_block_cars[i].unknown_4)
            act_mission->game->InternTempActDel(road_block_cars[i].car);
        if (NewMissionActors[0] != road_block_cars[i].car && NewMissionActors[1] != road_block_cars[i].car &&
            !act_mission->FrmActBuff_StoreActor(road_block_cars[i].car, false))
        {
            road_block_cars[i].car->GameDone();
            road_block_cars[i].car->Release();
        }
    }
    road_block_cars.clear();
    for (i = road_block_irises.size(); i--;)
    {
        if (!road_block_irises[i].unknown_4)
            act_mission->game->InternTempActDel(road_block_irises[i].actor);
        road_block_irises[i].actor->GameDone();
        road_block_irises[i].actor->Release();
    }
    road_block_irises.clear();
    for (i = road_block_enemies.size(); i--;)
    {
        if (!road_block_enemies[i].unknown_4)
            act_mission->game->InternTempActDel(road_block_enemies[i].actor);
        if (!act_mission->FrmActBuff_StoreActor(road_block_enemies[i].actor, false))
        {
            road_block_enemies[i].actor->GameDone();
            road_block_enemies[i].actor->Release();
        }
    }
    road_block_enemies.clear();
}

#pragma auto_inline(off)

// STUB: GAME 0x49a120
C_shvestky::S_shvestky::~S_shvestky()
{
    StubLog("C_shvestky::S_shvestky::~S_shvestky");
}

#pragma auto_inline(on)

// The semaphores' lights in each of their ten phases as numbers, the table game.cpp has as bitfields (red for five,
// green for four, then orange): TestOverRed's test of a red light (the name is ours)
#define SEMAPHORE_RED 1
#define SEMAPHORE_ORANGE 2
#define SEMAPHORE_GREEN 4
// GLOBAL: GAME 0x624b60
static const int semaphore_colors[10] = {
    SEMAPHORE_RED, SEMAPHORE_RED, SEMAPHORE_RED, SEMAPHORE_RED, SEMAPHORE_RED,
    SEMAPHORE_GREEN, SEMAPHORE_GREEN, SEMAPHORE_GREEN, SEMAPHORE_GREEN,
    SEMAPHORE_ORANGE,
};

// The police by the time passed (PS2's order): the actors in their faces at unknown_14's periods, at most five
// temporary cars, the failure's menu once the player is in the cooler (as C_game::Tick's); then, while the manager is
// on, the red lights at unknown_1c's periods (TestOverRed) and the records at unknown_4's: their times, the road blocks
// (PC only) and their states (CreateShvestkyTest); last the player's bar on the HUD. The members are read through
// their classes' accessors: as inline calls still to come, they leave the nested calls the original calls out of line
// there (the temporary cars' std::copy, the angry ones' erase in pop_back, the sizes in TestOverRed and
// CreateShvestkyTest).
// FUNCTION: GAME 0x49a300
void C_shvestky::Tick(uint time)
{
    if (unknown_14.Tick(time))
        TickDoRiciActors(false);
    while (unknown_8c.size() > 5)
    {
        act_mission->GetGame()->InvalidateActor(unknown_8c[0], false);
        mafiaIndicators.RadarRemoveCar(unknown_8c[0]);
        act_mission->GetGame()->RemoveTemporaryActor(unknown_8c[0]);
        unknown_8c.erase(unknown_8c.begin());
    }
    // The player in the cooler: once the screen has faded out, the failure's menu, as C_game::Tick's
    if (AIManager.police.PlukovnikuMasVChladkuPlajeraFrafera() && mafiaIndicators.FadeInOutGetState())
    {
        if (mafiaIndicators.FadeInOutIsDark())
        {
            act_mission->GetGame()->StopAllSounds();
            switch (GM_Menu::ExecuteMenu(new GM_GameFailed(unknown_34), false, true))
            {
            case GAMEFAILED_RESULT_EXIT:
                act_mission->GetGame()->unknown_48 = 1;
                break;
            case GAMEFAILED_RESULT_4FC:
                act_mission->GetGame()->unknown_48 = 2;
                break;
            case MAINMENU_LOAD_GAME:
            case GAMEFAILED_RESULT_RETRY:
                act_mission->GetGame()->unknown_48 = 3;
                break;
            }
            mafiaIndicators.FadeInOut(false, 0, 0);
        }
        else
            mafiaIndicators.FadeInOut(true, 1000, 0);
    }
    if (!unknown_0)
        return;
    if (act_mission->GetGame()->GetUnknown2f14Bit6())
        return;
    int i;
    if (act_mission->GetGame()->GetUnknown2b14() < 10)
    {
        for (i = unknown_3c.size(); i--;)
        {
            unknown_3c[i]->flags &= SHVF_UNKNOWN_2000 | SHVF_UNKNOWN_4000;
            unknown_3c[i]->unknown_18 = 0;
            unknown_3c[i]->flags2 = 0;
            unknown_3c[i]->crash_time = 0;
        }
        return;
    }
    if (AIManager.police.PanoveHrajeteSiNaPoldy())
    {
        DobreMiPokuril_MuzeJitDom(act_mission->GetGame()->GetPlayer(), false);
        return;
    }
    if (unknown_1c.Tick(time))
        TestOverRed();
    if (unknown_4.Tick(time))
    {
        // The records' times
        for (i = unknown_3c.size(); i--;)
        {
            int item = unknown_3c[i]->human->inventory.GetItemInRightHand(NULL);
            if (item >= SHVITEM_FIRST && item <= SHVITEM_LAST)
            {
                unknown_3c[i]->flags |= SHVF_WEAPON;
                if (!unknown_3c[i]->human->car)
                    C_traffic_generator::PlayerHoMaVytaseneho();
            }
            else
                unknown_3c[i]->flags &= ~SHVF_WEAPON;
            if (unknown_3c[i]->car && unknown_3c[i]->car->GetSpeed() > unknown_c)
                unknown_3c[i]->flags |= SHVF_SPEEDING;
            else
                unknown_3c[i]->flags &= ~SHVF_SPEEDING;
            if (unknown_3c[i]->unknown_18 > 0)
            {
                unknown_3c[i]->unknown_18 -= unknown_4.period;
                if (unknown_3c[i]->unknown_18 <= 0)
                {
                    unknown_3c[i]->unknown_18 = 0;
                    unknown_3c[i]->flags &= ~SHVF_UNKNOWN_38;
                }
            }
            if (unknown_3c[i]->crash_time > 0)
            {
                unknown_3c[i]->crash_time -= unknown_4.period;
                if (unknown_3c[i]->crash_time <= 0)
                {
                    unknown_3c[i]->crash_time = 0;
                    unknown_3c[i]->flags2 &= ~SHVF2_UNKNOWN_38;
                }
            }
            if (unknown_3c[i]->flags & SHVF_UNKNOWN_40)
            {
                if (unknown_3c[i]->car && unknown_3c[i]->car->GetSpeed() > 11.0f)
                {
                    unknown_3c[i]->unknown_14 += unknown_4.period;
                    if (!unknown_3c[i]->car->unknown_2170)
                        unknown_3c[i]->flags &= ~SHVF_UNKNOWN_40;
                    if (unknown_3c[i]->unknown_14 > 5000)
                        unknown_3c[i]->unknown_14 = 5000;
                }
                else
                {
                    unknown_3c[i]->flags &= ~SHVF_UNKNOWN_40;
                    unknown_3c[i]->unknown_14 = 0;
                }
            }
            if (unknown_3c[i]->unknown_20 > 0)
            {
                unknown_3c[i]->unknown_20 -= unknown_4.period;
                if (unknown_3c[i]->unknown_20 <= 0)
                {
                    unknown_3c[i]->unknown_20 = 0;
                    unknown_3c[i]->flags &= ~SHVF_UNKNOWN_200;
                }
            }
            if (unknown_3c[i]->unknown_24 > 0)
            {
                unknown_3c[i]->unknown_24 -= unknown_4.period;
                if (unknown_3c[i]->unknown_24 <= 0)
                {
                    unknown_3c[i]->unknown_24 = 0;
                    unknown_3c[i]->flags &= ~(SHVF_RED_LIGHT | SHVF_UNKNOWN_1000);
                }
            }
            if (unknown_3c[i]->unknown_c > 0)
            {
                unknown_3c[i]->unknown_c -= unknown_4.period;
                if (unknown_3c[i]->unknown_c <= 0)
                {
                    unknown_3c[i]->unknown_c = 0;
                    unknown_3c[i]->flags &= ~SHVF_UNKNOWN_800;
                }
            }
            if (unknown_3c[i]->fire_time > 0)
            {
                unknown_3c[i]->fire_time -= unknown_4.period;
                if (unknown_3c[i]->fire_time <= 0)
                {
                    unknown_3c[i]->fire_time = 0;
                    unknown_3c[i]->flags &= ~SHVF_FIRE;
                }
            }
            // The dead out of the angry ones
            if (unknown_3c[i]->nasrany.size())
            {
                for (uint j = 0; j < unknown_3c[i]->nasrany.size();)
                {
                    if (unknown_3c[i]->nasrany[j]->GetUnknown5e())
                    {
                        unknown_3c[i]->nasrany[j] = unknown_3c[i]->nasrany.back();
                        unknown_3c[i]->nasrany.pop_back();
                    }
                    else
                        j++;
                }
            }
            if (unknown_3c[i]->unknown_40 > 0)
            {
                unknown_3c[i]->unknown_40 -= unknown_4.period;
                if (unknown_3c[i]->unknown_40 < 0)
                    unknown_3c[i]->unknown_4c = 0;
            }
            if (unknown_3c[i]->state > SHVSTATE_NONE)
            {
                if (!unknown_3c[i]->nasrany.size())
                {
                    unknown_3c[i]->unknown_44 -= unknown_4.period;
                    if (!unknown_3c[i]->unknown_50)
                        unknown_3c[i]->unknown_44 = -1;
                    if (unknown_3c[i]->unknown_44 < 0)
                        DobreMiPokuril_MuzeJitDom(unknown_3c[i]->human, true);
                    else
                    {
                        if (unknown_3c[i]->fire_time > 2000)
                            unknown_3c[i]->fire_time = 2000;
                        if (unknown_3c[i]->unknown_18 > 2000)
                            unknown_3c[i]->unknown_18 = 2000;
                        if (unknown_3c[i]->unknown_20 > 2000)
                            unknown_3c[i]->unknown_20 = 2000;
                    }
                }
                else if (unknown_3c[i]->unknown_50)
                {
                    switch (unknown_3c[i]->state)
                    {
                    case SHVSTATE_1:
                        unknown_3c[i]->unknown_44 = 20000;
                        break;
                    case SHVSTATE_ARREST:
                        unknown_3c[i]->unknown_44 = 40000;
                        break;
                    case SHVSTATE_KILL:
                        unknown_3c[i]->unknown_44 = 60000;
                        break;
                    }
                    unknown_3c[i]->unknown_48 = unknown_3c[i]->unknown_44;
                    if (unknown_3c[i]->human == act_mission->GetGame()->GetPlayer() && unknown_3c[i]->unknown_50)
                        mafiaIndicators.SetUnknown46e8(1.0f);
                }
            }
            if (unknown_3c[i]->human == act_mission->GetGame()->GetPlayer())
            {
                if (unknown_3c[i]->state && unknown_3c[i]->unknown_50)
                    mafiaIndicators.AddFlags(INDF_UNKNOWN_10);
                else
                    mafiaIndicators.RemoveFlags(INDF_UNKNOWN_10);
            }
        }
        // The road blocks (PC only): one comes up for the first record whose police car comes near enough to it, and
        // goes once none is near, its actors handed to TickDoRiciActors
        std::vector<C_game::S_RoadBlock> &road_blocks = act_mission->GetGame()->road_blocks;
        for (int j = road_blocks.size(); j--;)
        {
            C_human *human = NULL;
            bool near_block = false;
            for (i = unknown_3c.size(); i--;)
            {
                if (unknown_3c[i]->car && unknown_3c[i]->unknown_2c && !unknown_3c[i]->nevi)
                {
                    S_vector d = unknown_3c[i]->car->GetFrame()->GetWorldPos() -
                        act_mission->roads->links[road_blocks[j].node].pos;
                    float dist = d.Square();
                    if (!road_blocks[j].unknown_2 && dist < 150.0f * 150.0f)
                        continue;
                    if (dist < 200.0f * 200.0f)
                    {
                        near_block = true;
                        human = unknown_3c[i]->human;
                        break;
                    }
                }
            }
            if (road_blocks[j].unknown_2 && !near_block)
            {
                road_blocks[j].unknown_2 = false;
                for (int k = road_blocks[j].unknown_4.size(); k--;)
                {
                    C_actor *act = (C_actor *)road_blocks[j].unknown_4[k];
                    if (act->GetType() == ACTOR_ENTITY)
                        ((C_entity *)act)->Event(NULL, ENTITY_EVENT_UNKNOWN_13, NULL);
                    AddDoRiciActor((C_actor *)road_blocks[j].unknown_4[k]);
                }
                road_blocks[j].unknown_4.clear();
            }
            else if (!road_blocks[j].unknown_2 && near_block)
            {
                road_blocks[j].unknown_2 = true;
                CreateRoadBlock(j, human);
            }
        }
        CreateShvestkyTest();
    }
    // The player's record: the HUD's bar of the time left
    for (i = unknown_3c.size(); i--;)
    {
        if (unknown_3c[i]->human == act_mission->GetGame()->GetPlayer())
        {
            if (unknown_3c[i]->unknown_50)
            {
                if (unknown_3c[i]->unknown_48 > 0 && unknown_3c[i]->unknown_44 < unknown_3c[i]->unknown_48)
                {
                    int left = unknown_3c[i]->unknown_44 - unknown_4.time;
                    if (left > unknown_3c[i]->unknown_44)
                        left = unknown_3c[i]->unknown_44;
                    int total = unknown_3c[i]->unknown_48 - unknown_4.period;
                    if (total < 1)
                        total = 1;
                    float f = (float)left / total;
                    if (f > 1.0f)
                        f = 1.0f;
                    else if (f < 0.0f)
                        f = 0.0f;
                    mafiaIndicators.SetUnknown46e8(f);
                }
                else
                    mafiaIndicators.SetUnknown46e8(1.0f);
            }
            break;
        }
    }
}

// The police cars' drivers who run a red light (PS2 name; PS2 has it out of line, the PC's Tick inlines it): of the two
// ends of the link a car is on, the one ahead of it, a semaphore's; a red light in the link's direction where the car,
// within 15 of the line along the link, crossed the line across it 1.5 from the link's position towards that end
// between its last position and this one. The lines come from the first end's position even after the swap, and p1
// takes the link's position, as PS2's (the same slot in both builds).
inline void C_shvestky::TestOverRed()
{
    for (int i = unknown_3c.size(); i--;)
    {
        if (unknown_3c[i]->car)
        {
            S_vector pos = unknown_3c[i]->car->GetWorldPos();
            S_vector dir = unknown_3c[i]->car->GetVelocity();
            ushort link = act_mission->roads->GetNearNode(unknown_3c[i]->car->GetFrame()->GetWorldPos());
            if (link != NEARNODE_NONE && !dir.IsNull())
            {
                dir.Normalize();
                ushort ends[2];
                ends[0] = act_mission->roads->links[link].unknown_10;
                ends[1] = act_mission->roads->links[link].unknown_12;
                if (ends[0] != NEARNODE_NONE && ends[1] != NEARNODE_NONE)
                {
                    S_vector p0 = act_mission->roads->GetPos(ends[0]);
                    S_vector p1 = act_mission->roads->GetPos(ends[1]);
                    if (dir.AngleTo(p0 - pos) > dir.AngleTo(p1 - pos))
                    {
                        ushort t = ends[0];
                        ends[0] = ends[1];
                        ends[1] = t;
                    }
                    bool ok = !(ends[0] & NEARNODE_FLAG) && act_mission->roads->nodes[ends[0]].unknown_c;
                    if (ok)
                    {
                        for (int j = unknown_9c.size(); j--;)
                        {
                            if ((unknown_9c[j] & NEARNODE_INDEX) == (ends[0] & NEARNODE_INDEX))
                            {
                                ok = false;
                                break;
                            }
                        }
                    }
                    if (ok)
                    {
                        p1 = act_mission->roads->links[link].pos;
                        S_vector2 d(p0.x - p1.x, p0.z - p1.z);
                        S_vector2 n(d.y, -d.x);
                        S_vector2 line_pos(p1.x, p1.z);
                        S_vector2 car_pos(pos.x, pos.z);
                        // unused, as PS2's
                        S_vector2 moved = unknown_3c[i]->pos - car_pos;
                        S_vector2 dn = d;
                        dn.Normalize();
                        line_pos += dn * 1.5f;
                        if ((float)fabs(car_pos.DistanceToLine(line_pos, d)) < 15.0f)
                        {
                            float side = car_pos.DistanceToLine(line_pos, n);
                            if (unknown_3c[i]->pos.DistanceToLine(line_pos, n) * side <= 0.0f)
                            {
                                int *phase = (float)fabs(d.x) > (float)fabs(d.y) ?
                                    &act_mission->GetGame()->unknown_19c : &act_mission->GetGame()->unknown_1a0;
                                if (phase && semaphore_colors[*phase] == SEMAPHORE_RED)
                                {
                                    unknown_3c[i]->flags |= SHVF_RED_LIGHT;
                                    unknown_3c[i]->unknown_24 += 4000;
                                    if (unknown_3c[i]->unknown_24 > 10000)
                                        unknown_3c[i]->unknown_24 = 10000;
                                    if (unknown_3c[i]->new_state < SHVSTATE_1)
                                    {
                                        unknown_3c[i]->new_state = SHVSTATE_1;
                                        unknown_3c[i]->reason = SHVREASON_RED_LIGHT;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            unknown_3c[i]->pos = S_vector2(pos.x, pos.z);
        }
    }
}

// The records' new states from their flags (PS2 name; PS2 has it out of line, the PC's Tick inlines it), the police
// told of a change, and the player's on the HUD with more policemen at its states 5 and 6
inline void C_shvestky::CreateShvestkyTest()
{
    for (int i = unknown_3c.size(); i--;)
    {
        if ((unknown_3c[i]->flags & SHVF_SPEEDING) && unknown_3c[i]->car)
        {
            if (unknown_3c[i]->car->GetSpeed() > unknown_10)
            {
                if (unknown_3c[i]->new_state < SHVSTATE_ARREST)
                {
                    unknown_3c[i]->new_state = SHVSTATE_ARREST;
                    unknown_3c[i]->reason = SHVREASON_SPEEDING;
                }
            }
            else
            {
                if (unknown_3c[i]->new_state < SHVSTATE_1)
                {
                    unknown_3c[i]->new_state = SHVSTATE_1;
                    unknown_3c[i]->reason = SHVREASON_SPEEDING;
                }
            }
        }
        if (unknown_3c[i]->flags & SHVF_UNKNOWN_4000)
        {
            if (unknown_3c[i]->new_state < SHVSTATE_ARREST)
                unknown_3c[i]->new_state = SHVSTATE_ARREST;
        }
        if (unknown_3c[i]->flags & SHVF_UNKNOWN_2000)
        {
            if (unknown_3c[i]->new_state < SHVSTATE_KILL)
            {
                unknown_3c[i]->new_state = SHVSTATE_KILL;
                unknown_3c[i]->reason = SHVREASON_UNKNOWN_9;
            }
        }
        if (unknown_3c[i]->flags & SHVF_UNKNOWN_8000)
        {
            if (unknown_3c[i]->unknown_6c)
            {
                if (unknown_3c[i]->new_state < SHVSTATE_ARREST)
                {
                    unknown_3c[i]->new_state = SHVSTATE_ARREST;
                    unknown_3c[i]->reason = SHVREASON_UNKNOWN_A;
                }
            }
            else
            {
                if (unknown_3c[i]->new_state < SHVSTATE_1)
                {
                    unknown_3c[i]->new_state = SHVSTATE_1;
                    unknown_3c[i]->reason = SHVREASON_UNKNOWN_A;
                }
            }
        }
        if (unknown_3c[i]->new_state < SHVSTATE_1 && (unknown_3c[i]->flags & SHVF_WEAPON))
        {
            if (unknown_3c[i]->human->car && unknown_3c[i]->human->car->GetType() == ACTOR_CAR)
            {
                if (unknown_3c[i]->human->unknown_1e5)
                {
                    int item = unknown_3c[i]->human->inventory.GetItemInRightHand(NULL);
                    if (unknown_3c[i]->human->seat)
                    {
                        if (item >= SHVITEM_CAR_FIRST && item <= SHVITEM_CAR_LAST)
                        {
                            unknown_3c[i]->new_state = SHVSTATE_ARREST;
                            unknown_3c[i]->reason = SHVREASON_WEAPON;
                        }
                    }
                    else
                    {
                        if (item >= SHVITEM_CAR_FIRST && item <= SHVITEM_DRIVER_LAST)
                        {
                            unknown_3c[i]->new_state = SHVSTATE_ARREST;
                            unknown_3c[i]->reason = SHVREASON_WEAPON;
                        }
                    }
                }
            }
            else if (!unknown_3c[i]->human->car)
            {
                unknown_3c[i]->new_state = SHVSTATE_ARREST;
                unknown_3c[i]->reason = SHVREASON_WEAPON;
            }
        }
        if ((unknown_3c[i]->flags & SHVF_UNKNOWN_800) && unknown_3c[i]->new_state < SHVSTATE_ARREST)
        {
            unknown_3c[i]->new_state = SHVSTATE_ARREST;
            unknown_3c[i]->reason = SHVREASON_UNKNOWN_7;
        }
        if ((unknown_3c[i]->flags & SHVF_UNKNOWN_40) && unknown_3c[i]->unknown_14 > 5800 &&
            unknown_3c[i]->new_state < SHVSTATE_1)
        {
            unknown_3c[i]->new_state = SHVSTATE_1;
            unknown_3c[i]->reason = SHVREASON_UNKNOWN_4;
        }
        if (unknown_3c[i]->new_state == SHVSTATE_1 && unknown_3c[i]->state < SHVSTATE_ARREST && unknown_3c[i]->car &&
            unknown_3c[i]->car->unknown_20b2)
            unknown_3c[i]->new_state = SHVSTATE_NONE;
        if (unknown_3c[i]->new_state == SHVSTATE_1 && unknown_3c[i]->unknown_28 >= 3)
            unknown_3c[i]->new_state = SHVSTATE_ARREST;
        if (unknown_3c[i]->new_state > unknown_3c[i]->state)
        {
            bool seen = true;
            if (unknown_3c[i]->nevi)
                seen = AIManager.police.PanoveMateOciNaZmrdoviNeboCumitePoBabach(unknown_3c[i]->human);
            if (unknown_3c[i]->nasrany.size())
            {
                if (seen)
                {
                    switch (unknown_3c[i]->new_state)
                    {
                    case SHVSTATE_ARREST:
                        AIManager.police.PanoveZmenaZatkneteHoZmrda(unknown_3c[i]->human);
                        break;
                    case SHVSTATE_KILL:
                        AIManager.police.PanoveZmenaZabijteHoZmrda(unknown_3c[i]->human);
                        break;
                    }
                    unknown_3c[i]->state = unknown_3c[i]->new_state;
                }
            }
            else
                unknown_3c[i]->state = unknown_3c[i]->new_state;
        }
        else if (unknown_3c[i]->new_state && unknown_3c[i]->nasrany.size() && unknown_3c[i]->nevi &&
            AIManager.police.PanoveMateOciNaZmrdoviNeboCumitePoBabach(unknown_3c[i]->human))
            AIManager.police.PanoveVytahneteBlokyAZapisteSiZmrda(unknown_3c[i]->human);
        bool gone = !unknown_3c[i]->nasrany.size() && unknown_3c[i]->unknown_44 < 0;
        int time_left = unknown_3c[i]->unknown_44;
        uint count = unknown_3c[i]->nasrany.size();
        if (unknown_3c[i]->unknown_50 && unknown_3c[i]->nevi)
        {
            if (unknown_3c[i]->new_state > SHVSTATE_NONE && unknown_3c[i]->state > SHVSTATE_NONE)
                VasekSelChlastatNaOslavu(unknown_3c[i]->human, unknown_3c[i]->human->GetWorldPos(), 150.0f, false,
                    unknown_3c[i]->new_state < SHVSTATE_1);
        }
        else if (unknown_3c[i]->unknown_50 ? (bool)(unknown_3c[i]->state > SHVSTATE_NONE) :
            (bool)(unknown_3c[i]->new_state > SHVSTATE_NONE))
            VasekSelChlastatNaOslavu(unknown_3c[i]->human, unknown_3c[i]->human->GetWorldPos(), 150.0f, false, false);
        if (gone && !unknown_3c[i]->nasrany.size())
        {
            unknown_3c[i]->state = SHVSTATE_NONE;
            unknown_3c[i]->unknown_44 = -1;
        }
        if (count == unknown_3c[i]->nasrany.size())
            unknown_3c[i]->unknown_44 = time_left;
        else
            mafiaIndicators.SetUnknown46e8(1.0f);
        if (unknown_3c[i]->human == act_mission->GetGame()->GetPlayer())
        {
            ushort flags = unknown_3c[i]->flags;
            int level = unknown_3c[i]->state;
            if ((flags & SHVF_UNKNOWN_4000) && unknown_3c[i]->unknown_50 && level < SHVSTATE_ARREST)
                level = SHVSTATE_ARREST;
            if ((flags & SHVF_UNKNOWN_2000) && level < SHVSTATE_KILL)
                level = SHVSTATE_KILL;
            if (level > SHVSTATE_NONE)
            {
                if (unknown_3c[i]->unknown_50)
                    level += 3;
                mafiaIndicators.AddFlags(INDF_UNKNOWN_4000000);
                float alpha = 1.0f;
                if (PocetNasranychZivych(unknown_3c[i]->human))
                {
                    if (AIManager.police.PanoveJdeteNekdoPoZmrdovi(unknown_3c[i]->human))
                        unknown_3c[i]->nevi = !AIManager.police.PanoveViditeZmrdaNeboSteTupci(unknown_3c[i]->human);
                    else
                        unknown_3c[i]->nevi = false;
                }
                else if (unknown_3c[i]->human->car)
                    unknown_3c[i]->nevi = unknown_3c[i]->unknown_54 != unknown_3c[i]->human->car;
                else
                    unknown_3c[i]->nevi = !unknown_3c[i]->unknown_58;
                int symbol = level;
                if (level > 3)
                    symbol = level - 3;
                if (unknown_3c[i]->nevi && !unknown_38)
                    symbol = 4;
                if (!AIManager.police.PanoveMateOciNaZmrdoviNeboCumitePoBabach(unknown_3c[i]->human))
                    alpha = .4f;
                mafiaIndicators.SetSymbol(symbol, alpha);
                if (level == 5)
                {
                    if (unknown_3c[i]->unknown_5c < 1)
                    {
                        unknown_3c[i]->unknown_5c = 1;
                        act_mission->GetGame()->GetShvestky()->DalsiChalpiDoPartie(unknown_3c[i]->human, 4, 200.0f);
                    }
                }
                else if (level == 6)
                {
                    if (unknown_3c[i]->unknown_5c < 2)
                    {
                        unknown_3c[i]->unknown_5c = 2;
                        act_mission->GetGame()->GetShvestky()->DalsiChalpiDoPartie(unknown_3c[i]->human, 0, 200.0f);
                        act_mission->GetGame()->GetShvestky()->DalsiChalpiDoPartie(unknown_3c[i]->human, 0, 200.0f);
                    }
                }
            }
            else
                mafiaIndicators.RemoveFlags(INDF_UNKNOWN_4000000);
        }
        unknown_3c[i]->new_state = SHVSTATE_NONE;
    }
}

#pragma auto_inline(off)

// STUB: GAME 0x49b860
void C_shvestky::SetActive(bool active, int unknown, bool unknown2)
{
    StubLog("C_shvestky::SetActive");
}

// STUB: GAME 0x49c7e0
void C_shvestky::AddPerson(C_human *human)
{
    StubLog("C_shvestky::AddPerson");
}

// STUB: GAME 0x49ca50
void C_shvestky::DelPerson(C_human *human)
{
    StubLog("C_shvestky::DelPerson");
}

// STUB: GAME 0x49cbb0
void C_shvestky::DriveCar(C_human *human, C_car *car)
{
    StubLog("C_shvestky::DriveCar");
}

// STUB: GAME 0x49d130
void C_shvestky::ForceArrest(C_human *human, bool a, bool b)
{
    StubLog("C_shvestky::ForceArrest");
}

// STUB: GAME 0x49d340
uint C_shvestky::GetHumanStatus(C_human *human)
{
    StubLog("C_shvestky::GetHumanStatus");
    return 0;
}

// STUB: GAME 0x49d390
int C_shvestky::JakPicusNaTomJest(C_human *human)
{
    StubLog("C_shvestky::JakPicusNaTomJest");
    return 0;
}

// STUB: GAME 0x49d3e0
uint C_shvestky::GetCrashTime(C_human *human)
{
    StubLog("C_shvestky::GetCrashTime");
    return 0;
}

// STUB: GAME 0x49d420
uint C_shvestky::GetFireTime(C_human *human)
{
    StubLog("C_shvestky::GetFireTime");
    return 0;
}

// STUB: GAME 0x49d460
int C_shvestky::PocetNasranych(C_human *human)
{
    StubLog("C_shvestky::PocetNasranych");
    return 0;
}

// STUB: GAME 0x49d4b0
int C_shvestky::PocetNasranychZivych(C_human *human)
{
    StubLog("C_shvestky::PocetNasranychZivych");
    return 0;
}

// STUB: GAME 0x49d560
bool C_shvestky::NeviDoKterePiceVlezl(C_human *human)
{
    StubLog("C_shvestky::NeviDoKterePiceVlezl");
    return false;
}

// STUB: GAME 0x49d980
void C_shvestky::CreateRoadBlock(int index, C_human *human)
{
    StubLog("C_shvestky::CreateRoadBlock");
}

// STUB: GAME 0x49e820
void C_shvestky::AddDoRiciActor(C_actor *act)
{
    StubLog("C_shvestky::AddDoRiciActor");
}

// STUB: GAME 0x49e9e0
void C_shvestky::DelDoRiciActor(C_actor *act)
{
    StubLog("C_shvestky::DelDoRiciActor");
}

// STUB: GAME 0x49ea80
void C_shvestky::TickDoRiciActors(bool)
{
    StubLog("C_shvestky::TickDoRiciActors");
}

// STUB: GAME 0x49eee0
void C_shvestky::DalsiChalpiDoPartie(C_human *human, uint unknown, float unknown2)
{
    StubLog("C_shvestky::DalsiChalpiDoPartie");
}

// STUB: GAME 0x49f850
void C_shvestky::DobreMiPokuril_MuzeJitDom(C_human *human, bool unknown)
{
    StubLog("C_shvestky::DobreMiPokuril_MuzeJitDom");
}

// STUB: GAME 0x49f980
void C_shvestky::VasekSelChlastatNaOslavu(C_human *human, const S_vector &pos, float range, bool unknown,
    bool unknown2)
{
    StubLog("C_shvestky::VasekSelChlastatNaOslavu");
}

#pragma auto_inline(on)


