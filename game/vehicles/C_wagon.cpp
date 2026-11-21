// The wagons (PS2 C_wagon.cpp), file 47 of docs/game-files.md: 0x4f1b50-0x4f5dbc, in PS2's order: the elevated
// railway's scripted train, a rail vehicle of its own whose path functions are C_railway's twins (IsStopPoint folded
// with C_railway's copy). GameInit's frame walker (S_hlp::cbEnumFrame) follows GameInit, its local struct's.
// Open: Update's S_quat product (its nested Dot and Cross inlined the other way round), GameInit's and AI's registers
// and frames, FindBackPos's operator*(float) and CreateForwardVector's load order (the budget), RecompileNewPath's
// registers.

#include "vehicles/C_wagon.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "actors/C_detector.h"
#include "ai/jp_ai_entity.h"
#include "C_chunk.h"
#include "collision/g_c_glob.h"
#include "collision/g_colls.h"
#include "I3D_cache.h"
#include "mission/C_program.h"
#include "mission/game.h"
#include "mission/Mission.h"
#include "tools.h"
#include "vehicles/C_car.h"
#include "vehicles/C_Vehicle.h"

// FUNCTION: GAME 0x4f1b50
C_wagon::C_wagon()
{
    type = ACTOR_WAGON;
    num_wagons = 0;
    wagon_length = 9.4f;
    vehicle_length = 5.2f;
    max_speed = 5.0f;
    acceleration = .5f;
}

// Declared: it stores the vtable again before the members' destructors
// FUNCTION: GAME 0x4f1c20
C_wagon::~C_wagon()
{
}

// The base's, its result converted again: the linker folded it with C_door's
// FUNCTION: GAME 0x4393a0 FOLDED
bool C_wagon::Init(I3D_frame *frm)
{
    return C_actor::Init(frm) ? true : false;
}

// FUNCTION: GAME 0x4f1ca0
int C_wagon::SaveGameGetSize()
{
    int size = C_actor::SaveGameGetSize() + 95;
    size += sizeof(uint) * forward_nodes.size();
    size += sizeof(uint) * back_nodes.size();
    return size;
}

// Saves the position, direction, the path's state, the actors it waits for and the script's event, then the two
// vectors' nodes (S_wagon_savegame)
// FUNCTION: GAME 0x4f1cf0
bool C_wagon::SaveGameSave(void **buf)
{
    if (!C_actor::SaveGameSave(buf))
        return false;
    S_wagon_savegame *s = (S_wagon_savegame *)*buf;
    s->version = WAGON_SAVEGAME_VERSION;
    s->pos = pos;
    s->dir = dir;
    s->deactivate_count = deactivate_count;
    s->stopped = stopped;
    s->found_next = found_next;
    s->back_pos = back_pos;
    s->front_pos = front_pos;
    s->stop_time = stop_time;
    s->drive_speed = drive_speed;
    s->safe_speed = safe_speed;
    int number = -1;
    if (blocker)
        s->blocker_number = blocker->number;
    else
        s->blocker_number = number;
    s->event_id = event_id;
    if (event_program && event_program->actor)
        number = event_program->actor->number;
    s->event_actor_number = number;
    s->num_forward_nodes = forward_nodes.size();
    s->num_back_nodes = back_nodes.size();
    s->node_dir = node_dir;
    *buf = (uchar *)*buf + sizeof(S_wagon_savegame);
    if (s->num_forward_nodes)
        memcpy(*buf, &forward_nodes[0], s->num_forward_nodes * sizeof(uint));
    *buf = (uchar *)*buf + s->num_forward_nodes * sizeof(uint);
    if (s->num_back_nodes)
        memcpy(*buf, &back_nodes[0], s->num_back_nodes * sizeof(uint));
    *buf = (uchar *)*buf + s->num_back_nodes * sizeof(uint);
    return true;
}

