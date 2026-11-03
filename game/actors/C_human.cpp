// The humans (PS2 C_human.cpp; the PC's file is the one at 0x56b180-0x58e34c, docs/game-files.md). The PC's starts with
// the cutscenes' virtual functions and their helpers, then the constructor (0x56efd0), then PS2's functions in PS2's
// order. Each "// C_HUMAN" line is the place of a function not written yet, whose stub follows the written functions.

#include "actors/C_human.h"

#include "I3D_cache.h"
#include "g_material.h"
#include "ai/jp_ai_entity.h"
#include "input/C_ff_manager.h"
#include "mission/C_effects.h"
#include "ui/g_indicators.h"
#include "ui/g_loadsave.h"
#include "ui/racing.h"
#include "vehicles/C_car.h"

#include "ai/C_person_traffic.h"
#include "collision/g_colls.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "vehicles/C_car_ex.h"

#include "ai/jp_ai_main.h"
#include "ai/pathweb.h"
#include "actors/C_player.h"

#include <math.h>

#include "actors/C_drop_in_weapon.h"
#include "actors/C_bridge.h"
#include "actors/C_public_physics.h"
#include "mission/C_recorder.h"
#include "stub.h"

// Adds the human to the cutscene, with the sizes of its three kinds of records (C_actor's: 0x5c, 0, 0)
// FUNCTION: GAME 0x56b180
bool C_human::vfunc_9()
{
    cutscene.vfunc_8(this, 0x28, 0x2c, 0x38);
    return true;
}

// C_HUMAN 0x56b1a0 vfunc_0
// C_HUMAN 0x56b670 vfunc_1
// C_HUMAN 0x56b8d0 vfunc_2
// C_HUMAN 0x56bec0 vfunc_3
// C_HUMAN 0x56c6e0 vfunc_12
// C_HUMAN 0x56c7c0 vfunc_4
// C_HUMAN 0x56cc70 vfunc_5
// C_HUMAN 0x56d130 Unknown56d130

// ai_loc's and ai_blob_log's methods (PS2's are at the start of its C_human.cpp too)

// FUNCTION: GAME 0x56e370
void ai_loc::Init(C_actor *act)
{
    actor = act;
    pos = actor->frame->GetWorldPos();
    flags = 0;
    time = 0;
}


// Finds the human's node and connection on the web, at most every 5 seconds unless he moved 2 metres, and the specials
// of the AI's segments he is in
// FUNCTION: GAME 0x56e3b0
void ai_loc::Update()
{
    if (((C_human *)actor)->car)
        return;
    if (!act_mission->web_path->num_nodes) {
        flags = 0;
        return;
    }
    S_vector pos = actor->frame->GetWorldPos();
    S_vector d = pos - this->pos;
    if (flags & AILOC_UPDATED) {
        if (d.x * d.x + d.y * d.y + d.z * d.z < 4.0f)
            return;
        if (act_mission->game->game_time < time + 5000)
            return;
    }
    uint found;
    AIManager.wayfind.GetPosition(pos, &node, &found, 3);
    flags = 0;
    this->pos = pos;
    time = act_mission->game->game_time;
    if (found & 1) {
        C_web_node *n = &act_mission->web_path->nodes[node];
        for (uint i = 0; i < n->num_conns; i++) {
            C_web_node *o = &act_mission->web_path->nodes[n->GetConn(i)->node];
            if (ai_path::isInConnection(n->GetPos(), o->GetPos(), pos, n->width * 0.01f, o->width * 0.01f)) {
                conn = i;
                node2 = o - act_mission->web_path->nodes;
                flags |= AILOC_CONN | AILOC_UPDATED;
                goto segments;
            }
        }
        flags |= AILOC_NODE | AILOC_UPDATED;
    } else
        flags |= AILOC_UPDATED;
segments:
    for (uint i = 0; i < AIManager.segments.NumSegments(); i++) {
        check_seg *seg = AIManager.segments.GetSegment(i);
        uint n = seg->unknown_c.size();
        if (!n)
            continue;
        bool in = false;
        if (flags & AILOC_CONN) {
            if (seg->existIndex(node, 0, n - 1) || seg->existIndex(node2, 0, n - 1))
                in = true;
        } else if (seg->existIndex(node, 0, n - 1))
            in = true;
        if (!in)
            continue;
        for (uint j = 0; j < seg->NumSpecials(); j++) {
            uint type;
            void *special;
            seg->GetSpecial(j, &type, &special);
            if (seg->isInSpecial(pos))
                AddSegment(type, special);
        }
    }
    SegListUpdate();
}

// Adds the special to the list, or counts it again
// FUNCTION: GAME 0x56e6f0
void ai_loc::AddSegment(uint type, void *special)
{
    for (uint i = 0; i < segs.size(); i++) {
        if (segs[i].special == special) {
            segs[i].time = act_mission->game->game_time;
            segs[i].count++;
            return;
        }
    }
    ai_seg_stat s;
    s.type = type;
    s.time = act_mission->game->game_time;
    s.count = 1;
    s.special = special;
    segs.push_back(s);
}

// Forgets the specials not found for 4 seconds
// FUNCTION: GAME 0x56e950
void ai_loc::SegListUpdate()
{
    for (int i = segs.size() - 1; i >= 0; i--) {
        if (segs[i].time + 4000 < act_mission->game->game_time)
            segs.erase(segs.begin() + i);
    }
}


// Logs the human's position, unless the last is less than 500 ms old
// FUNCTION: GAME 0x56ea00
void ai_blob_log::Blob(void *owner)
{
    if (last != -1 && act_mission->game->game_time < blobs[last & 0xf].time + 500)
        return;
    int prev = last & 0xf;
    last++;
    ai_blob &b = blobs[last & 0xf];
    b.actor = (C_actor *)owner;
    b.time = act_mission->game->game_time;
    b.pos = ((C_human *)owner)->frame->GetWorldPos() + S_vector(0.0f, 1.0f, 0.0f);
    if (blobs[prev].actor) {
        b.dir = b.pos - blobs[prev].pos;
        b.dir.Normalize();
    } else
        b.dir = ((C_human *)owner)->frame->GetWorldDir();
    b.flags = 0;
    if (((C_human *)owner)->crouching)
        b.flags = AIBLOB_CROUCHING;
    if (((C_human *)owner)->unknown_1ec)
        b.flags |= AIBLOB_UNKNOWN_4;
    switch (((C_human *)owner)->anim_id & 0x1ff) {
    case 5:
        b.flags |= AIBLOB_UNKNOWN_1;
        break;
    case 0x5e:
    case 0x5f:
        b.flags |= AIBLOB_UNKNOWN_8;
        break;
    case 0x60:
        b.flags |= AIBLOB_UNKNOWN_8 | AIBLOB_UNKNOWN_1;
        break;
    }
}

// FUNCTION: GAME 0x56ee60
ai_blob *ai_blob_log::GetBlob(uint back)
{
    if (last == -1)
        return NULL;
    if (back > (uint)last)
        return NULL;
    int i = (last - back) & 0xf;
    ai_blob *b = new ai_blob;
    *b = blobs[i];
    return b;
}

// FUNCTION: GAME 0x56eeb0
void ai_blob_log::Clear()
{
    memset(blobs, 0, sizeof(blobs));
    last = -1;
}

// FUNCTION: GAME 0x56eed0
float _get_delta_y_(S_vector &pos)
{
    S_vector from(pos.x, pos.y + 1.0f, pos.z);
    S_vector dir(0.0f, -6.0f, 0.0f);
    S_vector hit;
    if (gifCollision.TestLineV(from, dir, &hit, NULL, 0, 0))
        return hit.y - pos.y;
    return 0.0f;
}

// FUNCTION: GAME 0x56ef50
float _get_delta_nodyn_y_(S_vector &pos)
{
    S_vector from(pos.x, pos.y + 1.0f, pos.z);
    S_vector dir(0.0f, -6.0f, 0.0f);
    S_vector hit;
    if (gifCollision.TestLineV(from, dir, &hit, NULL, (uint)-1, 0))
        return hit.y - pos.y;
    return 0.0f;
}


// The members in their order, the two entries at 0x9c4 an inlined loop; the defaults are PS2's, in its order. The
// inventory's arrays are constructed out of line (vector constructor iterator) only with an inline call in the body
// after the members, which leaves no code: SetDefaultProperties is ours, for that call (open: what the original's was)
// FUNCTION: GAME 0x56efd0
C_human::C_human()
{
    type = ACTOR_HUMAN;
    subtype = ACTORSUB_HUMAN;
    unknown_680 = 0xc;
    SetDefaultProperties();
}


// The model's first animation stopped; the members' destructors do the rest
// FUNCTION: GAME 0x56f210
C_human::~C_human()
{
    if (frame && frame->GetType() == FRAME_MODEL)
        ((I3D_model *)frame)->StopAnimation(0);
}


// A human's frame is a model: C_actor's, then the model's targets turned off (C_actor::Init has done so already) and
// its frame "base"
// FUNCTION: GAME 0x56f350
bool C_human::Init(I3D_frame *frm)
{
    if (frm && frm->GetType() != FRAME_MODEL)
        return false;
    if (!C_actor::Init(frm))
        return false;
    human_type = 0;
    target_e = frame->FindChildFrame("targetE", ENUMF_TARGET);
    target_n = frame->FindChildFrame("targetN", ENUMF_TARGET);
    if (target_e)
        target_e->SetOn(false);
    if (target_n)
        target_n->SetOn(false);
    // STRING: GAME 0x632444
    base_frame = frame->FindChildFrame("base", ENUMF_ALL);
    return true;
}


// FUNCTION: GAME 0x56f400
void C_human::DuplicateFrom(C_actor *act)
{
    C_actor::DuplicateFrom(act);
    C_human *human = (C_human *)act;
    memcpy(max_properties, human->max_properties, sizeof(max_properties));
    human_type = human->human_type;
}


// The original returns true (mov al, 1), as C_actor's does: the base's is void here
// FUNCTION: GAME 0x56f440
void C_human::SaveData(C_chunk *chunk)
{
    uchar version = HUMAN_DATA_VERSION;
    chunk->Write(&version, sizeof(version));
    chunk->Write(&human_type, sizeof(human_type));
    chunk->Write(&unknown_680, sizeof(unknown_680));
    chunk->Write(max_properties, sizeof(max_properties));
}


// The first versions' 16 floats: the second scaled the energies (1 / (1 - it), where it lies in (0, 1]) and went, the
// rest moved down by one, and the last, new, is 0.5
// FUNCTION: GAME 0x56f760
static void _ReadOldCharStruct(C_chunk *chunk, C_human *human)
{
    float old[16];
    chunk->Read(old, sizeof(old));
    float k = 1.0f;
    if (old[1] > 0.0f && old[1] <= 1.0f)
        k = 1.0f / (1.0f - old[1]);
    human->max_properties[HUMAN_PROPERTY_STRENGTH] = old[0];
    human->max_properties[HUMAN_PROPERTY_ENERGY] = old[2] * k;
    human->max_properties[HUMAN_PROPERTY_ENERGY_HAND_L] = old[3] * k;
    human->max_properties[HUMAN_PROPERTY_ENERGY_HAND_R] = old[4] * k;
    human->max_properties[HUMAN_PROPERTY_ENERGY_LEG_L] = old[5] * k;
    human->max_properties[HUMAN_PROPERTY_ENERGY_LEG_R] = old[6] * k;
    human->max_properties[HUMAN_PROPERTY_REACTIONS] = old[7];
    human->max_properties[HUMAN_PROPERTY_SPEED] = old[8];
    human->max_properties[HUMAN_PROPERTY_AGGRESIVITY] = old[9];
    human->max_properties[HUMAN_PROPERTY_INTELLIGENCE] = old[10];
    human->max_properties[HUMAN_PROPERTY_SHOOTING] = old[11];
    human->max_properties[HUMAN_PROPERTY_SIGHT] = old[12];
    human->max_properties[HUMAN_PROPERTY_HEARING] = old[13];
    human->max_properties[HUMAN_PROPERTY_DRIVING] = old[14];
    human->max_properties[HUMAN_PROPERTY_MASS] = old[15];
    human->max_properties[HUMAN_PROPERTY_15] = 0.5f;
}

// Version 4's 15 floats, the 16th 0.5
// FUNCTION: GAME 0x56f8c0
static void _ReadOldCharStructVer4(C_chunk *chunk, C_human *human)
{
    float old[15];
    chunk->Read(old, sizeof(old));
    human->max_properties[HUMAN_PROPERTY_STRENGTH] = old[0];
    human->max_properties[HUMAN_PROPERTY_ENERGY] = old[1];
    human->max_properties[HUMAN_PROPERTY_ENERGY_HAND_L] = old[2];
    human->max_properties[HUMAN_PROPERTY_ENERGY_HAND_R] = old[3];
    human->max_properties[HUMAN_PROPERTY_ENERGY_LEG_L] = old[4];
    human->max_properties[HUMAN_PROPERTY_ENERGY_LEG_R] = old[5];
    human->max_properties[HUMAN_PROPERTY_REACTIONS] = old[6];
    human->max_properties[HUMAN_PROPERTY_SPEED] = old[7];
    human->max_properties[HUMAN_PROPERTY_AGGRESIVITY] = old[8];
    human->max_properties[HUMAN_PROPERTY_INTELLIGENCE] = old[9];
    human->max_properties[HUMAN_PROPERTY_SHOOTING] = old[10];
    human->max_properties[HUMAN_PROPERTY_SIGHT] = old[11];
    human->max_properties[HUMAN_PROPERTY_HEARING] = old[12];
    human->max_properties[HUMAN_PROPERTY_DRIVING] = old[13];
    human->max_properties[HUMAN_PROPERTY_MASS] = old[14];
    human->max_properties[HUMAN_PROPERTY_15] = 0.5f;
}

// The versions SaveData has written: 1 had two more dwords first, 3 and 4 the type, 5 and 6 the properties as they
// are. The original returns true (mov al, 1) at each case's end, as C_actor's does: the base's is void here
// FUNCTION: GAME 0x56f4e0
void C_human::LoadData(C_chunk *chunk)
{
    char version;
    chunk->Read(&version, sizeof(version));
    switch (version)
    {
    case 1:
    {
        int unknown1, unknown2;
        chunk->Read(&unknown1, sizeof(unknown1));
        chunk->Read(&unknown2, sizeof(unknown2));
        _ReadOldCharStruct(chunk, this);
        return;
    }
    case 2:
        _ReadOldCharStruct(chunk, this);
        return;
    case 3:
        chunk->Read(&human_type, sizeof(human_type));
        _ReadOldCharStruct(chunk, this);
        return;
    case 4:
        chunk->Read(&human_type, sizeof(human_type));
        _ReadOldCharStructVer4(chunk, this);
        return;
    case 5:
        chunk->Read(&human_type, sizeof(human_type));
        chunk->Read(max_properties, sizeof(max_properties));
        return;
    case HUMAN_DATA_VERSION:
        chunk->Read(&human_type, sizeof(human_type));
        chunk->Read(&unknown_680, sizeof(unknown_680));
        chunk->Read(max_properties, sizeof(max_properties));
        return;
    }
}


// The sizes added one by one, as PS2's: one expression calls them in another order
// FUNCTION: GAME 0x56f9e0
int C_human::SaveGameGetSize()
{
    int size = C_actor::SaveGameGetSize() + sizeof(S_human_savegame);
    size += C_actor::Actor_SaveGameGetSize(car);
    size += C_actor::Actor_SaveGameGetSize(using_car);
    size += inventory.SaveGameGetSize();
    return size;
}


// C_actor's, the human's struct, then the two cars by their frames' names and the inventory
// FUNCTION: GAME 0x56fa20
bool C_human::SaveGameSave(void **buf)
{
    if (!C_actor::SaveGameSave(buf))
        return false;
    S_human_savegame *s = (S_human_savegame *)*buf;
    s->version = HUMAN_SAVEGAME_VERSION;
    s->pos = pos;
    s->dir = dir;
    s->anim_id = anim_id;
    s->anim_set = anim_set;
    s->seat = seat;
    s->crouching = crouching;
    s->aimed = aimed;
    s->anim_left = anim_left;
    s->unknown_1ec = unknown_1ec;
    s->unknown_200 = unknown_200;
    s->unknown_218 = unknown_218;
    s->unknown_21c = unknown_21c;
    s->unknown_220 = unknown_220;
    s->unknown_228 = unknown_228;
    s->unknown_22c = unknown_22c;
    s->unknown_22d = unknown_22d;
    s->tilt = tilt;
    s->unknown_2b0 = unknown_2b0;
    s->unknown_2b4 = unknown_2b4;
    s->unknown_2c0 = unknown_2c0;
    s->unknown_2cc = unknown_2cc;
    s->action_state = action_state;
    s->unknown_410 = unknown_410;
    s->fall = fall;
    s->unknown_420 = unknown_420;
    s->unknown_42c = unknown_42c;
    s->unknown_430 = unknown_430;
    s->unknown_43c = unknown_43c;
    s->unknown_440 = unknown_440;
    s->unknown_44c = unknown_44c;
    s->unknown_450 = unknown_450;
    s->agony_anim = agony_anim;
    s->unknown_45c = unknown_45c;
    s->unknown_460 = unknown_460;
    s->unknown_464 = unknown_464;
    s->unknown_468 = unknown_468;
    s->unknown_584 = unknown_584;
    s->idle_time = idle_time;
    s->breath_time = breath_time;
    memcpy(s->properties, properties, sizeof(properties));
    memcpy(s->max_properties, max_properties, sizeof(max_properties));
    s->ground_material = ground_material;
    s->wingman = wingman;
    s->wingman_name = wingman_name;
    s->bleed_count = bleed_count;
    s->bleed_time = bleed_time;
    s->unknown_6ec = unknown_6ec;
    s->unknown_6ee = unknown_6ee;
    s->unknown_6f0 = unknown_6f0;
    *buf = (uchar *)*buf + sizeof(S_human_savegame);
    C_actor::Actor_SaveGameSave(buf, car);
    C_actor::Actor_SaveGameSave(buf, using_car);
    inventory.SaveGameSave(buf);
    return true;
}


