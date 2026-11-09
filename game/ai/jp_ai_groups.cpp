// The AI's groups, following and fights (PS2 jp_ai_groups.cpp; the PC's file is most likely the one at
// 0x530390-0x53857c, after jp_ai_entity.cpp as the names sort, docs/game-files.md). Written so far: the managers' Init
// and Destroy, which C_game's Init and Done call, PS2's shapes, and the groups' Tick, the following's Update and the
// fights' Tick, which C_game::Tick calls; stubs of what the script's commands call (GetGroup, LeaveParty, GetParty), of
// the group's ProcessEvents, which the groups' Tick calls, and of the groups' and the parties' helpers the fights' Tick
// calls (GetCarFactor, GetCarClosestAndUsable, TakeCar, ComputePosition, mustCarHunt, DisbandCarParty), after it. The
// copies of the implicit functions they call (ai_group's and ai_fight_followship's operator= and destructors, the
// vectors' helpers) are compiler-generated.

#include "ai/jp_ai_groups.h"

#include "actors/C_player.h"
#include "ai/jp_ai_entity.h"
#include "ai/jp_ai_main.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "vehicles/C_car.h"
#include "stub.h"

// STUB: GAME 0x5310d0
ai_group *ai_groups::GetGroup(uint id)
{
    StubLog("ai_groups::GetGroup");
    return NULL;
}

// FUNCTION: GAME 0x531130
void ai_groups::Init()
{
    unlock_time = -1;
    last_id = 0;
    groups.clear();
    free_actors.clear();
}

// FUNCTION: GAME 0x5311a0
void ai_groups::Destroy()
{
    unlock_time = -1;
    last_id = 0;
    free_actors.clear();
    for (int i = groups.size() - 1; i >= 0; i--) {
        groups[i].members.clear();
        groups[i].attackers.clear();
        groups[i].events.clear();
        groups[i].cars.clear();
        groups[i].car_factors.clear();
    }
    groups.clear();
}

// The player's controls unlocked once the game's time passes unlock_time, the groups' events, then the actors the
// groups let go, from the last: one farther than 150 from the camera is put away, a car hidden and without its dynamic
// collisions first (PS2's the same)
// FUNCTION: GAME 0x531340
void ai_groups::Tick()
{
    uint time = act_mission->game->game_time;
    if (unlock_time != -1 && unlock_time < time)
    {
        unlock_time = -1;
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
            free_actors[j]->SetActState(ACTSTATE_NONE);
            free_actors.erase(free_actors.begin() + j);
        }
    }
}

// STUB: GAME 0x530920
void ai_group::ProcessEvents(uint time)
{
    StubLog("ai_group::ProcessEvents");
}

// STUB: GAME 0x5318f0
bool ai_groups::Load(void **buf)
{
    StubLog("ai_groups::Load");
    return false;
}

// STUB: GAME 0x5314d0
uint ai_groups::GetSize()
{
    StubLog("ai_groups::GetSize");
    return 0;
}

// STUB: GAME 0x5315b0
bool ai_groups::Save(void **buf)
{
    StubLog("ai_groups::Save");
    return true;
}

