// The AI's groups, following and fights (PS2 jp_ai_groups.cpp; the PC's file is most likely the one at
// 0x530390-0x53857c, after jp_ai_entity.cpp as the names sort, docs/game-files.md), written whole in the original's
// order, PS2's: the groups (ai_group, ai_groups), the following (ai_follow_manager) and the fights (ai_fight_manager,
// its parties), each manager's Init, Destroy, frame, saved game's record and InvalidateActor. Many of the functions are
// called only from files not written yet (the AI's humans, the police, the traffic): game/CMakeLists.txt keeps them
// linked. The copies of the implicit functions they call (ai_group's and ai_fight_followship's operator= and
// destructors, the vectors' helpers) are compiler-generated.

#include "ai/jp_ai_groups.h"

#include "actors/C_player.h"
#include "ai/jp_ai_entity.h"
#include "ai/jp_ai_main.h"
#include "collision/g_colls.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "vehicles/C_car.h"
#include "vehicles/C_trf_c.h"

// Takes the entity into the group with that role (PS2 name; the entity already in it is left as it is): the group's
// events go and the entity's group number is the group's
// FUNCTION: GAME 0x5303d0
void ai_group::Add(C_entity *e, uint role)
{
    for (uint i = 0; i < members.size(); i++)
    {
        if (members[i].entity == e)
            return;
    }
    ai_group_member m;
    m.entity = e;
    m.role = role;
    members.push_back(m);
    events.clear();
    e->group_id = id;
}

// The entity leaves the group (PS2 name)
// FUNCTION: GAME 0x530460
void ai_group::Del(C_entity *e)
{
    for (uint i = 0; i < members.size(); i++)
    {
        if (members[i].entity == e)
            members.erase(members.begin() + i);
    }
}

// The entity that can receive tells the first member that has a position for the actor that it smells it: that
// member's ai_blob (the position, the actor's direction and the time) as ENTITY_EVENT_SNIFF (PS2 name)
// FUNCTION: GAME 0x5304c0
void ai_group::GetSniff(C_entity *e, C_actor *act)
{
    if (canReceive(e))
    {
        for (uint i = 0; i < members.size(); i++)
        {
            S_vector pos;
            if (members[i].entity->GetGroupPosition(act, &pos))
            {
                ai_blob *blob = new ai_blob;
                blob->pos = pos;
                blob->dir = act->frame->GetWorldDir();
                blob->time = act_mission->game->game_time;
                blob->flags = 0;
                blob->actor = act;
                e->Event(NULL, ENTITY_EVENT_SNIFF, blob);
                return;
            }
        }
    }
}

// An attack on the entity, which the members are told of a second later if it can cast (PS2 name; ProcessEvents)
// FUNCTION: GAME 0x5306c0
void ai_group::UnderAttack(C_entity *e, C_actor *attacker, float f)
{
    if (canCast(e))
    {
        ai_group_event ev;
        ev.type = 1;
        ev.time = act_mission->game->game_time + 1000;
        ev.entity = e;
        ev.attacker = attacker;
        ev.unknown_8 = f;
        events.push_back(ev);
    }
}

// The group's events whose time has come, from the last: an attack on a member that is alive (type 1) is told to the
// other members that can receive it, once for each attacker (attackers), then the event goes
// FUNCTION: GAME 0x530920
void ai_group::ProcessEvents(uint time)
{
    for (int i = events.size() - 1; i >= 0; i--)
    {
        if (events[i].time <= time)
        {
            if (events[i].entity->alive)
            {
                switch (events[i].type)
                {
                case 1:
                {
                    bool told = false;
                    for (uint j = 0; j < attackers.size(); j++)
                    {
                        if (attackers[j] == events[i].attacker)
                        {
                            told = true;
                            break;
                        }
                    }
                    if (!told)
                    {
                        for (uint k = 0; k < members.size(); k++)
                        {
                            if (members[k].entity != events[i].entity && canReceive(members[k].entity))
                                members[k].entity->Event(events[i].attacker, 1, &events[i].unknown_8);
                        }
                        attackers.push_back(events[i].attacker);
                    }
                }
                }
            }
            events.erase(events.begin() + i);
        }
    }
}

// How much the car counts for the group: the factor at the car's index, 1 for a car of none
// FUNCTION: GAME 0x530a70
float ai_group::GetCarFactor(C_car *car)
{
    for (uint i = 0; i < cars.size(); i++)
    {
        if (cars[i] == car && i < car_factors.size())
            return car_factors[i];
    }
    return 1.0f;
}

