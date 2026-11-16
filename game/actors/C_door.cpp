// The doors (PS2 C_door.cpp), file 15 of docs/game-files.md: 0x439290-0x43bd8c, the functions in PS2's order (PS2's
// ChangeSceneInit and ChangeSceneDone aren't on the PC; Collision is C_actor's folded copy). The range's first
// function, 0x439240, is C_dog.cpp's deferred copy; I3D.h's SetRot and GetHRBoundVolume(I3D_bvolume &) end it.
// Open: TurnFrame's matrix temporary (C_door.h) in its four callers, and GameInit's frame.

#include "actors/C_door.h"

#include "C_chunk.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "g_material.h"

#include <string.h>

// The settings' defaults: a wooden door, opened by a quarter turn either way
// FUNCTION: GAME 0x439290
C_door::C_door()
{
    type = ACTOR_DOOR;
    open_away = true;
    reverse = false;
    max_angle = PI / 2;
    locked = false;
    open = false;
    passive = false;
    close_speed = .004f;
    open_speed = .004f;
    // STRING: GAME 0x632570
    strcpy(open_sound, "d_woodop.wav");
    // STRING: GAME 0x632560
    strcpy(close_sound, "d_woodcl.wav");
    // STRING: GAME 0x632550
    strcpy(lock_sound, "d_woodlo.wav");
    in_omni_sector = false;
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x439390
C_door::~C_door()
{
}

// The base's, its result converted again (PS2's the same); the linker folded it with C_traffic_generator's and
// C_wagon's
// FUNCTION: GAME 0x4393a0 FOLDED
bool C_door::Init(I3D_frame *frm)
{
    return C_actor::Init(frm) ? true : false;
}

// The base's, then the settings, but passive
// FUNCTION: GAME 0x4393c0
void C_door::DuplicateFrom(C_actor *act)
{
    C_actor::DuplicateFrom(act);
    C_door *door = (C_door *)act;
    open_away = door->open_away;
    reverse = door->reverse;
    max_angle = door->max_angle;
    open = door->open;
    locked = door->locked;
    open_speed = door->open_speed;
    close_speed = door->close_speed;
    strcpy(open_sound, door->open_sound);
    strcpy(close_sound, door->close_sound);
    strcpy(lock_sound, door->lock_sound);
    in_omni_sector = door->in_omni_sector;
}

// A hit on a frame whose property names glass breaks it, pushed along the hit's direction
// FUNCTION: GAME 0x439480
bool C_door::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act, uint u,
    I3D_frame *frm)
{
    // STRING: GAME 0x632580
    if (frm && strstr(frm->GetProperty(), "glass"))
    {
        S_vector dir = v1;
        dir.Normalize();
        act_mission->game->glass.BreakGlass(frm, &dir, "lustr1.wav", true);
    }
    return true;
}

// Opens or closes the door (the topmost of the linked doors, and with it the linked ones), with the sounds, and
// tells the scene's sectors where the frame went
// FUNCTION: GAME 0x439610
void C_door::SetState(E_state state, C_actor *actor, bool unknown1, bool to_master)
{
    if (master && to_master)
    {
        master->SetState(state, actor, unknown1, true);
        return;
    }
    switch (state)
    {
    case STATE_CLOSING:
        if (act_mission->options.game_sounds >= 1 && !(door_flags & DOORF_SILENT) &&
            strlen(close_sound) && act_mission->game->initialized)
        {
            act_mission->game->Play3DSound(frame, close_sound, I3DSOUND_POINT, S_vector(0.0f, 0.0f, 0.0f),
                4.0f, 15.0f, 1.0f, false, .001f, false);
        }
        if (!in_omni_sector)
        {
            I3D_frame *frm = frame;
            I3D_scene *scene = act_mission->scene;
            scene->SetFrameSectorPos(frm, frm->GetWorldPos());
        }
        break;
    case STATE_OPENING:
        if (locked)
        {
            if (act_mission->options.game_sounds >= 1 && !(door_flags & DOORF_SILENT) &&
                strlen(lock_sound) && act_mission->game->initialized &&
                (uint)(act_mission->game->game_time - lock_sound_time) > 1000)
            {
                lock_sound_time = act_mission->game->game_time;
                act_mission->game->Play3DSound(frame, lock_sound, I3DSOUND_POINT,
                    S_vector(0.0f, 0.0f, 0.0f), 4.0f, 15.0f, 1.0f, false, .001f, false);
            }
            return;
        }
        if (act_mission->options.game_sounds >= 1 && !(door_flags & DOORF_SILENT) &&
            strlen(open_sound) && act_mission->game->initialized)
        {
            act_mission->game->Play3DSound(frame, open_sound, I3DSOUND_POINT, S_vector(0.0f, 0.0f, 0.0f),
                4.0f, 15.0f, 1.0f, false, .001f, false);
        }
        if (open_away && actor)
        {
            reverse = normal.AngleTo(frame->GetWorldPos() - actor->frame->GetWorldPos()) > PI / 2;
            if (!in_omni_sector)
            {
                int side = reverse ? -1 : 1;
                act_mission->scene->SetFrameSectorPos(frame, frame->GetWorldPos() + normal * side);
            }
        }
        break;
    }
    if (linked)
        linked->SetState(state, actor, true, false);
    this->state = state;
    SetActState(ACTSTATE_ACTIVE);
}

