// The physical things (PS2 C_PhThings.cpp; the PC's file is the one at 0x459e90-0x460a1c, docs/game-files.md): boxes
// and wheels the physics moves, which the public physics, C_box_new, C_crash_obj and C_drop_in_weapon derive from.

#include "actors/C_PhThings.h"

#include "mission/Mission.h"
#include "mission/game.h"
#include "g_material.h"
#include "tools.h"

// The defaults of a thing before InitBox or InitWheel: no type, the gravity's 10 and AI's step of .028 s
// FUNCTION: GAME 0x459e90
C_PhThing::C_PhThing()
{
    thing_type = 0;
    width = 0.0f;
    radius = 0.0f;
    gravity = 10.0f;
    max_step = .028f;
    velocity.Zero();
    rot_velocity.Zero();
    contact_normal = S_vector(0.0f, 1.0f, 0.0f);
    has_center_offset = false;
    hit_sound = -1;
    material = -1;
    center_offset.Zero();
    hit_sound_restart = true;
}

// FUNCTION: GAME 0x459f90
C_PhThing::~C_PhThing()
{
}

// Takes the frame's world matrix, its inverse, both without the translation, and the axes
// FUNCTION: GAME 0x459fd0
bool C_PhThing::Init(I3D_frame *frm)
{
    if (!C_actor::Init(frm))
        return false;
    pos = frame->GetWorldPos();
    matrix = frame->GetMatrix();
    inv_matrix = matrix;
    inv_matrix.Invert();
    rot_matrix = matrix;
    rot_matrix(3).Zero();
    inv_rot_matrix = inv_matrix;
    inv_rot_matrix(3).Zero();
    dir_x = matrix(0);
    dir_y = matrix(1);
    dir = matrix(2);
    DebustizeFrame(frm);
    return true;
}

// The thing put into the game at rest: its matrices, its collision (a sphere around the frame's box, put into the
// moving objects' grid), the rest counters cleared, the hit sound's frame, then inactive where it doesn't move
// FUNCTION: GAME 0x45a120
void C_PhThing::GameInit()
{
    C_actor::GameInit();
    matrix = frame->GetMatrix();
    inv_matrix = matrix;
    inv_matrix.Invert();
    rot_matrix = matrix;
    rot_matrix(3).Zero();
    inv_rot_matrix = inv_matrix;
    inv_rot_matrix(3).Zero();
    dir_x = matrix(0);
    dir_y = matrix(1);
    dir = matrix(2);
    pos = frame->GetMatrix()(3);
    I3D_bbox bbox = frame->GetHRBoundVolume().bbox;
    float sq = bbox.max.Square() > bbox.min.Square() ? bbox.max.Square() : bbox.min.Square();
    float sphere_radius;
    if (IsMrgZeroLess(sq))
        sphere_radius = 0.0f;
    else if (fabs(sq - 1.0f) < MRG_ZERO)
        sphere_radius = 1.0f;
    else
        sphere_radius = (float)sqrt(sq);
    memset(&coll, 0, sizeof(coll));
    coll.type = DYNCOLL_UNKNOWN_40;
    ((I3D_bsphere &)coll.bsphere).pos = pos;
    ((I3D_bsphere &)coll.bsphere).radius = sphere_radius;
    ((I3D_bbox &)coll.bbox).min = bbox.min;
    ((I3D_bbox &)coll.bbox).max = bbox.max;
    coll.frame = frame;
    coll.matrix = &matrix;
    coll.inv_matrix = &inv_matrix;
    coll.owner = this;
    coll.flags = DYNCOLLF_UNKNOWN_800;
    gifCollision.DynInsert(&coll);
    gifCollision.DynUpdate(&coll);
    if (has_center_offset)
        center = center_offset * matrix;
    else
        center = pos;
    spring_force = 0.0f;
    rot_velocity.x = 0.0f;
    rot_velocity.y = 0.0f;
    rot_velocity.z = 0.0f;
    rest_time = 0.0f;
    contact_pos.x = 0.0f;
    contact_pos.y = 0.0f;
    contact_pos.z = 0.0f;
    rest_limit = .6f;
    still_time = 0;
    rest_pos = pos;
    sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
    if (sound)
    {
        sound->LinkTo(act_mission->scene->primary_sector, 0);
        sound->SetPos(S_vector(0.0f, 0.0f, 0.0f));
    }
    memset(contact_materials, 0, sizeof(contact_materials));
    hit_index = 0;
    hit_impulse = 0.0f;
    hit_sound_time = 0;
    time_scale = 1.0f;
    wake_resets_rest = true;
    if (velocity.Square() < .01f)
        SetActState(ACTSTATE_INACTIVE);
}

// A thing at rest runs a step to find the actors it touches: with any it stays awake, else it goes inactive
// FUNCTION: GAME 0x45a560
void C_PhThing::FindActors()
{
    if (velocity.Square() < .01f)
    {
        if (!net_direct)
        {
            rest_time = 0.0f;
            still_time = 0;
            AI(1);
            Update(1);
        }
        if (surr_acts.size())
        {
            rest_time = rest_limit + 1.0f;
            still_time = 4001;
            wake_resets_rest = false;
            SetActState(ACTSTATE_ACTIVE);
        }
        else
            SetActState(ACTSTATE_INACTIVE);
    }
}