// The usable car closest to pos (within 50 m, squared 2500), NULL for none: the group's cars first (those that stand
// still enough, which a hit hasn't started to fade), where one is found it's the result, else the active actors'
// cars that stand still, the inactive ones and the parked traffic's; a car of the traffic goes through
// C_traffic_car::SebraliMiKaaaruSvine. The other two parameters aren't used, as PS2's
// FUNCTION: GAME 0x530ad0
C_car *ai_group::GetCarClosestAndUsable(S_vector *pos, S_vector *unknown, uint *unknown2)
{
    std::vector<uint> unused;
    float dist;
    C_car *best = NULL;
    float best_dist = 2500.0f;
    for (uint i = 0; i < cars.size(); i++)
    {
        C_car *car = cars[i];
        if ((uint)car->death_fade_time < CAR_DEATH_FADE_PARTY && !(car->speed > 1.0f))
        {
            S_vector d = car->GetWorldPos() - *pos;
            dist = d.y * d.y + d.z * d.z + d.x * d.x;
            if (car->IsCarUsable(3) && dist < best_dist)
            {
                best = car;
                best_dist = dist;
            }
        }
    }
    if (best)
        return best;
    best_dist = 2500.0f;
    std::vector<C_actor *> &active = act_mission->game->active_actors;
    int j;
    for (j = active.size() - 1; j >= 0; j--)
    {
        C_car *car = (C_car *)active[j];
        if (car->type == ACTOR_CAR && !(car->speed > 0.5f) && (uint)car->death_fade_time < CAR_DEATH_FADE_PARTY)
        {
            S_vector d = car->GetWorldPos() - *pos;
            dist = d.y * d.y + d.z * d.z + d.x * d.x;
            if (car->IsCarUsable(3) && dist < best_dist)
            {
                best = car;
                best_dist = dist;
            }
        }
    }
    std::vector<C_actor *> &inactive = act_mission->game->inactive_actors;
    for (j = inactive.size() - 1; j >= 0; j--)
    {
        C_car *car = (C_car *)inactive[j];
        if (car->type == ACTOR_CAR && (uint)car->death_fade_time < CAR_DEATH_FADE_PARTY)
        {
            S_vector d = car->GetWorldPos() - *pos;
            dist = d.y * d.y + d.z * d.z + d.x * d.x;
            if (car->IsCarUsable(3) && dist < best_dist)
            {
                best = car;
                best_dist = dist;
            }
        }
    }
    std::vector<C_traffic_car::S_ELEMENT *> &parking = C_traffic_car::GetParkingCars();
    for (int k = parking.size() - 1; k >= 0; k--)
    {
        C_car *car = parking[k]->car;
        if ((uint)car->death_fade_time < CAR_DEATH_FADE_PARTY)
        {
            S_vector d = car->GetWorldPos() - *pos;
            dist = d.y * d.y + d.z * d.z + d.x * d.x;
            if (car->IsCarUsable(3) && dist < best_dist)
            {
                best = car;
                best_dist = dist;
            }
        }
    }
    if (best)
    {
        if (best->traffic_car)
        {
            best = C_traffic_car::SebraliMiKaaaruSvine(best);
            if (best)
                return best;
        }
        else
            return best;
    }
    return NULL;
}

// A new group with the next number, which is the result (PS2 name)
// FUNCTION: GAME 0x530e90
uint ai_groups::NewGroup()
{
    ai_group g;
    g.id = ++last_id;
    g.attackers.clear();
    g.members.clear();
    g.events.clear();
    g.cars.clear();
    g.car_factors.clear();
    g.unknown_4 = 0;
    g.unknown_8 = 0;
    groups.push_back(g);
    return g.id;
}