// Reads what SaveGameSave wrote, finds the actors again and places the frame and its collision
// FUNCTION: GAME 0x4f1eb0
bool C_wagon::SaveGameLoad(void **buf)
{
    if (!C_actor::SaveGameLoad(buf))
        return false;
    S_wagon_savegame *s = (S_wagon_savegame *)*buf;
    if (s->version != WAGON_SAVEGAME_VERSION)
        return false;
    pos = s->pos;
    dir = s->dir;
    deactivate_count = s->deactivate_count;
    stopped = s->stopped;
    found_next = s->found_next;
    back_pos = s->back_pos;
    front_pos = s->front_pos;
    stop_time = s->stop_time;
    drive_speed = s->drive_speed;
    safe_speed = s->safe_speed;
    C_actor *act = act_mission->GetGame()->GetActorFromGUID(s->blocker_number);
    if (act && act->type != ACTOR_WAGON)
        act = NULL;
    blocker = act;
    event_id = s->event_id;
    event_program = NULL;
    act = act_mission->GetGame()->GetActorFromGUID(s->event_actor_number);
    if (act)
    {
        if (act->type == ACTOR_DETECTOR)
            event_program = (C_detector *)act;
        else if (act->type == ACTOR_ENTITY)
            event_program = (C_entity *)act;
    }
    node_dir = s->node_dir;
    back_nodes.clear();
    forward_nodes.clear();
    *buf = (uchar *)*buf + sizeof(S_wagon_savegame);
    if (s->num_forward_nodes)
    {
        forward_nodes.resize(s->num_forward_nodes);
        memcpy(&forward_nodes[0], *buf, s->num_forward_nodes * sizeof(uint));
    }
    *buf = (uchar *)*buf + s->num_forward_nodes * sizeof(uint);
    if (s->num_back_nodes)
    {
        back_nodes.resize(s->num_back_nodes);
        memcpy(&back_nodes[0], *buf, s->num_back_nodes * sizeof(uint));
    }
    *buf = (uchar *)*buf + s->num_back_nodes * sizeof(uint);
    frame->SetPos(pos);
    frame->SetDir(dir, 0.0f);
    frame->Update();
    coll_matrix = GetFrame()->GetMatrix();
    coll_matrix.Invert();
    I3D_bvolume bvol = GetFrame()->GetHRBoundVolume();
    ((I3D_bsphere &)coll.bsphere).pos = bvol.bsphere.pos * GetFrame()->GetMatrix();
    gifCollision.DynUpdate(&coll);
    return true;
}

// Extends the back vector with the nodes behind its last one, along the closed connections but the one back to the
// node before, until it has 100 of them or three lie farther from pos than back_dist
// FUNCTION: GAME 0x4f22d0
void C_wagon::CreateBackVector(S_vector pos)
{
    uint count = 0;
    while (back_nodes.size() < 100)
    {
        C_web_node *node = &act_mission->web_path->nodes[back_nodes.back()];
        bool found = false;
        uint i = node->num_conns;
        while (i)
        {
            i--;
            if (node->GetConn(i)->flags & WEBF_ONEWAY)
            {
                if (back_nodes.size() >= 2 && back_nodes[back_nodes.size() - 2] == node->GetConn(i)->node)
                    continue;
                found = true;
                back_nodes.push_back(node->GetConn(i)->node);
                break;
            }
        }
        if (!found)
            return;
        float dist = (act_mission->web_path->nodes[back_nodes.back()].GetPos() - pos).Magnitude();
        float limit = back_dist;
        if (dist > limit)
            count++;
        if (count > 2)
            break;
    }
}

// Extends the forward vector from node index, along the connections not closed but the one back to the node before,
// until its length passes dist or it has 100 nodes
// FUNCTION: GAME 0x4f24c0
void C_wagon::CreateForwardVector(uint index, float dist)
{
    float total = 0.0f;
    uint n = forward_nodes.size();
    if (n)
    {
        S_vector prev = GetCheckpoints()[forward_nodes[0]].GetPos();
        for (uint i = 1; i < n; i++)
        {
            S_vector cur = GetCheckpoints()[forward_nodes[i]].GetPos();
            prev -= cur;
            total += prev.Magnitude();
            prev = GetCheckpoints()[forward_nodes[i]].GetPos();
        }
    }
    if (!(total > dist))
    {
        C_web_node *node;
        if (n)
            node = &GetCheckpoints()[forward_nodes[n - 1]];
        else
            node = &GetCheckpoints()[index];
        while (forward_nodes.size() < 100 && !(total > dist))
        {
            S_vector prev = node->GetPos();
            found_next = false;
            uint i = node->num_conns;
            while (i)
            {
                i--;
                if (!(node->GetConn(i)->flags & WEBF_ONEWAY))
                {
                    if (forward_nodes.size() >= 2 && forward_nodes[forward_nodes.size() - 2] == node->GetConn(i)->node)
                        continue;
                    found_next = true;
                    forward_nodes.push_back(node->GetConn(i)->node);
                    node = &GetCheckpoints()[node->GetConn(i)->node];
                    break;
                }
            }
            if (!found_next)
                break;
            prev -= node->GetPos();
            total += prev.Magnitude();
        }
    }
}