// Releases the sound, takes the collision out and forgets the actors
// FUNCTION: GAME 0x45a630
void C_PhThing::GameDone()
{
    C_actor::GameDone();
    if (sound)
        sound->Release();
    gifCollision.DynErase(&coll);
    surr_acts.clear();
}

// Steps the physics by the tick (at most 83 ms, time_scale the rest) in steps of at most max_step, unless the
// thing has rested long enough
// FUNCTION: GAME 0x45a690
void C_PhThing::AI(uint time)
{
    if (rest_time > rest_limit)
        return;
    if (still_time > 4000)
        return;
    time_scale = 1.0f;
    if (time > 83)
    {
        time_scale = 83.0f / time;
        time = 83;
    }
    else if (time < 1)
        return;
    float t = time * .001f;
    if (t > max_step)
    {
        int n = (int)(t / max_step) + 1;
        t /= n;
        for (int i = 0; i < n; i++)
        {
            switch (thing_type)
            {
            case PHTHING_WHEEL:
                MoveWheel(t);
                break;
            default:
                MoveBox(t);
            }
        }
    }
    else
    {
        switch (thing_type)
        {
        case PHTHING_WHEEL:
            MoveWheel(t);
            break;
        default:
            MoveBox(t);
        }
    }
}

// The frame's update after the physics: the rest counters and the actors touched (their times run out, or they have
// moved and the rest starts again), the frame placed in its sector, the collision updated, then the hit sound
// FUNCTION: GAME 0x45a7a0
void C_PhThing::Update(uint time)
{
    if (time > 83)
        time = 83;
    else if (time < 1)
        return;
    float dt = time * .001f;
    if (!net_direct)
    {
        if (velocity.Magnitude() < .3f && rot_velocity.Magnitude() < .1f)
            rest_time += dt;
        else
            rest_time = 0.0f;
        if ((pos - rest_pos).Square() < .04f)
            still_time += time;
        else
        {
            still_time = 0;
            rest_pos = pos;
        }
        if (!(rest_time > rest_limit) && still_time <= 4000)
        {
            if (surr_acts.size())
            {
                for (int i = surr_acts.size(); i--;)
                {
                    surr_acts[i].time -= time;
                    if (surr_acts[i].time < 0)
                        surr_acts.erase(surr_acts.begin() + i);
                }
            }
        }
        else if (surr_acts.size())
        {
            for (int i = surr_acts.size(); i--;)
            {
                S_vector d = surr_acts[i].pos - surr_acts[i].act->GetWorldPos();
                if (d.Square() > 1.6e-5f)
                {
                    surr_acts[i].pos = surr_acts[i].act->GetWorldPos();
                    rest_time = 0.0f;
                    still_time = 0;
                    break;
                }
            }
        }
        else
            SetActState(ACTSTATE_INACTIVE);
        if (velocity.y < -85.0f)
            SetActState(ACTSTATE_NONE);
    }
    frame->Update();
    act_mission->scene->SetFrameSector(frame);
    UpdateImportantVariables();
    ((I3D_bsphere &)coll.bsphere).pos = pos;
    gifCollision.DynUpdate(&coll);
    if (sound && hit_sound >= 0)
    {
        hit_sound_time += time;
        bool playing = sound->IsPlaying(false);
        float lo = weight * .4f;
        float hi = weight * 5.6f;
        if (!(playing && hit_sound_time <= 150) && hit_impulse > lo && contact_materials[hit_index] > 0 &&
            contact_materials[hit_index] < numOfMaterials && (hit_sound_restart || !sound->IsPlaying(false)))
        {
            sound->SetOn(false);
            if (sound_table.ConstructSound(sound,
                (&materialList[contact_materials[hit_index]].grenade_sound)[hit_sound]))
            {
                sound->SetOn(true);
                float volume;
                if (hit_impulse > hi)
                    volume = 1.0f;
                else
                    volume = (hit_impulse - lo) / (hi - lo) * .8f + .19f;
                sound->SetVolume(volume);
                sound->SetPos(hit_point);
            }
            hit_sound_time = 0;
        }
    }
    hit_impulse = 0.0f;
    hit_index = 0;
}

// An active thing's rest starts again where it was woken up before
// FUNCTION: GAME 0x45adc0
void C_PhThing::ChangeState()
{
    if (act_state == ACTSTATE_ACTIVE)
    {
        if (wake_resets_rest)
        {
            rest_time = 0.0f;
            still_time = 0;
        }
        wake_resets_rest = true;
    }
}