// The moving door turns on by the time: it opens or closes, and when it gets there it is at rest, the using object
// offering the opposite action; the frame turns and the collision follows. Open: the original's CLOSING case
// inlines TurnFrame's axis constructor and calls its operator*= (-1.0f) out of line, ours the reverse.
// FUNCTION: GAME 0x4399c0
void C_door::AI(uint time)
{
    switch (state)
    {
    case STATE_OPENING:
        angle += time * open_speed;
        if (angle >= max_angle)
        {
            angle = max_angle;
            state = STATE_OPEN;
            SetActState(ACTSTATE_INACTIVE);
            if (!(door_flags & DOORF_NO_USE))
                using_object.item.ammo_loaded = USING_ACTION_CLOSE_DOOR;
        }
        TurnFrame();
        break;
    case STATE_CLOSING:
        angle -= time * close_speed;
        if (angle <= 0.0f)
        {
            angle = 0.0f;
            state = STATE_CLOSED;
            SetActState(ACTSTATE_INACTIVE);
            if (!(door_flags & DOORF_NO_USE))
                using_object.item.ammo_loaded = USING_ACTION_OPEN_DOOR;
        }
        TurnFrame();
        break;
    }
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_door::Update(uint time)
{
}

// AI while the door moves
// FUNCTION: GAME 0x439e90
void C_door::NetDirect(uint time)
{
    if (state >= STATE_OPENING && state <= STATE_CLOSING)
        AI(time);
}

// Puts the door into the game, once (DOORF_IN_GAMEINIT is also the mark that it has been, and the master's while it
// runs the linked door's). A door in a child frame goes first and is linked; one in a master's frame waits for the
// master and then plays no sounds and has no using object. The frame goes into a sector, the collision (a box in the
// moving objects' grid), the normal and the using object (between the door's and its linked door's frames, or at the
// middle of the leaf's far edge) are made, and the door opened or closed, its frame turned to it. Open: the frame
// has one 12-byte temporary more and its slots in another order (the original's edge shares a slot), the normal's
// sign tests load their fabs in another order (the original's fxch pairs), and the sphere's radius loads its factors
// the other way round.
// FUNCTION: GAME 0x439eb0
void C_door::GameInit()
{
    if (door_flags & DOORF_IN_GAMEINIT)
        return;
    lock_sound_time = 0;
    if (frame)
        DebustizeFrame(frame);
    if (passive)
        door_flags |= DOORF_NO_COLL | DOORF_NO_USE;
    int i = frame->NumChildren();
    while (i)
    {
        C_actor **p = GetFrameActor(frame->GetChild(--i));
        if (p && *p && (*p)->type == ACTOR_DOOR)
        {
            door_flags |= DOORF_IN_GAMEINIT;
            (*p)->GameInit();
            door_flags &= ~DOORF_IN_GAMEINIT;
            linked = (C_door *)*p;
            linked->master = this;
            break;
        }
    }
    bool slave = false;
    I3D_frame *parent = frame->GetParent();
    if (parent)
    {
        C_actor **p = GetFrameActor(parent);
        if (p && *p && (*p)->type == ACTOR_DOOR)
        {
            master = (C_door *)*p;
            if (!(((C_door *)*p)->door_flags & DOORF_IN_GAMEINIT))
                return;
            slave = true;
            door_flags |= DOORF_NO_USE | DOORF_SILENT;
        }
    }
    C_actor::GameInit();
    bool in_sector = true;
    I3D_frame *old_parent = saved_parent;
    if (old_parent)
    {
        C_actor **p = GetFrameActor(old_parent);
        if (!(p && *p && (*p)->type == ACTOR_DOOR))
        {
            old_parent = old_parent->GetParent();
            if (old_parent)
            {
                p = GetFrameActor(old_parent);
                if (p && *p && (*p)->type == ACTOR_DOOR)
                {
                    in_sector = false;
                    frame->SetPos(saved_pos);
                    frame->SetRot(saved_rot);
                    frame->Update();
                    frame->LinkTo(saved_parent, 0);
                }
            }
        }
    }
    if (in_omni_sector)
        frame->LinkTo(act_mission->scene->omni_sector, I3DLINK_UPDMATRIX);
    else if (in_sector)
    {
        frame->LinkTo(act_mission->scene->primary_sector, I3DLINK_UPDMATRIX);
        I3D_frame *frm = frame;
        I3D_scene *scene = act_mission->scene;
        scene->SetFrameSectorPos(frm, frm->GetWorldPos());
    }
    if (slave)
    {
        frame->LinkTo(old_parent->GetParent(), I3DLINK_UPDMATRIX);
        old_parent->Update();
    }
    if (locked)
        door_flags |= DOORF_LOCKED;
    if (reverse)
        door_flags |= DOORF_REVERSE;
    rot = frame->GetWorldRot();
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
    coll.flags = DYNCOLLF_UNKNOWN_1000;
    const S_matrix &world = frame->GetMatrix();
    coll.matrix = (S_matrix *)&world;
    inv_matrix = world;
    inv_matrix.Invert();
    coll.inv_matrix = &inv_matrix;
    if (!(door_flags & DOORF_NO_COLL))
        gifCollision.DynInsert(&coll);

    // The normal: across the leaf's thinner horizontal side, turned to the side the leaf's box lies on
    I3D_bvolume bvol2;
    frame->GetHRBoundVolume(bvol2);
    ext = bvol2.bbox.max - bvol2.bbox.min;
    if (ext.x > ext.z)
    {
        normal = S_vector(0.0f, 0.0f, 1.0f);
        if (fabs(bvol2.bbox.min.x) > fabs(bvol2.bbox.max.x))
            normal *= -1.0f;
    }
    else
    {
        normal = S_vector(1.0f, 0.0f, 0.0f);
        if (fabs(bvol2.bbox.min.z) < fabs(bvol2.bbox.max.z))
            normal *= -1.0f;
    }
    S_matrix rotation = frame->GetMatrix();
    rotation(3).x = rotation(3).y = rotation(3).z = 0.0f;
    normal *= rotation;
    if (open)
    {
        SetState(STATE_OPENING, (C_actor *)act_mission->game->player, true, true);
        state = STATE_OPEN;
        angle = max_angle;
    }
    else
    {
        state = STATE_CLOSED;
        angle = 0.0f;
    }

    // The leaf's far edge from the hinge: the box's side of the greatest distance along x or z, and the point 0.4 m
    // beyond it
    frame->GetHRBoundVolume(bvol);
    S_vector edge;
    float reach = fabs(bvol.bbox.min.x);
    leaf_end = S_vector(bvol.bbox.min.x - .4f, 0.0f, 0.0f);
    edge = S_vector(bvol.bbox.min.x, 0.0f, 0.0f);
    float t = fabs(bvol.bbox.max.x);
    if (t > reach)
    {
        reach = t;
        leaf_end = S_vector(bvol.bbox.max.x + .4f, 0.0f, 0.0f);
        edge = S_vector(bvol.bbox.max.x, 0.0f, 0.0f);
    }
    t = fabs(bvol.bbox.min.z);
    if (t > reach)
    {
        reach = t;
        leaf_end = S_vector(0.0f, 0.0f, bvol.bbox.min.z - .4f);
        edge = S_vector(0.0f, 0.0f, bvol.bbox.min.z);
    }
    t = fabs(bvol.bbox.max.z);
    if (t > reach)
    {
        leaf_end = S_vector(0.0f, 0.0f, bvol.bbox.max.z + .4f);
        edge = S_vector(0.0f, 0.0f, bvol.bbox.max.z);
    }
    if (!(door_flags & DOORF_NO_USE))
    {
        if (linked)
        {
            using_object.pos = (frame->GetWorldPos() + linked->frame->GetWorldPos()) * .5f;
            using_object.radius_2 = edge.Magnitude() + .15f > .75f ? edge.Magnitude() + .15f : .75f;
        }
        else
        {
            using_object.pos = edge * .5f;
            using_object.pos *= frame->GetMatrix();
            using_object.radius_2 = edge.Magnitude() * .5f + .15f > .75f ? edge.Magnitude() * .5f + .15f : .75f;
        }
        using_object.matrix = NULL;
        using_object.flags = 0;
        using_object.frame = NULL;
        using_object.actor = this;
        using_object.item.id = GAMEITEM_SPECIAL_ACTION;
        using_object.item.ammo_loaded = state ? USING_ACTION_OPEN_DOOR : USING_ACTION_CLOSE_DOOR;
        using_object.item.using_object = &using_object;
        act_mission->game->using_object.AddObject(&using_object);
        door_flags |= DOORF_USING;
    }

    TurnFrame();
    SetActState(ACTSTATE_INACTIVE);
    door_flags |= DOORF_IN_GAMEINIT;
}

// The base's, the using object and the collision taken out, and the lock and the direction GameInit found given back
// FUNCTION: GAME 0x43ad60
void C_door::GameDone()
{
    C_actor::GameDone();
    if (!(door_flags & DOORF_NO_USE))
        act_mission->game->using_object.DelObject(&using_object);
    if (!(door_flags & DOORF_NO_COLL))
        gifCollision.DynErase(&coll);
    locked = (door_flags & DOORF_LOCKED) != 0;
    reverse = (door_flags & DOORF_REVERSE) != 0;
}

// The using object put into the game's or taken out, unless the door has none
// FUNCTION: GAME 0x43add0
void C_door::EnableUsingObject(bool enable)
{
    if (door_flags & DOORF_NO_USE)
        return;
    if (enable)
    {
        if (!(door_flags & DOORF_USING))
        {
            act_mission->game->using_object.AddObject(&using_object);
            door_flags |= DOORF_USING;
        }
    }
    else if (door_flags & DOORF_USING)
    {
        act_mission->game->using_object.DelObject(&using_object);
        door_flags &= ~DOORF_USING;
    }
}

// The settings, after their version; true
// FUNCTION: GAME 0x43ae50
bool C_door::SaveData(C_chunk *chunk)
{
    int version = DOOR_DATA_VERSION;
    chunk->Write(&version, sizeof(version));
    chunk->Write(&passive, sizeof(passive));
    chunk->Write(&open_away, sizeof(open_away));
    chunk->Write(&reverse, sizeof(reverse));
    chunk->Write(&max_angle, sizeof(max_angle));
    chunk->Write(&open, sizeof(open));
    chunk->Write(&locked, sizeof(locked));
    chunk->Write(&open_speed, sizeof(open_speed));
    chunk->Write(&close_speed, sizeof(close_speed));
    chunk->Write(open_sound, sizeof(open_sound));
    chunk->Write(close_sound, sizeof(close_sound));
    chunk->Write(lock_sound, sizeof(lock_sound));
    chunk->Write(&in_omni_sector, sizeof(in_omni_sector));
    return true;
}

// The settings by the version they were saved with: 5 had no passive and open_away, 6 no passive, 7 no in_omni_sector;
// true
// FUNCTION: GAME 0x43b030
bool C_door::LoadData(C_chunk *chunk)
{
    int version;
    chunk->Read(&version, sizeof(version));
    switch (version)
    {
    case DOOR_DATA_VERSION:
        chunk->Read(&passive, sizeof(passive));
        chunk->Read(&open_away, sizeof(open_away));
        chunk->Read(&reverse, sizeof(reverse));
        chunk->Read(&max_angle, sizeof(max_angle));
        chunk->Read(&open, sizeof(open));
        chunk->Read(&locked, sizeof(locked));
        chunk->Read(&open_speed, sizeof(open_speed));
        chunk->Read(&close_speed, sizeof(close_speed));
        chunk->Read(open_sound, sizeof(open_sound));
        chunk->Read(close_sound, sizeof(close_sound));
        chunk->Read(lock_sound, sizeof(lock_sound));
        chunk->Read(&in_omni_sector, sizeof(in_omni_sector));
        break;
    case 7:
        chunk->Read(&passive, sizeof(passive));
        chunk->Read(&open_away, sizeof(open_away));
        chunk->Read(&reverse, sizeof(reverse));
        chunk->Read(&max_angle, sizeof(max_angle));
        chunk->Read(&open, sizeof(open));
        chunk->Read(&locked, sizeof(locked));
        chunk->Read(&open_speed, sizeof(open_speed));
        chunk->Read(&close_speed, sizeof(close_speed));
        chunk->Read(open_sound, sizeof(open_sound));
        chunk->Read(close_sound, sizeof(close_sound));
        chunk->Read(lock_sound, sizeof(lock_sound));
        break;
    case 6:
        chunk->Read(&open_away, sizeof(open_away));
        chunk->Read(&reverse, sizeof(reverse));
        chunk->Read(&max_angle, sizeof(max_angle));
        chunk->Read(&open, sizeof(open));
        chunk->Read(&locked, sizeof(locked));
        chunk->Read(&open_speed, sizeof(open_speed));
        chunk->Read(&close_speed, sizeof(close_speed));
        chunk->Read(open_sound, sizeof(open_sound));
        chunk->Read(close_sound, sizeof(close_sound));
        chunk->Read(lock_sound, sizeof(lock_sound));
        break;
    case 5:
        chunk->Read(&reverse, sizeof(reverse));
        chunk->Read(&max_angle, sizeof(max_angle));
        chunk->Read(&open, sizeof(open));
        chunk->Read(&locked, sizeof(locked));
        chunk->Read(&open_speed, sizeof(open_speed));
        chunk->Read(&close_speed, sizeof(close_speed));
        chunk->Read(open_sound, sizeof(open_sound));
        chunk->Read(close_sound, sizeof(close_sound));
        chunk->Read(lock_sound, sizeof(lock_sound));
        break;
    }
    return true;
}

// FUNCTION: GAME 0x43b4f0
int C_door::SaveGameGetSize()
{
    return C_actor::SaveGameGetSize() + sizeof(S_door_savegame);
}

// The actor's, then the door's state
// FUNCTION: GAME 0x43b500
bool C_door::SaveGameSave(void **buf)
{
    if (!C_actor::SaveGameSave(buf))
        return false;
    S_door_savegame *s = (S_door_savegame *)*buf;
    s->version = DOOR_SAVEGAME_VERSION;
    s->angle = angle;
    s->door_flags = door_flags;
    s->state = state;
    s->reverse = reverse;
    s->locked = locked;
    *buf = (uchar *)*buf + sizeof(S_door_savegame);
    return true;
}

// The actor's, then the door's state; the frame turned as far as the door is open, its collision and using object
// put in as they were. The accessors after TurnFrame are inline calls still to come, which leave its quaternion
// product, SetRot and GetHRBoundVolume out of line as the original's (their spelling is a guess).
// FUNCTION: GAME 0x43b560
bool C_door::SaveGameLoad(void **buf)
{
    if (!C_actor::SaveGameLoad(buf))
        return false;
    S_door_savegame *s = (S_door_savegame *)*buf;
    if (s->version != DOOR_SAVEGAME_VERSION)
        return false;
    angle = s->angle;
    door_flags = s->door_flags;
    state = (E_state)s->state;
    reverse = s->reverse;
    locked = s->locked;
    *buf = (uchar *)*buf + sizeof(S_door_savegame);
    // A closed door gets the switch's action, where GameInit and AI give it USING_ACTION_OPEN_DOOR (PS2's here too)
    if (!(door_flags & DOORF_NO_USE))
    {
        switch (state)
        {
        case STATE_OPEN:
        case STATE_OPENING:
            using_object.item.ammo_loaded = USING_ACTION_CLOSE_DOOR;
            break;
        case STATE_CLOSED:
        case STATE_CLOSING:
            using_object.item.ammo_loaded = USING_ACTION_USE_SWITCH;
            break;
        }
    }
    TurnFrame();
    if (door_flags & DOORF_USING)
        act_mission->GetGame()->using_object.AddObject(GetUsingObject());
    else
        act_mission->GetGame()->using_object.DelObject(GetUsingObject());
    return true;
}

// The door opened to open (0 to 1) of its way at once, standing open: with nothing after it, TurnFrame is inlined
// whole
// FUNCTION: GAME 0x43b810
void C_door::SetOpenAngle(float open)
{
    angle = open * max_angle;
    state = STATE_OPEN;
    TurnFrame();
}

// The point the door's leaf ends in, in the world
// FUNCTION: GAME 0x43bab0
bool C_door::GetBoundPoints2D(std::vector<S_vector2> &points, I3D_frame *frm)
{
    if (!frame)
        return false;
    S_vector v = leaf_end * frame->GetMatrix();
    S_vector2 p(v.x, v.z);
    points.push_back(p);
    return true;
}

// The collision taken out of the game when the actor leaves it
// FUNCTION: GAME 0x43bcc0
void C_door::ChangeState()
{
    switch (act_state)
    {
    case ACTSTATE_NONE:
        if (!(door_flags & DOORF_NO_COLL))
            gifCollision.DynErase(&coll);
        break;
    }
}