// Whether the path node is a stop: the linker folded it with C_railway's identical copy
// FUNCTION: GAME 0x4f28c0 FOLDED
bool C_wagon::IsStopPoint(uint index)
{
    return (act_mission->web_path->nodes[index].flags & WEBF_UNKNOWN_8) ? true : false;
}

// The distance to the next stop along the path ahead, -1 for none
// FUNCTION: GAME 0x4f28f0
float C_wagon::MustStop()
{
    if (back_nodes.size() != 0)
    {
        C_web_node *node = &act_mission->web_path->nodes[back_nodes[0]];
        S_vector prev = front_pos - node->GetPos();
        float dist = 0.0f;
        dist += prev.Magnitude();
        prev = node->GetPos();
        if (IsStopPoint(back_nodes[0]))
            return dist;
        int count = forward_nodes.size();
        for (int i = 0; i < count; i++)
        {
            node = &act_mission->web_path->nodes[forward_nodes[i]];
            prev -= node->GetPos();
            dist += prev.Magnitude();
            if (IsStopPoint(forward_nodes[i]))
                return dist;
            prev = node->GetPos();
        }
    }
    return -1.0f;
}

// The position dist along the path behind pos, from the node index on, which it moves on to
// FUNCTION: GAME 0x4f2b50
S_vector C_wagon::FindBackPos(S_vector pos, float dist, int *index)
{
    uint size = back_nodes.size();
    uint i = *index;
    S_vector d;
    for (; i <= size; i++)
    {
        d = act_mission->web_path->nodes[back_nodes[i]].GetPos() - pos;
        if (d.Magnitude() > dist + 0.001f)
            break;
    }
    if (i == size)
        i--;
    S_vector result;
    if (i == *index)
    {
        d.Normalize();
        result = pos + d * dist;
    }
    else
    {
        S_vector a = act_mission->web_path->nodes[back_nodes[i - 1]].GetPos() - pos;
        S_vector b = act_mission->web_path->nodes[back_nodes[i]].GetPos() - pos;
        S_vector c = act_mission->web_path->nodes[back_nodes[i]].GetPos() -
                     act_mission->web_path->nodes[back_nodes[i - 1]].GetPos();
        float len_a = a.Magnitude();
        float len_b = b.Magnitude();
        float delta = len_b - len_a;
        if (delta < MRG_ZERO)
        {
            result = act_mission->web_path->nodes[back_nodes[i]].GetPos();
        }
        else
        {
            S_vector v = c * ((dist - len_a) / delta);
            result = act_mission->web_path->nodes[back_nodes[i - 1]].GetPos() + v;
            *index = i;
        }
    }
    return result;
}

// Turns the wagon's spring angle on and swings the two ends p1 and p2 by it: out is their middle, dir their
// direction, normalized
// FUNCTION: GAME 0x4f3110
void C_wagon::SpringProcess(uint time, int index, S_vector *out, S_vector *dir, S_vector p1, S_vector p2)
{
    if (drive_speed < MRG_ZERO)
        return;
    I3D_frame *frm = index == 0 ? frame : wagon_frames[index - 1];
    S_matrix m = frm->GetMatrix();
    m(3).z = 0.0f;
    m(3).y = 0.0f;
    m(3).x = 0.0f;
    float k = drive_speed / max_speed;
    wagon_angles[index] += (float)time / (1000.0f / (12.0f * k));
    if (wagon_angles[index] > PI * 2.0f)
        wagon_angles[index] -= PI * 2.0f;
    S_vector v(1.0f, 0.0f, 0.0f);
    v *= m;
    float s = (float)sin(wagon_angles[index] + PI / 2);
    p1 += v * s * 0.01f * k;
    p2 -= v * s * 0.01f * k;
    *dir = p1 - p2;
    *out = p2 + *dir * 0.5f;
    dir->Normalize();
}