// The player's camera and the frame first, as they were before the load; then the struct (the animation's id into
// next_anim_id), the frame placed, the car got into again or the human put beside the one he was getting into, the
// inventory, the wingman's HUD, the death of a human loaded without energy, the collisions and the stolen cars
// FUNCTION: GAME 0x56fd30
bool C_human::SaveGameLoad(void **buf)
{
    if (!C_actor::SaveGameLoad(buf))
        return false;
    if (car)
    {
        if (act_mission->game->player == this)
        {
            switch (car->type)
            {
            case ACTOR_CAR:
                if (aimed)
                {
                    Do_Aimed(false);
                    ((C_player *)this)->unknown_af5 = false;
                    act_mission->game->camera.EnableFreeLook(false, false);
                }
                act_mission->game->camera.LezeDoToyoty(false);
                act_mission->game->camera.SetCar(NULL);
                break;
            case ACTOR_RAILWAY:
                act_mission->game->camera.Link(NULL);
                break;
            }
        }
        frame->LinkTo(act_mission->scene->primary_sector, 0);
    }
    S_human_savegame *s = (S_human_savegame *)*buf;
    if (s->version != HUMAN_SAVEGAME_VERSION)
        return false;
    *buf = (uchar *)*buf + sizeof(S_human_savegame);
    car = (C_car *)C_actor::Actor_SaveGameLoad(buf);
    using_car = (C_car *)C_actor::Actor_SaveGameLoad(buf);
    pos = s->pos;
    dir = s->dir;
    next_anim_id = s->anim_id;
    anim_id = 0;
    anim_set = s->anim_set;
    seat = s->seat;
    crouching = s->crouching;
    aimed = s->aimed;
    anim_left = s->anim_left;
    unknown_1ec = s->unknown_1ec;
    unknown_200 = s->unknown_200;
    unknown_218 = s->unknown_218;
    unknown_21c = s->unknown_21c;
    unknown_220 = s->unknown_220;
    unknown_228 = s->unknown_228;
    unknown_22c = s->unknown_22c;
    unknown_22d = s->unknown_22d;
    tilt = s->tilt;
    unknown_2b0 = s->unknown_2b0;
    unknown_2b4 = s->unknown_2b4;
    unknown_2c0 = s->unknown_2c0;
    unknown_2cc = s->unknown_2cc;
    action_state = s->action_state;
    unknown_410 = s->unknown_410;
    fall = s->fall;
    unknown_420 = s->unknown_420;
    unknown_42c = s->unknown_42c;
    unknown_430 = s->unknown_430;
    unknown_43c = s->unknown_43c;
    unknown_440 = s->unknown_440;
    unknown_44c = s->unknown_44c;
    unknown_450 = s->unknown_450;
    agony_anim = s->agony_anim;
    unknown_45c = s->unknown_45c;
    unknown_460 = s->unknown_460;
    unknown_464 = s->unknown_464;
    unknown_468 = s->unknown_468;
    unknown_584 = s->unknown_584;
    idle_time = s->idle_time;
    breath_time = s->breath_time;
    memcpy(properties, s->properties, sizeof(properties));
    memcpy(max_properties, s->max_properties, sizeof(max_properties));
    ground_material = s->ground_material;
    wingman = s->wingman;
    wingman_name = s->wingman_name;
    bleed_count = s->bleed_count;
    bleed_time = s->bleed_time;
    unknown_6ec = s->unknown_6ec;
    unknown_6ee = s->unknown_6ee;
    unknown_6f0 = s->unknown_6f0;
    unknown_414 = 0;
    frame->SetPos(s->pos);
    frame->SetDir(s->dir, 0.0f);
    frame->Update();
    PersonAnim(0.0f, true, false);
    if (car && car->type == ACTOR_CAR)
    {
        Intern_UseCar(car, seat);
        if (car->unknown_4a4 < 800.0f)
            mafiaIndicators.show_flags |= INDF_SPEED_LIMITER;
        else
            mafiaIndicators.show_flags &= ~INDF_SPEED_LIMITER;
    }
    else if (car && car->type == ACTOR_RAILWAY)
    {
        using_car = car;
        Intern_UseRailway(true);
        pos = s->pos;
        dir = s->dir;
        frame->SetPos(s->pos);
        frame->SetDir(s->dir, 0.0f);
        frame->Update();
    }
    else
        car = NULL;
    if (using_car && using_car->type == ACTOR_CAR)
    {
        C_car *c = using_car;
        if (unknown_410 == 1)
        {
            car = c;
            using_car = NULL;
            car->LockVehicle(false);
            car->OpenDoor(seat, 0.0f, true);
            Intern_UseCar(car, seat);
            if (car->unknown_4a4 < 800.0f)
                mafiaIndicators.show_flags |= INDF_SPEED_LIMITER;
            else
                mafiaIndicators.show_flags &= ~INDF_SPEED_LIMITER;
        }
        else
        {
            unknown_410 = 0;
            using_car = NULL;
            car = NULL;
            S_vector side(1.0f, 0.0f, 0.0f);
            side *= c->frame->GetRot().RotationMatrix();
            if (c->IsSeatLeft(seat))
                side *= -1.0f;
            pos += side;
            frame->SetPos(pos);
            frame->Update();
        }
    }
    inventory.SaveGameLoad(buf);
    ChangeWeaponModel();
    if (wingman > -1)
    {
        mafiaIndicators.PlayerSetWingmanName(wingman, wingman_name);
        switch (wingman)
        {
        case 0:
            mafiaIndicators.show_flags |= INDF_WINGMAN1;
            break;
        case 1:
            mafiaIndicators.show_flags |= INDF_WINGMAN2;
            break;
        case 2:
            mafiaIndicators.show_flags |= INDF_WINGMAN3;
            break;
        case 3:
            mafiaIndicators.show_flags |= INDF_WINGMAN4;
            break;
        }
        int lives = (int)(properties[HUMAN_PROPERTY_ENERGY] / max_properties[HUMAN_PROPERTY_ENERGY] * 100.0f);
        if (alive)
            lives = Max(lives, 1);
        mafiaIndicators.PlayerSetWingmanLives(wingman, lives);
    }
    if (!car && properties[HUMAN_PROPERTY_ENERGY] < .01f)
        _intern_ForceDeath();
    if (act_state == ACTSTATE_0 && !colls_active)
        ActivateColls(0);
    else if (act_state != ACTSTATE_0 && colls_active)
        DeactivateColls(false);
    if (car && car->type == ACTOR_CAR)
        mafiaLoadSave.CarIsStolen(car);
    if (using_car && using_car->type == ACTOR_CAR)
        mafiaLoadSave.CarIsStolen(using_car);
    return true;
}


// PS2's, without its fake shadow; the collisions go off for good (PS2 passes false)
// FUNCTION: GAME 0x570570
void C_human::Death()
{
    unknown_420.Zero();
    unknown_430.Zero();
    unknown_43c = 0.0f;
    act_mission->game->DelFrameForShadow(frame, SHADOW_TYPE_0);
    PoseNullRotate();
    DeactivateColls(true);
    anim_machine.DelExtendedAnim(4.0f);
    act_mission->game->shvestky.DelPerson(this);
    if (act_mission->game->player == this)
        act_mission->game->camera.SetMode(CAMERA_DEATH, true);
    if (base_frame)
        act_mission->game->LightnessDel(base_frame);
    if (locked_car)
    {
        locked_car->Lock(false);
        locked_car = NULL;
    }
    if (locked_vehicle)
    {
        locked_vehicle->LockVehicle(false);
        locked_vehicle = NULL;
    }
    dead = true;
}


// FUNCTION: GAME 0x570660
void C_human::Extern_AI_Update(uint time)
{
    if (anim_machine.IsActive())
        anim_machine.Tick(time);
    frame->Tick(time);
    UpdateDynColl();
}


// The human's sounds off, and the player's own one
// FUNCTION: GAME 0x5706a0
void C_human::SoundsChange(int i)
{
    if (unknown_6b8)
        unknown_6b8->SetOn(false, true);
    if (unknown_6c0)
        unknown_6c0->SetOn(false, true);
    if (unknown_6c4)
        unknown_6c4->SetOn(false, true);
    if (unknown_6c8)
        unknown_6c8->SetOn(false, true);
    if (voice)
        voice->SetOn(false, true);
    if (type == ACTOR_PLAYER && ((C_player *)this)->unknown_af8)
        ((C_player *)this)->unknown_af8->SetOn(false, true);
}

// C_HUMAN 0x570740 AI
// C_HUMAN 0x5716d0 Intern_UseCar(bool)
// C_HUMAN 0x571b90 Intern_UseRailway
// C_HUMAN 0x5721b0 _intern_UseCarTruck
// C_HUMAN 0x572530 ThrowFromCarTruck

// The human's frame of a tick: the AI and the shots alone while ACTF_UNKNOWN_1 or 4 stops the rest; else the carried
// body's animation, the logs of the positions and of the web, the walk, the turn, the movement, the animations and
// what ends them (the car's actions among them), the frame's placement in a car or on foot, the idle animations and
// the death's position
// FUNCTION: GAME 0x572b80
void C_human::Update(uint time)
{
    if (unknown_a58 > 0) {
        unknown_a58 -= time;
        return;
    }
    if (unknown_234 >= 0)
        unknown_234 -= time;
    if (act_flags & (ACTF_UNKNOWN_1 | ACTF_UNKNOWN_4)) {
        C_human::AI(time);
        if (base_frame)
            act_mission->scene->SetFrameSectorPos(frame, base_frame->GetWorldPos());
        if (unknown_1e8 > 0 && unknown_1e8 <= 3 && unknown_1ec)
            Shooting();
        return;
    }
    if (dead_body_human) {
        dead_body_human->Extern_AI_Update(time);
        if (!dead_body) {
            dead_body_time -= time;
            if (dead_body_time <= 0)
                dead_body_human = NULL;
        }
    }
    blob_log.Blob(this);
    loc.Update();

    // The walk speeds up and slows down
    if (action_state != 11) {
        if (!car && !using_car) {
            if (unknown_420.Square() < 1e-16f) {
                if (unknown_43c > 0.0f) {
                    unknown_43c -= time * .004f;
                    if (unknown_43c < 0.0f)
                        unknown_43c = 0.0f;
                    action_state = 2;
                }
            } else {
                unknown_430 = unknown_420;
                if (unknown_43c < 1.0f) {
                    unknown_43c += time * .004f;
                    if (unknown_43c > 1.0f)
                        unknown_43c = 1.0f;
                    unknown_420 *= unknown_43c;
                }
            }
        } else if (((car && car->type == ACTOR_CAR) || (using_car && using_car->type == ACTOR_CAR)) && seat != -1) {
            unknown_43c = 0.0f;
            unknown_420.x = 0.0f;
            unknown_420.y = 0.0f;
            unknown_420.z = 0.0f;
        }
    }

    // The turn
    if ((float)fabs(unknown_42c) > MRG_ZERO) {
        S_matrix m;
        m.Identity();
        float angle = time * unknown_42c * .001f;
        m(0, 0) = m(2, 2) = (float)cos(angle);
        m(2, 0) = -(m(0, 2) = (float)sin(angle));
        dir = dir.RotateByMatrix(m);
    }
    Movement(time);
    if (unknown_a60)
        TickWeaponEffect(time);
    if (unknown_a6c > 0) {
        unknown_a6c -= time;
        if (unknown_a6c <= 0) {
            I3D_animation_set *as = NULL;
            // STRING: GAME 0x636a5c
            anim_cache.Create("gun08 stoj Pump.i3d", &as, NULL, NULL);
            if (as) {
                anim_machine.SetExtendedAnim(as, 1.0f, 4.0f, true);
                as->Release();
            }
        }
    }
    if (!(act_flags & ACTF_UNKNOWN_1))
        PersonAnim(0.0f, true, false);

    // The animation's end
    if (anim_machine.IsActive()) {
        if (anim_machine.Tick(time) & 1) {
            if (unknown_23c[unknown_2a0].unknown_0 == 3) {
                if (dead) {
                    action_state = 8;
                    next_anim_id = 0;
                    anim_id = 0;
                } else {
                    if (anim_id != 0x68 && next_anim_id != 0x69)
                        unknown_22e = true;
                    if (!unknown_410) {
                        // Out of a car over its side: the police see the car stolen
                        if (act_mission->game->shvestky.active && anim_id >= 0x132 && anim_id <= 0x13b)
                            act_mission->game->shvestky.CarSteal(this, false, false);
                        if (alive) {
                            action_state = 1;
                            anim_id = 0;
                            next_anim_id = 1;
                            PersonAnim(0.0f, true, false);
                        }
                    } else {
                        switch (unknown_410) {
                        case 1:
                            Intern_UseCar(true);
                            break;
                        case 2:
                        case 5:
                            Intern_UseCar(false);
                            break;
                        case 3:
                            Intern_UseRailway(true);
                            break;
                        case 4:
                            Intern_UseRailway(false);
                            break;
                        case 6:
                            {
                                // Seated in the car
                                C_car *c = car;
                                next_anim_id = !seat ? (c->IsFormula() ? 0xf7 : 0xf6) : 0xf8;
                                PersonAnim(0.0f, true, false);
                                unknown_5e8 = 250;
                                unknown_5e4 = 0;
                                unknown_b0 = 0;
                                if (!seat) {
                                    if (type != ACTOR_PLAYER) {
                                        c->EnableSounds(false);
                                        c->SetEngineOn(true, false);
                                        c->EnableSounds(true);
                                    }
                                    c->SetSpeedLimit(speed_limiter ? 16 : 1000);
                                    if (act_mission->game->shvestky.active)
                                        act_mission->game->shvestky.DriveCar(this, c);
                                }
                                dir = S_vector(0.0f, 0.0f, 1.0f);
                                const S_vector &seat_pos = c->v_seats[seat].pos;
                                pos = seat_pos;
                                frame->SetPos(seat_pos);
                                frame->SetDir(dir, 0.0f);
                                frame->Update();
                                action_state = 9;
                                if (act_mission->game->player == this) {
                                    c->unknown_490 = act_mission->game->camera.unknown_22;
                                    c->SetSteeringLinearity(mafiaLoadSave.settings.steering_linearity);
                                    if (!seat) {
                                        mafiaIndicators.show_flags |= INDF_RADAR | INDF_TACHOMETR;
                                        if (speed_limiter)
                                            mafiaIndicators.show_flags |= INDF_SPEED_LIMITER;
                                        if (C_player::can_car_damage)
                                            mafiaIndicators.show_flags |= INDF_CAR_DAMAGE;
                                        mafiaIndicators.RadarSetPlayerCar(c);
                                        c->auto_gearing = !((C_player *)this)->unknown_adb;
                                    }
                                } else
                                    c->auto_gearing = true;
                                if (unknown_b4 > 0) {
                                    unknown_b8 = 250;
                                    unknown_b4 += 100;
                                }
                            }
                            break;
                        }
                        unknown_410 = 0;
                        using_car = NULL;
                    }
                }
            } else
                action_state = 6;
        }
    } else if (action_state == 4)
        action_state = 1;

    // The frame's animation, and in a car its shots' animations
    if (!(act_flags & ACTF_CUTSCENE)) {
        if (!car || car->type != ACTOR_CAR || seat == -1 || anim_machine.IsActive() || unknown_5e8 >= 0)
            frame->Tick(time);
        if (car && car->type == ACTOR_CAR && seat != -1 && IsReady()) {
            unknown_5e8 -= time;
            if (unknown_5e8 < 0 && (unknown_5e4 == 2 || unknown_5e4 == 0)) {
                if (unknown_5e4 == 2) {
                    next_anim_id = 0xfd;
                    if (!car->IsSeatLeft(seat))
                        next_anim_id += 3;
                    if (unknown_1e8 != 1)
                        next_anim_id += 6;
                    if (!seat)
                        next_anim_id = 0x109;
                } else {
                    next_anim_id = !seat ? (car->IsFormula() ? 0xf7 : 0xf6) : 0xf8;
                    if (weapon_model)
                        weapon_model->SetOn(false);
                }
                PersonAnim(0.0f, true, false);
                unknown_5f4 = .5f;
                uint anim_time, length;
                anim_machine.GetActiveAnimTime(anim_time, length);
                unknown_5ec = anim_machine.anim_index;
                unknown_5f0 = (float)length;
                frame->Tick(0);
            }
        }
    }

    // Seated in a car: the frame placed as he is
    if (car && car->type == ACTOR_CAR && seat != -1) {
        if (action_state == 4) {
            frame->SetPos(pos);
            dir.y = 0.0f;
            dir.Normalize();
            frame->SetDir(dir, 0.0f);
            frame->Update();
        }
        return;
    }

    // Bleeding
    if (bleed_count) {
        bleed_time -= time;
        if (bleed_time <= 0) {
            bleed_count--;
            bleed_time = rnd(2000) + 500;
            S_vector p = frame->GetWorldPos() + S_vector(0.0f, .5f, 0.0f);
            S_vector d(0.0f, -1.0f, .01f);
            if (act_mission->options.game_effects >= 2)
                act_mission->game->blood_manager.AddBlood(p, d, .5f, 0, false);
        }
    }
    if (look_frame)
        PoseSetPoseNormal(look_frame->GetWorldPos() + S_vector(0.0f, 1.7f, 0.0f));
    if (unknown_2b0)
        PoseInterpolate(time);

    // Moved by the animation's track: tested against the collisions
    if (unknown_464) {
        switch (unknown_464) {
        case 1:
            {
            S_vector d = pos - unknown_468;
            S_vector to;
            int hit;
            if (car)
                hit = TestMoveCollInOwner(unknown_468, d, to);
            else
                hit = TestMoveColl(unknown_468, d, to);
            if (hit) {
                S_vector off = to - (unknown_468 + d);
                off.y = 0.0f;
                anim_machine.unknown_114 += off;
            }
            pos = unknown_468 = to;
            }
            break;
        }
        if (anim_machine.track == -1 && anim_machine.unknown_80 == -1) {
            unknown_464 = 0;
            fall = .01f;
        }
    }

    // The weapon in hand
    switch (unknown_1e8) {
    case 1:
    case 2:
    case 3:
        if (unknown_1ec)
            Shooting();
        break;
    case 0:
    case 5:
    case 6:
    case 7:
        if ((!car || car->type != ACTOR_CAR || seat == -1) && aimed && !anim_machine.unknown_f4 && action_state != 5 &&
            !unknown_1ec) {
            I3D_animation_set *as = NULL;
            switch (unknown_1e8) {
            case 0:
            case 5:
                // STRING: GAME 0x636a44
                anim_cache.Create("boj HPT box kryt.i3d", &as, NULL, NULL);
                break;
            case 6:
                // STRING: GAME 0x636a2c
                anim_cache.Create("boj HPT nuz kryt.i3d", &as, NULL, NULL);
                break;
            case 7:
                // STRING: GAME 0x636a14
                anim_cache.Create("boj HPT basb kryt.i3d", &as, NULL, NULL);
                break;
            }
            if (as) {
                unknown_22c = true;
                anim_machine.SetExtendedAnim(as, .8f, 4.0f, false);
                as->Release();
            }
        }
        break;
    }

    // The frame placed
    if (!unknown_a36) {
        frame->SetPos(pos);
        dir.y = 0.0f;
        dir.Normalize();
        frame->SetDir(dir, 0.0f);
        frame->Update();
    } else if (anim_id >= 0x93 && anim_id <= 0xa2) {
        frame->SetPos(pos);
        if (anim_machine.track > -1 && C_animations_machine_de_luxe::tracks[anim_machine.track].has_directions)
            frame->SetDir(dir, 0.0f);
        frame->Update();
    }
    if (base_frame)
        act_mission->scene->SetFrameSectorPos(frame, base_frame->GetWorldPos());

    // The idle animations
    if (alive) {
        if (idle_time > 0 && !look_frame) {
            idle_time -= time;
            if (idle_time < 0 && idle_anims) {
                next_anim_id = 2;
                next_anim_id += rnd(3);
                PersonAnim(0.0f, true, false);
            }
        }
        if (breath_time >= 0)
            breath_time -= time;
    }
    UpdateDynColl();

    // The death's position
    if (dead) {
        if (action_state != 4 && (anim_id < 0x93 || anim_id > 0xa2)) {
            if (unknown_a36)
                ProcessDeathPos(time);
            else
                RecompileDeathPos();
        } else if (anim_id >= 0x93 && anim_id <= 0xa2)
            ProcessDeathPos(time);
    }
}


