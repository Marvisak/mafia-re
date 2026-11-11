// The chases (PS2 C_sledovacka.cpp; the PC's file is the one at 0x4a0320-0x4aa2ac, docs/game-files.md), which starts
// with the constructor, written whole in PS2's order: the constructor, EnableChangeTables, Init and Done, which the
// game's Init and Done call, HoniMiHoNekdoZKanclu, the AI's AddCarHunt, AddCarEscape and AddCarMoveTo, RemoveCar, the
// path's RecompilePath and PreparePath, TickDriver and Tick, which the game's Tick calls, the status' functions,
// VypicujMiToyotu, which C_game::InvalidateActor calls, DevaaateeeZasNeznaCas and the saved game's three. Open:
// RecompilePath's frame (the original's ebp frame aligned to 8, whose trigger isn't known), SaveGameSave's and
// SaveGameLoad's registers, AddCarEscape's (car and this swapped), and:
// Tick (18 KB) is in VC6's large-function regime, where more source means less inlining at the end: the original
// calls helpers out of line from the 0x800 branch's side search on (its first, an S_vector operator- at 0x4a76fc) that
// ours inlines. C_car::SetDynamicCollsOff's read through C_Vehicle::GetUnknown1fac spends some of it (64% to 80%);
// about 150 dead statements anywhere in Tick still give the original's decisions at the end (96%), but they also raise
// the budget of its first nested calls (the path's clear() inlined, where the original calls erase), so what the
// original had more of is most likely inline calls, not source: accessors at the members Tick reads directly
// (world_pos, unknown_c3c, act_state, the target's car) or in the helpers it calls often (GetSpeed, GetType) each spend
// a little and keep those decisions, about 30 of them a few calls' worth; GetVelocityRef in the chases beside the
// target sends the 0x2 branch's erase out of line. Not found yet. Its frame is 0x714, the original's 0x744.

#include "ai/C_sledovacka.h"

#include "actors/C_actor.h"
#include "actors/C_bridge.h"
#include "ai/C_driver.h"
#include "ai/road_web.h"
#include "actors/C_human.h"
#include "collision/g_colls.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "ui/g_indicators.h"
#include "vehicles/C_car.h"
#include "vehicles/C_trf_c.h"

#include "stub.h"

// FUNCTION: GAME 0x4a0320
C_sledovacka::C_sledovacka() :
    road_net(0)
{
}

// The same code as the force feedback's destructor, whose copy the linker kept (0x444670): the marker is that one's
C_sledovacka::~C_sledovacka()
{
}

// GLOBAL: GAME 0x634a18
bool C_sledovacka::change_tables = true;