// AI's sweep (C_railway::Collide's twin, an inline helper of its own: the vector operators inside it are calls the
// original leaves out of line, where AI's own code inlines them): a sphere goes ahead of the front along the move d,
// the front moves on by d, and what the sphere meets is hit: a car takes the impulse, a wagon in the way stops this
// one (or ends its path if it has finished), a pedestrian or anything else is pushed aside, the world raises the event
inline void C_wagon::Collide(S_vector &d)
{
    S_vector from = d;
    from.Normalize();
    from *= (wagon_length - vehicle_length) * 0.5f;
    from += front_pos;
    gifCollision.DynSetNextCollOwner(&coll, NULL);
    S_vector hitpos;
    S_vector normal;
    g_collision_header *hit = (g_collision_header *)gifCollision.TestSphereB(from, d, 1.0f, &hitpos, &normal,
        COLF_UNKNOWN_2, COLF_ALL);
    gifCollision.DynSetNextCollOwner(NULL, NULL);
    hitpos += normal * -1.5f;
    front_pos += d;
    if (hit && (hit->flags & COLLHF_UNKNOWN_40))
    {
        C_actor *owner = hit->owner;
        if (owner)
        {
            switch (owner->type)
            {
            case ACTOR_CAR:
            {
                normal.y = 0.0f;
                normal.Normalize();
                S_vector vel = d * drive_speed;
                ((C_car *)owner)->DoCollisionStatic(hitpos, normal, vel);
                break;
            }
            case ACTOR_WAGON:
                if (owner->act_state == 2)
                {
                    if (event_program)
                        event_program->SetEvent(event_id);
                    SetActState(2);
                }
                else
                {
                    front_pos += hitpos - from;
                    drive_speed = 0;
                    blocker = owner;
                }
                break;
            default:
            {
                I3D_frame *ofrm = owner->frame;
                if (!ofrm)
                    break;
                S_vector wp = ofrm->GetWorldPos();
                float side = S_vector2(wp.x, wp.z).DistanceToLine(S_vector2(front_pos.x, front_pos.z),
                    S_vector2(d.x, d.z));
                S_vector perp;
                if (side < 0.0f)
                    perp = S_vector(d.z, d.y, -d.x);
                else
                    perp = S_vector(-d.z, d.y, d.x);
                perp.Normalize();
                perp *= 5.0f;
                owner->Collision(this, hitpos, perp, normal, 40000.0f, frame);
                break;
            }
            }
        }
        else
        {
            if (event_program)
                event_program->SetEvent(event_id);
            SetActState(2);
        }
    }
}