// The carried dead's animation by the carrier's (PS2's is empty)
// FUNCTION: GAME 0x573ce0
void C_human::ChangeDeadAnim()
{
    if (!dead_body || !dead_body_human)
        return;
    bool unknown = false;
    bool loop = true;
    // STRING: GAME 0x636b54
    const char *anim = "NoseniMrtvolyMrtStat.I3D";
    switch (next_anim_id & 0x1ff) {
    case 6:
    case 0x11:
    case 0x12:
        // STRING: GAME 0x636b38
        anim = "NoseniMrtvolyMrtWalk.I3D";
        break;
    case 7:
    case 0x13:
    case 0x14:
        // STRING: GAME 0x636b1c
        anim = "NoseniMrtvolyMrtBack.I3D";
        break;
    case 8:
        // STRING: GAME 0x636b00
        anim = "NoseniMrtvolyMrtLeft.I3D";
        break;
    case 9:
        // STRING: GAME 0x636ae4
        anim = "NoseniMrtvolyMrtRight.I3D";
        break;
    case 0xb:
        // STRING: GAME 0x636ac8
        anim = "NoseniMrtvolyMrtStrafL.I3D";
        break;
    case 0xc:
        // STRING: GAME 0x636aac
        anim = "NoseniMrtvolyMrtStrafR.I3D";
        break;
    case 0xa7:
        // STRING: GAME 0x636a8c
        anim = "NoseniMrtvolyMrtNahozeni.I3D";
        unknown = true;
        loop = false;
        break;
    case 0xa8:
        // STRING: GAME 0x636a70
        anim = "NoseniMrtvolyMrtShozeni.I3D";
        loop = false;
        break;
    }
    dead_body_human->Do_PlayAnim(anim, !loop, unknown);
}


// Plays next_anim_id unless it is playing: in the stance of the weapon and the crouch, in the set's animations
// (anim_set), the replaced animation (anims) or the table's, with its idle and breath times, its track's move tested
// against the collisions (unknown_464) and the jumps' landing height
// FUNCTION: GAME 0x573e50
void C_human::PersonAnim(float f, bool b1, bool b2)
{
    static char buff[128];
    uint add = 0;
    next_anim_id &= 0x1ff;
    bool walk = next_anim_id >= 5 && next_anim_id <= 0x14;
    if (next_anim_id <= 0x5d && b1) {
        if (crouching) {
            if (unknown_1e8 == 2 || unknown_1e8 == 3)
                add = 0x8000;
            else if (unknown_1e8 == 1 && aimed)
                add = 0x4000;
            else
                add = 0x2000;
        } else if (unknown_1e8 == 2)
            add = aimed ? 0x400 : 0x1000;
        else if (unknown_1e8 == 3)
            add = aimed ? 0x800 : 0x1000;
        else
            add = unknown_1e8 == 1 && aimed ? 0x200 : 0;
    }
    bool idle = next_anim_id == 1;
    bool breath = next_anim_id >= 2 && next_anim_id <= 4;
    if (!dead_body && !box)
        next_anim_id |= add;
    if (breath_time > -1 && anim_id > next_anim_id && anim_id - next_anim_id < 4)
        next_anim_id = anim_id;
    if (anim_set) {
        uint id = next_anim_id & 0x1ff;
        switch (anim_set) {
        case HUMAN_ANIMSET_CALM:
            if (add < 0x2000) {
                if (add == 0x200)
                    add = 0;
                else if (add == 0x1000)
                    add = 0x800;
                switch (id) {
                case 1:
                    next_anim_id = add | 0x15;
                    break;
                case 2:
                    next_anim_id = add | 0x16;
                    break;
                case 3:
                case 4:
                    next_anim_id = add | 0x17;
                    break;
                case 6:
                    next_anim_id = add | 0x18;
                    break;
                case 7:
                    next_anim_id = add | 0x19;
                    break;
                case 8:
                    next_anim_id = add | 0x1a;
                    break;
                case 9:
                    next_anim_id = add | 0x1b;
                    break;
                case 0xb:
                    next_anim_id = add | 0x1c;
                    break;
                case 0xc:
                    next_anim_id = add | 0x1d;
                    break;
                case 0x11:
                    next_anim_id = add | 0x1e;
                    break;
                case 0x12:
                    next_anim_id = add | 0x1f;
                    break;
                case 0x13:
                    next_anim_id = add | 0x20;
                    break;
                case 0x14:
                    next_anim_id = add | 0x21;
                    break;
                }
            }
            break;
        case HUMAN_ANIMSET_2:
            if (add > 0 && add < 0x1000) {
                switch (id) {
                case 1:
                    next_anim_id = add | 0x22;
                    break;
                case 2:
                    next_anim_id = add | 0x23;
                    break;
                case 3:
                case 4:
                    next_anim_id = add | 0x24;
                    break;
                case 6:
                    next_anim_id = add | 0x25;
                    break;
                case 7:
                    next_anim_id = add | 0x26;
                    break;
                case 8:
                    next_anim_id = add | 0x27;
                    break;
                case 9:
                    next_anim_id = add | 0x28;
                    break;
                case 0xb:
                    next_anim_id = add | 0x29;
                    break;
                case 0xc:
                    next_anim_id = add | 0x2a;
                    break;
                case 0x11:
                    next_anim_id = add | 0x2b;
                    break;
                case 0x12:
                    next_anim_id = add | 0x2c;
                    break;
                case 0x13:
                    next_anim_id = add | 0x2d;
                    break;
                case 0x14:
                    next_anim_id = add | 0x2e;
                    break;
                }
            }
            break;
        case HUMAN_ANIMSET_PANIC:
            {
                uint cur = anim_id & 0x1ff;
                switch (id) {
                case 1:
                case 2:
                case 3:
                case 4:
                    // The panic's stances stay, its downs go on into them
                    if (cur == 0x3e || cur == 0x41 || cur == 0x44)
                        next_anim_id = cur;
                    else if (cur == 0x3d)
                        next_anim_id = 0x3e;
                    else if (cur == 0x40)
                        next_anim_id = 0x41;
                    else if (cur == 0x43)
                        next_anim_id = 0x44;
                    break;
                case 7:
                    next_anim_id = 0x4a;
                    break;
                case 5:
                    // A run of the panic's, the one running kept
                    switch (cur) {
                    case 0x46:
                        next_anim_id = cur;
                        break;
                    case 0x48:
                        next_anim_id = cur;
                        break;
                    default:
                        next_anim_id = rnd(2) * 2 + 0x46;
                        break;
                    }
                    break;
                }
            }
            break;
        }
    }
    if (next_anim_id == anim_id)
        return;

    if (unknown_580)
        RepairWeaponPosition();
    // Out of the wreck: the player's camera on foot again
    if (act_mission->game->player == this && anim_id == 0x12f) {
        act_mission->game->camera.PopMode(false);
        act_mission->game->camera.SetCar(NULL);
    }
    if (unknown_a35) {
        unknown_a35 = false;
        ActivateColls(2);
    }
    if (idle)
        idle_time = rnd(5000) + 5000;
    else
        idle_time = -1;
    if (!breath)
        breath_time = -1;
    if (walk) {
        if (unknown_94 & 1) {
            unknown_94 &= ~1;
            f = .5f;
        } else
            unknown_94 |= 1;
    }
    anim_id = next_anim_id;
    if (dead_body_human)
        ChangeDeadAnim();

    // The animation's file: the replaced one, else the table's
    S_anim_info *af = NULL;
    int i = anims.size();
    while (i--) {
        if (anims[i].id == anim_id) {
            af = &anims[i];
            break;
        }
    }
    if (!af) {
        int index = GetAnimIndex(anim_id);
        if (index > -1)
            af = &_anim_info_[index];
    }
    if (!af)
        return;
    strcpy(buff, af->name);
    char *side = strchr(buff, '?');
    if (side)
        *side = anim_left ? 'L' : 'R';
    if (next_anim_id == 0x68 || next_anim_id == 0x69) {
        // Onto the box, of the height and length Do_Climb found
        S_vector scale(unknown_90, unknown_8c, 1.0f);
        anim_machine.SetAnim(buff, false, false, b2, 4.0f, f, true, false, &scale, true, (uint)-1);
    } else if (next_anim_id == 0x130 || next_anim_id == 0x131) {
        // Over the car
        S_vector scale(unknown_8c, 1.0f, 1.0f);
        anim_machine.SetAnim(buff, false, false, b2, 4.0f, f, true, false, &scale, true, (uint)-1);
    } else
        anim_machine.SetAnim(buff, false, false, b2, 4.0f, f, true, false, NULL, true, anim_id);

    if (anim_machine.track != -1) {
        // A track moves the human
        unknown_43c = 0.0f;
        unknown_420.Zero();
        action_state = 4;
        anim_machine.DelExtendedAnim(4.0f);
        unknown_464 = 0;
        uint id = anim_id & 0x1ff;
        if ((id < 0xf6 || id > 0x13f) && id != 0x140 && id != 0x141 && id != 0x68 && id != 0x69 && id != 0x130 &&
            id != 0x131) {
            unknown_464 = 1;
            unknown_468 = pos;
        }
        if ((id >= 0x5e && id <= 0x60) || (id >= 0x6a && id <= 0x72) || (id >= 0x62 && id <= 0x65)) {
            // The jumps and falls: the landing's height by the track's end
            char *ext = strrchr(buff, '.');
            if (ext) {
                *ext = 0;
                strcat(buff, ".tck");
            }
            S_vector end, d;
            if (C_animations_machine_de_luxe::GetPlacementFromTCK(buff, end, d)) {
                end *= -1.0f;
                end *= frame->GetMatrix();
                float lift = 0.0f;
                d = end - pos;
                S_vector hit, normal;
                if (gifCollision.TestLineH(pos + S_vector(0.0f, .2f, 0.0f), d, &hit, &normal, 2, 0)) {
                    hit -= pos;
                    float len = d.Magnitude();
                    float len_hit = hit.Magnitude();
                    S_vector up(0.0f, 1.0f, 0.0f);
                    float angle = normal.AngleTo(up);
                    if (angle < PI / 4) {
                        float h = (float)tan(angle) * (len - len_hit);
                        if (h > 4.0f)
                            h = 4.0f;
                        end.y += h + .2f;
                        lift = h + .2f;
                    }
                }
                anim_machine.SetLandHeight(_get_delta_y_(end) + lift);
            }
        }
    } else if (action_state == 4)
        action_state = 1;
    if (breath) {
        uint time, length;
        anim_machine.GetActiveAnimTime(time, length);
        breath_time = length;
        if (breath_time > 280)
            breath_time -= 280;
    }
}


// Ticks the animation and the frame to the time of the animation's "notify" note of that value (PS2 name), which
// Use_Actor calls
// FUNCTION: GAME 0x5747d0
void C_human::ShiftAnimation(int note)
{
    S_anim_info *af = NULL;
    int index = GetAnimIndex(anim_id);
    if (index > -1)
        af = &_anim_info_[index];
    if (!af)
        return;
    char name[128];
    strcpy(name, af->name);
    char *side = strchr(name, '?');
    if (side)
        *side = anim_left ? 'L' : 'R';
    I3D_animation_set *as = NULL;
    anim_cache.Create(name, &as, NULL, NULL);
    if (!as)
        return;
    // STRING: GAME 0x63244c
    int link = as->GetAnimIndex("notify");
    int num = as->NumNoteKeys(link, NULL);
    for (int i = 0; i < num; i++) {
        int time;
        short value;
        as->GetNoteKey(link, i, time, value);
        if (value == note) {
            anim_machine.Tick(time);
            frame->Tick(time);
        }
    }
    as->Release();
}


// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_human::NetDirect(uint time)
{
}


// The fire's steps when the weapon is ready (PS2 name; PS2's _fire_action_aimed_, _onoff_ and _wait_ follow it)
// GLOBAL: GAME 0x6368d0
static S_fire_action _fire_action_ready_[] = {{FIRE_ACTION_END, 0}};

// FUNCTION: GAME 0x5748f0
void C_human::ChangeState()
{
    switch (act_state)
    {
    case ACTSTATE_0:
        ActivateColls(0);
        if (type == ACTOR_ENTITY && ((C_entity *)this)->IsEventID(EVENT_USE_AB, NULL) && ab_created)
            act_mission->GetGame()->GetUsingObject().AddObject(&((C_entity *)this)->using_object);
        if (!shadow && !car)
            shadow = act_mission->GetGame()->AddFrameForShadow(frame, SHADOW_TYPE_0, SHADOW_TYPE_0, NULL, NULL);
        break;
    case ACTSTATE_2:
        if (dead)
        {
            if (!car && !using_car)
            {
                // The animations played to their ends
                I3D_animation_set *set = ((I3D_model *)frame)->GetAnimationSet(0);
                if (set)
                {
                    uint end = set->GetEndTime();
                    uint time = ((I3D_model *)frame)->GetAnimTime(0);
                    if (((I3D_model *)frame)->GetAnimBlend(0) > .5f && time < end)
                        frame->Tick(end - time);
                }
                set = ((I3D_model *)frame)->GetAnimationSet(1);
                if (set)
                {
                    uint end = set->GetEndTime();
                    uint time = ((I3D_model *)frame)->GetAnimTime(1);
                    if (((I3D_model *)frame)->GetAnimBlend(1) > .5f && time < end)
                        frame->Tick(end - time);
                }
            }
            // STRING: GAME 0x63270c
            I3D_frame *back = frame->FindChildFrame("back1", ENUMF_ALL);
            if (back)
            {
                using_objects[0].pos = back->GetWorldPos();
                using_objects[0].pos.y += .5f;
                using_objects[0].unknown_c = .7f;
                using_objects[0].unknown_10 = 0;
                using_objects[0].flags = 0;
                using_objects[0].frame = NULL;
                using_objects[0].actor = this;
                using_objects[0].item.id = GAMEITEM_UNKNOWN_1;
                using_objects[0].item.ammo_loaded = 0x7dc;
                using_objects[0].item.using_object = &using_objects[0];
                if (!car)
                    act_mission->GetGame()->GetUsingObject().AddObject(&using_objects[0]);
                using_objects[1].pos = back->GetWorldPos();
                using_objects[1].pos.y += .5f;
                using_objects[1].unknown_c = .7f;
                using_objects[1].unknown_10 = 0;
                using_objects[1].flags = 0;
                using_objects[1].frame = NULL;
                using_objects[1].actor = this;
                using_objects[1].item.id = GAMEITEM_UNKNOWN_1;
                using_objects[1].item.ammo_loaded = 0x7dd;
                using_objects[1].item.using_object = &using_objects[1];
            }
            PoseValidate(false);
        }
    case ACTSTATE_1:
        DeactivateColls(dead);
        if (type == ACTOR_ENTITY)
            act_mission->GetGame()->GetUsingObject().DelObject(&((C_entity *)this)->using_object);
        if (shadow)
        {
            act_mission->GetGame()->DelFrameForShadow(frame, SHADOW_TYPE_0);
            shadow = 0;
        }
        break;
    }
}


