// The chases (PS2 C_sledovacka.cpp; the PC's file is the one at 0x4a0320-0x4aa2ac, docs/game-files.md), which starts
// with the constructor. Written so far: the constructor and the destructor, which the game's constructor and destructor
// call, Init and Done, which its Init and Done call, and Tick, which its Tick calls; what the script's commands call
// (RemoveCar, SetActionStatus) and what Tick calls of the file's (RecompilePath, PreparePath, TickDriver) are stubs.
// Open: Tick (18 KB) is in VC6's large-function regime, where more source means less inlining at the end: the original
// calls helpers out of line from the 0x800 branch's side search on (its first, an S_vector operator- at 0x4a76fc) that
// ours inlines. About 250 dead statements anywhere in Tick give the original's decisions in the whole function (64% to
// 86%), so the original had that much more source there, or a costlier helper; not found yet. Its frame is 0x6f8, the
// original's 0x744.

#include "ai/C_sledovacka.h"

#include "actors/C_actor.h"
#include "ai/C_driver.h"
#include "ai/road_web.h"
#include "actors/C_human.h"
#include "collision/g_colls.h"
#include "mission/Mission.h"
#include "vehicles/C_car.h"
#include "vehicles/C_trf_c.h"

#include "stub.h"

// FUNCTION: GAME 0x4a0320
C_sledovacka::C_sledovacka() :
    unknown_20(0)
{
}

// The same code as the force feedback's destructor, whose copy the linker kept (0x444670): the marker is that one's
C_sledovacka::~C_sledovacka()
{
}

// The bridges from the mission's last actor, as PS2's: the road node nearer to the bridge of the two ends of the link
// GetNearNode finds. The mission's roads and actors are locals, and the index is masked again where the node is taken,
// as PS2's.
// FUNCTION: GAME 0x4a0360
void C_sledovacka::Init()
{
    C_roads *roads = act_mission->roads;
    std::vector<C_actor *> &actors = act_mission->actors;
    for (int i = actors.size(); i--;)
    {
        if (actors[i]->GetType() == ACTOR_BRIDGE)
        {
            S_BRIDGE b;
            b.actor = actors[i];
            ushort n = roads->GetNearNode(actors[i]->GetWorldPos()) & NEARNODE_INDEX;
            S_vector d0 = roads->nodes[roads->links[n].unknown_14].pos - actors[i]->GetWorldPos();
            S_vector d1 = roads->nodes[roads->links[n].unknown_16].pos - actors[i]->GetWorldPos();
            if (d0.Square() < d1.Square())
                b.node = roads->links[n & NEARNODE_INDEX].unknown_14;
            else
                b.node = roads->links[n & NEARNODE_INDEX].unknown_16;
            bridges.push_back(b);
        }
    }
}

// The cars from the last one, as PS2's
// FUNCTION: GAME 0x4a0660
void C_sledovacka::Done()
{
    for (int i = elements.size(); i--;)
        delete elements[i].driver;
    elements.clear();
    bridges.clear();
}

#pragma auto_inline(off)

// STUB: GAME 0x4a1c30
void C_sledovacka::RemoveCar(C_car *car)
{
    StubLog("C_sledovacka::RemoveCar");
}

#pragma auto_inline(on)

// What the paths' nodes index (the name is ours)
// GLOBAL: GAME 0x624ba4
static const int unknown_624ba4[10] = {1, 1, 1, 1, 1, 4, 4, 4, 4, 2};