// A step of a wheel's physics: the weight, the tyre's spring and friction on the ground it found last step move
// the velocities, then MoveCount moves the wheel and the ground under it is tested again (a line down, one along the
// ground's normal where that is steep, one along the move), which sets the spring for the next step or, where the
// wheel touches the ground moving into it, calls CollisionCount.
// FUNCTION: GAME 0x45ade0
void C_PhThing::MoveWheel(float time)
{
    S_vector torque, force;
    torque.Zero();
    force.Zero();
    float speed = velocity.Magnitude();
    if (spring_force > 0.0f)
    {
        // The rolling friction, along the velocity's opposite
        S_vector drag;
        drag = -velocity;
        drag.Normalize();
        if (speed < 1.0f)
            drag *= speed;
        drag *= rolling_friction / radius * (weight * gravity);
        force = drag;
        torque = drag.Cross((pos + contact_normal * -radius) - pos);
        // The sliding friction of the tyre's contact point
        S_vector v = velocity - rot_velocity.Cross(contact_normal * -radius);
        S_vector n = v;
        n.Normalize();
        S_vector side = n * (-friction * weight * gravity);
        float s = (float)sin(v.AngleTo(S_vector(0.0f, 1.0f, 0.0f)));
        if (s < 1.0f)
            side *= s;
        if (v.Magnitude() < 1.0f)
            side *= v.Magnitude();
        force += side;
        torque += side.Cross(contact_normal * -radius);
        // The spring
        S_vector spring = contact_normal * spring_force;
        force += spring;
        torque += spring.Cross(contact_pos - pos);
    }
    velocity += (force + gravity_force) * (time / weight);
    S_vector step = velocity * time;
    S_vector w = torque;
    w *= inv_rot_matrix;
    w *= inv_inertia;
    w *= rot_matrix;
    w *= time;
    rot_velocity += w;
    S_vector old_pos = pos;
    S_vector old_dir = dir;
    S_vector old_dir_y = dir_y;
    S_vector old_dir_x = dir_x;
    MoveCount(time, step);
    S_matrix wm = frame->GetMatrix();
    S_matrix inv = wm;
    inv.Inverse(S_matrix(wm));
    spring_force = 0.0f;
    contact_dist = contact_reach;
    float len = step.Magnitude();
    len = (float)cos(step.AngleTo(S_vector(0.0f, -1.0f, 0.0f))) * len;
    float reach = radius + radius + contact_reach + len;
    S_vector down = S_vector(0.0f, -1.0f, 0.0f) * reach;
    S_vector hitpos;
    S_vector xpt, xdir, r, t1, t2, nrm;
    S_plane wheel_plane, ground_plane;
    bool flag;
    S_vector from = pos;
    gifCollision.DynSetNextCollOwner(&coll, NULL);
    int hit = gifCollision.TestLineV(from, down, &hitpos, &contact_normal, 2, 0);
    gifCollision.DynSetNextCollOwner(NULL, NULL);
    if (hit)
    {
        g_collision_header *col = CollisionFilter((uint *)hit, hitpos, contact_normal, flag);
        if (col)
        {
            contact_materials[0] = col->material;
            if (contact_normal.AngleTo(S_vector(0.0f, 1.0f, 0.0f)) > 0.35f)
            {
                S_vector back = -contact_normal;
                len = step.Magnitude();
                len = (float)cos(step.AngleTo(back)) * len;
                reach = contact_reach + radius + len;
                S_vector down2 = back * reach;
                gifCollision.DynSetNextCollOwner(&coll, NULL);
                hit = gifCollision.TestLineV(from, down2, &hitpos, &contact_normal, 2, 0);
                gifCollision.DynSetNextCollOwner(NULL, NULL);
                if (hit)
                {
                    if (CollisionFilter((uint *)hit, hitpos, contact_normal, flag))
                        contact_dist = (hitpos - old_pos).Magnitude() - (len + radius);
                }
            }
            else
            {
                contact_dist = (hitpos - old_pos).Magnitude() - (len + radius);
            }
        }
    }
    if (contact_dist < contact_reach)
    {
        wheel_plane = S_plane(1.0f, 0.0f, 0.0f, 0.0f) * wm;
        ground_plane = S_plane(contact_normal, -hitpos.Dot(contact_normal));
        if (wheel_plane.IntersectionLine(ground_plane, xpt, xdir))
        {
            r = pos - xpt;
            t1 = xdir.Cross(r);
            t2 = t1.Cross(xdir);
            nrm = -t2;
            nrm.Normalize();
            nrm *= radius;
            nrm += pos;
            S_vector axis = dir_x;
            float half = width * 0.5f;
            if (contact_normal.Dot(dir_x) > 0.0f)
                half = -half;
            axis *= half;
            axis += nrm;
            contact_pos = axis;
        }
        else
        {
            S_vector axis = dir_x;
            float half = width * 0.5f;
            if (contact_normal.Dot(dir_x) > 0.0f)
                half = -half;
            axis *= half;
            axis += pos;
            contact_pos = axis;
        }
        S_vector off = contact_normal * -contact_reach;
        S_vector c2 = contact_pos + off;
        contact_dist = c2.DistanceToPlane(ground_plane);
        if (contact_dist <= 0.0f)
            contact_dist = 0.0f;
        spring_force = (contact_reach - contact_dist) * stiffness;
    }
    if (contact_dist < 0.0001f && contact_normal.Dot(velocity) < 0.0f)
    {
        contact_pos *= inv;
        contact_pos *= matrix;
        dir = old_dir;
        dir_y = old_dir_y;
        dir_x = old_dir_x;
        pos = old_pos;
        frame->SetPos(pos);
        frame->SetDir1(dir, dir_y);
        CollisionCount(velocity, rot_velocity, contact_normal, contact_pos, time, 0, NULL);
        MoveCount(time, velocity * time);
    }
    else
    {
        S_vector p3;
        S_vector ahead = step;
        float l2 = ahead.Magnitude();
        ahead.Normalize();
        ahead *= l2 + radius;
        gifCollision.DynSetNextCollOwner(&coll, NULL);
        hit = gifCollision.TestLineV(from, ahead, &hitpos, &contact_normal, 2, 0);
        gifCollision.DynSetNextCollOwner(NULL, NULL);
        if (hit)
        {
            g_collision_header *col = CollisionFilter((uint *)hit, hitpos, contact_normal, flag);
            if (col)
            {
                contact_materials[0] = col->material;
                wheel_plane = S_plane(1.0f, 0.0f, 0.0f, 0.0f) * wm;
                ground_plane = S_plane(contact_normal, -hitpos.Dot(contact_normal));
                if (wheel_plane.IntersectionLine(ground_plane, xpt, xdir))
                {
                    r = pos - xpt;
                    t1 = xdir.Cross(r);
                    t2 = t1.Cross(xdir);
                    nrm = -t2;
                    nrm.Normalize();
                    nrm *= radius;
                    nrm += pos;
                    S_vector axis = dir_x;
                    float half = width * 0.5f;
                    if (contact_normal.Dot(dir_x) > 0.0f)
                        half = -half;
                    axis *= half;
                    axis += nrm;
                    p3 = axis;
                }
                else
                {
                    S_vector axis = dir_x;
                    float half = width * 0.5f;
                    if (contact_normal.Dot(dir_x) > 0.0f)
                        half = -half;
                    axis *= half;
                    axis += pos;
                    p3 = axis;
                }
                S_vector off = contact_normal * -contact_reach;
                S_vector c2 = p3 + off;
                contact_dist = c2.DistanceToPlane(ground_plane);
            }
        }
        if (contact_dist < 0.0001f && contact_normal.Dot(velocity) < 0.0f)
        {
            contact_pos *= inv;
            contact_pos *= matrix;
            p3 *= inv;
            p3 *= matrix;
            dir = old_dir;
            dir_y = old_dir_y;
            dir_x = old_dir_x;
            pos = old_pos;
            frame->SetPos(pos);
            frame->SetDir1(dir, dir_y);
            CollisionCount(velocity, rot_velocity, contact_normal, p3, time, 0, NULL);
            MoveCount(time, velocity * time);
        }
    }
    matrix = frame->GetMatrix();
    inv_matrix = matrix;
    inv_matrix.Invert();
    rot_matrix = matrix;
    rot_matrix(3).Zero();
    inv_rot_matrix = inv_matrix;
    inv_rot_matrix(3).Zero();
    center = pos;
}