// FUNCTION: GAME 0x574bd0
void C_human::GameInit()
{
    orig_model = NULL;
    C_actor::GameInit();
    base_frame = frame->FindChildFrame("base", ENUMF_ALL);
    if (frame)
    {
        frame->m_RefCount++;
        act_mission->GetScene()->DeleteFrame(frame);
    }
    DebustizeFrame(frame);
    NoShadowFrame(frame);
    car = NULL;
    using_car = NULL;
    action_state = 1;
    unknown_410 = 0;
    anim_machine.Init(this);
    next_anim_id = 1;
    memcpy(properties, max_properties, sizeof(properties));
    anim_id = 0;
    anim_set = 0;
    unknown_420.Zero();
    unknown_430 = S_vector(0.0f, 0.0f, 0.0f);
    unknown_42c = 0;
    fall = 0.0f;
    unknown_43c = 0.0f;
    unknown_22c = false;
    idle_time = -1;
    breath_time = -1;
    unknown_5e0 = false;
    unknown_22d = false;
    unknown_450 = 500;
    agony_anim = -1;
    wingman = -1;
    unknown_460 = 0;
    fire_target = NULL;
    unknown_464 = 0;
    bleed_count = 0;
    unknown_6ee = 0;
    traffic = NULL;
    box = NULL;
    unknown_94 = 0;
    unknown_70c = 0;
    unknown_710 = pos;
    unknown_71c = 1.0f;
    unknown_6bc = 0;
    unknown_414 = 0;
    no_anim_hit = false;
    unknown_1ed = false;
    unknown_1fc = false;
    unknown_1fd = false;
    unknown_1fe = false;
    last_hit_time = 0;
    last_hit_render = 0;
    idle_anims = false;
    unknown_22e = true;
    unknown_6d0 = -1;
    dead_body = NULL;
    dead_body_human = 0;
    dead_body_time = -1;
    unknown_a35 = false;
    killed_by_shot = false;
    unknown_a36 = false;
    stealing_car = NULL;
    unknown_a58 = -1;
    unknown_a60 = 0;
    unknown_a6c = -1;
    unknown_a70 = NULL;
    killer = NULL;
    unknown_22f = false;
    unknown_230 = false;
    unknown_9a0 = 1;
    unknown_b0 = 0;
    unknown_b4 = 0;
    unknown_b8 = -1;
    speed_limiter = false;
    unknown_234 = -1;
    score_bonus = false;
    unknown_a7a = false;
    unknown_a0 = 0;
    unknown_580 = false;
    unknown_581 = false;
    shut_up = false;
    ab_created = true;
    unknown_220 = 0;
    locked_car = NULL;
    locked_vehicle = NULL;
    unknown_9ac = 1.0f;
    unknown_9b0 = 1.0f;
    unknown_9b4 = 1.0f;
    unknown_9b8 = 1.0f;
    unknown_9c0 = 5000;
    unknown_5e4 = 0;
    unknown_5e8 = 0;
    unknown_5ec = 0;
    unknown_5f0 = 0;
    unknown_5f4 = .5f;
    unknown_2b0 = false;
    pose_back1 = driver->CreatePose();
    ((I3D_model *)frame)->SetPose("back1", pose_back1);
    pose_back2 = driver->CreatePose();
    // STRING: GAME 0x6369d4
    ((I3D_model *)frame)->SetPose("back2", pose_back2);
    pose_neck = driver->CreatePose();
    ((I3D_model *)frame)->SetPose("neck", pose_neck);
    look_frame = NULL;
    unknown_2a0 = 0;
    unknown_23c[0].unknown_0 = 3;
    CreateDynColls();

    unknown_6b8 = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
    if (unknown_6b8)
    {
        unknown_6b8->SetOn(false);
        if (act_mission->GetGame()->GetPlayer() == this)
            unknown_6b8->SetPos(S_vector(0.0f, 1.5f, 0.0f));
        else
            unknown_6b8->SetPos(S_vector(0.0f, .1f, 0.0f));
        unknown_6b8->LinkTo(frame, 0);
        unknown_6b8->Update();
    }
    unknown_6c0 = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
    if (unknown_6c0)
    {
        unknown_6c0->SetOn(false);
        unknown_6c0->SetPos(S_vector(0.0f, 1.0f, 0.0f));
        unknown_6c0->LinkTo(frame, 0);
        unknown_6c0->Update();
        unknown_6c0->priority = 0x60;
    }
    unknown_6c4 = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
    if (unknown_6c4)
    {
        unknown_6c4->SetOn(false);
        unknown_6c4->SetPos(S_vector(0.0f, 1.0f, 0.0f));
        unknown_6c4->LinkTo(frame, 0);
        unknown_6c4->Update();
    }
    unknown_6c8 = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
    if (unknown_6c8)
    {
        unknown_6c8->SetOn(false);
        unknown_6c8->SetPos(S_vector(0.0f, 1.0f, 0.0f));
        unknown_6c8->LinkTo(frame, 0);
        unknown_6c8->Update();
    }
    voice = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
    if (voice)
    {
        voice->SetLoop(false);
        voice->SetOn(false);
        voice->SetPos(S_vector(0.0f, 1.7f, 0.0f));
        voice->LinkTo(frame, 0);
        voice->Update();
        voice->SetSoundType(I3DSOUND_POINT);
        voice->SetRange(7.0f, 15.0f, 1.0f, 0.0f);
        voice->SetVolume(mafiaLoadSave.settings.speech_volume / (mafiaLoadSave.settings.sound_volume + .01f) * .95f +
            .05f);
        voice->SetOutVol(0.0f);
    }

    if (frame)
        shot_skeleton.Init(frame);
    I3D_frame *notify = frame->FindChildFrame("notify", ENUMF_ALL);
    if (notify)
        notify->SetCallback(Unknown56d130, LS3D_CALLBACK_NOTE);
    for (int i = anims.size(); i--;)
        delete[] anims[i].name;
    anims.clear();
    aimed = false;
    unknown_21c = false;
    unknown_1ec = false;
    crouching = false;
    unknown_228 = 0;
    unknown_1e8 = 0;
    ground_material = 0;
    tilt = .5f;
    shadow = act_mission->GetGame()->AddFrameForShadow(frame, SHADOW_TYPE_0, SHADOW_TYPE_0, NULL, NULL);
    unknown_a88 = 0;

    weapon_model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
    if (weapon_model)
        // STRING: GAME 0x636b80
        weapon_model->SetName("weapon");
    weapon_model2 = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
    if (weapon_model2)
        weapon_model2->SetName("weapon");
    // STRING: GAME 0x636b78
    right_hand = frame->FindChildFrame("gun1", ENUMF_ALL);
    // STRING: GAME 0x636b70
    left_hand = frame->FindChildFrame("gun2", ENUMF_ALL);
    if (!right_hand)
        // STRING: GAME 0x632740
        right_hand = frame->FindChildFrame("r_hand", ENUMF_ALL);
    if (!left_hand)
        // STRING: GAME 0x634488
        left_hand = frame->FindChildFrame("l_hand", ENUMF_ALL);
    if (left_hand)
    {
        left_hand_target = driver->CreateFrame(FRAME_TARGET);
        left_hand_target->SetPos(S_vector(0.0f, 0.0f, 0.0f));
        left_hand_target->LinkTo(left_hand, 0);
        left_hand_target->Update();
    }
    else
        left_hand_target = NULL;
    if (right_hand)
    {
        right_hand_target = driver->CreateFrame(FRAME_TARGET);
        right_hand_target->SetPos(S_vector(0.0f, 0.0f, 0.0f));
        right_hand_target->LinkTo(right_hand, 0);
        right_hand_target->Update();
    }
    else
        right_hand_target = NULL;

    fire_action = _fire_action_ready_;
    fire_step = 0;
    fire_time = 0;
    inventory.Clear();
    inventory.SetCanHaveBigWeapon(true, NULL);
    ChangeWeaponModel();
    PoseValidate(true);
    // The human put on the ground below him, his own collisions left out
    gifCollision.DynSetNextCollOwnerMult(colls, num_colls, NULL);
    S_vector ground;
    g_collision_header *coll = (g_collision_header *)gifCollision.TestLineV(S_vector(pos.x, pos.y + 1.0f, pos.z),
        S_vector(0.0f, -5.0f, 0.0f), &ground, NULL, COLF_UNKNOWN_2, 0);
    gifCollision.DynSetNextCollOwner(NULL, NULL);
    if (coll)
    {
        pos = ground;
        ground_material = coll->material;
    }
    if (base_frame)
        act_mission->GetGame()->LightnessAdd(this, base_frame, 0, S_vector(0.0f, 1.0f, 0.0f));
    loc.Init(this);
    blob_log.Clear();
    unknown_a99 = true;
    unknown_a98 = false;
    unknown_a8c = 1.0f;
    unknown_a90 = 0;
    unknown_a9a = false;
}


// FUNCTION: GAME 0x5757c0
void C_human::GameDone()
{
    // Out of the truck's collisions
    if (car && car->type == ACTOR_CAR && car->subtype == ACTORSUB_CAR_EX && seat == -1)
    {
        for (int i = num_colls; i--;)
        {
            if (colls[i])
                gifCollision.DynGetOut(colls[i], ((C_car_ex *)car)->dyn_colls);
        }
    }
    if (orig_model)
    {
        _intern_ChangeModel(orig_model, true);
        delete[] orig_model;
        orig_model = NULL;
    }
    HideLegs((I3D_model *)frame, false);
    if (unknown_a70)
    {
        unknown_a70->LinkTo(NULL, 0);
        unknown_a70->Release();
        unknown_a70 = NULL;
    }
    act_mission->GetGame()->GetUsingObject().DelObject(&using_objects[0]);
    act_mission->GetGame()->GetUsingObject().DelObject(&using_objects[1]);
    // The placement before C_actor::GameDone puts the frame back, for the pedestrian
    S_vector old_pos = pos;
    S_vector old_dir = dir;
    C_actor::GameDone();
    if (base_frame)
        act_mission->GetGame()->LightnessDel(base_frame);
    act_mission->GetGame()->DelFrameForShadow(frame, SHADOW_TYPE_0);
    anim_machine.StopAnims();
    if (frame)
    {
        frame->SetOn(true);
        if (!traffic || !traffic->traffic_frame)
            act_mission->GetScene()->AddFrame(frame);
        frame->Release();
    }
    if (frame)
    {
        if (pose_back1)
        {
            ((I3D_model *)frame)->SetPose("back1", NULL);
            pose_back1->Release();
            pose_back1 = NULL;
        }
        if (pose_back2)
        {
            ((I3D_model *)frame)->SetPose("back2", NULL);
            pose_back2->Release();
            pose_back2 = NULL;
        }
        if (pose_neck)
        {
            ((I3D_model *)frame)->SetPose("neck", NULL);
            pose_neck->Release();
            pose_neck = NULL;
        }
    }
    EraseDynColls();
    if (unknown_6b8)
    {
        unknown_6b8->SetOn(false);
        unknown_6b8->Release();
        unknown_6b8 = NULL;
    }
    if (unknown_6c0)
    {
        unknown_6c0->SetOn(false);
        unknown_6c0->Release();
        unknown_6c0 = NULL;
    }
    if (unknown_6c4)
    {
        unknown_6c4->SetOn(false);
        unknown_6c4->Release();
        unknown_6c4 = NULL;
    }
    if (unknown_6c8)
    {
        unknown_6c8->SetOn(false);
        unknown_6c8->Release();
        unknown_6c8 = NULL;
    }
    if (voice)
    {
        voice->SetOn(false);
        voice->Release();
        voice = NULL;
    }
    I3D_frame *notify = frame->FindChildFrame("notify", ENUMF_ALL);
    if (notify)
        notify->SetCallback(NULL, LS3D_CALLBACK_NOTE);
    if (weapon_model)
    {
        weapon_model->LinkTo(NULL, 0);
        weapon_model->Release();
        weapon_model = NULL;
    }
    if (weapon_model2)
    {
        weapon_model2->LinkTo(NULL, 0);
        weapon_model2->Release();
        weapon_model2 = NULL;
    }
    if (left_hand_target)
    {
        left_hand_target->Release();
        left_hand_target = NULL;
    }
    if (right_hand_target)
    {
        right_hand_target->Release();
        right_hand_target = NULL;
    }
    if (target_e)
    {
        target_e->SetOn(false);
        target_e->LinkTo(frame, 0);
    }
    if (target_n)
    {
        target_n->SetOn(false);
        target_n->LinkTo(frame, 0);
    }
    for (int i = anims.size(); i--;)
        delete[] anims[i].name;
    anims.clear();
    // The pedestrian the human was made of back at his place
    if (traffic)
    {
        if (!act_mission->GetGame()->game_done && traffic->element)
        {
            traffic->element->unknown_1a5 = false;
            traffic->element->frame->SetPos(old_pos);
            traffic->element->frame->SetDir(old_dir, 0.0f);
            traffic->element->frame->Update();
            traffic->element->ActivateFree(true, false);
        }
        if (traffic->traffic_frame)
        {
            frame->Release();
            frame = NULL;
        }
        delete traffic;
        traffic = NULL;
    }
}


// FUNCTION: GAME 0x575ca0
void C_human::CreateDynColls()
{
    num_colls = 2;
    colls = new tDynamicCollObject *[num_colls];
    memset(colls, 0, num_colls * sizeof(tDynamicCollObject *));
    coll_frames = new I3D_frame *[num_colls];
    // The lower sphere, at the model's root
    coll_frames[0] = frame;
    colls[0] = new tDynamicCollObject;
    memset(colls[0], 0, sizeof(tDynamicCollObject));
    colls[0]->type = DYNCOLL_UNKNOWN_42;
    colls[0]->flags = DYNCOLLF_UNKNOWN_100 | DYNCOLLF_LOWER;
    ((I3D_bsphere &)colls[0]->bsphere).pos = coll_frames[0]->GetWorldPos();
    ((I3D_bsphere &)colls[0]->bsphere).radius = .3f;
    colls[0]->frame = frame;
    colls[0]->owner = this;
    gifCollision.DynInsert(colls[0]);
    // The upper one, at "base"
    coll_frames[1] = frame->FindChildFrame("base", ENUMF_ALL);
    if (!coll_frames[1])
    {
        EraseDynColls();
        return;
    }
    colls[1] = new tDynamicCollObject;
    memset(colls[1], 0, sizeof(tDynamicCollObject));
    colls[1]->type = DYNCOLL_UNKNOWN_42;
    colls[1]->flags = DYNCOLLF_UNKNOWN_100 | DYNCOLLF_UPPER;
    ((I3D_bsphere &)colls[1]->bsphere).pos = coll_frames[1]->GetWorldPos();
    ((I3D_bsphere &)colls[1]->bsphere).radius = 1.2f;
    colls[1]->frame = frame;
    colls[1]->owner = this;
    gifCollision.DynInsert(colls[1]);
    colls_active = true;
}


// FUNCTION: GAME 0x575ed0
void C_human::EraseDynColls()
{
    colls_active = false;
    for (int i = num_colls; i--;)
    {
        if (colls[i])
        {
            gifCollision.DynErase(colls[i]);
            delete colls[i];
        }
    }
    num_colls = 0;
    if (colls)
        delete[] colls;
    colls = NULL;
    if (coll_frames)
        delete[] coll_frames;
    coll_frames = NULL;
}


// FUNCTION: GAME 0x575f60
void C_human::DeactivateColls(bool keep_upper)
{
    colls_active = false;
    for (int i = num_colls; i--;)
    {
        if (colls[i])
        {
            if (keep_upper && (colls[i]->flags & DYNCOLLF_UPPER))
                continue;
            gifCollision.DynErase(colls[i]);
            act_mission->GetGame()->InvalidateColl(colls[i]);
        }
    }
}


// FUNCTION: GAME 0x575fc0
void C_human::ActivateColls(int mode)
{
    if (car)
        return;
    colls_active = true;
    for (int i = num_colls; i--;)
    {
        if (!colls[i])
            continue;
        switch (mode)
        {
        case 0:
            if ((colls[i]->flags & DYNCOLLF_UPPER) || alive)
                gifCollision.DynInsert(colls[i]);
            break;
        case 1:
            if ((colls[i]->flags & DYNCOLLF_UPPER) && alive)
                gifCollision.DynInsert(colls[i]);
            break;
        case 2:
            if ((colls[i]->flags & DYNCOLLF_LOWER) && alive)
                gifCollision.DynInsert(colls[i]);
            break;
        }
    }
}


// The spheres' heights above their frames (PS2 name, a file static)
// GLOBAL: GAME 0x6368fc
static float dyn_colls_offset[2] = {.8f, -.3f};

// FUNCTION: GAME 0x576040
void C_human::UpdateDynColl()
{
    if (colls_active)
    {
        if (!car)
        {
            for (int i = num_colls; i--;)
            {
                if (!colls[i])
                    continue;
                if (unknown_a35 && (colls[i]->flags & DYNCOLLF_LOWER))
                    continue;
                ((I3D_bsphere &)colls[i]->bsphere).pos = coll_frames[i]->GetWorldPos();
                ((I3D_bsphere &)colls[i]->bsphere).pos.y += dyn_colls_offset[i];
                gifCollision.DynUpdate(colls[i]);
            }
        }
        else if (car->type == ACTOR_RAILWAY || (car->type == ACTOR_CAR && seat == -1))
        {
            // On the train or the truck the spheres stay in the vehicle's space
            S_matrix m = frame->GetParent()->GetMatrix();
            m.Inverse(S_matrix(m));
            for (int i = num_colls; i--;)
            {
                if (!colls[i])
                    continue;
                ((I3D_bsphere &)colls[i]->bsphere).pos = coll_frames[i]->GetWorldPos();
                ((I3D_bsphere &)colls[i]->bsphere).pos.y += dyn_colls_offset[i];
                ((I3D_bsphere &)colls[i]->bsphere).pos *= m;
            }
        }
    }
    else if (!car)
    {
        // A dead human's upper sphere
        for (int i = num_colls; i--;)
        {
            if (colls[i] && dead && (colls[i]->flags & DYNCOLLF_UPPER))
            {
                ((I3D_bsphere &)colls[i]->bsphere).pos = coll_frames[i]->GetWorldPos();
                ((I3D_bsphere &)colls[i]->bsphere).pos.y += dyn_colls_offset[i];
                gifCollision.DynUpdate(colls[i]);
            }
        }
    }
}


// The human force feedback effect of a hit, which InitFFEffects creates (0x647404; the name is ours)
static int ff_hit;