// The chases' cars by the time passed, at most 83 ms, from the last one, as PS2's: by their modes (the flags 1, 2
// and 4 chase the target along the roads, straight after it and beside it; 0x100010, 0x40 and 0x20 drive the
// driver's own path), then what ends a chase (the status) and the brakes; a second pass stops the cars whose chase
// ended. The function is too large for VC6's inline budget: from the middle of the chases beside the target on,
// its calls to inline functions are out of line, as the original's.
// FUNCTION: GAME 0x4a3e70
void C_sledovacka::Tick(uint time)
{
    if (time > 83)
        time = 83;
    int i;
    for (i = elements.size(); i--;)
    {
        S_ELEMENT &e = elements[i];
        if (e.car->act_state == ACTSTATE_1)
            continue;
        if (e.car->act_state == ACTSTATE_2)
            e.status = 2;
        if (e.status)
            continue;
        // The target: unknown_c's car while it sits in one
        if (e.unknown_c)
        {
            if (((C_human *)e.unknown_c)->car)
                e.unknown_8 = ((C_human *)e.unknown_c)->car;
            else
                e.unknown_8 = e.unknown_c;
        }
        // Along the roads: a path of the roads' nodes to the target, found again once the car has passed its last
        // node but one; every 200 ms a line to the target, which, free within 120 m, switches to mode 2
        bool near_target = false;
        if (e.flags & 1)
        {
            if (e.driver->unknown_170 >= e.driver->unknown_160.size() - 1 || !e.driver->unknown_160.size())
            {
                ushort from = unknown_20->GetNearNode(e.car->GetPos()) | NEARNODE_FLAG;
                ushort to;
                if (e.unknown_8)
                    to = unknown_20->GetNearNode(e.unknown_8->GetPos()) | NEARNODE_FLAG;
                else
                    to = unknown_20->GetNearNode(e.unknown_10) | NEARNODE_FLAG;
                if (from != to && from != NEARNODE_NONE && to != NEARNODE_NONE)
                {
                    std::vector<ushort> path;
                    unknown_20->FindPath(from, to, path);
                    e.driver->unknown_160.clear();
                    for (uint j = 0; j < path.size(); j++)
                    {
                        S_car_node node;
                        memset(&node, 0, sizeof(node));
                        node.unknown_60 = path[j];
                        e.driver->unknown_160.push_back(node);
                    }
                    if (PreparePath(i))
                    {
                        if (!RecompilePath(i))
                        {
                            e.driver->unknown_160.clear();
                            e.driver->unknown_170 = 0;
                        }
                    }
                    else
                    {
                        e.driver->unknown_160.clear();
                        e.driver->unknown_170 = 0;
                    }
                    for (int k = e.driver->unknown_160.size(); k--;)
                        e.driver->unknown_160[k].unknown_48 = 1000.0f;
                    S_car_node node;
                    memset(&node, 0, sizeof(node));
                    node.pos = e.car->unknown_398;
                    node.unknown_44 = false;
                    node.unknown_45 = true;
                    node.unknown_46 = true;
                    node.unknown_48 = 1000.0f;
                    node.unknown_4c = 1.5f;
                    node.unknown_54 = true;
                    e.driver->unknown_160.insert(e.driver->unknown_160.begin(), node);
                    if (!e.unknown_8)
                    {
                        node.unknown_44 = false;
                        node.unknown_45 = true;
                        node.unknown_46 = true;
                        node.unknown_48 = 10.0f;
                        node.unknown_4c = 1.5f;
                        node.unknown_54 = true;
                        node.pos = e.unknown_10;
                        e.driver->unknown_160.push_back(node);
                    }
                }
                e.driver->unknown_170 = 1;
                if (e.unknown_28)
                    e.unknown_3c = -1.0f;
                else
                    e.unknown_3c = 10000.0f;
                e.unknown_28 = 1;
            }
            // The target 1 m up, or the place
            TickDriver(i, time);
            e.unknown_2c += time;
            if (e.unknown_2c > 200)
            {
                e.unknown_2c = 0;
                S_vector dir;
                if (e.unknown_8)
                {
                    if (e.unknown_8->GetType() == ACTOR_CAR)
                        dir = ((C_car *)e.unknown_8)->unknown_398 - e.car->unknown_398;
                    else
                        dir = (e.unknown_8->GetWorldPos() + S_vector(0.0f, 1.0f, 0.0f)) - e.car->unknown_398;
                }
                else
                    dir = (e.unknown_10 + S_vector(0.0f, 1.0f, 0.0f)) - e.car->unknown_398;
                if (dir.Magnitude() < 120.0f)
                {
                    if (!gifCollision.TestLineHStatic(e.car->unknown_398, dir, NULL, NULL, 0))
                    {
                        if (!(e.driver->unknown_1dc & 0x10000000))
                        {
                        e.flags = e.flags & ~1 | 2;
                        S_car_node node;
                        memset(&node, 0, sizeof(node));
                        node.pos = e.car->unknown_398;
                        node.unknown_44 = false;
                        node.unknown_45 = true;
                        node.unknown_46 = true;
                        node.unknown_48 = 1000.0f;
                        node.unknown_4c = 3.0f;
                        node.unknown_54 = true;
                        e.driver->unknown_160.clear();
                        e.driver->unknown_160.push_back(node);
                        if (e.unknown_8)
                        {
                            if (e.unknown_8->GetType() == ACTOR_CAR)
                                node.pos = ((C_car *)e.unknown_8)->unknown_398;
                            else
                                node.pos = e.unknown_8->GetWorldPos() + S_vector(0.0f, 0.2f, 0.0f);
                        }
                        else
                            node.pos = e.unknown_10;
                        node.unknown_30 = (node.pos - e.car->unknown_398).Magnitude();
                        e.driver->unknown_160.push_back(node);
                        e.driver->unknown_170 = 1;
                        e.driver->Tick(time);
                        e.unknown_2c = 0;
                        e.unknown_28 = 0;
                        }
                    }
                    else if (!e.driver->unknown_160.size())
                        e.status = 2;
                }
            }
            // Further than 100 m away the car is invisible, without its dynamic collisions
            if (e.unknown_8)
            {
                if (!e.car->GetUnknown5f() && (e.unknown_8->GetWorldPos() - e.car->unknown_398).Square() > 10000.0f)
                {
                    e.car->invisible = true;
                    e.car->SetInvisible(true);
                }
                else
                {
                    e.car->invisible = false;
                    e.car->SetInvisible(false);
                }
            }
            else
            {
                if (!e.car->GetUnknown5f() && (e.unknown_10 - e.car->unknown_398).Square() > 10000.0f)
                {
                    e.car->invisible = true;
                    e.car->SetInvisible(true);
                }
                else
                {
                    e.car->invisible = false;
                    e.car->SetInvisible(false);
                }
            }
            if (e.unknown_8)
            {
                if ((float)e.driver->unknown_170 > e.unknown_3c)
                    e.unknown_28 += time;
                if ((float)e.unknown_28 > e.unknown_38)
                    e.status = 2;
            }
            // What ends the chase: a car without energy (4), a wrecked car, a broken wheel or a car on its roof for
            // 10 s (1 within 50 m of the target, else 2), a driver stuck for 20 s (1)
            int show = e.car->SetShowEnergy(1);
            if (e.car->ShowCarEnergy() == 0.0f)
                e.status = 4;
            e.car->SetShowEnergy(show);
            if (e.unknown_8)
            {
                if ((e.unknown_8->GetWorldPos() - e.car->unknown_398).Square() < 2500.0f)
                    near_target = true;
            }
            else
            {
                if ((e.unknown_10 - e.car->unknown_398).Square() < 2500.0f)
                    near_target = true;
            }
            show = e.car->SetShowEnergy(1);
            if (e.car->ShowCarEnergy() < 0.15f)
                e.status = near_target ? 1 : 2;
            e.car->SetShowEnergy(show);
            for (int n = e.car->GetNumWheels(); n--;)
            {
                if (e.car->GetWheel(n)->unknown_120 & 0x40000000)
                {
                    e.status = near_target ? 1 : 2;
                    break;
                }
            }
            if (e.car->GetUnknownC54().AngleTo(S_vector(0.0f, 1.0f, 0.0f)) > 0.52f)
            {
                if (e.car->GetSpeed() < 3.0f)
                {
                    e.unknown_2c += time;
                    if (e.unknown_2c > 10000)
                        e.status = near_target ? 1 : 2;
                }
                else
                    e.unknown_2c = 0;
            }
            if (e.driver->unknown_220 > 20000 && (e.car->unknown_2214 || e.car->unknown_2215) &&
                e.car->GetUnknown5f())
                e.status = 1;
        }
        // Straight after the target: its places appended to the path (passed nodes dropped but four), every 500 ms a
        // line to it; near enough (8 m, more by the angle between the two cars' velocities) it switches to mode 4
        else if (e.flags & 2)
        {
            if (!(e.driver->unknown_1dc & 0x10000000))
            {
                while (e.driver->unknown_170 > 4)
                {
                    e.driver->unknown_160.erase(e.driver->unknown_160.begin());
                    e.driver->unknown_170--;
                    e.unknown_3c -= 1.0f;
                }
                float max_dist = 5.0f;
                S_car_node node;
                if (e.unknown_8)
                {
                    if (e.unknown_8->GetType() == ACTOR_CAR && ((C_car *)e.unknown_8)->GetSpeed() > 10.0f)
                        max_dist = ((C_car *)e.unknown_8)->GetSpeed() * 0.3f + 5.0f;
                    node = e.driver->unknown_160[e.driver->unknown_160.size() - 1];
                    float dist;
                    if (e.unknown_8->GetType() == ACTOR_CAR)
                        dist = (node.pos - ((C_car *)e.unknown_8)->unknown_398).Magnitude();
                    else
                        dist = (node.pos - e.unknown_8->GetWorldPos()).Magnitude();
                    if (dist > max_dist)
                    {
                        if (e.unknown_8->GetType() == ACTOR_CAR)
                            node.pos = ((C_car *)e.unknown_8)->unknown_398;
                        else
                            node.pos = e.unknown_8->GetWorldPos();
                        node.unknown_30 = dist;
                        e.driver->unknown_160.push_back(node);
                    }
                }
                else
                {
                    node = e.driver->unknown_160[e.driver->unknown_160.size() - 1];
                    float dist = (node.pos - e.unknown_10).Magnitude();
                    if (dist > max_dist)
                    {
                        node.pos = e.unknown_10;
                        node.unknown_30 = dist;
                        e.driver->unknown_160.push_back(node);
                    }
                }
                TickDriver(i, time);
                if (e.unknown_8)
                {
                    e.unknown_2c += time;
                    if (e.unknown_2c > 500)
                    {
                        e.unknown_2c = 0;
                        S_vector dir;
                        if (e.unknown_8->GetType() == ACTOR_CAR)
                            dir = ((C_car *)e.unknown_8)->unknown_398 - e.car->unknown_398;
                        else
                            dir = (e.unknown_8->GetWorldPos() + S_vector(0.0f, 1.0f, 0.0f)) - e.car->unknown_398;
                        if (dir.Magnitude() < 100.0f &&
                            !gifCollision.TestLineHStatic(e.car->unknown_398, dir, NULL, NULL, 0))
                        {
                            e.unknown_28 = 0;
                            e.unknown_3c = 100000.0f;
                        }
                        else
                            e.unknown_3c = -1.0f;
                    }
                }
                else
                {
                    e.unknown_28 = 0;
                    e.unknown_3c = 100000.0f;
                }
                if (e.unknown_8 != e.unknown_c)
                {
                    e.driver->unknown_1fc = e.unknown_c;
                    e.driver->unknown_200 = e.unknown_8;
                    e.driver->unknown_204 = NULL;
                    if (e.unknown_8->GetType() == ACTOR_CAR && ((C_car *)e.unknown_8)->GetSpeed() < 2.0f)
                    {
                        e.driver->unknown_1fc = NULL;
                        e.driver->unknown_200 = NULL;
                        e.driver->unknown_204 = e.unknown_8;
                    }
                }
                else
                {
                    e.driver->unknown_1fc = NULL;
                    e.driver->unknown_200 = NULL;
                    e.driver->unknown_204 = e.unknown_8;
                }
                float dist;
                if (e.unknown_8 && e.unknown_8->GetType() == ACTOR_CAR && e.car->GetSpeed() > 8.0f)
                {
                    float angle = e.car->GetVelocityRef().AngleTo(((C_car *)e.unknown_8)->GetVelocityRef());
                    if ((e.unknown_8->GetPos() - e.car->GetPos()).Dot(e.car->GetVelocityRef()) < 0.0f)
                        angle = 0.0f;
                    dist = angle * 40.0f / PI + 8.0f;
                }
                else
                    dist = 8.0f;
                dist *= dist;
                if ((node.pos - e.car->unknown_398).Square() < dist)
                {
                    e.unknown_28 = 0;
                    e.unknown_2c = 0;
                    e.flags = e.flags & ~2 | 4;
                    if (e.unknown_8 == e.unknown_c)
                        e.status = 1;
                }
            }
            else
                e.driver->Tick(time);
            // Further than 100 m away the car is invisible, without its dynamic collisions
            if (e.unknown_8)
            {
                if (!e.car->GetUnknown5f() && (e.unknown_8->GetWorldPos() - e.car->unknown_398).Square() > 10000.0f)
                {
                    e.car->invisible = true;
                    e.car->SetInvisible(true);
                }
                else
                {
                    e.car->invisible = false;
                    e.car->SetInvisible(false);
                }
            }
            else
            {
                if (!e.car->GetUnknown5f() && (e.unknown_10 - e.car->unknown_398).Square() > 10000.0f)
                {
                    e.car->invisible = true;
                    e.car->SetInvisible(true);
                }
                else
                {
                    e.car->invisible = false;
                    e.car->SetInvisible(false);
                }
            }
            if (e.unknown_8)
            {
                if ((float)e.driver->unknown_170 > e.unknown_3c)
                    e.unknown_28 += time;
                if ((float)e.unknown_28 > e.unknown_38)
                    e.status = 2;
            }
            int show = e.car->SetShowEnergy(1);
            if (e.car->ShowCarEnergy() == 0.0f)
                e.status = 4;
            e.car->SetShowEnergy(show);
            if (e.unknown_8)
            {
                if ((e.unknown_8->GetWorldPos() - e.car->unknown_398).Square() < 2500.0f)
                    near_target = true;
            }
            else
            {
                if ((e.unknown_10 - e.car->unknown_398).Square() < 2500.0f)
                    near_target = true;
            }
            show = e.car->SetShowEnergy(1);
            if (e.car->ShowCarEnergy() < 0.15f)
                e.status = near_target ? 1 : 2;
            e.car->SetShowEnergy(show);
            for (int n = e.car->GetNumWheels(); n--;)
            {
                if (e.car->GetWheel(n)->unknown_120 & 0x40000000)
                {
                    e.status = near_target ? 1 : 2;
                    break;
                }
            }
            if (e.car->GetUnknownC54().AngleTo(S_vector(0.0f, 1.0f, 0.0f)) > 0.52f)
            {
                if (e.car->GetSpeed() < 3.0f)
                {
                    e.unknown_2c += time;
                    if (e.unknown_2c > 10000)
                        e.status = near_target ? 1 : 2;
                }
                else
                    e.unknown_2c = 0;
            }
            if (e.driver->unknown_220 > 20000 && (e.car->unknown_2214 || e.car->unknown_2215) &&
                e.car->GetUnknown5f())
                e.status = 1;
        }
        // Beside the target: a path of two nodes, the car's place and the target's half a second ahead
        else if (e.flags & 4)
        {
            if (!(e.driver->unknown_1dc & 0x10000000))
            {
                S_car_node node;
                memset(&node, 0, sizeof(node));
                node.pos = e.car->unknown_398;
                node.unknown_44 = false;
                node.unknown_45 = true;
                node.unknown_46 = true;
                node.unknown_48 = 1000.0f;
                node.unknown_4c = 2.0f;
                node.unknown_54 = true;
                e.driver->unknown_160.clear();
                e.driver->unknown_160.push_back(node);
                float max_dist;
                if (e.unknown_8 && e.unknown_8->GetType() == ACTOR_CAR && e.car->GetSpeed() > 8.0f)
                {
                    float angle = e.car->GetVelocityRef().AngleTo(((C_car *)e.unknown_8)->GetVelocityRef());
                    if ((e.unknown_8->GetPos() - e.car->GetPos()).Dot(e.car->GetVelocityRef()) < 0.0f)
                        angle = 0.0f;
                    max_dist = angle * 40.0f / PI + 10.0f;
                }
                else
                    max_dist = 10.0f;
                float max_dist_sq = max_dist * max_dist;
                if (e.unknown_8)
                {
                    if (e.unknown_8->GetType() == ACTOR_CAR)
                        node.pos = ((C_car *)e.unknown_8)->unknown_398;
                    else
                        node.pos = e.unknown_8->GetWorldPos();
                }
                else
                    node.pos = e.unknown_10;
                float dist_sq = (node.pos - e.car->unknown_398).Square();
                if (dist_sq > max_dist_sq)
                {
                    e.unknown_28 = 0;
                    e.unknown_2c = 0;
                    e.unknown_3c = 100000.0f;
                    e.flags = e.flags & ~4 | 2;
                }
                else
                {
                    S_vector v;
                    if (e.unknown_8 && e.unknown_8->GetType() == ACTOR_CAR)
                        v = ((C_car *)e.unknown_8)->GetVelocityRef() * 0.5f;
                    else
                        v.Zero();
                    node.pos += v;
                }
                node.unknown_30 = (node.pos - e.driver->unknown_160[0].pos).Magnitude();
                e.driver->unknown_160.push_back(node);
                e.driver->unknown_170 = 1;
                if (!(e.flags & 0xa0000000))
                    e.driver->Tick(time);
                if (e.flags & 0x100)
                {
                    if ((node.pos - e.car->unknown_398).Square() < 64.0f)
                        e.car->SetBrake(1.0f);
                    e.driver->unknown_1fc = NULL;
                    e.driver->unknown_200 = NULL;
                    e.driver->unknown_204 = e.unknown_8;
                }
                else if ((e.flags & 0x2000200) && !(e.flags & 0xc000000))
                {
                    C_actor *stopped = e.car->unknown_20cc;
                    e.car->unknown_20cc = NULL;
                    if (stopped == e.unknown_8 && dist_sq < 64.0f)
                    {
                        e.car->unknown_20cc = e.unknown_8;
                        e.car->SetBrake(1.0f);
                        e.flags &= ~0x2000000;
                    }
                    if (e.flags & 0x200)
                    {
                        bool found1 = false;
                        bool found2 = false;
                        bool found3 = false;
                        for (int j = elements.size(); j--;)
                        {
                            if (j != i && elements[j].unknown_8 == e.unknown_8)
                            {
                                if ((elements[j].flags & 0xc001400) && (elements[j].flags & 0xc0000000))
                                {
                                    if (elements[j].flags & 0x1000000)
                                        found2 = true;
                                    else
                                        found1 = true;
                                }
                                if (elements[j].flags & 0x2000200)
                                    found3 = true;
                            }
                        }
                        if (found3 && (!found2 || !found1) && !(e.flags & 0x200000))
                        {
                            e.flags |= (rand() % 100 > 50 ? 0x4000000 : 0x8000000) | 0x81000000;
                            if (!found1)
                                e.flags &= ~0x1000000;
                        }
                    }
                    e.driver->unknown_1fc = e.unknown_c;
                    e.driver->unknown_200 = e.unknown_8;
                    e.driver->unknown_204 = NULL;
                }
                else
                {
                    // The chases after a car (0xc001400) and the cars that follow one (0x800), only after a car.
                    // Their points are the target's navigation points (beside it, a side by 0x1000000), the
                    // driver's path its second node.
                    if (e.flags & 0xc001400)
                    {
                        if (e.unknown_8->GetType() == ACTOR_CAR)
                        {
                            if (e.flags & 0x40000000)
                            {
                                // Beside the target, until it is behind, too near or turned away (the second change
                                // of the flags by a flag, as PS2's; VC6 threads it away)
                                bool done = false;
                                e.driver->unknown_1fc = e.unknown_c;
                                e.driver->unknown_200 = e.unknown_8;
                                e.driver->unknown_204 = NULL;
                                S_vector d = e.car->GetAIPos(true) - ((C_car *)e.unknown_8)->GetAIPos(false);
                                float dist = d.Square();
                                if (e.flags & 0x400)
                                {
                                    if (d.Dot(e.car->velocity) < 0.0f || dist < 0.36f ||
                                        (dist < 1.0f && d.AngleTo(e.car->unknown_c3c) < 1.3f))
                                    {
                                        e.flags &= ~0x40000000;
                                        done = true;
                                    }
                                }
                                else
                                {
                                    C_car *target = (C_car *)e.unknown_8;
                                    S_vector wheel =
                                        target->GetWheel(target->GetNumWheels() - 1)->frame->GetWorldPos();
                                    if ((wheel - e.car->GetAIPos(true)).Dot(e.car->velocity) < 0.0f)
                                        e.car->SetBrake(0.01f);
                                    if (e.car->GetSpeed() - ((C_car *)e.unknown_8)->GetSpeed() > 1.0f)
                                        e.car->SetBrake(1.0f);
                                    if (d.AngleTo(e.car->velocity) > 2.1f || dist < 0.36f ||
                                        (dist < 1.0f && d.AngleTo(e.car->unknown_c3c) < 1.3f))
                                    {
                                        e.flags &= ~0x40000000;
                                        done = true;
                                    }
                                }
                                if (done)
                                    e.flags &= ~0xc000000;
                            }
                            else if (e.flags & 0x80000000)
                            {
                                // To the target's side, ahead by the target's velocity, as long as nothing is in
                                // the way; beside it (0x40000000) once there and both go the same way
                                S_vector p1, p2, p3, p4;
                                ((C_car *)e.unknown_8)->GetNavigationPoints(e.car->GetVehicleSize(false).x + 0.15f,
                                    p1, p2, p3, p4);
                                if (!(e.flags & 0x1000000))
                                    p1 = p2;
                                p1 *= e.unknown_8->GetFrame()->GetMatrix();
                                node.pos = p1;
                                S_vector dir = ((C_car *)e.unknown_8)->velocity;
                                dir.Normalize();
                                dir *= e.car->GetVehicleSize(false).z;
                                dir = node.pos + dir - e.car->GetAIPos(true);
                                if (!gifCollision.TestLineH(e.car->GetAIPos(true), dir, NULL, NULL, 0xd02, 0))
                                {
                                    e.driver->unknown_160[1].pos = node.pos;
                                    e.driver->unknown_160[1].unknown_30 =
                                        (node.pos - e.driver->unknown_160[0].pos).Magnitude();
                                    if (e.flags & 0x400)
                                    {
                                        if ((((C_car *)e.unknown_8)->GetAIPos(true) - e.car->GetAIPos(true))
                                            .AngleTo(e.car->velocity) > 1.2f)
                                        {
                                            e.flags &= ~0x80000000;
                                            if (((C_car *)e.unknown_8)->velocity.AngleTo(e.car->velocity) < 0.5f)
                                                e.flags |= 0x40000000;
                                        }
                                    }
                                    else
                                    {
                                        if ((e.car->GetAIPos(true) - ((C_car *)e.unknown_8)->GetAIPos(false))
                                            .AngleTo(e.car->velocity) < 1.2f)
                                        {
                                            e.flags &= ~0x80000000;
                                            if (((C_car *)e.unknown_8)->velocity.AngleTo(e.car->velocity) < 0.5f)
                                                e.flags |= 0x40000000;
                                        }
                                    }
                                    e.driver->Tick(time);
                                }
                                else
                                {
                                    e.flags &= ~0x80000000;
                                    e.car->SetBrake(1.0f);
                                }
                            }
                            else
                            {
                                if (((C_car *)e.unknown_8)->velocity.AngleTo(e.car->velocity) < 0.5f &&
                                    (e.unknown_8->GetPos() - e.car->GetPos()).Dot(e.car->velocity) > 0.0f)
                                {
                                    // The target ahead, going the same way: a side no other chase of it takes
                                    bool found1 = false;
                                    bool found2 = false;
                                    int j = elements.size();
                                    while (j--)
                                    {
                                        if (j != i && (elements[j].flags & 0xc001400) &&
                                            (elements[j].flags & 0xc0000000) && elements[j].unknown_8 == e.unknown_8)
                                        {
                                            if (elements[j].flags & 0x1000000)
                                                found1 = true;
                                            else
                                                found2 = true;
                                        }
                                    }
                                    // The loop's index after it is -1, as PS2's, so this reads before the vector
                                    if ((found1 || found2) && !(elements[j].flags & 0xc000000))
                                    {
                                        e.flags = e.flags & ~0x1000000 | 0x80000000;
                                        if (!found1)
                                            e.flags |= 0x1000000;
                                        else if (found2)
                                        {
                                            e.flags &= ~0x80000000;
                                            if (!(e.flags & 0x200000))
                                                e.flags |= 0x2000000;
                                        }
                                    }
                                    else
                                    {
                                        // The first side if the line 5 m back along unknown_c48 is free for
                                        // 4 m, else the second if forward is
                                        S_vector p1, p2, p3, p4;
                                        ((C_car *)e.unknown_8)->GetNavigationPoints(
                                            e.car->GetVehicleSize(false).x + 0.15f, p1, p2, p3, p4);
                                        S_vector dir = e.car->GetUnknownC48() * -5.0f;
                                        S_vector from = e.car->unknown_398;
                                        S_vector hit;
                                        hit = from + dir;
                                        gifCollision.TestLineH(from, dir, &hit, NULL, 0xd02, 0);
                                        if ((from - hit).Square() > 16.0f)
                                        {
                                            p1 *= e.unknown_8->GetFrame()->GetMatrix();
                                            p3 *= e.unknown_8->GetFrame()->GetMatrix();
                                            dir = p1 - p3;
                                            dir.Normalize();
                                            // A local for the first factor: it stays on the FPU stack, as the
                                            // original's
                                            float speed = e.car->GetSpeed();
                                            dir *= speed * e.car->GetSpeed() * 0.04f;
                                            if (!gifCollision.TestLineH(p3, dir, &hit, NULL, 0xd02, 0))
                                                e.flags |= 0x81000000;
                                        }
                                        else
                                        {
                                            dir = e.car->GetUnknownC48() * 5.0f;
                                            hit = from + dir;
                                            gifCollision.TestLineH(from, dir, &hit, NULL, 0xd02, 0);
                                            if ((from - hit).Square() > 16.0f)
                                            {
                                                p2 *= e.unknown_8->GetFrame()->GetMatrix();
                                                p4 *= e.unknown_8->GetFrame()->GetMatrix();
                                                dir = p2 - p4;
                                                dir.Normalize();
                                                float speed = e.car->GetSpeed();
                                                dir *= speed * e.car->GetSpeed() * 0.04f;
                                                if (!gifCollision.TestLineH(p4, dir, &hit, NULL, 0xd02, 0))
                                                    e.flags = e.flags & ~0x1000000 | 0x80000000;
                                            }
                                        }
                                    }
                                    if (!(e.flags & 0x80000000) &&
                                        (e.unknown_8->GetWorldPos() - e.car->GetAIPos(true)).Square() < 25.0f)
                                        e.car->SetBrake(1.0f);
                                }
                                e.driver->unknown_1fc = NULL;
                                e.driver->unknown_200 = NULL;
                                e.driver->unknown_204 = e.unknown_8;
                            }
                        }
                    }
                    else if (e.flags & 0x800)
                    {
                        if (e.unknown_8->GetType() == ACTOR_CAR)
                        {
                            if (e.flags & 0x20000000)
                            {
                                // Behind the target, by its velocity; until the car is ahead of it
                                e.driver->unknown_1fc = e.unknown_c;
                                e.driver->unknown_200 = e.unknown_8;
                                e.driver->unknown_204 = NULL;
                                S_vector dir = ((C_car *)e.unknown_8)->velocity;
                                dir.Normalize();
                                dir *= e.car->GetVehicleSize(false).z + 5.0f;
                                node.pos = dir + ((C_car *)e.unknown_8)->GetAIPos(true);
                                e.driver->unknown_160[1].pos = node.pos;
                                e.driver->unknown_160[1].unknown_30 =
                                    (node.pos - e.driver->unknown_160[0].pos).Magnitude();
                                e.driver->Tick(time);
                                if ((e.car->GetPos() - ((C_car *)e.unknown_8)->GetAIPos(true))
                                    .AngleTo(((C_car *)e.unknown_8)->velocity) < 0.25f)
                                    e.car->SetBrake(1.0f);
                                if ((e.unknown_8->GetPos() - e.car->GetAIPos(false)).Dot(e.car->GetVelocityRef())
                                    > 0.0f)
                                    e.flags &= ~0x20000000;
                            }
                            else if (e.flags & 0x80000000)
                            {
                                // To the target's side, as the chases' (no 0x400 here); behind it (0x20000000)
                                // once there
                                S_vector p1, p2, p3, p4;
                                ((C_car *)e.unknown_8)->GetNavigationPoints(e.car->GetVehicleSize(false).x + 0.15f,
                                    p1, p2, p3, p4);
                                if (!(e.flags & 0x1000000))
                                    p1 = p2;
                                p1 *= e.unknown_8->GetFrame()->GetMatrix();
                                S_vector dir = ((C_car *)e.unknown_8)->velocity;
                                dir.Normalize();
                                dir *= e.car->GetVehicleSize(false).z;
                                node.pos = p1 + dir;
                                dir = node.pos - e.car->GetAIPos(true);
                                if (!gifCollision.TestLineH(e.car->GetAIPos(true), dir, NULL, NULL, 0xd02, 0))
                                {
                                    e.driver->unknown_160[1].pos = node.pos;
                                    e.driver->unknown_160[1].unknown_30 =
                                        (node.pos - e.driver->unknown_160[0].pos).Magnitude();
                                    if ((((C_car *)e.unknown_8)->GetAIPos(true) - e.car->GetAIPos(false))
                                        .Dot(((C_car *)e.unknown_8)->velocity) < 0.0f)
                                    {
                                        e.flags &= ~0x80000000;
                                        if (((C_car *)e.unknown_8)->velocity.AngleTo(e.car->velocity) < 0.5f)
                                            e.flags |= 0x20000000;
                                    }
                                    e.driver->Tick(time);
                                }
                                else
                                {
                                    e.flags &= ~0x80000000;
                                    e.car->SetBrake(1.0f);
                                }
                            }
                            else
                            {
                                // The chases' side search without the other cars (PS2 runs the driver after it)
                                if (((C_car *)e.unknown_8)->velocity.AngleTo(e.car->velocity) < 0.5f &&
                                    (e.unknown_8->GetPos() - e.car->GetPos()).Dot(e.car->velocity) > 0.0f)
                                {
                                    S_vector p1, p2, p3, p4;
                                    ((C_car *)e.unknown_8)->GetNavigationPoints(e.car->GetVehicleSize(false).x + 0.15f,
                                        p1, p2, p3, p4);
                                    S_vector dir = e.car->GetUnknownC48() * -5.0f;
                                    S_vector from = e.car->unknown_398;
                                    S_vector hit;
                                    hit = from + dir;
                                    gifCollision.TestLineH(from, dir, &hit, NULL, 0xd02, 0);
                                    if ((from - hit).Square() > 16.0f)
                                    {
                                        p1 *= e.unknown_8->GetFrame()->GetMatrix();
                                        p3 *= e.unknown_8->GetFrame()->GetMatrix();
                                        dir = p1 - p3;
                                        dir.Normalize();
                                        float speed = e.car->GetSpeed();
                                        dir *= speed * e.car->GetSpeed() * 0.04f;
                                        if (!gifCollision.TestLineH(p3, dir, &hit, NULL, 0xd02, 0))
                                            e.flags |= 0x81000000;
                                    }
                                    else
                                    {
                                        dir = e.car->GetUnknownC48() * 5.0f;
                                        hit = from + dir;
                                        gifCollision.TestLineH(from, dir, &hit, NULL, 0xd02, 0);
                                        if ((from - hit).Square() > 16.0f)
                                        {
                                            p2 *= e.unknown_8->GetFrame()->GetMatrix();
                                            p4 *= e.unknown_8->GetFrame()->GetMatrix();
                                            dir = p2 - p4;
                                            dir.Normalize();
                                            float speed = e.car->GetSpeed();
                                            dir *= speed * e.car->GetSpeed() * 0.04f;
                                            if (!gifCollision.TestLineH(p4, dir, &hit, NULL, 0xd02, 0))
                                                e.flags = e.flags & ~0x1000000 | 0x80000000;
                                        }
                                    }
                                    if (!(e.flags & 0x80000000) &&
                                        (e.unknown_8->GetWorldPos() - e.car->GetAIPos(true)).Square() < 25.0f)
                                        e.car->SetBrake(1.0f);
                                }
                                e.driver->unknown_1fc = NULL;
                                e.driver->unknown_200 = NULL;
                                e.driver->unknown_204 = e.unknown_8;
                            }
                        }
                    }
                }
            }
            else
                e.driver->Tick(time);
            int show = e.car->SetShowEnergy(1);
            if (e.car->ShowCarEnergy() == 0.0f)
                e.status = 4;
            e.car->SetShowEnergy(show);
            show = e.car->SetShowEnergy(1);
            if (e.car->ShowCarEnergy() < 0.15f)
                e.status = 1;
            e.car->SetShowEnergy(show);
            for (int n = e.car->GetNumWheels(); n--;)
            {
                if (e.car->GetWheel(n)->unknown_120 & 0x40000000)
                {
                    e.status = 1;
                    break;
                }
            }
            if (e.car->GetUnknownC54().AngleTo(S_vector(0.0f, 1.0f, 0.0f)) > 0.52f)
            {
                if (e.car->GetSpeed() < 3.0f)
                {
                    e.unknown_2c += time;
                    if (e.unknown_2c > 10000)
                        e.status = 1;
                }
                else
                    e.unknown_2c = 0;
            }
            if (e.driver->unknown_220 > 20000 || !e.unknown_8 && e.driver->unknown_220 > 10000)
            {
                if ((e.car->unknown_2214 || e.car->unknown_2215) && e.car->GetUnknown5f())
                    e.status = 1;
            }
            if (e.unknown_8 && e.unknown_8->GetType() == ACTOR_CAR)
            {
                if (((C_car *)e.unknown_8)->IsUnknown540())
                    e.unknown_28 = 0;
                else
                {
                    e.unknown_28 += time;
                    if (e.unknown_28 > 500 && ((C_car *)e.unknown_8)->GetSpeed() < 1.5f)
                        e.status = 1;
                }
            }
            else
                e.status = 1;
        }
        // The driver's path, braking where the target is behind the car and too near
        else if (e.flags & 0x100010)
        {
            e.driver->unknown_208 = 1;
            if (!(e.driver->unknown_1dc & 0x10000000))
            {
                TickDriver(i, time);
                if (!(e.flags & 0x100000))
                {
                    S_vector dir = e.unknown_8->GetWorldPos() - e.car->GetWorldPos();
                    if (dir.Square() > e.unknown_38 && e.car->GetSpeed() > e.unknown_3c &&
                        e.car->GetVelocityRef().Dot(dir) < 0.0f)
                        e.car->SetBrake(1.0f);
                }
                else
                {
                    e.unknown_28 += time;
                    if (e.unknown_28 > 15000)
                    {
                        e.flags &= ~0x100000;
                        e.driver->unknown_298 = false;
                        e.unknown_28 = 0;
                    }
                }
            }
            else
                e.driver->Tick(time);
            if (!e.driver->unknown_170)
                e.status = 1;
            int show = e.car->SetShowEnergy(1);
            if (e.car->ShowCarEnergy() == 0.0f)
                e.status = 4;
            e.car->SetShowEnergy(show);
            show = e.car->SetShowEnergy(1);
            if (e.car->ShowCarEnergy() < 0.15f)
                e.status = 2;
            e.car->SetShowEnergy(show);
            for (int n = e.car->GetNumWheels(); n--;)
            {
                if (e.car->GetWheel(n)->unknown_120 & 0x40000000)
                {
                    e.status = 2;
                    break;
                }
            }
            if (e.car->GetUnknownC54().AngleTo(S_vector(0.0f, 1.0f, 0.0f)) > 0.52f)
            {
                if (e.car->GetSpeed() < 3.0f)
                {
                    e.unknown_2c += time;
                    if (e.unknown_2c > 10000)
                        e.status = 2;
                }
                else
                    e.unknown_2c = 0;
            }
            if (e.driver->unknown_220 > 20000 && (e.car->unknown_2214 || e.car->unknown_2215) &&
                e.car->GetUnknown5f())
                e.status = 1;
        }
        // The driver's path; a car stopped for 2 s (15 s where its node's table entry has 4) sets 0x100000
        else if ((e.flags & 0x40) && !(e.flags & 0x100000))
        {
            e.driver->unknown_208 = 0;
            e.driver->Tick(time);
            if (!e.driver->unknown_170)
                e.status = 1;
            if (!(e.flags & 0x200000))
            {
                if (e.car->GetUnknownC2c() && e.car->GetSpeed() < 1.0f)
                    e.unknown_28 += time;
                else
                    e.unknown_28 = 0;
                if (e.unknown_28 > 2000)
                {
                    if (e.driver->unknown_1a0)
                    {
                        if (e.driver->unknown_160[e.driver->unknown_170].unknown_68 &&
                            !(unknown_624ba4[*e.driver->unknown_160[e.driver->unknown_170].unknown_68] & 4))
                            e.unknown_28 -= time;
                        else if (e.unknown_28 > 15000)
                        {
                            e.flags |= 0x100000;
                            e.driver->unknown_298 = true;
                            e.unknown_28 = 0;
                        }
                    }
                    else
                    {
                        e.flags |= 0x100000;
                        e.driver->unknown_298 = true;
                        e.unknown_28 = 0;
                    }
                }
            }
        }
        else if (e.flags & 0x20)
        {
            e.driver->Tick(time);
            if (!e.driver->unknown_170)
                e.status = 1;
        }
        // unknown_430 (the horn most likely) while unknown_40 runs, which a traffic car ahead keeps up; the brakes
        // near a slow target
        if (!(e.flags & 0x60))
        {
            if (!e.car->GetUnknown431() && !(e.flags & 0x100000))
            {
                e.unknown_40 -= time;
                std::vector<C_traffic_car::S_ELEMENT *> elems = C_traffic_car::GetElems();
                int k;
                for (k = elems.size(); k--;)
                {
                    C_traffic_car::S_ELEMENT *&te = elems[k];
                    S_vector dir = te->car->GetPos() - e.car->GetPos();
                    float f = e.car->GetSpeed() * (1.0f / 28.0f);
                    if (f > 1.0f)
                        f = 1.0f;
                    f *= 2500.0f;
                    if (dir.Square() < f && dir.AngleTo(e.car->GetVelocityRef()) < 0.5f)
                        break;
                }
                if (e.unknown_40 > 0)
                    e.car->SetUnknown430(true);
                else
                {
                    e.car->SetUnknown430(false);
                    if (k < 0)
                        e.unknown_40 = -1;
                }
                if (e.car->GetSpeed() > 4.0f && e.unknown_40 < -200 - rand() % 800)
                    e.unknown_40 = rand() % 600 + 200;
            }
            if (!(e.flags & 0x10))
            {
                if (e.unknown_8 != e.unknown_c)
                {
                    float dist_sq = (e.car->GetPos() - e.unknown_8->GetPos()).Square();
                    if (((C_car *)e.unknown_8)->GetSpeed() < 2.0f && dist_sq < 900.0f)
                    {
                        if (dist_sq > 225.0f)
                        {
                            if (e.car->GetSpeed() > 14.0f)
                                e.car->SetBrake(1.0f);
                        }
                        else if (e.car->GetSpeed() > 5.0f)
                            e.car->SetBrake(1.0f);
                    }
                    else if (dist_sq < 400.0f)
                    {
                        if (e.flags & 0x2000200)
                        {
                            if (e.car->GetSpeed() - ((C_car *)e.unknown_8)->GetSpeed() > 15.0f &&
                                e.car->GetSpeed() > 15.0f)
                                e.car->SetBrake(1.0f);
                        }
                        else if (dist_sq < 225.0f)
                        {
                            if (e.car->GetSpeed() - ((C_car *)e.unknown_8)->GetSpeed() > 4.0f)
                                e.car->SetBrake(1.0f);
                        }
                        else if (e.car->GetSpeed() - ((C_car *)e.unknown_8)->GetSpeed() > 4.0f &&
                            e.car->GetSpeed() > 17.0f)
                            e.car->SetBrake(1.0f);
                    }
                }
                else if (e.unknown_8)
                {
                    float dist_sq = (e.car->GetPos() - e.unknown_8->GetPos()).Square();
                    if (dist_sq < 1600.0f && e.car->GetSpeed() > 14.0f)
                        e.car->SetBrake(1.0f);
                    if (dist_sq < 625.0f)
                    {
                        if (e.car->GetSpeed() > 5.0f)
                            e.car->SetBrake(1.0f);
                    }
                    else if (dist_sq < 225.0f)
                    {
                        if (e.car->GetSpeed() > 2.5f)
                            e.car->SetBrake(1.0f);
                    }
                }
            }
        }
        // A driver (seat 0) with unknown_5e set ends the chase (3) and the loop
        C_human *owner = e.car->GetOwner(0);
        if (owner && owner->GetUnknown5e())
        {
            e.status = 3;
            break;
        }
    }
    // The cars whose chase ended stop, but those of status 3
    for (i = elements.size(); i--;)
    {
        S_ELEMENT &e = elements[i];
        if (e.status && e.status != 3)
        {
            e.car->SetUnknown4cc(false);
            e.car->SetBrake(1.0f);
            e.car->SetPower(0.0f, true);
        }
    }
}

#pragma auto_inline(off)

// STUB: GAME 0x4a1da0
bool C_sledovacka::RecompilePath(int index)
{
    StubLog("C_sledovacka::RecompilePath");
    return false;
}

// STUB: GAME 0x4a28c0
bool C_sledovacka::PreparePath(int index)
{
    StubLog("C_sledovacka::PreparePath");
    return false;
}

// STUB: GAME 0x4a2a80
void C_sledovacka::TickDriver(int index, uint time)
{
    StubLog("C_sledovacka::TickDriver");
}

// STUB: GAME 0x4a86d0
void C_sledovacka::SetActionStatus(C_car *car, E_STATUS status)
{
    StubLog("C_sledovacka::SetActionStatus");
}

#pragma auto_inline(on)