// A step of a box's physics: the weight's force into the velocity, the move, then the box's points against the level and
// the actors, where they went in the thing is moved out and each point's impulse is counted
// FUNCTION: GAME 0x45d210
void C_PhThing::MoveBox(float time)
{
    float t = time / weight;
    test_all_points = !test_all_points;
    velocity += gravity_force * t;

    S_vector old_pos = pos;
    S_vector old_dir = dir;
    S_vector old_dir_y = dir_y;
    S_vector old_dir_x = dir_x;
    S_vector old_center = center;
    S_vector move = velocity * time;
    MoveCount(time, move);

    S_matrix wm = frame->GetMatrix();
    if (has_center_offset)
        center = center_offset * wm;
    else
        center = pos;

    S_vector push(0.0f, 0.0f, 0.0f);
    C_actor *hit_acts[PHTHING_BOX_POINTS];
    memset(hit_acts, 0, sizeof(hit_acts));
    S_vector world[PHTHING_BOX_POINTS];
    S_vector normals[PHTHING_BOX_POINTS];
    S_vector hit_pos[PHTHING_BOX_POINTS];
    if (velocity.Square() < 1.0f)
        test_all_points = true;

    contact_mask = 0;
    bool found = false;
    int i;
    for (i = PHTHING_BOX_POINTS; i--;)
    {
        world[i] = box_points[i] * wm;
        S_vector v;
        v = world[i] - old_center;
        if (!(v.Dot(velocity) > 0.0f || test_all_points))
            continue;
        S_vector n = v;
        n.Normalize();
        v += n * 0.3f;
        gifCollision.DynSetNextCollOwner(&coll, 0);
        int hit = gifCollision.TestLineV(old_center, v, &hit_pos[i], &contact_normal, 2, 0);
        gifCollision.DynSetNextCollOwner(0, 0);
        if (!hit)
            continue;
        bool b;
        g_collision_header *col = CollisionFilter((uint *)hit, hit_pos[i], contact_normal, b);
        if (!col)
            continue;
        if (col->flags & COLLHF_UNKNOWN_40)
            hit_acts[i] = col->owner;
        if (b)
            hit_acts[i] = NULL;
        contact_materials[i] = col->material;
        contact_dist = world[i].Dot(contact_normal) - hit_pos[i].Dot(contact_normal);
        if (contact_dist <= 0.0f)
        {
            normals[i] = contact_normal;
            found = true;
            contact_mask |= 1 << i;
        }
    }

    if (found)
    {
        if (velocity.Square() > 0.6f)
            found = false;
        float count = 0.0f;
        for (i = PHTHING_BOX_POINTS; i--;)
        {
            if (!(contact_mask & (1 << i)))
                continue;
            S_vector r = world[i] - center;
            S_vector rel = velocity - rot_velocity.Cross(r);
            if (rel.Dot(normals[i]) < 0.0f)
            {
                if (!found)
                {
                    pos = old_pos;
                    dir_y = old_dir_y;
                    dir = old_dir;
                    dir_x = old_dir_x;
                    found = true;
                }
                CollisionCount(velocity, rot_velocity, normals[i], world[i], time, i, hit_acts[i]);
                count += 1.0f;
            }
            C_actor *a = hit_acts[i];
            if (a)
            {
                int j = surr_acts.size();
                while (j--)
                {
                    if (surr_acts[j].act == a)
                    {
                        surr_acts[j].time = 1000;
                        break;
                    }
                }
                if (j < 0)
                {
                    S_SURR_ACT s;
                    s.act = a;
                    s.pos = a->GetWorldPos();
                    s.time = 1000;
                    surr_acts.push_back(s);
                }
            }
        }
        if (count > 0.5f)
            MoveCount(time, velocity * time);
        wm = frame->GetMatrix();
        for (i = PHTHING_BOX_POINTS; i--;)
        {
            if (!(contact_mask & (1 << i)))
                continue;
            world[i] = box_points[i] * wm;
            contact_dist = normals[i].Dot(world[i]) - normals[i].Dot(hit_pos[i]);
            if (contact_dist <= 0.0f)
            {
                float d = -contact_dist;
                S_vector p = normals[i] * d;
                if (!hit_acts[i] && p.Square() > push.Square())
                    push = p;
            }
        }
    }
    pos += push;
    frame->SetPos(pos);
    frame->Update();
    matrix = frame->GetMatrix();
    inv_matrix = matrix;
    inv_matrix.Invert();
    rot_matrix = matrix;
    rot_matrix(3).Zero();
    inv_rot_matrix = inv_matrix;
    inv_rot_matrix(3).Zero();
    center = has_center_offset ? center_offset * matrix : pos;
}