// The human hit (PS2 name; the parameters' names are ours): false where the hit doesn't count. dir is the hit's
// direction, pos where it hits, f its damage, act who hit, part the body part (HIT_PART_*). A hit on a human without
// energy left plays his agony's death, unless it plays already.
// FUNCTION: GAME 0x5762a0
bool C_human::Hit(int type, const S_vector &dir, const S_vector &pos, const S_vector &v3, float f, C_actor *act,
    uint part, I3D_frame *frm)
{
    if (type == HIT_SHOT && act == this)
        return false;
    if (CRacing && !CRacing->unknown_2c7)
        return false;
    if (act_mission->GetGame()->cheats & GAMEF_INVULNERABLE) {
        if (act_mission->GetGame()->GetPlayer() == this)
            return false;
        if (human_type == HUMAN_TYPE_UNKNOWN_2)
            return false;
    }
    if (unknown_94 & HUMANF_EVENT_HIT_FROM_PLAYER && GetType() == ACTOR_ENTITY &&
        act == act_mission->GetGame()->GetPlayer())
        ((C_entity *)this)->SetEvent(EVENT_HIT_FROM_PLAYER);
    if (!alive || properties[HUMAN_PROPERTY_ENERGY] < .01f) {
        if (agony_anim > -1 && anim_id - agony_anim != 0x93) {
            Death();
            unknown_23c[0].unknown_4 = agony_anim + 0x96;
            unknown_23c[1].unknown_0 = 3;
            unknown_23c[0].unknown_8 = 0;
            unknown_23c[0].unknown_10 = 3;
            unknown_23c[0].unknown_0 = 1;
            unknown_23c[0].unknown_c = false;
            action_state = 6;
            unknown_2a0 = 0;
            PersonAnim(0.0f, true, false);
            return true;
        }
        return false;
    }
    if (act_mission->GetGame()->GetPlayer() == this)
        mafiaIndicators.show_flags &= ~(INDF_THIEF | INDF_MAP);
    killer = act;
    if (box)
        Do_FuckingBoxDrop();
    if (dead_body)
        Do_DeadBodyDrop();
    unknown_21c = false;
    anim_machine.DelExtendedAnim(4.0f);
    unknown_1fe = false;
    unknown_1ec = false;
    aimed = false;
    PoseNullRotate();
    ResetWeaponEffects();
    unknown_581 = false;
    if (unknown_230)
        ChangeWeaponModel();
    if (action_state == 10 && act_mission->GetGame()->GetPlayer() == this) {
        mafiaIndicators.show_flags &= ~INDF_THIEF;
        stealing_car = NULL;
        unknown_44c = -1;
        if (weapon_model)
            weapon_model->SetOn(true);
        if (weapon_model2)
            weapon_model2->SetOn(true);
        act_mission->GetGame()->Stop3DSound(unknown_99c, false, .001f);
        act_mission->GetGame()->shvestky.CarSteal(this, false, false);
        act_mission->GetGame()->GetCamera().EnableFreeLook(false, false);
    }
    if (car && car->GetType() == ACTOR_CAR && seat != -1)
        return HitInCar(type, dir, pos, v3, f, act, part, frm);
    switch (unknown_410) {
    case 1:
        Intern_UseCar(true);
        break;
    case 2:
    case 5:
        if (anim_machine.track != -1) {
            uint time, length;
            anim_machine.GetActiveAnimTime(time, length);
            if (time + 240 < length) {
                anim_machine.Tick(length - time - 240);
                frame->SetPos(this->pos);
                frame->SetDir(dir, 0.0f);
                frame->Update();
            }
        }
        Intern_UseCar(false);
        break;
    case 3:
        Intern_UseRailway(true);
        break;
    case 4:
        Intern_UseRailway(false);
        break;
    }
    unknown_410 = 0;
    using_car = NULL;
    bool fall = false;
    bool heavy = false;
    bool stroke = false;
    bool agony = false;
    bool landing = false;
    switch (type) {
    case HIT_UNKNOWN_7: {
        properties[HUMAN_PROPERTY_ENERGY] = -1.0f;
        S_vector particle_pos(0.0f, 1.4f, 0.0f);
        fall = true;
        if (act_mission->options.particles >= 1)
            act_mission->GetEffects()->CreateParticle(HITPARTICLE_UNKNOWN_28, particle_pos, dir, 0, frame);
        break;
    }
    case HIT_SHOT: {
        float damage = f;
        switch (part) {
        case HIT_PART_HAND_R:
            f *= .3f;
            properties[HUMAN_PROPERTY_ENERGY_HAND_R] -= f;
            break;
        case HIT_PART_HAND_L:
            f *= .3f;
            properties[HUMAN_PROPERTY_ENERGY_HAND_L] -= f;
            break;
        case HIT_PART_LEG_R:
            f *= .5f;
            properties[HUMAN_PROPERTY_ENERGY_LEG_R] -= f;
            break;
        case HIT_PART_LEG_L:
            f *= .5f;
            properties[HUMAN_PROPERTY_ENERGY_LEG_L] -= f;
            break;
        case HIT_PART_BODY:
            break;
        case HIT_PART_HEAD:
            f *= 1.5f;
            break;
        }
        properties[HUMAN_PROPERTY_ENERGY] -= f;
        bool close_range = false;
        if (act && (act->GetType() == ACTOR_PLAYER || act->GetType() == ACTOR_ENTITY)) {
            S_item_def *weapon = ((C_human *)act)->inventory.GetWeaponInRightHand();
            if (weapon)
                close_range = weapon->flags & ITEMF_CLOSE_RANGE ? true : false;
        }
        if (close_range) {
            // Written out with the parentheses: Square() or another grouping orders the FPU's squares otherwise
            S_vector d = frame->GetWorldPos() - act->GetFrame()->GetWorldPos();
            float dist2 = (d.x * d.x + d.y * d.y) + d.z * d.z;
            if (dist2 < 25.0f) {
                part = HIT_PART_BODY;
                fall = true;
                if (dist2 < 9.0f)
                    properties[HUMAN_PROPERTY_ENERGY] = 0.0f;
            }
        } else if (damage > 80.0f && part < HIT_PART_BODY)
            heavy = true;
        if (act_mission->GetGame()->GetPlayer() == this)
            mafiaIndicators.AddBloodSplash(rand() * 800 / RAND_MAX, rand() * 600 / RAND_MAX,
                rand() * 400 / RAND_MAX + 50);
        if (heavy) {
            if (crouching && part != HIT_PART_HEAD)
                fall = true;
            if (!crouching && part == HIT_PART_BODY)
                fall = true;
        }
        if (!fall && properties[HUMAN_PROPERTY_ENERGY] < .01f && part == HIT_PART_BODY && !rnd(5) &&
            act_mission->GetGame()->GetPlayer() != this)
            agony = true;
        S_vector blood_dir = dir + S_vector(rnd(), rnd(), rnd()) * .3f;
        blood_dir.Normalize();
        S_vector blood_pos = pos + blood_dir * .3f;
        blood_dir *= 3.0f;
        if (act_mission->options.game_effects >= 1)
            act_mission->GetGame()->blood_manager.AddBlood(blood_pos, blood_dir, 1.0f, 0, false);
        if (!bleed_count && !act && !rnd(3) && properties[HUMAN_PROPERTY_ENERGY] > 1.0f &&
            properties[HUMAN_PROPERTY_ENERGY] / max_properties[HUMAN_PROPERTY_ENERGY] < .3f) {
            bleed_count = 10;
            bleed_time = 500;
        }
        break;
    }
    case HIT_STROKE:
        stroke = true;
        properties[HUMAN_PROPERTY_ENERGY] -= f;
        break;
    case HIT_UNKNOWN_2:
        properties[HUMAN_PROPERTY_ENERGY] -= f;
        if (f > 100.0f)
            fall = true;
        else if (f > 50.0f) {
            heavy = true;
            part = rnd(4) + HIT_PART_HAND_R;
        } else
            part = HIT_PART_BODY;
        break;
    case HIT_UNKNOWN_8: {
        float len2 = dir.Square();
        properties[HUMAN_PROPERTY_ENERGY] -= f;
        if (len2 > 9.0f) {
            S_vector side(dir.z, 0.0f, -dir.x);
            fall = true;
            if (!rnd(2)) {
                side.x *= -1.0f;
                side.z *= -1.0f;
            }
            (S_vector &)dir += side;
        } else
            part = HIT_PART_BODY;
        break;
    }
    case HIT_UNKNOWN_3:
        properties[HUMAN_PROPERTY_ENERGY] -= f;
        properties[HUMAN_PROPERTY_ENERGY_HAND_L] -= f * .3f;
        properties[HUMAN_PROPERTY_ENERGY_HAND_R] -= f * .3f;
        properties[HUMAN_PROPERTY_ENERGY_LEG_L] -= f * .5f;
        properties[HUMAN_PROPERTY_ENERGY_LEG_R] -= f * .5f;
        break;
    case HIT_UNKNOWN_4: {
        properties[HUMAN_PROPERTY_ENERGY] -= f;
        part = rnd(2) + HIT_PART_LEG_R;
        S_vector blood_dir = dir + S_vector(rnd(), rnd(), rnd()) * .3f;
        blood_dir.Normalize();
        S_vector blood_pos = GetWorldPos();
        blood_pos.y += .6f;
        blood_dir *= 2.0f;
        if (act_mission->options.game_effects >= 1)
            act_mission->GetGame()->blood_manager.AddBlood(blood_pos, blood_dir, 1.0f, 0, false);
        if (act_mission->options.particles >= 1)
            act_mission->GetEffects()->CreateParticle(SHOTPARTICLE_BODY, blood_pos, blood_dir, 0, NULL);
        break;
    }
    case HIT_UNKNOWN_5:
        break;
    case HIT_LANDING:
        landing = true;
        properties[HUMAN_PROPERTY_ENERGY] -= f;
        break;
    case HIT_UNKNOWN_9:
        properties[HUMAN_PROPERTY_ENERGY] -= f;
        break;
    }
    if (act) {
        if (act->GetType() == ACTOR_ENTITY)
            ((C_entity *)act)->cbHitInfo(type, f, this);
        if (act->GetType() == ACTOR_ENTITY || act->GetType() == ACTOR_PLAYER)
            ((C_human *)act)->unknown_9b0 += 1.0f;
    }
    if (properties[HUMAN_PROPERTY_ENERGY] < .01f) {
        act_mission->GetGame()->DeathHumanTalk(this);
        if (act_mission->GetGame()->score_on && act) {
            bool by_player = false;
            if (act->GetType() == ACTOR_CAR) {
                if (((C_car *)act)->GetOwner(0) == act_mission->GetGame()->GetPlayer())
                    by_player = true;
            } else if (act->GetType() == ACTOR_PLAYER && act == act_mission->GetGame()->GetPlayer())
                by_player = true;
            if (by_player) {
                if (human_type == HUMAN_TYPE_POLICE)
                    act_mission->GetGame()->ScoreSet(act_mission->GetGame()->score);
                else if (score_bonus)
                    act_mission->GetGame()->ScoreSet(act_mission->GetGame()->score + 500);
            }
        }
        properties[HUMAN_PROPERTY_ENERGY] = 0.0f;
        alive = false;
        if (type == HIT_SHOT)
            killed_by_shot = true;
        InventoryTruncate();
        Do_WeaponDrop();
        FreeHand();
    }
    if (properties[HUMAN_PROPERTY_ENERGY_HAND_L] < 0.0f)
        properties[HUMAN_PROPERTY_ENERGY_HAND_L] = 0.0f;
    if (properties[HUMAN_PROPERTY_ENERGY_HAND_R] < 0.0f)
        properties[HUMAN_PROPERTY_ENERGY_HAND_R] = 0.0f;
    if (properties[HUMAN_PROPERTY_ENERGY_LEG_L] < 0.0f)
        properties[HUMAN_PROPERTY_ENERGY_LEG_L] = 0.0f;
    if (properties[HUMAN_PROPERTY_ENERGY_LEG_R] < 0.0f)
        properties[HUMAN_PROPERTY_ENERGY_LEG_R] = 0.0f;
    if (heavy && part > HIT_PART_LEG_L)
        fall = true;
    unknown_1ec = false;
    if (ff_manager.human_effects && act_mission->GetGame()->GetPlayer() == this) {
        float gain = f * .033333f;
        if (gain > 1.0f)
            gain = 1.0f;
        else if (gain < .2f)
            gain = .2f;
        ff_manager.SetGain(ff_hit, gain * mafiaLoadSave.settings.ff_player_movement);
        S_vector2 hit_dir(dir.x, dir.z);
        S_vector look = frame->GetWorldDir();
        S_vector2 back(-look.x, -look.z);
        float angle = AngleTo2(hit_dir, back);
        if (angle < 0.0f)
            angle += PI * 2.0f;
        ff_manager.SetDirection(ff_hit, angle);
        ff_manager.Start(ff_hit);
    }
    if (wingman > -1) {
        int lives = (int)(properties[HUMAN_PROPERTY_ENERGY] / max_properties[HUMAN_PROPERTY_ENERGY] * 100.0f);
        if (alive)
            lives = Max(lives, 1);
        mafiaIndicators.PlayerSetWingmanLives(wingman, lives);
    }
    if (no_anim_hit && alive)
        return true;
    uint time = GetTickCount();
    int render_number = act_mission->render_number;
    if (time - last_hit_time < 200 || render_number == last_hit_render) {
        if (alive)
            return true;
        if (fall) {
            fall = false;
            heavy = true;
        }
    }
    last_hit_time = time;
    last_hit_render = render_number;
    if (!car) {
        if (landing)
            HitLanding();
        else if (agony)
            HitAgony(dir);
        else if (fall)
            HitFall(dir, part);
        else if (stroke)
            HitStroke(dir, part);
        else
            HitGun(dir, part, heavy);
    } else if (car->GetType() == ACTOR_CAR && seat == -1)
        HitGun(dir, part, false);
    else if (car->GetType() == ACTOR_RAILWAY)
        HitGun(dir, part, false);
    if (type == HIT_UNKNOWN_8 && !alive && anim_machine.track == -1) {
        uint anim_time, length;
        anim_machine.GetActiveAnimTime(anim_time, length);
        anim_time = (uint)(length * .8f);
        ((I3D_model *)frame)->SetAnimTime(anim_machine.anim_index, anim_time);
    }
    PersonAnim(0.0f, true, false);
    return true;
}

// The fall's landing (PS2 name)
inline void C_human::HitLanding()
{
    anim_id = 0;
    if (alive) {
        next_anim_id = 0x67;
        PersonAnim(0.0f, true, false);
        uint time, length;
        anim_machine.GetActiveAnimTime(time, length);
        if (length > 250) {
            unknown_44c = length - 250;
            unknown_430.Zero();
            unknown_43c = 0.0f;
            action_state = 5;
        }
        NewScream(rnd(7) + 0x55);
    } else {
        unknown_23c[0].unknown_4 = GetAnimIDForDeath(HIT_PART_BODY, crouching, this->dir, this->dir);
        unknown_23c[0].unknown_8 = 0;
        unknown_23c[0].unknown_10 = 3;
        unknown_23c[0].unknown_0 = 1;
        unknown_23c[0].unknown_c = false;
        unknown_23c[1].unknown_0 = 3;
        action_state = 6;
        unknown_2a0 = 0;
        Movement(1);
        NewScream(rnd(2) + 0x5c);
    }
}

// The agony after a shot in the body (PS2 name)
inline void C_human::HitAgony(const S_vector &dir)
{
    dead = true;
    unknown_45c = rnd(8) + 5;
    float angle = dir.AngleTo(this->dir);
    if (crouching)
        agony_anim = angle < PI / 2 ? 12 : 8;
    else
        agony_anim = angle < PI / 2 ? 4 : 0;
    anim_id = agony_anim + 0x93;
    RecompileDeathPos();
    anim_id = 0;
    unknown_23c[0].unknown_4 = agony_anim + 0x93;
    unknown_23c[0].unknown_8 = 0;
    unknown_23c[0].unknown_10 = 3;
    unknown_23c[0].unknown_0 = 1;
    unknown_23c[0].unknown_c = false;
    unknown_23c[1].unknown_0 = 2;
    action_state = 6;
    unknown_2a0 = 0;
    Movement(1);
    NewScream(rnd(3) + 0x5e);
}

// A stroke's hit (PS2 name)
inline void C_human::HitStroke(const S_vector &dir, uint part)
{
    if (alive && anim_id >= 0x3d && anim_id <= 0x45)
        return;
    anim_id = 0;
    if (alive) {
        if (part == HIT_PART_HEAD) {
            float angle = dir.AngleTo(this->dir);
            if (angle < PI / 4)
                unknown_23c[0].unknown_4 = 0xcd;
            else if (angle > PI * 3 / 4)
                unknown_23c[0].unknown_4 = 0xec;
            else {
                S_vector2 look(this->dir.x, this->dir.z);
                S_vector2 hit(dir.x, dir.z);
                if (AngleTo2(look, hit) > 0.0f)
                    unknown_23c[0].unknown_4 = 0xed;
                else
                    unknown_23c[0].unknown_4 = 0xee;
            }
        } else {
            S_vector2 look(this->dir.x, this->dir.z);
            S_vector2 hit(dir.x, dir.z);
            if (AngleTo2(look, hit) > 0.0f)
                unknown_23c[0].unknown_4 = 0xf0;
            else
                unknown_23c[0].unknown_4 = 0xef;
        }
        unknown_23c[0].unknown_8 = -250;
        NewScream(rnd(7) + 0x55);
    } else {
        unknown_23c[0].unknown_4 = GetAnimIDForDeath(part, crouching, this->dir, dir);
        unknown_23c[0].unknown_8 = 0;
        NewScream(rnd(2) + 0x5c);
    }
    unknown_23c[0].unknown_10 = 3;
    unknown_23c[0].unknown_0 = 1;
    unknown_23c[0].unknown_c = false;
    unknown_23c[1].unknown_0 = 3;
    action_state = 6;
    unknown_2a0 = 0;
    Movement(1);
}

// A shot's hit (PS2 name), heavy for the strong hits' animations
inline void C_human::HitGun(const S_vector &dir, uint part, bool heavy)
{
    if (alive) {
        if (anim_id >= 0x3d && anim_id <= 0x45)
            return;
        if (act_mission->GetGame()->GetPlayer() == this && !heavy && part != HIT_PART_LEG_R && part != HIT_PART_LEG_L &&
            part != HIT_PART_HEAD) {
            HitGunHPT(dir, part, heavy);
            return;
        }
    }
    anim_id = 0;
    if (alive) {
        float angle = dir.AngleTo(this->dir);
        if (heavy) {
            if (crouching) {
                if (part == HIT_PART_HEAD)
                    unknown_23c[0].unknown_4 = angle < PI / 2 ? 0xe1 : 0xe0;
            } else {
                switch (part) {
                case HIT_PART_HAND_R:
                    unknown_23c[0].unknown_4 = angle < PI / 2 ? 0xcf : 0xce;
                    break;
                case HIT_PART_HAND_L:
                    unknown_23c[0].unknown_4 = angle < PI / 2 ? 0xd1 : 0xd0;
                    break;
                case HIT_PART_LEG_R:
                    unknown_23c[0].unknown_4 = 0xd2;
                    break;
                case HIT_PART_LEG_L:
                    unknown_23c[0].unknown_4 = 0xd3;
                    break;
                case HIT_PART_BODY:
                    break;
                case HIT_PART_HEAD:
                    unknown_23c[0].unknown_4 = angle < PI / 2 ? 0xd5 : 0xd4;
                    break;
                }
            }
        } else if (crouching) {
            switch (part) {
            case HIT_PART_HAND_R:
                unknown_23c[0].unknown_4 = angle < PI / 2 ? 0xd7 : 0xd6;
                break;
            case HIT_PART_HAND_L:
                unknown_23c[0].unknown_4 = angle < PI / 2 ? 0xd9 : 0xd8;
                break;
            case HIT_PART_LEG_R:
                unknown_23c[0].unknown_4 = 0xda;
                break;
            case HIT_PART_LEG_L:
                unknown_23c[0].unknown_4 = 0xdb;
                break;
            case HIT_PART_BODY:
                unknown_23c[0].unknown_4 = angle < PI / 2 ? 0xdd : 0xdc;
                break;
            case HIT_PART_HEAD:
                unknown_23c[0].unknown_4 = angle < PI / 2 ? 0xdf : 0xde;
                break;
            }
        } else {
            switch (part) {
            case HIT_PART_HAND_R:
                unknown_23c[0].unknown_4 = angle < PI / 2 ? 0xc5 : 0xc4;
                break;
            case HIT_PART_HAND_L:
                unknown_23c[0].unknown_4 = angle < PI / 2 ? 0xc7 : 0xc6;
                break;
            case HIT_PART_LEG_R:
                unknown_23c[0].unknown_4 = 0xc8;
                break;
            case HIT_PART_LEG_L:
                unknown_23c[0].unknown_4 = 0xc9;
                break;
            case HIT_PART_BODY:
                unknown_23c[0].unknown_4 = angle < PI / 2 ? 0xcb : 0xca;
                break;
            case HIT_PART_HEAD:
                unknown_23c[0].unknown_4 = angle < PI / 2 ? 0xcd : 0xcc;
                break;
            }
        }
        unknown_23c[0].unknown_8 = -250;
        if (heavy && (part == HIT_PART_LEG_R || part == HIT_PART_LEG_L))
            NewScream(0x54);
        else
            NewScream(rnd(7) + 0x55);
    } else {
        unknown_23c[0].unknown_4 = GetAnimIDForDeath(part, crouching, this->dir, dir);
        unknown_23c[0].unknown_8 = 0;
        NewScream(rnd(2) + 0x5c);
    }
    unknown_23c[0].unknown_10 = 3;
    unknown_23c[0].unknown_0 = 1;
    unknown_23c[0].unknown_c = false;
    unknown_23c[1].unknown_0 = 3;
    action_state = 6;
    unknown_2a0 = 0;
    Movement(1);
}