// The groups by their states, from the last group (PS2's the same): the followers get into the leader's car
// (FOLLOWSTATE_0), then, with FOLLOWF_HAS_POS and an entity driving, the car takes the group to its place, hunting
// where a follower is in danger (FOLLOWSTATE_2, until the chase's status says the drive ended), or else they ride along
// (FOLLOWSTATE_3); they get out after the leader (FOLLOWSTATE_1) and stay with the leader (FOLLOWSTATE_5). Every 2 s
// (check_time) the entities that follow without AIACTION_FOLLOW or with AIACTION_FIRE also leave their group, and a
// group left without followers goes. Only the PC's FOLLOWSTATE_1 also clears FOLLOWF_MUSTLEAVE on foot, testing is_car
// as an earlier group or frame left it.
// FUNCTION: GAME 0x5325a0
void ai_follow_manager::Update()
{
    bool check = false;
    uint time = act_mission->GetGame()->GetGameTime();
    if (time > check_time)
    {
        check_time = time + 2000;
        check = true;
    }
    bool is_car;
    for (int i = groups.size() - 1; i >= 0; i--)
    {
        ai_follow_group &g = groups[i];
        C_car *car;
        int n;
        int j;
        switch (g.state)
        {
        case FOLLOWSTATE_0:
            n = 0;
            car = ((C_human *)g.leader)->car;
            if (!car)
            {
                g.state = FOLLOWSTATE_1;
                continue;
            }
            is_car = car->type == ACTOR_CAR;
            for (j = g.followers.size() - 1; j >= 0; j--)
                if (((C_human *)g.followers[j])->car != car)
                    n++;
            if (n)
                break;
            if (g.flags & FOLLOWF_HAS_POS && is_car && car->GetOwner(0) && car->GetOwner(0)->type == ACTOR_ENTITY)
            {
                g.unknown_2c = act_mission->GetGame()->GetGameTime() + 6000;
                bool danger = false;
                for (j = g.followers.size() - 1; j >= 0; j--)
                {
                    C_entity *e = (C_entity *)g.followers[j];
                    if (e->IsInDanger())
                    {
                        danger = true;
                        break;
                    }
                }
                if (danger)
                    act_mission->GetGame()->GetSledovacka()->AddCarHunt(car, NULL, 0, 0, g.pos, 1.0f);
                else
                    act_mission->GetGame()->GetSledovacka()->AddCarMoveTo(car, g.pos, false, true, 0);
                g.state = FOLLOWSTATE_2;
                continue;
            }
            g.state = FOLLOWSTATE_3;
            continue;
        case FOLLOWSTATE_1:
            n = 0;
            car = ((C_human *)g.leader)->car;
            if (car)
            {
                is_car = car->type == ACTOR_CAR;
                g.state = FOLLOWSTATE_0;
                if (is_car && g.car && car != g.car)
                {
                    g.car = NULL;
                    g.flags &= ~FOLLOWF_MUSTLEAVE;
                }
                continue;
            }
            for (j = g.followers.size() - 1; j >= 0; j--)
                if (((C_human *)g.followers[j])->car)
                    n++;
            if (n)
                break;
            g.state = FOLLOWSTATE_5;
            if (is_car && !g.car)
                g.flags &= ~FOLLOWF_MUSTLEAVE;
            continue;
        case FOLLOWSTATE_2:
            car = ((C_human *)g.leader)->car;
            if (!car)
            {
                g.state = FOLLOWSTATE_1;
                continue;
            }
            // Written so, VC6 knows is_car in each branch and threads the test (the original stores 1 and 0 there)
            if (car->type == ACTOR_CAR)
                is_car = true;
            else
                is_car = false;
            if (!is_car)
            {
                g.state = FOLLOWSTATE_3;
                continue;
            }
            for (j = 0; j < g.followers.size(); j++)
            {
                if (((C_human *)g.followers[j])->car != car)
                {
                    g.state = FOLLOWSTATE_0;
                    break;
                }
            }
            switch (act_mission->GetGame()->GetSledovacka()->ActionStatus(car))
            {
            case C_sledovacka::STATUS_DONE:
                g.flags |= FOLLOWF_DRIVE_DONE;
                act_mission->GetGame()->GetSledovacka()->RemoveCar(car);
                g.state = FOLLOWSTATE_5;
                break;
            case C_sledovacka::STATUS_FAILED:
            case C_sledovacka::STATUS_DRIVER_DEAD:
            case C_sledovacka::STATUS_NO_ENERGY:
                act_mission->GetGame()->GetSledovacka()->RemoveCar(car);
                g.state = FOLLOWSTATE_1;
                g.flags |= FOLLOWF_MUSTLEAVE;
                g.car = car;
                break;
            }
            break;
        case FOLLOWSTATE_3:
            car = ((C_human *)g.leader)->car;
            if (!car)
                g.state = FOLLOWSTATE_1;
            for (j = 0; j < g.followers.size(); j++)
            {
                if (((C_human *)g.followers[j])->car != car)
                {
                    g.state = FOLLOWSTATE_0;
                    break;
                }
            }
            break;
        case FOLLOWSTATE_4:
            car = ((C_human *)g.leader)->car;
            if (!car)
            {
                g.state = FOLLOWSTATE_1;
                g.stop_end = 0;
                break;
            }
            if (car->type == ACTOR_CAR)
                is_car = true;
            else
                is_car = false;
            if (!is_car)
            {
                g.state = FOLLOWSTATE_3;
                g.stop_end = 0;
                break;
            }
            if (g.stop_end < time)
            {
                g.unknown_2c = act_mission->GetGame()->GetGameTime() + 6000;
                bool danger = false;
                for (j = g.followers.size() - 1; j >= 0; j--)
                {
                    C_entity *e = (C_entity *)g.followers[j];
                    if (e->IsInDanger())
                    {
                        danger = true;
                        break;
                    }
                }
                if (danger)
                    act_mission->GetGame()->GetSledovacka()->AddCarHunt(car, NULL, 0, 0, g.pos, 1.0f);
                else
                    act_mission->GetGame()->GetSledovacka()->AddCarMoveTo(car, g.pos, false, true, 0);
                g.state = FOLLOWSTATE_2;
                g.stop_end = 0;
                break;
            }
            act_mission->GetGame()->GetSledovacka()->RemoveCar(car);
            for (j = 0; j < g.followers.size(); j++)
            {
                if (((C_human *)g.followers[j])->car != car)
                {
                    g.state = FOLLOWSTATE_0;
                    break;
                }
            }
            break;
        case FOLLOWSTATE_5:
            car = ((C_human *)g.leader)->car;
            for (j = 0; j < g.followers.size(); j++)
            {
                if (((C_human *)g.followers[j])->car != car)
                {
                    g.state = car ? FOLLOWSTATE_0 : FOLLOWSTATE_1;
                    if (g.car && car && car->type == ACTOR_CAR && car != g.car)
                    {
                        g.car = NULL;
                        g.flags &= ~FOLLOWF_MUSTLEAVE;
                    }
                    break;
                }
            }
            break;
        }
        if (check)
        {
            for (j = g.followers.size() - 1; j >= 0; j--)
            {
                if (g.followers[j]->type == ACTOR_ENTITY)
                {
                    ai_action_manager &am = ((C_entity *)g.followers[j])->action_manager;
                    if (!am.hasAction(AIACTION_FOLLOW, NULL, false) || am.hasAction(AIACTION_FIRE, NULL, false))
                    {
                        g.followers.erase(g.followers.begin() + j);
                        if (!g.followers.size())
                            groups.erase(groups.begin() + i);
                    }
                }
            }
        }
    }
}