// FUNCTION: GAME 0x4a0350
void C_sledovacka::EnableChangeTables(bool enable)
{
    change_tables = enable;
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
            S_vector d0 = roads->nodes[roads->links[n].node0].pos - actors[i]->GetWorldPos();
            S_vector d1 = roads->nodes[roads->links[n].node1].pos - actors[i]->GetWorldPos();
            if (d0.Square() < d1.Square())
                b.node = roads->links[n & NEARNODE_INDEX].node0;
            else
                b.node = roads->links[n & NEARNODE_INDEX].node1;
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

// Whether the car is in the chases (PS2 name, Czech slang: roughly "is someone from the office chasing him")
// FUNCTION: GAME 0x4a0700
bool C_sledovacka::HoniMiHoNekdoZKanclu(C_car *car)
{
    int i;
    for (i = elements.size(); i--;)
    {
        if (elements[i].car == car)
            break;
    }
    return i >= 0 ? true : false;
}

// A car sent after the target (the player or an entity, its car while it sits in one), or to pos without one, as PS2's:
// false for no car, a car already chased with or a target of another type. Its driver gets the car, the time the
// chase may lose the target for (time, in seconds) and factor; SLEDF_ROADS with flags' bits that carry over. The car
// goes red on the radar, to its highest level where the tables change. A target already within 12 m and in sight ends
// the chase at once (STATUS_DONE), unless its car moves.
// FUNCTION: GAME 0x4a0760
bool C_sledovacka::AddCarHunt(C_car *car, C_actor *target, uint flags, uint time, const S_vector &pos, float factor)
{
    if (!car)
        return false;
    for (int i = elements.size(); i--;)
    {
        if (elements[i].car == car)
            return false;
    }
    if (target && target->GetType() != ACTOR_PLAYER && target->GetType() != ACTOR_ENTITY)
        return false;
    car->axis = CARAXIS_UNKNOWN_C00;
    car->SetEngineOn(true, true);
    S_ELEMENT e;
    memset(&e, 0, sizeof(e));
    e.car = car;
    e.target = target;
    e.driver = new C_driver;
    if (target)
    {
        e.chased = ((C_human *)target)->car;
        e.driver->unknown_1fc = target;
        e.driver->unknown_200 = ((C_human *)target)->car;
    }
    else
        e.target_pos = pos;
    e.driver->SetCar(car);
    e.driver->unknown_218 = 1000.0f;
    e.driver->next_node = 0;
    e.driver->unknown_84 = factor;
    e.driver->unknown_208 = 1;
    e.flags = SLEDF_ROADS;
    e.flags |= flags & (SLEDF_UNKNOWN_100 | SLEDF_UNKNOWN_200 | SLEDF_UNKNOWN_400 | SLEDF_UNKNOWN_800 |
        SLEDF_UNKNOWN_1000 | SLEDF_UNKNOWN_200000);
    e.status = 0;
    e.vole_mode = target ? 1 : 2;
    e.limit = time * 1000.0f;
    elements.push_back(e);
    car->unknown_bc = true;
    mafiaIndicators.RadarRemoveCar(car);
    mafiaIndicators.RadarAddCar(car, 0xffff0000);
    if (!road_net)
        road_net = act_mission->roads;
    if (change_tables && car->GetLevelsCount() > 1)
        car->SetActualLevel(car->GetLevelsCount() - 1);
    // The target 1 m up, or the place
    S_vector dir;
    if (e.target)
        dir = (e.target->GetWorldPos() + S_vector(0.0f, 1.0f, 0.0f)) - e.car->world_pos;
    else
        dir = (e.target_pos + S_vector(0.0f, 1.0f, 0.0f)) - e.car->world_pos;
    if (dir.Magnitude() < 12.0f)
    {
        if (!gifCollision.TestLineHStatic(e.car->world_pos, dir, NULL, NULL, 0))
        {
            // The element just added (PS2's the same): the target's car standing, or no car
            S_ELEMENT &last = elements.back();
            if (last.target)
            {
                C_car *target_car = ((C_human *)last.target)->car;
                if (target_car)
                {
                    if (target_car->GetType() == ACTOR_CAR)
                    {
                        if (!target_car->IsUnknown540())
                            last.status = STATUS_DONE;
                    }
                    else
                        last.status = STATUS_DONE;
                }
                else
                    last.status = STATUS_DONE;
            }
            else
                last.status = STATUS_DONE;
        }
    }
    return true;
}

// A car sent away from the target (the player or an entity) to pos along the roads (PS2 name; SLEDF_ESCAPE): false
// for no car or no target, a car already chased with or a target of another type. Further than dist from the target
// and faster than speed (in km/h) the car brakes where it drives away (Tick); its driver gets factor. The roads'
// nodes from the car's link to the one nearest to pos (FindPath runs a second time, as PS2's), the other end of the
// car's link first, the last link dropped where pos lies between it and the node before, then pos.
// FUNCTION: GAME 0x4a0af0
bool C_sledovacka::AddCarEscape(C_car *car, C_actor *target, const S_vector &pos, bool unknown, float speed, float dist,
    float factor)
{
    if (!car || !target)
        return false;
    for (int i = elements.size(); i--;)
    {
        if (elements[i].car == car)
            return false;
    }
    if (target->GetType() != ACTOR_PLAYER && target->GetType() != ACTOR_ENTITY)
        return false;
    car->axis = CARAXIS_UNKNOWN_C00;
    car->SetEngineOn(true, true);
    S_ELEMENT e;
    memset(&e, 0, sizeof(e));
    e.car = car;
    e.target = target;
    e.limit = dist * dist;
    e.limit2 = speed * (1.0f / 3.6f);
    e.chased = ((C_human *)target)->car;
    e.driver = new C_driver;
    e.driver->SetCar(car);
    e.driver->unknown_84 = factor;
    e.driver->next_node = 0;
    e.driver->unknown_218 = unknown ? 0.0f : 1000.0f;
    e.driver->unknown_208 = 1;
    e.flags = SLEDF_ESCAPE;
    e.status = 0;
    e.vole_mode = 3;
    elements.push_back(e);
    car->unknown_bc = true;
    if (!road_net)
        road_net = act_mission->roads;
    ushort from = road_net->GetNearNode(car->GetPos()) | NEARNODE_FLAG;
    ushort to = road_net->GetNearNode(pos) | NEARNODE_FLAG;
    if (from != NEARNODE_NONE && to != NEARNODE_NONE)
    {
        std::vector<ushort> path;
        if (from != to)
        {
            if (!road_net->FindPath(from, to, path))
                return false;
        }
        else
            path.push_back(from);
        road_net->FindPath(from, to, path);
        // The other end of the car's link first
        ushort first = road_net->links[from & NEARNODE_INDEX].end0 == path[1] ?
            road_net->links[from & NEARNODE_INDEX].end1 : road_net->links[from & NEARNODE_INDEX].end0;
        path.insert(path.begin(), first);
        // The last link dropped where pos lies between it and the node before (a node or a link)
        if (path[path.size() - 2] & NEARNODE_FLAG)
        {
            if ((pos - road_net->links[path[path.size() - 1] & NEARNODE_INDEX].pos)
                .Dot(pos - road_net->links[path[path.size() - 2] & NEARNODE_INDEX].pos) < 0.0f)
                path.pop_back();
        }
        else
        {
            if ((pos - road_net->links[path[path.size() - 1] & NEARNODE_INDEX].pos)
                .Dot(pos - road_net->nodes[path[path.size() - 2]].pos) < 0.0f)
                path.pop_back();
        }
        e.driver->path.clear();
        for (uint j = 0; j < path.size(); j++)
        {
            S_car_node node;
            memset(&node, 0, sizeof(node));
            node.road_node = path[j];
            e.driver->path.push_back(node);
        }
        if (PreparePath(elements.size() - 1))
        {
            if (!RecompilePath(elements.size() - 1))
            {
                e.driver->path.clear();
                e.driver->next_node = 0;
            }
        }
        else
        {
            e.driver->path.clear();
            e.driver->next_node = 0;
        }
        for (int k = e.driver->path.size(); k--;)
            e.driver->path[k].unknown_48 = 1000.0f;
        // pos last
        S_car_node node;
        memset(&node, 0, sizeof(node));
        node.pos[0] = pos;
        node.cur_lane = 0;
        node.num_lanes = 1;
        node.num_type2 = 1;
        node.unknown_48 = 1000.0f;
        node.unknown_4c = 1.5f;
        node.unknown_54 = true;
        node.distance[0] = (node.pos[0] - e.driver->path.back().pos[0]).Magnitude();
        e.driver->path.push_back(node);
    }
    e.driver->next_node = 1;
    if (car->GetLevelsCount() > 1)
        car->SetActualLevel(car->GetLevelsCount() - 1);
    return true;
}

// A car sent to pos, as PS2's: false for no car or a car already chased with. Direct, it drives straight there
// (SLEDF_MOVE_DIRECT: a path of two nodes, its place and pos); else along the roads (SLEDF_MOVE_ROADS, with flags'
// SLEDF_UNKNOWN_200000): the roads' nodes between the links nearest to the car and to pos, the first and the last
// dropped where the car or pos lies between them and the next, then the car's place before them and pos after.
// FUNCTION: GAME 0x4a1220
bool C_sledovacka::AddCarMoveTo(C_car *car, const S_vector &pos, bool direct, bool unknown, uint flags)
{
    if (!car)
        return false;
    for (int i = elements.size(); i--;)
    {
        if (elements[i].car == car)
            return false;
    }
    car->axis = CARAXIS_UNKNOWN_C00;
    car->SetEngineOn(true, true);
    S_ELEMENT e;
    memset(&e, 0, sizeof(e));
    e.car = car;
    e.driver = new C_driver;
    e.driver->SetCar(car);
    e.driver->next_node = 0;
    e.driver->unknown_84 = 1.0f;
    e.driver->unknown_208 = 0;
    e.driver->unknown_218 = unknown ? 0.0f : 1000.0f;
    e.status = 0;
    e.vole_mode = 4;
    if (direct)
    {
        e.flags = SLEDF_MOVE_DIRECT;
        elements.push_back(e);
        S_car_node node;
        memset(&node, 0, sizeof(node));
        node.pos[0] = car->world_pos;
        node.cur_lane = 0;
        node.num_lanes = 1;
        node.num_type2 = 1;
        node.unknown_48 = 2.5f;
        node.unknown_4c = 2.0f;
        node.unknown_54 = true;
        e.driver->path.clear();
        e.driver->path.push_back(node);
        node.pos[0] = pos;
        e.driver->path.push_back(node);
        e.driver->next_node = 1;
        e.driver->unknown_19c = DRIVER19C_UNKNOWN_10;
        return true;
    }
    e.flags = SLEDF_MOVE_ROADS | flags & SLEDF_UNKNOWN_200000;
    elements.push_back(e);
    e.driver->path.clear();
    if (!road_net)
        road_net = act_mission->roads;
    ushort from = road_net->GetNearNode(car->GetPos()) | NEARNODE_FLAG;
    ushort to = road_net->GetNearNode(pos) | NEARNODE_FLAG;
    if (from != NEARNODE_NONE && to != NEARNODE_NONE)
    {
        S_car_node node;
        memset(&node, 0, sizeof(node));
        node.pos[0] = car->world_pos;
        node.cur_lane = 0;
        node.num_lanes = 1;
        node.num_type2 = 1;
        node.unknown_48 = 5.0f;
        node.unknown_4c = 1.5f;
        node.unknown_54 = true;
        std::vector<ushort> path;
        if (from != to)
        {
            if (!road_net->FindPath(from, to, path))
                return false;
            // The first link dropped where the car lies between it and the next node, the last where pos does
            if (path[1] & NEARNODE_FLAG)
            {
                if ((node.pos[0] - road_net->links[path[0] & NEARNODE_INDEX].pos)
                    .Dot(node.pos[0] - road_net->links[path[1] & NEARNODE_INDEX].pos) < 0.0f)
                    path.erase(path.begin());
            }
            else
            {
                if ((node.pos[0] - road_net->links[path[0] & NEARNODE_INDEX].pos)
                    .Dot(node.pos[0] - road_net->nodes[path[1]].pos) < 0.0f)
                    path.erase(path.begin());
            }
            if (path.size() > 1)
            {
                if (path[path.size() - 2] & NEARNODE_FLAG)
                {
                    if ((pos - road_net->links[path[path.size() - 1] & NEARNODE_INDEX].pos)
                        .Dot(pos - road_net->links[path[path.size() - 2] & NEARNODE_INDEX].pos) < 0.0f)
                        path.pop_back();
                }
                else
                {
                    if ((pos - road_net->links[path[path.size() - 1] & NEARNODE_INDEX].pos)
                        .Dot(pos - road_net->nodes[path[path.size() - 2]].pos) < 0.0f)
                        path.pop_back();
                }
            }
            else
            {
                if (path[0] & NEARNODE_FLAG)
                {
                    if ((pos - node.pos[0]).Dot(pos - road_net->links[path[path.size() - 1] & NEARNODE_INDEX].pos) <
                        0.0f)
                        path.pop_back();
                }
                else
                {
                    if ((pos - node.pos[0]).Dot(pos - road_net->nodes[path[path.size() - 1]].pos) < 0.0f)
                        path.pop_back();
                }
            }
            for (uint j = 0; j < path.size(); j++)
            {
                S_car_node n;
                memset(&n, 0, sizeof(n));
                n.road_node = path[j];
                e.driver->path.push_back(n);
            }
            if (PreparePath(elements.size() - 1))
            {
                if (!RecompilePath(elements.size() - 1))
                {
                    e.driver->path.clear();
                    e.driver->next_node = 0;
                }
            }
            else
            {
                e.driver->path.clear();
                e.driver->next_node = 0;
            }
        }
        // The car's place first, pos last
        e.driver->path.insert(e.driver->path.begin(), node);
        memset(&node, 0, sizeof(node));
        node.pos[0] = pos;
        node.cur_lane = 0;
        node.num_lanes = 1;
        node.num_type2 = 1;
        node.unknown_48 = 5.0f;
        node.unknown_4c = 1.5f;
        node.unknown_54 = true;
        node.distance[0] = (node.pos[0] - e.driver->path.back().pos[0]).Magnitude();
        e.driver->path.push_back(node);
    }
    e.driver->next_node = 1;
    return true;
}

// The car let go: what the chase changed back (its horn, its collisions on, the brakes unless its driver died, its
// level where the tables change or in vole modes 3 and 4), then its element and its driver gone (PS2's the same)
// FUNCTION: GAME 0x4a1c30
void C_sledovacka::RemoveCar(C_car *car)
{
    for (int i = elements.size(); i--;)
    {
        if (elements[i].car == car)
        {
            elements[i].car->unknown_bc = false;
            elements[i].car->SetHorn(false);
            elements[i].car->invisible = false;
            elements[i].car->SetDynamicCollsOff(false);
            if (elements[i].status != STATUS_DRIVER_DEAD)
            {
                elements[i].car->SetPower(0.0f, true);
                elements[i].car->SetBrake(1.0f);
            }
            if (change_tables || elements[i].vole_mode == 3 || elements[i].vole_mode == 4)
            {
                if (elements[i].car->GetActualLevel() > 0)
                    elements[i].car->SetActualLevel(0);
            }
            elements[i].car->axis = 0;
            elements[i].car->unknown_2100 = 1.0f;
            delete elements[i].driver;
            elements.erase(elements.begin() + i);
            return;
        }
    }
}

// What the paths' nodes' semaphore phases index (the name is ours): the semaphores' lights in each of their ten
// phases, C_shvestky.cpp's semaphore_colors (red 1 for five, green 4 for four, then orange 2)
// GLOBAL: GAME 0x624ba4
static const int phase_colors[10] = {1, 1, 1, 1, 1, 4, 4, 4, 4, 2};

// The lanes' positions of a path found on the roads (PS2 name), after PreparePath, as PS2's: each node's position from
// its road node, its speed and its kind of node (2.5 on a link, 4.1 on a node) and its road's lanes (the positions
// of its lanes beside the line through its neighbours, half the first lane's width beside it where the road has no
// lanes); a crossing's positions moved to where the lanes from the nodes before and after it meet; the semaphores'
// phases of the nodes before a crossing with a semaphore (a bridge's own, or C_game's two by the road's direction);
// and each position's distance from the previous node's. Always true.
// FUNCTION: GAME 0x4a1da0
bool C_sledovacka::RecompilePath(int index)
{
    std::vector<S_car_node> &path = elements[index].driver->path;
    int i;
    for (i = 0; i < path.size(); i++)
    {
        if (path[i].road_node & NEARNODE_FLAG)
            path[i].unknown_48 = road_net->links[path[i].road_node & NEARNODE_INDEX].unknown_c;
        else
            path[i].unknown_48 = road_net->nodes[path[i].road_node].unknown_10;
        if (path[i].road_node & NEARNODE_FLAG)
            path[i].unknown_4c = 2.5f;
        else
            path[i].unknown_4c = 4.1f;
        path[i].pos[0] = road_net->GetPos(path[i].road_node);
        path[i].pos[1] = path[i].pos[0];
        path[i].pos[2] = path[i].pos[0];
        path[i].pos[3] = path[i].pos[0];
    }
    for (i = 0; i < path.size(); i++)
    {
        path[i].unknown_54 = path[i].lanes->unknown_c;
        path[i].fixed = false;
        // The direction along the road at the node: from the previous node to the next
        int ends[2];
        if (i == 0)
        {
            ends[0] = i;
            ends[1] = i + 1;
            if (path[i].road_node & NEARNODE_FLAG)
                path[i].fixed = true;
        }
        else if (i == path.size() - 1)
        {
            ends[0] = i - 1;
            ends[1] = i;
            if (path[i].road_node & NEARNODE_FLAG)
                path[i].fixed = true;
        }
        else
        {
            ends[0] = i - 1;
            ends[1] = i + 1;
            if (path[i].road_node & NEARNODE_FLAG)
                path[i].fixed = true;
        }
        S_vector2 pts[2];
        int k;
        for (k = 0; k < 2; k++)
        {
            S_vector p = road_net->GetPos(path[ends[k]].road_node);
            pts[k] = S_vector2(p.x, p.z);
        }
        S_vector2 dir = pts[1] - pts[0];
        S_vector2 n(dir.y, -dir.x);
        float len = n.Magnitude();
        if (len > MRG_ZERO)
        {
            float r = 1.0f / len;
            n.x *= r;
            n.y *= r;
        }
        // The lanes to drive on: those of types 2 and 3, else those of type 1
        S_road_lanes *lanes = path[i].lanes;
        float offset = 0.0f;
        int num = 0;
        int num2 = 0;
        bool no_23 = true;
        bool no_lanes = true;
        for (k = 0; k < 4; k++)
        {
            if (lanes->lane[k].type == 2 || lanes->lane[k].type == 3)
            {
                no_23 = false;
                no_lanes = false;
                break;
            }
            if (lanes->lane[k].type == 1)
                no_lanes = false;
        }
        if (no_lanes)
        {
            S_vector2 off = n * lanes->lane[0].width * 0.5f;
            path[i].pos[0].x += off.x;
            path[i].pos[0].z += off.y;
            path[i].lane[0] = 0;
            num2 = num = 1;
        }
        else
        {
            for (k = 0; k < 4; k++)
            {
                if (lanes->lane[k].type == 2 || lanes->lane[k].type == 3 || no_23 && lanes->lane[k].type == 1)
                {
                    offset += lanes->lane[k].width * 0.5f;
                    S_vector2 off = n * offset;
                    path[i].pos[num].x += off.x;
                    path[i].pos[num].z += off.y;
                    path[i].lane[num++] = k;
                    offset += lanes->lane[k].width * 0.5f;
                    if (lanes->lane[k].type == 2)
                        num2++;
                }
                else
                    offset += lanes->lane[k].width;
            }
        }
        path[i].cur_lane = 0;
        path[i].num_lanes = num;
        path[i].num_type2 = num2;
    }
    // A crossing: each lane's position where the line along the previous two nodes meets the line along the next two
    S_vector p;
    S_vector2 a, b, c, d, r;
    for (i = 2; i < path.size(); i++)
    {
        if (!path[i].fixed && i < path.size() - 2)
        {
            p = elements[index].driver->path[i - 1].pos[elements[index].driver->path[i - 1].cur_lane];
            a = S_vector2(p.x, p.z);
            p -= elements[index].driver->path[i - 2].pos[elements[index].driver->path[i - 2].cur_lane];
            b = S_vector2(p.x, p.z);
            for (int k = 0; k < elements[index].driver->path[i + 1].num_lanes; k++)
            {
                p = elements[index].driver->path[i + 1].pos[k];
                c = S_vector2(p.x, p.z);
                p = path[i + 1].pos[k] - path[i + 2].pos[k];
                d = S_vector2(p.x, p.z);
                float angle = (float)fabs(AngleTo2(d, b));
                if (angle < 2.65f && angle > 0.35f)
                {
                    r.Intersection(a, b, c, d);
                    // A sharp turn: nearer to the lanes' nodes
                    if (angle < 1.35f)
                    {
                        d = c - r;
                        d.x *= 0.55f;
                        d.y *= 0.55f;
                        b = a - r;
                        b.x *= 0.1f;
                        b.y *= 0.1f;
                        r += b;
                        r += d;
                    }
                    p.x = r.x;
                    p.z = r.y;
                    p.y = path[i].pos[k].y;
                    path[i].pos[k] = p;
                }
            }
            path[i].fixed = true;
        }
    }
    // The semaphores: the node before a crossing that has one waits at its phase
    for (i = path.size(); i--;)
    {
        if (!(path[i].road_node & NEARNODE_FLAG) && i > 0 && road_net->nodes[path[i].road_node].semaphore)
        {
            int j;
            for (j = bridges.size(); j--;)
            {
                if (bridges[j].node == path[i].road_node)
                    break;
            }
            if (j < 0)
            {
                S_vector2 dist(path[i - 1].pos[0].x - path[i].pos[0].x, path[i - 1].pos[0].z - path[i].pos[0].z);
                dist.x = (float)fabs(dist.x);
                dist.y = (float)fabs(dist.y);
                if (dist.x > dist.y)
                    path[i - 1].semaphore_phase = &act_mission->game->semaphore_phase_x;
                else
                    path[i - 1].semaphore_phase = &act_mission->game->semaphore_phase_z;
            }
            else
                path[i - 1].semaphore_phase = &((C_bridge *)bridges[j].actor)->phase;
        }
    }
    // Each lane's distance from the previous node's position on it
    for (i = 0; i < path.size() - 1; i++)
    {
        for (int k = path[i + 1].num_lanes; k--;)
            path[i + 1].distance[k] = (path[i].pos[k] - path[i + 1].pos[k]).Magnitude();
    }
    return true;
}

// Each node's road from the first one on (PS2 name), the road's lanes towards the next node: the first node's road
// node found past the links at the path's start, then each node's lanes those of its road node towards the next one
// (a link keeps the previous node's). False for a path of fewer than two nodes or where a road node doesn't lead to
// the next one.
// FUNCTION: GAME 0x4a28c0
bool C_sledovacka::PreparePath(int index)
{
    if (elements[index].driver->path.size() < 2)
        return false;
    ushort prev = elements[index].driver->path[1].road_node;
    ushort n = elements[index].driver->path[0].road_node;
    while (n & NEARNODE_FLAG)
    {
        ushort next;
        if (road_net->links[n & NEARNODE_INDEX].end0 == prev)
            next = road_net->links[n & NEARNODE_INDEX].end1;
        else
            next = road_net->links[n & NEARNODE_INDEX].end0;
        prev = n;
        n = next;
    }
    int i;
    for (i = 0; i < 4; i++)
    {
        if (road_net->nodes[n].conns[i] == prev)
            break;
    }
    if (i == 4)
        return false;
    S_road_lanes *lanes = &road_net->nodes[n].lanes[i];
    for (uint j = 0; j < elements[index].driver->path.size(); j++)
    {
        if (elements[index].driver->path[j].road_node & NEARNODE_FLAG)
            elements[index].driver->path[j].lanes = lanes;
        else
        {
            elements[index].driver->path[j].lanes = lanes;
            int k;
            for (k = 0; k < 4; k++)
            {
                if (road_net->nodes[elements[index].driver->path[j].road_node].conns[k] ==
                    elements[index].driver->path[j + 1].road_node)
                    break;
            }
            if (k == 4)
                return false;
            lanes = &road_net->nodes[elements[index].driver->path[j].road_node].lanes[k];
        }
    }
    return true;
}

// Where a line meets the plane: VE_BreakageGlass.cpp's (Insanity3D's Intersection with its IntersectionPosition
// written in), which TickDriver inlines for the line of the car's velocity in the space of the car ahead
inline bool S_plane::Intersection(const S_vector &pos, const S_vector &dir, S_vector &ret) const
{
    float f = normal.Dot(dir);
    if (fabs(f) < MRG_ZERO)
        return false;
    f = -(normal.Dot(pos) + d) / f;
    ret = pos + dir * f;
    return true;
}

// The car's driver, around a car in its way (blocking_car, which follows the driver's while the driver has
// DRIVER19C_BLOCKED; without one the driver only ticks): two of the four points that car gives around itself
// (GetNavigationPoints), by where the line of the car's velocity passes it in its own space (its sides' planes when
// the car comes across it, its ends' when along it), and two more 200 m and 400 m on, inserted into the path at the
// next node for the driver's tick, then taken out again; the car in the way is forgotten once the driver is past the
// second point. Then the node to drive to, from the one before the next node: on while the next one is nearer, one
// more once the car is between the two, and back while the line to it from 1 m above the car is blocked (TestLineV),
// the car in the way forgotten where that node is behind the car. Above 30 m/s the car brakes a little. PS2's stops
// the node search after 200 rounds and makes a node 0 node 1 in mise16-mesto.
// Open: the frame's slots (90%, 100% with registers and stack masked): with S_vector::Normalize's reciprocal in sq, one
// variable as SetNormalized's, its locals stop sharing the slot of the car in the way and the frame is the original's
// (99.78%), but for the game alone that spelling makes 13 other functions worse (C_human::AI, C_seagull's, ...)
// FUNCTION: GAME 0x4a2a80
void C_sledovacka::TickDriver(int index, uint time)
{
    // The first test through a reference: VC6 then reads the driver's flags again for the second, as the original
    S_ELEMENT &e = elements[index];
    if (!(e.driver->unknown_19c & DRIVER19C_BLOCKED) && !e.blocking_car)
    {
        e.driver->Tick(time);
        return;
    }
    if (elements[index].driver->unknown_19c & DRIVER19C_BLOCKED &&
        elements[index].driver->blocking_car != elements[index].blocking_car)
        elements[index].blocking_car = elements[index].driver->blocking_car;
    // The car ahead's points, and the car's position and velocity in its space
    C_car *car = elements[index].blocking_car;
    S_vector p1, p2, p3, p4;
    car->GetNavigationPoints(elements[index].car->GetVehicleSize(false).x + 0.5f, p1, p2, p3, p4);
    float angle = elements[index].car->velocity.AngleTo(car->GetDir());
    S_matrix m = car->GetInvModelMatrix(false);
    S_vector from = elements[index].car->GetAIPos(true) * m;
    m(3) = S_vector(0.0f, 0.0f, 0.0f);
    S_vector dir = elements[index].car->velocity * m;
    S_plane pl;
    pl.d = 0.0f;
    S_vector pt;
    if (angle > 1.05f && angle < 2.1f)
    {
        // Across it: the side the line crosses
        if (elements[index].car->velocity.Dot(car->GetUnknownC48()) > 0.0f)
        {
            pl.normal = S_vector(-1.0f, 0.0f, 0.0f);
            if (!pl.Intersection(from, dir, pt))
                pt = S_vector(0.0f, 0.0f, 1.0f);
            if (pt.z < 0.0f)
            {
                p1 = p3;
                p2 = p4;
            }
        }
        else
        {
            pl.normal = S_vector(1.0f, 0.0f, 0.0f);
            if (!pl.Intersection(from, dir, pt))
                pt = S_vector(0.0f, 0.0f, 1.0f);
            if (pt.z > 0.0f)
            {
                p3 = p1;
                p1 = p2;
                p2 = p3;
            }
            else
            {
                p1 = p4;
                p2 = p3;
            }
        }
    }
    else
    {
        // Along it: the end the line comes to
        if (car->GetDir().Dot(elements[index].car->velocity) > 0.0f)
        {
            pl.normal = S_vector(0.0f, 0.0f, -1.0f);
            if (!pl.Intersection(from, dir, pt))
                pt = S_vector(1.0f, 0.0f, 0.0f);
            if (pt.x < 0.0f)
            {
                p2 = p1;
                p1 = p3;
            }
            else
                p1 = p4;
        }
        else
        {
            pl.normal = S_vector(0.0f, 0.0f, 1.0f);
            if (!pl.Intersection(from, dir, pt))
                pt = S_vector(1.0f, 0.0f, 0.0f);
            if (pt.x > 0.0f)
            {
                p1 = p2;
                p2 = p4;
            }
            else
                p2 = p3;
        }
    }
    // In the world, and two more on along them
    p1 *= car->GetFrame()->GetMatrix();
    p2 *= car->GetFrame()->GetMatrix();
    p3 = p2 - p1;
    p3.Normalize();
    p3 *= 200.0f;
    p4 = p3 * 2.0f;
    p3 += p2;
    p4 += p2;
    // Inserted at the next node, last first
    int n = elements[index].driver->next_node;
    S_car_node node;
    memset(&node, 0, sizeof(node));
    node.pos[0] = p4;
    node.cur_lane = 0;
    node.num_lanes = 1;
    node.num_type2 = 1;
    node.unknown_48 = 1000.0f;
    node.unknown_4c = 0.5f;
    node.unknown_54 = true;
    elements[index].driver->path.insert(elements[index].driver->path.begin() + n, node);
    node.pos[0] = p3;
    elements[index].driver->path.insert(elements[index].driver->path.begin() + n, node);
    node.pos[0] = p2;
    elements[index].driver->path.insert(elements[index].driver->path.begin() + n, node);
    node.pos[0] = p1;
    elements[index].driver->path.insert(elements[index].driver->path.begin() + n, node);
    if (n > 0)
        elements[index].driver->path[n].distance[0] =
            (elements[index].driver->path[n].pos[0] - elements[index].driver->path[n - 1].pos[0]).Magnitude();
    else
        elements[index].driver->path[n].distance[0] = 0.0f;
    elements[index].driver->path[n + 1].distance[0] =
        (elements[index].driver->path[n + 1].pos[0] - elements[index].driver->path[n].pos[0]).Magnitude();
    elements[index].driver->path[n + 2].distance[0] =
        (elements[index].driver->path[n + 2].pos[0] - elements[index].driver->path[n + 1].pos[0]).Magnitude();
    elements[index].driver->path[n + 3].distance[0] =
        (elements[index].driver->path[n + 3].pos[0] - elements[index].driver->path[n + 2].pos[0]).Magnitude();
    // The first point passed (or near) already: on to the next
    if ((elements[index].car->GetAIPos(true) - p1).Magnitude() < 0.6f ||
        (elements[index].car->GetAIPos(true) - p1).Dot(elements[index].car->velocity) > 0.0f)
        elements[index].driver->next_node++;
    else
        elements[index].driver->unknown_214 = false;
    elements[index].driver->Tick(time);
    elements[index].driver->unknown_214 = true;
    if (elements[index].driver->next_node > n + 1)
        elements[index].blocking_car = NULL;
    elements[index].driver->path.erase(elements[index].driver->path.begin() + n);
    elements[index].driver->path.erase(elements[index].driver->path.begin() + n);
    elements[index].driver->path.erase(elements[index].driver->path.begin() + n);
    elements[index].driver->path.erase(elements[index].driver->path.begin() + n);
    // The node to drive to, from the one before the next node: on while the next one is nearer, then one more once
    // the car is between the two
    int k = n - 1;
    if (!n)
        k = 0;
    S_vector a, b;
    while (true)
    {
        a = elements[index].driver->path[k].pos[elements[index].driver->path[k].cur_lane];
        b = elements[index].driver->path[k + 1].pos[elements[index].driver->path[k + 1].cur_lane];
        if ((a - elements[index].car->GetAIPos(true)).Square() > (b - elements[index].car->GetAIPos(true)).Square() &&
            k < elements[index].driver->path.size() - 1)
            k++;
        else
            break;
    }
    if ((elements[index].car->GetAIPos(true) - a).Dot(elements[index].car->GetAIPos(true) - b) < 0.0f)
    {
        k++;
        if (k >= elements[index].driver->path.size())
            k = elements[index].driver->path.size() - 1;
    }
    // Back while the line from 1 m above the car to it is blocked
    while (k > n)
    {
        S_vector d = elements[index].driver->path[k].pos[elements[index].driver->path[k].cur_lane] -
            (elements[index].car->GetAIPos(true) + S_vector(0.0f, 1.0f, 0.0f));
        if (!gifCollision.TestLineV(elements[index].car->GetAIPos(true) + S_vector(0.0f, 1.0f, 0.0f), d, NULL, NULL,
            COLF_ALL, 0))
            break;
        k--;
    }
    if ((elements[index].driver->path[k].pos[elements[index].driver->path[k].cur_lane] -
        elements[index].car->GetAIPos(true)).Dot(elements[index].car->velocity) < 0.0f)
        elements[index].blocking_car = NULL;
    elements[index].driver->next_node = k;
    if (elements[index].car->GetSpeed() > 30.0f)
        elements[index].car->SetBrake(0.3f);
}

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
        if (e.car->act_state == ACTSTATE_INACTIVE)
            continue;
        if (e.car->act_state == ACTSTATE_NONE)
            e.status = 2;
        if (e.status)
            continue;
        // The target: its car while it sits in one
        if (e.target)
        {
            if (((C_human *)e.target)->car)
                e.chased = ((C_human *)e.target)->car;
            else
                e.chased = e.target;
        }
        // Along the roads: a path of the roads' nodes to the target, found again once the car has passed its last
        // node but one; every 200 ms a line to the target, which, free within 120 m, switches to mode 2
        bool near_target = false;
        if (e.flags & SLEDF_ROADS)
        {
            if (e.driver->next_node >= e.driver->path.size() - 1 || !e.driver->path.size())
            {
                ushort from = road_net->GetNearNode(e.car->GetPos()) | NEARNODE_FLAG;
                ushort to;
                if (e.chased)
                    to = road_net->GetNearNode(e.chased->GetPos()) | NEARNODE_FLAG;
                else
                    to = road_net->GetNearNode(e.target_pos) | NEARNODE_FLAG;
                if (from != to && from != NEARNODE_NONE && to != NEARNODE_NONE)
                {
                    std::vector<ushort> path;
                    road_net->FindPath(from, to, path);
                    e.driver->path.clear();
                    for (uint j = 0; j < path.size(); j++)
                    {
                        S_car_node node;
                        memset(&node, 0, sizeof(node));
                        node.road_node = path[j];
                        e.driver->path.push_back(node);
                    }
                    if (PreparePath(i))
                    {
                        if (!RecompilePath(i))
                        {
                            e.driver->path.clear();
                            e.driver->next_node = 0;
                        }
                    }
                    else
                    {
                        e.driver->path.clear();
                        e.driver->next_node = 0;
                    }
                    for (int k = e.driver->path.size(); k--;)
                        e.driver->path[k].unknown_48 = 1000.0f;
                    S_car_node node;
                    memset(&node, 0, sizeof(node));
                    node.pos[0] = e.car->world_pos;
                    node.cur_lane = 0;
                    node.num_lanes = 1;
                    node.num_type2 = 1;
                    node.unknown_48 = 1000.0f;
                    node.unknown_4c = 1.5f;
                    node.unknown_54 = true;
                    e.driver->path.insert(e.driver->path.begin(), node);
                    if (!e.chased)
                    {
                        node.cur_lane = 0;
                        node.num_lanes = 1;
                        node.num_type2 = 1;
                        node.unknown_48 = 10.0f;
                        node.unknown_4c = 1.5f;
                        node.unknown_54 = true;
                        node.pos[0] = e.target_pos;
                        e.driver->path.push_back(node);
                    }
                }
                e.driver->next_node = 1;
                if (e.timer)
                    e.limit2 = -1.0f;
                else
                    e.limit2 = 10000.0f;
                e.timer = 1;
            }
            // The target 1 m up, or the place
            TickDriver(i, time);
            e.timer2 += time;
            if (e.timer2 > 200)
            {
                e.timer2 = 0;
                S_vector dir;
                if (e.chased)
                {
                    if (e.chased->GetType() == ACTOR_CAR)
                        dir = ((C_car *)e.chased)->world_pos - e.car->world_pos;
                    else
                        dir = (e.chased->GetWorldPos() + S_vector(0.0f, 1.0f, 0.0f)) - e.car->world_pos;
                }
                else
                    dir = (e.target_pos + S_vector(0.0f, 1.0f, 0.0f)) - e.car->world_pos;
                if (dir.Magnitude() < 120.0f)
                {
                    if (!gifCollision.TestLineHStatic(e.car->world_pos, dir, NULL, NULL, 0))
                    {
                        if (!(e.driver->flags & 0x10000000))
                        {
                        e.flags = e.flags & ~SLEDF_ROADS | SLEDF_STRAIGHT;
                        S_car_node node;
                        memset(&node, 0, sizeof(node));
                        node.pos[0] = e.car->world_pos;
                        node.cur_lane = 0;
                        node.num_lanes = 1;
                        node.num_type2 = 1;
                        node.unknown_48 = 1000.0f;
                        node.unknown_4c = 3.0f;
                        node.unknown_54 = true;
                        e.driver->path.clear();
                        e.driver->path.push_back(node);
                        if (e.chased)
                        {
                            if (e.chased->GetType() == ACTOR_CAR)
                                node.pos[0] = ((C_car *)e.chased)->world_pos;
                            else
                                node.pos[0] = e.chased->GetWorldPos() + S_vector(0.0f, 0.2f, 0.0f);
                        }
                        else
                            node.pos[0] = e.target_pos;
                        node.distance[0] = (node.pos[0] - e.car->world_pos).Magnitude();
                        e.driver->path.push_back(node);
                        e.driver->next_node = 1;
                        e.driver->Tick(time);
                        e.timer2 = 0;
                        e.timer = 0;
                        }
                    }
                    else if (!e.driver->path.size())
                        e.status = 2;
                }
            }
            // Further than 100 m away the car is invisible, without its dynamic collisions
            if (e.chased)
            {
                if (!e.car->GetRendered() && (e.chased->GetWorldPos() - e.car->world_pos).Square() > 10000.0f)
                {
                    e.car->invisible = true;
                    e.car->SetDynamicCollsOff(true);
                }
                else
                {
                    e.car->invisible = false;
                    e.car->SetDynamicCollsOff(false);
                }
            }
            else
            {
                if (!e.car->GetRendered() && (e.target_pos - e.car->world_pos).Square() > 10000.0f)
                {
                    e.car->invisible = true;
                    e.car->SetDynamicCollsOff(true);
                }
                else
                {
                    e.car->invisible = false;
                    e.car->SetDynamicCollsOff(false);
                }
            }
            if (e.chased)
            {
                if ((float)e.driver->next_node > e.limit2)
                    e.timer += time;
                if ((float)e.timer > e.limit)
                    e.status = 2;
            }
            // What ends the chase: a car without energy (4), a wrecked car, a broken wheel or a car on its roof for
            // 10 s (1 within 50 m of the target, else 2), a driver stuck for 20 s (1)
            int show = e.car->SetShowEnergy(1);
            if (e.car->ShowCarEnergy() == 0.0f)
                e.status = 4;
            e.car->SetShowEnergy(show);
            if (e.chased)
            {
                if ((e.chased->GetWorldPos() - e.car->world_pos).Square() < 2500.0f)
                    near_target = true;
            }
            else
            {
                if ((e.target_pos - e.car->world_pos).Square() < 2500.0f)
                    near_target = true;
            }
            show = e.car->SetShowEnergy(1);
            if (e.car->ShowCarEnergy() < 0.15f)
                e.status = near_target ? 1 : 2;
            e.car->SetShowEnergy(show);
            for (int n = e.car->GetNumWheels(); n--;)
            {
                if (e.car->GetWheel(n)->flags & 0x40000000)
                {
                    e.status = near_target ? 1 : 2;
                    break;
                }
            }
            if (e.car->GetUp().AngleTo(S_vector(0.0f, 1.0f, 0.0f)) > 0.52f)
            {
                if (e.car->GetSpeed() < 3.0f)
                {
                    e.timer2 += time;
                    if (e.timer2 > 10000)
                        e.status = near_target ? 1 : 2;
                }
                else
                    e.timer2 = 0;
            }
            if (e.driver->unknown_220 > 20000 && (e.car->unknown_2214 || e.car->unknown_2215) &&
                e.car->GetRendered())
                e.status = 1;
        }
        // Straight after the target: its places appended to the path (passed nodes dropped but four), every 500 ms a
        // line to it; near enough (8 m, more by the angle between the two cars' velocities) it switches to mode 4
        else if (e.flags & SLEDF_STRAIGHT)
        {
            if (!(e.driver->flags & 0x10000000))
            {
                while (e.driver->next_node > 4)
                {
                    e.driver->path.erase(e.driver->path.begin());
                    e.driver->next_node--;
                    e.limit2 -= 1.0f;
                }
                float max_dist = 5.0f;
                S_car_node node;
                if (e.chased)
                {
                    if (e.chased->GetType() == ACTOR_CAR && ((C_car *)e.chased)->GetSpeed() > 10.0f)
                        max_dist = ((C_car *)e.chased)->GetSpeed() * 0.3f + 5.0f;
                    node = e.driver->path[e.driver->path.size() - 1];
                    float dist;
                    if (e.chased->GetType() == ACTOR_CAR)
                        dist = (node.pos[0] - ((C_car *)e.chased)->world_pos).Magnitude();
                    else
                        dist = (node.pos[0] - e.chased->GetWorldPos()).Magnitude();
                    if (dist > max_dist)
                    {
                        if (e.chased->GetType() == ACTOR_CAR)
                            node.pos[0] = ((C_car *)e.chased)->world_pos;
                        else
                            node.pos[0] = e.chased->GetWorldPos();
                        node.distance[0] = dist;
                        e.driver->path.push_back(node);
                    }
                }
                else
                {
                    node = e.driver->path[e.driver->path.size() - 1];
                    float dist = (node.pos[0] - e.target_pos).Magnitude();
                    if (dist > max_dist)
                    {
                        node.pos[0] = e.target_pos;
                        node.distance[0] = dist;
                        e.driver->path.push_back(node);
                    }
                }
                TickDriver(i, time);
                if (e.chased)
                {
                    e.timer2 += time;
                    if (e.timer2 > 500)
                    {
                        e.timer2 = 0;
                        S_vector dir;
                        if (e.chased->GetType() == ACTOR_CAR)
                            dir = ((C_car *)e.chased)->world_pos - e.car->world_pos;
                        else
                            dir = (e.chased->GetWorldPos() + S_vector(0.0f, 1.0f, 0.0f)) - e.car->world_pos;
                        if (dir.Magnitude() < 100.0f &&
                            !gifCollision.TestLineHStatic(e.car->world_pos, dir, NULL, NULL, 0))
                        {
                            e.timer = 0;
                            e.limit2 = 100000.0f;
                        }
                        else
                            e.limit2 = -1.0f;
                    }
                }
                else
                {
                    e.timer = 0;
                    e.limit2 = 100000.0f;
                }
                if (e.chased != e.target)
                {
                    e.driver->unknown_1fc = e.target;
                    e.driver->unknown_200 = e.chased;
                    e.driver->unknown_204 = NULL;
                    if (e.chased->GetType() == ACTOR_CAR && ((C_car *)e.chased)->GetSpeed() < 2.0f)
                    {
                        e.driver->unknown_1fc = NULL;
                        e.driver->unknown_200 = NULL;
                        e.driver->unknown_204 = e.chased;
                    }
                }
                else
                {
                    e.driver->unknown_1fc = NULL;
                    e.driver->unknown_200 = NULL;
                    e.driver->unknown_204 = e.chased;
                }
                float dist;
                if (e.chased && e.chased->GetType() == ACTOR_CAR && e.car->GetSpeed() > 8.0f)
                {
                    float angle = e.car->GetVelocityRef().AngleTo(((C_car *)e.chased)->GetVelocityRef());
                    if ((e.chased->GetPos() - e.car->GetPos()).Dot(e.car->GetVelocityRef()) < 0.0f)
                        angle = 0.0f;
                    dist = angle * 40.0f / PI + 8.0f;
                }
                else
                    dist = 8.0f;
                dist *= dist;
                if ((node.pos[0] - e.car->world_pos).Square() < dist)
                {
                    e.timer = 0;
                    e.timer2 = 0;
                    e.flags = e.flags & ~SLEDF_STRAIGHT | SLEDF_BESIDE;
                    if (e.chased == e.target)
                        e.status = 1;
                }
            }
            else
                e.driver->Tick(time);
            // Further than 100 m away the car is invisible, without its dynamic collisions
            if (e.chased)
            {
                if (!e.car->GetRendered() && (e.chased->GetWorldPos() - e.car->world_pos).Square() > 10000.0f)
                {
                    e.car->invisible = true;
                    e.car->SetDynamicCollsOff(true);
                }
                else
                {
                    e.car->invisible = false;
                    e.car->SetDynamicCollsOff(false);
                }
            }
            else
            {
                if (!e.car->GetRendered() && (e.target_pos - e.car->world_pos).Square() > 10000.0f)
                {
                    e.car->invisible = true;
                    e.car->SetDynamicCollsOff(true);
                }
                else
                {
                    e.car->invisible = false;
                    e.car->SetDynamicCollsOff(false);
                }
            }
            if (e.chased)
            {
                if ((float)e.driver->next_node > e.limit2)
                    e.timer += time;
                if ((float)e.timer > e.limit)
                    e.status = 2;
            }
            int show = e.car->SetShowEnergy(1);
            if (e.car->ShowCarEnergy() == 0.0f)
                e.status = 4;
            e.car->SetShowEnergy(show);
            if (e.chased)
            {
                if ((e.chased->GetWorldPos() - e.car->world_pos).Square() < 2500.0f)
                    near_target = true;
            }
            else
            {
                if ((e.target_pos - e.car->world_pos).Square() < 2500.0f)
                    near_target = true;
            }
            show = e.car->SetShowEnergy(1);
            if (e.car->ShowCarEnergy() < 0.15f)
                e.status = near_target ? 1 : 2;
            e.car->SetShowEnergy(show);
            for (int n = e.car->GetNumWheels(); n--;)
            {
                if (e.car->GetWheel(n)->flags & 0x40000000)
                {
                    e.status = near_target ? 1 : 2;
                    break;
                }
            }
            if (e.car->GetUp().AngleTo(S_vector(0.0f, 1.0f, 0.0f)) > 0.52f)
            {
                if (e.car->GetSpeed() < 3.0f)
                {
                    e.timer2 += time;
                    if (e.timer2 > 10000)
                        e.status = near_target ? 1 : 2;
                }
                else
                    e.timer2 = 0;
            }
            if (e.driver->unknown_220 > 20000 && (e.car->unknown_2214 || e.car->unknown_2215) &&
                e.car->GetRendered())
                e.status = 1;
        }
        // Beside the target: a path of two nodes, the car's place and the target's half a second ahead
        else if (e.flags & SLEDF_BESIDE)
        {
            if (!(e.driver->flags & 0x10000000))
            {
                S_car_node node;
                memset(&node, 0, sizeof(node));
                node.pos[0] = e.car->world_pos;
                node.cur_lane = 0;
                node.num_lanes = 1;
                node.num_type2 = 1;
                node.unknown_48 = 1000.0f;
                node.unknown_4c = 2.0f;
                node.unknown_54 = true;
                e.driver->path.clear();
                e.driver->path.push_back(node);
                float max_dist;
                if (e.chased && e.chased->GetType() == ACTOR_CAR && e.car->GetSpeed() > 8.0f)
                {
                    float angle = e.car->GetVelocityRef().AngleTo(((C_car *)e.chased)->GetVelocityRef());
                    if ((e.chased->GetPos() - e.car->GetPos()).Dot(e.car->GetVelocityRef()) < 0.0f)
                        angle = 0.0f;
                    max_dist = angle * 40.0f / PI + 10.0f;
                }
                else
                    max_dist = 10.0f;
                float max_dist_sq = max_dist * max_dist;
                if (e.chased)
                {
                    if (e.chased->GetType() == ACTOR_CAR)
                        node.pos[0] = ((C_car *)e.chased)->world_pos;
                    else
                        node.pos[0] = e.chased->GetWorldPos();
                }
                else
                    node.pos[0] = e.target_pos;
                float dist_sq = (node.pos[0] - e.car->world_pos).Square();
                if (dist_sq > max_dist_sq)
                {
                    e.timer = 0;
                    e.timer2 = 0;
                    e.limit2 = 100000.0f;
                    e.flags = e.flags & ~SLEDF_BESIDE | SLEDF_STRAIGHT;
                }
                else
                {
                    S_vector v;
                    if (e.chased && e.chased->GetType() == ACTOR_CAR)
                        v = ((C_car *)e.chased)->GetVelocityRef() * 0.5f;
                    else
                        v.Zero();
                    node.pos[0] += v;
                }
                node.distance[0] = (node.pos[0] - e.driver->path[0].pos[0]).Magnitude();
                e.driver->path.push_back(node);
                e.driver->next_node = 1;
                if (!(e.flags & (SLEDF_UNKNOWN_20000000 | SLEDF_UNKNOWN_80000000)))
                    e.driver->Tick(time);
                if (e.flags & SLEDF_UNKNOWN_100)
                {
                    if ((node.pos[0] - e.car->world_pos).Square() < 64.0f)
                        e.car->SetBrake(1.0f);
                    e.driver->unknown_1fc = NULL;
                    e.driver->unknown_200 = NULL;
                    e.driver->unknown_204 = e.chased;
                }
                else if ((e.flags & (SLEDF_UNKNOWN_200 | SLEDF_UNKNOWN_2000000)) &&
                    !(e.flags & (SLEDF_UNKNOWN_4000000 | SLEDF_UNKNOWN_8000000)))
                {
                    C_actor *stopped = e.car->unknown_20cc;
                    e.car->unknown_20cc = NULL;
                    if (stopped == e.chased && dist_sq < 64.0f)
                    {
                        e.car->unknown_20cc = e.chased;
                        e.car->SetBrake(1.0f);
                        e.flags &= ~SLEDF_UNKNOWN_2000000;
                    }
                    if (e.flags & SLEDF_UNKNOWN_200)
                    {
                        bool found1 = false;
                        bool found2 = false;
                        bool found3 = false;
                        for (int j = elements.size(); j--;)
                        {
                            if (j != i && elements[j].chased == e.chased)
                            {
                                if ((elements[j].flags & (SLEDF_UNKNOWN_400 | SLEDF_UNKNOWN_1000 |
                                    SLEDF_UNKNOWN_4000000 | SLEDF_UNKNOWN_8000000)) &&
                                    (elements[j].flags & (SLEDF_UNKNOWN_40000000 | SLEDF_UNKNOWN_80000000)))
                                {
                                    if (elements[j].flags & SLEDF_UNKNOWN_1000000)
                                        found2 = true;
                                    else
                                        found1 = true;
                                }
                                if (elements[j].flags & (SLEDF_UNKNOWN_200 | SLEDF_UNKNOWN_2000000))
                                    found3 = true;
                            }
                        }
                        if (found3 && (!found2 || !found1) && !(e.flags & SLEDF_UNKNOWN_200000))
                        {
                            e.flags |= (rand() % 100 > 50 ? SLEDF_UNKNOWN_4000000 : SLEDF_UNKNOWN_8000000) |
                                (SLEDF_UNKNOWN_1000000 | SLEDF_UNKNOWN_80000000);
                            if (!found1)
                                e.flags &= ~SLEDF_UNKNOWN_1000000;
                        }
                    }
                    e.driver->unknown_1fc = e.target;
                    e.driver->unknown_200 = e.chased;
                    e.driver->unknown_204 = NULL;
                }
                else
                {
                    // The chases after a car (0xc001400) and the cars that follow one (0x800), only after a car.
                    // Their points are the target's navigation points (beside it, a side by 0x1000000), the
                    // driver's path its second node.
                    if (e.flags & (SLEDF_UNKNOWN_400 | SLEDF_UNKNOWN_1000 | SLEDF_UNKNOWN_4000000 |
                        SLEDF_UNKNOWN_8000000))
                    {
                        if (e.chased->GetType() == ACTOR_CAR)
                        {
                            if (e.flags & SLEDF_UNKNOWN_40000000)
                            {
                                // Beside the target, until it is behind, too near or turned away (the second change
                                // of the flags by a flag, as PS2's; VC6 threads it away)
                                bool done = false;
                                e.driver->unknown_1fc = e.target;
                                e.driver->unknown_200 = e.chased;
                                e.driver->unknown_204 = NULL;
                                S_vector d = e.car->GetAIPos(true) - ((C_car *)e.chased)->GetAIPos(false);
                                float dist = d.Square();
                                if (e.flags & SLEDF_UNKNOWN_400)
                                {
                                    if (d.Dot(e.car->velocity) < 0.0f || dist < 0.36f ||
                                        (dist < 1.0f && d.AngleTo(e.car->unknown_c3c) < 1.3f))
                                    {
                                        e.flags &= ~SLEDF_UNKNOWN_40000000;
                                        done = true;
                                    }
                                }
                                else
                                {
                                    C_car *target = (C_car *)e.chased;
                                    S_vector wheel =
                                        target->GetWheel(target->GetNumWheels() - 1)->frame->GetWorldPos();
                                    if ((wheel - e.car->GetAIPos(true)).Dot(e.car->velocity) < 0.0f)
                                        e.car->SetBrake(0.01f);
                                    if (e.car->GetSpeed() - ((C_car *)e.chased)->GetSpeed() > 1.0f)
                                        e.car->SetBrake(1.0f);
                                    if (d.AngleTo(e.car->velocity) > 2.1f || dist < 0.36f ||
                                        (dist < 1.0f && d.AngleTo(e.car->unknown_c3c) < 1.3f))
                                    {
                                        e.flags &= ~SLEDF_UNKNOWN_40000000;
                                        done = true;
                                    }
                                }
                                if (done)
                                    e.flags &= ~(SLEDF_UNKNOWN_4000000 | SLEDF_UNKNOWN_8000000);
                            }
                            else if (e.flags & SLEDF_UNKNOWN_80000000)
                            {
                                // To the target's side, ahead by the target's velocity, as long as nothing is in
                                // the way; beside it (0x40000000) once there and both go the same way
                                S_vector p1, p2, p3, p4;
                                ((C_car *)e.chased)->GetNavigationPoints(e.car->GetVehicleSize(false).x + 0.15f,
                                    p1, p2, p3, p4);
                                if (!(e.flags & SLEDF_UNKNOWN_1000000))
                                    p1 = p2;
                                p1 *= e.chased->GetFrame()->GetMatrix();
                                node.pos[0] = p1;
                                S_vector dir = ((C_car *)e.chased)->velocity;
                                dir.Normalize();
                                dir *= e.car->GetVehicleSize(false).z;
                                dir = node.pos[0] + dir - e.car->GetAIPos(true);
                                if (!gifCollision.TestLineH(e.car->GetAIPos(true), dir, NULL, NULL, 0xd02, 0))
                                {
                                    e.driver->path[1].pos[0] = node.pos[0];
                                    e.driver->path[1].distance[0] =
                                        (node.pos[0] - e.driver->path[0].pos[0]).Magnitude();
                                    if (e.flags & SLEDF_UNKNOWN_400)
                                    {
                                        if ((((C_car *)e.chased)->GetAIPos(true) - e.car->GetAIPos(true))
                                            .AngleTo(e.car->velocity) > 1.2f)
                                        {
                                            e.flags &= ~SLEDF_UNKNOWN_80000000;
                                            if (((C_car *)e.chased)->velocity.AngleTo(e.car->velocity) < 0.5f)
                                                e.flags |= SLEDF_UNKNOWN_40000000;
                                        }
                                    }
                                    else
                                    {
                                        if ((e.car->GetAIPos(true) - ((C_car *)e.chased)->GetAIPos(false))
                                            .AngleTo(e.car->velocity) < 1.2f)
                                        {
                                            e.flags &= ~SLEDF_UNKNOWN_80000000;
                                            if (((C_car *)e.chased)->velocity.AngleTo(e.car->velocity) < 0.5f)
                                                e.flags |= SLEDF_UNKNOWN_40000000;
                                        }
                                    }
                                    e.driver->Tick(time);
                                }
                                else
                                {
                                    e.flags &= ~SLEDF_UNKNOWN_80000000;
                                    e.car->SetBrake(1.0f);
                                }
                            }
                            else
                            {
                                if (((C_car *)e.chased)->velocity.AngleTo(e.car->velocity) < 0.5f &&
                                    (e.chased->GetPos() - e.car->GetPos()).Dot(e.car->velocity) > 0.0f)
                                {
                                    // The target ahead, going the same way: a side no other chase of it takes
                                    bool found1 = false;
                                    bool found2 = false;
                                    int j = elements.size();
                                    while (j--)
                                    {
                                        if (j != i && (elements[j].flags & (SLEDF_UNKNOWN_400 | SLEDF_UNKNOWN_1000 |
                                            SLEDF_UNKNOWN_4000000 | SLEDF_UNKNOWN_8000000)) &&
                                            (elements[j].flags & (SLEDF_UNKNOWN_40000000 | SLEDF_UNKNOWN_80000000)) &&
                                            elements[j].chased == e.chased)
                                        {
                                            if (elements[j].flags & SLEDF_UNKNOWN_1000000)
                                                found1 = true;
                                            else
                                                found2 = true;
                                        }
                                    }
                                    // The loop's index after it is -1, as PS2's, so this reads before the vector
                                    if ((found1 || found2) &&
                                        !(elements[j].flags & (SLEDF_UNKNOWN_4000000 | SLEDF_UNKNOWN_8000000)))
                                    {
                                        e.flags = e.flags & ~SLEDF_UNKNOWN_1000000 | SLEDF_UNKNOWN_80000000;
                                        if (!found1)
                                            e.flags |= SLEDF_UNKNOWN_1000000;
                                        else if (found2)
                                        {
                                            e.flags &= ~SLEDF_UNKNOWN_80000000;
                                            if (!(e.flags & SLEDF_UNKNOWN_200000))
                                                e.flags |= SLEDF_UNKNOWN_2000000;
                                        }
                                    }
                                    else
                                    {
                                        // The first side if the line 5 m back along unknown_c48 is free for
                                        // 4 m, else the second if forward is
                                        S_vector p1, p2, p3, p4;
                                        ((C_car *)e.chased)->GetNavigationPoints(
                                            e.car->GetVehicleSize(false).x + 0.15f, p1, p2, p3, p4);
                                        S_vector dir = e.car->GetUnknownC48() * -5.0f;
                                        S_vector from = e.car->world_pos;
                                        S_vector hit;
                                        hit = from + dir;
                                        gifCollision.TestLineH(from, dir, &hit, NULL, 0xd02, 0);
                                        if ((from - hit).Square() > 16.0f)
                                        {
                                            p1 *= e.chased->GetFrame()->GetMatrix();
                                            p3 *= e.chased->GetFrame()->GetMatrix();
                                            dir = p1 - p3;
                                            dir.Normalize();
                                            // A local for the first factor: it stays on the FPU stack, as the
                                            // original's
                                            float speed = e.car->GetSpeed();
                                            dir *= speed * e.car->GetSpeed() * 0.04f;
                                            if (!gifCollision.TestLineH(p3, dir, &hit, NULL, 0xd02, 0))
                                                e.flags |= (SLEDF_UNKNOWN_1000000 | SLEDF_UNKNOWN_80000000);
                                        }
                                        else
                                        {
                                            dir = e.car->GetUnknownC48() * 5.0f;
                                            hit = from + dir;
                                            gifCollision.TestLineH(from, dir, &hit, NULL, 0xd02, 0);
                                            if ((from - hit).Square() > 16.0f)
                                            {
                                                p2 *= e.chased->GetFrame()->GetMatrix();
                                                p4 *= e.chased->GetFrame()->GetMatrix();
                                                dir = p2 - p4;
                                                dir.Normalize();
                                                float speed = e.car->GetSpeed();
                                                dir *= speed * e.car->GetSpeed() * 0.04f;
                                                if (!gifCollision.TestLineH(p4, dir, &hit, NULL, 0xd02, 0))
                                                    e.flags = e.flags & ~SLEDF_UNKNOWN_1000000 | SLEDF_UNKNOWN_80000000;
                                            }
                                        }
                                    }
                                    if (!(e.flags & SLEDF_UNKNOWN_80000000) &&
                                        (e.chased->GetWorldPos() - e.car->GetAIPos(true)).Square() < 25.0f)
                                        e.car->SetBrake(1.0f);
                                }
                                e.driver->unknown_1fc = NULL;
                                e.driver->unknown_200 = NULL;
                                e.driver->unknown_204 = e.chased;
                            }
                        }
                    }
                    else if (e.flags & SLEDF_UNKNOWN_800)
                    {
                        if (e.chased->GetType() == ACTOR_CAR)
                        {
                            if (e.flags & SLEDF_UNKNOWN_20000000)
                            {
                                // Behind the target, by its velocity; until the car is ahead of it
                                e.driver->unknown_1fc = e.target;
                                e.driver->unknown_200 = e.chased;
                                e.driver->unknown_204 = NULL;
                                S_vector dir = ((C_car *)e.chased)->velocity;
                                dir.Normalize();
                                dir *= e.car->GetVehicleSize(false).z + 5.0f;
                                node.pos[0] = dir + ((C_car *)e.chased)->GetAIPos(true);
                                e.driver->path[1].pos[0] = node.pos[0];
                                e.driver->path[1].distance[0] =
                                    (node.pos[0] - e.driver->path[0].pos[0]).Magnitude();
                                e.driver->Tick(time);
                                if ((e.car->GetPos() - ((C_car *)e.chased)->GetAIPos(true))
                                    .AngleTo(((C_car *)e.chased)->velocity) < 0.25f)
                                    e.car->SetBrake(1.0f);
                                if ((e.chased->GetPos() - e.car->GetAIPos(false)).Dot(e.car->GetVelocityRef())
                                    > 0.0f)
                                    e.flags &= ~SLEDF_UNKNOWN_20000000;
                            }
                            else if (e.flags & SLEDF_UNKNOWN_80000000)
                            {
                                // To the target's side, as the chases' (no 0x400 here); behind it (0x20000000)
                                // once there
                                S_vector p1, p2, p3, p4;
                                ((C_car *)e.chased)->GetNavigationPoints(e.car->GetVehicleSize(false).x + 0.15f,
                                    p1, p2, p3, p4);
                                if (!(e.flags & SLEDF_UNKNOWN_1000000))
                                    p1 = p2;
                                p1 *= e.chased->GetFrame()->GetMatrix();
                                S_vector dir = ((C_car *)e.chased)->velocity;
                                dir.Normalize();
                                dir *= e.car->GetVehicleSize(false).z;
                                node.pos[0] = p1 + dir;
                                dir = node.pos[0] - e.car->GetAIPos(true);
                                if (!gifCollision.TestLineH(e.car->GetAIPos(true), dir, NULL, NULL, 0xd02, 0))
                                {
                                    e.driver->path[1].pos[0] = node.pos[0];
                                    e.driver->path[1].distance[0] =
                                        (node.pos[0] - e.driver->path[0].pos[0]).Magnitude();
                                    if ((((C_car *)e.chased)->GetAIPos(true) - e.car->GetAIPos(false))
                                        .Dot(((C_car *)e.chased)->velocity) < 0.0f)
                                    {
                                        e.flags &= ~SLEDF_UNKNOWN_80000000;
                                        if (((C_car *)e.chased)->velocity.AngleTo(e.car->velocity) < 0.5f)
                                            e.flags |= SLEDF_UNKNOWN_20000000;
                                    }
                                    e.driver->Tick(time);
                                }
                                else
                                {
                                    e.flags &= ~SLEDF_UNKNOWN_80000000;
                                    e.car->SetBrake(1.0f);
                                }
                            }
                            else
                            {
                                // The chases' side search without the other cars (PS2 runs the driver after it)
                                if (((C_car *)e.chased)->velocity.AngleTo(e.car->velocity) < 0.5f &&
                                    (e.chased->GetPos() - e.car->GetPos()).Dot(e.car->velocity) > 0.0f)
                                {
                                    S_vector p1, p2, p3, p4;
                                    ((C_car *)e.chased)->GetNavigationPoints(e.car->GetVehicleSize(false).x + 0.15f,
                                        p1, p2, p3, p4);
                                    S_vector dir = e.car->GetUnknownC48() * -5.0f;
                                    S_vector from = e.car->world_pos;
                                    S_vector hit;
                                    hit = from + dir;
                                    gifCollision.TestLineH(from, dir, &hit, NULL, 0xd02, 0);
                                    if ((from - hit).Square() > 16.0f)
                                    {
                                        p1 *= e.chased->GetFrame()->GetMatrix();
                                        p3 *= e.chased->GetFrame()->GetMatrix();
                                        dir = p1 - p3;
                                        dir.Normalize();
                                        float speed = e.car->GetSpeed();
                                        dir *= speed * e.car->GetSpeed() * 0.04f;
                                        if (!gifCollision.TestLineH(p3, dir, &hit, NULL, 0xd02, 0))
                                            e.flags |= (SLEDF_UNKNOWN_1000000 | SLEDF_UNKNOWN_80000000);
                                    }
                                    else
                                    {
                                        dir = e.car->GetUnknownC48() * 5.0f;
                                        hit = from + dir;
                                        gifCollision.TestLineH(from, dir, &hit, NULL, 0xd02, 0);
                                        if ((from - hit).Square() > 16.0f)
                                        {
                                            p2 *= e.chased->GetFrame()->GetMatrix();
                                            p4 *= e.chased->GetFrame()->GetMatrix();
                                            dir = p2 - p4;
                                            dir.Normalize();
                                            float speed = e.car->GetSpeed();
                                            dir *= speed * e.car->GetSpeed() * 0.04f;
                                            if (!gifCollision.TestLineH(p4, dir, &hit, NULL, 0xd02, 0))
                                                e.flags = e.flags & ~SLEDF_UNKNOWN_1000000 | SLEDF_UNKNOWN_80000000;
                                        }
                                    }
                                    if (!(e.flags & SLEDF_UNKNOWN_80000000) &&
                                        (e.chased->GetWorldPos() - e.car->GetAIPos(true)).Square() < 25.0f)
                                        e.car->SetBrake(1.0f);
                                }
                                e.driver->unknown_1fc = NULL;
                                e.driver->unknown_200 = NULL;
                                e.driver->unknown_204 = e.chased;
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
                if (e.car->GetWheel(n)->flags & 0x40000000)
                {
                    e.status = 1;
                    break;
                }
            }
            if (e.car->GetUp().AngleTo(S_vector(0.0f, 1.0f, 0.0f)) > 0.52f)
            {
                if (e.car->GetSpeed() < 3.0f)
                {
                    e.timer2 += time;
                    if (e.timer2 > 10000)
                        e.status = 1;
                }
                else
                    e.timer2 = 0;
            }
            if (e.driver->unknown_220 > 20000 || !e.chased && e.driver->unknown_220 > 10000)
            {
                if ((e.car->unknown_2214 || e.car->unknown_2215) && e.car->GetRendered())
                    e.status = 1;
            }
            if (e.chased && e.chased->GetType() == ACTOR_CAR)
            {
                if (((C_car *)e.chased)->IsUnknown540())
                    e.timer = 0;
                else
                {
                    e.timer += time;
                    if (e.timer > 500 && ((C_car *)e.chased)->GetSpeed() < 1.5f)
                        e.status = 1;
                }
            }
            else
                e.status = 1;
        }
        // The driver's path, braking where the target is behind the car and too near
        else if (e.flags & (SLEDF_ESCAPE | SLEDF_UNKNOWN_100000))
        {
            e.driver->unknown_208 = 1;
            if (!(e.driver->flags & 0x10000000))
            {
                TickDriver(i, time);
                if (!(e.flags & SLEDF_UNKNOWN_100000))
                {
                    S_vector dir = e.chased->GetWorldPos() - e.car->GetWorldPos();
                    if (dir.Square() > e.limit && e.car->GetSpeed() > e.limit2 &&
                        e.car->GetVelocityRef().Dot(dir) < 0.0f)
                        e.car->SetBrake(1.0f);
                }
                else
                {
                    e.timer += time;
                    if (e.timer > 15000)
                    {
                        e.flags &= ~SLEDF_UNKNOWN_100000;
                        e.driver->unknown_298 = false;
                        e.timer = 0;
                    }
                }
            }
            else
                e.driver->Tick(time);
            if (!e.driver->next_node)
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
                if (e.car->GetWheel(n)->flags & 0x40000000)
                {
                    e.status = 2;
                    break;
                }
            }
            if (e.car->GetUp().AngleTo(S_vector(0.0f, 1.0f, 0.0f)) > 0.52f)
            {
                if (e.car->GetSpeed() < 3.0f)
                {
                    e.timer2 += time;
                    if (e.timer2 > 10000)
                        e.status = 2;
                }
                else
                    e.timer2 = 0;
            }
            if (e.driver->unknown_220 > 20000 && (e.car->unknown_2214 || e.car->unknown_2215) &&
                e.car->GetRendered())
                e.status = 1;
        }
        // The driver's path; a car stopped for 2 s (15 s where its node's table entry has 4) sets 0x100000
        else if ((e.flags & SLEDF_MOVE_ROADS) && !(e.flags & SLEDF_UNKNOWN_100000))
        {
            e.driver->unknown_208 = 0;
            e.driver->Tick(time);
            if (!e.driver->next_node)
                e.status = 1;
            if (!(e.flags & SLEDF_UNKNOWN_200000))
            {
                if (e.car->GetEngineOn() && e.car->GetSpeed() < 1.0f)
                    e.timer += time;
                else
                    e.timer = 0;
                if (e.timer > 2000)
                {
                    if (e.driver->blocking_car)
                    {
                        if (e.driver->path[e.driver->next_node].semaphore_phase &&
                            !(phase_colors[*e.driver->path[e.driver->next_node].semaphore_phase] & 4))
                            e.timer -= time;
                        else if (e.timer > 15000)
                        {
                            e.flags |= SLEDF_UNKNOWN_100000;
                            e.driver->unknown_298 = true;
                            e.timer = 0;
                        }
                    }
                    else
                    {
                        e.flags |= SLEDF_UNKNOWN_100000;
                        e.driver->unknown_298 = true;
                        e.timer = 0;
                    }
                }
            }
        }
        else if (e.flags & SLEDF_MOVE_DIRECT)
        {
            e.driver->Tick(time);
            if (!e.driver->next_node)
                e.status = 1;
        }
        // The horn while horn_time runs, which a traffic car ahead keeps up, unless the siren is on; the brakes near
        // a slow target
        if (!(e.flags & (SLEDF_MOVE_DIRECT | SLEDF_MOVE_ROADS)))
        {
            if (!e.car->GetSiren() && !(e.flags & SLEDF_UNKNOWN_100000))
            {
                e.horn_time -= time;
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
                if (e.horn_time > 0)
                    e.car->SetHorn(true);
                else
                {
                    e.car->SetHorn(false);
                    if (k < 0)
                        e.horn_time = -1;
                }
                if (e.car->GetSpeed() > 4.0f && e.horn_time < -200 - rand() % 800)
                    e.horn_time = rand() % 600 + 200;
            }
            if (!(e.flags & SLEDF_ESCAPE))
            {
                if (e.chased != e.target)
                {
                    float dist_sq = (e.car->GetPos() - e.chased->GetPos()).Square();
                    if (((C_car *)e.chased)->GetSpeed() < 2.0f && dist_sq < 900.0f)
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
                        if (e.flags & (SLEDF_UNKNOWN_200 | SLEDF_UNKNOWN_2000000))
                        {
                            if (e.car->GetSpeed() - ((C_car *)e.chased)->GetSpeed() > 15.0f &&
                                e.car->GetSpeed() > 15.0f)
                                e.car->SetBrake(1.0f);
                        }
                        else if (dist_sq < 225.0f)
                        {
                            if (e.car->GetSpeed() - ((C_car *)e.chased)->GetSpeed() > 4.0f)
                                e.car->SetBrake(1.0f);
                        }
                        else if (e.car->GetSpeed() - ((C_car *)e.chased)->GetSpeed() > 4.0f &&
                            e.car->GetSpeed() > 17.0f)
                            e.car->SetBrake(1.0f);
                    }
                }
                else if (e.chased)
                {
                    float dist_sq = (e.car->GetPos() - e.chased->GetPos()).Square();
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
        // A dead driver (seat 0) ends the chase (3) and the loop
        C_human *owner = e.car->GetOwner(0);
        if (owner && owner->GetDead())
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
            e.car->SetAutoGearing(false);
            e.car->SetBrake(1.0f);
            e.car->SetPower(0.0f, true);
        }
    }
}




// The car's element, searched from the last one in each of these (PS2's the same)
// FUNCTION: GAME 0x4a8670
uint C_sledovacka::ActionStatus(C_car *car)
{
    for (int i = elements.size(); i--;)
    {
        if (elements[i].car == car)
            return elements[i].status;
    }
    return STATUS_DONE;
}

// FUNCTION: GAME 0x4a86d0
bool C_sledovacka::SetActionStatus(C_car *car, E_STATUS status)
{
    for (int i = elements.size(); i--;)
    {
        if (elements[i].car == car)
        {
            elements[i].status = status;
            return true;
        }
    }
    return false;
}

// FUNCTION: GAME 0x4a8730
int C_sledovacka::GetVoleMode(C_car *car)
{
    for (int i = elements.size(); i--;)
    {
        if (elements[i].car == car)
            return elements[i].vole_mode;
    }
    return 0;
}

// The actor is gone: its car taken from the chases, the chases after it done, and the drivers forget it
// FUNCTION: GAME 0x4a8790
void C_sledovacka::VypicujMiToyotu(C_actor *act)
{
    for (int i = elements.size(); i--;)
    {
        if (act == elements[i].car)
        {
            RemoveCar((C_car *)act);
            continue;
        }
        if (act == elements[i].chased)
            elements[i].status = STATUS_DONE;
        if (act == elements[i].target)
            elements[i].status = STATUS_DONE;
        if (act == elements[i].driver->blocking_car)
        {
            elements[i].driver->blocking_car = NULL;
            elements[i].driver->unknown_19c &= ~DRIVER19C_BLOCKED;
        }
        if (act == elements[i].blocking_car)
        {
            elements[i].blocking_car = NULL;
            elements[i].driver->unknown_19c &= ~DRIVER19C_BLOCKED;
        }
        if (act == elements[i].driver->unknown_1a4)
        {
            elements[i].driver->unknown_1a4 = NULL;
            elements[i].driver->unknown_19c &= ~DRIVER19C_BLOCKED;
        }
        if (act == elements[i].driver->unknown_1fc)
            elements[i].driver->unknown_1fc = NULL;
        if (act == elements[i].driver->unknown_200)
            elements[i].driver->unknown_200 = NULL;
    }
}

// FUNCTION: GAME 0x4a88c0
uint C_sledovacka::DevaaateeeZasNeznaCas(C_car *car)
{
    uint time = 0;
    int i;
    for (i = elements.size(); i--;)
    {
        if (elements[i].car == car)
            break;
    }
    if (i >= 0 && elements[i].vole_mode == 1 && !(elements[i].flags & SLEDF_BESIDE) && elements[i].chased)
        time = elements[i].timer;
    return time;
}

// The cars whose chase can be saved: the car still in the game, it and the actors the chase and its driver keep (a
// driver's 0x1fc and 0x200 can be 1) not unknown_65; a record each and its path's nodes, after the head
// FUNCTION: GAME 0x4a8940
int C_sledovacka::SaveGameGetSize()
{
    int nodes = 0;
    int num = 0;
    for (int i = elements.size(); i--;)
    {
        S_ELEMENT &e = elements[i];
        if (e.car->unknown_65 || e.car->act_state == ACTSTATE_NONE)
            continue;
        if (e.chased && e.chased->unknown_65)
            continue;
        if (e.target && e.target->unknown_65)
            continue;
        if (e.driver->blocking_car && e.driver->blocking_car->unknown_65)
            continue;
        if (e.driver->unknown_1a4 && e.driver->unknown_1a4->unknown_65)
            continue;
        if ((uint)e.driver->unknown_1fc > 1 && e.driver->unknown_1fc->unknown_65)
            continue;
        if ((uint)e.driver->unknown_200 > 1 && e.driver->unknown_200->unknown_65)
            continue;
        num++;
        nodes += e.driver->path.size() * sizeof(S_car_node);
    }
    return sizeof(S_sledovacka_savegame_head) + num * sizeof(S_sledovacka_savegame) + nodes;
}

// The records first, then the nodes of the first num elements' paths (from the last of those), which are the saved
// cars' only where none was left out
// Open: the registers (57%, 85% with registers and stack masked): the original keeps this in ebp and the record's
// pointer 8 bytes in, and tests the actors with cmp byte ptr where ours loads the byte
// FUNCTION: GAME 0x4a8a50
bool C_sledovacka::SaveGameSave(void **buf)
{
    int num = 0;
    S_sledovacka_savegame_head *head = (S_sledovacka_savegame_head *)*buf;
    head->version = SLEDOVACKA_SAVEGAME_VERSION;
    *buf = (uchar *)*buf + sizeof(S_sledovacka_savegame_head);
    S_sledovacka_savegame *rec = (S_sledovacka_savegame *)*buf;
    int i;
    for (i = elements.size(); i--;)
    {
        if (elements[i].car->unknown_65 || elements[i].car->act_state == ACTSTATE_NONE)
            continue;
        if (elements[i].chased && elements[i].chased->unknown_65)
            continue;
        if (elements[i].target && elements[i].target->unknown_65)
            continue;
        if (elements[i].driver->blocking_car && elements[i].driver->blocking_car->unknown_65)
            continue;
        if (elements[i].driver->unknown_1a4 && elements[i].driver->unknown_1a4->unknown_65)
            continue;
        if ((uint)elements[i].driver->unknown_1fc > 1 && elements[i].driver->unknown_1fc->unknown_65)
            continue;
        if ((uint)elements[i].driver->unknown_200 > 1 && elements[i].driver->unknown_200->unknown_65)
            continue;
        memset(rec, 0, sizeof(S_sledovacka_savegame));
        rec->car = elements[i].car->number;
        if (elements[i].chased)
            rec->chased = elements[i].chased->number;
        if (elements[i].target)
            rec->target = elements[i].target->number;
        rec->flags = elements[i].flags;
        rec->status = elements[i].status;
        rec->vole_mode = elements[i].vole_mode;
        rec->unknown_28 = elements[i].timer;
        rec->unknown_2c = elements[i].timer2;
        rec->driver_124 = elements[i].driver->unknown_124;
        memcpy(rec->driver_130, elements[i].driver->unknown_130, sizeof(rec->driver_130));
        memcpy(rec->driver_14c, elements[i].driver->unknown_14c, sizeof(rec->driver_14c));
        rec->next_node = elements[i].driver->next_node;
        rec->driver_174 = elements[i].driver->unknown_174;
        rec->driver_17c = elements[i].driver->unknown_17c;
        rec->driver_180 = elements[i].driver->unknown_180;
        rec->driver_184 = elements[i].driver->unknown_184;
        rec->driver_188 = elements[i].driver->unknown_188;
        rec->driver_18a = elements[i].driver->unknown_18a;
        rec->driver_18c = elements[i].driver->unknown_18c;
        rec->driver_190 = elements[i].driver->unknown_190;
        rec->driver_194 = elements[i].driver->unknown_194;
        rec->driver_19c = elements[i].driver->unknown_19c;
        if (elements[i].driver->blocking_car)
            rec->driver_blocking_car = elements[i].driver->blocking_car->number;
        if (elements[i].driver->unknown_1a4)
            rec->driver_1a4 = elements[i].driver->unknown_1a4->number;
        rec->driver_1bc = elements[i].driver->unknown_1bc;
        rec->driver_1c4 = elements[i].driver->unknown_1c4;
        rec->driver_1d8 = elements[i].driver->unknown_1d8;
        rec->driver_flags = elements[i].driver->flags;
        rec->driver_1e0 = elements[i].driver->unknown_1e0;
        rec->driver_218 = elements[i].driver->unknown_218;
        if ((uint)elements[i].driver->unknown_1fc > 1)
            rec->driver_1fc = elements[i].driver->unknown_1fc->number;
        if ((uint)elements[i].driver->unknown_200 > 1)
            rec->driver_200 = elements[i].driver->unknown_200->number;
        rec->driver_208 = elements[i].driver->unknown_208;
        rec->num_nodes = elements[i].driver->path.size();
        rec->unknown_38 = elements[i].limit;
        rec->unknown_3c = elements[i].limit2;
        num++;
        rec++;
    }
    head->num = num;
    *buf = (uchar *)*buf + num * sizeof(S_sledovacka_savegame);
    for (i = head->num; i--;)
    {
        if (elements[i].car->unknown_65)
            continue;
        if (elements[i].chased && elements[i].chased->unknown_65)
            continue;
        if (elements[i].target && elements[i].target->unknown_65)
            continue;
        if (elements[i].driver->blocking_car && elements[i].driver->blocking_car->unknown_65)
            continue;
        if (elements[i].driver->unknown_1a4 && elements[i].driver->unknown_1a4->unknown_65)
            continue;
        if ((uint)elements[i].driver->unknown_1fc > 1 && elements[i].driver->unknown_1fc->unknown_65)
            continue;
        if ((uint)elements[i].driver->unknown_200 > 1 && elements[i].driver->unknown_200->unknown_65)
            continue;
        memcpy(*buf, elements[i].driver->path.begin(), elements[i].driver->path.size() * sizeof(S_car_node));
        *buf = (uchar *)*buf + elements[i].driver->path.size() * sizeof(S_car_node);
    }
    return true;
}

// The chases there are let go; each record's car chases again with a new driver (whose actors are looked up only
// where the new driver has them already, and the chased one where the record has a car: the original's), then the
// paths' nodes and their roads' lanes, as PreparePath, for the chases along the roads. The nodes' counts aren't freed.
// FUNCTION: GAME 0x4a8fa0
bool C_sledovacka::SaveGameLoad(void **buf)
{
    S_sledovacka_savegame_head *head = (S_sledovacka_savegame_head *)*buf;
    if (head->version != SLEDOVACKA_SAVEGAME_VERSION)
        return false;
    *buf = (uchar *)*buf + sizeof(S_sledovacka_savegame_head);
    int i;
    for (i = elements.size(); i--;)
        RemoveCar(elements[i].car);
    ushort *nodes = new ushort[head->num];
    S_sledovacka_savegame *rec = (S_sledovacka_savegame *)*buf;
    for (i = 0; i < head->num; i++, rec++)
    {
        S_ELEMENT e;
        memset(&e, 0, sizeof(e));
        e.car = (C_car *)act_mission->game->GetActorFromGUID(rec->car);
        if (rec->car)
            e.chased = act_mission->game->GetActorFromGUID(rec->chased);
        if (rec->target)
            e.target = act_mission->game->GetActorFromGUID(rec->target);
        e.flags = rec->flags;
        e.status = rec->status;
        e.vole_mode = rec->vole_mode;
        e.timer = rec->unknown_28;
        e.timer2 = rec->unknown_2c;
        e.car->SetUnknownC68(-1);
        e.car->unknown_bc = true;
        e.driver = new C_driver;
        e.driver->SetCar(e.car);
        e.driver->unknown_124 = rec->driver_124;
        memcpy(e.driver->unknown_130, rec->driver_130, sizeof(rec->driver_130));
        memcpy(e.driver->unknown_14c, rec->driver_14c, sizeof(rec->driver_14c));
        e.driver->next_node = rec->next_node;
        e.driver->unknown_174 = rec->driver_174;
        e.driver->unknown_17c = rec->driver_17c;
        e.driver->unknown_180 = rec->driver_180;
        e.driver->unknown_184 = rec->driver_184;
        e.driver->unknown_188 = rec->driver_188;
        e.driver->unknown_18a = rec->driver_18a;
        e.driver->unknown_18c = rec->driver_18c;
        e.driver->unknown_190 = rec->driver_190;
        e.driver->unknown_194 = rec->driver_194;
        e.driver->unknown_19c = rec->driver_19c;
        if (e.driver->blocking_car)
            e.driver->blocking_car = (C_car *)act_mission->game->GetActorFromGUID(rec->driver_blocking_car);
        if (e.driver->unknown_1a4)
            e.driver->unknown_1a4 = act_mission->game->GetActorFromGUID(rec->driver_1a4);
        e.driver->unknown_1bc = rec->driver_1bc;
        e.driver->unknown_1c4 = rec->driver_1c4;
        e.driver->unknown_1d8 = rec->driver_1d8;
        e.driver->flags = rec->driver_flags;
        e.driver->unknown_1e0 = rec->driver_1e0;
        e.driver->unknown_218 = rec->driver_218;
        if (e.driver->unknown_1fc)
            e.driver->unknown_1fc = act_mission->game->GetActorFromGUID(rec->driver_1fc);
        if (e.driver->unknown_200)
            e.driver->unknown_200 = act_mission->game->GetActorFromGUID(rec->driver_200);
        e.driver->unknown_208 = rec->driver_208;
        nodes[i] = rec->num_nodes;
        e.limit = rec->unknown_38;
        e.limit2 = rec->unknown_3c;
        elements.push_back(e);
        mafiaIndicators.RadarRemoveCar(e.car);
        mafiaIndicators.RadarAddCar(e.car, 0xffff0000);
    }
    *buf = (uchar *)*buf + head->num * sizeof(S_sledovacka_savegame);
    for (i = head->num; i--;)
    {
        elements[i].driver->path.resize(nodes[i]);
        memcpy(elements[i].driver->path.begin(), *buf, nodes[i] * sizeof(S_car_node));
        *buf = (uchar *)*buf + nodes[i] * sizeof(S_car_node);
    }
    if (!road_net)
        road_net = act_mission->roads;
    for (i = head->num; i--;)
    {
        if (elements[i].flags & (SLEDF_STRAIGHT | SLEDF_BESIDE | SLEDF_MOVE_DIRECT))
            continue;
        if (elements[i].driver->path.size() < 2)
            continue;
        ushort prev = elements[i].driver->path[1].road_node;
        ushort n = elements[i].driver->path[0].road_node;
        while (n & NEARNODE_FLAG)
        {
            ushort next;
            if (road_net->links[n & NEARNODE_INDEX].end0 == prev)
                next = road_net->links[n & NEARNODE_INDEX].end1;
            else
                next = road_net->links[n & NEARNODE_INDEX].end0;
            prev = n;
            n = next;
        }
        int k;
        for (k = 0; k < 4; k++)
        {
            if (road_net->nodes[n].conns[k] == prev)
                break;
        }
        if (k == 4)
            continue;
        S_road_lanes *lanes = &road_net->nodes[n].lanes[k];
        for (uint j = 0; j < elements[i].driver->path.size(); j++)
        {
            if (elements[i].driver->path[j].road_node & NEARNODE_FLAG || j == elements[i].driver->path.size() - 1)
                elements[i].driver->path[j].lanes = lanes;
            else
            {
                elements[i].driver->path[j].lanes = lanes;
                for (k = 0; k < 4; k++)
                {
                    if (road_net->nodes[elements[i].driver->path[j].road_node].conns[k] ==
                        elements[i].driver->path[j + 1].road_node)
                        break;
                }
                if (k == 4)
                    break;
                lanes = &road_net->nodes[elements[i].driver->path[j].road_node].lanes[k];
            }
        }
    }
    return true;
}