// The player's hit on a hand or the body (PS2 name): an extended animation over the one playing, by the weapon held
inline void C_human::HitGunHPT(const S_vector &dir, uint part, bool heavy)
{
    float angle = dir.AngleTo(this->dir);
    int id = -1;
    switch (unknown_1e8) {
    case 0:
        id = 0xe2;
        break;
    case 1:
        id = aimed ? 0xe4 : 0xe2;
        break;
    case 2:
        id = aimed ? 0xe6 : 0xea;
        break;
    case 3:
        id = aimed ? 0xe8 : 0xea;
        break;
    }
    if (angle > PI / 2)
        id++;
    int index = GetAnimIndex(id);
    if (index > -1) {
        I3D_animation_set *set = NULL;
        anim_cache.Create(_anim_info_[index].name, &set, NULL, NULL);
        if (set) {
            anim_machine.SetExtendedAnim(set, 1.0f, 4.0f, true);
            anim_machine.unknown_10c = true;
            set->Release();
        }
    }
}

// The fall the hit throws the human into, turned to the hit (PS2 name)
inline void C_human::HitFall(const S_vector &dir, uint part)
{
    anim_id = 0;
    S_vector d = dir;
    d.y = 0.0f;
    float angle = d.AngleTo(this->dir);
    if (angle < PI / 4) {
        this->dir = d;
        unknown_23c[0].unknown_0 = 1;
        unknown_23c[0].unknown_4 = alive ? 0x72 : rnd(2) ? 0x70 : 0x71;
        unknown_23c[0].unknown_c = false;
        unknown_23c[0].unknown_10 = 0;
        unknown_23c[1].unknown_0 = 3;
    } else {
        if (angle > PI * 3 / 4) {
            this->dir = -d;
            unknown_23c[0].unknown_0 = 1;
            unknown_23c[0].unknown_4 = 0x6a;
            unknown_23c[0].unknown_c = false;
            unknown_23c[1].unknown_0 = 1;
            unknown_23c[1].unknown_c = false;
            unknown_23c[2].unknown_0 = 3;
        } else {
            S_vector2 look(this->dir.x, this->dir.z);
            S_vector2 hit(d.x, d.z);
            if (AngleTo2(look, hit) > 0.0f) {
                unknown_23c[0].unknown_4 = 0x6b;
                this->dir = S_vector(d.z, d.y, -d.x);
            } else {
                unknown_23c[0].unknown_4 = 0x6c;
                this->dir = S_vector(-d.z, d.y, d.x);
            }
            unknown_23c[0].unknown_0 = 1;
            unknown_23c[0].unknown_c = false;
            unknown_23c[1].unknown_0 = 1;
            unknown_23c[1].unknown_c = false;
            unknown_23c[2].unknown_0 = 3;
        }
        unknown_23c[1].unknown_10 = 0;
        unknown_23c[0].unknown_10 = 0;
        unknown_23c[1].unknown_4 = alive ? 0x6f : rnd(2) ? 0x6d : 0x6e;
    }
    this->dir.Normalize();
    frame->SetDir(this->dir, 0.0f);
    frame->Update();
    action_state = 6;
    unknown_2a0 = 0;
    Movement(1);
    if (alive)
        NewScream(rnd(7) + 0x55);
    else
        NewScream(rnd(2) + 0x5c);
}


// The human hit in a car's seat (PS2 name): true
// FUNCTION: GAME 0x578290
bool C_human::HitInCar(int type, const S_vector &dir, const S_vector &pos, const S_vector &v3, float f, C_actor *act,
    uint part, I3D_frame *frm)
{
    if (type == HIT_SHOT || type == HIT_UNKNOWN_5)
        properties[HUMAN_PROPERTY_ENERGY] -= f;
    if (act && act->GetType() == ACTOR_ENTITY)
        ((C_entity *)act)->cbHitInfo(type, f, this);
    if (properties[HUMAN_PROPERTY_ENERGY] < .01f) {
        properties[HUMAN_PROPERTY_ENERGY] = 0.0f;
        alive = false;
        if (!net_direct && !seat)
            car->driver_killed = true;
        if (unknown_5e4 == 2)
            next_anim_id = car->IsSeatLeft(seat) ? 0x10d : 0x10e;
        else if (!seat)
            next_anim_id = 0x10a;
        else if (unknown_1e8 == 2 || unknown_1e8 == 3)
            next_anim_id = 0x10c;
        else
            next_anim_id = 0x10b;
        PersonAnim(0.0f, true, true);
        uint time, length;
        anim_machine.GetActiveAnimTime(time, length);
        unknown_5e8 = length;
        unknown_450 = length;
        action_state = 8;
        if (act_mission->GetGame()->score_on && act) {
            bool by_player = false;
            if (act->GetType() == ACTOR_CAR) {
                if (((C_car *)act)->GetOwner(0) == act_mission->GetGame()->GetPlayer())
                    by_player = true;
            } else if (act->GetType() == ACTOR_PLAYER && act == act_mission->GetGame()->GetPlayer())
                by_player = true;
            if (by_player) {
                if (human_type == HUMAN_TYPE_POLICE)
                    act_mission->GetGame()->ScoreSet(act_mission->GetGame()->score);
                else if (score_bonus)
                    act_mission->GetGame()->ScoreSet(act_mission->GetGame()->score + 500);
            }
        }
        InventoryTruncate();
    }
    NewScream(rnd(7) + 0x55);
    if (wingman > -1) {
        int lives = (int)(properties[HUMAN_PROPERTY_ENERGY] / max_properties[HUMAN_PROPERTY_ENERGY] * 100.0f);
        if (alive)
            lives = Max(lives, 1);
        mafiaIndicators.PlayerSetWingmanLives(wingman, lives);
    }
    return true;
}


// The actor ran into the human (PS2 name; the parameters' names are ours): a car or a train faster than 3 hits him,
// a slower car pushes him aside, a fast one sounds; false
// FUNCTION: GAME 0x5784a0
bool C_human::Collision(C_actor *act, const S_vector &pos, const S_vector &dir, const S_vector &v3, float f,
    I3D_frame *frm)
{
    if (!act)
        return false;
    switch (act->GetType()) {
    case ACTOR_CAR: {
        if (car)
            return false;
        float speed = dir.Magnitude();
        if (speed > 3.0f) {
            speed -= 2.5f;
            S_vector hit_dir = dir;
            Hit(HIT_UNKNOWN_8, hit_dir, pos, v3, speed * 4.2f, act, 0, NULL);
        } else {
            if (anim_machine.track == -1) {
                S_vector d = act->GetFrame()->GetWorldPos() - GetFrame()->GetWorldPos();
                S_vector from = GetFrame()->GetWorldPos() - d;
                from.y += .5f;
                d *= 2.0f;
                I3D_COLLISION col;
                if (act_mission->GetScene()->TestColHierarchy(from, d, act->GetFrame(), col, 0, NULL, 0, NULL) &&
                    col.dist > .8f) {
                    d.Normalize();
                    d *= col.dist - .8f;
                    from += d;
                    S_vector e = act->GetFrame()->GetWorldPos() - GetFrame()->GetWorldPos();
                    if ((act->GetFrame()->GetWorldPos() - from).Square() > e.Square()) {
                        this->pos = from;
                        this->pos.y += _get_delta_nodyn_y_(this->pos);
                        GetFrame()->SetPos(this->pos);
                        GetFrame()->Update();
                    }
                }
            }
            Do_SideJump(pos, dir);
        }
        if (act_mission->options.game_sounds >= 2 && speed > 5.0f) {
            I3D_sound *sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
            if (sound) {
                sound->SetPos(GetFrame()->GetWorldPos() + S_vector(0.0f, 1.0f, 0.0f));
                sound_table.ConstructSound(sound, SOUND_RUN_OVER);
                sound->SetOn(true);
                act_mission->GetGame()->Play3DSound(sound);
            }
        }
        if (!alive && act_mission->GetGame()->shvestky.active)
            act_mission->GetGame()->shvestky.NextNaSrackySoudruh(((C_car *)act)->GetOwner(0));
        return false;
    }
    case ACTOR_RAILWAY:
    case ACTOR_AIRPLANE:
    case ACTOR_WAGON: {
        float speed = dir.Magnitude();
        if (speed > 3.0f) {
            speed -= 2.5f;
            if (act->GetType() == ACTOR_WAGON)
                speed = properties[HUMAN_PROPERTY_ENERGY] + 1.0f;
            Hit(HIT_UNKNOWN_2, dir, pos, v3, speed * 4.2f, act, 0, NULL);
        }
        if (act_mission->options.game_sounds >= 2 && speed > 5.0f) {
            I3D_sound *sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
            if (sound) {
                sound->SetPos(GetFrame()->GetWorldPos() + S_vector(0.0f, 1.0f, 0.0f));
                sound_table.ConstructSound(sound, SOUND_RUN_OVER);
                sound->SetOn(true);
                act_mission->GetGame()->Play3DSound(sound);
            }
        }
        return false;
    }
    }
    return false;
}


// Whether the actor at pos moving by dir runs into the human, a sphere of radius + 0.7 around his chest moving by a
// tenth of his direction (PC only; the parameters' names are ours): the part of the move where they first meet into
// dist, 0 where they already do, and the human into coll
// FUNCTION: GAME 0x578c60
bool C_human::IsPossibleColl(C_actor *act, S_vector pos, S_vector dir, float radius, float &dist, C_actor **coll)
{
    if (car)
        return false;
    S_vector move = this->dir * .1f;
    S_vector center(this->pos.x, this->pos.y + 1.0f, this->pos.z);
    S_vector a = center - pos;
    S_vector b = move - dir;
    float r = radius + .7f;
    float c = a.Square();
    float r2 = r * r;
    float t;
    if (c <= r2)
        t = 0.0f;
    else {
        float aa = b.Square();
        if (aa <= 0.0f)
            t = -1.0f;
        else {
            float bb = b.Dot(a) * 2.0f;
            float disc = bb * bb - (c - r2) * aa * 4.0f;
            if (disc < 0.0f)
                t = disc;
            else {
                float s = (float)sqrt(disc);
                float inv = 1.0f / (aa * 2.0f);
                float t1 = (s - bb) * inv;
                t = (-bb - s) * inv;
                if (t1 < t)
                    t = t1;
            }
        }
    }
    dist = t;
    if (t >= 0.0f) {
        if (coll)
            *coll = this;
        return true;
    }
    return false;
}

// C_HUMAN 0x578e20 Unknown578e20
// C_HUMAN 0x578ee0 PoseInterpolate
// C_HUMAN 0x579630 PoseSetPoseNormal
// C_HUMAN 0x579ea0 PoseSetPoseAimed

// FUNCTION: GAME 0x57a560
void C_human::PoseNullRotate()
{
    unknown_2b4 = unknown_2c0 = unknown_2cc = S_vector(0.0f, 0.0f, 1.0f);
    look_frame = NULL;
    unknown_2b0 = true;
    unknown_a88 &= ~2;
    unknown_a88 |= 5;
}

// FUNCTION: GAME 0x57a5f0
void C_human::PoseValidate(bool valid)
{
    if (frame)
    {
        ((I3D_model *)frame)->SetPoseBlend(valid ? 1.0f : 0.0f);
        frame->Tick(1);
    }
}

// C_HUMAN 0x57a5f0 PoseValidate

// FUNCTION: GAME 0x57a630
int C_human::GetAnimIDForDeath(int i, bool b, const S_vector &v1, const S_vector &v2)
{
    float angle = v2.AngleTo(v1);
    if (b)
    {
        if (i == 6)
        {
            if (angle < PI / 2)
                return (rnd(2) ? 2 : 0) + 0x90;
            return (rnd(2) ? 1 : 0) + 0x91;
        }
        if (angle < PI / 2)
            return rnd(2) + 0x8c;
        return rnd(2) + 0x8e;
    }
    if (i == 6)
    {
        if (angle < PI / 2)
            return (rnd(2) ? 2 : 0) + 0x89;
        return (rnd(2) ? 1 : 0) + 0x8a;
    }
    if (angle < PI / 2)
        return rnd(3) + 0x83;
    return rnd(3) + 0x86;
}

// C_HUMAN 0x57a710 Movement
// C_HUMAN 0x57b660 MovementIntern
// C_HUMAN 0x57cb60 TestMoveColl

// FUNCTION: GAME 0x57cfe0
void C_human::DynSetNextCollOwnerMult()
{
    int n = unknown_698.size();
    if (n)
    {
        unknown_6a8.clear();
        int i;
        for (i = num_colls; i--; )
            unknown_6a8.push_back(colls[i]);
        for (i = unknown_698.size(); i--; )
            unknown_6a8.push_back(unknown_698[i]);
        gifCollision.DynSetNextCollOwnerMult(&unknown_6a8[0], unknown_6a8.size(), NULL);
    }
    else
        gifCollision.DynSetNextCollOwnerMult(colls, num_colls, NULL);
}

// C_HUMAN 0x57d0e0 TestMoveCollInOwner

// FUNCTION: GAME 0x57d2e0
void C_human::Do_StopAnim()
{
    action_state = 1;
    if (alive)
    {
        anim_id = 0;
        next_anim_id = 1;
        PersonAnim(0.0f, true, false);
    }
}


// FUNCTION: GAME 0x57d310
void C_human::Do_Breath()
{
    if (!alive)
        return;
    if (action_state == 1 || action_state == 2 || action_state == 3)
    {
        action_state = 1;
        unknown_42c = 0;
        unknown_420.Zero();
        next_anim_id = 1;
    }
}


// FUNCTION: GAME 0x57d360
void C_human::Go_Front(bool b)
{
    if (car && car->type == ACTOR_CAR && seat != -1)
        return;
    if (crouching || box || dead_body)
        b = false;
    action_state = 2;
    unknown_420 = dir * (b ? 5.0f : 1.7f) * properties[HUMAN_PROPERTY_SPEED];
    next_anim_id = b ? 5 : 6;
}


// FUNCTION: GAME 0x57d430
void C_human::Go_Back()
{
    if (car && car->type == ACTOR_CAR && seat != -1)
        return;
    action_state = 2;
    unknown_420 = -dir * 1.5f * properties[HUMAN_PROPERTY_SPEED];
    next_anim_id = 7;
}


// FUNCTION: GAME 0x57d4d0
void C_human::Go_Left(bool b)
{
    if (car && car->type == ACTOR_CAR && seat != -1)
        return;
    if (crouching || box || dead_body)
        b = false;
    action_state = 2;
    unknown_42c = 0;
    unknown_420.x = -dir.z;
    unknown_420.z = dir.x;
    unknown_420.y = dir.y;
    unknown_420 *= (b ? 3.0f : 1.5f) * properties[HUMAN_PROPERTY_SPEED];
    next_anim_id = b ? 0xd : 0xb;
}

// FUNCTION: GAME 0x57d5a0
void C_human::Go_Right(bool b)
{
    if (car && car->type == ACTOR_CAR && seat != -1)
        return;
    if (crouching || box || dead_body)
        b = false;
    action_state = 2;
    unknown_42c = 0;
    unknown_420.x = dir.z;
    unknown_420.z = -dir.x;
    unknown_420.y = dir.y;
    unknown_420 *= (b ? 3.0f : 1.5f) * properties[HUMAN_PROPERTY_SPEED];
    next_anim_id = b ? 0xe : 0xc;
}

// FUNCTION: GAME 0x57d670
void C_human::Go_FrontLeft(bool b)
{
    if (car && car->type == ACTOR_CAR && seat != -1)
        return;
    if (crouching || box || dead_body)
        b = false;
    S_vector side(-dir.z, dir.y, dir.x);
    action_state = 2;
    unknown_42c = 0;
    unknown_420 = side + dir;
    unknown_420.Normalize();
    unknown_420 *= (b ? 5.0f : 1.7f) * properties[HUMAN_PROPERTY_SPEED];
    next_anim_id = b ? 0xf : 0x11;
}

// FUNCTION: GAME 0x57d860
void C_human::Go_FrontRight(bool b)
{
    if (car && car->type == ACTOR_CAR && seat != -1)
        return;
    if (crouching || box || dead_body)
        b = false;
    S_vector side(dir.z, dir.y, -dir.x);
    action_state = 2;
    unknown_42c = 0;
    unknown_420 = side + dir;
    unknown_420.Normalize();
    unknown_420 *= (b ? 5.0f : 1.7f) * properties[HUMAN_PROPERTY_SPEED];
    next_anim_id = b ? 0x10 : 0x12;
}

// FUNCTION: GAME 0x57da50
void C_human::Go_BackLeft()
{
    if (car && car->type == ACTOR_CAR && seat != -1)
        return;
    S_vector side(-dir.z, dir.y, dir.x);
    action_state = 2;
    unknown_42c = 0;
    unknown_420 = side + -dir;
    unknown_420.Normalize();
    unknown_420 *= properties[HUMAN_PROPERTY_SPEED] * 1.5f;
    next_anim_id = 0x13;
}

// FUNCTION: GAME 0x57dc10
void C_human::Go_BackRight()
{
    if (car && car->type == ACTOR_CAR && seat != -1)
        return;
    S_vector side(dir.z, dir.y, -dir.x);
    action_state = 2;
    unknown_42c = 0;
    unknown_420 = side + -dir;
    // The original tests the speed's length against MRG_ZERO here and does nothing with the result (PS2 goes on to
    // normalize it, as Go_BackLeft does)
    if (IsMrgZeroLess(unknown_420.Square()))
    {
    }
    unknown_420 *= properties[HUMAN_PROPERTY_SPEED] * 1.5f;
    next_anim_id = 0x14;
}