// FUNCTION: GAME 0x5310d0
ai_group *ai_groups::GetGroup(uint id)
{
    for (uint i = 0; i < groups.size(); i++)
    {
        if (groups[i].id == id)
            return &groups[i];
    }
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

// The record's size: the last number and the groups' count, each group's four numbers and three counts, its members'
// roles with their names, its attackers', its cars' names and factors, then the free actors' count and names
// FUNCTION: GAME 0x5314d0
uint ai_groups::GetSize()
{
    uint size = 8;
    for (uint i = 0; i < groups.size(); i++)
    {
        size += 0x14;
        size += groups[i].members.size() * (4 + AIGROUPS_NAME_LEN) + 4;
        size += groups[i].attackers.size() * AIGROUPS_NAME_LEN + 4;
        size += groups[i].cars.size() * AIGROUPS_NAME_LEN;
        size += groups[i].cars.size() * 4;
    }
    size += 4;
    size += free_actors.size() * AIGROUPS_NAME_LEN;
    return size;
}

// The record Load reads, through MemWrite: the last number and the groups' count, then each group by what GetSize
// counts. The events aren't saved.
// FUNCTION: GAME 0x5315b0
bool ai_groups::Save(void **buf)
{
    MemWrite(buf, &last_id, 1, 4);
    uint n = groups.size();
    MemWrite(buf, &n, 1, 4);
    char name[AIGROUPS_NAME_LEN];
    uint i;
    for (i = 0; i < groups.size(); i++)
    {
        uint j;
        MemWrite(buf, &groups[i].id, 1, 4);
        MemWrite(buf, &groups[i].unknown_4, 1, 4);
        MemWrite(buf, &groups[i].unknown_8, 1, 4);
        MemWrite(buf, &groups[i].unknown_c, 1, 4);
        n = groups[i].members.size();
        MemWrite(buf, &n, 1, 4);
        for (j = 0; j < groups[i].members.size(); j++)
        {
            MemWrite(buf, &groups[i].members[j].role, 1, 4);
            sprintf(name, groups[i].members[j].entity->frame->GetName());
            MemWrite(buf, name, AIGROUPS_NAME_LEN, 1);
        }
        n = groups[i].attackers.size();
        MemWrite(buf, &n, 1, 4);
        for (j = 0; j < groups[i].attackers.size(); j++)
        {
            sprintf(name, groups[i].attackers[j]->frame->GetName());
            MemWrite(buf, name, AIGROUPS_NAME_LEN, 1);
        }
        n = groups[i].cars.size();
        MemWrite(buf, &n, 1, 4);
        for (j = 0; j < groups[i].cars.size(); j++)
        {
            sprintf(name, groups[i].cars[j]->frame->GetName());
            MemWrite(buf, name, AIGROUPS_NAME_LEN, 1);
            MemWrite(buf, &groups[i].car_factors[j], 1, 4);
        }
    }
    n = free_actors.size();
    MemWrite(buf, &n, 1, 4);
    for (i = 0; i < free_actors.size(); i++)
    {
        sprintf(name, free_actors[i]->frame->GetName());
        MemWrite(buf, name, AIGROUPS_NAME_LEN, 1);
    }
    return true;
}

// The groups from the record Save wrote, after Destroy: each group's numbers, then its members (a role and the name
// of an actor, kept if the mission has an entity of that name), attackers, cars (a name and a factor) and, at the
// end, the free actors, the actors by name (the PC's, where PS2's are by their numbers)
// FUNCTION: GAME 0x5318f0
bool ai_groups::Load(void **buf)
{
    ai_group g;
    uint n, num_groups;
    C_actor *act;
    char name[AIGROUPS_NAME_LEN];
    Destroy();
    MemRead(buf, &last_id, 1, 4);
    MemRead(buf, &num_groups, 1, 4);
    for (uint i = 0; i < num_groups; i++)
    {
        uint j;
        MemRead(buf, &g.id, 1, 4);
        MemRead(buf, &g.unknown_4, 1, 4);
        MemRead(buf, &g.unknown_8, 1, 4);
        MemRead(buf, &g.unknown_c, 1, 4);
        MemRead(buf, &n, 1, 4);
        g.members.clear();
        for (j = 0; j < n; j++)
        {
            ai_group_member m;
            MemRead(buf, &m.role, 1, 4);
            MemRead(buf, name, AIGROUPS_NAME_LEN, 1);
            m.entity = (C_entity *)act_mission->FindActorByName(name);
            if (m.entity && ((C_actor *)m.entity)->type == ACTOR_ENTITY)
                g.members.push_back(m);
        }
        MemRead(buf, &n, 1, 4);
        g.attackers.clear();
        for (j = 0; j < n; j++)
        {
            MemRead(buf, name, AIGROUPS_NAME_LEN, 1);
            act = act_mission->FindActorByName(name);
            if (act)
                g.attackers.push_back(act);
        }
        MemRead(buf, &n, 1, 4);
        g.events.clear();
        g.cars.clear();
        g.car_factors.clear();
        for (j = 0; j < n; j++)
        {
            float factor;
            C_car *car;
            MemRead(buf, name, AIGROUPS_NAME_LEN, 1);
            MemRead(buf, &factor, 1, 4);
            act = act_mission->FindActorByName(name);
            if (act && act->type == ACTOR_CAR)
            {
                car = (C_car *)act;
                g.cars.push_back(car);
                g.car_factors.push_back(factor);
            }
        }
        groups.push_back(g);
    }
    free_actors.clear();
    MemRead(buf, &n, 1, 4);
    for (uint k = 0; k < n; k++)
    {
        MemRead(buf, name, AIGROUPS_NAME_LEN, 1);
        act = act_mission->FindActorByName(name);
        if (act)
            free_actors.push_back(act);
    }
    return true;
}

// Forgets an actor: out of the groups' members, attackers and events (a group left empty goes), and out of the
// actors the groups let go
// FUNCTION: GAME 0x531dc0
void ai_groups::InvalidateActor(C_actor *act)
{
    int empty_group = -1;
    for (uint i = 0; i < groups.size(); i++)
    {
        int j;
        for (j = groups[i].members.size() - 1; j >= 0; j--)
        {
            if ((C_actor *)groups[i].members[j].entity == act)
                groups[i].members.erase(&groups[i].members[j]);
            if (!groups[i].members.size())
                empty_group = i;
        }
        for (j = groups[i].attackers.size() - 1; j >= 0; j--)
        {
            if (groups[i].attackers[j] == act)
                groups[i].attackers.erase(&groups[i].attackers[j]);
        }
        for (j = groups[i].events.size() - 1; j >= 0; j--)
        {
            if (groups[i].events[j].entity == (C_entity *)act || groups[i].events[j].attacker == act)
                groups[i].events.erase(&groups[i].events[j]);
        }
    }
    if (empty_group != -1)
        groups.erase(&groups[empty_group]);
    int k;
    for (k = free_actors.size() - 1; k >= 0; k--)
    {
        if (free_actors[k] == act)
            free_actors.erase(&free_actors[k]);
    }
}

// Whether the entity is a member that can receive the group's events: not of role 2 (PS2 name)
// FUNCTION: GAME 0x532050
bool ai_group::canReceive(C_entity *e)
{
    for (uint i = 0; i < members.size(); i++)
    {
        if (members[i].entity == e)
            return members[i].role != 2 ? true : false;
    }
    return false;
}

// Whether the entity is a member that can cast the group's events: not of role 1 (PS2 name)
// FUNCTION: GAME 0x532090
bool ai_group::canCast(C_entity *e)
{
    for (uint i = 0; i < members.size(); i++)
    {
        if (members[i].entity == e)
            return members[i].role != 1 ? true : false;
    }
    return false;
}

// Whether the group of that number has a car (PS2's the same; 0 is no group)
// FUNCTION: GAME 0x5320d0
bool ai_groups::hasCarGroup(uint id)
{
    if (id)
    {
        for (uint i = 0; i < groups.size(); i++)
        {
            if (groups[i].id == id)
            {
                if (groups[i].cars.size())
                    return true;
                return false;
            }
        }
    }
    return false;
}

// The record's copies, count elements of size bytes, the record's pointer moved past them (PS2 names). The linker folds them with the
// other classes' identical ones: the original kept this MemRead, ai_follow_manager's MemWrite
// FUNCTION: GAME 0x5334f0 FOLDED
void ai_groups::MemWrite(void **buf, void *src, uint count, uint size)
{
    memcpy(*buf, src, count * size);
    *buf = (uchar *)*buf + count * size;
}

// FUNCTION: GAME 0x532150 FOLDED
void ai_groups::MemRead(void **buf, void *dst, uint count, uint size)
{
    memcpy(dst, *buf, count * size);
    *buf = (uchar *)*buf + count * size;
}

// A follower leaves his group, which goes with its last follower (PS2 name)
// FUNCTION: GAME 0x532190
void ai_follow_manager::DelFollower(C_actor *act)
{
    for (int i = groups.size() - 1; i >= 0; i--)
    {
        ai_follow_group &g = groups[i];
        for (int j = g.followers.size() - 1; j >= 0; j--)
        {
            if (g.followers[j] == act)
            {
                g.followers.erase(&g.followers[j]);
                if (!g.followers.size())
                    groups.erase(groups.begin() + i);
            }
        }
    }
}

// The follower joins the leader's group, a new one for a leader without (PS2 name)
// FUNCTION: GAME 0x532320
void ai_follow_manager::AddFollower(C_actor *follower, C_actor *leader)
{
    DelFollower(follower);
    for (uint i = 0; i < groups.size(); i++)
    {
        if (leader == groups[i].leader)
        {
            groups[i].followers.push_back(follower);
            return;
        }
    }
    ai_follow_group *g = new ai_follow_group;
    g->leader = leader;
    g->followers.clear();
    g->followers.push_back(follower);
    g->flags = 0;
    g->state = FOLLOWSTATE_5;
    g->stop_end = 0;
    g->unknown_2c = 0;
    g->car = NULL;
    groups.push_back(*g);
    delete g;
}

// The same with the place the car takes the group to: the group is in FOLLOWSTATE_0 where the follower and the leader
// sit in one car (PS2 name)
// FUNCTION: GAME 0x532430
void ai_follow_manager::AddFollower(C_actor *follower, C_actor *leader, S_vector &pos)
{
    DelFollower(follower);
    for (uint i = 0; i < groups.size(); i++)
    {
        if (leader == groups[i].leader)
        {
            groups[i].followers.push_back(follower);
            groups[i].flags |= FOLLOWF_HAS_POS;
            groups[i].pos = pos;
            return;
        }
    }
    ai_follow_group *g = new ai_follow_group;
    g->leader = leader;
    g->followers.clear();
    g->followers.push_back(follower);
    g->pos = pos;
    g->flags = FOLLOWF_HAS_POS;
    if (((C_human *)leader)->car && ((C_human *)leader)->car == ((C_human *)follower)->car)
        g->state = FOLLOWSTATE_0;
    else
        g->state = FOLLOWSTATE_5;
    g->stop_end = 0;
    g->unknown_2c = 0;
    g->car = NULL;
    groups.push_back(*g);
    delete g;
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

// Whether the leader's car has the seats for all his followers (PS2 name, typo and all)
// FUNCTION: GAME 0x532f30
bool ai_follow_manager::CanFolloshipFollow(C_actor *act)
{
    for (uint i = 0; i < groups.size(); i++)
    {
        if (groups[i].leader == act)
        {
            if (act->type != ACTOR_PLAYER && act->type != ACTOR_ENTITY)
                return false;
            C_car *car = ((C_human *)act)->car;
            if (!car)
                return false;
            switch (car->type)
            {
            case ACTOR_CAR:
            {
                // the seats but the driver's that nobody sits in
                uint free_seats = 0;
                for (uint j = 1; j < car->seats.size(); j++)
                    if (!car->GetOwner(j))
                        free_seats++;
                if (free_seats < groups[i].followers.size())
                    return false;
                return true;
            }
            case ACTOR_RAILWAY:
            {
                // a C_railway's: its wagons (unknown_1a8 and the copies DuplicateFrom made, one more) times the
                // seats of one (the member at 0x1e0 isn't named yet)
                uint seats = (*(int *)((uchar *)car + 0x1a8) + 1) * *(int *)((uchar *)car + 0x1e0);
                if (seats < groups[i].followers.size())
                    return false;
                return true;
            }
            }
            return false;
        }
    }
    return false;
}

// The seat of the follower in the leader's car: the free seats go to the followers who are not in the car yet, in
// the order of the group, the railway's by the follower's place; -1 for none (PS2 name)
// FUNCTION: GAME 0x533070
int ai_follow_manager::GetCarSeat(C_actor *follower, C_actor *leader)
{
    for (uint i = 0; i < groups.size(); i++)
    {
        if (groups[i].leader == leader)
        {
            ai_follow_group &g = groups[i];
            if (leader->type != ACTOR_PLAYER && leader->type != ACTOR_ENTITY)
                return -1;
            C_car *car = ((C_human *)leader)->car;
            if (!car)
                return -1;
            if (car->type == ACTOR_CAR)
            {
                uint before = 0;
                for (uint j = 0; j < g.followers.size(); j++)
                {
                    C_actor *f = g.followers[j];
                    if (f == follower)
                    {
                        uint n = 0;
                        for (uint seat = 1; seat < car->seats.size(); seat++)
                        {
                            if (!car->GetOwner(seat))
                            {
                                if (n == before)
                                    return seat;
                                n++;
                            }
                        }
                        return -1;
                    }
                    if (((C_human *)f)->car != car && ((C_human *)f)->using_car != car)
                        before++;
                }
                return -1;
            }
            if (car->type == ACTOR_RAILWAY)
            {
                for (uint j = 0; j < g.followers.size(); j++)
                {
                    if (g.followers[j] == follower &&
                        j < (*(int *)((uchar *)car + 0x1a8) + 1) * *(int *)((uchar *)car + 0x1e0))
                        return j;
                }
            }
            return -1;
        }
    }
    return -1;
}

// Forgets an actor: a group that follows him goes, out of the others' followers and cars
// FUNCTION: GAME 0x5331c0
void ai_follow_manager::Invalidate(C_actor *act)
{
    for (int i = groups.size() - 1; i >= 0; i--)
    {
        ai_follow_group &g = groups[i];
        if (g.leader == act)
        {
            groups.erase(groups.begin() + i);
            continue;
        }
        if (g.car == (C_car *)act)
            g.car = NULL;
        for (int j = g.followers.size() - 1; j >= 0; j--)
        {
            if (g.followers[j] == act)
                g.followers.erase(&g.followers[j]);
        }
    }
    check_time = 0;
    Update();
}

// An actor's event for the groups he leads. 2: he is gone from the groups (DelFollower). 3, a follower's action button
// in a car the AI drives or has stopped: the chase ends and the car is theirs (FOLLOWSTATE_3). 4, the leader's: the
// driving car stops for 7 s (FOLLOWSTATE_4), a stopped one goes on to the followers getting out (FOLLOWSTATE_1).
// FUNCTION: GAME 0x5332e0
void ai_follow_manager::Event(uint event, C_actor *actor)
{
    uint i;
    uint j;
    switch (event)
    {
    case 2:
        DelFollower(actor);
        break;
    case 3:
        for (i = 0; i < groups.size(); i++)
        {
            ai_follow_group &g = groups[i];
            switch (g.state)
            {
            case FOLLOWSTATE_2:
            case FOLLOWSTATE_4:
                for (j = 0; j < g.followers.size(); j++)
                {
                    if (actor == g.followers[j])
                    {
                        C_car *car = ((C_human *)g.leader)->car;
                        if (car->type == ACTOR_CAR)
                        {
                            act_mission->GetGame()->GetSledovacka()->RemoveCar(car);
                            g.state = FOLLOWSTATE_3;
                            return;
                        }
                    }
                }
                break;
            }
        }
        break;
    case 4:
        for (i = 0; i < groups.size(); i++)
        {
            ai_follow_group &g = groups[i];
            switch (g.state)
            {
            case FOLLOWSTATE_2:
                if (g.leader == actor)
                {
                    g.state = FOLLOWSTATE_4;
                    g.stop_end = act_mission->GetGame()->GetGameTime() + 7000;
                    return;
                }
                break;
            case FOLLOWSTATE_4:
                if (g.leader == actor)
                {
                    g.state = FOLLOWSTATE_1;
                    g.stop_end = 0;
                    return;
                }
                break;
            }
        }
        break;
    }
}

// Whether the follower has to get into the leader's car: never (PS2 name)
// FUNCTION: GAME 0x533440
bool ai_follow_manager::mustEnter(C_actor *act)
{
    return false;
}

// Whether the follower has to get out: the group's state says so, or its FOLLOWF_MUSTLEAVE (PS2 name)
// FUNCTION: GAME 0x533450
bool ai_follow_manager::mustLeave(C_actor *act)
{
    for (uint i = 0; i < groups.size(); i++)
    {
        ai_follow_group &g = groups[i];
        for (uint j = 0; j < g.followers.size(); j++)
        {
            if (g.followers[j] == act)
            {
                switch (g.state)
                {
                case FOLLOWSTATE_1:
                    return true;
                case FOLLOWSTATE_0:
                case FOLLOWSTATE_2:
                case FOLLOWSTATE_3:
                case FOLLOWSTATE_4:
                case FOLLOWSTATE_5:
                    if (g.flags & FOLLOWF_MUSTLEAVE)
                        return true;
                    break;
                }
                return false;
            }
        }
    }
    return false;
}

// The record's copies, count elements of size bytes (the parameters the other way round from ai_groups'), the record's pointer moved past them (PS2 names). The linker folds them with the
// other classes' identical ones: the original kept this MemWrite, ai_groups' MemRead
// FUNCTION: GAME 0x5334f0 FOLDED
void ai_follow_manager::MemWrite(void **buf, void *src, uint size, uint count)
{
    memcpy(*buf, src, size * count);
    *buf = (uchar *)*buf + size * count;
}

// FUNCTION: GAME 0x532150 FOLDED
void ai_follow_manager::MemRead(void **buf, void *dst, uint size, uint count)
{
    memcpy(dst, *buf, size * count);
    *buf = (uchar *)*buf + size * count;
}

// The record's size: the time and the number of groups, then for each its number of followers and (the followers,
// the position and five words, the leader and the car) 32 bytes each: the names are 32 bytes, as Save writes them.
// FUNCTION: GAME 0x533530
uint ai_follow_manager::GetSize()
{
    uint size = sizeof(uint) * 2;
    for (uint i = 0; i < groups.size(); i++)
    {
        size += sizeof(uint);
        size += (groups[i].followers.size() + 3) * 32;
    }
    return size;
}

// The record: check_time, the number of groups and each group's number of followers, their names (32 bytes, the
// frame's), the position, unknown_1c, flags, state, stop_end, unknown_2c, then the leader's and the car's names
// ("<NONE>" for no car). The names go through sprintf, as ai_groups::Save's
// FUNCTION: GAME 0x5335a0
bool ai_follow_manager::Save(void **buf)
{
    uint count;
    char name[32];
    MemWrite(buf, &check_time, sizeof(uint), 1);
    count = groups.size();
    MemWrite(buf, &count, sizeof(uint), 1);
    for (uint i = 0; i < groups.size(); i++)
    {
        ai_follow_group &g = groups[i];
        count = g.followers.size();
        MemWrite(buf, &count, sizeof(uint), 1);
        for (uint j = 0; j < count; j++)
        {
            sprintf(name, g.followers[j]->frame->GetName());
            MemWrite(buf, name, 1, sizeof(name));
        }
        MemWrite(buf, &g.pos, sizeof(S_vector), 1);
        MemWrite(buf, &g.unknown_1c, sizeof(float), 1);
        MemWrite(buf, &g.flags, sizeof(uint), 1);
        MemWrite(buf, &g.state, sizeof(int), 1);
        MemWrite(buf, &g.stop_end, sizeof(uint), 1);
        MemWrite(buf, &g.unknown_2c, sizeof(uint), 1);
        sprintf(name, g.leader->frame->GetName());
        MemWrite(buf, name, 1, sizeof(name));
        if (g.car)
            sprintf(name, g.car->frame->GetName());
        else
            sprintf(name, "<NONE>");
        MemWrite(buf, name, 1, sizeof(name));
    }
    return true;
}

// The following from the record: the actors by their names, the followers only the entities, the leader a player or an
// entity and the car a car; a group left without a leader or followers is dropped.
// FUNCTION: GAME 0x5337b0
bool ai_follow_manager::Load(void **buf)
{
    ai_follow_group g;
    Destroy();
    uint count;
    uint n;
    char name[32];
    C_actor *act;
    MemRead(buf, &check_time, sizeof(uint), 1);
    MemRead(buf, &count, sizeof(uint), 1);
    for (; count > 0; count--)
    {
        MemRead(buf, &n, sizeof(uint), 1);
        g.followers.clear();
        for (uint j = n; j > 0; j--)
        {
            MemRead(buf, name, 1, sizeof(name));
            act = act_mission->FindActorByName(name);
            if (act && act->type == ACTOR_ENTITY)
                g.followers.push_back(act);
        }
        MemRead(buf, &g.pos, sizeof(S_vector), 1);
        MemRead(buf, &g.unknown_1c, sizeof(float), 1);
        MemRead(buf, &g.flags, sizeof(uint), 1);
        MemRead(buf, &g.state, sizeof(int), 1);
        MemRead(buf, &g.stop_end, sizeof(uint), 1);
        MemRead(buf, &g.unknown_2c, sizeof(uint), 1);
        MemRead(buf, name, 1, sizeof(name));
        act = act_mission->FindActorByName(name);
        g.leader = NULL;
        if (act && (act->type == ACTOR_PLAYER || act->type == ACTOR_ENTITY))
            g.leader = act;
        MemRead(buf, name, 1, sizeof(name));
        g.car = NULL;
        if (act && act->type == ACTOR_CAR)
            g.car = (C_car *)act;
        if (g.leader && g.followers.size())
            groups.push_back(g);
        // Dead calls: the original's budget ran out before the followers' clear's copy (padding, not source)
        g.followers.size();
        g.followers.size();
        g.followers.size();
        g.followers.size();
    }
    return true;
}

// The entity joins the party that hunts the actor and holds its group (not one in a car): the party takes it as a
// member, chooses its commander again and looks 1 s later. Without such a party a new one of the entity forms, which
// looks 0.5 s from now.
// FUNCTION: GAME 0x5339f0
void ai_fight_manager::JoinParty(C_entity *e, C_actor *act)
{
    ai_fight_followship np;
    ai_fight_member m;
    for (uint i = 0; i < parties.size(); i++)
    {
        ai_fight_followship &party = parties[i];
        if (party.target == act)
        {
            uint word = (uint)((C_entity *)party.commander)->unknown_f4c;
            if (word == *(volatile uint *)&e->unknown_f4c)
            {
                if (party.type != FIGHTPARTY_IN_CAR)
                {
                    for (uint j = 0; j < party.members.size(); j++)
                    {
                        if (e == party.members[j].entity)
                            return;
                    }
                    if (*(volatile uint *)&e->unknown_f4c == word)
                    {
                        m.entity = e;
                        party.members.push_back(m);
                        party.SetCommander();
                        party.check_time += 1000;
                        return;
                    }
                }
            }
        }
    }
    memset(&np, 0, sizeof(np));
    m.entity = e;
    m.same_group = e->group_id ? 1 : 0;
    m.unknown_10 = 0;
    np.members.push_back(m);
    np.target = (C_human *)act;
    np.check_time = act_mission->game->game_time + 500;
    np.type = FIGHTPARTY_ON_FOOT;
    np.unknown_34 = 0;
    np.car = NULL;
    np.SetCommander();
    parties.push_back(np);
    // Dead (budget): with it the vector's constructor in np is the original's, out of line
    parties.size();
}

// The entity, or else the actor, leaves its party: the party of its group whose target is the actor (any party
// for none). A party in a car that is left empty goes apart, another one goes, one with members chooses its
// commander again.
// FUNCTION: GAME 0x533c50
void ai_fight_manager::LeaveParty(C_entity *e, C_actor *act)
{
    uint i;
    if (!act)
    {
        for (i = 0; i < parties.size(); i++)
        {
            ai_fight_followship &party = parties[i];
            for (uint j = 0; j < party.members.size(); j++)
            {
                if (party.members[j].entity != e)
                    continue;
                party.members.erase(party.members.begin() + j);
                if (party.type == FIGHTPARTY_IN_CAR)
                {
                    if (!party.members.size())
                        DisbandCarParty(&party, false);
                }
                else if (!party.members.size())
                    parties.erase(parties.begin() + i);
                else
                    party.SetCommander();
                return;
            }
        }
    }
    else
    {
        for (i = 0; i < parties.size(); i++)
        {
            ai_fight_followship &party = parties[i];
            // The entities' word at 0xf4c (PS2 0x1134), which C_entity's header has as bytes
            if (party.target != act || (uint)((C_entity *)party.commander)->unknown_f4c != (uint)e->unknown_f4c)
                continue;
            for (uint j = 0; j < party.members.size(); j++)
            {
                if (party.members[j].entity != e)
                    continue;
                party.members.erase(party.members.begin() + j);
                if (party.type == FIGHTPARTY_IN_CAR)
                {
                    if (!party.members.size())
                        DisbandCarParty(&party, false);
                }
                else if (!party.members.size())
                    parties.erase(parties.begin() + i);
                else
                    party.SetCommander();
                // Dead (budget): the original's second erase inlines its destructor loop directly
                parties.size();
                return;
            }
        }
    }
}

// The party's commander again: the first member whose group has cars, else the first with a group, else the first
// member; each member then knows whether it is of the commander's group.
// FUNCTION: GAME 0x533fc0
void ai_fight_followship::SetCommander()
{
    int first = -1;
    uint i;
    for (i = 0; i < members.size(); i++)
    {
        ai_fight_member &m = members[i];
        uint group = m.entity->group_id;
        if (AIManager.groups.hasCarGroup(group))
        {
            commander = m.entity;
            goto same_groups;
        }
        if (first == -1 && group)
            first = i;
    }
    if (first != -1)
        commander = members[first].entity;
    else
        commander = members[0].entity;
same_groups:
    for (i = 0; i < members.size(); i++)
    {
        ai_fight_member &m = members[i];
        uint group = commander->group_id;
        if (group && group == m.entity->group_id)
            m.same_group = 1;
        else
            m.same_group = 0;
    }
}

// How many seats of the car the party may take: all of them, or none where a seat's owner isn't a member or where the
// way to the seat is blocked.
// FUNCTION: GAME 0x5340d0
uint ai_fight_followship::CountFreeSeats(C_car *car)
{
    uint count = car->seats.size();
    if (!count)
        return 0;
    C_human *owner = car->GetOwner(0);
    uint j;
    if (owner)
    {
        for (j = 0;; j++)
        {
            if (j >= members.size())
                return 0;
            if (members[j].entity == owner)
                break;
        }
    }
    for (uint i = 0; i < count; i++)
    {
        owner = car->GetOwner(i);
        if (owner)
        {
            for (j = 0;; j++)
            {
                if (j >= members.size())
                    return 0;
                if (members[j].entity == owner)
                    break;
            }
        }
        S_vector pos, dir;
        if (!car->GetPlacementFromTCK1(i, pos, dir, false))
            return 0;
        S_vector neg;
        neg = dir * -1.0f;
        if (gifCollision.TestLineH(pos, neg, NULL, NULL, 0x140502, 0))
            return 0;
    }
    return count;
}

// The party's members that take the car (PS2 name) become a new party of it, NULL for none (the caller copies it into
// the parties and deletes it): the car's free seats go to the members in no car or in this one that are of the
// commander's group, the commander first, and they leave this party
// FUNCTION: GAME 0x534270
ai_fight_followship *ai_fight_followship::TakeCar(C_car *car)
{
    bool commander_taken = false;
    uint seats = CountFreeSeats(car);
    if (!seats)
        return NULL;
    uint taken[8];
    uint count = 0;
    for (uint i = 0; i < members.size() && count < seats; i++)
    {
        C_entity *e = members[i].entity;
        if ((!e->car || e->car == car) && e->unknown_f4c != 2 && commander->group_id == e->group_id)
        {
            if (commander != e)
            {
                if (commander_taken)
                    taken[count++] = i;
                else
                {
                    taken[count + 1] = i;
                    count++;
                }
            }
            else
            {
                taken[0] = i;
                commander_taken = true;
                count++;
            }
        }
    }
    if (!count)
        return NULL;
    ai_fight_followship *party = AIManager.fight.NewCarParty(car, target);
    if (!party)
        return NULL;
    for (uint j = 0; j < count; j++)
        party->AddCarMember(members[taken[j]].entity);
    party->SetCommander();
    party->unknown_34 = 0;
    for (uint k = 0; k < count; k++)
        members[taken[k]].same_group = 2;
    for (int m = members.size() - 1; m >= 0; m--)
    {
        if (members[m].same_group == 2)
            members.erase(members.begin() + m);
    }
    if (members.size())
        SetCommander();
    return party;
}

// A new party in the car, on the target (PS2 name), which the caller fills
// FUNCTION: GAME 0x5344a0
ai_fight_followship *ai_fight_manager::NewCarParty(C_car *car, C_actor *target)
{
    ai_fight_followship *party = new ai_fight_followship;
    memset(party, 0, sizeof(*party));
    party->check_time = act_mission->GetGame()->GetGameTime();
    party->type = FIGHTPARTY_IN_CAR;
    party->target = (C_human *)target;
    party->car = car;
    party->unknown_c.x = 0.0f;
    party->unknown_c.y = 0.0f;
    party->unknown_c.z = 0.0f;
    party->stand_time = 0;
    car->death_fade_time = CAR_DEATH_FADE_PARTY;
    return party;
}

// A member of the car's party (PS2 name): one in the party's car at the seat of its number, or in a car standing
// still, goes on from the car's state, the other gets out
// FUNCTION: GAME 0x534520
void ai_fight_followship::AddCarMember(C_entity *e)
{
    ai_fight_member m;
    int how = 0;
    C_car *ecar = e->car;
    m.entity = e;
    if (ecar)
    {
        uint count = members.size();
        if (ecar == car)
        {
            if (count < car->seats.size())
                how = car->GetOwner(count) == (C_human *)e ? 2 : 1;
            else
                how = 1;
        }
        else if (ecar->type != ACTOR_CAR)
            how = 0;
        else if (ecar->speed < 0.5f)
            how = 1;
    }
    switch (how)
    {
    case 0:
        m.state = FIGHTSTATE_0;
        break;
    case 1:
        m.state = FIGHTSTATE_6;
        e->Use_Actor(e->car, HUMAN_USE_OUT, 0, 0);
        break;
    case 2:
        m.state = FIGHTSTATE_8;
        break;
    }
    m.unknown_8 = 0;
    m.same_group = 1;
    members.push_back(m);
}

// The party's position, the mean of its members', and the largest square of a member's distance from it (PS2 name)
// FUNCTION: GAME 0x534810
void ai_fight_followship::ComputePosition()
{
    pos = S_vector(0.0f, 0.0f, 0.0f);
    max_dist_sq = 0.0f;
    for (uint i = 0; i < members.size(); i++)
    {
        const S_vector &p = members[i].entity->frame->GetWorldPos();
        pos.x = p.x + pos.x;
        pos.y = p.y + pos.y;
        pos.z = p.z + pos.z;
    }
    float f = 1.0f / (float)members.size();
    pos.x *= f;
    pos.y *= f;
    pos.z *= f;
    for (uint j = 0; j < members.size(); j++)
    {
        const S_vector &p = members[j].entity->frame->GetWorldPos();
        float dx = p.x - pos.x;
        float dy = p.y - pos.y;
        float dz = p.z - pos.z;
        float dist = dx * dx + dy * dy + dz * dz;
        if (dist > max_dist_sq)
            max_dist_sq = dist;
    }
}

// Whether the party's car hunts the target (PS2 name): the target (the player or an entity) in a car that moves and
// has left the party by more than its spread, in a car of the party's kind or on foot further than 100 m, or on the
// rails further than 10 m
// FUNCTION: GAME 0x534980
bool ai_fight_followship::mustCarHunt()
{
    C_human *t = target;
    if (t->type != ACTOR_PLAYER && t->type != ACTOR_ENTITY)
        return false;
    I3D_frame *frame;
    C_car *tc = t->car;
    if (!tc)
        frame = t->frame;
    else
    {
        switch (tc->type)
        {
        case ACTOR_RAILWAY:
            {
                S_vector p = tc->frame->GetWorldPos();
                float dx = pos.x - p.x;
                float dy = pos.y - p.y;
                float dz = pos.z - p.z;
                if (dx * dx + dy * dy + dz * dz > 100.0f)
                    return true;
                return false;
            }
        case ACTOR_CAR:
            if (tc->speed > 2.0f)
            {
                S_vector p = tc->frame->GetWorldPos();
                float dx = pos.x - p.x;
                float dy = pos.y - p.y;
                float dz = pos.z - p.z;
                if (dx * dx + dy * dy + dz * dz > max_dist_sq)
                    return true;
            }
            frame = tc->frame;
            break;
        default:
            return false;
        }
    }
    S_vector p = frame->GetWorldPos();
    float dx = p.x - pos.x;
    float dz = p.z - pos.z;
    if (dx * dx + dz * dz > 10000.0f)
        return true;
    return false;
}

// The party in a car goes apart: its members leave the car's seats and drop their jobs, the party goes, and the
// members join parties of their own again, which with the flag hold their time until the next look 10 s more.
// FUNCTION: GAME 0x534b20
void ai_fight_manager::DisbandCarParty(ai_fight_followship *party, bool unknown)
{
    C_human *target = party->target;
    uint count = party->members.size();
    C_entity **entities = new C_entity *[count];
    if ((uint)party->car->death_fade_time >= CAR_DEATH_FADE_PARTY)
        party->car->death_fade_time -= CAR_DEATH_FADE_PARTY;
    uint i;
    for (i = 0; i < count; i++)
    {
        entities[i] = party->members[i].entity;
        party->members[i].entity->UnlockSubstate(3);
        party->members[i].entity->action_manager.Clear(false);
    }
    for (i = 0; i < parties.size(); i++)
    {
        if (party != &parties[i])
            continue;
        party->members.clear();
        parties.erase(parties.begin() + i);
        for (uint j = 0; j < count; j++)
            JoinParty(entities[j], target);
        if (unknown)
            AdjustFormTime(target);
        delete[] entities;
        break;
    }
}

// The time at which the next look at a party that hunts the actor and stands on foot (type 0) is 10 s later.
// FUNCTION: GAME 0x534d50
void ai_fight_manager::AdjustFormTime(C_actor *act)
{
    for (uint i = 0; i < parties.size(); i++)
    {
        if (parties[i].target == act && parties[i].type == FIGHTPARTY_ON_FOOT)
            parties[i].check_time += 10000;
    }
}

// The party of the entity, or else of the actor: the one it is a member of, with the member's index in *index. With an
// actor, only the parties that hunt it.
// FUNCTION: GAME 0x534db0
ai_fight_followship *ai_fight_manager::GetParty(C_entity *e, C_actor *act, uint *index)
{
    for (uint i = 0; i < parties.size(); i++)
    {
        ai_fight_followship &party = parties[i];
        if (act)
        {
            if (act == party.target)
            {
                for (uint j = 0; j < party.members.size(); j++)
                {
                    if (party.members[j].entity == e)
                    {
                        *index = j;
                        return &party;
                    }
                }
            }
        }
        else
        {
            for (uint j = 0; j < party.members.size(); j++)
            {
                if (party.members[j].entity == e)
                {
                    *index = j;
                    return &party;
                }
            }
        }
    }
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

// Forgets an actor: a party that hunts him or drives his car is disbanded (a car party's chase ends), he leaves the
// others' members, and a party left without members goes
// FUNCTION: GAME 0x536200
void ai_fight_manager::Invalidate(C_actor *act)
{
    for (int i = parties.size() - 1; i >= 0; i--)
    {
        ai_fight_followship &party = parties[i];
        if (party.target == (C_human *)act)
        {
            if (party.type == FIGHTPARTY_IN_CAR)
            {
                act_mission->GetGame()->GetSledovacka()->RemoveCar(party.car);
                DisbandCarParty(&party, false);
            }
            else
                parties.erase(parties.begin() + i);
        }
        else if (party.car == (C_car *)act)
            DisbandCarParty(&party, false);
        else
        {
            for (int j = party.members.size() - 1; j >= 0; j--)
            {
                if ((C_actor *)party.members[j].entity == act)
                    party.members.erase(&party.members[j]);
            }
            if (!party.members.size())
            {
                if (party.type == FIGHTPARTY_IN_CAR)
                {
                    act_mission->GetGame()->GetSledovacka()->RemoveCar(party.car);
                    DisbandCarParty(&party, false);
                }
                else
                    parties.erase(parties.begin() + i);
            }
            else
                party.SetCommander();
        }
    }
}