// Turns the thing by its rotation velocity over the time, about its own axes one after the other, and moves it by move:
// its position is turned about the centre too where it has one, and the frame follows
// FUNCTION: GAME 0x45ddb0
void C_PhThing::MoveCount(float time, const S_vector &move)
{
    S_vector w = rot_velocity * time;
    S_matrix m = inv_matrix;
    m(3, 0) = 0.0f;
    m(3, 1) = 0.0f;
    m(3, 2) = 0.0f;
    w *= m;
    S_quat q;
    q.Make(dir_x, w.x);
    S_quat acc = q;
    q.Make(dir_y, w.y);
    acc *= q;
    q.Make(dir, w.z);
    acc *= q;
    S_matrix rm = acc.RotationMatrix();
    dir *= rm;
    dir_y *= rm;
    if (has_center_offset)
    {
        S_matrix m1, m2;
        m1.Identity();
        m2.Identity();
        m1(3, 0) = -center.x;
        m1(3, 1) = -center.y;
        m1(3, 2) = -center.z;
        m2(3) = center;
        rm = m1 * rm;
        rm = rm * m2;
        pos *= rm;
    }
    S_vector t, u;
    t = dir_y.Cross(dir);
    dir_x = t;
    u = dir.Cross(dir_x);
    dir_y = u;
    dir_x.Normalize();
    dir_y.Normalize();
    dir.Normalize();
    pos += move;
    frame->SetPos(pos);
    frame->SetDir1(dir, dir_y);
}

