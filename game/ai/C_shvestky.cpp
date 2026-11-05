// The police (PS2 C_shvestky.cpp; the PC's file is the one at 0x499b40-0x4a031c, docs/game-files.md), which starts with
// the constructor and the destructor. Written so far: those two, which the game's constructor and destructor call,
// GameInit and GameDone, which C_game's Init and Done call, and Tick, which C_game::Tick calls, with PS2's TestOverRed
// and CreateShvestkyTest, which the PC inlines in it, and TickDoRiciActors, which both call; the records' destructor,
// what TickDoRiciActors calls of the file's (FreeMyTempActor, GlobalDelNasrany), what Tick calls of it
// (DobreMiPokuril_MuzeJitDom, VasekSelChlastatNaOslavu, the road blocks' CreateRoadBlock) and what the script's
// commands call are stubs.

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

// PS2's order, with the PC's values for speed_limit and speed_limit_arrest. The periods are milliseconds most likely;
// the timers are set through S_timer::Reset, whose inline calls leave the first clear's _Destroy out of line, as the
// original.
// FUNCTION: GAME 0x499cd0
void C_shvestky::GameInit()
{
    active = false;
    records_timer.Reset(330);
    speed_limit = 17.0f;
    speed_limit_arrest = 28.0f;
    records.clear();
    red_light_timer.Reset(217);
    unknown_24.Reset(4013);
    unknown_2c = 0;
    unknown_30 = 0;
    road_block_cars.clear();
    road_block_irises.clear();
    do_rici_timer.Reset(913);
    do_rici_actors.clear();
}

// The police's records deleted, the next mission's actors kept out of its lists, its temporary cars taken from the game
// and the radar, the road blocks' cars, policemen and enemies ended and released but those the next mission keeps
// (PS2's order)
// FUNCTION: GAME 0x499dc0
void C_shvestky::GameDone()
{
    int i, j;
    for (i = records.size(); i--;)
        delete records[i];
    records.clear();
    for (j = 0; j < 2; j++)
    {
        C_actor *act = NewMissionActors[j];
        if (!act)
            continue;
        for (i = do_rici_actors.size(); i--;)
        {
            if (do_rici_actors[i] == act)
            {
                do_rici_actors[i] = do_rici_actors.back();
                do_rici_actors.pop_back();
                break;
            }
        }
    }
    for (i = temp_cars.size(); i--;)
    {
        if (NewMissionActors[0] == temp_cars[i])
            act_mission->game->InternTempActDel(temp_cars[i]);
        else
        {
            act_mission->game->InvalidateActor(temp_cars[i], false);
            mafiaIndicators.RadarRemoveCar(temp_cars[i]);
            act_mission->game->RemoveTemporaryActor(temp_cars[i]);
        }
    }
    temp_cars.clear();
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
        if (!road_block_cars[i].free)
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
        if (!road_block_irises[i].free)
            act_mission->game->InternTempActDel(road_block_irises[i].actor);
        road_block_irises[i].actor->GameDone();
        road_block_irises[i].actor->Release();
    }
    road_block_irises.clear();
    for (i = road_block_enemies.size(); i--;)
    {
        if (!road_block_enemies[i].free)
            act_mission->game->InternTempActDel(road_block_enemies[i].actor);
        if (!act_mission->FrmActBuff_StoreActor(road_block_enemies[i].actor, false))
        {
            road_block_enemies[i].actor->GameDone();
            road_block_enemies[i].actor->Release();
        }
    }
    road_block_enemies.clear();
}

// STUB: GAME 0x49a120
C_shvestky::S_shvestky::~S_shvestky()
{
    StubLog("C_shvestky::S_shvestky::~S_shvestky");
}

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

