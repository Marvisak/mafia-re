// The rails' turnouts (PS2 C_turnout.cpp), file 41 of docs/game-files.md: 0x4bc360-0x4bd64c, in PS2's order (Init is
// C_pumper's folded copy, Hit C_actor's). Open: NodesOK's registers, LoadData's one difference.

#include "vehicles/C_turnout.h"

#include <math.h>
#include <string.h>

#include "ai/pathweb.h"
#include "C_chunk.h"
#include "collision/g_c_glob.h"
#include "collision/g_colls.h"
#include "g_material.h"
#include "mission/game.h"
#include "mission/Mission.h"
#include "vehicles/C_wagon.h"
#include "stub.h"

// The parameters' defaults. A temporary C_actor is constructed and destroyed at once, as on PS2: a base constructor
// called as a statement
// FUNCTION: GAME 0x4bc360
C_turnout::C_turnout()
{
    C_actor();
    type = ACTOR_TURNOUT;
    switched = false;
    max_angle = .5f;
    turn_speed = .004f;
    // STRING: GAME 0x634af8
    strcpy(rail1_name, "rail1");
    // STRING: GAME 0x634af0
    strcpy(rail2_name, "rail2");
    sound_id = 0x85;
    node_id = 0x7b;
    rail1_node_id = 0x7c;
    rail2_node_id = 0x7d;
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x4bc490
C_turnout::~C_turnout()
{
}

// The original's is C_pumper's, folded: it logs that name, so ours folds too
// STUB: GAME 0x47c760 FOLDED
bool C_turnout::Init(I3D_frame *frm)
{
    StubLog("C_pumper::Init");
    return C_actor::Init(frm);
}

// A version byte, then the parameters
// FUNCTION: GAME 0x4bc4a0
bool C_turnout::SaveData(C_chunk *chunk)
{
    uchar version = 0;
    chunk->Write(&version, sizeof(version));
    chunk->Write(&switched, sizeof(switched));
    chunk->Write(&max_angle, sizeof(max_angle));
    chunk->Write(&turn_speed, sizeof(turn_speed));
    chunk->Write(&sound_id, sizeof(sound_id));
    chunk->Write(&node_id, sizeof(node_id));
    chunk->Write(&rail1_node_id, sizeof(rail1_node_id));
    chunk->Write(&rail2_node_id, sizeof(rail2_node_id));
    chunk->Write(rail1_name, sizeof(rail1_name));
    chunk->Write(rail2_name, sizeof(rail2_name));
    return true;
}

// FUNCTION: GAME 0x4bc600
bool C_turnout::LoadData(C_chunk *chunk)
{
    uchar version;
    chunk->Read(&version, sizeof(version));
    chunk->Read(&switched, sizeof(switched));
    chunk->Read(&max_angle, sizeof(max_angle));
    chunk->Read(&turn_speed, sizeof(turn_speed));
    chunk->Read(&sound_id, sizeof(sound_id));
    chunk->Read(&node_id, sizeof(node_id));
    chunk->Read(&rail1_node_id, sizeof(rail1_node_id));
    chunk->Read(&rail2_node_id, sizeof(rail2_node_id));
    chunk->Read(rail1_name, sizeof(rail1_name));
    chunk->Read(rail2_name, sizeof(rail2_name));
    return true;
}

// Copies the parameters
// FUNCTION: GAME 0x4bc830
void C_turnout::DuplicateFrom(C_actor *act)
{
    C_turnout *other = (C_turnout *)act;
    switched = other->switched;
    max_angle = other->max_angle;
    turn_speed = other->turn_speed;
    sound_id = other->sound_id;
    node_id = other->node_id;
    rail1_node_id = other->rail1_node_id;
    rail2_node_id = other->rail2_node_id;
    switched = other->switched;
    strcpy(rail1_name, other->rail1_name);
    strcpy(rail2_name, other->rail2_name);
}

// Finds the two rails' frames, turns the frame to the switch's position, connects the web's nodes accordingly, and
// gives the turnout its collision, its sound and its using object
// FUNCTION: GAME 0x4bc8c0
void C_turnout::GameInit()
{
    C_actor::GameInit();
    rail1_frame = act_mission->GetScene()->FindFrame(rail1_name, ENUMF_ALL);
    rail2_frame = act_mission->GetScene()->FindFrame(rail2_name, ENUMF_ALL);
    dir = frame->GetDir();
    if (!switched)
    {
        state = TURNOUT_RAIL1;
        frame->SetDir(dir, max_angle);
        angle = max_angle;
        if (rail2_frame)
            rail2_frame->SetOn(false);
        if (rail1_frame)
            rail1_frame->SetOn(true);
        uint node, rail1_node, rail2_node;
        if (NodesOK(node, rail1_node, rail2_node))
        {
            act_mission->web_path->CreateConnect(node, rail1_node);
            act_mission->web_path->FlipOneWay(rail1_node, node, false);
        }
    }
    else
    {
        state = TURNOUT_RAIL2;
        frame->SetDir(dir, -max_angle);
        angle = -max_angle;
        if (rail1_frame)
            rail1_frame->SetOn(false);
        if (rail2_frame)
            rail2_frame->SetOn(true);
        uint node, rail1_node, rail2_node;
        if (NodesOK(node, rail1_node, rail2_node))
        {
            act_mission->web_path->CreateConnect(node, rail2_node);
            act_mission->web_path->FlipOneWay(rail2_node, node, false);
        }
    }
    frame->Update();
    memset(&coll, 0, sizeof(coll));
    coll.type = DYNCOLL_UNKNOWN_42;
    coll.frame = frame;
    coll.owner = this;
    ((I3D_bsphere &)coll.bsphere).radius = 0.2f;
    coll.flags = 0;
    rot_matrix = frame->GetRot().RotationMatrix();
    SetCollPos();
    gifCollision.DynInsert(&coll);
    sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
    if (sound)
    {
        if (!sound_table.ConstructSound(sound, sound_id))
        {
            sound->Release();
            sound = NULL;
        }
        else
        {
            sound->SetOn(false);
            sound->SetLoop(false);
            sound->LinkTo(frame, 0);
        }
    }
    using_obj.pos = frame->GetWorldPos();
    using_obj.radius_2 = 1.5f;
    using_obj.matrix = NULL;
    using_obj.flags = 0;
    using_obj.frame = NULL;
    using_obj.actor = this;
    using_obj.item.id = GAMEITEM_SPECIAL_ACTION;
    using_obj.item.ammo_loaded = USING_ACTION_USE_SWITCH;
    using_obj.item.using_object = &using_obj;
    act_mission->game->using_object.AddObject(&using_obj);
}

// Takes the collision, the sound and the using object out, and connects the nodes as they were before GameInit
// FUNCTION: GAME 0x4bcd40
void C_turnout::GameDone()
{
    C_actor::GameDone();
    gifCollision.DynErase(&coll);
    if (sound)
        sound->Release();
    act_mission->game->using_object.DelObject(&using_obj);
    if (state != TURNOUT_RAIL1 && state != TURNOUT_TO_RAIL2)
    {
        uint node, rail1_node, rail2_node;
        if (NodesOK(node, rail1_node, rail2_node))
            act_mission->web_path->CreateConnect(node, rail2_node);
    }
    else
    {
        uint node, rail1_node, rail2_node;
        if (NodesOK(node, rail1_node, rail2_node))
            act_mission->web_path->CreateConnect(node, rail1_node);
    }
}

// Swings the frame to the end Start asked for; once there, the turnout's nodes are connected the new way and the
// wagons' paths are made again
// FUNCTION: GAME 0x4bce10
void C_turnout::AI(uint time)
{
    switch (state)
    {
    case TURNOUT_TO_RAIL2:
    {
        angle -= time * turn_speed;
        if (angle < -max_angle)
        {
            angle = -max_angle;
            state = TURNOUT_RAIL2;
            if (rail2_frame)
                rail2_frame->SetOn(true);
            if (rail1_frame)
                rail1_frame->SetOn(false);
            uint node, rail1_node, rail2_node;
            if (NodesOK(node, rail1_node, rail2_node))
            {
                act_mission->web_path->CreateConnect(node, rail1_node);
                act_mission->web_path->CreateConnect(node, rail2_node);
                act_mission->web_path->FlipOneWay(rail2_node, node, false);
                C_wagon::RecompileNewPath();
            }
        }
        frame->SetDir(dir, angle);
        break;
    }
    case TURNOUT_TO_RAIL1:
    {
        angle += time * turn_speed;
        if (angle > max_angle)
        {
            angle = max_angle;
            state = TURNOUT_RAIL1;
            if (rail2_frame)
                rail2_frame->SetOn(false);
            if (rail1_frame)
                rail1_frame->SetOn(true);
            uint node, rail1_node, rail2_node;
            if (NodesOK(node, rail1_node, rail2_node))
            {
                act_mission->web_path->CreateConnect(node, rail1_node);
                act_mission->web_path->CreateConnect(node, rail2_node);
                act_mission->web_path->FlipOneWay(rail1_node, node, false);
                C_wagon::RecompileNewPath();
            }
        }
        frame->SetDir(dir, angle);
        break;
    }
    }
}

// Takes the frame's rotation, fits the collision to it and the actor's position and direction to the frame
// FUNCTION: GAME 0x4bcfe0
void C_turnout::Update(uint time)
{
    frame->Update();
    rot_matrix = frame->GetRot().RotationMatrix();
    SetCollPos();
    gifCollision.DynUpdate(&coll);
    pos = frame->GetPos();
    dir = frame->GetDir();
}

// Asks for the swing to the other end, with the switch's sound
// FUNCTION: GAME 0x4bd210
void C_turnout::Start()
{
    switch (state)
    {
    case TURNOUT_RAIL1:
        state = TURNOUT_TO_RAIL2;
        break;
    case TURNOUT_RAIL2:
        state = TURNOUT_TO_RAIL1;
        break;
    default:
        return;
    }
    if (sound)
    {
        sound->SetCurrTime(0);
        sound->SetOn(true);
    }
}

// The sphere 0.8 above the turnout, along the frame's y axis: the x and z axes are rotated too, unused
// FUNCTION: GAME 0x4bd270
void C_turnout::SetCollPos()
{
    S_vector x(1.0f, 0.0f, 0.0f);
    S_vector y(0.0f, 1.0f, 0.0f);
    S_vector z(0.0f, 0.0f, 1.0f);
    x = x * rot_matrix;
    y = y * rot_matrix;
    z = z * rot_matrix;
    ((I3D_bsphere &)coll.bsphere).pos = pos + y * 0.8f;
}

// FUNCTION: GAME 0x4bd390
bool C_turnout::NodesOK(uint &node, uint &rail1_node, uint &rail2_node)
{
    C_web_path *path = act_mission->web_path;
    node = path->FindNodeID(node_id);
    path = act_mission->web_path;
    rail1_node = path->FindNodeID(rail1_node_id);
    path = act_mission->web_path;
    rail2_node = path->FindNodeID(rail2_node_id);
    bool ok = true;
    if (node == -1 || rail1_node == -1 || rail2_node == -1)
        ok = false;
    if (node == rail1_node || node == rail2_node || rail1_node == rail2_node)
        ok = false;
    return ok;
}

// FUNCTION: GAME 0x4bd420
int C_turnout::SaveGameGetSize()
{
    return C_actor::SaveGameGetSize() + 16;
}

// A version byte (2), the state and the angle, in the 16 bytes after the actor's
// FUNCTION: GAME 0x4bd430
bool C_turnout::SaveGameSave(void **buf)
{
    if (!C_actor::SaveGameSave(buf))
        return false;
    uchar *p = (uchar *)*buf;
    *p = 2;
    *(int *)(p + 4) = state;
    *(float *)(p + 12) = angle;
    return true;
}

// The nodes connected as before GameInit, then the state and the angle loaded and the rails and the nodes set as
// they give
// FUNCTION: GAME 0x4bd470
bool C_turnout::SaveGameLoad(void **buf)
{
    if (!C_actor::SaveGameLoad(buf))
        return false;
    uchar *p = (uchar *)*buf;
    if (*p != 2)
        return false;
    if (state == TURNOUT_RAIL1 || state == TURNOUT_TO_RAIL2)
    {
        uint node, rail2_node, rail1_node;
        if (NodesOK(node, rail1_node, rail2_node))
            act_mission->web_path->CreateConnect(node, rail1_node);
    }
    else
    {
        uint node, rail1_node, rail2_node;
        if (NodesOK(node, rail1_node, rail2_node))
            act_mission->web_path->CreateConnect(node, rail2_node);
    }
    state = *(int *)(p + 4);
    angle = *(float *)(p + 12);
    frame->SetDir(dir, angle);
    if (state == TURNOUT_TO_RAIL2 || state == TURNOUT_RAIL1)
    {
        uint node, rail2_node, rail1_node;
        if (rail2_frame)
            rail2_frame->SetOn(false);
        if (rail1_frame)
            rail1_frame->SetOn(true);
        if (NodesOK(node, rail1_node, rail2_node))
        {
            act_mission->web_path->CreateConnect(node, rail1_node);
            act_mission->web_path->FlipOneWay(rail1_node, node, false);
        }
    }
    else
    {
        uint node, rail1_node, rail2_node;
        if (rail2_frame)
            rail2_frame->SetOn(true);
        if (rail1_frame)
            rail1_frame->SetOn(false);
        if (NodesOK(node, rail1_node, rail2_node))
        {
            act_mission->web_path->CreateConnect(node, rail2_node);
            act_mission->web_path->FlipOneWay(rail2_node, node, false);
        }
    }
    Update(0);
    return true;
}