// The response to a contact at point with the normal: where the contact point moves against the normal, the impulse
// along it (into vel the linear and into rot_vel the rotational velocity change it makes) and the friction impulse
// against the sliding part of the contact's velocity, limited by the weight; the strongest impulse (its size, its
// point and index) is remembered for the hit sound
// FUNCTION: GAME 0x45e510
void C_PhThing::CollisionCount(S_vector &vel, S_vector &rot_vel, S_vector &normal, S_vector &point, float time,
    ushort index, C_actor *act)
{
    S_vector imp, tang;
    S_vector r = point - center;
    S_vector rel = velocity - rot_velocity.Cross(r);
    S_vector a;
    a = r.Cross(normal);
    a *= inv_rot_matrix;
    a *= inv_inertia;
    a *= rot_matrix;
    float vn = rel.Dot(normal);
    if (vn < 0.0f)
    {
        float e = (-bounce) * vn;
        float d = a.Cross(r).Dot(normal) + 1.0f / weight;
        if (fabs(d) < 1e-6f)
            d = 1e-6f;
        float j = e / d;
        imp = normal * j;
        vel += imp * (1.0f / weight);
        a = r.Cross(imp);
        a *= inv_rot_matrix;
        a *= inv_inertia;
        a *= rot_matrix;
        rot_vel -= a;
        float mag = imp.Magnitude();
        if (mag > hit_impulse)
        {
            hit_impulse = mag;
            hit_index = index;
            hit_point = point;
        }
        tang = rel;
        tang.Normalize();
        tang = tang.Cross(normal).Cross(normal);
        tang.Normalize();
        float fr = mag * friction * sin(rel.AngleTo(normal));
        float cap = gravity * friction * weight * 3.0f;
        if (fr > cap)
            fr = cap;
        tang *= fr;
        vel += tang * (1.0f / weight);
        a = tang.Cross(r);
        a *= inv_rot_matrix;
        a *= inv_inertia;
        a *= rot_matrix;
        rot_vel += a;
    }
}

// A wheel: a disc's inertia, the weight's force, inactive where it doesn't move
// FUNCTION: GAME 0x45ed30
void C_PhThing::InitWheel(S_THING_WHEEL *wheel)
{
    weight = wheel->weight;
    radius = wheel->radius;
    width = wheel->width;
    inv_inertia.z = inv_inertia.y = inv_inertia.x = 1.0f / ((radius * radius) * weight * .5f);
    velocity = wheel->velocity;
    if (velocity.Square() < 1e-5f)
        SetActState(ACTSTATE_INACTIVE);
    friction = wheel->friction;
    gravity_force = S_vector(0.0f, -1.0f, 0.0f) * weight * gravity;
    rolling_friction = wheel->rolling_friction;
    contact_reach = .03f;
    bounce = 1.05f;
    thing_type = PHTHING_WHEEL;
    stiffness = gravity * weight * 37.03704f;
}

// The thing set up as a box from the struct and the frame's bound box: the box's centre, its inertia (one value
// for a roughly cubic box, else from its longest edge), its eight corners and six face centres, the gravity's
// force and the constants of the collision response. A thing without velocity starts inactive.
// FUNCTION: GAME 0x45ee90
void C_PhThing::InitBox(S_THING_BOX *box)
{
    weight = box->weight;
    friction = box->friction;
    velocity = box->velocity;
    if (velocity.Square() < .00001f)
        SetActState(ACTSTATE_INACTIVE);
    I3D_bbox bb = frame->GetHRBoundVolume().bbox;
    float sx = bb.max.x - bb.min.x;
    float sy = bb.max.y - bb.min.y;
    float sz = bb.max.z - bb.min.z;
    has_center_offset = true;
    center_offset = S_vector(sx * .5f + bb.min.x, sy * .5f + bb.min.y, sz * .5f + bb.min.z);
    float a = sx + .05f;
    float b = sy + .05f;
    float c = sz + .05f;
    if (!(a / b > 2.0f || a / b < .5f || a / c > 2.0f || a / c < .5f || c / b > 2.0f || c / b < .5f))
    {
        float avg = (c + b + a) * (1.0f / 3.0f);
        inv_inertia.z = inv_inertia.y = inv_inertia.x = 1.0f / (avg * avg * weight * (1.0f / 3.0f));
    }
    else if (a > b)
    {
        if (a > c)
        {
            float aa = a * a;
            float k = 1.0f / (aa * weight * (1.0f / 3.0f));
            inv_inertia.z = inv_inertia.y = k;
            inv_inertia.x = k * (aa / (b > c ? b * b : c * c));
        }
        else
            goto longest_c;   // the same block as the b <= c arm
    }
    else if (b > c)
    {
        float bb2 = b * b;
        float k = 1.0f / (bb2 * weight * (1.0f / 3.0f));
        inv_inertia.z = inv_inertia.x = k;
        inv_inertia.y = k * (bb2 / (a > c ? a * a : c * c));
    }
    else
    {
    longest_c:
        float cc = c * c;
        float k = 1.0f / (cc * weight * (1.0f / 3.0f));
        inv_inertia.x = inv_inertia.y = k;
        inv_inertia.z = k * (cc / (b > a ? b * b : a * a));
    }
    box_points[0] = bb.min;
    box_points[1] = S_vector(bb.min.x, bb.min.y, bb.max.z);
    box_points[2] = S_vector(bb.max.x, bb.min.y, bb.max.z);
    box_points[3] = S_vector(bb.max.x, bb.min.y, bb.min.z);
    box_points[4] = bb.max;
    box_points[5] = S_vector(bb.min.x, bb.max.y, bb.min.z);
    box_points[6] = S_vector(bb.min.x, bb.max.y, bb.max.z);
    box_points[7] = S_vector(bb.max.x, bb.max.y, bb.min.z);
    box_points[8] = S_vector((bb.max.x - bb.min.x) * .5f + bb.min.x, (bb.max.y - bb.min.y) * .5f + bb.min.y, bb.min.z);
    box_points[9] = S_vector((bb.max.x - bb.min.x) * .5f + bb.min.x, (bb.max.y - bb.min.y) * .5f + bb.min.y, bb.max.z);
    box_points[10] = S_vector((bb.max.x - bb.min.x) * .5f + bb.min.x, bb.min.y, (bb.max.z - bb.min.z) * .5f + bb.min.z);
    box_points[11] = S_vector((bb.max.x - bb.min.x) * .5f + bb.min.x, bb.max.y, (bb.max.z - bb.min.z) * .5f + bb.min.z);
    box_points[12] = S_vector(bb.min.x, (bb.max.y - bb.min.y) * .5f + bb.min.y, (bb.max.z - bb.min.z) * .5f + bb.min.z);
    box_points[13] = S_vector(bb.max.x, (bb.max.y - bb.min.y) * .5f + bb.min.y, (bb.max.z - bb.min.z) * .5f + bb.min.z);
    gravity_force = S_vector(0.0f, -1.0f, 0.0f) * weight * gravity;
    bounce = 1.01f;
    contact_reach = .04f;
    thing_type = PHTHING_BOX;
    stiffness = gravity * weight * 12.5f;
    unknown_258 = weight * 4.0f;
}