// The police by the time passed (PS2's order): the actors gone to hell (do_rici_actors) at do_rici_timer's periods, at
// most five temporary cars, the failure's menu once the player is in the cooler (as C_game::Tick's); then, while the
// manager is on, the red lights at red_light_timer's periods (TestOverRed) and the records at records_timer's: their
// times, the road blocks (PC only) and their states (CreateShvestkyTest); last the player's bar on the HUD. The
// members are read through their classes' accessors: as inline calls still to come, they leave the nested calls the
// original calls out of line there (the temporary cars' std::copy, the angry ones' erase in pop_back, the sizes in
// TestOverRed and CreateShvestkyTest).
// FUNCTION: GAME 0x49a300
void C_shvestky::Tick(uint time)
{
    if (do_rici_timer.Tick(time))
        TickDoRiciActors(false);
    while (temp_cars.size() > 5)
    {
        act_mission->GetGame()->InvalidateActor(temp_cars[0], false);
        mafiaIndicators.RadarRemoveCar(temp_cars[0]);
        act_mission->GetGame()->RemoveTemporaryActor(temp_cars[0]);
        temp_cars.erase(temp_cars.begin());
    }
    // The player in the cooler: once the screen has faded out, the failure's menu, as C_game::Tick's
    if (AIManager.police.PlukovnikuMasVChladkuPlajeraFrafera() && mafiaIndicators.FadeInOutGetState())
    {
        if (mafiaIndicators.FadeInOutIsDark())
        {
            act_mission->GetGame()->StopAllSounds();
            switch (GM_Menu::ExecuteMenu(new GM_GameFailed(arrest_text), false, true))
            {
            case GAMEFAILED_RESULT_EXIT:
                act_mission->GetGame()->mission_end = 1;
                break;
            case GAMEFAILED_RESULT_4FC:
                act_mission->GetGame()->mission_end = 2;
                break;
            case MAINMENU_LOAD_GAME:
            case GAMEFAILED_RESULT_RETRY:
                act_mission->GetGame()->mission_end = 3;
                break;
            }
            mafiaIndicators.FadeInOut(false, 0, 0);
        }
        else
            mafiaIndicators.FadeInOut(true, 1000, 0);
    }
    if (!active)
        return;
    if (act_mission->GetGame()->GetCheatsBit6())
        return;
    int i;
    if (act_mission->GetGame()->GetInitFrames() < 10)
    {
        for (i = records.size(); i--;)
        {
            records[i]->flags &= SHVF_HIT_POLICEMAN | SHVF_FORCE_ARREST;
            records[i]->collision_time = 0;
            records[i]->flags2 = 0;
            records[i]->crash_time = 0;
        }
        return;
    }
    if (AIManager.police.PanoveHrajeteSiNaPoldy())
    {
        DobreMiPokuril_MuzeJitDom(act_mission->GetGame()->GetPlayer(), false);
        return;
    }
    if (red_light_timer.Tick(time))
        TestOverRed();
    if (records_timer.Tick(time))
    {
        // The records' times
        for (i = records.size(); i--;)
        {
            int item = records[i]->human->inventory.GetItemInRightHand(NULL);
            if (item >= SHVITEM_FIRST && item <= SHVITEM_LAST)
            {
                records[i]->flags |= SHVF_WEAPON;
                if (!records[i]->human->car)
                    C_traffic_generator::PlayerHoMaVytaseneho();
            }
            else
                records[i]->flags &= ~SHVF_WEAPON;
            if (records[i]->car && records[i]->car->GetSpeed() > speed_limit)
                records[i]->flags |= SHVF_SPEEDING;
            else
                records[i]->flags &= ~SHVF_SPEEDING;
            if (records[i]->collision_time > 0)
            {
                records[i]->collision_time -= records_timer.period;
                if (records[i]->collision_time <= 0)
                {
                    records[i]->collision_time = 0;
                    records[i]->flags &= ~SHVF_CRASH;
                }
            }
            if (records[i]->crash_time > 0)
            {
                records[i]->crash_time -= records_timer.period;
                if (records[i]->crash_time <= 0)
                {
                    records[i]->crash_time = 0;
                    records[i]->flags2 &= ~SHVF2_CRASH;
                }
            }
            if (records[i]->flags & SHVF_ILLEGAL_PATH)
            {
                if (records[i]->car && records[i]->car->GetSpeed() > 11.0f)
                {
                    records[i]->illegal_path_time += records_timer.period;
                    if (!records[i]->car->illegal_path)
                        records[i]->flags &= ~SHVF_ILLEGAL_PATH;
                    if (records[i]->illegal_path_time > 5000)
                        records[i]->illegal_path_time = 5000;
                }
                else
                {
                    records[i]->flags &= ~SHVF_ILLEGAL_PATH;
                    records[i]->illegal_path_time = 0;
                }
            }
            if (records[i]->boxing_time > 0)
            {
                records[i]->boxing_time -= records_timer.period;
                if (records[i]->boxing_time <= 0)
                {
                    records[i]->boxing_time = 0;
                    records[i]->flags &= ~SHVF_BOXING;
                }
            }
            if (records[i]->signal_time > 0)
            {
                records[i]->signal_time -= records_timer.period;
                if (records[i]->signal_time <= 0)
                {
                    records[i]->signal_time = 0;
                    records[i]->flags &= ~(SHVF_RED_LIGHT | SHVF_SIGNAL_MARK);
                }
            }
            if (records[i]->car_steal_time > 0)
            {
                records[i]->car_steal_time -= records_timer.period;
                if (records[i]->car_steal_time <= 0)
                {
                    records[i]->car_steal_time = 0;
                    records[i]->flags &= ~SHVF_CAR_STEAL;
                }
            }
            if (records[i]->fire_time > 0)
            {
                records[i]->fire_time -= records_timer.period;
                if (records[i]->fire_time <= 0)
                {
                    records[i]->fire_time = 0;
                    records[i]->flags &= ~SHVF_FIRE;
                }
            }
            // The dead out of the angry ones
            if (records[i]->nasrany.size())
            {
                for (uint j = 0; j < records[i]->nasrany.size();)
                {
                    if (records[i]->nasrany[j]->GetDead())
                    {
                        records[i]->nasrany[j] = records[i]->nasrany.back();
                        records[i]->nasrany.pop_back();
                    }
                    else
                        j++;
                }
            }
            if (records[i]->hits_time > 0)
            {
                records[i]->hits_time -= records_timer.period;
                if (records[i]->hits_time < 0)
                    records[i]->hits = 0;
            }
            if (records[i]->state > SHVSTATE_NONE)
            {
                if (!records[i]->nasrany.size())
                {
                    records[i]->wanted_time -= records_timer.period;
                    if (!records[i]->alerted)
                        records[i]->wanted_time = -1;
                    if (records[i]->wanted_time < 0)
                        DobreMiPokuril_MuzeJitDom(records[i]->human, true);
                    else
                    {
                        if (records[i]->fire_time > 2000)
                            records[i]->fire_time = 2000;
                        if (records[i]->collision_time > 2000)
                            records[i]->collision_time = 2000;
                        if (records[i]->boxing_time > 2000)
                            records[i]->boxing_time = 2000;
                    }
                }
                else if (records[i]->alerted)
                {
                    switch (records[i]->state)
                    {
                    case SHVSTATE_1:
                        records[i]->wanted_time = 20000;
                        break;
                    case SHVSTATE_ARREST:
                        records[i]->wanted_time = 40000;
                        break;
                    case SHVSTATE_KILL:
                        records[i]->wanted_time = 60000;
                        break;
                    }
                    records[i]->wanted_time_start = records[i]->wanted_time;
                    if (records[i]->human == act_mission->GetGame()->GetPlayer() && records[i]->alerted)
                        mafiaIndicators.SetProgress(1.0f);
                }
            }
            if (records[i]->human == act_mission->GetGame()->GetPlayer())
            {
                if (records[i]->state && records[i]->alerted)
                    mafiaIndicators.AddFlags(INDF_PROGRESS);
                else
                    mafiaIndicators.RemoveFlags(INDF_PROGRESS);
            }
        }
        // The road blocks (PC only): one comes up for the first record whose police car comes near enough to it, and
        // goes once none is near, its actors handed to TickDoRiciActors
        std::vector<C_game::S_RoadBlock> &road_blocks = act_mission->GetGame()->road_blocks;
        for (int j = road_blocks.size(); j--;)
        {
            C_human *human = NULL;
            bool near_block = false;
            for (i = records.size(); i--;)
            {
                if (records[i]->car && records[i]->block_way && !records[i]->nevi)
                {
                    S_vector d = records[i]->car->GetFrame()->GetWorldPos() -
                        act_mission->roads->links[road_blocks[j].node].pos;
                    float dist = d.Square();
                    if (!road_blocks[j].active && dist < 150.0f * 150.0f)
                        continue;
                    if (dist < 200.0f * 200.0f)
                    {
                        near_block = true;
                        human = records[i]->human;
                        break;
                    }
                }
            }
            if (road_blocks[j].active && !near_block)
            {
                road_blocks[j].active = false;
                for (int k = road_blocks[j].unknown_4.size(); k--;)
                {
                    C_actor *act = (C_actor *)road_blocks[j].unknown_4[k];
                    if (act->GetType() == ACTOR_ENTITY)
                        ((C_entity *)act)->Event(NULL, ENTITY_EVENT_DISMISS, NULL);
                    AddDoRiciActor((C_actor *)road_blocks[j].unknown_4[k]);
                }
                road_blocks[j].unknown_4.clear();
            }
            else if (!road_blocks[j].active && near_block)
            {
                road_blocks[j].active = true;
                CreateRoadBlock(j, human);
            }
        }
        CreateShvestkyTest();
    }
    // The player's record: the HUD's bar of the time left
    for (i = records.size(); i--;)
    {
        if (records[i]->human == act_mission->GetGame()->GetPlayer())
        {
            if (records[i]->alerted)
            {
                if (records[i]->wanted_time_start > 0 && records[i]->wanted_time < records[i]->wanted_time_start)
                {
                    int left = records[i]->wanted_time - records_timer.time;
                    if (left > records[i]->wanted_time)
                        left = records[i]->wanted_time;
                    int total = records[i]->wanted_time_start - records_timer.period;
                    if (total < 1)
                        total = 1;
                    float f = (float)left / total;
                    if (f > 1.0f)
                        f = 1.0f;
                    else if (f < 0.0f)
                        f = 0.0f;
                    mafiaIndicators.SetProgress(f);
                }
                else
                    mafiaIndicators.SetProgress(1.0f);
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
    for (int i = records.size(); i--;)
    {
        if (records[i]->car)
        {
            S_vector pos = records[i]->car->GetWorldPos();
            S_vector dir = records[i]->car->GetVelocity();
            ushort link = act_mission->roads->GetNearNode(records[i]->car->GetFrame()->GetWorldPos());
            if (link != NEARNODE_NONE && !dir.IsNull())
            {
                dir.Normalize();
                ushort ends[2];
                ends[0] = act_mission->roads->links[link].end0;
                ends[1] = act_mission->roads->links[link].end1;
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
                    bool ok = !(ends[0] & NEARNODE_FLAG) && act_mission->roads->nodes[ends[0]].semaphore;
                    if (ok)
                    {
                        for (int j = bridge_nodes.size(); j--;)
                        {
                            if ((bridge_nodes[j] & NEARNODE_INDEX) == (ends[0] & NEARNODE_INDEX))
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
                        S_vector2 moved = records[i]->pos - car_pos;
                        S_vector2 dn = d;
                        dn.Normalize();
                        line_pos += dn * 1.5f;
                        if ((float)fabs(car_pos.DistanceToLine(line_pos, d)) < 15.0f)
                        {
                            float side = car_pos.DistanceToLine(line_pos, n);
                            if (records[i]->pos.DistanceToLine(line_pos, n) * side <= 0.0f)
                            {
                                int *phase = (float)fabs(d.x) > (float)fabs(d.y) ?
                                    &act_mission->GetGame()->semaphore_phase_x : &act_mission->GetGame()->semaphore_phase_z;
                                if (phase && semaphore_colors[*phase] == SEMAPHORE_RED)
                                {
                                    records[i]->flags |= SHVF_RED_LIGHT;
                                    records[i]->signal_time += 4000;
                                    if (records[i]->signal_time > 10000)
                                        records[i]->signal_time = 10000;
                                    if (records[i]->new_state < SHVSTATE_1)
                                    {
                                        records[i]->new_state = SHVSTATE_1;
                                        records[i]->reason = SHVREASON_RED_LIGHT;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            records[i]->pos = S_vector2(pos.x, pos.z);
        }
    }
}

// The records' new states from their flags (PS2 name; PS2 has it out of line, the PC's Tick inlines it), the police
// told of a change, and the player's on the HUD with more policemen at its states 5 and 6
inline void C_shvestky::CreateShvestkyTest()
{
    for (int i = records.size(); i--;)
    {
        if ((records[i]->flags & SHVF_SPEEDING) && records[i]->car)
        {
            if (records[i]->car->GetSpeed() > speed_limit_arrest)
            {
                if (records[i]->new_state < SHVSTATE_ARREST)
                {
                    records[i]->new_state = SHVSTATE_ARREST;
                    records[i]->reason = SHVREASON_SPEEDING;
                }
            }
            else
            {
                if (records[i]->new_state < SHVSTATE_1)
                {
                    records[i]->new_state = SHVSTATE_1;
                    records[i]->reason = SHVREASON_SPEEDING;
                }
            }
        }
        if (records[i]->flags & SHVF_FORCE_ARREST)
        {
            if (records[i]->new_state < SHVSTATE_ARREST)
                records[i]->new_state = SHVSTATE_ARREST;
        }
        if (records[i]->flags & SHVF_HIT_POLICEMAN)
        {
            if (records[i]->new_state < SHVSTATE_KILL)
            {
                records[i]->new_state = SHVSTATE_KILL;
                records[i]->reason = SHVREASON_HIT_POLICEMAN;
            }
        }
        if (records[i]->flags & SHVF_RAM_POLICE_CAR)
        {
            if (records[i]->hard_ram)
            {
                if (records[i]->new_state < SHVSTATE_ARREST)
                {
                    records[i]->new_state = SHVSTATE_ARREST;
                    records[i]->reason = SHVREASON_RAM_POLICE_CAR;
                }
            }
            else
            {
                if (records[i]->new_state < SHVSTATE_1)
                {
                    records[i]->new_state = SHVSTATE_1;
                    records[i]->reason = SHVREASON_RAM_POLICE_CAR;
                }
            }
        }
        if (records[i]->new_state < SHVSTATE_1 && (records[i]->flags & SHVF_WEAPON))
        {
            if (records[i]->human->car && records[i]->human->car->GetType() == ACTOR_CAR)
            {
                if (records[i]->human->aimed)
                {
                    int item = records[i]->human->inventory.GetItemInRightHand(NULL);
                    if (records[i]->human->seat)
                    {
                        if (item >= SHVITEM_CAR_FIRST && item <= SHVITEM_CAR_LAST)
                        {
                            records[i]->new_state = SHVSTATE_ARREST;
                            records[i]->reason = SHVREASON_WEAPON;
                        }
                    }
                    else
                    {
                        if (item >= SHVITEM_CAR_FIRST && item <= SHVITEM_DRIVER_LAST)
                        {
                            records[i]->new_state = SHVSTATE_ARREST;
                            records[i]->reason = SHVREASON_WEAPON;
                        }
                    }
                }
            }
            else if (!records[i]->human->car)
            {
                records[i]->new_state = SHVSTATE_ARREST;
                records[i]->reason = SHVREASON_WEAPON;
            }
        }
        if ((records[i]->flags & SHVF_CAR_STEAL) && records[i]->new_state < SHVSTATE_ARREST)
        {
            records[i]->new_state = SHVSTATE_ARREST;
            records[i]->reason = SHVREASON_CAR_STEAL;
        }
        if ((records[i]->flags & SHVF_ILLEGAL_PATH) && records[i]->illegal_path_time > 5800 &&
            records[i]->new_state < SHVSTATE_1)
        {
            records[i]->new_state = SHVSTATE_1;
            records[i]->reason = SHVREASON_ILLEGAL_PATH;
        }
        if (records[i]->new_state == SHVSTATE_1 && records[i]->state < SHVSTATE_ARREST && records[i]->car &&
            records[i]->car->police_model)
            records[i]->new_state = SHVSTATE_NONE;
        if (records[i]->new_state == SHVSTATE_1 && records[i]->offenses >= 3)
            records[i]->new_state = SHVSTATE_ARREST;
        if (records[i]->new_state > records[i]->state)
        {
            bool seen = true;
            if (records[i]->nevi)
                seen = AIManager.police.PanoveMateOciNaZmrdoviNeboCumitePoBabach(records[i]->human);
            if (records[i]->nasrany.size())
            {
                if (seen)
                {
                    switch (records[i]->new_state)
                    {
                    case SHVSTATE_ARREST:
                        AIManager.police.PanoveZmenaZatkneteHoZmrda(records[i]->human);
                        break;
                    case SHVSTATE_KILL:
                        AIManager.police.PanoveZmenaZabijteHoZmrda(records[i]->human);
                        break;
                    }
                    records[i]->state = records[i]->new_state;
                }
            }
            else
                records[i]->state = records[i]->new_state;
        }
        else if (records[i]->new_state && records[i]->nasrany.size() && records[i]->nevi &&
            AIManager.police.PanoveMateOciNaZmrdoviNeboCumitePoBabach(records[i]->human))
            AIManager.police.PanoveVytahneteBlokyAZapisteSiZmrda(records[i]->human);
        bool gone = !records[i]->nasrany.size() && records[i]->wanted_time < 0;
        int time_left = records[i]->wanted_time;
        uint count = records[i]->nasrany.size();
        if (records[i]->alerted && records[i]->nevi)
        {
            if (records[i]->new_state > SHVSTATE_NONE && records[i]->state > SHVSTATE_NONE)
                VasekSelChlastatNaOslavu(records[i]->human, records[i]->human->GetWorldPos(), 150.0f, false,
                    records[i]->new_state < SHVSTATE_1);
        }
        else if (records[i]->alerted ? (bool)(records[i]->state > SHVSTATE_NONE) :
            (bool)(records[i]->new_state > SHVSTATE_NONE))
            VasekSelChlastatNaOslavu(records[i]->human, records[i]->human->GetWorldPos(), 150.0f, false, false);
        if (gone && !records[i]->nasrany.size())
        {
            records[i]->state = SHVSTATE_NONE;
            records[i]->wanted_time = -1;
        }
        if (count == records[i]->nasrany.size())
            records[i]->wanted_time = time_left;
        else
            mafiaIndicators.SetProgress(1.0f);
        if (records[i]->human == act_mission->GetGame()->GetPlayer())
        {
            ushort flags = records[i]->flags;
            int level = records[i]->state;
            if ((flags & SHVF_FORCE_ARREST) && records[i]->alerted && level < SHVSTATE_ARREST)
                level = SHVSTATE_ARREST;
            if ((flags & SHVF_HIT_POLICEMAN) && level < SHVSTATE_KILL)
                level = SHVSTATE_KILL;
            if (level > SHVSTATE_NONE)
            {
                if (records[i]->alerted)
                    level += 3;
                mafiaIndicators.AddFlags(INDF_SYMBOL);
                float alpha = 1.0f;
                if (PocetNasranychZivych(records[i]->human))
                {
                    if (AIManager.police.PanoveJdeteNekdoPoZmrdovi(records[i]->human))
                        records[i]->nevi = !AIManager.police.PanoveViditeZmrdaNeboSteTupci(records[i]->human);
                    else
                        records[i]->nevi = false;
                }
                else if (records[i]->human->car)
                    records[i]->nevi = records[i]->known_car != records[i]->human->car;
                else
                    records[i]->nevi = !records[i]->known_on_foot;
                int symbol = level;
                if (level > 3)
                    symbol = level - 3;
                if (records[i]->nevi && !unknown_38)
                    symbol = 4;
                if (!AIManager.police.PanoveMateOciNaZmrdoviNeboCumitePoBabach(records[i]->human))
                    alpha = .4f;
                mafiaIndicators.SetSymbol(symbol, alpha);
                if (level == 5)
                {
                    if (records[i]->support_level < 1)
                    {
                        records[i]->support_level = 1;
                        act_mission->GetGame()->GetShvestky()->DalsiChalpiDoPartie(records[i]->human, 4, 200.0f);
                    }
                }
                else if (level == 6)
                {
                    if (records[i]->support_level < 2)
                    {
                        records[i]->support_level = 2;
                        act_mission->GetGame()->GetShvestky()->DalsiChalpiDoPartie(records[i]->human, 0, 200.0f);
                        act_mission->GetGame()->GetShvestky()->DalsiChalpiDoPartie(records[i]->human, 0, 200.0f);
                    }
                }
            }
            else
                mafiaIndicators.RemoveFlags(INDF_SYMBOL);
        }
        records[i]->new_state = SHVSTATE_NONE;
    }
}

// STUB: GAME 0x49b860
void C_shvestky::SetActive(bool on, int text, bool unknown2)
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

// STUB: GAME 0x49e7e0
bool C_shvestky::FreeMyTempActor(C_actor *act)
{
    StubLog("C_shvestky::FreeMyTempActor");
    return false;
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

// PS2's order and shapes. The distance is the camera's on the ground, 150 m: an entity goes once it is that far, or by
// its AISTATEF_DISMISSED where its human_type is 4; one in a car (of ACTOR_CAR) goes only once that far, the car
// emptied of it and the loop run again for the car, else it stays and the records forget it. A car goes once it is
// that far with nobody in its seats and no player or entity having it as a car, or once its frame is off; every other
// actor goes at once. The members are read through their classes' accessors, as Tick reads them: as inline calls still
// to come, they leave S_vector's constructor in the inlined operator- out of line, as the original calls it. The test
// of the global actors' pointer itself, not of the actor, is PS2's too.
// FUNCTION: GAME 0x49ea80
void C_shvestky::TickDoRiciActors(bool all)
{
    S_vector pos = act_mission->scene->GetActiveCamera()->GetWorldPos();
    bool again;
    do
    {
        again = false;
        for (uint i = 0; i < do_rici_actors.size();)
        {
            S_vector d = pos - do_rici_actors[i]->GetFrame()->GetWorldPos();
            d.y = 0.0f;
            float dist = d.Square();
            bool remove = all;
            if (!remove)
            {
                switch (do_rici_actors[i]->GetType())
                {
                case ACTOR_ENTITY:
                    remove = dist > 22500.0f;
                    if (!do_rici_actors[i]->GetDead() && ((C_entity *)do_rici_actors[i])->human_type == 4)
                        remove = (((C_entity *)do_rici_actors[i])->ai_state_flags & AISTATEF_DISMISSED) != 0;
                    if (remove)
                    {
                        C_car *car = ((C_entity *)do_rici_actors[i])->GetCar();
                        if (car && car->GetType() == ACTOR_CAR)
                        {
                            remove = dist > 22500.0f;
                            if (remove)
                            {
                                car->SetOwner(NULL, ((C_entity *)do_rici_actors[i])->seat, 0.0f);
                                again = true;
                            }
                            else
                                GlobalDelNasrany(do_rici_actors[i]);
                        }
                    }
                    break;
                case ACTOR_CAR:
                    if (dist > 22500.0f)
                    {
                        remove = true;
                        C_car *car = (C_car *)do_rici_actors[i];
                        for (int j = car->GetNumSeats(); j--;)
                        {
                            if (car->GetOwner(j))
                            {
                                remove = false;
                                break;
                            }
                        }
                        if (remove)
                        {
                            for (C_actor **p = C_actor::GetGlobalActors().begin();
                                p < C_actor::GetGlobalActors().end(); p++)
                            {
                                if (p && ((*p)->GetType() == ACTOR_PLAYER || (*p)->GetType() == ACTOR_ENTITY) &&
                                    (((C_human *)*p)->GetCar() == car || ((C_human *)*p)->using_car == car))
                                {
                                    remove = false;
                                    break;
                                }
                            }
                        }
                    }
                    else if (!do_rici_actors[i]->GetFrame()->IsOn())
                        remove = true;
                    break;
                default:
                    remove = true;
                }
            }
            if (remove)
            {
                GlobalDelNasrany(do_rici_actors[i]);
                if (do_rici_actors[i]->GetType() == ACTOR_CAR)
                {
                    mafiaIndicators.RadarRemoveCar((C_car *)do_rici_actors[i]);
                    act_mission->GetGame()->GetSledovacka()->RemoveCar((C_car *)do_rici_actors[i]);
                    if (((C_car *)do_rici_actors[i])->police_car)
                        C_traffic_car::ChlupatejVystrik();
                }
                act_mission->GetGame()->InvalidateActor(do_rici_actors[i], false);
                if (!FreeMyTempActor(do_rici_actors[i]))
                {
                    act_mission->GetGame()->InvalidateActor(do_rici_actors[i], false);
                    act_mission->GetGame()->RemoveTemporaryActor(do_rici_actors[i]);
                }
                do_rici_actors[i] = do_rici_actors.back();
                do_rici_actors.pop_back();
            }
            else
                i++;
        }
    } while (again);
    if (all)
        do_rici_actors.clear();
}

// STUB: GAME 0x49eee0
void C_shvestky::DalsiChalpiDoPartie(C_human *human, uint unknown, float unknown2)
{
    StubLog("C_shvestky::DalsiChalpiDoPartie");
}

// STUB: GAME 0x49f540
void C_shvestky::GlobalDelNasrany(C_actor *act)
{
    StubLog("C_shvestky::GlobalDelNasrany");
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

// STUB: GAME 0x49d0b0
void C_shvestky::CarSteal(C_human *human, bool a, bool b)
{
    StubLog("C_shvestky::CarSteal");
}

// STUB: GAME 0x49d220
void C_shvestky::NextNaSrackySoudruh(C_human *human)
{
    StubLog("C_shvestky::NextNaSrackySoudruh");
}

// STUB: GAME 0x49d030
void C_shvestky::HumanBoxing(C_human *human)
{
    StubLog("C_shvestky::HumanBoxing");
}

// STUB: GAME 0x49d520
int C_shvestky::PocetPrestupku(C_human *human)
{
    StubLog("C_shvestky::PocetPrestupku");
    return 0;
}

// STUB: GAME 0x49a140
void C_shvestky::DezoSebralGumyIRadio(C_car *car)
{
    StubLog("C_shvestky::DezoSebralGumyIRadio");
}

// STUB: GAME 0x49cb20
void C_shvestky::Firing(C_human *human)
{
    StubLog("C_shvestky::Firing");
}

// STUB: GAME 0x4a0230
void C_shvestky::RenamePoliceCar(C_car *car)
{
    StubLog("C_shvestky::RenamePoliceCar");
}