// C_HUMAN 0x57d5a0 Go_Right
// C_HUMAN 0x57d670 Go_FrontLeft
// C_HUMAN 0x57d860 Go_FrontRight
// C_HUMAN 0x57da50 Go_BackLeft
// C_HUMAN 0x57dc10 Go_BackRight
// C_HUMAN 0x57dd00 ChangeAnim

// FUNCTION: GAME 0x57dfc0
int C_human::GetAnimIDForUseCar(bool b1, bool b2, bool b3, bool b4, bool b5, bool b6)
{
    if (b6)
    {
        int id = 0x13c;
        if (b5)
            id = 0x13d;
        if (b1)
            id += 2;
        return id;
    }
    int id = 0;
    if (b1)
        id = 1;
    if (!b2)
        id |= 2;
    if (b3)
        id |= 4;
    if (b5)
        id |= 0x10;
    return id + 0x10f;
}

// C_HUMAN 0x57e020 Intern_UseCar(C_car *, int)
// C_HUMAN 0x57e500 Intern_FromCar
// C_HUMAN 0x57e980 RepairWeaponPosition
// C_HUMAN 0x57ec20 ChangeWeaponModel
// C_HUMAN 0x57f550 Do_ChangeWeapon

// FUNCTION: GAME 0x57f830
void C_human::Do_Aimed(bool aim)
{
    if (box || dead_body)
    {
        aimed = false;
        return;
    }
    if (aim && unknown_230)
        ChangeWeaponModel();
    aimed = aim;
    if (!aim && unknown_22c)
    {
        anim_machine.DelExtendedAnim(4.0f);
        unknown_22c = false;
    }
}


// FUNCTION: GAME 0x57f8a0
void C_human::Do_Crouched(bool crouch)
{
    if (box || dead_body)
    {
        crouching = false;
        return;
    }
    if (crouching && !crouch)
    {
        // Stays down where there's no room to stand up
        S_vector pos = frame->GetWorldPos();
        pos.y += 1.0f;
        gifCollision.DynSetNextCollOwnerMult(colls, num_colls, NULL);
        S_vector dir(0.0f, .55f, 0.0f);
        int hit = gifCollision.TestSphereB(pos, dir, .3f, NULL, NULL, COLF_UNKNOWN_2, 0);
        gifCollision.DynSetNextCollOwner(NULL, NULL);
        if (hit)
            crouch = true;
    }
    crouching = crouch;
}


// The item's ammunition cut to its table's most (PS2's InventoryTruncate does it to every item of its vector)
static inline void TruncateAmmo(S_GameItem &item)
{
    if (item.id > 1 && item.id < numOfGameItems)
    {
        S_item_def &def = gameItemTable[item.id];
        if ((uint)item.ammo_loaded > def.ammo_max)
            item.ammo_loaded = def.ammo_max;
        if ((uint)item.ammo_spare > def.spare_max)
            item.ammo_spare = def.spare_max;
    }
}

// FUNCTION: GAME 0x57f9a0
void C_human::InventoryTruncate()
{
    TruncateAmmo(inventory.right_hand);
    TruncateAmmo(inventory.unknown_80);
    for (int i = 5; i--; )
        TruncateAmmo(inventory.unknown_30[i]);
}

// C_HUMAN 0x57faa0 DropOutItems

// FUNCTION: GAME 0x5801f0
C_bridge *C_human::TozNaselJsemTenNejvyssiMost()
{
    if (car && car->type == ACTOR_CAR && (car->vehicle_flags >> 25) & 1)
    {
        std::vector<C_actor *> &actors = C_actor::GetGlobalActors();
        for (int i = actors.size(); i--; )
        {
            C_actor *act = actors[i];
            if (act->type == ACTOR_BRIDGE)
            {
                C_bridge *bridge = (C_bridge *)act;
                S_vector d = frame->GetWorldPos() - bridge->frame->GetWorldPos();
                if (d.x * d.x + d.y * d.y + d.z * d.z < 10000.0f && bridge->unknown_144 >= 3 && bridge->unknown_144 <= 5)
                    return bridge;
            }
        }
    }
    return NULL;
}

// C_HUMAN 0x5802f0 Can_DropOutFromCar_Free
// C_HUMAN 0x5805c0 Can_DropOutFromCar
// C_HUMAN 0x5808c0 Can_EnterToCar
// C_HUMAN 0x580ca0 ZasrateLezeniZAuta_ve2rano
// C_HUMAN 0x581220 ZasrateVybliknutiZAuta_ve3rano
// C_HUMAN 0x581eb0 Do_ClimbInCarLR
// C_HUMAN 0x582180 Use_Actor
// C_HUMAN 0x583590 Do_Shoot

// FUNCTION: GAME 0x583f40
bool C_human::Do_ThrowGrenade(S_vector &dir)
{
    if (!IsReady())
        return false;
    uint id = inventory.GetItemInRightHand(NULL);
    if (id != 5 && id != 0xf)
        return false;
    unknown_200 = dir;
    unknown_21c = false;
    if (this == act_mission->GetGame()->player)
        mafiaIndicators.show_flags &= ~INDF_THIEF;
    anim_machine.DelExtendedAnim(4.0f);
    next_anim_id = 0xa3;
    PersonAnim(0.0f, true, false);
    unknown_460 = act_mission->GetGame()->game_time;
    return true;
}

// C_HUMAN 0x584010 Can_SpecialStroke

// FUNCTION: GAME 0x584480
bool C_human::Do_Castrate(C_human *human)
{
    if (!human)
        return false;
    if (!human->IsReady())
        return false;
    C_game *game = act_mission->GetGame();
    if (game->cheats & 1)
    {
        if (game->player == human)
            return false;
        if (human->human_type == 2)
            return false;
    }
    next_anim_id = 0xf1;
    PersonAnim(0.0f, true, false);
    human->next_anim_id = 0xf2;
    human->PersonAnim(0.0f, true, false);
    human->properties[HUMAN_PROPERTY_ENERGY] -= properties[HUMAN_PROPERTY_STRENGTH] * 10.0f;
    return true;
}

// FUNCTION: GAME 0x584550
bool C_human::Do_HardHead(C_human *human)
{
    if (!human)
        return false;
    if (!human->IsReady())
        return false;
    C_game *game = act_mission->GetGame();
    if (game->cheats & 1)
    {
        if (game->player == human)
            return false;
        if (human->human_type == 2)
            return false;
    }
    next_anim_id = 0xf3;
    PersonAnim(0.0f, true, false);
    human->next_anim_id = 0xf4;
    human->PersonAnim(0.0f, true, false);
    human->properties[HUMAN_PROPERTY_ENERGY] -= properties[HUMAN_PROPERTY_STRENGTH] * 10.0f;
    return true;
}

// C_HUMAN 0x584550 Do_HardHead
// C_HUMAN 0x584620 Shooting
// C_HUMAN 0x585b40 Do_Reload
// C_HUMAN 0x585c60 Do_Holster
// C_HUMAN 0x585d90 Do_WeaponDrop

// FUNCTION: GAME 0x585e80
int C_human::Do_PlayAnim(const char *anim, bool simple, bool unknown)
{
    idle_time = -1;
    breath_time = -1;
    unknown_420.Zero();
    unknown_43c = 0.0f;
    unknown_22e = false;
    fall = 0.0f;
    unknown_21c = false;
    if (this == act_mission->GetGame()->player)
        mafiaIndicators.show_flags &= ~INDF_THIEF;
    anim_machine.SetAnim(anim, false, false, simple, 4.0f, 0.0f, true, unknown, NULL, true, (uint)-1);
    if (anim_machine.track != -1)
        action_state = 4;
    uint time, length;
    anim_machine.GetActiveAnimTime(time, length);
    return length;
}

// C_HUMAN 0x585f40 GetBoundPoints2D
// C_HUMAN 0x586190 Do_Jump
// C_HUMAN 0x586400 Do_Climb
// C_HUMAN 0x587040 Do_SideJump
// C_HUMAN 0x587190 _intern_ChangeModel
// C_HUMAN 0x5877a0 InitFFEffects
// C_HUMAN 0x587880 DoneFFEffects
// C_HUMAN 0x5878d0 _intern_ForceDeath

// FUNCTION: GAME 0x587bc0
void C_human::Do_ShockStart(int i)
{
    if (box || dead_body)
        return;
    if (i < 0)
        i = 0;
    else if (i > 3)
        i = 3;
    next_anim_id = (rnd(2) ? 0x2f : 0x33) + i;
    PersonAnim(0.0f, true, false);
}

// FUNCTION: GAME 0x587c20
void C_human::Do_ShockDown(int i)
{
    if (box || dead_body)
        return;
    if (i < 0)
        i = 0;
    else if (i > 2)
        i = 2;
    unknown_23c[0].unknown_4 = i * 3 + 0x3d;
    unknown_23c[0].unknown_0 = 1;
    unknown_23c[1].unknown_0 = 1;
    unknown_23c[0].unknown_c = false;
    unknown_23c[0].unknown_10 = 0;
    unknown_23c[1].unknown_4 = i * 3 + 0x3e;
    unknown_23c[1].unknown_c = false;
    unknown_23c[1].unknown_10 = 0;
    unknown_23c[2].unknown_0 = 3;
    action_state = 6;
    unknown_2a0 = 0;
}

// FUNCTION: GAME 0x587cb0
void C_human::Do_ShockUp()
{
    if (box || dead_body)
        return;
    if (anim_id == 0x3e || anim_id == 0x41 || anim_id == 0x44)
    {
        next_anim_id = anim_id + 1;
        PersonAnim(0.0f, true, false);
    }
}

// C_HUMAN 0x587c20 Do_ShockDown
// C_HUMAN 0x587cb0 Do_ShockUp

// FUNCTION: GAME 0x587cf0
void C_human::Do_ShockStumble()
{
    if (box || dead_body)
        return;
    if (anim_id == 0x46 || anim_id == 0x4a || anim_id == 0x48)
    {
        next_anim_id = anim_id + 1;
        PersonAnim(0.0f, true, false);
    }
}


// FUNCTION: GAME 0x587d30
void C_human::cbCollTester(tDynamicCollObject *obj)
{
    if (act_mission->options.game_effects >= 2)
    {
        ushort v = unknown_6ee;
        if (!v)
        {
            v = 10;
            unknown_6f0 = 6;
            unknown_6ee = v;
        }
        unknown_6ec = v;
    }
}

// C_HUMAN 0x587d70 Do_ThrowCocotFromCar
// C_HUMAN 0x588b40 _intern_ThrowMeFromCar
// C_HUMAN 0x589140 Do_FuckingBox
// C_HUMAN 0x589680 Do_FuckingBoxCancel
// C_HUMAN 0x589c20 Do_DeadBody
// C_HUMAN 0x58a5a0 EineMeineKleineAutoInLuft
// C_HUMAN 0x58a790 Can_Aimed

// FUNCTION: GAME 0x58a960
bool C_human::Do_CornerBreath(E_corner_view view)
{
    if (box || dead_body)
        return false;
    switch (view)
    {
    case CORNER_0:
        next_anim_id = 0x4e;
        return true;
    case CORNER_1:
        next_anim_id = 0x4f;
        return true;
    }
    return false;
}

// FUNCTION: GAME 0x58a9a0
bool C_human::Do_CornerView(E_corner_view view)
{
    if (box || dead_body)
        return false;
    switch (view)
    {
    case CORNER_0:
        next_anim_id = 0x50;
        break;
    case CORNER_1:
        next_anim_id = 0x51;
        break;
    case CORNER_2:
        if (!crouching)
            return false;
        next_anim_id = 0x52;
        break;
    default:
        return false;
    }
    SetWaitAnim();
    return true;
}

// FUNCTION: GAME 0x58aa60
bool C_human::Do_CornerFire(E_corner_view view)
{
    if (box || dead_body)
        return false;
    if (unknown_1e8 == 2 || unknown_1e8 == 0)
        return false;
    switch (view)
    {
    case CORNER_0:
        next_anim_id = 0x53;
        if (!crouching && inventory.GetWeaponInRightHand()->unknown_50 == 4)
            next_anim_id = 0x54;
        break;
    case CORNER_1:
        next_anim_id = 0x55;
        if (!crouching && inventory.GetWeaponInRightHand()->unknown_50 == 4)
            next_anim_id = 0x56;
        break;
    case CORNER_2:
        if (!crouching)
            return false;
        next_anim_id = 0x57;
        break;
    default:
        return false;
    }
    SetWaitAnim();
    aimed = true;
    return true;
}

// FUNCTION: GAME 0x58ab80
bool C_human::Do_CornerFireAimed(E_corner_view view)
{
    if (box || dead_body)
        return false;
    if (unknown_1e8 == 2 || unknown_1e8 == 0)
        return false;
    switch (view)
    {
    case CORNER_0:
        next_anim_id = 0x58;
        if (!crouching && inventory.GetWeaponInRightHand()->unknown_50 == 4)
            next_anim_id = 0x59;
        break;
    case CORNER_1:
        next_anim_id = 0x5a;
        if (!crouching && inventory.GetWeaponInRightHand()->unknown_50 == 4)
            next_anim_id = 0x5b;
        break;
    case CORNER_2:
        if (!crouching)
            return false;
        next_anim_id = 0x5c;
        break;
    default:
        return false;
    }
    PersonAnim(0.0f, true, false);
    aimed = true;
    return true;
}

// C_HUMAN 0x58a9a0 Do_CornerView
// C_HUMAN 0x58aa60 Do_CornerFire
// C_HUMAN 0x58ab80 Do_CornerFireAimed

// FUNCTION: GAME 0x58ac50
void C_human::Force_CanWorking()
{
    action_state = 1;
}

// FUNCTION: GAME 0x58ac60
void C_human::Turn_Turn()
{
    next_anim_id = 0xa;
    unknown_43c = 0.0f;
    unknown_430.Zero();
    PersonAnim(0.0f, true, false);
}

// C_HUMAN 0x58ac60 Turn_Turn

// FUNCTION: GAME 0x58ac90
void C_human::Activate()
{
    dead = false;
    alive = true;
    rendered = false;
    frame->SetOn(true);
    pos = frame->GetPos();
    dir = frame->GetDir();
    act_state = ACTSTATE_0;
    unknown_410 = 0;
    action_state = 1;
    anim_machine.Init(this);
    next_anim_id = 1;
    memcpy(properties, max_properties, sizeof(properties));
    anim_id = 0;
    anim_set = 0;
    car = NULL;
    using_car = NULL;
    unknown_420.Zero();
    unknown_430 = S_vector(0.0f, 0.0f, 0.0f);
    unknown_42c = 0;
    fall = 0.0f;
    unknown_43c = 0.0f;
    idle_time = -1;
    unknown_710 = pos;
    breath_time = -1;
    unknown_22c = false;
    unknown_5e0 = false;
    unknown_22d = false;
    unknown_450 = 1500;
    agony_anim = -1;
    wingman = -1;
    unknown_460 = 0;
    fire_target = NULL;
    unknown_464 = 0;
    bleed_count = 0;
    unknown_6ee = 0;
    traffic = NULL;
    box = NULL;
    unknown_94 = 0;
    unknown_70c = 0;
    unknown_71c = 1.0f;
    unknown_6bc = 0;
    unknown_414 = 0;
    no_anim_hit = false;
    unknown_1ed = false;
    unknown_1fc = false;
    unknown_1fd = false;
    unknown_1fe = false;
    unknown_5e4 = 0;
    unknown_5e8 = 0;
    unknown_5ec = 0;
    unknown_5f0 = 0;
    unknown_5f4 = .5f;
    unknown_2b0 = false;
    unknown_2a0 = 0;
    unknown_23c[0].unknown_0 = 3;
    last_hit_time = 0;
    unknown_9ac = 1.0f;
    unknown_9b0 = 1.0f;
    unknown_9b4 = 1.0f;
    unknown_9b8 = 1.0f;
    last_hit_render = 0;
    idle_anims = false;
    unknown_22e = true;
    unknown_6d0 = -1;
    dead_body = NULL;
    dead_body_human = 0;
    dead_body_time = -1;
    unknown_a35 = false;
    killed_by_shot = false;
    unknown_a36 = false;
    stealing_car = NULL;
    unknown_a58 = -1;
    unknown_a60 = 0;
    unknown_a6c = -1;
    unknown_a70 = NULL;
    killer = NULL;
    unknown_22f = false;
    unknown_230 = false;
    unknown_9a0 = 1;
    unknown_b0 = 0;
    unknown_b4 = 0;
    unknown_b8 = -1;
    speed_limiter = false;
    unknown_234 = -1;
    score_bonus = false;
    unknown_a7a = false;
    unknown_a0 = 0;
    unknown_580 = false;
    unknown_581 = false;
    shut_up = false;
    ab_created = true;
    unknown_220 = 0;
    locked_car = NULL;
    locked_vehicle = NULL;
    CreateDynColls();
    for (int i = anims.size(); i--;)
        delete[] anims[i].name;
    anims.clear();
    aimed = false;
    unknown_21c = false;
    unknown_1ec = false;
    crouching = false;
    unknown_228 = 0;
    unknown_1e8 = 0;
    ground_material = 0;
    tilt = .5f;
    shadow = act_mission->GetGame()->AddFrameForShadow(frame, SHADOW_TYPE_0, SHADOW_TYPE_0, NULL, NULL);
    unknown_a88 = 0;
    fire_action = _fire_action_ready_;
    fire_step = 0;
    fire_time = 0;
    inventory.Clear();
    inventory.SetCanHaveBigWeapon(true, NULL);
    ChangeWeaponModel();
    PoseValidate(true);
    gifCollision.DynSetNextCollOwnerMult(colls, num_colls, NULL);
    S_vector ground;
    g_collision_header *coll = (g_collision_header *)gifCollision.TestLineV(S_vector(pos.x, pos.y + 1.0f, pos.z),
        S_vector(0.0f, -5.0f, 0.0f), &ground, NULL, COLF_UNKNOWN_2, 0);
    gifCollision.DynSetNextCollOwner(NULL, NULL);
    if (coll)
    {
        pos = ground;
        ground_material = coll->material;
    }
    if (base_frame)
        act_mission->GetGame()->LightnessAdd(this, base_frame, 0, S_vector(0.0f, 1.0f, 0.0f));
    loc.Init(this);
    blob_log.Clear();
}


