// The bottles (PS2 C_bottle.cpp; the PC's file is the one at 0x412350-0x413d8c, docs/game-files.md), in PS2's order:
// a thrown object's physics, which steps the bottle along its motion and its fall, bounces it off what it hits and
// turns it about a random axis, and the collisions and hits that throw it.

#include "actors/C_bottle.h"

#include "mission/Mission.h"

// PS2's, its stores in this order
// FUNCTION: GAME 0x412350
C_bottle::C_bottle()
{
    use_coll = true;
    type = ACTOR_BOTTLE;
    unknown_6d = true;
    moving = true;
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x4123b0
C_bottle::~C_bottle()
{
}

// FUNCTION: GAME 0x4123c0
bool C_bottle::Init(I3D_frame *frm)
{
    if (!frm)
        return false;
    if (!C_actor::Init(frm))
        return false;
    DebustizeFrame(frm);
    return true;
}

// A step of the time's length (in ms) along the motion, tested against the collisions, but where the bottle rests or
// falls too fast: at a hit it is put back off the surface, slows to the bounce's share and leaves along the surface's
// reflection, or comes to rest on a floor at a low speed; without a hit it moves on and falls faster. Then it turns
// about its axis. The fall's (float)time is the conversion the step's division made (time * .01 converts again).
// Open: the frame, the original's code with two slots in another order. The first Magnitude's IsMrgZeroLess parameter
// shares len's slot in the original, the normalizations' sq in ours, and the sort then puts their slot below step; no
// declaration order or spelling tried (len, dist, ext, from, the locals at the top) moved it.
// FUNCTION: GAME 0x4123f0
void C_bottle::AI(uint time)
{
    if (fall_speed > 50.0f)
    {
        if (set_states)
            SetActState(ACTSTATE_NONE);
        else
            moving = false;
        return;
    }
    if (speed <= MRG_ZERO && fall_speed <= MRG_ZERO)
    {
        if (set_states)
            SetActState(ACTSTATE_INACTIVE);
        else
            moving = false;
        return;
    }
    S_vector from = pos;
    S_vector step(0.0f, -fall_speed, 0.0f);
    if (speed != 0.0f)
        step += move_dir * speed;
    float len = step.Magnitude();
    float dist = len * (time / 1000.0f);
    step.Normalize();
    step *= dist;
    S_vector ext = step;
    ext.Normalize();
    ext *= radius;
    gifCollision.DynSetNextCollOwner(&coll, NULL);
    S_vector hit_pos, normal;
    g_collision_header *col;
    if (test_all)
        col = (g_collision_header *)gifCollision.TestLineV(from, step + ext, &hit_pos, &normal, COLF_ALL, 0);
    else
        col = (g_collision_header *)gifCollision.TestLineV(from, step + ext, &hit_pos, &normal, COLF_UNKNOWN_2, 0);
    gifCollision.DynSetNextCollOwner(NULL, NULL);
    hit_material = col ? col->material : 0;
    hit = col != NULL;
    if (col)
    {
        if (col->flags & COLLHF_UNKNOWN_40)
            hit_actor = col->owner;
        else
            hit_actor = NULL;
        bounces++;
        hit_normal = normal;
        ext *= -1.0f;
        float angle = normal.AngleTo(ext);
        ext.Normalize();
        if (angle < PI)
            ext *= radius;
        pos = hit_pos + ext;
        fall_speed = 0.0f;
        if (len < 3.0f && normal.AngleTo(S_vector(0.0f, 1.0f, 0.0f)) < .5)
        {
            speed = 0.0f;
            fall_speed = 0.0f;
            move_dir.Zero();
            hit = false;
        }
        else
        {
            speed = len * bounce;
            if (step.AngleTo(normal) < PI / 2)
                normal *= -1.0f;
            step.Normalize();
            normal.Normalize();
            move_dir = normal.Cross(step);
            move_dir *= 2.0f;
            move_dir = move_dir.Cross(normal);
            move_dir -= step;
            move_dir.Normalize();
            pos += move_dir * .01f;
        }
    }
    else
    {
        pos += step;
        fall_speed += (float)time * .01;
    }
    S_quat q;
    q.Make(rot_axis, len * time / 2000.0f);
    S_matrix m = q.RotationMatrix();
    dir *= m;
    axis *= m;
}

// The frame placed at the bottle, turned to dir with axis up, and its sector; the collision moved with it
// FUNCTION: GAME 0x413040
void C_bottle::Update(uint time)
{
    frame->SetPos(pos);
    frame->SetDir1(dir, axis);
    frame->Update();
    act_mission->scene->SetFrameSector(frame);
    if (use_coll)
    {
        ((I3D_bsphere &)coll.bsphere).pos = pos;
        gifCollision.DynUpdate(&coll);
    }
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_bottle::NetDirect(uint time)
{
}

// At rest, its radius from the frame's box in the world and its axis from the frame's world matrix; the collision
// put in where use_coll is set
// FUNCTION: GAME 0x413120
void C_bottle::GameInit()
{
    C_actor::GameInit();
    speed = 0.0f;
    fall_speed = 0.0f;
    hit_normal = S_vector(0.0f, 0.0f, 0.0f);
    rot_axis = S_vector(rnd(), rnd(), rnd());
    set_states = true;
    bounces = 0;
    hit = false;
    hit_material = 0;
    hit_actor = NULL;
    bounce = .4f;
    test_all = false;
    I3D_bvolume bvol = frame->GetHRBoundVolume();
    S_vector size = bvol.bbox.max - bvol.bbox.min;
    radius = (frame->GetWorldScale() * size).Magnitude() * .5f;
    S_matrix m = frame->GetMatrix();
    m(3, 0) = m(3, 1) = m(3, 2) = 0.0f;
    axis = S_vector(0.0f, 1.0f, 0.0f);
    axis *= m;
    if (use_coll)
    {
        memset(&coll, 0, sizeof(coll));
        coll.type = DYNCOLL_UNKNOWN_42;
        ((I3D_bsphere &)coll.bsphere).pos = pos;
        ((I3D_bsphere &)coll.bsphere).radius = radius;
        coll.frame = frame;
        coll.owner = this;
        coll.flags = DYNCOLLF_UNKNOWN_400;
        gifCollision.DynInsert(&coll);
    }
    start_pos = pos;
    start_motion = move_dir * speed;
    start_fall_speed = fall_speed;
    unknown_124 = 0;
    SetActState(ACTSTATE_INACTIVE);
}

// FUNCTION: GAME 0x413440
void C_bottle::GameDone()
{
    if (use_coll)
        gifCollision.DynErase(&coll);
    C_actor::GameDone();
}

// FUNCTION: GAME 0x413470
int C_bottle::SaveGameGetSize()
{
    return C_actor::SaveGameGetSize() + sizeof(S_bottle_savegame);
}

// FUNCTION: GAME 0x413480
bool C_bottle::SaveGameSave(void **buf)
{
    if (!C_actor::SaveGameSave(buf))
        return false;
    S_bottle_savegame *s = (S_bottle_savegame *)*buf;
    s->version = BOTTLE_SAVEGAME_VERSION;
    s->pos = pos;
    s->dir = dir;
    s->move_dir = move_dir;
    s->speed = speed;
    s->fall_speed = fall_speed;
    s->set_states = set_states;
    s->hit_normal = hit_normal;
    s->radius = radius;
    s->axis = axis;
    s->bounces = bounces;
    s->hit = hit;
    s->hit_material = hit_material;
    *buf = (uchar *)*buf + sizeof(S_bottle_savegame);
    return true;
}

// The frame placed at the saved position and direction
// FUNCTION: GAME 0x413560
bool C_bottle::SaveGameLoad(void **buf)
{
    if (!C_actor::SaveGameLoad(buf))
        return false;
    S_bottle_savegame *s = (S_bottle_savegame *)*buf;
    if (s->version != BOTTLE_SAVEGAME_VERSION)
        return false;
    pos = s->pos;
    dir = s->dir;
    move_dir = s->move_dir;
    speed = s->speed;
    fall_speed = s->fall_speed;
    set_states = s->set_states;
    hit_normal = s->hit_normal;
    radius = s->radius;
    axis = s->axis;
    bounces = s->bounces;
    hit = s->hit;
    hit_material = s->hit_material;
    *buf = (uchar *)*buf + sizeof(S_bottle_savegame);
    frame->SetPos(pos);
    frame->SetDir(dir, 0.0f);
    frame->Update();
    return true;
}

// FUNCTION: GAME 0x4136a0
void C_bottle::AbsoluteImpulse(S_vector &v)
{
    speed = v.Magnitude();
    move_dir = v;
    move_dir.Normalize();
    fall_speed = 0.0f;
    SetActState(ACTSTATE_ACTIVE);
}

// The PC's moves v1, where PS2's moves the bottle (PS2's v2 + move_dir * speed the same)
// FUNCTION: GAME 0x413840
bool C_bottle::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    I3D_frame *frm)
{
    switch (act->GetType())
    {
    case ACTOR_PLAYER:
    case ACTOR_BOTTLE:
    case ACTOR_ENTITY:
        break;
    default:
        return false;
    }
    S_vector motion = v2 + move_dir * speed;
    rot_axis = S_vector(rnd(), rnd(), rnd());
    speed = motion.Magnitude();
    move_dir = motion;
    move_dir.Normalize();
    (S_vector &)v1 += hit_normal * .01f;
    SetActState(ACTSTATE_ACTIVE);
    return true;
}

// The PC's changes v1 in place, where PS2's changes a copy
// FUNCTION: GAME 0x413ac0
bool C_bottle::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
    uint u, I3D_frame *frm)
{
    switch (type)
    {
    case HIT_SHOT:
    case HIT_STROKE:
    case HIT_EXPLOSION:
        {
            S_vector &v = (S_vector &)v1;
            v.y = (float)fabs(v.y);
            v *= 15.0f;
            S_vector motion = v + move_dir * speed;
            rot_axis = S_vector(rnd(), rnd(), rnd());
            speed = motion.Magnitude();
            move_dir = motion;
            move_dir.Normalize();
            pos += hit_normal * .01f;
            SetActState(ACTSTATE_ACTIVE);
        }
        break;
    }
    return true;
}
