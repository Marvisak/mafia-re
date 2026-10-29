// The police (PS2 C_shvestky.cpp; the PC's file is the one at 0x499b40-0x4a031c, docs/game-files.md), which starts with
// the constructor and the destructor. Written so far: those two, which the game's constructor and destructor call, and
// GameInit and GameDone, which C_game's Init and Done call; the records' destructor, TickDoRiciActors and what the
// script's commands call are stubs.

#include "ai/C_shvestky.h"
#include "actors/C_actor.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "ui/g_indicators.h"
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

#pragma auto_inline(on)