// FUNCTION: GAME 0x58b330
void C_human::Deactivate()
{
    frame->SetOn(false);
    act_mission->GetGame()->GetUsingObject().DelObject(&using_objects[0]);
    act_mission->GetGame()->GetUsingObject().DelObject(&using_objects[1]);
    if (orig_model)
    {
        _intern_ChangeModel(orig_model, true);
        delete[] orig_model;
        orig_model = NULL;
    }
    if (base_frame)
        act_mission->GetGame()->LightnessDel(base_frame);
    act_mission->GetGame()->DelFrameForShadow(frame, SHADOW_TYPE_0);
    anim_machine.StopAnims();
    PoseNullRotate();
    EraseDynColls();
    ResetWeaponEffects();
    unknown_581 = false;
    for (int i = anims.size(); i--;)
        delete[] anims[i].name;
    anims.clear();
    car = NULL;
    using_car = NULL;
    HideLegs((I3D_model *)frame, false);
}

// C_HUMAN 0x58b470 NewScream

// FUNCTION: GAME 0x58b690
void C_human::Do_FuckingBoxDrop()
{
    if (!box)
        return;
    if (Do_FuckingBoxCancel())
        return;
    box->LinkTo(act_mission->GetScene()->GetPrimarySector(), 1);
    act_mission->GetScene()->SetFrameSector(box);
    C_drop_in_weapon *weapon = new C_drop_in_weapon;
    if (weapon->Init(box))
    {
        S_THING_BOX thing_box;
        thing_box.weight = 10.0f;
        thing_box.unknown_10 = .45f;
        thing_box.velocity = dir;
        weapon->InitBox(&thing_box);
        weapon->item.id = 0;
        weapon->item.ammo_loaded = box_index;
        act_mission->GetGame()->AddTemporaryActor(weapon);
    }
    else
        weapon->Release();
    Do_FuckingBox(NULL);
}


// FUNCTION: GAME 0x58b7e0
void C_human::Do_DeadBodyDrop()
{
    if (!dead_body)
        return;
    Do_DeadBody(NULL);
    anim_id = 0;
    next_anim_id = 0xa8;
    PersonAnim(0.0f, true, false);
    SetWaitAnim();
}

// FUNCTION: GAME 0x58b870
void C_human::ResetActions()
{
    Do_Crouched(false);
    aimed = false;
    unknown_1ec = false;
    if (alive)
    {
        anim_id = 0;
        next_anim_id = 1;
        PersonAnim(0.0f, true, false);
    }
    PoseNullRotate();
    if (type == ACTOR_PLAYER)
    {
        ((C_player *)this)->fade_alpha = 1.0f;
        ((C_player *)this)->SetAlpha(1.0f);
    }
}

// C_HUMAN 0x58b870 ResetActions
// C_HUMAN 0x58b8d0 RecompileDeathPos
// C_HUMAN 0x58bf30 Unknown58bf30
// C_HUMAN 0x58c4f0 Unknown58c4f0
// C_HUMAN 0x58c980 Unknown58c980

// FUNCTION: GAME 0x58cec0
void C_human::ProcessDeathPos(uint time)
{
    int length = unknown_a54;
    if (length < 0 || unknown_a50 >= length)
        return;
    unknown_a50 += time;
    if (unknown_a50 > length)
        unknown_a50 = length;
    float t = (float)unknown_a50 / length;
    frame->SetPos(unknown_a38 + unknown_a44 * t);
    frame->Update();
}


// FUNCTION: GAME 0x58cfb0
bool C_human::IsThisCarStealing(C_car *car)
{
    if (car)
    {
        std::vector<C_actor *> &actors = act_mission->GetGame()->active_actors;
        for (int i = actors.size(); i--; )
        {
            C_actor *act = actors[i];
            if (act->type == ACTOR_PLAYER || act->type == ACTOR_ENTITY)
            {
                C_human *human = (C_human *)act;
                if (human->stealing_car == car || human->car == car || human->using_car == car)
                    return true;
            }
        }
    }
    return false;
}


// The models a hand holds, into the vector
static inline void AddHandModels(I3D_frame *hand, std::vector<I3D_frame *> &models)
{
    if (hand)
    {
        for (int i = hand->NumChildren(); i--; )
        {
            if (hand->GetChild(i)->GetType() == FRAME_MODEL)
                models.push_back(hand->GetChild(i));
        }
    }
}

// FUNCTION: GAME 0x58d020
void C_human::FreeHand()
{
    Do_Holster();
    std::vector<I3D_frame *> models;
    AddHandModels(frame->FindChildFrame("r_hand", 0xffff), models);
    AddHandModels(frame->FindChildFrame("l_hand", 0xffff), models);
    int i;
    for (i = models.size(); i--; )
    {
        act_mission->GetGame()->AddFramePlacement(models[i]);
        models[i]->LinkTo(act_mission->GetScene()->GetPrimarySector(), 1);
        act_mission->GetScene()->SetFrameSector(models[i]);
        C_public_physics *phys = new C_public_physics;
        if (phys->Init(models[i]))
        {
            S_THING_BOX thing_box;
            thing_box.weight = 5.0f;
            thing_box.unknown_10 = .45f;
            thing_box.velocity = S_vector(0.0f, -.1f, 0.0f);
            phys->InitBox(&thing_box);
            act_mission->GetGame()->AddTemporaryActor(phys);
            phys->unknown_c0 = 0x80000;
        }
        else
        {
            phys->Release();
            models[i]->SetOn(false);
        }
    }
}


// FUNCTION: GAME 0x58d270
void C_human::BezZasobnikuWeaponEffect(int i, I3D_model *model)
{
    if (i >= 9 && i <= 10)
    {
        // STRING: GAME 0x636e8c
        I3D_frame *frm = model->FindChildFrame("clip", 0xffff);
        if (frm && frm->GetType() == FRAME_VISUAL)
            frm->SetOn(false);
    }
}

// C_HUMAN 0x58d2c0 StartWeaponEffect
// C_HUMAN 0x58da70 TickWeaponEffect
// C_HUMAN 0x58df60 ResetWeaponEffects

// FUNCTION: GAME 0x58e210
void C_human::FakeWeaponEffect(I3D_frame *frm, int i, bool b)
{
    if (i == 9)
    {
        // STRING: GAME 0x636ea0
        I3D_frame *z = frm->FindChildFrame("zaver1", 0xffff);
        if (z)
        {
            z->SetPos(0.0f, 0.0f, -.04f);
            z->Update();
        }
    }
}


// Empty on the PC: its calls go to the empty function the linker folded it into (0x4aabe0)
void C_human::HideLegs(I3D_model *model, bool hide)
{
}


// FUNCTION: GAME 0x58e270
bool C_human::Can_Hit_FilterHumanType(C_human *shooter, C_human *human)
{
    // Whether a human of the first type can hit one of the second, by the types' bit numbers (PS2 name)
    // GLOBAL: GAME 0x636908
    static bool can_hit[11][11] = {
        {true, true, true, true, true, true, true, true, true, true, true},
        {true, false, true, true, true, true, true, true, true, true, true},
        {true, true, false, true, true, true, true, true, true, true, true},
        {true, true, true, false, false, true, true, true, true, true, true},
        {true, true, true, false, false, true, true, true, true, true, true},
        {true, true, true, true, true, false, true, true, true, true, true},
        {true, true, true, true, true, true, false, true, true, true, true},
        {true, true, true, true, true, true, true, true, true, true, true},
        {true, true, true, true, true, true, true, true, true, true, true},
        {true, true, true, true, true, true, true, true, true, true, true},
        {true, true, true, true, true, true, true, true, true, true, true},
    };
    uint type = shooter->human_type;
    if (!type)
        return true;
    int s = 0;
    while (!(type & 1))
    {
        type >>= 1;
        s++;
    }
    type = human->human_type;
    if (!type)
        return true;
    int h = 0;
    while (!(type & 1))
    {
        type >>= 1;
        h++;
    }
    if (s > 10 || h > 10)
        return true;
    bool result = can_hit[s][h];
    if (s == 1 && h == 0)
    {
        ai_character_log *log = ((C_entity *)shooter)->logs.GetMaxAngryCharacter();
        if (human != (log ? log->character : NULL))
            result = false;
    }
    return result;
}


// The stubs of the functions not written yet, after all their callers in the file

#pragma auto_inline(off)

// STUB: GAME 0x56b1a0
int C_human::vfunc_0(int a)
{
    StubLog("C_human::vfunc_0");
    return C_actor::vfunc_0(a);
}

// STUB: GAME 0x56b670
void C_human::vfunc_1(int a)
{
    StubLog("C_human::vfunc_1");
    C_actor::vfunc_1(a);
}

// STUB: GAME 0x56b8d0
void C_human::vfunc_2(int a)
{
    StubLog("C_human::vfunc_2");
    C_actor::vfunc_2(a);
}

// STUB: GAME 0x56bec0
bool C_human::vfunc_3(int a)
{
    StubLog("C_human::vfunc_3");
    return C_actor::vfunc_3(a);
}

// STUB: GAME 0x56c6e0
void C_human::vfunc_12()
{
    StubLog("C_human::vfunc_12");
    C_actor::vfunc_12();
}

// STUB: GAME 0x56c7c0
S_vector C_human::vfunc_4(int a)
{
    StubLog("C_human::vfunc_4");
    return C_actor::vfunc_4(a);
}

// STUB: GAME 0x56cc70
S_vector C_human::vfunc_5(int a)
{
    StubLog("C_human::vfunc_5");
    return C_actor::vfunc_5(a);
}

// STUB: GAME 0x56d130
uint __stdcall C_human::Unknown56d130(I3D_frame *frm, LS3D_CALLBACK_MESSAGE message, uint param1, uint param2)
{
    StubLog("C_human::Unknown56d130");
    return 0;
}

// STUB: GAME 0x570740
void C_human::AI(uint time)
{
    StubLog("C_human::AI");
    C_actor::AI(time);
}

// STUB: GAME 0x5716d0
void C_human::Intern_UseCar(bool b)
{
    StubLog("C_human::Intern_UseCar(bool)");
}

// STUB: GAME 0x571b90
void C_human::Intern_UseRailway(bool b)
{
    StubLog("C_human::Intern_UseRailway");
}

// STUB: GAME 0x5721b0
void C_human::_intern_UseCarTruck(C_car_ex *car, bool unknown)
{
    StubLog("C_human::_intern_UseCarTruck");
}

// STUB: GAME 0x572530
void C_human::ThrowFromCarTruck(S_vector dir, float f)
{
    StubLog("C_human::ThrowFromCarTruck");
}

// STUB: GAME 0x578e20
void C_human::Unknown578e20(int a, int b)
{
    StubLog("C_human::Unknown578e20");
}

// STUB: GAME 0x578ee0
void C_human::PoseInterpolate(uint time)
{
    StubLog("C_human::PoseInterpolate");
}

// STUB: GAME 0x579630
void C_human::PoseSetPoseNormal(S_vector pos)
{
    StubLog("C_human::PoseSetPoseNormal");
}

// STUB: GAME 0x579ea0
void C_human::PoseSetPoseAimed(S_vector pos)
{
    StubLog("C_human::PoseSetPoseAimed");
}

// STUB: GAME 0x57a710
void C_human::Movement(uint time)
{
    StubLog("C_human::Movement");
}

// STUB: GAME 0x57b660
void C_human::MovementIntern(uint time)
{
    StubLog("C_human::MovementIntern");
}

// STUB: GAME 0x57cb60
int C_human::TestMoveColl(const S_vector &from, const S_vector &to, S_vector &result)
{
    StubLog("C_human::TestMoveColl");
    return false;
}

// STUB: GAME 0x57d0e0
int C_human::TestMoveCollInOwner(const S_vector &from, const S_vector &to, S_vector &result)
{
    StubLog("C_human::TestMoveCollInOwner");
    return false;
}

// STUB: GAME 0x57dd00
void C_human::ChangeAnim(char *anim1, char *anim2)
{
    StubLog("C_human::ChangeAnim");
}

// STUB: GAME 0x57e020
void C_human::Intern_UseCar(C_car *car, int seat)
{
    StubLog("C_human::Intern_UseCar");
}

// STUB: GAME 0x57e500
void C_human::Intern_FromCar(I3D_frame *frm)
{
    StubLog("C_human::Intern_FromCar");
}

// STUB: GAME 0x57e980
void C_human::RepairWeaponPosition()
{
    StubLog("C_human::RepairWeaponPosition");
}

// STUB: GAME 0x57ec20
void C_human::ChangeWeaponModel()
{
    StubLog("C_human::ChangeWeaponModel");
}

// STUB: GAME 0x57f550
void C_human::Do_ChangeWeapon(int i, bool b)
{
    StubLog("C_human::Do_ChangeWeapon");
}

// STUB: GAME 0x57faa0
bool C_human::DropOutItems(std::vector<S_GameItem> &items)
{
    StubLog("C_human::DropOutItems");
    return false;
}

// STUB: GAME 0x5802f0
bool C_human::Can_DropOutFromCar_Free(C_car *car, int i)
{
    StubLog("C_human::Can_DropOutFromCar_Free");
    return false;
}

// STUB: GAME 0x5805c0
bool C_human::Can_DropOutFromCar(int i)
{
    StubLog("C_human::Can_DropOutFromCar");
    return false;
}

// STUB: GAME 0x5808c0
bool C_human::Can_EnterToCar(C_car *car, int i, bool b)
{
    StubLog("C_human::Can_EnterToCar");
    return false;
}

// STUB: GAME 0x580ca0
void C_human::ZasrateLezeniZAuta_ve2rano()
{
    StubLog("C_human::ZasrateLezeniZAuta_ve2rano");
}

// STUB: GAME 0x581220
void C_human::ZasrateVybliknutiZAuta_ve3rano()
{
    StubLog("C_human::ZasrateVybliknutiZAuta_ve3rano");
}

// STUB: GAME 0x581eb0
void C_human::Do_ClimbInCarLR()
{
    StubLog("C_human::Do_ClimbInCarLR");
}

// STUB: GAME 0x582180
bool C_human::Use_Actor(C_actor *act, int a, int b, int c)
{
    StubLog("C_human::Use_Actor");
    return false;
}

// STUB: GAME 0x583590
void C_human::Do_Shoot(bool b, S_vector *dir)
{
    StubLog("C_human::Do_Shoot");
}

// STUB: GAME 0x584010
bool C_human::Can_SpecialStroke()
{
    StubLog("C_human::Can_SpecialStroke");
    return false;
}

// STUB: GAME 0x584620
void C_human::Shooting()
{
    StubLog("C_human::Shooting");
}

// STUB: GAME 0x585b40
void C_human::Do_Reload()
{
    StubLog("C_human::Do_Reload");
}

// STUB: GAME 0x585c60
void C_human::Do_Holster()
{
    StubLog("C_human::Do_Holster");
}

// STUB: GAME 0x585d90
void C_human::Do_WeaponDrop()
{
    StubLog("C_human::Do_WeaponDrop");
}

// STUB: GAME 0x585f40
bool C_human::GetBoundPoints2D(std::vector<S_vector2> &points, I3D_frame *frm)
{
    StubLog("C_human::GetBoundPoints2D");
    return C_actor::GetBoundPoints2D(points, frm);
}

// STUB: GAME 0x586190
void C_human::Do_Jump()
{
    StubLog("C_human::Do_Jump");
}

// STUB: GAME 0x586400
bool C_human::Do_Climb()
{
    StubLog("C_human::Do_Climb");
    return false;
}

// STUB: GAME 0x587040
void C_human::Do_SideJump(const S_vector &v1, const S_vector &v2)
{
    StubLog("C_human::Do_SideJump");
}

// STUB: GAME 0x587190
void C_human::_intern_ChangeModel(char *name, bool unknown)
{
    StubLog("C_human::_intern_ChangeModel");
}

// STUB: GAME 0x5877a0
void C_human::InitFFEffects()
{
    StubLog("C_human::InitFFEffects");
}

// STUB: GAME 0x587880
void C_human::DoneFFEffects()
{
    StubLog("C_human::DoneFFEffects");
}

// STUB: GAME 0x5878d0
void C_human::_intern_ForceDeath()
{
    StubLog("C_human::_intern_ForceDeath");
}

// STUB: GAME 0x587d70
void C_human::Do_ThrowCocotFromCar(C_car *car, int i)
{
    StubLog("C_human::Do_ThrowCocotFromCar");
}

// STUB: GAME 0x588b40
void C_human::_intern_ThrowMeFromCar()
{
    StubLog("C_human::_intern_ThrowMeFromCar");
}

// STUB: GAME 0x589140
void C_human::Do_FuckingBox(I3D_frame *frm)
{
    StubLog("C_human::Do_FuckingBox");
}

// STUB: GAME 0x589680
bool C_human::Do_FuckingBoxCancel()
{
    StubLog("C_human::Do_FuckingBoxCancel");
    return false;
}

// STUB: GAME 0x589c20
void C_human::Do_DeadBody(I3D_frame *frm)
{
    StubLog("C_human::Do_DeadBody");
}

// STUB: GAME 0x58a5a0
void C_human::EineMeineKleineAutoInLuft(C_car *car)
{
    StubLog("C_human::EineMeineKleineAutoInLuft");
}

// STUB: GAME 0x58a790
bool C_human::Can_Aimed()
{
    StubLog("C_human::Can_Aimed");
    return false;
}

// STUB: GAME 0x58b470
void C_human::NewScream(uint id)
{
    StubLog("C_human::NewScream");
}

// STUB: GAME 0x58b8d0
void C_human::RecompileDeathPos()
{
    StubLog("C_human::RecompileDeathPos");
}

// STUB: GAME 0x58bf30
void C_human::Unknown58bf30(int a)
{
    StubLog("C_human::Unknown58bf30");
}

// STUB: GAME 0x58c4f0
void C_human::Unknown58c4f0(int a)
{
    StubLog("C_human::Unknown58c4f0");
}

// STUB: GAME 0x58c980
void C_human::Unknown58c980(int a, int b, int c, int d, int e, int f, int g, int h, int i)
{
    StubLog("C_human::Unknown58c980");
}

// STUB: GAME 0x58d2c0
void C_human::StartWeaponEffect(int i)
{
    StubLog("C_human::StartWeaponEffect");
}

// STUB: GAME 0x58da70
void C_human::TickWeaponEffect(uint time)
{
    StubLog("C_human::TickWeaponEffect");
}

// STUB: GAME 0x58df60
void C_human::ResetWeaponEffects()
{
    StubLog("C_human::ResetWeaponEffects");
}

// STUB: GAME 0x51af20
void C_human::GetActorSpeed(S_vector &speed)
{
    StubLog("C_human::GetActorSpeed");
    C_actor::GetActorSpeed(speed);
}

#pragma auto_inline(on)