// Another actor ran into the thing at pos with the velocity: wakes it up, pushes it (a tenth over the velocity) and
// remembers the hardest such hit's material and position, for the hit's sound
// FUNCTION: GAME 0x45f520
bool C_PhThing::Collision(C_actor *act, const S_vector &point, const S_vector &vel, const S_vector &v3, float f,
    I3D_frame *frm)
{
    SetActState(ACTSTATE_ACTIVE);
    still_time = 0;
    rest_time = 0.0f;
    S_vector rel = point - center;
    S_vector impuls = vel * 1.1f;
    float inv_weight = 1.0f / weight;
    velocity += impuls * inv_weight;
    S_vector torque = rel.Cross(impuls);
    torque *= inv_rot_matrix;
    torque *= inv_inertia;
    torque *= rot_matrix;
    rot_velocity -= torque;
    float speed = vel.Magnitude();
    if (material >= 0 && speed > hit_impulse)
    {
        hit_index = 0;
        hit_impulse = speed;
        contact_materials[0] = material;
        hit_point = point;
    }
    return true;
}

// A hit wakes the thing up and pushes it: a shot or a stroke along dir at the position, an explosion away from the
// position, from a point of the thing moved along its axes by the side the push is on, so it spins
// FUNCTION: GAME 0x45f760
bool C_PhThing::Hit(int type, const S_vector &along, const S_vector &point, const S_vector &v3, float f, C_actor *act,
    uint u, I3D_frame *frm)
{
    SetActState(ACTSTATE_ACTIVE);
    still_time = 0;
    rest_time = 0.0f;
    switch (type)
    {
    case HIT_SHOT:
    {
        S_vector impuls = along;
        impuls.Normalize();
        impuls *= f * .5f;
        ExternalImpuls(impuls, point);
        break;
    }
    case HIT_EXPLOSION:
    {
        S_vector rel = center - point;
        S_vector impuls = rel;
        impuls.Normalize();
        impuls *= f;
        S_vector p = center;
        if (dir_y.Dot(impuls) > 0.0f)
            p += dir_y * .025f;
        else
            p -= dir_y * .025f;
        if (along.Dot(impuls) > 0.0f)
            p -= along * .06f;
        else
            p += along * .06f;
        if (dir_x.Dot(impuls) > 0.0f)
            p -= dir_x * .04f;
        else
            p += dir_x * .04f;
        ExternalImpuls(impuls, p);
        return true;
    }
    case HIT_STROKE:
    {
        S_vector impuls = along;
        impuls.Normalize();
        impuls *= f * 2.0f;
        S_vector p = point;
        ExternalImpuls(impuls, p);
        break;
    }
    }
    return true;
}

// The relative position of the point, its velocity, the weight and the point's impulse response (the inverse
// inertia applied to the point's position crossed with the direction, in the world's space).
// FUNCTION: GAME 0x45fe90
void C_PhThing::GetRVelMassJ(const S_vector &point, const S_vector &axis, S_vector *rel, S_vector *point_vel,
    float *mass, S_vector *rot_response)
{
    *rel = point - center;
    *point_vel = velocity - rot_velocity.Cross(*rel);
    *mass = weight;
    *rot_response = rel->Cross(axis);
    *rot_response *= inv_rot_matrix;
    *rot_response *= inv_inertia;
    *rot_response *= rot_matrix;
}

// An impulse at a world position: the velocity and the rotation take it, and the thing wakes up.
// FUNCTION: GAME 0x460000
void C_PhThing::ExternalImpuls(const S_vector &impuls, const S_vector &point)
{
    SetActState(ACTSTATE_ACTIVE);
    still_time = 0;
    rest_time = 0.0f;
    S_vector r = point - center;
    velocity += impuls * (1.0f / weight);
    S_vector t = r.Cross(impuls);
    t *= inv_rot_matrix;
    t *= inv_inertia;
    t *= rot_matrix;
    rot_velocity -= t;
}