// The wagon's brain: drives along its rails to the speed it wants (slowing for a stop), takes the next node off the
// path once it is past the one it is at, puts itself on the rails behind its front (as C_railway's Place) and
// sweeps a sphere along the way it moved, which what it meets is hit by (a car takes its impulse, a wagon in the way
// stops it, a pedestrian is pushed aside); at the path's end or the world in its way it raises its event
// FUNCTION: GAME 0x4f3520
void C_wagon::AI(uint time)
{
    // Stopped by a wagon in the way: free again once it moves on, at its path's end raises the event
    if (blocker)
    {
        if (blocker->act_state == 0)
        {
            blocker = NULL;
            return;
        }
        if (blocker->act_state == 2)
        {
            if (event_program)
                event_program->SetEvent(event_id);
            SetActState(2);
        }
        return;
    }
    // Waiting at a stop: 10 s, then it drives on
    if (stopped)
    {
        stop_time += time;
        if (stop_time > 10000)
        {
            stop_time = 0;
            stopped = false;
            drive_speed = 0;
            safe_speed = max_speed;
            if (sound)
            {
                sound->SetVolume(0.0f);
                sound->SetOn(true);
            }
        }
        for (int zi = 0; zi <= num_wagons; zi++)
        {
            pos_deltas[zi].Zero();
            dir_deltas[zi].Zero();
        }
        return;
    }
    // The wagon's front is on its path: past the node it was going to, take the next, then drive and put the wagons
    // behind it on the rails
    if (back_nodes.size())
    {
        S_vector d = act_mission->web_path->nodes[back_nodes[0]].GetPos() - front_pos;
        if (node_dir.IsNull())
            node_dir = d;
        if (d.AngleTo(node_dir) >= PI / 2 || d.Magnitude() < 0.1f)
        {
            uint *first = back_nodes.begin();
            if (IsStopPoint(*first))
                safe_speed = 0;
            C_web_node *node = &act_mission->web_path->nodes[*first];
            if (!forward_nodes.empty())
            {
                back_nodes.insert(first, forward_nodes.front());
                forward_nodes.erase(forward_nodes.begin());
            }
            else
            {
                for (ushort i = node->num_conns; i--;)
                {
                    if (node->GetConn(i)->flags & WEBF_ONEWAY)
                        continue;
                    if (back_nodes.size() >= 2 && first[back_nodes.size() - 2] == node->GetConn(i)->node)
                        continue;
                    back_nodes.insert(first, node->GetConn(i)->node);
                    break;
                }
            }
            while (back_nodes.size() > 2)
            {
                S_vector back = front_pos - act_mission->web_path->nodes[back_nodes.back()].GetPos();
                if (!(back.Magnitude() > back_dist))
                    break;
                S_vector back2 = front_pos - act_mission->web_path->nodes[back_nodes[back_nodes.size() - 2]].GetPos();
                if (!(back2.Magnitude() > back_dist))
                    break;
                back_nodes.pop_back();
            }
            d = act_mission->web_path->nodes[back_nodes[0]].GetPos() - front_pos;
            node_dir = d;
        }
        float brake_dist = drive_speed * drive_speed / (2.0f * acceleration);
        if (found_next)
            CreateForwardVector(back_nodes[0], brake_dist);
        d.Normalize();
        float step = time * drive_speed * 0.001f;
        d *= step;
        Collide(d);
        d = act_mission->web_path->nodes[back_nodes[0]].GetPos() - front_pos;
        float angle = d.AngleTo(node_dir);
        if (forward_nodes.empty())
        {
            if (angle >= PI / 2 || d.Magnitude() < 0.1f)
            {
                if (event_program)
                    event_program->SetEvent(event_id);
                SetActState(2);
            }
        }
        if (safe_speed > drive_speed)
        {
            drive_speed += time * acceleration * 0.001f;
            if (drive_speed > safe_speed)
                drive_speed = safe_speed;
        }
        else if (safe_speed < drive_speed)
        {
            drive_speed -= time * acceleration * 0.001f;
            if (drive_speed < safe_speed)
                drive_speed = safe_speed;
        }
        if (drive_speed == 0 && safe_speed == 0)
        {
            stop_time = 0;
            stopped = true;
            if (sound)
                sound->SetOn(false);
        }
        if (sound)
            sound->SetVolume(drive_speed / max_speed);
        float dist = MustStop();
        if (dist > -1.0f && dist <= brake_dist)
        {
            safe_speed = dist * acceleration * 2.0f;
            if (safe_speed >= 0.0f)
                safe_speed = (float)sqrt(safe_speed);
        }
        int index = 1;
        back_pos = FindBackPos(front_pos, vehicle_length, &index);
        *pos_deltas = pos;
        *dir_deltas = dir;
        SpringProcess(time, 0, &pos, &dir, front_pos, back_pos);
        *pos_deltas = pos - *pos_deltas;
        *dir_deltas = dir - *dir_deltas;
        S_vector back = back_pos;
        if (num_wagons)
        {
            for (int i = 0; i < num_wagons; i++)
            {
                S_vector a = FindBackPos(back, wagon_length - vehicle_length, &index);
                S_vector b = FindBackPos(a, vehicle_length, &index);
                pos_deltas[i + 1] = wagon_pos[i];
                dir_deltas[i + 1] = wagon_dirs[i];
                SpringProcess(time, i + 1, &wagon_pos[i], &wagon_dirs[i], a, b);
                pos_deltas[i + 1] = wagon_pos[i] - pos_deltas[i + 1];
                dir_deltas[i + 1] = wagon_dirs[i] - dir_deltas[i + 1];
                back = b;
            }
        }
    }
    else
    {
        // Not on a path yet: start at the nearest rail node
        int node = act_mission->web_path->GetNearNode(pos, -1.0f, 0xc);
        if (node != -1)
        {
            back_nodes.push_back(node);
            S_vector delta = act_mission->web_path->nodes[back_nodes[0]].GetPos() - front_pos;
            front_pos += delta;
            back_pos += delta;
        }
        CreateBackVector(front_pos);
    }
}