// FUNCTION: GAME 0x532da0
void ai_follow_manager::Init()
{
    check_time = 0;
    groups.clear();
}

// FUNCTION: GAME 0x532e30
void ai_follow_manager::Destroy()
{
    check_time = 0;
    for (int i = groups.size() - 1; i >= 0; i--)
        groups[i].followers.clear();
    groups.clear();
}

// STUB: GAME 0x5332e0
void ai_follow_manager::Event(uint event, C_actor *actor)
{
    StubLog("ai_follow_manager::Event");
}

// STUB: GAME 0x533530
uint ai_follow_manager::GetSize()
{
    StubLog("ai_follow_manager::GetSize");
    return 0;
}

// STUB: GAME 0x5335a0
bool ai_follow_manager::Save(void **buf)
{
    StubLog("ai_follow_manager::Save");
    return true;
}

// STUB: GAME 0x5337b0
bool ai_follow_manager::Load(void **buf)
{
    StubLog("ai_follow_manager::Load");
    return false;
}

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

// The fights' frame (PS2 name), the parties from the last, as PS2's. A party without members goes. Every 2 s a
// party's position is computed again and its car considered: a party on foot takes a car of its commander's group
// when its target drives away, a party in a car tells its members where the target is and, once all are in, hunts it
// through the chases (C_sledovacka), or gets them out. Between those the members of a party in a car go through
// their states, each locking its entity's substate with the state's number. Most distances are written out on a local
// vector: VC6 then squares the components in the original's order. Open: VC6's inline budget leaves the call
// push_back nests inline in the party taking a car (the original calls the vector's insert(P, x) out of line), and the
// order of the squares of the distance to the player.
// FUNCTION: GAME 0x5350b0
void ai_fight_manager::Tick()
{
    uint time = act_mission->GetGame()->GetGameTime();
    for (int i = parties.size() - 1; i >= 0; i--)
    {
        ai_fight_followship &party = parties[i];
        if (!party.members.size())
        {
            parties.erase(parties.begin() + i);
            continue;
        }
        S_vector pos, pos2;
        if (party.check_time >= time)
        {
            switch (party.type)
            {
            case FIGHTPARTY_IN_CAR:
                {
                    // The members' states, which the part below sets too: recent_shot and recent_seen whether the
                    // entity's log of the target has it shooting within the last 2 s and seen within the last 3 s
                    bool mode1 = party.unknown_34 == 1;
                    bool mode2 = party.unknown_34 == 2;
                    for (uint j = 0; j < party.members.size(); j++)
                    {
                        ai_fight_member &m = party.members[j];
                        m.entity->LockSubstate(3, 8, 2.0f, 0, false);
                        bool recent_shot = false;
                        bool recent_seen = false;
                        ai_character_log *log = m.entity->logs.GetLog(party.target);
                        if (log && time - log->shot_time < 2000)
                            recent_shot = true;
                        if (log && time - log->seen_time < 3000)
                            recent_seen = true;
                        switch (m.state)
                        {
                        case FIGHTSTATE_0:
                            if (!party.unknown_34)
                                break;
                            if (!j)
                            {
                                if (!recent_shot)
                                    m.state = FIGHTSTATE_9;
                                else if (party.members.size() > 2)
                                    m.state = FIGHTSTATE_9;
                                else
                                    m.state = FIGHTSTATE_2;
                            }
                            else
                            {
                                if (party.unknown_34 == 2)
                                    m.state = FIGHTSTATE_2;
                                if (party.unknown_34 == 1)
                                    m.state = FIGHTSTATE_1;
                            }
                            break;
                        case FIGHTSTATE_1:
                            {
                                pos = m.entity->frame->GetWorldPos();
                                pos2 = party.car->frame->GetWorldPos();
                                S_vector d = pos2 - pos;
                                float dist = d.x * d.x + d.y * d.y + d.z * d.z;
                                if (recent_shot && recent_seen)
                                {
                                    if (dist < 25.0f)
                                        m.state = FIGHTSTATE_3;
                                    else
                                        m.state = FIGHTSTATE_2;
                                }
                                else if (m.entity == party.commander)
                                    m.state = FIGHTSTATE_4;
                                else if (dist < 25.0f)
                                    m.state = FIGHTSTATE_3;
                                else
                                    m.entity->LockSubstate(3, 8, 1.0f, 0, false);
                            }
                            break;
                        case FIGHTSTATE_3:
                            m.entity->LockSubstate(3, 8, 3.0f, 0, false);
                            break;
                        case FIGHTSTATE_4:
                            if (recent_shot)
                            {
                                m.state = FIGHTSTATE_3;
                                break;
                            }
                            if (mode2)
                            {
                                m.state = FIGHTSTATE_3;
                                break;
                            }
                            if (party.car->GetPlacementFromTCK1(j, pos, pos2, false))
                            {
                                pos2 = m.entity->frame->GetWorldPos();
                                // Far from the player, near the seat and the seat free: the member gets in
                                S_vector cam = act_mission->GetGame()->GetPlayer()->frame->GetWorldPos();
                                if ((cam - pos2).Square() > 10000.0f)
                                {
                                    S_vector d = pos2 - pos;
                                    if (d.x * d.x + d.y * d.y + d.z * d.z < 400.0f && !party.car->GetOwner(j))
                                    {
                                        m.entity->Intern_UseCar(party.car, j);
                                        m.state = FIGHTSTATE_8;
                                        if (!j)
                                            party.car->SetEngineOn(true, true);
                                        break;
                                    }
                                }
                                S_vector d = pos2 - pos;
                                d.y = 0.0f;
                                if (d.x * d.x + d.y * d.y + d.z * d.z < 0.16f)
                                    m.state = party.car->GetOwner(j) ? FIGHTSTATE_11 : FIGHTSTATE_5;
                                else
                                    m.entity->LockSubstate(3, 8, 4.0f, 0, false);
                            }
                            else
                                m.state = FIGHTSTATE_3;
                            break;
                        case FIGHTSTATE_5:
                            if (m.entity->car == party.car)
                            {
                                m.state = FIGHTSTATE_8;
                                if (!j)
                                    party.car->SetEngineOn(true, true);
                                break;
                            }
                            if (m.entity->car)
                            {
                                m.state = FIGHTSTATE_6;
                                break;
                            }
                            if (party.car->GetPlacementFromTCK1(j, pos, pos2, false))
                            {
                                pos2 = m.entity->frame->GetWorldPos();
                                S_vector d = pos2 - pos;
                                d.y = 0.0f;
                                if (d.x * d.x + d.y * d.y + d.z * d.z > 1.0f)
                                {
                                    m.state = FIGHTSTATE_4;
                                    break;
                                }
                            }
                            m.entity->LockSubstate(3, 8, 5.0f, 0, false);
                            break;
                        case FIGHTSTATE_6:
                            if (!m.entity->car)
                                m.state = FIGHTSTATE_2;
                            else
                                m.entity->LockSubstate(3, 8, 6.0f, 0, false);
                            break;
                        case FIGHTSTATE_7:
                            m.entity->LockSubstate(3, 8, 7.0f, 0, false);
                            break;
                        case FIGHTSTATE_8:
                            m.entity->LockSubstate(3, 8, 8.0f, 0, false);
                            break;
                        case FIGHTSTATE_9:
                            if (recent_shot && recent_seen && !j)
                            {
                                m.state = FIGHTSTATE_3;
                                break;
                            }
                            if (mode1)
                            {
                                m.state = FIGHTSTATE_4;
                                break;
                            }
                            m.entity->LockSubstate(3, 8, 9.0f, 0, false);
                            break;
                        case FIGHTSTATE_10:
                            if (recent_shot && recent_seen && !j)
                            {
                                m.state = FIGHTSTATE_3;
                                break;
                            }
                            m.entity->LockSubstate(3, 8, 10.0f, 0, false);
                            break;
                        case FIGHTSTATE_11:
                            m.entity->LockSubstate(3, 8, 11.0f, 0, false);
                            break;
                        }
                    }
                }
                break;
            }
        }
        else
        {
            party.check_time = time + 2000;
            party.ComputePosition();
            switch (party.type)
            {
            case FIGHTPARTY_ON_FOOT:
                {
                    // The target driving away in a car of its own: the closest usable car of the commander's group
                    // becomes a new party's
                    C_human *target = party.target;
                    if (target->type == ACTOR_PLAYER || target->type == ACTOR_ENTITY)
                    {
                        C_car *target_car = target->car;
                        if (target_car && target_car->type == ACTOR_CAR)
                        {
                            pos = target_car->frame->GetWorldPos() - party.pos;
                            if (pos.Square() > 400.0f && target_car->speed > 3.0f)
                            {
                                ai_group *g = AIManager.groups.GetGroup(party.commander->group_id);
                                uint found = 0;
                                if (g)
                                {
                                    S_vector v;
                                    C_car *car = g->GetCarClosestAndUsable(&party.pos, &v, &found);
                                    if (car)
                                    {
                                        ai_fight_followship *p = party.TakeCar(car);
                                        if (p)
                                        {
                                            if (found)
                                            {
                                                p->unknown_34 = 2;
                                                p->unknown_c = v;
                                                parties.push_back(*p);
                                            }
                                            else
                                            {
                                                p->unknown_34 = 1;
                                                parties.push_back(*p);
                                            }
                                            delete p;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                break;
            case FIGHTPARTY_IN_CAR:
                {
                    // The members told where the target is, counting those in the car
                    uint in_car = 0;
                    ai_blob *blob = new ai_blob;
                    blob->pos = party.target->GetWorldPos();
                    blob->dir = party.target->GetWorldDir();
                    blob->time = time;
                    blob->flags = 0;
                    blob->actor = party.target;
                    uint num = party.members.size();
                    for (uint j = 0; j < num; j++)
                    {
                        if (party.members[j].entity->car == party.car)
                            in_car++;
                        ai_blob *b = new ai_blob;
                        memcpy(b, blob, sizeof(ai_blob));
                        party.members[j].entity->Event(NULL, ENTITY_EVENT_SNIFF, b);
                    }
                    delete blob;
                    if (party.stand_time < FIGHT_HUNT_ENDED)
                    {
                        if (num == in_car && party.car->speed < 2.0f)
                            party.stand_time += 2000;
                        else
                            party.stand_time = 4000;
                    }
                    if (party.stand_time >= 10000 && party.stand_time < FIGHT_HUNT_ENDED)
                    {
                        act_mission->GetGame()->GetSledovacka()->SetActionStatus(party.car,
                            C_sledovacka::STATUS_FAILED);
                        party.stand_time = FIGHT_HUNT_ENDED;
                    }
                    if (num == in_car)
                    {
                        // All in: a target on foot near the car or the car standing gets them out, else the car
                        // hunts it
                        C_human *target = party.target;
                        if ((target->type == ACTOR_PLAYER || target->type == ACTOR_ENTITY) && !target->car)
                        {
                            pos = party.car->frame->GetWorldPos();
                            pos2 = target->frame->GetWorldPos();
                            S_vector d = pos2 - pos;
                            if (d.Square() < 625.0f)
                                act_mission->GetGame()->GetSledovacka()->RemoveCar(party.car);
                            if (party.car->speed < 0.5f)
                            {
                                for (uint j = 0; j < party.members.size(); j++)
                                    party.members[j].state = FIGHTSTATE_6;
                            }
                        }
                        else
                        {
                            C_human *owner = party.car->GetOwner(0);
                            if (owner && !owner->alive)
                            {
                                act_mission->GetGame()->GetSledovacka()->RemoveCar(party.car);
                                if (party.car->speed < 0.5f)
                                {
                                    for (uint j = 0; j < party.members.size(); j++)
                                        party.members[j].state = FIGHTSTATE_6;
                                    party.car->death_fade_time = 0;
                                }
                            }
                            else if (party.stand_time >= FIGHT_HUNT_ENDED)
                            {
                                party.car->death_fade_time = CAR_DEATH_FADE_START;
                                act_mission->GetGame()->GetSledovacka()->RemoveCar(party.car);
                                for (uint j = 0; j < party.members.size(); j++)
                                    party.members[j].state = FIGHTSTATE_6;
                                return;
                            }
                            else
                            {
                                uint j;
                                for (j = 0; j < party.members.size(); j++)
                                {
                                    if (!j)
                                        party.members[j].state = FIGHTSTATE_7;
                                    else
                                        party.members[j].state = FIGHTSTATE_8;
                                }
                                ai_group *g = AIManager.groups.GetGroup(party.commander->group_id);
                                float factor = 1.0f;
                                if (g)
                                    factor = g->GetCarFactor(party.car);
                                act_mission->GetGame()->GetSledovacka()->AddCarHunt(party.car, party.target, 0x200,
                                    300, S_vector(0.0f, 0.0f, 0.0f), factor);
                                switch (act_mission->GetGame()->GetSledovacka()->ActionStatus(party.car))
                                {
                                case C_sledovacka::STATUS_DONE:
                                    act_mission->GetGame()->GetSledovacka()->RemoveCar(party.car);
                                    for (j = 0; j < party.members.size(); j++)
                                        party.members[j].state = FIGHTSTATE_6;
                                    return;
                                case C_sledovacka::STATUS_FAILED:
                                    party.stand_time = FIGHT_HUNT_ENDED;
                                    party.car->death_fade_time = CAR_DEATH_FADE_START;
                                    act_mission->GetGame()->GetSledovacka()->RemoveCar(party.car);
                                    for (j = 0; j < party.members.size(); j++)
                                        party.members[j].state = FIGHTSTATE_6;
                                    return;
                                case C_sledovacka::STATUS_DRIVER_DEAD:
                                    act_mission->GetGame()->GetSledovacka()->RemoveCar(party.car);
                                    for (j = 0; j < party.members.size(); j++)
                                        party.members[j].state = FIGHTSTATE_6;
                                    return;
                                case C_sledovacka::STATUS_NO_ENERGY:
                                    party.car->death_fade_time = CAR_DEATH_FADE_START;
                                    act_mission->GetGame()->GetSledovacka()->RemoveCar(party.car);
                                    for (j = 0; j < party.members.size(); j++)
                                        party.members[j].state = FIGHTSTATE_6;
                                    return;
                                }
                            }
                        }
                    }
                    else if (party.mustCarHunt() && party.stand_time < FIGHT_HUNT_ENDED)
                    {
                        // Not all in yet: the members walk to the car, or wait in it
                        if (party.car->speed > 0.5f)
                            break;
                        party.stand_time = 0;
                        pos = party.car->frame->GetWorldPos() - party.pos;
                        pos.y = 0.0f;
                        pos *= 0.8f;
                        float dist = pos.Square();
                        uint j;
                        if (party.commander->car == party.car)
                        {
                            for (j = 0; j < party.members.size(); j++)
                            {
                                if (party.members[j].entity->car == party.car)
                                    party.members[j].state = FIGHTSTATE_8;
                                else
                                    party.members[j].state = FIGHTSTATE_4;
                            }
                        }
                        else if (dist <= party.max_dist_sq && dist <= 49.0f)
                        {
                            for (j = 0; j < party.members.size(); j++)
                            {
                                if (party.members[j].entity->car == party.car)
                                    party.members[j].state = FIGHTSTATE_8;
                                else
                                    party.members[j].state = FIGHTSTATE_4;
                            }
                        }
                        else
                        {
                            for (j = 0; j < party.members.size(); j++)
                            {
                                if (party.members[j].entity->car == party.car)
                                    party.members[j].state = FIGHTSTATE_8;
                                else
                                {
                                    if (party.members[j].state == FIGHTSTATE_2)
                                        party.members[j].state = FIGHTSTATE_1;
                                    if (party.members[j].state == FIGHTSTATE_3)
                                        party.members[j].state = FIGHTSTATE_4;
                                }
                            }
                        }
                    }
                    else
                    {
                        // The hunt over: the members out of the standing car, the party disbanded once none is in
                        // FIGHTSTATE_5
                        act_mission->GetGame()->GetSledovacka()->RemoveCar(party.car);
                        if (party.car->speed < 0.5f)
                        {
                            int waiting = 0;
                            for (uint j = 0; j < party.members.size(); j++)
                            {
                                if (party.members[j].entity->car)
                                    party.members[j].state = FIGHTSTATE_6;
                                if (!party.members[j].entity->car && party.members[j].state != FIGHTSTATE_5)
                                    party.members[j].state = FIGHTSTATE_2;
                                if (party.members[j].state == FIGHTSTATE_5)
                                    waiting++;
                            }
                            if (!waiting)
                                DisbandCarParty(&party, false);
                        }
                        return;
                    }
                }
                break;
            }
        }
    }
}

// STUB: GAME 0x530a70
float ai_group::GetCarFactor(C_car *car)
{
    StubLog("ai_group::GetCarFactor");
    return 1.0f;
}

// STUB: GAME 0x530ad0
C_car *ai_group::GetCarClosestAndUsable(S_vector *pos, S_vector *unknown, uint *unknown2)
{
    StubLog("ai_group::GetCarClosestAndUsable");
    return NULL;
}

// STUB: GAME 0x534270
ai_fight_followship *ai_fight_followship::TakeCar(C_car *car)
{
    StubLog("ai_fight_followship::TakeCar");
    return NULL;
}

// STUB: GAME 0x534810
void ai_fight_followship::ComputePosition()
{
    StubLog("ai_fight_followship::ComputePosition");
}

// STUB: GAME 0x534980
bool ai_fight_followship::mustCarHunt()
{
    StubLog("ai_fight_followship::mustCarHunt");
    return false;
}

// STUB: GAME 0x534b20
void ai_fight_manager::DisbandCarParty(ai_fight_followship *party, bool unknown)
{
    StubLog("ai_fight_manager::DisbandCarParty");
}

// STUB: GAME 0x531dc0
void ai_groups::InvalidateActor(C_actor *act)
{
    StubLog("ai_groups::InvalidateActor");
}

// STUB: GAME 0x5331c0
void ai_follow_manager::Invalidate(C_actor *act)
{
    StubLog("ai_follow_manager::Invalidate");
}

// STUB: GAME 0x536200
void ai_fight_manager::Invalidate(C_actor *act)
{
    StubLog("ai_fight_manager::Invalidate");
}