// FUNCTION: GAME 0x460140
void C_PhThing::InvalidateActor(C_actor *act)
{
    for (int i = surr_acts.size(); i--;)
    {
        if (surr_acts[i].act == act)
        {
            surr_acts[i] = surr_acts.back();
            surr_acts.pop_back();
        }
    }
}

// FUNCTION: GAME 0x4601f0
g_collision_header *C_PhThing::CollisionFilter(uint *obj, S_vector &point, S_vector &normal, bool &car)
{
    g_collision_header *coll = (g_collision_header *)obj;
    if (coll->flags & COLLHF_UNKNOWN_40 && coll->owner)
        car = coll->owner->GetType() == ACTOR_CAR;
    return coll->material == MAT_WATER_DEATH ? NULL : coll;
}

// The matrices, the centre and the directions taken again from the frame's world matrix
// FUNCTION: GAME 0x460220
void C_PhThing::UpdateImportantVariables()
{
    matrix = frame->GetMatrix();
    inv_matrix = matrix;
    inv_matrix.Invert();
    rot_matrix = matrix;
    rot_matrix(3).Zero();
    inv_rot_matrix = inv_matrix;
    inv_rot_matrix(3).Zero();
    if (has_center_offset)
        center = center_offset * matrix;
    else
        center = frame->GetMatrix()(3);
    pos = frame->GetMatrix()(3);
    dir_x = matrix(0);
    dir_y = matrix(1);
    dir = matrix(2);
}

// FUNCTION: GAME 0x4603b0
int C_PhThing::SaveGameGetSize()
{
    return C_actor::SaveGameGetSize() + 0xd3;
}

// Saves the actor, then the thing's frame, box or wheel and velocities (S_phthing_savegame)
// FUNCTION: GAME 0x4603c0
bool C_PhThing::SaveGameSave(void **buf)
{
    if (!C_actor::SaveGameSave(buf))
        return false;
    S_phthing_savegame *s = (S_phthing_savegame *)*buf;
    S_matrix unknown_43;
    s->version = PHTHING_SAVEGAME_VERSION;
    s->pos = frame->GetPos();
    s->rot = frame->GetRot();
    s->center_offset = center_offset;
    s->thing_type = thing_type;
    s->unknown_2e = true;
    s->max_step = max_step;
    s->weight = weight;
    s->inv_inertia = inv_inertia;
    s->unknown_43 = unknown_43;
    s->velocity = velocity;
    s->rot_velocity = rot_velocity;
    s->friction = friction;
    s->bounce = bounce;
    s->contact_reach = contact_reach;
    s->stiffness = stiffness;
    s->unknown_258 = unknown_258;
    s->width = width;
    s->radius = radius;
    s->rolling_friction = rolling_friction;
    s->spring_force = spring_force;
    s->contact_pos = contact_pos;
    s->unknown_cb = contact_reach;
    s->rest_time = rest_time;
    *buf = (uchar *)*buf + sizeof(S_phthing_savegame);
    return true;
}

// Reads what SaveGameSave wrote back, places the frame and makes the matrices and the collision again
// FUNCTION: GAME 0x4605a0
bool C_PhThing::SaveGameLoad(void **buf)
{
    if (!C_actor::SaveGameLoad(buf))
        return false;
    S_phthing_savegame *s = (S_phthing_savegame *)*buf;
    if (s->version != PHTHING_SAVEGAME_VERSION)
        return false;
    frame->SetPos(s->pos);
    frame->SetRot(s->rot);
    frame->Update();
    pos = frame->GetPos();
    center_offset = s->center_offset;
    thing_type = s->thing_type;
    max_step = s->max_step;
    weight = s->weight;
    inv_inertia = s->inv_inertia;
    velocity = s->velocity;
    rot_velocity = s->rot_velocity;
    friction = s->friction;
    bounce = s->bounce;
    contact_reach = s->contact_reach;
    stiffness = s->stiffness;
    unknown_258 = s->unknown_258;
    width = s->width;
    radius = s->radius;
    rolling_friction = s->rolling_friction;
    spring_force = s->spring_force;
    contact_pos = s->contact_pos;
    contact_dist = s->contact_reach;
    rest_time = s->rest_time;
    *buf = (uchar *)*buf + sizeof(S_phthing_savegame);
    matrix = frame->GetMatrix();
    inv_matrix = matrix;
    inv_matrix.Invert();
    rot_matrix = matrix;
    rot_matrix(3).Zero();
    inv_rot_matrix = inv_matrix;
    inv_rot_matrix(3).Zero();
    dir_x = matrix(0);
    dir_y = matrix(1);
    dir = matrix(2);
    gravity_force = S_vector(0.0f, -1.0f, 0.0f) * weight * gravity;
    if (has_center_offset)
        center = center_offset * matrix;
    else
        center = pos;
    ((I3D_bsphere &)coll.bsphere).pos = pos;
    gifCollision.DynUpdate(&coll);
    return true;
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_PhThing::NetDirect(uint time)
{
}