// Places the frame and its collision, counts the stop down and turns the frames of wheel_frames
// FUNCTION: GAME 0x4f48f0
void C_wagon::Update(uint time)
{
    if (stopped || blocker)
        return;
    frame->SetPos(pos);
    frame->SetDir(dir, 0.0f);
    act_mission->scene->SetFrameSector(frame);
    frame->Update();
    if (num_wagons)
    {
        for (int w = 0; w < num_wagons; w++)
        {
            if (wagon_frames[w])
            {
                wagon_frames[w]->SetPos(wagon_pos[w]);
                wagon_frames[w]->SetDir(wagon_dirs[w], 0.0f);
                act_mission->scene->SetFrameSector(wagon_frames[w]);
                wagon_frames[w]->Update();
            }
        }
    }
    coll_matrix = frame->GetMatrix();
    coll_matrix.Invert();
    I3D_bvolume bvol = frame->GetHRBoundVolume();
    ((I3D_bsphere &)coll.bsphere).pos = bvol.bsphere.pos * frame->GetMatrix();
    gifCollision.DynUpdate(&coll);
    if (deactivate_count > 0)
    {
        deactivate_count--;
        if (deactivate_count == 0)
        {
            forward_nodes.clear();
            SetActState(ACTSTATE_INACTIVE);
        }
    }
    S_quat q;
    float f = time * drive_speed;
    f = f * 0.001f;
    f = f * inv_wheel_radius;
    q.Make(S_vector(1.0f, 0.0f, 0.0f), -f);
    for (int i = wheel_frames.size(); i > 0;)
    {
        i--;
        wheel_frames[i]->SetRot(wheel_frames[i]->GetRot() * q);
        wheel_frames[i]->Update();
    }
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_wagon::NetDirect(uint time)
{
}

// Sets the wagon up for the mission: the arrays of the model's carriages (the wagon is the first of them), a sound that
// follows it, its collision and the wheels' frames, whose first one gives the size the wheels' speed is scaled by
// FUNCTION: GAME 0x4f4d70
void C_wagon::GameInit()
{
    int i;
    char buf[100];
    C_actor::GameInit();
    node_dir.Zero();
    wagon_angles = new float[num_wagons + 1];
    wagon_angles[0] = rnd() * PI;
    pos_deltas = new S_vector[num_wagons + 1];
    dir_deltas = new S_vector[num_wagons + 1];
    for (i = 0; i <= num_wagons; i++)
    {
        pos_deltas[i].Zero();
        dir_deltas[i].Zero();
    }
    C_actor::DebustizeFrame(frame);
    if (num_wagons)
    {
        wagon_frames = new I3D_frame *[num_wagons];
        wagon_pos = new S_vector[num_wagons];
        wagon_dirs = new S_vector[num_wagons];
        I3D_frame *vehicle = frame;
        const char *file = ((I3D_model *)vehicle)->GetFileName();
        const char *name = strrchr(file, '\\');
        if (!name)
            name = file;
        else
            name++;
        for (i = 0; i < num_wagons; i++)
        {
            wagon_angles[i + 1] = rnd() * PI;
            wagon_frames[i] = (I3D_frame *)::driver->CreateFrame(FRAME_MODEL);
            if (model_cache.Open((I3D_model *)wagon_frames[i], name, 0, NULL, NULL, NULL) == I3D_OK)
            {
                float off = (float)((i + 1) * 5);
                wagon_pos[i] = S_vector(pos.x, off + pos.y, pos.z);
                wagon_dirs[i] = dir;
                wagon_frames[i]->SetPos(pos);
                wagon_frames[i]->SetDir(dir, 0.0f);
                wagon_frames[i]->Update();
                sprintf(buf, "%s_vagon%i", vehicle->GetName(), i);
                wagon_frames[i]->SetName(buf);
                I3D_frame *wf = wagon_frames[i];
                I3D_scene *scene = act_mission->scene;
                scene->SetFrameSector(wf);
                C_actor::DebustizeFrame(wagon_frames[i]);
            }
            else
            {
                wagon_frames[i]->Release();
                wagon_frames[i] = NULL;
            }
        }
    }
    else
        wagon_frames = NULL;
    drive_speed = 0.0f;
    safe_speed = max_speed;
    stop_time = 0;
    stopped = false;
    back_dist = (float)(num_wagons + 1) * wagon_length + 30.0f;
    dir.Normalize();
    float half = vehicle_length * .5f;
    front_pos = pos + dir * half;
    back_pos = pos - dir * half;
    back_nodes.clear();
    forward_nodes.clear();
    found_next = true;
    sound = (I3D_sound *)::driver->CreateFrame(FRAME_SOUND);
    if (sound)
    {
        // STRING: GAME 0x634ce8
        if (sound_cache.Open(sound, "nadzemka.wav", 0, NULL, NULL, NULL) != I3D_OK)
        {
            sound->Release();
            sound = NULL;
        }
        else
        {
            sound->SetOn(true);
            sound->SetVolume(0.0f);
            sound->SetOutVol(0.0f);
            sound->SetLoop(true);
            sound->SetSoundType(I3DSOUND_POINT);
            sound->SetCone(2.0f * PI, 2.0f * PI);
            sound->SetRange(10.0f, 65.0f, 1.0f, 0.0f);
            sound->SetPos(S_vector(0.0f, 0.0f, 0.0f));
            sound->LinkTo(frame, 0);
            sound->Update();
        }
    }
    memset(&coll, 0, sizeof(coll));
    coll.type = DYNCOLL_UNKNOWN_40;
    coll.frame = frame;
    coll.owner = this;
    I3D_bvolume bvol;
    frame->GetHRBoundVolume(bvol);
    (I3D_bbox &)coll.bbox = bvol.bbox;
    S_vector ext = frame->GetWorldScale();
    float mx = Max(ext.x, Max(ext.y, ext.z));
    ((I3D_bsphere &)coll.bsphere).radius = mx * bvol.bsphere.radius;
    ((I3D_bsphere &)coll.bsphere).pos = bvol.bsphere.pos * frame->GetMatrix();
    coll.flags = DYNCOLLF_UNKNOWN_2000;
    coll.matrix = (S_matrix *)&frame->GetMatrix();
    coll_matrix = *coll.matrix;
    coll_matrix.Invert();
    coll.inv_matrix = &coll_matrix;
    event_program = NULL;
    blocker = NULL;
    gifCollision.DynInsert(&coll);
    deactivate_count = 2;
    // The frames named like a wheel
    struct S_hlp
    {
        // FUNCTION: GAME 0x4f5b30
        static void cbEnumFrame(I3D_frame *frm, std::vector<I3D_frame *> &frames)
        {
            const char *name = frm->GetName();
            const char *dot = strrchr(name, '.');
            if (dot)
                name = dot + 1;
            // STRING: GAME 0x634cf8
            if (strlen(name) >= 4 && strnicmp(name, "kolo", 4) == 0)
                frames.push_back(frm);
            for (int j = frm->NumChildren(); j--;)
                cbEnumFrame(frm->GetChild(j), frames);
        }
    };
    S_hlp::cbEnumFrame(frame, wheel_frames);
    if (wheel_frames.size())
    {
        I3D_bvolume wheel = wheel_frames[0]->GetHRBoundVolumeW();
        inv_wheel_radius = 1.0f / ((wheel.bbox.max.y - wheel.bbox.min.y) * .5f);
    }
}

// Undoes GameInit: the path emptied, the sound and the carriages' frames given back, the arrays freed
// FUNCTION: GAME 0x4f5bc0
void C_wagon::GameDone()
{
    int i;
    back_nodes.clear();
    forward_nodes.clear();
    if (sound)
    {
        sound->LinkTo(NULL, 0);
        sound->SetOn(false);
        sound->Release();
        sound = NULL;
    }
    if (wagon_frames)
    {
        for (i = 0; i < num_wagons; i++)
        {
            if (wagon_frames[i])
            {
                wagon_frames[i]->LinkTo(NULL, 0);
                wagon_frames[i]->Release();
            }
        }
        delete wagon_pos;
        delete wagon_dirs;
        delete wagon_angles;
        delete pos_deltas;
        delete dir_deltas;
        delete wagon_frames;
        wagon_frames = NULL;
    }
    C_actor::GameDone();
    gifCollision.DynErase(&coll);
}

// Recomputes the path ahead of every wagon, to the distance it brakes in
// FUNCTION: GAME 0x4f5d10
void C_wagon::RecompileNewPath()
{
    for (int i = C_actor::global_actors.size(); i--;)
    {
        C_wagon *wagon = (C_wagon *)C_actor::global_actors[i];
        if (wagon->type == ACTOR_WAGON)
        {
            wagon->forward_nodes.clear();
            wagon->CreateForwardVector(wagon->back_nodes[0],
                                       wagon->drive_speed * wagon->drive_speed / (2.0f * wagon->acceleration));
        }
    }
}
