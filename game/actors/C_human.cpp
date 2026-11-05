// The humans (PS2 C_human.cpp; the PC's file is the one at 0x56b180-0x58e34c, docs/game-files.md), written whole. The
// PC's starts with the cutscenes' virtual functions, their helpers and the animations' callback, then the constructor
// (0x56efd0), then PS2's functions in PS2's order (PS2's Hit* helpers and SetWaitAnim are inline). RecompileDeathPos's
// three static helpers come after their first callers, where VC6 puts a static function it doesn't inline.

#include "actors/C_human.h"

#include "I3D_cache.h"
#include "g_material.h"
#include "ai/jp_ai_entity.h"
#include "input/C_ff_manager.h"
#include "ijoy.h"
#include "mission/C_effects.h"
#include "ui/g_indicators.h"
#include "ui/g_loadsave.h"
#include "ui/racing.h"
#include "vehicles/C_car.h"

#include "ai/C_person_traffic.h"
#include "collision/g_colls.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "mission/m_cache.h"
#include "vehicles/C_car_ex.h"
#include "vehicles/C_railway.h"
#include "vehicles/C_trf_c.h"
#include "vehicles/C_railway.h"

#include "ai/jp_ai_main.h"
#include "ai/pathweb.h"
#include "actors/C_player.h"

#include <math.h>

#include "actors/C_drop_in_weapon.h"
#include "actors/C_bridge.h"
#include "actors/C_crash_obj.h"
#include "actors/C_public_physics.h"
#include "mission/C_recorder.h"
#include "actors/C_door.h"
#include "vehicles/C_trf_c.h"
#include "vehicles/C_turnout.h"
#include "stub.h"

// Adds the human to the cutscene, with the sizes of its three kinds of records (C_actor's: 0x5c, 0, 0)
// FUNCTION: GAME 0x56b180
bool C_human::AddToCutscene()
{
    cutscene.AddActor(this, 0x28, 0x2c, 0x38);
    return true;
}


// Records the human's frame, animation and link at the time where the last record is old enough (100 ms, 40 ms after a
// new pose), a pose was set or a link is new: the record's type, -1 for none
// FUNCTION: GAME 0x56b1a0
int C_human::RecordKey(int time)
{
    bool force = false;
    if (records->cur)
    {
        int dt = time - records->last_key_time;
        if (dt >= 100)
            force = true;
        if ((pose_flags & HUMAN_POSEF_NEW) && dt >= 40)
            force = true;
        if (pose_flags & HUMAN_POSEF_UNKNOWN_8)
            force = true;
    }
    else
        force = true;
    S_cutscene_link *link = cutscene_link;
    if ((!link || !link->is_new) && !force)
        return -1;
    I3D_frame *frm = frame;
    S_vector pos = frm->GetPos();
    S_quat rot = frm->GetRot();
    uint link_key = 0;
    if (link && link->is_new)
    {
        if (strcmp(link->name, ""))
        {
            int id = cutscene.AddAnimName(link->name);
            if (id != -1)
                link_key = ((link->kind & 0xf) << 8 | id & 0xff) << HUMAN_REC_LINK_ID_SHIFT;
        }
        else
            link_key = (link->kind & 0xf) << HUMAN_REC_LINK_KIND_SHIFT | HUMAN_REC_LINK_NONE << HUMAN_REC_LINK_ID_SHIFT;
        link->is_new = false;
    }
    S_human_record *rec;
    switch (pose_flags)
    {
    case 0:
        rec = (S_human_record *)AddRecord(records, HUMAN_REC_TYPE_PLAIN);
        rec->time = time;
        rec->type = HUMAN_REC_TYPE_PLAIN;
        rec->pos = pos;
        rec->rot = rot;
        rec->anim = anim_id & HUMAN_REC_ANIM | link_key;
        return rec->type;
    case HUMAN_POSEF_NULL_ROTATE:
        rec = (S_human_record *)AddRecord(records, HUMAN_REC_TYPE_PLAIN);
        rec->time = time;
        rec->type = HUMAN_REC_TYPE_PLAIN;
        rec->pos = pos;
        rec->rot = rot;
        rec->anim = anim_id & HUMAN_REC_ANIM | link_key | HUMAN_POSEF_NULL_ROTATE << HUMAN_REC_POSE_SHIFT;
        return rec->type;
    case HUMAN_POSEF_AIMED:
        rec = (S_human_record *)AddRecord(records, HUMAN_REC_TYPE_PLAIN);
        rec->time = time;
        rec->type = HUMAN_REC_TYPE_PLAIN;
        rec->pos = pos;
        rec->rot = rot;
        rec->anim = anim_id & HUMAN_REC_ANIM | link_key | HUMAN_POSEF_AIMED << HUMAN_REC_POSE_SHIFT;
        return rec->type;
    case HUMAN_POSEF_UNKNOWN_8 | HUMAN_POSEF_AIMED:
        rec = (S_human_record *)AddRecord(records, HUMAN_REC_TYPE_POSED);
        rec->time = time;
        rec->type = HUMAN_REC_TYPE_POSED;
        rec->pos = pos;
        rec->rot = rot;
        rec->anim = anim_id & HUMAN_REC_ANIM | link_key | HUMAN_POSEF_AIMED << HUMAN_REC_POSE_SHIFT;
        rec->pose_pos = pose_pos;
        pose_flags &= ~HUMAN_POSEF_UNKNOWN_8;
        return rec->type;
    case HUMAN_POSEF_AIMED | HUMAN_POSEF_NEW:
        rec = (S_human_record *)AddRecord(records, HUMAN_REC_TYPE_POSED);
        rec->time = time;
        rec->type = HUMAN_REC_TYPE_POSED;
        rec->pos = pos;
        rec->rot = rot;
        rec->anim = anim_id & HUMAN_REC_ANIM | link_key |
            (HUMAN_POSEF_AIMED | HUMAN_POSEF_NEW) << HUMAN_REC_POSE_SHIFT;
        rec->pose_pos = pose_pos;
        pose_flags &= ~(HUMAN_POSEF_NULL_ROTATE | HUMAN_POSEF_NEW);
        return rec->type;
    case HUMAN_POSEF_NEW:
        rec = (S_human_record *)AddRecord(records, HUMAN_REC_TYPE_POSED);
        rec->time = time;
        rec->type = HUMAN_REC_TYPE_POSED;
        rec->pos = pos;
        rec->rot = rot;
        rec->anim = anim_id & HUMAN_REC_ANIM | link_key | HUMAN_POSEF_NEW << HUMAN_REC_POSE_SHIFT;
        rec->pose_pos = pose_pos;
        pose_flags &= ~(HUMAN_POSEF_NULL_ROTATE | HUMAN_POSEF_NEW);
        return rec->type;
    case HUMAN_POSEF_UNKNOWN_8 | HUMAN_POSEF_AIMED | HUMAN_POSEF_NEW:
        rec = (S_human_record *)AddRecord(records, HUMAN_REC_TYPE_POSED);
        rec->time = time;
        rec->type = HUMAN_REC_TYPE_POSED;
        rec->pos = pos;
        rec->rot = rot;
        rec->anim = anim_id & HUMAN_REC_ANIM | link_key |
            (HUMAN_POSEF_AIMED | HUMAN_POSEF_NEW) << HUMAN_REC_POSE_SHIFT;
        rec->pose_pos = pose_pos;
        pose_flags &= ~(HUMAN_POSEF_UNKNOWN_8 | HUMAN_POSEF_NULL_ROTATE | HUMAN_POSEF_NEW);
        return rec->type;
    }
    return -1;
}


// Plays the current record's animation: the human's own through PersonAnim, a cutscene's on the model's other channel
// (cutscene_channel_0 switches them), from the time it was recorded
// FUNCTION: GAME 0x56b670
void C_human::PlayRecord(int time)
{
    S_human_record *rec = (S_human_record *)records->cur;
    S_human_record *timed = NULL;
    if (rec->type == HUMAN_REC_TYPE_TIMED)
        timed = rec;
    if (rec->type == HUMAN_REC_TYPE_POSED)
        timed = rec;
    next_anim_id = rec->anim & HUMAN_REC_ANIM;
    uint flags = rec->anim & HUMAN_REC_ANIM_FLAGS;
    if (next_anim_id < HUMAN_REC_ANIM_CUTSCENE)
    {
        if (flags & HUMAN_REC_ANIM_RESET)
            anim_machine.Reset();
        PersonAnim(0.0f, true, false);
        return;
    }
    I3D_model *model = (I3D_model *)frame;
    S_cutscene_anim *anim = cutscene.GetAnimByIndex(next_anim_id - HUMAN_REC_ANIM_CUTSCENE);
    if (!anim)
        return;
    if (anim_id == next_anim_id && !timed)
        return;
    cutscene_channel_0 = !cutscene_channel_0;
    if (cutscene_channel_0)
    {
        if (flags & HUMAN_REC_ANIM_RESET)
            cutscene_blend = 1.0f;
        else
            cutscene_blend = 0.0f;
        if (flags & HUMAN_REC_ANIM_LOOP)
            model->SetAnimation(anim->anim_set, 0, 1.0f, I3DANIMOP_LOOP);
        else
            model->SetAnimation(anim->anim_set, 0, 1.0f, I3DANIMOP_ONCE);
    }
    else
    {
        if (flags & HUMAN_REC_ANIM_RESET)
            cutscene_blend = 0.0f;
        else
            cutscene_blend = 1.0f;
        if (flags & HUMAN_REC_ANIM_LOOP)
            model->SetAnimation(anim->anim_set, 1, 1.0f, I3DANIMOP_LOOP);
        else
            model->SetAnimation(anim->anim_set, 1, 1.0f, I3DANIMOP_ONCE);
    }
    if ((flags & HUMAN_REC_ANIM_TCK) && anim->tck.length > 0)
    {
        tck_on = true;
        tck_time = 0;
    }
    else
        tck_on = false;
    cutscene_step = time - rec->time;
    cutscene_blending = true;
    if (timed)
    {
        int t = (timed->unknown_28 & 0xfff) * 40;
        if (cutscene_channel_0)
            model->SetAnimTime(0, t);
        else
            model->SetAnimTime(1, t);
    }
    if (flags & HUMAN_REC_ANIM_BASE)
    {
        int i = model->NumInterpolators() - 1;
        while (i--)
        {
            I3D_interpolator *ip = model->GetInterpolators()[i];
            if (ip->frame && !strcmp(ip->frame->GetOrigName(), "base"))
            {
                if (cutscene_channel_0)
                    model->GetInterpolators()[i]->ClearAnimation(1);
                else
                    model->GetInterpolators()[i]->ClearAnimation(0);
            }
        }
    }
    anim_id = next_anim_id;
}


// Plays the records for a step of the time (the first step after PlayRecord is cutscene_step's): the frame placed
// between the current record and the next by the cutscene's time, the two channels' blend moved, the .tck's offset
// and direction added, the model ticked; the frame's sector by its "base" frame
// FUNCTION: GAME 0x56b8d0
void C_human::PlayStep(int time)
{
    S_cutscene_anim *a = NULL;
    I3D_model *model = (I3D_model *)frame;
    S_human_record *rec = (S_human_record *)records->cur;
    S_human_record *next = (S_human_record *)records->next;
    int dt;
    if (cutscene_step > 0)
    {
        dt = cutscene_step;
        cutscene_step = -1;
    }
    else
        dt = time;
    S_vector pos;
    S_quat rot;
    S_vector offset, dir;
    if (cutscene.scaled_time > rec->time && !(rec->anim & HUMAN_REC_ANIM_NO_BLEND))
    {
        if (next != rec && cutscene.scaled_time < next->time)
        {
            float f = (float)(cutscene.scaled_time - rec->time) / (next->time - rec->time);
            pos = rec->pos * (1.0f - f) + next->pos * f;
            rot = rec->rot;
            rot = rot.Slerp(next->rot, f, true);
        }
        else
        {
            pos = next->pos;
            rot = next->rot;
        }
    }
    else
    {
        pos = rec->pos;
        rot = rec->rot;
    }
    next_anim_id = rec->anim & HUMAN_REC_ANIM;
    if (next_anim_id < HUMAN_REC_ANIM_CUTSCENE)
        PersonAnim(0.0f, true, false);
    else
        a = cutscene.GetAnimByIndex(next_anim_id - HUMAN_REC_ANIM_CUTSCENE);
    if (cutscene_blending)
    {
        if (cutscene_channel_0)
            cutscene_blend += dt * .004f;
        else
            cutscene_blend -= dt * .004f;
        if (cutscene_blend >= 1.0f)
        {
            model->StopAnimation(1);
            cutscene_blend = 1.0f;
            cutscene_blending = false;
        }
        if (cutscene_blend <= 0.0f)
        {
            model->StopAnimation(0);
            cutscene_blend = 0.0f;
            cutscene_blending = false;
        }
        ((I3D_model *)frame)->SetAnimBlend(0, cutscene_blend);
        ((I3D_model *)frame)->SetAnimBlend(1, 1.0f - cutscene_blend);
    }
    if (tck_on && a)
    {
        tck_time += dt;
        S_cutscene_tck &tck = a->tck;
        int key;
        float f;
        if ((uint)tck_time >= tck.length)
            offset = tck.positions[tck.positions.size() - 1];
        else
        {
            key = tck_time / tck.key_time;
            f = (float)(tck_time % tck.key_time) / tck.key_time;
            offset.x = (tck.positions[key + 1].x - tck.positions[key].x) * f + tck.positions[key].x;
            offset.y = (tck.positions[key + 1].y - tck.positions[key].y) * f + tck.positions[key].y;
            offset.z = (tck.positions[key + 1].z - tck.positions[key].z) * f + tck.positions[key].z;
        }
        if (tck.has_directions)
        {
            // the last key by the count of the positions (as the original)
            if ((uint)tck_time >= tck.length)
                dir = tck.directions[tck.positions.size() - 1];
            else
            {
                dir.x = (tck.directions[key + 1].x - tck.directions[key].x) * f + tck.directions[key].x;
                dir.y = (tck.directions[key + 1].y - tck.directions[key].y) * f + tck.directions[key].y;
                dir.z = (tck.directions[key + 1].z - tck.directions[key].z) * f + tck.directions[key].z;
            }
        }
        S_matrix m = rot.RotationMatrix();
        pos += offset * m;
        if (tck.has_directions)
            rot.SetDir(dir * m, 0.0f);
    }
    model->Tick(dt);
    model->Update();
    if (!car)
    {
        frame->SetPos(pos);
        frame->SetRot(rot);
        frame->Update();
    }
    if (base_frame)
        act_mission->scene->SetFrameSectorPos(frame, base_frame->GetWorldPos());
    else
    {
        act_mission->scene->SetFrameSectorPos(frame, frame->GetWorldPos() + S_vector(0.0f, .5f, 0.0f));
    }
}


// Places the human as the records give it for the time (a jump in the cutscene): the frame between the last record
// up to the time and the next, a cutscene animation from its start (and its .tck), then the last record's link and
// pose
// FUNCTION: GAME 0x56bec0
bool C_human::PlaceAtTime(int time)
{
    if (!records->first_block)
        return true;
    S_human_record *rec = (S_human_record *)(records->first_block + 8);
    int anim = rec->anim & HUMAN_REC_ANIM;
    uint flags = rec->anim & HUMAN_REC_ANIM_FLAGS;
    uint anim_time = rec->time;
    S_human_record *prev = rec;
    while (rec && rec->time <= records->last_key_time)
    {
        if (rec->time <= time)
        {
            prev = rec;
            if ((rec->anim & HUMAN_REC_ANIM) != anim)
            {
                anim = rec->anim & HUMAN_REC_ANIM;
                anim_time = rec->time;
            }
            if (rec->type == HUMAN_REC_TYPE_TIMED)
                anim_time = rec->time - (rec->unknown_28 & 0xfff) * 40;
            if ((rec->anim & HUMAN_REC_ANIM_FLAGS) != flags)
                flags = rec->anim & HUMAN_REC_ANIM_FLAGS;
        }
        if (rec->time >= records->last_key_time)
            break;
        rec = (S_human_record *)((uchar *)rec + records->record_sizes[rec->type]);
    }
    S_human_record *next = prev->time < records->last_key_time ?
        (S_human_record *)((uchar *)prev + records->record_sizes[prev->type]) : prev;
    S_human_record *posed = prev->type == HUMAN_REC_TYPE_POSED ? prev : NULL;
    S_vector pos;
    S_quat rot;
    S_vector offset, dir;
    if (time <= prev->time)
    {
        pos = prev->pos;
        rot = prev->rot;
    }
    else if (next != prev && time < next->time)
    {
        if (prev->anim & HUMAN_REC_ANIM_NO_BLEND)
        {
            pos = prev->pos;
            rot = prev->rot;
        }
        else
        {
            float f = (float)(time - prev->time) / (next->time - prev->time);
            pos = prev->pos * (1.0f - f) + next->pos * f;
            rot = prev->rot;
            rot = rot.Slerp(next->rot, f, true);
        }
    }
    else
    {
        pos = next->pos;
        rot = next->rot;
    }
    if (anim >= HUMAN_REC_ANIM_CUTSCENE)
    {
        I3D_model *model = (I3D_model *)frame;
        S_cutscene_anim *a = cutscene.GetAnimByIndex(anim - HUMAN_REC_ANIM_CUTSCENE);
        if (a)
        {
            if ((flags & HUMAN_REC_ANIM_TCK) && a->tck.length > 0)
                tck_on = true;
            else
                tck_on = false;
            int t;
            if (flags & HUMAN_REC_ANIM_LOOP)
                t = (time - anim_time) % a->anim_set->GetEndTime();
            else
                t = time - anim_time;
            model->StopAnimation(1);
            model->SetAnimation(a->anim_set, 0, 1.0f, 0);
            model->SetAnimTime(0, t);
            if (tck_on)
            {
                S_cutscene_tck &tck = cutscene.GetAnimByIndex(anim - HUMAN_REC_ANIM_CUTSCENE)->tck;
                tck_time = t;
                int key;
                float f;
                if ((uint)tck_time >= tck.length)
                    offset = tck.positions[tck.positions.size() - 1];
                else
                {
                    key = tck_time / tck.key_time;
                    f = (float)(tck_time % tck.key_time) / tck.key_time;
                    offset.x = (tck.positions[key + 1].x - tck.positions[key].x) * f + tck.positions[key].x;
                    offset.y = (tck.positions[key + 1].y - tck.positions[key].y) * f + tck.positions[key].y;
                    offset.z = (tck.positions[key + 1].z - tck.positions[key].z) * f + tck.positions[key].z;
                }
                if (tck.has_directions)
                {
                    // the last key by the count of the positions (as the original)
                    if ((uint)tck_time >= tck.length)
                        dir = tck.directions[tck.positions.size() - 1];
                    else
                    {
                        dir.x = (tck.directions[key + 1].x - tck.directions[key].x) * f + tck.directions[key].x;
                        dir.y = (tck.directions[key + 1].y - tck.directions[key].y) * f + tck.directions[key].y;
                        dir.z = (tck.directions[key + 1].z - tck.directions[key].z) * f + tck.directions[key].z;
                    }
                }
                S_matrix m = rot.RotationMatrix();
                pos += offset * m;
                if (tck.has_directions)
                    rot.SetDir(dir * m, 0.0f);
            }
        }
        model->Tick(0);
        model->Update();
    }
    frame->SetPos(pos);
    frame->SetRot(rot);
    frame->Update();
    if (base_frame)
        act_mission->scene->SetFrameSectorPos(frame, base_frame->GetWorldPos());
    else
        act_mission->scene->SetFrameSectorPos(frame, frame->GetWorldPos() + S_vector(0.0f, .5f, 0.0f));
    int kind = prev->anim >> HUMAN_REC_LINK_KIND_SHIFT & 0xf;
    if (kind)
    {
        int id = prev->anim >> HUMAN_REC_LINK_ID_SHIFT & 0xff;
        if (id == HUMAN_REC_LINK_NONE)
            SetCutsceneLink(NULL, kind);
        else
        {
            id += 0x100;
            char name[64];
            strcpy(name, cutscene.GetAnimName(id));
            C_actor *act = (C_actor *)cutscene.GetAnimValue(id);
            if (!act)
            {
                for (uint i = 0; i < act_mission->actors.size(); i++)
                {
                    if (!strcmp(act_mission->actors[i]->frame->GetName(), name))
                    {
                        act = act_mission->actors[i];
                        cutscene.SetAnimValue(id, (int)act);
                        break;
                    }
                }
            }
            if (act)
                SetCutsceneLink(act, kind);
        }
    }
    switch (prev->anim >> HUMAN_REC_POSE_SHIFT)
    {
    case HUMAN_POSEF_NULL_ROTATE:
        PoseNullRotate();
        return true;
    case HUMAN_POSEF_AIMED | HUMAN_POSEF_NEW:
        PoseSetPoseAimed(posed->pose_pos);
        break;
    case HUMAN_POSEF_NEW:
        PoseSetPoseNormal(posed->pose_pos);
        return true;
    case HUMAN_POSEF_UNKNOWN_8 | HUMAN_POSEF_AIMED | HUMAN_POSEF_NEW:
        PoseSetPoseAimed(posed->pose_pos);
        break;
    }
    return true;
}


// Writes the records to "<frame's name>.dump": the last key's time, then each record's time and animation up to it
// FUNCTION: GAME 0x56c6e0
void C_human::DumpRecords()
{
    int i = 0;
    char name[32];
    // STRING: GAME 0x6369cc
    sprintf(name, "%s.dump", frame->GetName());
    // STRING: GAME 0x6369c8
    FILE *f = fopen(name, "wt");
    // STRING: GAME 0x6369b4
    fprintf(f, "lastKeyTime = %d\n", records->last_key_time);
    S_human_record *rec = (S_human_record *)(records->first_block + 8);
    while (rec->time < records->last_key_time)
    {
        // STRING: GAME 0x636984
        fprintf(f, "key[%d](%d):   animid = %d      animflags = %d\n", i++, rec->time, rec->anim & HUMAN_REC_ANIM,
            rec->anim & HUMAN_REC_ANIM_FLAGS);
        rec = (S_human_record *)((uchar *)rec + records->record_sizes[rec->type]);
    }
    fprintf(f, "key[%d](%d):   animid = %d      animflags = %d\n", i, rec->time, rec->anim & HUMAN_REC_ANIM,
        rec->anim & HUMAN_REC_ANIM_FLAGS);
    fclose(f);
}


// The position the records give for the time: the last record's up to it, blended towards the next, and with a
// cutscene animation's .tck the offset of its key, turned by the record's rotation (0 without records)
// FUNCTION: GAME 0x56c7c0
S_vector C_human::GetRecordPos(int time)
{
    bool blended = false;
    int anim_start = 0;
    if (!records->first_block)
        return S_vector(0.0f, 0.0f, 0.0f);
    S_human_record *rec = (S_human_record *)(records->first_block + 8);
    int anim = rec->anim & HUMAN_REC_ANIM;
    uint flags = rec->anim & HUMAN_REC_ANIM_FLAGS;
    int anim_time = rec->time;
    S_human_record *prev = rec;
    while (rec && rec->time <= records->last_key_time)
    {
        if (rec->time <= time)
        {
            prev = rec;
            if ((rec->anim & HUMAN_REC_ANIM) != anim)
            {
                anim = rec->anim & HUMAN_REC_ANIM;
                anim_time = rec->time;
            }
            if ((rec->anim & HUMAN_REC_ANIM_FLAGS) != flags)
                flags = rec->anim & HUMAN_REC_ANIM_FLAGS;
        }
        if (rec->time >= records->last_key_time)
            break;
        rec = (S_human_record *)((uchar *)rec + records->record_sizes[rec->type]);
    }
    S_human_record *next = prev->time < records->last_key_time ?
        (S_human_record *)((uchar *)prev + records->record_sizes[prev->type]) : prev;
    S_human_record *timed = prev->type == HUMAN_REC_TYPE_TIMED ? prev : NULL;
    if (prev->type == HUMAN_REC_TYPE_POSED)
        timed = prev;
    S_vector pos;
    S_quat rot;
    if (time <= prev->time)
    {
        pos = prev->pos;
        rot = prev->rot;
    }
    else if (next != prev && time < next->time)
    {
        float f = (float)(time - prev->time) / (next->time - prev->time);
        blended = true;
        pos = prev->pos * (1.0f - f) + next->pos * f;
        rot = prev->rot;
        rot = rot.Slerp(next->rot, f, true);
    }
    else
    {
        pos = next->pos;
        rot = next->rot;
    }
    if (anim >= HUMAN_REC_ANIM_CUTSCENE)
    {
        S_cutscene_anim *a = cutscene.GetAnimByIndex(anim - HUMAN_REC_ANIM_CUTSCENE);
        if (a)
        {
            if (timed)
                anim_start = (int)((float)a->anim_set->GetEndTime() * (timed->unknown_28 & 0xfff) * .01f);
            if ((flags & HUMAN_REC_ANIM_TCK) && a->tck.length > 0)
                tck_on = true;
            else
                tck_on = false;
            if (flags & HUMAN_REC_ANIM_LOOP)
                int length = a->anim_set->GetEndTime();
            if (tck_on)
            {
                S_cutscene_tck &tck = cutscene.GetAnimByIndex(anim - HUMAN_REC_ANIM_CUTSCENE)->tck;
                tck_time = anim_start - anim_time + time;
                S_vector offset;
                if ((uint)tck_time >= tck.length)
                    offset = tck.positions[tck.positions.size() - 1];
                else
                {
                    int key = tck_time / tck.key_time;
                    int rest = tck_time % tck.key_time;
                    if (!key && !rest)
                        offset = S_vector(0.0f, 0.0f, 0.0f);
                    else
                    {
                        float f = (float)rest / tck.key_time;
                        offset.x = (tck.positions[key + 1].x - tck.positions[key].x) * f + tck.positions[key].x;
                        offset.y = (tck.positions[key + 1].y - tck.positions[key].y) * f + tck.positions[key].y;
                        offset.z = (tck.positions[key + 1].z - tck.positions[key].z) * f + tck.positions[key].z;
                    }
                }
                if (blended)
                {
                    pos = prev->pos;
                    rot = prev->rot;
                }
                pos += offset * rot.RotationMatrix();
            }
        }
    }
    return pos;
}


// The rotation the records give for the time: the last record's up to it, blended towards the next, and with a
// cutscene animation's .tck that has directions turned to its key's direction (0 without records)
// FUNCTION: GAME 0x56cc70
S_quat C_human::GetRecordRot(int time)
{
    bool blended = false;
    int anim_start = 0;
    if (!records->first_block)
        return S_quat(0.0f, 0.0f, 0.0f, 0.0f);
    S_human_record *rec = (S_human_record *)(records->first_block + 8);
    int anim = rec->anim & HUMAN_REC_ANIM;
    uint flags = rec->anim & HUMAN_REC_ANIM_FLAGS;
    int anim_time = rec->time;
    S_human_record *prev = rec;
    while (rec && rec->time <= records->last_key_time)
    {
        if (rec->time <= time)
        {
            prev = rec;
            if ((rec->anim & HUMAN_REC_ANIM) != anim)
            {
                anim = rec->anim & HUMAN_REC_ANIM;
                anim_time = rec->time;
            }
            if ((rec->anim & HUMAN_REC_ANIM_FLAGS) != flags)
                flags = rec->anim & HUMAN_REC_ANIM_FLAGS;
        }
        if (rec->time >= records->last_key_time)
            break;
        rec = (S_human_record *)((uchar *)rec + records->record_sizes[rec->type]);
    }
    S_human_record *next = prev->time < records->last_key_time ?
        (S_human_record *)((uchar *)prev + records->record_sizes[prev->type]) : prev;
    S_human_record *timed = prev->type == HUMAN_REC_TYPE_TIMED ? prev : NULL;
    if (prev->type == HUMAN_REC_TYPE_POSED)
        timed = prev;
    S_quat rot;
    if (time <= prev->time)
        rot = prev->rot;
    else if (next != prev && time < next->time)
    {
        float f = (float)(time - prev->time) / (next->time - prev->time);
        blended = true;
        rot = prev->rot;
        rot = rot.Slerp(next->rot, f, true);
    }
    else
        rot = next->rot;
    if (anim >= HUMAN_REC_ANIM_CUTSCENE)
    {
        S_cutscene_anim *a = cutscene.GetAnimByIndex(anim - HUMAN_REC_ANIM_CUTSCENE);
        if (a)
        {
            if (timed)
                anim_start = (int)((float)a->anim_set->GetEndTime() * (timed->unknown_28 & 0xfff) * .01f);
            if ((flags & HUMAN_REC_ANIM_TCK) && a->tck.length > 0)
                tck_on = true;
            else
                tck_on = false;
            if (flags & HUMAN_REC_ANIM_LOOP)
                int length = a->anim_set->GetEndTime();
            if (tck_on)
            {
                S_cutscene_tck &tck = cutscene.GetAnimByIndex(anim - HUMAN_REC_ANIM_CUTSCENE)->tck;
                tck_time = anim_start - anim_time + time;
                int key, rest;
                float f;
                S_vector offset, dir;
                if ((uint)tck_time >= tck.length)
                    offset = tck.positions[tck.positions.size() - 1];
                else
                {
                    key = tck_time / tck.key_time;
                    rest = tck_time % tck.key_time;
                    if (!key && !rest)
                    {
                        offset = S_vector(0.0f, 0.0f, 0.0f);
                        dir = S_vector(0.0f, 0.0f, 1.0f);
                        goto placed;
                    }
                    f = (float)rest / tck.key_time;
                    offset.x = (tck.positions[key + 1].x - tck.positions[key].x) * f + tck.positions[key].x;
                    offset.y = (tck.positions[key + 1].y - tck.positions[key].y) * f + tck.positions[key].y;
                    offset.z = (tck.positions[key + 1].z - tck.positions[key].z) * f + tck.positions[key].z;
                }
                if (tck.has_directions)
                {
                    // the last key by the count of the positions (as the original)
                    if ((uint)tck_time >= tck.length)
                        dir = tck.directions[tck.positions.size() - 1];
                    else
                    {
                        dir.x = (tck.directions[key + 1].x - tck.directions[key].x) * f + tck.directions[key].x;
                        dir.y = (tck.directions[key + 1].y - tck.directions[key].y) * f + tck.directions[key].y;
                        dir.z = (tck.directions[key + 1].z - tck.directions[key].z) * f + tck.directions[key].z;
                    }
                }
            placed:
                if (blended)
                    rot = prev->rot;
                S_matrix m = rot.RotationMatrix();
                // turned as GetRecordPos's, and not used
                S_vector v = offset * m;
                if (tck.has_directions)
                    rot.SetDir(dir * m, 0.0f);
            }
        }
    }
    return rot;
}


// Whether the notes open and close the car's doors (PS2 name), which Use_Actor clears while it shifts the animation
extern bool do_snd_car_door;

// The notes of a human's animations, which the model's "notify" frame gets (PS2 name; GameInit and
// _intern_ChangeModel set it): the steps' sounds and tracks, the car's doors, the weapon from hand to hand, the grenade
// thrown, the weapons' effects, the screams
// FUNCTION: GAME 0x56d130
uint __stdcall human_notify_callback(I3D_frame *frm, LS3D_CALLBACK_MESSAGE message, uint param1, uint param2)
{
    if (!frm)
        return 0;
    C_actor **p = GetFrameActor(frm->GetParent());
    if (!p || !*p)
        return 0;
    C_human *human = (C_human *)*p;
    if (human->type != ACTOR_ENTITY && human->type != ACTOR_PLAYER)
        return 0;
    switch (param1) {
    case NOTE_STEP_R:
    case NOTE_STEP_L:
    {
        // A step's sound, at most every 300 ms, louder the faster he goes
        C_game *game = act_mission->GetGame();
        if (game->game_time - human->step_time <= 300)
            break;
        human->step_time = game->game_time;
        float volume = human->velocity.Magnitude() / human->properties[HUMAN_PROPERTY_SPEED] / 2.4f;
        if (volume > 1.0f)
            volume = 1.0f;
        else if (volume < .5f)
            volume = .5f;
        if (act_mission->options.game_sounds >= (human->type == ACTOR_PLAYER ? OPTION_MEDIUM : OPTION_HIGH) &&
            human->step_sound && materialList[human->ground_material].step_sound &&
            (act_mission->GetScene()->GetActiveCamera()->GetWorldPos() - human->frame->GetWorldPos()).Square() <
            625.0f) {
            if (!human->step_sound->IsPlaying(false))
                human->step_sound->SetOn(false);
            if (sound_table.ConstructSound(human->step_sound, materialList[human->ground_material].step_sound)) {
                human->step_sound->SetOn(true);
                human->step_sound->SetVolume(volume);
            }
        }
        S_vector pos = frm->GetWorldPos();
        act_mission->GetGame()->sound_arena.SoundEmit(pos, volume * 10.0f, .5f, 1, human);
        // The foot's track, while the steps leave one (cbCollTester's blood)
        if (!human->track_steps)
            break;
        human->track_steps--;
        I3D_frame *foot = human->frame->FindChildFrame(param1 == NOTE_STEP_R ?
            // STRING: GAME 0x63487c
            "r_foot" :
            // STRING: GAME 0x6369dc
            "l_foot", ENUMF_ALL);
        if (!foot)
            break;
        const S_vector &foot_pos = foot->GetWorldPos();
        S_vector track_pos(foot_pos.x, foot_pos.y + .3f, foot_pos.z);
        S_vector normal(0.0f, -1.0f, 0.0f);
        S_vector dir = human->frame->GetWorldDir();
        S_vector size(.08f, .16f, .5f);
        float alpha = (float)(human->track_steps + 2) / human->track_steps_max;
        if (alpha > 1.0f)
            alpha = 1.0f;
        act_mission->GetGame()->CreateTrackEffect(track_pos, normal, dir, size, 0.0f, human->track_texture, false,
            alpha, false, false, NULL);
        break;
    }
    case NOTE_FALL_LOWER:
    {
        if (act_mission->options.game_sounds < OPTION_HIGH)
            break;
        I3D_sound *sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
        if (!sound)
            break;
        if (human->anim_id >= ANIM_JUMPL1 && human->anim_id <= ANIM_JUMPR3)
            sound->SetVolume(.5f);
        else
            sound->SetVolume(1.0f);
        sound->SetPos(human->GetWorldPos() + S_vector(0.0f, 1.0f, 0.0f));
        sound_table.ConstructSound(sound, SOUND_FALL_LOWER);
        sound->SetOn(true);
        act_mission->GetGame()->Play3DSound(sound);
        break;
    }
    case NOTE_DOOR_OPEN:
        if (!do_snd_car_door)
            break;
        if (human->using_car && human->using_car->type == ACTOR_CAR)
            human->using_car->OpenDoor(human->seat, 1.0f, false);
        else if (human->car && human->car->type == ACTOR_CAR)
            human->car->OpenDoor(human->seat, 1.0f, false);
        else if (human->last_car && human->last_car->type == ACTOR_CAR)
            human->last_car->OpenDoor(human->seat, 1.0f, false);
        break;
    case NOTE_DOOR_CLOSE:
        if (!do_snd_car_door)
            break;
        if (human->using_car && human->using_car->type == ACTOR_CAR)
            human->using_car->OpenDoor(human->seat, 0.0f, false);
        else if (human->car && human->car->type == ACTOR_CAR)
            human->car->OpenDoor(human->seat, 0.0f, false);
        else if (human->last_car && human->last_car->type == ACTOR_CAR)
            human->last_car->OpenDoor(human->seat, 0.0f, false);
        break;
    case NOTE_FIRE_HOLD:
        human->fire_held = true;
        human->note_fire = true;
        break;
    case NOTE_FIRE_RELEASE:
        human->fire_held = false;
        human->note_fire = false;
        break;
    case NOTE_WEAPON_TO_TARGET:
        if ((human->weapon_kind == WEAPON_KIND_RIFLE || human->weapon_kind == WEAPON_KIND_SMG) &&
            human->left_hand_target && !human->loading_shells)
            ((I3D_target *)human->left_hand_target)->AddToTarget(human->weapon_model);
        break;
    case NOTE_WEAPON_FROM_TARGET:
        if (human->left_hand_target &&
            (human->weapon_kind == WEAPON_KIND_RIFLE || human->weapon_kind == WEAPON_KIND_SMG)) {
            ((I3D_target *)human->left_hand_target)->DeleteFromTarget(human->weapon_model);
            human->weapon_displaced = true;
        }
        break;
    case NOTE_WEAPON_TO_LEFT_HAND:
        if ((human->weapon_kind == WEAPON_KIND_RIFLE || human->weapon_kind == WEAPON_KIND_SMG) &&
            human->left_hand_target && human->weapon_model) {
            ((I3D_target *)human->left_hand_target)->DeleteFromTarget(human->weapon_model);
            human->weapon_model->LinkTo(human->left_hand, I3DLINK_UPDMATRIX);
            human->weapon_displaced = true;
        }
        break;
    case NOTE_WEAPON_TO_RIGHT_HAND:
        if (!human->loading_shells && human->left_hand_target && human->weapon_model && human->weapon_kind) {
            human->weapon_model->SetPos(S_vector(0.0f, 0.0f, 0.0f));
            human->weapon_model->SetDir(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
            human->weapon_model->LinkTo(human->right_hand, 0);
            human->weapon_model->Update();
            if (human->weapon_kind == WEAPON_KIND_RIFLE || human->weapon_kind == WEAPON_KIND_SMG)
                ((I3D_target *)human->left_hand_target)->AddToTarget(human->weapon_model);
        }
        break;
    case NOTE_CHANGE_WEAPON:
        if (human->changing_weapon)
            human->ChangeWeaponModel();
        break;
    case NOTE_WEAPON_SOUND:
        if (human->num_strike_frames) {
            human->striking = true;
            for (int i = 0; i < human->num_strike_frames; i++)
                human->strike_pos[i] = human->strike_frames[i]->GetWorldPos();
        }
        if (human->weapon_sound) {
            if (human->weapon_sound->IsPlaying(false))
                human->weapon_sound->SetOn(false);
            if (human->crouching)
                human->weapon_sound->SetPos(S_vector(0.0f, .6f, 1.0f));
            else
                human->weapon_sound->SetPos(S_vector(0.0f, 1.3f, 1.0f));
            human->weapon_sound->Update();
            sound_table.ConstructSound(human->weapon_sound, human->inventory.GetWeaponInRightHand()->unknown_54);
            human->weapon_sound->SetOn(true, true);
        }
        break;
    case NOTE_THROW:
    {
        // The grenade or the Molotov cocktail in the hand thrown along throw_dir
        if (!human->weapon_model)
            break;
        I3D_frame *grenade = driver->CreateFrame(FRAME_MODEL);
        grenade->Duplicate(human->weapon_model);
        S_vector pos = human->weapon_model->GetWorldPos();
        S_vector dir = human->throw_dir;
        dir.Normalize();
        pos += dir;
        G_Inventory &inv = human->inventory;
        act_mission->GetGame()->NewGrenade(inv.right_hand.id == GAMEITEM_GRENADE ? NEWGRENADE_GRENADE :
            NEWGRENADE_MOLOTOV, pos, human->throw_dir, true, grenade);
        inv.RemoveByIdx(0, INVENTORY_HAND);
        human->ChangeWeaponModel();
        break;
    }
    case NOTE_STAGE_BLEND_END:
        if (human->anim_machine.stage_blending) {
            if (human->anim_machine.unknown_18 > 0.0f)
                human->anim_machine.unknown_14 = 1.0f;
            else
                human->anim_machine.unknown_14 = 0.0f;
        }
        break;
    case WEAPON_EFFECT_OPEN:
    case WEAPON_EFFECT_CLOSE:
    case WEAPON_EFFECT_PUMP:
    case WEAPON_EFFECT_SHELLS:
    case WEAPON_EFFECT_NEW_CLIP:
        if (human->reload_effects || param1 == WEAPON_EFFECT_PUMP)
            human->StartWeaponEffect(param1);
        break;
    case NOTE_CAR_SIT:
    {
        if (act_mission->options.game_sounds < (human->type == ACTOR_PLAYER ? OPTION_MEDIUM : OPTION_HIGH))
            break;
        uint id = SOUND_CAR_SIT;
        if (human->car && human->car->type == ACTOR_CAR && human->car->IsFormula())
            id = SOUND_SPORTCAR_SIT;
        if (human->using_car && human->using_car->type == ACTOR_CAR && human->using_car->IsFormula())
            id = SOUND_SPORTCAR_SIT;
        I3D_sound *sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
        if (!sound)
            break;
        sound->SetPos(human->GetWorldPos() + S_vector(0.0f, 1.0f, 0.0f));
        sound_table.ConstructSound(sound, id);
        sound->SetOn(true);
        act_mission->GetGame()->Play3DSound(sound);
        break;
    }
    case NOTE_FALL_UPPER:
    {
        if (act_mission->options.game_sounds < OPTION_HIGH)
            break;
        I3D_sound *sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
        if (sound) {
            if (human->anim_id >= ANIM_JUMPL1 && human->anim_id <= ANIM_JUMPR3)
                sound->SetVolume(.3f);
            else
                sound->SetVolume(1.0f);
            sound->SetPos(human->GetWorldPos() + S_vector(0.0f, 1.0f, 0.0f));
            sound_table.ConstructSound(sound, SOUND_FALL_UPPER);
            sound->SetOn(true);
            act_mission->GetGame()->Play3DSound(sound);
        }
        if (human->rendered) {
            I3D_frame *back = human->frame->FindChildFrame("back2", ENUMF_ALL);
            if (back && act_mission->options.particles >= OPTION_HIGH)
                act_mission->effects->CreateParticle(PARTICLE_LANDING, back->GetWorldPos(),
                    S_vector(0.0f, 1.0f, 0.0f), 0, NULL);
        }
        break;
    }
    case NOTE_POLICE_SCREAM_1:
        switch (act_mission->GetGame()->shvestky.PocetPrestupku(act_mission->GetGame()->player ?
            act_mission->GetGame()->player : human)) {
        case 1:
            human->NewScream(VOICE_UNKNOWN_98);
            break;
        case 2:
            human->NewScream(VOICE_UNKNOWN_9B);
            break;
        case 3:
            human->NewScream(VOICE_UNKNOWN_A2);
            break;
        }
        break;
    case NOTE_POLICE_SCREAM_2:
        switch (act_mission->GetGame()->shvestky.PocetPrestupku(act_mission->GetGame()->player ?
            act_mission->GetGame()->player : human)) {
        case 1:
            human->NewScream(VOICE_UNKNOWN_99);
            break;
        case 2:
            human->NewScream(VOICE_UNKNOWN_9C);
            break;
        case 3:
            human->NewScream(VOICE_UNKNOWN_9C);
            break;
        }
        break;
    case NOTE_POLICE_SCREAM_3:
        switch (act_mission->GetGame()->shvestky.PocetPrestupku(act_mission->GetGame()->player ?
            act_mission->GetGame()->player : human)) {
        case 1:
            human->NewScream(VOICE_UNKNOWN_9A);
            break;
        case 2:
            human->NewScream(VOICE_UNKNOWN_9D);
            break;
        case 3:
            human->NewScream(VOICE_UNKNOWN_A3);
            break;
        }
        break;
    case NOTE_SCREAM_88:
        human->NewScream(VOICE_UNKNOWN_88);
        break;
    }
    return 0;
}


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
    if (((C_human *)owner)->fire_held)
        b.flags |= AIBLOB_FIRING;
    switch (((C_human *)owner)->anim_id & ANIM_BASE) {
    case ANIM_RUN1:
        b.flags |= AIBLOB_RUNNING;
        break;
    case ANIM_UNKNOWN_5E:
    case ANIM_JUMP1:
        b.flags |= AIBLOB_JUMPING;
        break;
    case ANIM_JUMPRX1:
        b.flags |= AIBLOB_JUMPING | AIBLOB_RUNNING;
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
    if (gifCollision.TestLineV(from, dir, &hit, NULL, COLF_ALL, 0))
        return hit.y - pos.y;
    return 0.0f;
}

// The members in their order, the two entries at 0x9c4 an inlined loop; the defaults are PS2's, in its order. The
// inventory's arrays are constructed out of line (vector constructor iterator) only with an inline call after them,
// which leaves no code: the pose position's S_vector constructor (0xa7c) is that call
// FUNCTION: GAME 0x56efd0
C_human::C_human()
{
    type = ACTOR_HUMAN;
    subtype = ACTORSUB_HUMAN;
    voice_type = HUMAN_VOICE_DEFAULT;
    max_properties[HUMAN_PROPERTY_STRENGTH] = 0.7f;
    max_properties[HUMAN_PROPERTY_ENERGY] = 100.0f;
    max_properties[HUMAN_PROPERTY_ENERGY_HAND_L] = 20.0f;
    max_properties[HUMAN_PROPERTY_ENERGY_HAND_R] = 20.0f;
    max_properties[HUMAN_PROPERTY_ENERGY_LEG_L] = 20.0f;
    max_properties[HUMAN_PROPERTY_ENERGY_LEG_R] = 20.0f;
    max_properties[HUMAN_PROPERTY_REACTIONS] = 0.7f;
    max_properties[HUMAN_PROPERTY_SPEED] = 1.0f;
    max_properties[HUMAN_PROPERTY_AGGRESIVITY] = 0.6f;
    max_properties[HUMAN_PROPERTY_INTELLIGENCE] = 0.8f;
    max_properties[HUMAN_PROPERTY_SHOOTING] = 0.7f;
    max_properties[HUMAN_PROPERTY_SIGHT] = 1.0f;
    max_properties[HUMAN_PROPERTY_HEARING] = 1.0f;
    max_properties[HUMAN_PROPERTY_DRIVING] = 0.8f;
    max_properties[HUMAN_PROPERTY_MASS] = 80.0f;
    max_properties[HUMAN_PROPERTY_15] = 0.5f;
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

// FUNCTION: GAME 0x56f440
bool C_human::SaveData(C_chunk *chunk)
{
    uchar version = HUMAN_DATA_VERSION;
    chunk->Write(&version, sizeof(version));
    chunk->Write(&human_type, sizeof(human_type));
    chunk->Write(&voice_type, sizeof(voice_type));
    chunk->Write(max_properties, sizeof(max_properties));
    return true;
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
// are; true for any version
// FUNCTION: GAME 0x56f4e0
bool C_human::LoadData(C_chunk *chunk)
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
        return true;
    }
    case 2:
        _ReadOldCharStruct(chunk, this);
        return true;
    case 3:
        chunk->Read(&human_type, sizeof(human_type));
        _ReadOldCharStruct(chunk, this);
        return true;
    case 4:
        chunk->Read(&human_type, sizeof(human_type));
        _ReadOldCharStructVer4(chunk, this);
        return true;
    case 5:
        chunk->Read(&human_type, sizeof(human_type));
        chunk->Read(max_properties, sizeof(max_properties));
        return true;
    case HUMAN_DATA_VERSION:
        chunk->Read(&human_type, sizeof(human_type));
        chunk->Read(&voice_type, sizeof(voice_type));
        chunk->Read(max_properties, sizeof(max_properties));
        return true;
    }
    return true;
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
    s->fire_held = fire_held;
    s->throw_dir = throw_dir;
    s->throw_time = throw_time;
    s->throw_swung = throw_swung;
    s->strike_time = strike_time;
    s->strike_index = strike_index;
    s->guarding = guarding;
    s->sniper_mode = sniper_mode;
    s->tilt = tilt;
    s->pose_moving = pose_moving;
    s->pose_back1_dir = pose_back1_dir;
    s->pose_back2_dir = pose_back2_dir;
    s->pose_neck_dir = pose_neck_dir;
    s->action_state = action_state;
    s->vehicle_action = vehicle_action;
    s->fall = fall;
    s->velocity = velocity;
    s->turn_speed = turn_speed;
    s->last_velocity = last_velocity;
    s->velocity_ramp = velocity_ramp;
    s->ground_normal = ground_normal;
    s->action_time = action_time;
    s->dead_time = dead_time;
    s->agony_anim = agony_anim;
    s->agony_count = agony_count;
    s->last_fire_time = last_fire_time;
    s->track_test = track_test;
    s->track_test_pos = track_test_pos;
    s->wagon = wagon;
    s->idle_time = idle_time;
    s->breath_time = breath_time;
    memcpy(s->properties, properties, sizeof(properties));
    memcpy(s->max_properties, max_properties, sizeof(max_properties));
    s->ground_material = ground_material;
    s->wingman = wingman;
    s->wingman_name = wingman_name;
    s->bleed_count = bleed_count;
    s->bleed_time = bleed_time;
    s->track_steps_max = track_steps_max;
    s->track_steps = track_steps;
    s->track_texture = track_texture;
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
    fire_held = s->fire_held;
    throw_dir = s->throw_dir;
    throw_time = s->throw_time;
    throw_swung = s->throw_swung;
    strike_time = s->strike_time;
    strike_index = s->strike_index;
    guarding = s->guarding;
    sniper_mode = s->sniper_mode;
    tilt = s->tilt;
    pose_moving = s->pose_moving;
    pose_back1_dir = s->pose_back1_dir;
    pose_back2_dir = s->pose_back2_dir;
    pose_neck_dir = s->pose_neck_dir;
    action_state = s->action_state;
    vehicle_action = s->vehicle_action;
    fall = s->fall;
    velocity = s->velocity;
    turn_speed = s->turn_speed;
    last_velocity = s->last_velocity;
    velocity_ramp = s->velocity_ramp;
    ground_normal = s->ground_normal;
    action_time = s->action_time;
    dead_time = s->dead_time;
    agony_anim = s->agony_anim;
    agony_count = s->agony_count;
    last_fire_time = s->last_fire_time;
    track_test = s->track_test;
    track_test_pos = s->track_test_pos;
    wagon = s->wagon;
    idle_time = s->idle_time;
    breath_time = s->breath_time;
    memcpy(properties, s->properties, sizeof(properties));
    memcpy(max_properties, s->max_properties, sizeof(max_properties));
    ground_material = s->ground_material;
    wingman = s->wingman;
    wingman_name = s->wingman_name;
    bleed_count = s->bleed_count;
    bleed_time = s->bleed_time;
    track_steps_max = s->track_steps_max;
    track_steps = s->track_steps;
    track_texture = s->track_texture;
    move_coll = 0;
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
        if (vehicle_action == HUMAN_VEHICLE_CAR_IN)
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
            vehicle_action = 0;
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
    if (act_state == ACTSTATE_ACTIVE && !colls_active)
        ActivateColls(HUMAN_COLLS_ALL);
    else if (act_state != ACTSTATE_ACTIVE && colls_active)
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
    velocity.Zero();
    last_velocity.Zero();
    velocity_ramp = 0.0f;
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
    if (step_sound)
        step_sound->SetOn(false, true);
    if (weapon_sound)
        weapon_sound->SetOn(false, true);
    if (reload_sound)
        reload_sound->SetOn(false, true);
    if (pump_sound)
        pump_sound->SetOn(false, true);
    if (voice)
        voice->SetOn(false, true);
    if (type == ACTOR_PLAYER && ((C_player *)this)->unknown_af8)
        ((C_player *)this)->unknown_af8->SetOn(false, true);
}


// The human's own frame of a tick (Update's, or C_entity's and C_player's): the car action Update put off, the fire's
// steps (the reload's animation, the cartridges, the ammunition reloaded), the car stealing's end, and the strikes: the
// line each striking frame moved along tested against the collisions, a hit's damage, sound and debris
// FUNCTION: GAME 0x570740
void C_human::AI(uint time)
{
    if (freeze_time > 0)
        return;

    // The car action Update put off
    if (next_car_action > HUMAN_CAR_ACTION_DUE - 1) {
        if (car_action_time -= time) {
            switch (next_car_action) {
            case HUMAN_CAR_ACTION_DUE + HUMAN_CAR_ACTION_CLIMB_OVER:
                Do_ClimbInCarLR();
                break;
            case HUMAN_CAR_ACTION_DUE + HUMAN_CAR_ACTION_GET_OUT:
                if (car && car->type == ACTOR_CAR && seat > -1)
                    Use_Actor(car, HUMAN_USE_OUT, 0, 0);
                break;
            }
            next_car_action = 0;
        }
    }
    if (!loading_shells && anim_machine.unknown_10e == 0xc && fire_action[fire_step].action == FIRE_ACTION_END)
        anim_machine.DelExtendedAnim(4.0f);

    // The car stealing given up
    if (stealing_car && action_state != HUMAN_ACTION_STEAL) {
        mafiaIndicators.show_flags &= ~INDF_THIEF;
        stealing_car = NULL;
        action_time = -1;
        if (weapon_model)
            weapon_model->SetOn(true);
        if (weapon_model2)
            weapon_model2->SetOn(true);
        act_mission->game->Stop3DSound(steal_sound, false, .001f);
        act_mission->game->shvestky.CarSteal(this, false, false);
        act_mission->game->camera.EnableFreeLook(false, false);
    }
    if (ratio_time >= 0) {
        hit_ratio = hits / shots;
        ratio_time -= time;
        if (ratio_time < 0) {
            ratio_time = 5000;
            shots = prev_shots = shots - prev_shots + 1.0f;
            hits = prev_hits = hits - prev_hits + 1.0f;
        }
    }

    // The fire's steps
    if (fire_action[fire_step].action != FIRE_ACTION_END) {
        fire_time += time;
        while (fire_time >= fire_action[fire_step].time) {
            int ammo;
            fire_time -= fire_action[fire_step].time;
            fire_step++;
            if (fire_action[fire_step].action == FIRE_ACTION_RELOAD && (inventory.GetAmmo(&ammo) || !ammo) &&
                !reload_forced)
                fire_step++;
            if (fire_action[fire_step].action == FIRE_ACTION_END) {
                S_GameItem *item;
                inventory.GetItemInRightHand(&item);
                if (item)
                    item->clip_out = 0;
                break;
            }
            switch (fire_action[fire_step].action) {
            case FIRE_ACTION_RELOAD:
                {
                    S_item_def *weapon = inventory.GetWeaponInRightHand();
                    reloading = true;
                    if (reload_sound)
                        reload_sound->SetOn(true);
                    if (!car || car->type != ACTOR_CAR || seat == -1) {
                        aimed = false;
                        idle_time = rnd(5000) + 5000;
                        breath_time = -1;
                        int gun = weapon->gun_anim;
                        char name[32];
                        if (crouching)
                            // STRING: GAME 0x6369fc
                            sprintf(name, "gun0%i drep Reload.i3d", gun);
                        else
                            // STRING: GAME 0x6369e4
                            sprintf(name, "gun0%i stoj Reload.i3d", gun);
                        if (gun != GUN_ANIM_SHOTGUN || anim_machine.unknown_10e != 0xc) {
                            I3D_animation_set *as = NULL;
                            anim_cache.Create(name, &as, NULL, NULL);
                            if (as) {
                                anim_machine.SetExtendedAnim(as, 1.0f, 4.0f, gun != GUN_ANIM_SHOTGUN);
                                if (gun == GUN_ANIM_SHOTGUN)
                                    anim_machine.unknown_10e = 0xc;
                                as->Release();
                            }
                        }
                    }
                }
                break;
            case FIRE_ACTION_CARTRIDGE:
                if (cartridge_frame && !reload_forced) {
                    bool close_range = false;
                    S_item_def *weapon = inventory.GetWeaponInRightHand();
                    if (weapon)
                        close_range = weapon->flags & ITEMF_CLOSE_RANGE;
                    act_mission->game->NewCartidge(cartridge_frame, dir, close_range, false);
                }
                break;
            case FIRE_ACTION_RESTART:
                fire_time = time;
                break;
            }
        }
    }

    // The weapon reloaded at the fire's end, the ammunition in reserve kept with the cheat
    if (reloading && fire_action[fire_step].action == FIRE_ACTION_END) {
        reloading = false;
        reload_forced = false;
        reload_effects = false;
        S_GameItem *item;
        uint id = inventory.GetItemInRightHand(&item);
        int spare;
        if ((bool)(act_mission->game->cheats & GAMEF_INFINITE_AMMO) && item)
            spare = item->ammo_spare;
        if (id == GAMEITEM_SHOTGUN)
            inventory.NabijJeden();
        else
            inventory.Nabij();
        if ((bool)(act_mission->game->cheats & GAMEF_INFINITE_AMMO) && item)
            item->ammo_spare = spare;
        if (act_mission->game->player == this) {
            int ammo;
            uint loaded = inventory.GetAmmo(&ammo);
            mafiaIndicators.PlayerSetAmmo(loaded, ammo);
        }
    }
    if (loading_shells) {
        if (!aimed)
            aimed = true;
        if (fire_action[fire_step].action == FIRE_ACTION_END) {
            if (inventory.right_hand.id == GAMEITEM_SHOTGUN && inventory.MuzeNabitJeden())
                Do_Reload();
            else {
                loading_shells = false;
                anim_machine.DelExtendedAnim(4.0f);
            }
        }
    }

    // The strikes
    if (!striking)
        return;
    gifCollision.DynSetNextCollOwnerMult(colls, num_colls, NULL);
    int sound = -1;
    for (int i = 0; i < num_strike_frames; i++) {
        S_vector cur = strike_frames[i]->GetWorldPos();
        S_vector d = cur - strike_pos[i];
        S_vector hit_pos, normal;
        g_collision_header *coll = (g_collision_header *)gifCollision.TestLineVInnerDyn(strike_pos[i], d, &hit_pos,
            &normal, COLF_UNKNOWN_1, 0);
        bool hit = coll != NULL;
        if (coll) {
            if (coll->flags & COLLHF_UNKNOWN_40) {
                // A moving object's: its actor hit
                C_actor *act = coll->owner;
                if (act) {
                    switch (act->type) {
                    case ACTOR_CAR:
                        if (this == act_mission->game->player)
                            ((C_player *)this)->SetCarDamage((C_car *)act);
                        sound = SOUND_STRIKE_CAR;
                        break;
                    case ACTOR_DROP_OUT:
                    case ACTOR_CRASH_OBJ:
                    case ACTOR_PUBLIC_PHYSICS:
                    case ACTOR_BOX_NEW:
                        {
                            int mat = ((C_PhThing *)act)->material;
                            if (mat > -1)
                                sound = materialList[mat].shot_sound;
                        }
                        break;
                    case ACTOR_PLAYER:
                    case ACTOR_DON_OCAS:
                    case ACTOR_ENTITY:
                        switch (inventory.right_hand.id) {
                        case GAMEITEM_NONE:
                        case GAMEITEM_KNUCKLEDUSTER:
                            sound = SOUND_STRIKE_PUNCH;
                            break;
                        case GAMEITEM_KNIFE:
                        case GAMEITEM_CROWBAR:
                        case GAMEITEM_BOTTLE:
                        case GAMEITEM_SWORD:
                            sound = SOUND_STRIKE_KNIFE;
                            break;
                        default:
                            sound = SOUND_STRIKE_BAT;
                        }
                        break;
                    }
                    float damage = 5.0f;
                    switch (inventory.right_hand.id) {
                    case GAMEITEM_NONE:
                        damage = 4.0f;
                        break;
                    case GAMEITEM_KNUCKLEDUSTER:
                        damage = 7.0f;
                        break;
                    case GAMEITEM_KNIFE:
                        damage = 6.0f;
                        break;
                    case GAMEITEM_BASEBALL_BAT:
                        damage = 10.0f;
                        break;
                    case GAMEITEM_SWORD:
                        damage = 50.0f;
                        break;
                    }
                    if (act->type == ACTOR_PLAYER || act->type == ACTOR_ENTITY) {
                        if (Can_Hit_FilterHumanType(this, (C_human *)act) &&
                            ((C_human *)act)->TestShotCollision(strike_pos[i], d, FLT_MAX)) {
                            damage *= properties[HUMAN_PROPERTY_STRENGTH];
                            if (strike_index == -1) {
                                damage *= 2.0f;
                                // From behind with these: the human's energy and more
                                if (inventory.right_hand.id == GAMEITEM_BASEBALL_BAT ||
                                    inventory.right_hand.id == GAMEITEM_KNIFE) {
                                    float angle = GetWorldDir().AngleTo(act->GetWorldDir());
                                    float angle_to = GetWorldDir().AngleTo(act->GetWorldPos() - GetWorldPos());
                                    if (angle < PI * .4f && angle_to < PI * .3f) {
                                        damage = ((C_human *)act)->properties[HUMAN_PROPERTY_ENERGY] + 1.0f;
                                        if (inventory.right_hand.id == GAMEITEM_BASEBALL_BAT)
                                            sound = SOUND_STRIKE_BACK;
                                    }
                                }
                            }
                            if (act_mission->options.particles >= OPTION_MEDIUM)
                                act_mission->effects->CreateParticle(SHOTPARTICLE_BODY, hit_pos, normal, 0, NULL);
                            act->Hit(HIT_STROKE, d, hit_pos, normal, damage, this, rnd(2) + HIT_PART_BODY, NULL);
                            if (act_mission->game->shvestky.active)
                                act_mission->game->shvestky.HumanBoxing(this);
                            striking = false;
                            break;
                        }
                        sound = -1;
                        hit = false;
                    } else {
                        if (act_mission->game->shvestky.active && act->type == ACTOR_DON_OCAS)
                            act_mission->game->shvestky.HumanBoxing(this);
                        act->Hit(HIT_STROKE, d, hit_pos, normal, damage * properties[HUMAN_PROPERTY_STRENGTH], this, 0,
                            coll->frame);
                        striking = false;
                        break;
                    }
                }
            } else {
                // The level's: crash objects, glass, the surface's sound
                S_item_def *weapon = inventory.GetWeaponInRightHand();
                if (coll->material == MAT_CRASH) {
                    if (weapon->kind == WEAPON_KIND_BAT) {
                        S_vector crash_dir = d;
                        crash_dir.Normalize();
                        crash_dir *= 2.0f;
                        float f = 5.0f;
                        switch (inventory.right_hand.id) {
                        case GAMEITEM_NONE:
                            f = 4.0f;
                            break;
                        case GAMEITEM_KNUCKLEDUSTER:
                            f = 7.0f;
                            break;
                        case GAMEITEM_KNIFE:
                            f = 6.0f;
                            break;
                        case GAMEITEM_BASEBALL_BAT:
                            f = 10.0f;
                            break;
                        }
                        float power = f * properties[HUMAN_PROPERTY_STRENGTH];
                        if (strike_index == -1)
                            power *= 2.0f;
                        CreateCrashModel(coll->frame, strike_pos[i], crash_dir, power);
                    }
                } else if (coll->material == MAT_GLASS) {
                    if (weapon->kind != WEAPON_KIND_KNIFE) {
                        coll->flags |= COLLHF_DISABLED;
                        S_vector glass_dir = d;
                        glass_dir.Normalize();
                        glass_dir *= .3f;
                        act_mission->game->glass.BreakGlass(coll->frame, &glass_dir, "lustr1.wav", true);
                    }
                } else {
                    switch (inventory.right_hand.id) {
                    case GAMEITEM_NONE:
                    case GAMEITEM_KNUCKLEDUSTER:
                        sound = materialList[coll->material].strike_sounds[2];
                        break;
                    case GAMEITEM_KNIFE:
                    case GAMEITEM_CROWBAR:
                    case GAMEITEM_BOTTLE:
                    case GAMEITEM_SWORD:
                        sound = materialList[coll->material].strike_sounds[1];
                        break;
                    default:
                        sound = materialList[coll->material].strike_sounds[0];
                    }
                }
                if (coll->material == MAT_MASLOVKA)
                    hit = false;
                striking = false;
            }
        }
        if (hit)
            act_mission->game->debris.GenerateDebris(strike_pos[i], 5, .02f, .5f, 2.0f, NULL);
        strike_pos[i] = cur;
    }
    gifCollision.DynSetNextCollOwner(NULL, NULL);

    // The strike's sound, on a free one of the shots' sounds
    if (sound > -1) {
        I3D_sound **sounds = act_mission->game->shoot_sounds;
        for (int k = 0; k < 10; k++) {
            if (sounds[k] && !sounds[k]->IsPlaying(false)) {
                if (sound_table.ConstructSound(sounds[k], sound)) {
                    sounds[k]->SetPos(pos);
                    act_mission->scene->SetFrameSector(sounds[k]);
                    sounds[k]->Update();
                    sounds[k]->SetOn(true, true);
                }
                break;
            }
        }
    }
}


// The end of getting into a car's seat (in: the car using_car, the human seated in it; Update's ends of animations,
// C_player::LockControls) or of getting out of it (the weapons back in hand, the items the inventory can't keep in a
// car dropped)
// FUNCTION: GAME 0x5716d0
void C_human::Intern_UseCar(bool in)
{
    if (in) {
        C_car *c = using_car;
        next_anim_id = !seat ? (c->IsFormula() ? ANIM_FORMULE_STATIV : ANIM_AUTO_RIDIC_STATIV) : ANIM_AUTO_SPOL_STATIV;
        PersonAnim(0.0f, true, false);
        car_stance_time = 250;
        car_stance = HUMAN_CARSTANCE_SEATED;
        unknown_b0 = 0;
        car = c;
        if (!seat) {
            if (type != ACTOR_PLAYER) {
                c->EnableSounds(false);
                c->SetEngineOn(true, false);
                c->EnableSounds(true);
            }
            c->SetSpeedLimit(speed_limiter ? 16 : 1000);
        }
        c->LockVehicle(false);
        c->SetEnterProtect1(seat, true);
        dir = S_vector(0.0f, 0.0f, 1.0f);
        const S_vector &seat_pos = c->v_seats[seat].pos;
        pos = seat_pos;
        frame->SetPos(seat_pos);
        frame->SetDir(dir, 0.0f);
        frame->Update();
        frame->LinkTo(car->frame, 0);
        action_state = HUMAN_ACTION_CAR;
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
            }
            act_mission->game->camera.LezeDoToyoty(false);
            // The car's lights for the player
            c->SetIndicatorAuto(true);
            c->SetLightUnknown800(true);
            c->SetLightUnknown8000(true);
            c->SetLight(false);
            c->SetLightUnknown10(false);
            c->SetLightUnknown20(false);
            c->invisible = false;
            c->SetDynamicCollsOn(true, c->GetUnknown1fac());
        }
        AIManager.police.PanoveKokotPremistujePrdel(this, NULL, false);
        using_car = NULL;
        HideLegs((I3D_model *)frame, true);
        vehicle_action = 0;
        if (next_car_action) {
            car_action_time = 250;
            next_car_action += HUMAN_CAR_ACTION_DUE;
        }
    } else {
        C_car *c = using_car;
        using_car = NULL;
        car = NULL;
        if (c)
            c->SetEnterProtect1(seat, true);
        if (act_mission->game->initialized)
            shadow = act_mission->GetGame()->AddFrameForShadow(frame, SHADOW_TYPE_0, SHADOW_TYPE_0, NULL, NULL);
        if (act_mission->GetGame()->player == this) {
            ((C_player *)this)->IndicatorSetAimed();
            act_mission->GetGame()->camera.LezeDoToyoty(false);
            act_mission->GetGame()->camera.SetCar(NULL);
        }
        {
            std::vector<S_GameItem> dropped;
            inventory.SetInCar(false, &dropped);
            if (dropped.size())
                DropOutItems(dropped);
            ChangeWeaponModel();
            if (weapon_model)
                weapon_model->SetOn(true);
            if (weapon_model2)
                weapon_model2->SetOn(true);
            fall = .1f;
        }
        vehicle_action = 0;
    }
}


// The end of getting into a rail vehicle (in: using_car, linked to the wagon's frame, his position made relative to it)
// or out of it (back in the scene at his place in the world); the animation machine's matrix moved with him
// FUNCTION: GAME 0x571b90
void C_human::Intern_UseRailway(bool in)
{
    if (in) {
        car = using_car;
        C_railway *rail = (C_railway *)car;
        using_car = NULL;
        S_matrix m = (wagon ? rail->wagon_frames[wagon - 1] : car->frame)->GetMatrix();
        m.Inverse(S_matrix(m));
        S_matrix anim_m = m;
        pos *= m;
        m(3).x = m(3).y = m(3).z = 0.0f;
        dir *= m;
        frame->LinkTo(wagon ? rail->wagon_frames[wagon - 1] : car->frame, 0);
        frame->SetPos(pos);
        frame->SetDir(dir, 0.0f);
        frame->Update();
        if (act_mission->game->initialized) {
            for (int i = num_colls; i--;) {
                if (colls[i])
                    gifCollision.DynGetIn(colls[i], &rail->dyn_colls[wagon]);
            }
        }
        anim_machine.RecompileMatrix(anim_m);
        I3D_frame *frm = wagon ? rail->wagon_frames[wagon - 1] : rail->frame;
        if (act_mission->game->player == this) {
            act_mission->game->camera.Link(frm);
            rail->RemoveOuterEntries();
            rail->AddInnerEntries();
        }
        rail->Unlock();
        AIManager.police.PanoveKokotPremistujePrdel(this, NULL, false);
        rail->SetRainDummy(true, wagon ? rail->wagon_frames[wagon - 1] : car->frame);
    } else {
        C_railway *rail = (C_railway *)car;
        car = NULL;
        using_car = NULL;
        S_matrix m = (wagon ? rail->wagon_frames[wagon - 1] : rail->frame)->GetMatrix();
        S_matrix anim_m = m;
        pos = frame->GetPos();
        dir = frame->GetDir();
        pos *= m;
        m(3).x = m(3).y = m(3).z = 0.0f;
        dir *= m;
        frame->SetPos(pos);
        frame->SetDir(dir, 0.0f);
        frame->Update();
        frame->LinkTo(act_mission->scene->primary_sector, 0);
        if (base_frame)
            act_mission->scene->SetFrameSectorPos(frame, base_frame->GetWorldPos());
        anim_machine.RecompileMatrix(anim_m);
        if (act_mission->game->initialized) {
            for (int i = num_colls; i--;) {
                if (colls[i])
                    gifCollision.DynGetOut(colls[i], &rail->dyn_colls[wagon]);
            }
            shadow = act_mission->GetGame()->AddFrameForShadow(frame, SHADOW_TYPE_0, SHADOW_TYPE_0, NULL, NULL);
        }
        if (act_mission->game->player == this) {
            act_mission->game->camera.Link(NULL);
            rail->RemoveInnerEntries();
            rail->AddOuterEntries();
            if (!act_mission->game->camera.unknown_22 && frame)
                frame->SetOn(true);
        }
        rail->Unlock();
        AIManager.police.PanoveKokotPremistujePrdel(this, rail, false);
        rail->SetRainDummy(false, wagon ? rail->wagon_frames[wagon - 1] : rail->frame);
    }
    vehicle_action = 0;
}


// Onto the truck's back (HUMAN_ENTERTOTRUCK) or off it: on, the human is placed at the truck's place for him, linked to
// its frame and put into its collisions; off, at its navigation point beside it, back in the scene
// FUNCTION: GAME 0x5721b0
void C_human::_intern_UseCarTruck(C_car_ex *truck, bool in)
{
    if (!truck || !truck->CanUseTruck() || box || dead_body)
        return;
    if (in) {
        using_car = NULL;
        car = truck;
        seat = -1;
        pos = truck->truck_pos;
        dir = truck->truck_dir;
        frame->LinkTo(truck->frame, 0);
        frame->SetPos(pos);
        frame->SetDir(dir, 0.0f);
        frame->Update();
        truck->DCAddPerson(this);
        if (act_mission->game->initialized) {
            for (int i = num_colls; i--;) {
                if (colls[i])
                    gifCollision.DynGetIn(colls[i], truck->dyn_colls);
            }
            act_mission->game->DelFrameForShadow(frame, SHADOW_TYPE_0);
            shadow = 0;
        }
        anim_machine.Reset();
        if (act_mission->game->player == this) {
            act_mission->game->camera.Link(truck->frame);
            act_mission->game->camera.SetMode(CAMERA_P_BEHIND_23, true);
        }
        AIManager.police.PanoveKokotPremistujePrdel(this, NULL, false);
        last_velocity.Zero();
        velocity_ramp = 0.0f;
        action_state = HUMAN_ACTION_STAND;
    } else if (car == truck) {
        car = NULL;
        using_car = NULL;
        S_vector p1, p2, p4;
        truck->GetNavigationPoints(.5f, p1, p2, pos, p4);
        pos *= truck->frame->GetMatrix();
        dir = truck->GetWorldDir();
        _get_delta_nodyn_y_(pos);
        frame->LinkTo(act_mission->scene->primary_sector, 0);
        frame->SetPos(pos);
        frame->SetDir(dir, 0.0f);
        frame->Update();
        truck->DCDelPerson(this);
        if (act_mission->game->initialized) {
            for (int i = num_colls; i--;) {
                if (colls[i])
                    gifCollision.DynGetOut(colls[i], truck->dyn_colls);
            }
            shadow = act_mission->GetGame()->AddFrameForShadow(frame, SHADOW_TYPE_0, SHADOW_TYPE_0, NULL, NULL);
        }
        if (act_mission->game->player == this)
            act_mission->game->camera.Link(NULL);
        AIManager.police.PanoveKokotPremistujePrdel(this, truck, false);
        last_velocity.Zero();
        velocity_ramp = 0.0f;
        action_state = HUMAN_ACTION_STAND;
    }
    vehicle_action = 0;
}


// Thrown off the truck's back (C_car_ex::Update's crashes), the hit's damage taken from the energy: back in the scene
// where he is, he falls to the side the throw comes from (PS2's takes the direction by reference)
// FUNCTION: GAME 0x572530
void C_human::ThrowFromCarTruck(S_vector dir, float damage)
{
    C_car_ex *truck = (C_car_ex *)car;
    if (!truck || truck->type != ACTOR_CAR || seat != -1)
        return;
    car = NULL;
    using_car = NULL;
    frame->LinkTo(act_mission->scene->primary_sector, I3DLINK_UPDMATRIX);
    pos = frame->GetPos();
    this->dir = frame->GetDir();
    frame->Update();
    truck->DCDelPerson(this);
    if (act_mission->game->initialized) {
        for (int i = num_colls; i--;) {
            if (colls[i])
                gifCollision.DynGetOut(colls[i], truck->dyn_colls);
        }
        shadow = act_mission->GetGame()->AddFrameForShadow(frame, SHADOW_TYPE_0, SHADOW_TYPE_0, NULL, NULL);
    }
    if (act_mission->game->player == this)
        act_mission->game->camera.Link(NULL);
    AIManager.police.PanoveKokotPremistujePrdel(this, truck, false);
    if ((properties[HUMAN_PROPERTY_ENERGY] -= damage) < .01f) {
        act_mission->game->DeathHumanTalk(this);
        properties[HUMAN_PROPERTY_ENERGY] = 0.0f;
        alive = false;
        Do_WeaponDrop();
        FreeHand();
    }
    HitFall(dir, HIT_PART_BODY);
}


// The human's frame of a tick: the AI and the shots alone while a cutscene plays the human (ACTF_CUTSCENE_PLAYED or
// ACTF_CUTSCENE_PLAY); else the carried body's animation, the logs of the positions and of the web, the walk, the
// turn, the movement, the animations and what ends them (the car's actions among them), the frame's placement in a
// car or on foot, the idle animations and the death's position
// FUNCTION: GAME 0x572b80
void C_human::Update(uint time)
{
    if (freeze_time > 0) {
        freeze_time -= time;
        return;
    }
    if (strike_anim_time >= 0)
        strike_anim_time -= time;
    if (act_flags & (ACTF_CUTSCENE_PLAYED | ACTF_CUTSCENE_PLAY)) {
        C_human::AI(time);
        if (base_frame)
            act_mission->scene->SetFrameSectorPos(frame, base_frame->GetWorldPos());
        if (weapon_kind > WEAPON_KIND_FISTS && weapon_kind <= WEAPON_KIND_SMG && fire_held)
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
    if (action_state != HUMAN_ACTION_JUMP) {
        if (!car && !using_car) {
            if (velocity.Square() < 1e-16f) {
                if (velocity_ramp > 0.0f) {
                    velocity_ramp -= time * .004f;
                    if (velocity_ramp < 0.0f)
                        velocity_ramp = 0.0f;
                    action_state = HUMAN_ACTION_WALK;
                }
            } else {
                last_velocity = velocity;
                if (velocity_ramp < 1.0f) {
                    velocity_ramp += time * .004f;
                    if (velocity_ramp > 1.0f)
                        velocity_ramp = 1.0f;
                    velocity *= velocity_ramp;
                }
            }
        } else if (((car && car->type == ACTOR_CAR) || (using_car && using_car->type == ACTOR_CAR)) && seat != -1) {
            velocity_ramp = 0.0f;
            velocity.x = 0.0f;
            velocity.y = 0.0f;
            velocity.z = 0.0f;
        }
    }

    // The turn
    if ((float)fabs(turn_speed) > MRG_ZERO) {
        S_matrix m;
        m.Identity();
        float angle = time * turn_speed * .001f;
        m(0, 0) = m(2, 2) = (float)cos(angle);
        m(2, 0) = -(m(0, 2) = (float)sin(angle));
        dir = dir.RotateByMatrix(m);
    }
    Movement(time);
    if (effect_step)
        TickWeaponEffect(time);
    if (pump_time > 0) {
        pump_time -= time;
        if (pump_time <= 0) {
            I3D_animation_set *as = NULL;
            // STRING: GAME 0x636a5c
            anim_cache.Create("gun08 stoj Pump.i3d", &as, NULL, NULL);
            if (as) {
                anim_machine.SetExtendedAnim(as, 1.0f, 4.0f, true);
                as->Release();
            }
        }
    }
    if (!(act_flags & ACTF_CUTSCENE_PLAYED))
        PersonAnim(0.0f, true, false);

    // The animation's end
    if (anim_machine.IsActive()) {
        if (anim_machine.Tick(time) & 1) {
            if (anim_steps[anim_step].type == HUMAN_STEP_END) {
                if (dead) {
                    action_state = HUMAN_ACTION_DEAD;
                    next_anim_id = 0;
                    anim_id = 0;
                } else {
                    if (anim_id != ANIM_NA_BEDNU_06M && next_anim_id != ANIM_NA_BEDNU_1M)
                        test_ground = true;
                    if (!vehicle_action) {
                        // Out of a car over its side: the police see the car stolen
                        if (act_mission->game->shvestky.active && anim_id >= ANIM_VYHOZ_MAN_FRONT_L &&
                            anim_id <= ANIM_VYHOZ_MRT_BACK_R)
                            act_mission->game->shvestky.CarSteal(this, false, false);
                        if (alive) {
                            action_state = HUMAN_ACTION_STAND;
                            anim_id = 0;
                            next_anim_id = ANIM_BREATH01A;
                            PersonAnim(0.0f, true, false);
                        }
                    } else {
                        switch (vehicle_action) {
                        case HUMAN_VEHICLE_CAR_IN:
                            Intern_UseCar(true);
                            break;
                        case HUMAN_VEHICLE_CAR_OUT:
                        case HUMAN_VEHICLE_CAR_THROWN_OUT:
                            Intern_UseCar(false);
                            break;
                        case HUMAN_VEHICLE_RAILWAY_IN:
                            Intern_UseRailway(true);
                            break;
                        case HUMAN_VEHICLE_RAILWAY_OUT:
                            Intern_UseRailway(false);
                            break;
                        case HUMAN_VEHICLE_SEAT_CHANGE:
                            {
                                // Seated in the car
                                C_car *c = car;
                                next_anim_id = !seat ? (c->IsFormula() ? ANIM_FORMULE_STATIV : ANIM_AUTO_RIDIC_STATIV) :
                                    ANIM_AUTO_SPOL_STATIV;
                                PersonAnim(0.0f, true, false);
                                car_stance_time = 250;
                                car_stance = HUMAN_CARSTANCE_SEATED;
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
                                action_state = HUMAN_ACTION_CAR;
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
                                if (next_car_action > 0) {
                                    car_action_time = 250;
                                    next_car_action += HUMAN_CAR_ACTION_DUE;
                                }
                            }
                            break;
                        }
                        vehicle_action = 0;
                        using_car = NULL;
                    }
                }
            } else
                action_state = HUMAN_ACTION_STEPS;
        }
    } else if (action_state == HUMAN_ACTION_TRACK)
        action_state = HUMAN_ACTION_STAND;

    // The frame's animation, and in a car its shots' animations
    if (!(act_flags & ACTF_CUTSCENE)) {
        if (!car || car->type != ACTOR_CAR || seat == -1 || anim_machine.IsActive() || car_stance_time >= 0)
            frame->Tick(time);
        if (car && car->type == ACTOR_CAR && seat != -1 && IsReady()) {
            car_stance_time -= time;
            if (car_stance_time < 0 && (car_stance == HUMAN_CARSTANCE_AIMED || car_stance == HUMAN_CARSTANCE_SEATED)) {
                if (car_stance == HUMAN_CARSTANCE_AIMED) {
                    next_anim_id = ANIM_AUTO_FIRE01_L;
                    if (!car->IsSeatLeft(seat))
                        next_anim_id += 3;
                    if (weapon_kind != WEAPON_KIND_PISTOL)
                        next_anim_id += 6;
                    if (!seat)
                        next_anim_id = ANIM_AUTO_FIRE03_L;
                } else {
                    next_anim_id = !seat ? (car->IsFormula() ? ANIM_FORMULE_STATIV : ANIM_AUTO_RIDIC_STATIV) :
                        ANIM_AUTO_SPOL_STATIV;
                    if (weapon_model)
                        weapon_model->SetOn(false);
                }
                PersonAnim(0.0f, true, false);
                car_aim = .5f;
                uint anim_time, length;
                anim_machine.GetActiveAnimTime(anim_time, length);
                car_anim_index = anim_machine.anim_index;
                car_anim_length = (float)length;
                frame->Tick(0);
            }
        }
    }

    // Seated in a car: the frame placed as he is
    if (car && car->type == ACTOR_CAR && seat != -1) {
        if (action_state == HUMAN_ACTION_TRACK) {
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
            if (act_mission->options.game_effects >= OPTION_HIGH)
                act_mission->game->blood_manager.AddBlood(p, d, .5f, 0, false);
        }
    }
    if (look_frame)
        PoseSetPoseNormal(look_frame->GetWorldPos() + S_vector(0.0f, 1.7f, 0.0f));
    if (pose_moving)
        PoseInterpolate(time);

    // Moved by the animation's track: tested against the collisions
    if (track_test) {
        switch (track_test) {
        case 1:
            {
            S_vector d = pos - track_test_pos;
            S_vector to;
            int hit;
            if (car)
                hit = TestMoveCollInOwner(track_test_pos, d, to);
            else
                hit = TestMoveColl(track_test_pos, d, to);
            if (hit) {
                S_vector off = to - (track_test_pos + d);
                off.y = 0.0f;
                anim_machine.unknown_114 += off;
            }
            pos = track_test_pos = to;
            }
            break;
        }
        if (anim_machine.track == -1 && anim_machine.unknown_80 == -1) {
            track_test = 0;
            fall = .01f;
        }
    }

    // The weapon in hand
    switch (weapon_kind) {
    case WEAPON_KIND_PISTOL:
    case WEAPON_KIND_RIFLE:
    case WEAPON_KIND_SMG:
        if (fire_held)
            Shooting();
        break;
    case WEAPON_KIND_FISTS:
    case WEAPON_KIND_KNUCKLEDUSTER:
    case WEAPON_KIND_KNIFE:
    case WEAPON_KIND_BAT:
        if ((!car || car->type != ACTOR_CAR || seat == -1) && aimed && !anim_machine.unknown_f4 &&
            action_state != HUMAN_ACTION_WAIT && !fire_held) {
            I3D_animation_set *as = NULL;
            switch (weapon_kind) {
            case WEAPON_KIND_FISTS:
            case WEAPON_KIND_KNUCKLEDUSTER:
                // STRING: GAME 0x636a44
                anim_cache.Create("boj HPT box kryt.i3d", &as, NULL, NULL);
                break;
            case WEAPON_KIND_KNIFE:
                // STRING: GAME 0x636a2c
                anim_cache.Create("boj HPT nuz kryt.i3d", &as, NULL, NULL);
                break;
            case WEAPON_KIND_BAT:
                // STRING: GAME 0x636a14
                anim_cache.Create("boj HPT basb kryt.i3d", &as, NULL, NULL);
                break;
            }
            if (as) {
                guarding = true;
                anim_machine.SetExtendedAnim(as, .8f, 4.0f, false);
                as->Release();
            }
        }
        break;
    }

    // The frame placed
    if (!death_pos_set) {
        frame->SetPos(pos);
        dir.y = 0.0f;
        dir.Normalize();
        frame->SetDir(dir, 0.0f);
        frame->Update();
    } else if (anim_id >= ANIM_SMRT_STOJ_BRICHO_T_PAD && anim_id <= ANIM_SMRT_DREP_ZADA_T_END) {
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
                next_anim_id = ANIM_BREATH01B;
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
        if (action_state != HUMAN_ACTION_TRACK &&
            (anim_id < ANIM_SMRT_STOJ_BRICHO_T_PAD || anim_id > ANIM_SMRT_DREP_ZADA_T_END)) {
            if (death_pos_set)
                ProcessDeathPos(time);
            else
                RecompileDeathPos();
        } else if (anim_id >= ANIM_SMRT_STOJ_BRICHO_T_PAD && anim_id <= ANIM_SMRT_DREP_ZADA_T_END)
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
    switch (next_anim_id & ANIM_BASE) {
    case ANIM_WALK1:
    case ANIM_WALKL1:
    case ANIM_WALKR1:
        // STRING: GAME 0x636b38
        anim = "NoseniMrtvolyMrtWalk.I3D";
        break;
    case ANIM_BACK1:
    case ANIM_BACKL1:
    case ANIM_BACKR1:
        // STRING: GAME 0x636b1c
        anim = "NoseniMrtvolyMrtBack.I3D";
        break;
    case ANIM_LEFT1:
        // STRING: GAME 0x636b00
        anim = "NoseniMrtvolyMrtLeft.I3D";
        break;
    case ANIM_RIGHT1:
        // STRING: GAME 0x636ae4
        anim = "NoseniMrtvolyMrtRight.I3D";
        break;
    case ANIM_STRAFL1:
        // STRING: GAME 0x636ac8
        anim = "NoseniMrtvolyMrtStrafL.I3D";
        break;
    case ANIM_STRAFR1:
        // STRING: GAME 0x636aac
        anim = "NoseniMrtvolyMrtStrafR.I3D";
        break;
    case ANIM_NOSENI_MRTVOLY_TOM_NAHOZENI:
        // STRING: GAME 0x636a8c
        anim = "NoseniMrtvolyMrtNahozeni.I3D";
        unknown = true;
        loop = false;
        break;
    case ANIM_NOSENI_MRTVOLY_TOM_SHOZENI:
        // STRING: GAME 0x636a70
        anim = "NoseniMrtvolyMrtShozeni.I3D";
        loop = false;
        break;
    }
    dead_body_human->Do_PlayAnim(anim, !loop, unknown);
}

// Plays next_anim_id unless it is playing: in the stance of the weapon and the crouch, in the set's animations
// (anim_set), the replaced animation (anims) or the table's, with its idle and breath times, its track's move tested
// against the collisions (track_test) and the jumps' landing height
// FUNCTION: GAME 0x573e50
void C_human::PersonAnim(float f, bool b1, bool b2)
{
    static char buff[128];
    uint add = 0;
    next_anim_id &= ANIM_BASE;
    bool walk = next_anim_id >= ANIM_RUN1 && next_anim_id <= ANIM_BACKR1;
    if (next_anim_id <= ANIM_SET_LAST && b1) {
        if (crouching) {
            if (weapon_kind == WEAPON_KIND_RIFLE || weapon_kind == WEAPON_KIND_SMG)
                add = ANIM_SET_CROUCH_RIFLE;
            else if (weapon_kind == WEAPON_KIND_PISTOL && aimed)
                add = ANIM_SET_CROUCH_PISTOL;
            else
                add = ANIM_SET_CROUCH;
        } else if (weapon_kind == WEAPON_KIND_RIFLE)
            add = aimed ? ANIM_SET_RIFLE : ANIM_SET_TWO_HANDED;
        else if (weapon_kind == WEAPON_KIND_SMG)
            add = aimed ? ANIM_SET_SMG : ANIM_SET_TWO_HANDED;
        else
            add = weapon_kind == WEAPON_KIND_PISTOL && aimed ? ANIM_SET_PISTOL : 0;
    }
    bool idle = next_anim_id == ANIM_BREATH01A;
    bool breath = next_anim_id >= ANIM_BREATH01B && next_anim_id <= ANIM_BREATH01D;
    if (!dead_body && !box)
        next_anim_id |= add;
    if (breath_time > -1 && anim_id > next_anim_id && anim_id - next_anim_id < 4)
        next_anim_id = anim_id;
    if (anim_set) {
        uint id = next_anim_id & ANIM_BASE;
        switch (anim_set) {
        case HUMAN_ANIMSET_CALM:
            if (add < ANIM_SET_CROUCH) {
                if (add == ANIM_SET_PISTOL)
                    add = 0;
                else if (add == ANIM_SET_TWO_HANDED)
                    add = ANIM_SET_SMG;
                switch (id) {
                case ANIM_BREATH01A:
                    next_anim_id = add | ANIM_ENM_KLID_STAT_A;
                    break;
                case ANIM_BREATH01B:
                    next_anim_id = add | ANIM_ENM_KLID_STAT_B;
                    break;
                case ANIM_BREATH01C:
                case ANIM_BREATH01D:
                    next_anim_id = add | ANIM_ENM_KLID_STAT_C;
                    break;
                case ANIM_WALK1:
                    next_anim_id = add | ANIM_ENM_KLID_WALK;
                    break;
                case ANIM_BACK1:
                    next_anim_id = add | ANIM_ENM_KLID_BACK;
                    break;
                case ANIM_LEFT1:
                    next_anim_id = add | ANIM_ENM_KLID_LEFT;
                    break;
                case ANIM_RIGHT1:
                    next_anim_id = add | ANIM_ENM_KLID_RIGHT;
                    break;
                case ANIM_STRAFL1:
                    next_anim_id = add | ANIM_ENM_KLID_STRAFL;
                    break;
                case ANIM_STRAFR1:
                    next_anim_id = add | ANIM_ENM_KLID_STRAFR;
                    break;
                case ANIM_WALKL1:
                    next_anim_id = add | ANIM_ENM_KLID_WALKL;
                    break;
                case ANIM_WALKR1:
                    next_anim_id = add | ANIM_ENM_KLID_WALKR;
                    break;
                case ANIM_BACKL1:
                    next_anim_id = add | ANIM_ENM_KLID_BACKL;
                    break;
                case ANIM_BACKR1:
                    next_anim_id = add | ANIM_ENM_KLID_BACKR;
                    break;
                }
            }
            break;
        case HUMAN_ANIMSET_2:
            if (add > 0 && add < ANIM_SET_TWO_HANDED) {
                switch (id) {
                case ANIM_BREATH01A:
                    next_anim_id = add | ANIM_ENM_STREH_STAT_A;
                    break;
                case ANIM_BREATH01B:
                    next_anim_id = add | ANIM_ENM_STREH_STAT_B;
                    break;
                case ANIM_BREATH01C:
                case ANIM_BREATH01D:
                    next_anim_id = add | ANIM_ENM_STREH_STAT_C;
                    break;
                case ANIM_WALK1:
                    next_anim_id = add | ANIM_ENM_STREH_WALK;
                    break;
                case ANIM_BACK1:
                    next_anim_id = add | ANIM_ENM_STREH_BACK;
                    break;
                case ANIM_LEFT1:
                    next_anim_id = add | ANIM_ENM_STREH_LEFT;
                    break;
                case ANIM_RIGHT1:
                    next_anim_id = add | ANIM_ENM_STREH_RIGHT;
                    break;
                case ANIM_STRAFL1:
                    next_anim_id = add | ANIM_ENM_STREH_STRAFL;
                    break;
                case ANIM_STRAFR1:
                    next_anim_id = add | ANIM_ENM_STREH_STRAFR;
                    break;
                case ANIM_WALKL1:
                    next_anim_id = add | ANIM_ENM_STREH_WALKL;
                    break;
                case ANIM_WALKR1:
                    next_anim_id = add | ANIM_ENM_STREH_WALKR;
                    break;
                case ANIM_BACKL1:
                    next_anim_id = add | ANIM_ENM_STREH_BACKL;
                    break;
                case ANIM_BACKR1:
                    next_anim_id = add | ANIM_ENM_STREH_BACKR;
                    break;
                }
            }
            break;
        case HUMAN_ANIMSET_PANIC:
            {
                uint cur = anim_id & ANIM_BASE;
                switch (id) {
                case ANIM_BREATH01A:
                case ANIM_BREATH01B:
                case ANIM_BREATH01C:
                case ANIM_BREATH01D:
                    // The panic's stances stay, its downs go on into them
                    if (cur == ANIM_X_PANIKA_DREP_STATIV || cur == ANIM_X_PANIKA_NA4_STATIV ||
                        cur == ANIM_X_PANIKA_LEH_STATIV)
                        next_anim_id = cur;
                    else if (cur == ANIM_X_PANIKA_DREP_DOLU)
                        next_anim_id = ANIM_X_PANIKA_DREP_STATIV;
                    else if (cur == ANIM_X_PANIKA_NA4_DOLU)
                        next_anim_id = ANIM_X_PANIKA_NA4_STATIV;
                    else if (cur == ANIM_X_PANIKA_LEH_DOLU)
                        next_anim_id = ANIM_X_PANIKA_LEH_STATIV;
                    break;
                case ANIM_BACK1:
                    next_anim_id = ANIM_PANIKA_RUN02A;
                    break;
                case ANIM_RUN1:
                    // A run of the panic's, the one running kept
                    switch (cur) {
                    case ANIM_PANIKA_RUN01A:
                        next_anim_id = cur;
                        break;
                    case ANIM_PANIKA_RUN03A:
                        next_anim_id = cur;
                        break;
                    default:
                        next_anim_id = rnd(2) * 2 + ANIM_PANIKA_RUN01A;
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

    if (weapon_displaced)
        RepairWeaponPosition();
    // Out of the wreck: the player's camera on foot again
    if (act_mission->game->player == this && anim_id == ANIM_AUTO_LEZENI_Z_VRAKU) {
        act_mission->game->camera.PopMode(false);
        act_mission->game->camera.SetCar(NULL);
    }
    if (lower_coll_off) {
        lower_coll_off = false;
        ActivateColls(HUMAN_COLLS_LOWER);
    }
    if (idle)
        idle_time = rnd(5000) + 5000;
    else
        idle_time = -1;
    if (!breath)
        breath_time = -1;
    if (walk) {
        if (human_flags & HUMANF_WALK_HALF) {
            human_flags &= ~HUMANF_WALK_HALF;
            f = .5f;
        } else
            human_flags |= HUMANF_WALK_HALF;
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
    if (next_anim_id == ANIM_NA_BEDNU_06M || next_anim_id == ANIM_NA_BEDNU_1M) {
        // Onto the box, of the height and length Do_Climb found
        S_vector scale(climb_reach, climb_scale, 1.0f);
        anim_machine.SetAnim(buff, false, false, b2, 4.0f, f, true, false, &scale, true, (uint)-1);
    } else if (next_anim_id == ANIM_AUTO_PRELEZ_L_TO_R || next_anim_id == ANIM_AUTO_PRELEZ_R_TO_L) {
        // Over the car
        S_vector scale(climb_scale, 1.0f, 1.0f);
        anim_machine.SetAnim(buff, false, false, b2, 4.0f, f, true, false, &scale, true, (uint)-1);
    } else
        anim_machine.SetAnim(buff, false, false, b2, 4.0f, f, true, false, NULL, true, anim_id);

    if (anim_machine.track != -1) {
        // A track moves the human
        velocity_ramp = 0.0f;
        velocity.Zero();
        action_state = HUMAN_ACTION_TRACK;
        anim_machine.DelExtendedAnim(4.0f);
        track_test = 0;
        uint id = anim_id & ANIM_BASE;
        if ((id < ANIM_AUTO_RIDIC_STATIV || id > ANIM_FORMULE_VYSTUP_R) && id != ANIM_SALINA_NASTUP &&
            id != ANIM_SALINA_VYSTUP && id != ANIM_NA_BEDNU_06M && id != ANIM_NA_BEDNU_1M &&
            id != ANIM_AUTO_PRELEZ_L_TO_R && id != ANIM_AUTO_PRELEZ_R_TO_L) {
            track_test = 1;
            track_test_pos = pos;
        }
        if ((id >= ANIM_UNKNOWN_5E && id <= ANIM_JUMPRX1) ||
            (id >= ANIM_SMRT_PAD_DOZADU && id <= ANIM_SMRT_PAD_DOPREDU_C) || (id >= ANIM_JUMPL1 && id <= ANIM_JUMPR3)) {
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
                if (gifCollision.TestLineH(pos + S_vector(0.0f, .2f, 0.0f), d, &hit, &normal, COLF_UNKNOWN_2, 0)) {
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
    } else if (action_state == HUMAN_ACTION_TRACK)
        action_state = HUMAN_ACTION_STAND;
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

// The hands' own fire steps (PS2 names): when the weapon is ready, the aim (Do_Shoot, Shooting, C_player's
// SetSniperMode) and the weapon taken out or put away (Do_ChangeWeapon); PS2's _fire_action_wait_ isn't on the PC
// GLOBAL: GAME 0x6368d0
S_fire_action _fire_action_ready_[] = {{FIRE_ACTION_END, 0}};
// GLOBAL: GAME 0x6368d8
S_fire_action _fire_action_aimed_[] = {{0, 250}, {FIRE_ACTION_END, 0}};
// GLOBAL: GAME 0x6368e8
S_fire_action _fire_action_onoff_[] = {{0, 500}, {FIRE_ACTION_END, 0}};

// Declared before human_notify_callback, which comes first in the file: defined here, as its place in .data after
// the fire's steps shows (PS2 name; PS2's is a file static, also after _fire_action_ready_)
// GLOBAL: GAME 0x6368f8
bool do_snd_car_door = true;

// FUNCTION: GAME 0x5748f0
void C_human::ChangeState()
{
    switch (act_state)
    {
    case ACTSTATE_ACTIVE:
        ActivateColls(HUMAN_COLLS_ALL);
        if (type == ACTOR_ENTITY && ((C_entity *)this)->IsEventID(EVENT_USE_AB, NULL) && ab_created)
            act_mission->GetGame()->GetUsingObject().AddObject(&((C_entity *)this)->using_object);
        if (!shadow && !car)
            shadow = act_mission->GetGame()->AddFrameForShadow(frame, SHADOW_TYPE_0, SHADOW_TYPE_0, NULL, NULL);
        break;
    case ACTSTATE_NONE:
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
                using_objects[0].item.id = GAMEITEM_SPECIAL_ACTION;
                using_objects[0].item.ammo_loaded = USING_ACTION_SEARCH_DEAD_BODY;
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
                using_objects[1].item.id = GAMEITEM_SPECIAL_ACTION;
                using_objects[1].item.ammo_loaded = USING_ACTION_MOVE_DEAD_BODY;
                using_objects[1].item.using_object = &using_objects[1];
            }
            PoseValidate(false);
        }
    case ACTSTATE_INACTIVE:
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
    action_state = HUMAN_ACTION_STAND;
    vehicle_action = 0;
    anim_machine.Init(this);
    next_anim_id = ANIM_BREATH01A;
    memcpy(properties, max_properties, sizeof(properties));
    anim_id = 0;
    anim_set = 0;
    velocity.Zero();
    last_velocity = S_vector(0.0f, 0.0f, 0.0f);
    turn_speed = 0;
    fall = 0.0f;
    velocity_ramp = 0.0f;
    guarding = false;
    idle_time = -1;
    breath_time = -1;
    striking = false;
    sniper_mode = false;
    dead_time = 500;
    agony_anim = -1;
    wingman = -1;
    last_fire_time = 0;
    fire_target = NULL;
    track_test = 0;
    bleed_count = 0;
    track_steps = 0;
    traffic = NULL;
    box = NULL;
    human_flags = 0;
    move_time = 0;
    move_pos = pos;
    move_ratio = 1.0f;
    step_time = 0;
    move_coll = 0;
    no_anim_hit = false;
    note_fire = false;
    reloading = false;
    reload_forced = false;
    loading_shells = false;
    last_hit_time = 0;
    last_hit_render = 0;
    idle_anims = false;
    test_ground = true;
    voice_priority = -1;
    dead_body = NULL;
    dead_body_human = 0;
    dead_body_time = -1;
    lower_coll_off = false;
    killed_by_shot = false;
    death_pos_set = false;
    stealing_car = NULL;
    freeze_time = -1;
    effect_step = 0;
    pump_time = -1;
    hand_clip = NULL;
    killer = NULL;
    drowned = false;
    changing_weapon = false;
    scream_areas = 1;
    unknown_b0 = 0;
    next_car_action = 0;
    car_action_time = -1;
    speed_limiter = false;
    strike_anim_time = -1;
    score_bonus = false;
    temp_driver = false;
    last_car = 0;
    weapon_displaced = false;
    reload_effects = false;
    shut_up = false;
    ab_created = true;
    strike_time = 0;
    locked_car = NULL;
    locked_vehicle = NULL;
    shots = 1.0f;
    hits = 1.0f;
    prev_shots = 1.0f;
    prev_hits = 1.0f;
    ratio_time = 5000;
    car_stance = HUMAN_CARSTANCE_SEATED;
    car_stance_time = 0;
    car_anim_index = 0;
    car_anim_length = 0;
    car_aim = .5f;
    pose_moving = false;
    pose_back1 = driver->CreatePose();
    ((I3D_model *)frame)->SetPose("back1", pose_back1);
    pose_back2 = driver->CreatePose();
    // STRING: GAME 0x6369d4
    ((I3D_model *)frame)->SetPose("back2", pose_back2);
    pose_neck = driver->CreatePose();
    ((I3D_model *)frame)->SetPose("neck", pose_neck);
    look_frame = NULL;
    anim_step = 0;
    anim_steps[0].type = HUMAN_STEP_END;
    CreateDynColls();

    step_sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
    if (step_sound)
    {
        step_sound->SetOn(false);
        if (act_mission->GetGame()->GetPlayer() == this)
            step_sound->SetPos(S_vector(0.0f, 1.5f, 0.0f));
        else
            step_sound->SetPos(S_vector(0.0f, .1f, 0.0f));
        step_sound->LinkTo(frame, 0);
        step_sound->Update();
    }
    weapon_sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
    if (weapon_sound)
    {
        weapon_sound->SetOn(false);
        weapon_sound->SetPos(S_vector(0.0f, 1.0f, 0.0f));
        weapon_sound->LinkTo(frame, 0);
        weapon_sound->Update();
        weapon_sound->priority = 0x60;
    }
    reload_sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
    if (reload_sound)
    {
        reload_sound->SetOn(false);
        reload_sound->SetPos(S_vector(0.0f, 1.0f, 0.0f));
        reload_sound->LinkTo(frame, 0);
        reload_sound->Update();
    }
    pump_sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
    if (pump_sound)
    {
        pump_sound->SetOn(false);
        pump_sound->SetPos(S_vector(0.0f, 1.0f, 0.0f));
        pump_sound->LinkTo(frame, 0);
        pump_sound->Update();
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
        notify->SetCallback(human_notify_callback, LS3D_CALLBACK_NOTE);
    for (int i = anims.size(); i--;)
        delete[] anims[i].name;
    anims.clear();
    aimed = false;
    throw_swung = false;
    fire_held = false;
    crouching = false;
    strike_index = 0;
    weapon_kind = WEAPON_KIND_FISTS;
    ground_material = 0;
    tilt = .5f;
    shadow = act_mission->GetGame()->AddFrameForShadow(frame, SHADOW_TYPE_0, SHADOW_TYPE_0, NULL, NULL);
    pose_flags = 0;

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
        left_hand_target = (I3D_target *)driver->CreateFrame(FRAME_TARGET);
        left_hand_target->SetPos(S_vector(0.0f, 0.0f, 0.0f));
        left_hand_target->LinkTo(left_hand, 0);
        left_hand_target->Update();
    }
    else
        left_hand_target = NULL;
    if (right_hand)
    {
        right_hand_target = (I3D_target *)driver->CreateFrame(FRAME_TARGET);
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
    cutscene_channel_0 = true;
    cutscene_blending = false;
    cutscene_blend = 1.0f;
    tck_time = 0;
    tck_on = false;
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
    if (hand_clip)
    {
        hand_clip->LinkTo(NULL, 0);
        hand_clip->Release();
        hand_clip = NULL;
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
    if (step_sound)
    {
        step_sound->SetOn(false);
        step_sound->Release();
        step_sound = NULL;
    }
    if (weapon_sound)
    {
        weapon_sound->SetOn(false);
        weapon_sound->Release();
        weapon_sound = NULL;
    }
    if (reload_sound)
    {
        reload_sound->SetOn(false);
        reload_sound->Release();
        reload_sound = NULL;
    }
    if (pump_sound)
    {
        pump_sound->SetOn(false);
        pump_sound->Release();
        pump_sound = NULL;
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
        case HUMAN_COLLS_ALL:
            if ((colls[i]->flags & DYNCOLLF_UPPER) || alive)
                gifCollision.DynInsert(colls[i]);
            break;
        case HUMAN_COLLS_UPPER:
            if ((colls[i]->flags & DYNCOLLF_UPPER) && alive)
                gifCollision.DynInsert(colls[i]);
            break;
        case HUMAN_COLLS_LOWER:
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
                if (lower_coll_off && (colls[i]->flags & DYNCOLLF_LOWER))
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

// The humans' force feedback effects, which InitFFEffects creates and DoneFFEffects deletes (the names are ours, by
// their users): the shots' (Shooting, ChangeWeaponModel), the jumps' (Do_Jump), the hits' (Hit) and the falls'
// (MovementIntern)
// GLOBAL: GAME 0x64740c
static int ff_shot;                     // 0x64740c
// GLOBAL: GAME 0x647408
static int ff_jump;                     // 0x647408
// GLOBAL: GAME 0x647404
static int ff_hit;                      // 0x647404
// GLOBAL: GAME 0x647400
static int ff_fall;                     // 0x647400

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
        if (human_type == HUMAN_TYPE_FRIEND)
            return false;
    }
    if (human_flags & HUMANF_EVENT_HIT_FROM_PLAYER && GetType() == ACTOR_ENTITY &&
        act == act_mission->GetGame()->GetPlayer())
        ((C_entity *)this)->SetEvent(EVENT_HIT_FROM_PLAYER);
    if (!alive || properties[HUMAN_PROPERTY_ENERGY] < .01f) {
        if (agony_anim > -1 && anim_id - agony_anim != ANIM_SMRT_STOJ_BRICHO_T_PAD) {
            Death();
            anim_steps[0].anim_id = agony_anim + ANIM_SMRT_STOJ_BRICHO_T_END;
            anim_steps[1].type = HUMAN_STEP_END;
            anim_steps[0].wait = 0;
            anim_steps[0].flags = HUMAN_STEPF_ONCE | HUMAN_STEPF_WAIT;
            anim_steps[0].type = HUMAN_STEP_ANIM;
            anim_steps[0].stance = false;
            action_state = HUMAN_ACTION_STEPS;
            anim_step = 0;
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
    throw_swung = false;
    anim_machine.DelExtendedAnim(4.0f);
    loading_shells = false;
    fire_held = false;
    aimed = false;
    PoseNullRotate();
    ResetWeaponEffects();
    reload_effects = false;
    if (changing_weapon)
        ChangeWeaponModel();
    if (action_state == HUMAN_ACTION_STEAL && act_mission->GetGame()->GetPlayer() == this) {
        mafiaIndicators.show_flags &= ~INDF_THIEF;
        stealing_car = NULL;
        action_time = -1;
        if (weapon_model)
            weapon_model->SetOn(true);
        if (weapon_model2)
            weapon_model2->SetOn(true);
        act_mission->GetGame()->Stop3DSound(steal_sound, false, .001f);
        act_mission->GetGame()->shvestky.CarSteal(this, false, false);
        act_mission->GetGame()->GetCamera().EnableFreeLook(false, false);
    }
    if (car && car->GetType() == ACTOR_CAR && seat != -1)
        return HitInCar(type, dir, pos, v3, f, act, part, frm);
    switch (vehicle_action) {
    case HUMAN_VEHICLE_CAR_IN:
        Intern_UseCar(true);
        break;
    case HUMAN_VEHICLE_CAR_OUT:
    case HUMAN_VEHICLE_CAR_THROWN_OUT:
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
    case HUMAN_VEHICLE_RAILWAY_IN:
        Intern_UseRailway(true);
        break;
    case HUMAN_VEHICLE_RAILWAY_OUT:
        Intern_UseRailway(false);
        break;
    }
    vehicle_action = 0;
    using_car = NULL;
    bool fall = false;
    bool heavy = false;
    bool stroke = false;
    bool agony = false;
    bool landing = false;
    switch (type) {
    case HIT_AIRPLANE: {
        properties[HUMAN_PROPERTY_ENERGY] = -1.0f;
        S_vector particle_pos(0.0f, 1.4f, 0.0f);
        fall = true;
        if (act_mission->options.particles >= OPTION_MEDIUM)
            act_mission->GetEffects()->CreateParticle(HITPARTICLE_AIRPLANE, particle_pos, dir, 0, frame);
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
                close_range = weapon->flags & ITEMF_CLOSE_RANGE;
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
        if (act_mission->options.game_effects >= OPTION_MEDIUM)
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
    case HIT_EXPLOSION:
        properties[HUMAN_PROPERTY_ENERGY] -= f;
        if (f > 100.0f)
            fall = true;
        else if (f > 50.0f) {
            heavy = true;
            part = rnd(4) + HIT_PART_HAND_R;
        } else
            part = HIT_PART_BODY;
        break;
    case HIT_CAR: {
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
    case HIT_FIRE:
        properties[HUMAN_PROPERTY_ENERGY] -= f;
        properties[HUMAN_PROPERTY_ENERGY_HAND_L] -= f * .3f;
        properties[HUMAN_PROPERTY_ENERGY_HAND_R] -= f * .3f;
        properties[HUMAN_PROPERTY_ENERGY_LEG_L] -= f * .5f;
        properties[HUMAN_PROPERTY_ENERGY_LEG_R] -= f * .5f;
        break;
    case HIT_DOG_BITE: {
        properties[HUMAN_PROPERTY_ENERGY] -= f;
        part = rnd(2) + HIT_PART_LEG_R;
        S_vector blood_dir = dir + S_vector(rnd(), rnd(), rnd()) * .3f;
        blood_dir.Normalize();
        S_vector blood_pos = GetWorldPos();
        blood_pos.y += .6f;
        blood_dir *= 2.0f;
        if (act_mission->options.game_effects >= OPTION_MEDIUM)
            act_mission->GetGame()->blood_manager.AddBlood(blood_pos, blood_dir, 1.0f, 0, false);
        if (act_mission->options.particles >= OPTION_MEDIUM)
            act_mission->GetEffects()->CreateParticle(SHOTPARTICLE_BODY, blood_pos, blood_dir, 0, NULL);
        break;
    }
    case HIT_IN_CAR:
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
            ((C_human *)act)->hits += 1.0f;
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
    fire_held = false;
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
    if (type == HIT_CAR && !alive && anim_machine.track == -1) {
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
        next_anim_id = ANIM_DOSKOK;
        PersonAnim(0.0f, true, false);
        uint time, length;
        anim_machine.GetActiveAnimTime(time, length);
        if (length > 250) {
            action_time = length - 250;
            last_velocity.Zero();
            velocity_ramp = 0.0f;
            action_state = HUMAN_ACTION_WAIT;
        }
        NewScream(rnd(7) + VOICE_HIT);
    } else {
        anim_steps[0].anim_id = GetAnimIDForDeath(HIT_PART_BODY, crouching, this->dir, this->dir);
        anim_steps[0].wait = 0;
        anim_steps[0].flags = HUMAN_STEPF_ONCE | HUMAN_STEPF_WAIT;
        anim_steps[0].type = HUMAN_STEP_ANIM;
        anim_steps[0].stance = false;
        anim_steps[1].type = HUMAN_STEP_END;
        action_state = HUMAN_ACTION_STEPS;
        anim_step = 0;
        Movement(1);
        NewScream(rnd(2) + VOICE_DEATH);
    }
}

// The agony after a shot in the body (PS2 name)
inline void C_human::HitAgony(const S_vector &dir)
{
    dead = true;
    agony_count = rnd(8) + 5;
    float angle = dir.AngleTo(this->dir);
    if (crouching)
        agony_anim = angle < PI / 2 ? 12 : 8;
    else
        agony_anim = angle < PI / 2 ? 4 : 0;
    anim_id = agony_anim + ANIM_SMRT_STOJ_BRICHO_T_PAD;
    RecompileDeathPos();
    anim_id = 0;
    anim_steps[0].anim_id = agony_anim + ANIM_SMRT_STOJ_BRICHO_T_PAD;
    anim_steps[0].wait = 0;
    anim_steps[0].flags = HUMAN_STEPF_ONCE | HUMAN_STEPF_WAIT;
    anim_steps[0].type = HUMAN_STEP_ANIM;
    anim_steps[0].stance = false;
    anim_steps[1].type = HUMAN_STEP_AGONY;
    action_state = HUMAN_ACTION_STEPS;
    anim_step = 0;
    Movement(1);
    NewScream(rnd(3) + VOICE_AGONY);
}

// A stroke's hit (PS2 name)
inline void C_human::HitStroke(const S_vector &dir, uint part)
{
    if (alive && anim_id >= ANIM_X_PANIKA_DREP_DOLU && anim_id <= ANIM_X_PANIKA_LEH_NAHORU)
        return;
    anim_id = 0;
    if (alive) {
        if (part == HIT_PART_HEAD) {
            float angle = dir.AngleTo(this->dir);
            if (angle < PI / 4)
                anim_steps[0].anim_id = ANIM_ZASAH_SM_HLAVA_B;
            else if (angle > PI * 3 / 4)
                anim_steps[0].anim_id = ANIM_BOJ_ZASAH_HL_D;
            else {
                S_vector2 look(this->dir.x, this->dir.z);
                S_vector2 hit(dir.x, dir.z);
                if (AngleTo2(look, hit) > 0.0f)
                    anim_steps[0].anim_id = ANIM_BOJ_ZASAH_HL_L;
                else
                    anim_steps[0].anim_id = ANIM_BOJ_ZASAH_HL_R;
            }
        } else {
            S_vector2 look(this->dir.x, this->dir.z);
            S_vector2 hit(dir.x, dir.z);
            if (AngleTo2(look, hit) > 0.0f)
                anim_steps[0].anim_id = ANIM_BOJ_ZASAH_BR_R;
            else
                anim_steps[0].anim_id = ANIM_BOJ_ZASAH_BR_L;
        }
        anim_steps[0].wait = -250;
        NewScream(rnd(7) + VOICE_HIT);
    } else {
        anim_steps[0].anim_id = GetAnimIDForDeath(part, crouching, this->dir, dir);
        anim_steps[0].wait = 0;
        NewScream(rnd(2) + VOICE_DEATH);
    }
    anim_steps[0].flags = HUMAN_STEPF_ONCE | HUMAN_STEPF_WAIT;
    anim_steps[0].type = HUMAN_STEP_ANIM;
    anim_steps[0].stance = false;
    anim_steps[1].type = HUMAN_STEP_END;
    action_state = HUMAN_ACTION_STEPS;
    anim_step = 0;
    Movement(1);
}

// A shot's hit (PS2 name), heavy for the strong hits' animations
inline void C_human::HitGun(const S_vector &dir, uint part, bool heavy)
{
    if (alive) {
        if (anim_id >= ANIM_X_PANIKA_DREP_DOLU && anim_id <= ANIM_X_PANIKA_LEH_NAHORU)
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
                    anim_steps[0].anim_id = angle < PI / 2 ? ANIM_ZASAH_LG_DREP_HLAVA_B : ANIM_ZASAH_LG_DREP_HLAVA_F;
            } else {
                switch (part) {
                case HIT_PART_HAND_R:
                    anim_steps[0].anim_id = angle < PI / 2 ? ANIM_ZASAH_LG_RAMENO_BR : ANIM_ZASAH_LG_RAMENO_FR;
                    break;
                case HIT_PART_HAND_L:
                    anim_steps[0].anim_id = angle < PI / 2 ? ANIM_ZASAH_LG_RAMENO_BL : ANIM_ZASAH_LG_RAMENO_FL;
                    break;
                case HIT_PART_LEG_R:
                    anim_steps[0].anim_id = ANIM_ZASAH_LG_NOHA_R;
                    break;
                case HIT_PART_LEG_L:
                    anim_steps[0].anim_id = ANIM_ZASAH_LG_NOHA_L;
                    break;
                case HIT_PART_BODY:
                    break;
                case HIT_PART_HEAD:
                    anim_steps[0].anim_id = angle < PI / 2 ? ANIM_ZASAH_LG_HLAVA_B : ANIM_ZASAH_LG_HLAVA_F;
                    break;
                }
            }
        } else if (crouching) {
            switch (part) {
            case HIT_PART_HAND_R:
                anim_steps[0].anim_id = angle < PI / 2 ? ANIM_ZASAH_DREP_RAMENO_BR : ANIM_ZASAH_DREP_RAMENO_FR;
                break;
            case HIT_PART_HAND_L:
                anim_steps[0].anim_id = angle < PI / 2 ? ANIM_ZASAH_DREP_RAMENO_BL : ANIM_ZASAH_DREP_RAMENO_FL;
                break;
            case HIT_PART_LEG_R:
                anim_steps[0].anim_id = ANIM_ZASAH_DREP_NOHA_R;
                break;
            case HIT_PART_LEG_L:
                anim_steps[0].anim_id = ANIM_ZASAH_DREP_NOHA_L;
                break;
            case HIT_PART_BODY:
                anim_steps[0].anim_id = angle < PI / 2 ? ANIM_ZASAH_DREP_ZADA : ANIM_ZASAH_DREP_BRICHO;
                break;
            case HIT_PART_HEAD:
                anim_steps[0].anim_id = angle < PI / 2 ? ANIM_ZASAH_SM_DREP_HLAVA_B : ANIM_ZASAH_SM_DREP_HLAVA_F;
                break;
            }
        } else {
            switch (part) {
            case HIT_PART_HAND_R:
                anim_steps[0].anim_id = angle < PI / 2 ? ANIM_ZASAH_SM_RAMENO_BR : ANIM_ZASAH_SM_RAMENO_FR;
                break;
            case HIT_PART_HAND_L:
                anim_steps[0].anim_id = angle < PI / 2 ? ANIM_ZASAH_SM_RAMENO_BL : ANIM_ZASAH_SM_RAMENO_FL;
                break;
            case HIT_PART_LEG_R:
                anim_steps[0].anim_id = ANIM_ZASAH_SM_NOHA_R;
                break;
            case HIT_PART_LEG_L:
                anim_steps[0].anim_id = ANIM_ZASAH_SM_NOHA_L;
                break;
            case HIT_PART_BODY:
                anim_steps[0].anim_id = angle < PI / 2 ? ANIM_ZASAH_SM_ZADA : ANIM_ZASAH_SM_BRICHO;
                break;
            case HIT_PART_HEAD:
                anim_steps[0].anim_id = angle < PI / 2 ? ANIM_ZASAH_SM_HLAVA_B : ANIM_ZASAH_SM_HLAVA_F;
                break;
            }
        }
        anim_steps[0].wait = -250;
        if (heavy && (part == HIT_PART_LEG_R || part == HIT_PART_LEG_L))
            NewScream(VOICE_HIT_LEG);
        else
            NewScream(rnd(7) + VOICE_HIT);
    } else {
        anim_steps[0].anim_id = GetAnimIDForDeath(part, crouching, this->dir, dir);
        anim_steps[0].wait = 0;
        NewScream(rnd(2) + VOICE_DEATH);
    }
    anim_steps[0].flags = HUMAN_STEPF_ONCE | HUMAN_STEPF_WAIT;
    anim_steps[0].type = HUMAN_STEP_ANIM;
    anim_steps[0].stance = false;
    anim_steps[1].type = HUMAN_STEP_END;
    action_state = HUMAN_ACTION_STEPS;
    anim_step = 0;
    Movement(1);
}

// The player's hit on a hand or the body (PS2 name): an extended animation over the one playing, by the weapon held
inline void C_human::HitGunHPT(const S_vector &dir, uint part, bool heavy)
{
    float angle = dir.AngleTo(this->dir);
    int id = -1;
    switch (weapon_kind) {
    case WEAPON_KIND_FISTS:
        id = ANIM_ZASAH_HPT_NOGUNF;
        break;
    case WEAPON_KIND_PISTOL:
        id = aimed ? ANIM_ZASAH_HPT_PISTOL1F : ANIM_ZASAH_HPT_NOGUNF;
        break;
    case WEAPON_KIND_RIFLE:
        id = aimed ? ANIM_ZASAH_HPT_PUSKA1F : ANIM_ZASAH_HPT_PUSKA2F;
        break;
    case WEAPON_KIND_SMG:
        id = aimed ? ANIM_ZASAH_HPT_SAPIKF : ANIM_ZASAH_HPT_PUSKA2F;
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
        anim_steps[0].type = HUMAN_STEP_ANIM;
        anim_steps[0].anim_id = alive ? ANIM_SMRT_PAD_DOPREDU_C :
            rnd(2) ? ANIM_SMRT_PAD_DOPREDU_A : ANIM_SMRT_PAD_DOPREDU_B;
        anim_steps[0].stance = false;
        anim_steps[0].flags = 0;
        anim_steps[1].type = HUMAN_STEP_END;
    } else {
        if (angle > PI * 3 / 4) {
            this->dir = -d;
            anim_steps[0].type = HUMAN_STEP_ANIM;
            anim_steps[0].anim_id = ANIM_SMRT_PAD_DOZADU;
            anim_steps[0].stance = false;
            anim_steps[1].type = HUMAN_STEP_ANIM;
            anim_steps[1].stance = false;
            anim_steps[2].type = HUMAN_STEP_END;
        } else {
            S_vector2 look(this->dir.x, this->dir.z);
            S_vector2 hit(d.x, d.z);
            if (AngleTo2(look, hit) > 0.0f) {
                anim_steps[0].anim_id = ANIM_SMRT_PAD_VLEVO;
                this->dir = S_vector(d.z, d.y, -d.x);
            } else {
                anim_steps[0].anim_id = ANIM_SMRT_PAD_VPRAVO;
                this->dir = S_vector(-d.z, d.y, d.x);
            }
            anim_steps[0].type = HUMAN_STEP_ANIM;
            anim_steps[0].stance = false;
            anim_steps[1].type = HUMAN_STEP_ANIM;
            anim_steps[1].stance = false;
            anim_steps[2].type = HUMAN_STEP_END;
        }
        anim_steps[1].flags = 0;
        anim_steps[0].flags = 0;
        anim_steps[1].anim_id = alive ? ANIM_SMRT_PAD_END_VSTAVANI :
            rnd(2) ? ANIM_SMRT_PAD_END_A : ANIM_SMRT_PAD_END_B;
    }
    this->dir.Normalize();
    frame->SetDir(this->dir, 0.0f);
    frame->Update();
    action_state = HUMAN_ACTION_STEPS;
    anim_step = 0;
    Movement(1);
    if (alive)
        NewScream(rnd(7) + VOICE_HIT);
    else
        NewScream(rnd(2) + VOICE_DEATH);
}

// The human hit in a car's seat (PS2 name): true
// FUNCTION: GAME 0x578290
bool C_human::HitInCar(int type, const S_vector &dir, const S_vector &pos, const S_vector &v3, float f, C_actor *act,
    uint part, I3D_frame *frm)
{
    if (type == HIT_SHOT || type == HIT_IN_CAR)
        properties[HUMAN_PROPERTY_ENERGY] -= f;
    if (act && act->GetType() == ACTOR_ENTITY)
        ((C_entity *)act)->cbHitInfo(type, f, this);
    if (properties[HUMAN_PROPERTY_ENERGY] < .01f) {
        properties[HUMAN_PROPERTY_ENERGY] = 0.0f;
        alive = false;
        if (!net_direct && !seat)
            car->driver_killed = true;
        if (car_stance == HUMAN_CARSTANCE_AIMED)
            next_anim_id = car->IsSeatLeft(seat) ? ANIM_SMRT_AUTO_SPOL_OUTL : ANIM_SMRT_AUTO_SPOL_OUTR;
        else if (!seat)
            next_anim_id = ANIM_SMRT_AUTO_RIDIC01;
        else if (weapon_kind == WEAPON_KIND_RIFLE || weapon_kind == WEAPON_KIND_SMG)
            next_anim_id = ANIM_SMRT_AUTO_SPOL_IN02;
        else
            next_anim_id = ANIM_SMRT_AUTO_SPOL_IN01;
        PersonAnim(0.0f, true, true);
        uint time, length;
        anim_machine.GetActiveAnimTime(time, length);
        car_stance_time = length;
        dead_time = length;
        action_state = HUMAN_ACTION_DEAD;
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
    NewScream(rnd(7) + VOICE_HIT);
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
            Hit(HIT_CAR, hit_dir, pos, v3, speed * 4.2f, act, 0, NULL);
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
        if (act_mission->options.game_sounds >= OPTION_HIGH && speed > 5.0f) {
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
            Hit(HIT_EXPLOSION, dir, pos, v3, speed * 4.2f, act, 0, NULL);
        }
        if (act_mission->options.game_sounds >= OPTION_HIGH && speed > 5.0f) {
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


// Notes a link to the actor (none for NULL) of the kind, with the frame's placement then, for the recording's next
// record (RecordKey), while the human is recorded
// FUNCTION: GAME 0x578e20
void C_human::SetCutsceneLink(C_actor *act, int kind)
{
    if (act_flags & ACTF_CUTSCENE_RECORDED)
    {
        S_cutscene_link *link = cutscene_link;
        if (!link)
            link = new S_cutscene_link;
        link->is_new = true;
        link->kind = kind;
        link->pos = frame->GetPos();
        link->rot = frame->GetRot();
        strcpy(link->name, act ? act->frame->GetName() : "");
        cutscene_link = link;
    }
}


// FUNCTION: GAME 0x578ee0
void C_human::PoseInterpolate(uint time)
{
    pose_moving = false;
    float speed = (float)time / 1000.0f * PI;
    float max = Max(.03f, speed);

    S_quat rot;
    S_matrix m;
    S_vector dir;

    rot = pose_back1->GetRot1();
    m = rot.RotationMatrix();
    dir = m(2);
    float angle = dir.AngleTo(pose_back1_dir);
    if (angle > max)
    {
        float t = speed / angle;
        S_vector v = dir * (1.0f - t) + pose_back1_dir * t;
        v.Normalize();
        pose_back1->SetDir(v, 0.0f);
        pose_moving = true;
    }
    else
        pose_back1->SetDir(pose_back1_dir, 0.0f);

    rot = pose_back2->GetRot1();
    m = rot.RotationMatrix();
    dir = m(2);
    angle = dir.AngleTo(pose_back2_dir);
    if (angle > max)
    {
        float t = speed / angle;
        S_vector v = dir * (1.0f - t) + pose_back2_dir * t;
        v.Normalize();
        pose_back2->SetDir(v, 0.0f);
        pose_moving = true;
    }
    else
        pose_back2->SetDir(pose_back2_dir, 0.0f);

    rot = pose_neck->GetRot1();
    m = rot.RotationMatrix();
    dir = m(2);
    angle = dir.AngleTo(pose_neck_dir);
    if (angle > max)
    {
        float t = speed / angle;
        S_vector v = dir * (1.0f - t) + pose_neck_dir * t;
        v.Normalize();
        pose_neck->SetDir(v, 0.0f);
        pose_moving = true;
    }
    else
        pose_neck->SetDir(pose_neck_dir, 0.0f);

    // A dead human's poses are taken off once they are where they turn to
    if (dead && !pose_moving)
    {
        if (pose_back1)
            ((I3D_model *)frame)->SetPose("back1", NULL);
        if (pose_back2)
            ((I3D_model *)frame)->SetPose("back2", NULL);
        if (pose_neck)
            ((I3D_model *)frame)->SetPose("neck", NULL);
    }
}


// FUNCTION: GAME 0x579630
bool C_human::PoseSetPoseNormal(S_vector pos)
{
    if (using_car)
        return false;
    if (car && car->type == ACTOR_CAR && seat != -1)
        return false;
    I3D_frame *neck = frame->FindChildFrame("neck", ENUMF_ALL);
    if (!neck)
        return false;
    S_vector d = pos - neck->GetWorldPos();
    if (d.AngleTo(dir) > PI / 2)
        return false;

    S_vector fdir = GetFrame()->GetWorldDir();
    fdir.Normalize();
    d.Normalize();
    S_vector back = fdir * 4.0f + d;
    back.Normalize();
    S_vector head = fdir * 2.0f + d * 3.0f;
    head.Normalize();
    if (!base_frame)
        return false;

    // Into the model's space
    S_matrix m = GetFrame()->GetMatrix();
    m.Inverse(S_matrix(m));
    m(3, 0) = m(3, 1) = m(3, 2) = 0.0f;
    back *= m;
    head *= m;
    pose_back1_dir = pose_back2_dir = back;
    pose_neck_dir = head;
    pose_flags &= ~(HUMAN_POSEF_AIMED | HUMAN_POSEF_NULL_ROTATE);
    pose_flags |= HUMAN_POSEF_NEW;
    pose_pos = pos;
    pose_moving = true;
    return true;
}


// The poses turned to aim at the position: the backs only, the neck straight
// FUNCTION: GAME 0x579ea0
bool C_human::PoseSetPoseAimed(S_vector pos)
{
    if (using_car)
        return false;
    if (car && car->type == ACTOR_CAR && seat != -1)
        return false;
    I3D_frame *neck = frame->FindChildFrame("neck", ENUMF_ALL);
    if (!neck)
        return false;
    S_vector d = pos - neck->GetWorldPos();
    if (d.AngleTo(dir) > PI / 3)
        return false;

    S_vector fdir = GetFrame()->GetWorldDir();
    fdir.Normalize();
    d.Normalize();
    S_vector back = fdir + d;
    back.Normalize();
    if (!base_frame)
        return false;

    // Into the model's space
    S_matrix m = GetFrame()->GetMatrix();
    m.Inverse(S_matrix(m));
    m(3, 0) = m(3, 1) = m(3, 2) = 0.0f;
    back *= m;
    pose_back1_dir = pose_back2_dir = back;
    pose_neck_dir = S_vector(0.0f, 0.0f, 1.0f);
    pose_flags &= ~HUMAN_POSEF_NULL_ROTATE;
    pose_flags |= HUMAN_POSEF_NEW | HUMAN_POSEF_AIMED;
    pose_pos = pos;
    pose_moving = true;
    return true;
}


// FUNCTION: GAME 0x57a560
void C_human::PoseNullRotate()
{
    pose_back1_dir = pose_back2_dir = pose_neck_dir = S_vector(0.0f, 0.0f, 1.0f);
    look_frame = NULL;
    pose_moving = true;
    pose_flags &= ~HUMAN_POSEF_AIMED;
    pose_flags |= HUMAN_POSEF_NULL_ROTATE | HUMAN_POSEF_NEW;
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

// FUNCTION: GAME 0x57a630
int C_human::GetAnimIDForDeath(int i, bool b, const S_vector &v1, const S_vector &v2)
{
    float angle = v2.AngleTo(v1);
    if (b)
    {
        if (i == HIT_PART_HEAD)
        {
            if (angle < PI / 2)
                return (rnd(2) ? 2 : 0) + ANIM_SMRT_DREP_HLAVA_ZEZADU;
            return (rnd(2) ? 1 : 0) + ANIM_SMRT_DREP_HLAVA_ZEPREDU;
        }
        if (angle < PI / 2)
            return rnd(2) + ANIM_SMRT_DREP_ZADA_A;
        return rnd(2) + ANIM_SMRT_DREP_BRICHO_A;
    }
    if (i == HIT_PART_HEAD)
    {
        if (angle < PI / 2)
            return (rnd(2) ? 2 : 0) + ANIM_SMRT_STOJ_HLAVA_ZEZADU;
        return (rnd(2) ? 1 : 0) + ANIM_SMRT_STOJ_HLAVA_ZEPREDU;
    }
    if (angle < PI / 2)
        return rnd(3) + ANIM_SMRT_STOJ_ZADA_A;
    return rnd(3) + ANIM_SMRT_STOJ_BRICHO_A;
}


// FUNCTION: GAME 0x57a710
void C_human::Movement(uint time)
{
    switch (action_state) {
    case HUMAN_ACTION_STAND:
        // Starts a fall
        if (fall <= MRG_ZERO) {
            if (!test_ground)
                break;
            fall = .05f;
        }
    case HUMAN_ACTION_FALL:
        if (act_mission->game->player == this && base_cache.IsOpen() && fall > 10.0f &&
                frame->GetWorldPos().y < -30.0f)
            fall = 60.0f;
        if (fall > 50.0f) {
            act_mission->game->DeathHumanTalk(this);
            properties[HUMAN_PROPERTY_ENERGY] = 0.0f;
            alive = false;
            Death();
            SetActState(ACTSTATE_NONE);
        }
    case HUMAN_ACTION_WALK:
        if (striking)
            striking = anim_machine.IsBlocking();
        MovementIntern(time);
        // How far the human got against how far he should have, every 100 ms for the player, 500 for the others
        move_time += time;
        if (move_time >= (GetType() == ACTOR_PLAYER ? 100 : 500)) {
            S_vector d = pos - move_pos;
            float expected = move_time * velocity.Magnitude() * .001f;
            if ((float)fabs(expected) < MRG_ZERO)
                move_ratio = 1.0f;
            else
                move_ratio = (d.Magnitude() / expected + move_ratio) * .5f;
            move_pos = pos;
            move_time = 0;
        }
        break;

    case HUMAN_ACTION_JUMP:
        if (striking)
            striking = anim_machine.IsBlocking();
        MovementIntern(time);
        break;

    case HUMAN_ACTION_STEPS:
        // The steps of the agony and the deaths
        switch (anim_steps[anim_step].type) {
        case HUMAN_STEP_WAIT:
            anim_steps[anim_step].wait -= time;
            if (anim_steps[anim_step].wait <= 0)
                anim_step++;
            break;
        case HUMAN_STEP_ANIM:
            next_anim_id = anim_steps[anim_step].anim_id;
            PersonAnim(0.0f, anim_steps[anim_step].stance, anim_steps[anim_step].flags & HUMAN_STEPF_ONCE);
            if (anim_steps[anim_step].flags & HUMAN_STEPF_WAIT && action_state != HUMAN_ACTION_TRACK) {
                uint anim_time, length;
                anim_machine.GetActiveAnimTime(anim_time, length);
                anim_steps[anim_step].wait += length;
                anim_steps[anim_step].type = HUMAN_STEP_WAIT;
            } else
                anim_step++;
            break;
        case HUMAN_STEP_AGONY:
            action_state = HUMAN_ACTION_AGONY;
            next_anim_id = agony_anim + ANIM_SMRT_STOJ_BRICHO_T_STATIV;
            if (rnd(4))
                action_time = 0;
            else
                action_time = rnd(2000) + 2000;
            break;
        case HUMAN_STEP_END:
            if (alive) {
                action_state = HUMAN_ACTION_STAND;
                next_anim_id = ANIM_BREATH01A;
                PersonAnim(0.0f, true, false);
            } else
                action_state = HUMAN_ACTION_DEAD;
            break;
        }
        break;

    case HUMAN_ACTION_DEAD:
        dead_time -= time;
        if (dead_time > 0)
            break;
        SetActState(ACTSTATE_NONE);
        if (killed_by_shot) {
            I3D_frame *back = frame->FindChildFrame("back2", ENUMF_ALL);
            if (back) {
                S_vector p = back->GetWorldPos() + S_vector(0.0f, .5f, 0.0f);
                S_vector d(0.0f, -1.0f, .01f);
                if (act_mission->options.game_effects >= OPTION_HIGH)
                    act_mission->game->blood_manager.AddBlood(p, d, .5f, 7, true);
            }
        }
        break;

    case HUMAN_ACTION_AGONY:
        // The agony: another one, or the death
        action_time -= time;
        if (action_time > 0)
            break;
        if (agony_count--) {
            anim_steps[1].type = HUMAN_STEP_AGONY;
            anim_steps[0].anim_id = agony_anim + ANIM_SMRT_STOJ_BRICHO_T_PLAZENI;
        } else {
            Death();
            action_state = HUMAN_ACTION_DEAD;
            anim_steps[1].type = HUMAN_STEP_END;
            anim_steps[0].anim_id = agony_anim + ANIM_SMRT_STOJ_BRICHO_T_END;
        }
        anim_steps[0].wait = 0;
        anim_steps[0].flags = HUMAN_STEPF_ONCE | HUMAN_STEPF_WAIT;
        anim_steps[0].type = HUMAN_STEP_ANIM;
        anim_steps[0].stance = false;
        action_state = HUMAN_ACTION_STEPS;
        anim_step = 0;
        PersonAnim(0.0f, true, false);
        break;

    case HUMAN_ACTION_STEAL:
        // Stealing the car
        action_time -= time;
        if (action_time <= 0) {
            action_state = HUMAN_ACTION_STAND;
            mafiaIndicators.show_flags &= ~INDF_THIEF;
            if (stealing_car)
                stealing_car->must_steal = false;
            if (weapon_model)
                weapon_model->SetOn(true);
            if (weapon_model2)
                weapon_model2->SetOn(true);
            I3D_sound *sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
            sound_table.ConstructSound(sound, SOUND_CAR_STOLEN);
            sound->SetPos(0.0f, 1.0f, 0.0f);
            sound->Update();
            sound->LinkTo(frame, 0);
            act_mission->game->Play3DSound(sound);
            act_mission->game->Stop3DSound(steal_sound, false, .001f);
            act_mission->game->shvestky.CarSteal(this, false, false);
            act_mission->game->camera.EnableFreeLook(false, false);
            stealing_car = NULL;
        } else if (act_mission->game->player == this)
            mafiaIndicators.ThiefIndicatorSetValue(1.0f - (float)action_time / steal_time);
        break;

    case HUMAN_ACTION_WAIT:
        // Waiting (SetWaitAnim's), still moving meanwhile
        action_time -= time;
        if (action_time <= 0) {
            switch (vehicle_action) {
            case HUMAN_VEHICLE_RAILWAY_IN:
                Intern_UseRailway(true);
                break;
            case HUMAN_VEHICLE_RAILWAY_OUT:
                Intern_UseRailway(false);
                break;
            }
            action_state = HUMAN_ACTION_STAND;
            striking = false;
        }
        if (!last_velocity.IsNull())
            pos += last_velocity * (float)time * .001f;
        break;

    case HUMAN_ACTION_CAR:
        // In the car: the doors closed once it goes, the shots from it
        if (!seat) {
            C_car *c = car;
            float speed = c->speed;
            if (speed > .5f && speed < 3.0f) {
                for (int i = c->seats.size(); i--;) {
                    bool b1, b2, b3, door_open;
                    c->GetSeatProperty(i, b1, b2, b3, door_open);
                    if (door_open)
                        c->OpenDoor(i, 0.0f, false);
                }
            }
        }
        if (car_stance_time >= 0)
            break;
        if (aimed) {
            bool can_aim;
            if (car_stance != HUMAN_CARSTANCE_AIMED &&
                (can_aim = seat ?
                weapon_kind == WEAPON_KIND_PISTOL || weapon_kind == WEAPON_KIND_SMG :
                weapon_kind == WEAPON_KIND_PISTOL)) {
                if (!car->IsWindowOpen(seat))
                    car->OpenWindow(seat, true);
                car_stance = HUMAN_CARSTANCE_AIMED;
                next_anim_id = ANIM_AUTO_FIRE01_L_ON;
                if (!car->IsSeatLeft(seat))
                    next_anim_id += 3;
                if (weapon_kind != WEAPON_KIND_PISTOL)
                    next_anim_id += 6;
                if (!seat)
                    next_anim_id = ANIM_AUTO_FIRE03_ON;
                PersonAnim(0.0f, false, true);
                uint anim_time, length;
                anim_machine.GetActiveAnimTime(anim_time, length);
                car_stance_time = length - 250;
                if (weapon_model)
                    weapon_model->SetOn(true);
                if (act_mission->game->player == this) {
                    act_mission->game->camera.EnableFreeLook(true, true);
                    mafiaIndicators.show_flags |= INDF_CROSSHAIR;
                }
                break;
            }
        } else if (car_stance == HUMAN_CARSTANCE_AIMED) {
            if (car->IsWindowOpen(seat))
                car->OpenWindow(seat, false);
            car_stance = HUMAN_CARSTANCE_SEATED;
            next_anim_id = ANIM_AUTO_FIRE01_L_OFF;
            if (!car->IsSeatLeft(seat))
                next_anim_id += 3;
            if (weapon_kind != WEAPON_KIND_PISTOL)
                next_anim_id += 6;
            if (!seat)
                next_anim_id = ANIM_AUTO_FIRE03_OFF;
            PersonAnim(0.0f, false, true);
            uint anim_time, length;
            anim_machine.GetActiveAnimTime(anim_time, length);
            car_stance_time = length - 250;
            if (act_mission->game->player == this) {
                act_mission->game->camera.EnableFreeLook(false, false);
                mafiaIndicators.show_flags &= ~INDF_CROSSHAIR;
            }
            break;
        }
        if (!car_stance && !seat) {
            car_stance = HUMAN_CARSTANCE_STEERING;
            next_anim_id = car->IsFormula() ? ANIM_FORMULE_VOLANT : ANIM_AUTO_RIDIC_VOLANT;
            PersonAnim(0.0f, true, false);
            car_anim_index = anim_machine.anim_index;
            if (anim_machine.stage_blending) {
                if (anim_machine.unknown_18 > 0.0f)
                    anim_machine.unknown_14 = 1.0f;
                else
                    anim_machine.unknown_14 = 0.0f;
            }
        }
        // The steering wheel's animation by the car's steering
        if (car_stance == HUMAN_CARSTANCE_STEERING) {
            ((I3D_model *)frame)->SetAnimTime(car_anim_index,
                (int)(((car->steer / car->steer_max + 1.0f) * .5f) * act_mission->game->wheel_anim_time));
            frame->Tick(0);
        }
        // The shot's animation by the angle to the target
        if (car_stance == HUMAN_CARSTANCE_AIMED) {
            float t = .5f;
            if (shot_frame) {
                S_vector dir = GetFrame()->GetWorldDir();
                S_vector d = throw_dir - shot_frame->GetWorldPos();
                if (d.Square() < 1.0f)
                    t = car_aim;
                else
                    t = dir.AngleTo(d) * (1.0f / PI);
            }
            float step = (float)time * .003f;
            if (fabs(t - car_aim) <= step)
                car_aim = t;
            else if (t < car_aim)
                car_aim -= step;
            else
                car_aim += step;
            ((I3D_model *)frame)->SetAnimTime(car_anim_index, (int)(car_anim_length * car_aim));
            frame->Tick(0);
        }
        if (anim_machine.stage_blending)
            anim_machine.StageBlendingTick(time);
        break;
    }
}


// The landing's force feedback effect, which InitFFEffects creates (0x647400; the name is ours, its bucket before
// ff_hit's)
// The human moved by his walk or the animation's move for that time (ms), on the ground or the vehicle he is in, with
// the fall and the landing: the ground found below him (a moving object's makes him test it again on the next move)
// and the move along it tested against the collisions; the water drowns him, a fall from the height starts the fall's
// animation, the landing after a long one hits him
// FUNCTION: GAME 0x57b660
void C_human::MovementIntern(uint time)
{
    float t = time * .001f;
    S_vector move;
    if (velocity.IsNull())
        move = last_velocity * velocity_ramp * t;
    else
        move = velocity * t;
    g_collision_header *coll = NULL;
    bool may_fall = true;
    if (test_ground || !move.IsNull() || action_state == HUMAN_ACTION_FALL) {
        DynSetNextCollOwnerMult();
        S_vector hitpos;
        if (car) {
            switch (car->type) {
            case ACTOR_RAILWAY:
                coll = (g_collision_header *)gifCollision.TestLineInVehicle(
                    &((C_railway *)car)->dyn_colls[wagon], pos + S_vector(0.0f, .5f, 0.0f),
                    S_vector(0.0f, -1.5f, 0.0f), &hitpos, &ground_normal, 2);
                break;
            case ACTOR_CAR:
                if (seat == -1)
                    coll = (g_collision_header *)gifCollision.TestLineInVehicle(((C_car_ex *)car)->dyn_colls,
                        pos + S_vector(0.0f, .5f, 0.0f), S_vector(0.0f, -1.5f, 0.0f), &hitpos, &ground_normal, 2);
                break;
            }
        } else {
            if (move_ratio < .3f)
                coll = (g_collision_header *)gifCollision.TestLineV(pos + S_vector(0.0f, .5f, 0.0f),
                    S_vector(0.0f, -1.5f, 0.0f), &hitpos, &ground_normal,
                    COLF_UNKNOWN_80000 | COLF_UNKNOWN_2, COLF2_UNKNOWN_800000);
            else {
                S_vector d = move;
                d.Normalize();
                gifCollision.step_actor = this;
                coll = (g_collision_header *)gifCollision.TestLineV(pos + S_vector(0.0f, .5f, 0.0f) + d * .3f,
                    S_vector(0.0f, -1.5f, 0.0f), &hitpos, &ground_normal,
                    COLF_UNKNOWN_80000 | COLF_UNKNOWN_2, COLF2_UNKNOWN_800000);
            }
            gifCollision.step_actor = NULL;
            if (coll && ground_normal.AngleTo(S_vector(0.0f, 1.0f, 0.0f)) > 1.0f)
                coll = NULL;
        }
        if (coll) {
            if (coll->material == MAT_WATER_DEATH) {
                if (!drowned) {
                    if (act_mission->GetGame()->GetPlayer() == this)
                        act_mission->GetGame()->GetCamera().SetMode(CAMERA_DEATH, true);
                    if (act_mission->options.particles >= OPTION_MEDIUM)
                        act_mission->GetEffects()->CreateParticle(PARTICLE_WATER_SPLASH, frame->GetWorldPos(),
                            S_vector(0.0f, 0.0f, 1.0f), 0, NULL);
                    if (act_mission->options.game_sounds >= OPTION_MEDIUM &&
                        (act_mission->GetScene()->GetActiveCamera()->GetWorldPos() - frame->GetWorldPos()).Square() <
                        900.0f) {
                        I3D_sound *sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
                        if (sound) {
                            sound->SetPos(frame->GetWorldPos());
                            sound_table.ConstructSound(sound, SOUND_WATER_SPLASH);
                            sound->SetVolume(.6f);
                            sound->SetOn(true);
                            act_mission->GetGame()->Play3DSound(sound);
                        }
                    }
                    drowned = true;
                    properties[HUMAN_PROPERTY_ENERGY] = 0.0f;
                    if (act_mission->GetGame()->GetPlayer() == this)
                        mafiaIndicators.PlayerSetLives(0);
                }
                move_coll = NULL;
            }
            ground_material = coll->material;
            if (type == ACTOR_PLAYER && ground_material == MAT_DEATH) {
                act_mission->GetGame()->game_failed = true;
                act_mission->GetGame()->WFall_Player();
                mafiaIndicators.PlayerSetLives(0);
            }
            if (hitpos.y * hitpos.y - pos.y * pos.y < .09f)
                may_fall = false;
            if (car)
                test_ground = false;
            else
                test_ground = coll->flags & COLLHF_UNKNOWN_40 || coll->flags & COLLHF_UNKNOWN_10000;
            if (pos.y - hitpos.y > .4f) {
                if (fall < 3.0f)
                    fall = 3.0f;
            }
        } else {
            test_ground = true;
            if (fall < 3.0f)
                fall = 3.0f;
        }
        if (action_state != HUMAN_ACTION_JUMP && !move.IsNull()) {
            float len = move.Magnitude();
            move = ground_normal.Cross(move.Cross(ground_normal));
            move.Normalize();
            move *= len;
        }
    }

    fall += t * 9.81;
    move.y -= fall * t;
    if (!move.IsNull()) {
        S_vector step = move;
        if ((action_state == HUMAN_ACTION_FALL || action_state == HUMAN_ACTION_STAND) && step.Square() < .01f) {
            step.Normalize();
            step *= .1f;
        }
        S_vector newpos;
        if (car)
            move_coll = (g_collision_header *)TestMoveCollInOwner(pos, step, newpos);
        else
            move_coll = (g_collision_header *)TestMoveColl(pos, step, newpos);
        if (move_coll && move_coll->material == MAT_WATER_DEATH) {
            if (!drowned) {
                if (act_mission->GetGame()->GetPlayer() == this)
                    act_mission->GetGame()->GetCamera().SetMode(CAMERA_DEATH, true);
                if (act_mission->options.particles >= OPTION_MEDIUM)
                    act_mission->GetEffects()->CreateParticle(PARTICLE_WATER_SPLASH, frame->GetWorldPos(),
                        S_vector(0.0f, 0.0f, 1.0f), 0, NULL);
                if (act_mission->options.game_sounds >= OPTION_MEDIUM &&
                    (act_mission->GetScene()->GetActiveCamera()->GetWorldPos() - frame->GetWorldPos()).Square() < 900.0f) {
                    I3D_sound *sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
                    if (sound) {
                        sound->SetPos(frame->GetWorldPos());
                        sound_table.ConstructSound(sound, SOUND_WATER_SPLASH);
                        sound->SetVolume(.6f);
                        sound->SetOn(true);
                        act_mission->GetGame()->Play3DSound(sound);
                    }
                }
                drowned = true;
                properties[HUMAN_PROPERTY_ENERGY] = 0.0f;
            }
            move_coll = NULL;
        }
        if (!move_coll)
            pos += move;
        else {
            bool landed = coll != NULL;
            if (action_state == HUMAN_ACTION_FALL && !landed) {
                float len = move.Magnitude();
                if (len > .01f) {
                    if ((newpos - pos).Magnitude() / len < .2f)
                        landed = true;
                } else
                    landed = true;
            }
            if (action_state == HUMAN_ACTION_FALL && landed) {
                action_state = HUMAN_ACTION_STAND;
                if (fall > 5.5f) {
                    velocity.Zero();
                    velocity_ramp = 0.0f;
                    if (alive) {
                        next_anim_id = ANIM_BREATH01A;
                        PersonAnim(0.0f, true, false);
                    }
                    if ((jump_fall ? 18 : 8) < fall)
                        Hit(HIT_LANDING, dir, pos, dir, (fall - (jump_fall ? 12 : 2)) * 10.0f, NULL, HIT_PART_BODY,
                            NULL);
                    else {
                        next_anim_id = ANIM_DOSKOK;
                        SetWaitAnim();
                    }
                    if (rendered && act_mission->options.particles >= OPTION_HIGH)
                        act_mission->GetEffects()->CreateParticle(PARTICLE_LANDING, frame->GetWorldPos(),
                            S_vector(0.0f, 1.0f, 0.0f), 0, NULL);
                    if (!step_sound->IsPlaying(false))
                        step_sound->SetOn(false);
                    if (sound_table.ConstructSound(step_sound, SOUND_LANDING)) {
                        step_sound->SetOn(true);
                        step_sound->SetVolume(1.0f);
                    }
                    if (ff_manager.human_effects && act_mission->GetGame()->GetPlayer() == this) {
                        float gain = fall * .1f;
                        if (gain > 1.0f)
                            gain = 1.0f;
                        else if (gain < .2f)
                            gain = .2f;
                        ff_manager.SetGain(ff_fall, gain);
                        ff_manager.Start(ff_fall);
                        ff_manager.SetGain(ff_fall, mafiaLoadSave.settings.ff_player_movement);
                    }
                } else if (alive) {
                    next_anim_id = ANIM_BREATH01A;
                    PersonAnim(0.0f, true, false);
                }
            } else if (action_state == HUMAN_ACTION_STAND) {
                newpos.x = pos.x;
                newpos.z = pos.z;
            }
            if (action_state != HUMAN_ACTION_FALL || landed)
                fall = 0.0f;
            pos = newpos;
            if (action_state == HUMAN_ACTION_JUMP)
                action_state = HUMAN_ACTION_STAND;
            if (move_coll->flags & COLLHF_UNKNOWN_40 && move_coll->owner) {
                C_actor *act = move_coll->owner;
                switch (act->type) {
                case ACTOR_BOTTLE:
                case ACTOR_GRENADE: {
                    S_vector d = dir;
                    d.Normalize();
                    act->Collision(this, newpos, d * 10.0f, dir, properties[HUMAN_PROPERTY_MASS], NULL);
                    break;
                }
                case ACTOR_CAR:
                case ACTOR_BRIDGE:
                    if (!car) {
                        S_vector d = dir;
                        act->Collision(this, newpos, d * -1.0f, dir, properties[HUMAN_PROPERTY_MASS],
                            NULL);
                    }
                    break;
                case ACTOR_ENTITY:
                    if (this == act_mission->GetGame()->GetPlayer())
                        ((C_entity *)act)->Event(this, ENTITY_EVENT_STEP_AWAY, NULL);
                    break;
                }
            }
        }
    }
    if (may_fall && action_state == HUMAN_ACTION_JUMP && fall < 10.0f)
        may_fall = false;
    if (may_fall && fall > 2.0f && action_state != HUMAN_ACTION_FALL) {
        jump_fall = action_state == HUMAN_ACTION_JUMP;
        next_anim_id = dead_body || box ? ANIM_BREATH01A : ANIM_PLACHTENI;
        action_state = HUMAN_ACTION_FALL;
        PersonAnim(0.0f, true, false);
    }
}


// The human's three spheres moved by dir from his place, against the level and the moving objects but his own
// collisions (DynSetNextCollOwnerMult's): the collision hit (TestPanak2's, a g_collision_header), 0 for none, and the
// new place into result, where the legs' sphere stops for a hit, else from + dir. A dead human lies: his spheres along
// his direction.
// FUNCTION: GAME 0x57cb60
int C_human::TestMoveColl(const S_vector &from, const S_vector &dir, S_vector &result)
{
    tgcPanak2 p;
    memset(&p, 0, sizeof(p));
    p.legs = frame->GetWorldPos() + S_vector(0.0f, .405f, 0.0f);
    if (alive) {
        p.body = p.legs + (crouching ? S_vector(0.0f, .3f, 0.0f) : S_vector(0.0f, .55f, 0.0f));
        p.head = p.legs + (crouching ? S_vector(0.0f, .6f, 0.0f) : S_vector(0.0f, 1.0f, 0.0f));
    } else {
        S_vector d = frame->GetWorldDir();
        p.body = p.legs + d * .5f;
        p.head = p.legs - d * .5f;
    }
    p.radius[0] = .4f;
    p.radius[1] = .25f;
    p.radius[2] = .35f;
    p.dir = dir;
    p.dir.y -= .015f;
    p.flags = (alive ? 0 : COLF_ALL) | COLF_UNKNOWN_80000 | COLF_UNKNOWN_2;
    p.flags2 = COLF2_UNKNOWN_800000;
    if (this != act_mission->GetGame()->GetPlayer())
        p.flags2 |= COLF2_UNKNOWN_80000;
    DynSetNextCollOwnerMult();
    int hit = gifCollision.TestPanak2(&p);
    gifCollision.DynSetNextCollOwner(NULL, NULL);
    if (!hit)
        result = from + dir;
    else
        result = p.pos_out - S_vector(0.0f, .4f, 0.0f);
    return hit;
}


// FUNCTION: GAME 0x57cfe0
inline void C_human::DynSetNextCollOwnerMult()
{
    int n = ghost_colls.size();
    if (n)
    {
        next_coll_owners.clear();
        int i;
        for (i = num_colls; i--; )
            next_coll_owners.push_back(colls[i]);
        for (i = ghost_colls.size(); i--; )
            next_coll_owners.push_back(ghost_colls[i]);
        gifCollision.DynSetNextCollOwnerMult(&next_coll_owners[0], next_coll_owners.size(), NULL);
    }
    else
        gifCollision.DynSetNextCollOwnerMult(colls, num_colls, NULL);
}


// The same in the vehicle the human is in, against its collisions, from his local place: the truck's for a human on
// it (no seat), the rail vehicle's of his wagon
// FUNCTION: GAME 0x57d0e0
int C_human::TestMoveCollInOwner(const S_vector &from, const S_vector &dir, S_vector &result)
{
    tgcPanak2 p;
    int hit = 0;
    p.legs = frame->GetPos() + S_vector(0.0f, .4f, 0.0f);
    p.body = p.legs + S_vector(0.0f, .75f, 0.0f);
    p.head = p.legs + S_vector(0.0f, 1.0f, 0.0f);
    p.radius[0] = .4f;
    p.radius[1] = .35f;
    p.radius[2] = .35f;
    p.dir = dir;
    p.dir.y -= .015f;
    p.flags = COLF_UNKNOWN_2;
    gifCollision.DynSetNextCollOwnerMult(colls, num_colls, NULL);
    switch (car->type) {
    case ACTOR_RAILWAY:
        hit = gifCollision.TestPanak2InVehicle(&((C_railway *)car)->dyn_colls[wagon], &p);
        break;
    case ACTOR_CAR:
        if (seat == -1)
            hit = gifCollision.TestPanak2InVehicle(((C_car_ex *)car)->dyn_colls, &p);
        break;
    }
    gifCollision.DynSetNextCollOwner(NULL, NULL);
    if (!hit)
        result = from + dir;
    else
        result = p.pos_out - S_vector(0.0f, .4f, 0.0f);
    return hit;
}


// FUNCTION: GAME 0x57d2e0
void C_human::Do_StopAnim()
{
    action_state = HUMAN_ACTION_STAND;
    if (alive)
    {
        anim_id = 0;
        next_anim_id = ANIM_BREATH01A;
        PersonAnim(0.0f, true, false);
    }
}

// FUNCTION: GAME 0x57d310
void C_human::Do_Breath()
{
    if (!alive)
        return;
    if (action_state == HUMAN_ACTION_STAND || action_state == HUMAN_ACTION_WALK || action_state == HUMAN_ACTION_FALL)
    {
        action_state = HUMAN_ACTION_STAND;
        turn_speed = 0;
        velocity.Zero();
        next_anim_id = ANIM_BREATH01A;
    }
}

// FUNCTION: GAME 0x57d360
void C_human::Go_Front(bool b)
{
    if (car && car->type == ACTOR_CAR && seat != -1)
        return;
    if (crouching || box || dead_body)
        b = false;
    action_state = HUMAN_ACTION_WALK;
    velocity = dir * (b ? 5.0f : 1.7f) * properties[HUMAN_PROPERTY_SPEED];
    next_anim_id = b ? ANIM_RUN1 : ANIM_WALK1;
}

// FUNCTION: GAME 0x57d430
void C_human::Go_Back()
{
    if (car && car->type == ACTOR_CAR && seat != -1)
        return;
    action_state = HUMAN_ACTION_WALK;
    velocity = -dir * 1.5f * properties[HUMAN_PROPERTY_SPEED];
    next_anim_id = ANIM_BACK1;
}

// FUNCTION: GAME 0x57d4d0
void C_human::Go_Left(bool b)
{
    if (car && car->type == ACTOR_CAR && seat != -1)
        return;
    if (crouching || box || dead_body)
        b = false;
    action_state = HUMAN_ACTION_WALK;
    turn_speed = 0;
    velocity.x = -dir.z;
    velocity.z = dir.x;
    velocity.y = dir.y;
    velocity *= (b ? 3.0f : 1.5f) * properties[HUMAN_PROPERTY_SPEED];
    next_anim_id = b ? ANIM_STRAFRL1 : ANIM_STRAFL1;
}

// FUNCTION: GAME 0x57d5a0
void C_human::Go_Right(bool b)
{
    if (car && car->type == ACTOR_CAR && seat != -1)
        return;
    if (crouching || box || dead_body)
        b = false;
    action_state = HUMAN_ACTION_WALK;
    turn_speed = 0;
    velocity.x = dir.z;
    velocity.z = -dir.x;
    velocity.y = dir.y;
    velocity *= (b ? 3.0f : 1.5f) * properties[HUMAN_PROPERTY_SPEED];
    next_anim_id = b ? ANIM_STRAFRR1 : ANIM_STRAFR1;
}

// FUNCTION: GAME 0x57d670
void C_human::Go_FrontLeft(bool b)
{
    if (car && car->type == ACTOR_CAR && seat != -1)
        return;
    if (crouching || box || dead_body)
        b = false;
    S_vector side(-dir.z, dir.y, dir.x);
    action_state = HUMAN_ACTION_WALK;
    turn_speed = 0;
    velocity = side + dir;
    velocity.Normalize();
    velocity *= (b ? 5.0f : 1.7f) * properties[HUMAN_PROPERTY_SPEED];
    next_anim_id = b ? ANIM_RUNL1 : ANIM_WALKL1;
}

// FUNCTION: GAME 0x57d860
void C_human::Go_FrontRight(bool b)
{
    if (car && car->type == ACTOR_CAR && seat != -1)
        return;
    if (crouching || box || dead_body)
        b = false;
    S_vector side(dir.z, dir.y, -dir.x);
    action_state = HUMAN_ACTION_WALK;
    turn_speed = 0;
    velocity = side + dir;
    velocity.Normalize();
    velocity *= (b ? 5.0f : 1.7f) * properties[HUMAN_PROPERTY_SPEED];
    next_anim_id = b ? ANIM_RUNR1 : ANIM_WALKR1;
}

// FUNCTION: GAME 0x57da50
void C_human::Go_BackLeft()
{
    if (car && car->type == ACTOR_CAR && seat != -1)
        return;
    S_vector side(-dir.z, dir.y, dir.x);
    action_state = HUMAN_ACTION_WALK;
    turn_speed = 0;
    velocity = side + -dir;
    velocity.Normalize();
    velocity *= properties[HUMAN_PROPERTY_SPEED] * 1.5f;
    next_anim_id = ANIM_BACKL1;
}

// FUNCTION: GAME 0x57dc10
void C_human::Go_BackRight()
{
    if (car && car->type == ACTOR_CAR && seat != -1)
        return;
    S_vector side(dir.z, dir.y, -dir.x);
    action_state = HUMAN_ACTION_WALK;
    turn_speed = 0;
    velocity = side + -dir;
    // The original tests the speed's length against MRG_ZERO here and does nothing with the result (PS2 goes on to
    // normalize it, as Go_BackLeft does)
    if (IsMrgZeroLess(velocity.Square()))
    {
    }
    velocity *= properties[HUMAN_PROPERTY_SPEED] * 1.5f;
    next_anim_id = ANIM_BACKR1;
}


// The animation of _anim_info_ named anim1 replaced by the file anim2, or given back its own without anim2 (the
// script's HUMAN_CHANGEANIM): an earlier replacement of it is dropped, and the animation is set again
// FUNCTION: GAME 0x57dd00
void C_human::ChangeAnim(char *anim1, char *anim2)
{
    int index = -1;
    int i;
    for (i = 0; _anim_info_[i].id != ANIM_INFO_END; i++) {
        if (!_strcmpi(anim1, _anim_info_[i].name)) {
            index = i;
            break;
        }
    }
    if (index != -1) {
        for (i = anims.size(); i--; ) {
            if (_anim_info_[index].id == anims[i].id) {
                delete[] anims[i].name;
                anims[i] = anims.back();
                anims.pop_back();
                break;
            }
        }
    }
    if (index != -1 && anim2) {
        S_anim_info a;
        a.id = _anim_info_[index].id;
        a.name = new char[strlen(anim2) + 1];
        strcpy(a.name, anim2);
        anims.push_back(a);
    }
    anim_id = 0;
}


// FUNCTION: GAME 0x57dfc0
int C_human::GetAnimIDForUseCar(bool b1, bool b2, bool b3, bool b4, bool b5, bool b6)
{
    if (b6)
    {
        int id = ANIM_FORMULE_NASTUP_L;
        if (b5)
            id = ANIM_FORMULE_VYSTUP_L;
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
    return id + ANIM_AUTO_SM_NAS_FL;
}


// Into the car's seat at once, without the animation (the scripts, the police, the traffic, the saved games): out of
// his animations, aim and collisions, seated as Intern_UseCar(true) seats him at the animation's end
// FUNCTION: GAME 0x57e020
void C_human::Intern_UseCar(C_car *c, int s)
{
    if (c->GetNumSeats() <= s || box || dead_body)
        return;
    c->SetOwner(this, s, properties[HUMAN_PROPERTY_MASS]);
    track_test = 0;
    if (this == act_mission->game->player)
        C_traffic_generator::PlayerInTaxi(c);
    C_traffic_generator::VyfunOdTeMasiny(c);
    anim_machine.DelExtendedAnim(4.0f);
    Do_Aimed(false);
    PoseNullRotate();
    PoseInterpolate(1000);
    DeactivateColls(false);
    fire_held = false;
    next_anim_id = !s ? (c->IsFormula() ? ANIM_FORMULE_STATIV : ANIM_AUTO_RIDIC_STATIV) : ANIM_AUTO_SPOL_STATIV;
    PersonAnim(0.0f, true, false);
    anim_machine.Tick(500);
    frame->Tick(500);
    DeactivateColls(false);
    unknown_b0 = 0;
    car = c;
    last_car = c;
    if (!s) {
        c->SetEngineOn(true, false);
        c->SetSpeedLimit(speed_limiter ? 16 : 1000);
        if (act_mission->game->player == this)
            c->auto_gearing = !((C_player *)this)->unknown_adb;
        else
            c->auto_gearing = true;
    }
    act_mission->game->DelFrameForShadow(frame, SHADOW_TYPE_0);
    shadow = 0;
    seat = s;
    dir = S_vector(0.0f, 0.0f, 1.0f);
    const S_vector &seat_pos = c->v_seats[s].pos;
    pos = seat_pos;
    frame->SetPos(seat_pos);
    frame->SetDir(dir, 0.0f);
    frame->Update();
    frame->LinkTo(car->frame, 0);
    frame->Update();
    action_state = HUMAN_ACTION_CAR;
    if (act_mission->game->player == this) {
        act_mission->game->camera.SetCar(c);
        c->unknown_490 = act_mission->game->camera.unknown_22;
        c->SetSteeringLinearity(mafiaLoadSave.settings.steering_linearity);
        if (!seat) {
            mafiaIndicators.show_flags |= INDF_RADAR | INDF_TACHOMETR;
            if (speed_limiter)
                mafiaIndicators.show_flags |= INDF_SPEED_LIMITER;
            if (C_player::can_car_damage)
                mafiaIndicators.show_flags |= INDF_CAR_DAMAGE;
            mafiaIndicators.show_flags &= ~INDF_CROSSHAIR;
            mafiaIndicators.RadarSetPlayerCar(c);
        }
        ((C_player *)this)->fade_alpha = 1.0f;
        ((C_player *)this)->SetAlpha(1.0f);
        // The car's lights for the player
        c->SetIndicatorAuto(true);
        c->SetLightUnknown800(true);
        c->SetLightUnknown8000(true);
        c->SetLight(false);
        c->SetLightUnknown10(false);
        c->SetLightUnknown20(false);
        c->invisible = false;
        c->SetDynamicCollsOn(true, c->GetUnknown1fac());
    }
    inventory.SetInCar(true, NULL);
    if (weapon_kind == WEAPON_KIND_RIFLE || weapon_kind == WEAPON_KIND_SMG) {
        if (!car->IsSeatLeft(seat))
            ChangeWeaponModel();
    }
    if (!seat && weapon_kind == WEAPON_KIND_PISTOL && car->IsSeatLeft(seat))
        ChangeWeaponModel();
    if (weapon_model)
        weapon_model->SetOn(false);
    if (weapon_model2)
        weapon_model2->SetOn(false);
    if (!seat && act_mission->game->shvestky.active)
        act_mission->game->shvestky.DriveCar(this, c);
    HideLegs((I3D_model *)frame, true);
}


// Out of the car at once onto the frame (the scripts, the traffic, the garage): placed at its world position and
// direction, back in the scene with his collisions and shadow, the items the inventory can't keep in hand dropped
// FUNCTION: GAME 0x57e500
void C_human::Intern_FromCar(I3D_frame *frm)
{
    if (!frm || !car || car->type != ACTOR_CAR || seat == -1)
        return;
    if (this == act_mission->GetGame()->player)
        C_traffic_generator::PlayerInTaxi(NULL);
    if (act_mission->GetGame()->player == this && aimed) {
        Do_Aimed(false);
        ((C_player *)this)->unknown_af5 = false;
        act_mission->GetGame()->GetCamera().EnableFreeLook(false, false);
    }
    C_car *c = car;
    action_state = HUMAN_ACTION_STAND;
    if (!seat) {
        c->SetEngineOn(false, false);
        if (act_mission->GetGame()->shvestky.active)
            act_mission->GetGame()->shvestky.DriveCar(this, NULL);
    }
    car = NULL;
    using_car = NULL;
    pos = frm->GetWorldPos();
    dir = frm->GetWorldDir();
    frame->SetPos(pos);
    frame->SetDir(dir, 0.0f);
    frame->Update();
    frame->LinkTo(act_mission->GetScene()->primary_sector, 0);
    if (base_frame)
        act_mission->scene->SetFrameSectorPos(frame, base_frame->GetWorldPos());
    if (act_mission->GetGame()->player == this)
        mafiaIndicators.show_flags &= ~(INDF_RADAR | INDF_TACHOMETR | INDF_SPEED_LIMITER | INDF_CAR_DAMAGE);
    c->SetOwner(NULL, seat, 0.0f);
    if (act_mission->GetGame()->initialized) {
        ActivateColls(HUMAN_COLLS_ALL);
        shadow = act_mission->GetGame()->AddFrameForShadow(frame, SHADOW_TYPE_0, SHADOW_TYPE_0, NULL, NULL);
    }
    if (act_mission->GetGame()->player == this) {
        ((C_player *)this)->IndicatorSetAimed();
        act_mission->GetGame()->GetCamera().SetCar(NULL);
    }
    std::vector<S_GameItem> dropped;
    inventory.SetInCar(false, &dropped);
    if (dropped.size())
        DropOutItems(dropped);
    ChangeWeaponModel();
    if (weapon_model)
        weapon_model->SetOn(true);
    if (weapon_model2)
        weapon_model2->SetOn(true);
    anim_machine.StopAnims();
    anim_machine.Reset();
    if (alive) {
        anim_id = 0;
        next_anim_id = ANIM_BREATH01A;
        PersonAnim(0.0f, true, false);
    }
    HideLegs((I3D_model *)frame, false);
}


// The weapon back in the hands after a reload let it go (PS2 name): in the right hand, a two-handed one held by the
// left hand's target too, by the right hand's on a car's right seat
// FUNCTION: GAME 0x57e980
void C_human::RepairWeaponPosition()
{
    if (loading_shells)
        return;
    if (fire_action[fire_step].action != FIRE_ACTION_END && fire_action != _fire_action_ready_ &&
        fire_action != _fire_action_aimed_ && fire_action != _fire_action_onoff_)
        return;
    if (!weapon_displaced)
        return;
    weapon_displaced = false;
    if (weapon_kind != WEAPON_KIND_RIFLE && weapon_kind != WEAPON_KIND_SMG)
        return;
    if (!left_hand || !right_hand)
        return;
    weapon_model->SetPos(S_vector(0.0f, 0.0f, 0.0f));
    weapon_model->SetDir(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
    weapon_model->LinkTo(right_hand, 0);
    weapon_model->Update();
    if (seat != -1 && (using_car && using_car->GetType() == ACTOR_CAR || car && car->GetType() == ACTOR_CAR))
    {
        if (weapon_kind == WEAPON_KIND_RIFLE || weapon_kind == WEAPON_KIND_SMG)
        {
            bool left = false;
            if (using_car && using_car->GetType() == ACTOR_CAR && using_car->GetNumSeats() > seat)
                left = using_car->IsSeatLeft(seat);
            if (car && car->GetType() == ACTOR_CAR && car->GetNumSeats() > seat)
                left = car->IsSeatLeft(seat);
            if (!left)
            {
                if (right_hand_target)
                {
                    weapon_model->LinkTo(left_hand, 0);
                    right_hand_target->AddToTarget(weapon_model);
                }
                return;
            }
        }
        else
        {
            if (weapon_kind == WEAPON_KIND_PISTOL && !seat)
                weapon_model->LinkTo(left_hand, 0);
            return;
        }
    }
    else if (weapon_kind != WEAPON_KIND_RIFLE && weapon_kind != WEAPON_KIND_SMG)
        return;
    if (left_hand_target)
        left_hand_target->AddToTarget(weapon_model);
}


// The weapons in the hands as their models, after the inventory changed (PS2 name): the right hand's weapon, a pistol
// in the left as the second model, the sounds, the frames a strike hits with, the hands' targets holding a
// two-handed weapon (as RepairWeaponPosition) and the fire's steps ready
// FUNCTION: GAME 0x57ec20
void C_human::ChangeWeaponModel()
{
    changing_weapon = false;
    ResetWeaponEffects();
    reload_effects = false;
    aimed = false;
    fire_held = false;
    throw_swung = false;
    if (this == act_mission->GetGame()->player && action_state != HUMAN_ACTION_STEAL)
        mafiaIndicators.show_flags &= ~INDF_THIEF;
    anim_machine.DelExtendedAnim(4.0f);
    reloading = false;
    reload_forced = false;
    loading_shells = false;
    strike_anim_time = -1;
    weapon_displaced = false;
    reload_effects = false;
    bool player = act_mission->GetGame()->player == this;
    if (player && ((C_player *)this)->unknown_af5)
    {
        ((C_player *)this)->unknown_af5 = false;
        act_mission->GetGame()->camera.EnableFreeLook(false, false);
    }
    if (type == ACTOR_ENTITY)
        ((C_entity *)this)->WeaponGetInfo();
    S_item_def *item = inventory.GetWeaponInRightHand();
    if (act_mission->GetGame()->player == this && !(item->flags & ITEMF_SCOPE))
        ((C_player *)this)->SetSniperMode(false);
    if (act_mission->GetGame()->player == this)
    {
        int spare;
        uint ammo = inventory.GetAmmo(&spare);
        mafiaIndicators.PlayerSetAmmo(ammo, spare);
    }
    if (!weapon_model)
        return;
    if (player)
        ((C_player *)this)->DelAlphaObject(weapon_model);
    if ((!car || car->GetType() != ACTOR_CAR || seat == -1) && !box && !dead_body)
    {
        weapon_model->SetOn(true);
        weapon_model2->SetOn(true);
    }
    if (left_hand_target)
        left_hand_target->DeleteFromTarget(weapon_model);
    if (right_hand_target)
        right_hand_target->DeleteFromTarget(weapon_model);
    weapon_model->LinkTo(NULL, 0);
    if (weapon_model2)
        weapon_model2->LinkTo(NULL, 0);
    S_item_def *item2 = NULL;
    if (weapon_model2)
        item2 = inventory.GetWeaponInLeftHand();
    weapon_kind = item->kind;
    if (weapon_sound)
    {
        weapon_sound->SetOn(false);
        sound_table.ConstructSound(weapon_sound, item->unknown_54);
    }
    if (reload_sound)
    {
        reload_sound->SetOn(false);
        sound_table.ConstructSound(reload_sound, item->reload_sound);
    }
    if (pump_sound)
    {
        pump_sound->SetOn(false);
        switch (inventory.GetItemInRightHand(NULL))
        {
        case GAMEITEM_SHOTGUN:
            sound_table.ConstructSound(pump_sound, SOUND_SHOTGUN_PUMP);
            break;
        case GAMEITEM_SPRINGFIELD:
        case GAMEITEM_MOSIN_NAGANT:
            sound_table.ConstructSound(pump_sound, SOUND_RIFLE_BOLT);
            break;
        }
    }
    weapon_model->Close();
    UnlinkAllFrameChild(weapon_model);
    bool loaded = false;
    if (strlen(item->model))
    {
        loaded = model_cache.Open(weapon_model, item->model, 0, NULL, NULL, NULL) == I3D_OK;
        if (loaded)
            weapon_model->SetName("weapon");
    }
    S_GameItem *in_hand;
    inventory.GetItemInRightHand(&in_hand);
    if (in_hand && in_hand->clip_out)
        BezZasobnikuWeaponEffect(in_hand->id, weapon_model);
    if (weapon_model2)
    {
        if (player)
            ((C_player *)this)->DelAlphaObject(weapon_model2);
        if (strlen(item2->model))
        {
            if (model_cache.Open(weapon_model2, item2->model, 0, NULL, NULL, NULL) == I3D_OK)
            {
                weapon_model2->SetPos(S_vector(0.0f, 0.0f, 0.0f));
                weapon_model2->SetDir(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
                weapon_model2->LinkTo(left_hand, 0);
                weapon_model2->Update();
                DebustizeFrame(weapon_model2);
                NoShadowFrame(weapon_model2);
                if (player)
                    ((C_player *)this)->AddAlphaObject(weapon_model2);
            }
        }
    }
    muzzle = -1;
    if (!loaded || !left_hand || !right_hand)
    {
        cartridge_frame = NULL;
        shot_frame = NULL;
    }
    else
    {
        DebustizeFrame(weapon_model);
        NoShadowFrame(weapon_model);
        weapon_model->SetPos(S_vector(0.0f, 0.0f, 0.0f));
        weapon_model->SetDir(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
        weapon_model->LinkTo(right_hand, 0);
        weapon_model->Update();
        if (player)
            ((C_player *)this)->AddAlphaObject(weapon_model);
        // STRING: GAME 0x636bb0
        cartridge_frame = weapon_model->FindChildFrame("zaver", ENUMF_ALL);
        // STRING: GAME 0x636ba8
        shot_frame = weapon_model->FindChildFrame("muzzle", ENUMF_ALL);
        if (shot_frame)
        {
            muzzle = atoi(shot_frame->GetProperty());
            if (act_mission->GetGame()->night_mission)
                muzzle++;
        }
        if (seat == -1 || (!using_car || using_car->GetType() != ACTOR_CAR) && (!car || car->GetType() != ACTOR_CAR))
        {
            if (weapon_kind == WEAPON_KIND_RIFLE || weapon_kind == WEAPON_KIND_SMG)
            {
            left_target:
                if (left_hand_target)
                    left_hand_target->AddToTarget(weapon_model);
            }
        }
        else if (weapon_kind == WEAPON_KIND_RIFLE || weapon_kind == WEAPON_KIND_SMG)
        {
            // On a car's right seat the right hand's target holds it
            bool left = false;
            if (using_car && using_car->GetType() == ACTOR_CAR && using_car->GetNumSeats() > seat)
                left = using_car->IsSeatLeft(seat);
            if (car && car->GetType() == ACTOR_CAR && car->GetNumSeats() > seat)
                left = car->IsSeatLeft(seat);
            if (left)
                goto left_target;
            if (right_hand_target)
            {
                weapon_model->LinkTo(left_hand, 0);
                right_hand_target->AddToTarget(weapon_model);
            }
        }
        else if (weapon_kind == WEAPON_KIND_PISTOL && !seat)
            weapon_model->LinkTo(left_hand, 0);
    }
    fire_action = _fire_action_ready_;
    fire_step = 0;
    fire_time = 0;
    if (act_mission->GetGame()->player == this && ff_manager.human_effects)
    {
        uint id = inventory.GetItemInRightHand(NULL);
        if (id == GAMEITEM_SHOTGUN || id == GAMEITEM_SAWED_OFF || id == GAMEITEM_SW_MAGNUM)
            ff_manager.SetDuration(ff_shot, 120000);
        else
            ff_manager.SetDuration(ff_shot, 80000);
    }
    striking = false;
    num_strike_frames = 0;
    if (!weapon_kind || weapon_kind == WEAPON_KIND_KNUCKLEDUSTER)
    {
        strike_frames[num_strike_frames] = left_hand;
        if (strike_frames[num_strike_frames])
            num_strike_frames++;
        strike_frames[num_strike_frames] = right_hand;
        if (strike_frames[num_strike_frames])
            num_strike_frames++;
    }
    else if (loaded && (weapon_kind == WEAPON_KIND_KNIFE || weapon_kind == WEAPON_KIND_BAT))
    {
        // STRING: GAME 0x636ba0
        strike_frames[num_strike_frames] = weapon_model->FindChildFrame("hit1", ENUMF_DUMMY);
        if (strike_frames[num_strike_frames])
            num_strike_frames++;
        // STRING: GAME 0x636b98
        strike_frames[num_strike_frames] = weapon_model->FindChildFrame("hit2", ENUMF_DUMMY);
        if (strike_frames[num_strike_frames])
            num_strike_frames++;
        // STRING: GAME 0x636b90
        strike_frames[num_strike_frames] = weapon_model->FindChildFrame("hit3", ENUMF_DUMMY);
        if (strike_frames[num_strike_frames])
            num_strike_frames++;
        // STRING: GAME 0x636b88
        strike_frames[num_strike_frames] = weapon_model->FindChildFrame("hit4", ENUMF_DUMMY);
        if (strike_frames[num_strike_frames])
            num_strike_frames++;
    }
    if (!car)
        act_mission->GetGame()->SetFrameForShadow(frame, SHADOW_TYPE_0, SHADOW_TYPE_0);
    if (act_mission->GetGame()->player == this)
        ((C_player *)this)->IndicatorSetAimed();
}


// The weapon taken out or put away (how 0) or thrown away (how 1), with the hands' animation and a sound near the
// camera (PS2 name; the parameters' names are ours: hide turns the old weapon's model off)
// FUNCTION: GAME 0x57f550
void C_human::Do_ChangeWeapon(int how, bool hide)
{
    if (!car)
    {
        if (changing_weapon)
            return;
        changing_weapon = true;
        if (weapon_model)
        {
            if (left_hand_target)
                left_hand_target->DeleteFromTarget(weapon_model);
            if (hide)
                weapon_model->SetOn(false);
        }
        ResetWeaponEffects();
        reload_effects = false;
        aimed = false;
        fire_held = false;
        throw_swung = false;
        if (this == act_mission->GetGame()->player)
            mafiaIndicators.show_flags &= ~INDF_THIEF;
        anim_machine.DelExtendedAnim(4.0f);
        reloading = false;
        reload_forced = false;
        loading_shells = false;
        strike_anim_time = -1;
        idle_time = rnd(5000) + 5000;
        breath_time = -1;
        if (act_mission->GetGame()->initialized)
        {
            fire_action = _fire_action_onoff_;
            fire_step = 0;
            const char *name = NULL;
            switch (how)
            {
            case HUMAN_WEAPON_ON_OFF:
                if (crouching)
                    // STRING: GAME 0x636bcc
                    name = "gun drep on off.i3d";
                else
                    // STRING: GAME 0x636bb8
                    name = "gun stoj on off.i3d";
                break;
            case HUMAN_WEAPON_THROW_AWAY:
                if (crouching)
                    // STRING: GAME 0x636bf8
                    name = "gun drep zahozeni.i3d";
                else
                    // STRING: GAME 0x636be0
                    name = "gun stoj zahozeni.i3d";
                ChangeWeaponModel();
                break;
            }
            if (name)
            {
                I3D_animation_set *set = NULL;
                anim_cache.Create(name, &set, NULL, NULL);
                if (set)
                {
                    anim_machine.SetExtendedAnim(set, 1.0f, 4.0f, true);
                    set->Release();
                }
            }
            if (act_mission->skip_frames > 0)
            {
                act_mission->skip_frames--;
                return;
            }
            if (act_mission->options.game_sounds >= OPTION_HIGH)
            {
                if ((act_mission->GetScene()->GetActiveCamera()->GetWorldPos() - frame->GetWorldPos()).Square() <
                    64.0f)
                {
                    I3D_sound *sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
                    if (sound)
                    {
                        sound->SetPos(frame->GetWorldPos() + S_vector(0.0f, 1.0f, 0.0f));
                        sound_table.ConstructSound(sound, SOUND_WEAPON_CHANGE);
                        sound->SetOn(true);
                        act_mission->GetGame()->Play3DSound(sound);
                    }
                }
            }
            return;
        }
    }
    // In a car, or while the game isn't running, the model just changes
    ChangeWeaponModel();
}


// FUNCTION: GAME 0x57f830
void C_human::Do_Aimed(bool aim)
{
    if (box || dead_body)
    {
        aimed = false;
        return;
    }
    if (aim && changing_weapon)
        ChangeWeaponModel();
    aimed = aim;
    if (!aim && guarding)
    {
        anim_machine.DelExtendedAnim(4.0f);
        guarding = false;
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
    TruncateAmmo(inventory.big_weapon);
    for (int i = INVENTORY_POCKETS; i--; )
        TruncateAmmo(inventory.pockets[i]);
}


// The items thrown out of the hands as dropped weapons where the human stands (PS2 name): each one's model on a
// C_drop_in_weapon of the game's temporary actors, its ammunition cut to its table's most; true if any was dropped
// FUNCTION: GAME 0x57faa0
bool C_human::DropOutItems(std::vector<S_GameItem> &items)
{
    if (!right_hand || !left_hand)
        return false;
    if (!items.size())
        return false;
    idle_time = rnd(5000) + 5000;
    breath_time = -1;
    aimed = false;
    fire_held = false;
    throw_swung = false;
    if (this == act_mission->GetGame()->player)
        mafiaIndicators.show_flags &= ~INDF_THIEF;
    bool dropped = false;
    for (int i = items.size(); i--;)
    {
        const char *name = gameItemTable[items[i].id].model;
        if (strlen(name))
        {
            I3D_model *model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
            if (model)
            {
                if (model_cache.Open(model, name, 0, NULL, NULL, NULL) == I3D_OK)
                {
                    DebustizeFrame(model);
                    NoShadowFrame(model);
                    if (!crouching)
                        model->SetPos(right_hand->GetWorldPos());
                    else
                    {
                        const S_vector &pos = frame->GetWorldPos();
                        model->SetPos(S_vector(pos.x, pos.y + .6f, pos.z));
                    }
                    model->SetDir(dir, 0.0f);
                    model->SetScale(S_vector(1.0f, 1.0f, 1.0f));
                    model->Update();
                    act_mission->GetScene()->SetFrameSectorPos(model, model->GetWorldPos());
                    if (!items[i].ammo_loaded)
                        FakeWeaponEffect(model, items[i].id, true);
                    if (items[i].clip_out)
                        BezZasobnikuWeaponEffect(items[i].id, model);
                    C_drop_in_weapon *weapon = new C_drop_in_weapon;
                    if (weapon->Init(model))
                    {
                    // The sound of the item falling, by its kind
                    switch (items[i].id)
                    {
                    case GAMEITEM_KNUCKLEDUSTER:
                    case GAMEITEM_KNIFE:
                    case GAMEITEM_BAR:
                    case GAMEITEM_CROWBAR:
                    case GAMEITEM_BOTTLE:
                    case GAMEITEM_SWORD:
                        weapon->unknown_348 = 0xb;
                        break;
                    case GAMEITEM_BASEBALL_BAT:
                    case GAMEITEM_BOARD:
                        weapon->unknown_348 = 0xa;
                        break;
                    case GAMEITEM_COLT_DETECTIVE:
                    case GAMEITEM_SW_MAGNUM:
                    case GAMEITEM_SW_MP:
                    case GAMEITEM_COLT_1911:
                        weapon->unknown_348 = 9;
                        break;
                    case GAMEITEM_MOLOTOV:
                    case GAMEITEM_THOMPSON:
                    case GAMEITEM_SHOTGUN:
                    case GAMEITEM_SAWED_OFF:
                    case GAMEITEM_SPRINGFIELD:
                    case GAMEITEM_MOSIN_NAGANT:
                        weapon->unknown_348 = 0xc;
                        break;
                    case GAMEITEM_GRENADE:
                        weapon->unknown_348 = 0;
                        break;
                    case GAMEITEM_BUCKET:
                        weapon->unknown_348 = 3;
                        break;
                    case GAMEITEM_FLASHLIGHT:
                        weapon->unknown_348 = 8;
                        break;
                    default:
                        weapon->unknown_348 = -1;
                        break;
                    }
                    S_THING_BOX thing_box;
                    thing_box.weight = 10.0f;
                    thing_box.velocity = S_vector(dir.z, .2f, -dir.x);
                    thing_box.unknown_10 = .45f;
                    weapon->InitBox(&thing_box);
                    weapon->item = items[i];
                    TruncateAmmo(weapon->item);
                    act_mission->GetGame()->AddTemporaryActor(weapon);
                    dropped = true;
                    }
                    else
                    {
                        weapon->Release();
                        model->LinkTo(NULL, 0);
                        model->Release();
                    }
                }
                else
                    model->Release();
            }
        }
    }
    return dropped;
}


// FUNCTION: GAME 0x5801f0
C_bridge *C_human::TozNaselJsemTenNejvyssiMost()
{
    if (car && car->type == ACTOR_CAR && car->IsUnknown2000000())
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


// Whether the human can get out of the car's seat on its free side (Do_ThrowCocotFromCar): the ground found below a
// point 2.5 m to the seat's side (GetSeatProperty's first flag, +x with it), and room for a sphere there and on the way
// out, the car's own collision left out
// FUNCTION: GAME 0x5802f0
bool C_human::Can_DropOutFromCar_Free(C_car *c, int s)
{
    if (!c)
        return false;
    bool b1, b2, b3, b4;
    if (!c->GetSeatProperty(s, b1, b2, b3, b4))
        return false;
    S_vector off(2.5f, 0.0f, 0.0f);
    if (!b1)
        off.x *= -1.0f;
    S_matrix m = c->frame->GetMatrix();
    m(3).x = m(3).y = m(3).z = 0.0f;
    off *= m;
    off.y = 0.0f;
    next_coll_owners.clear();
    next_coll_owners.push_back((tDynamicCollObject *)c->dyn_coll);
    gifCollision.DynSetNextCollOwnerMult(&next_coll_owners[0], next_coll_owners.size(), NULL);
    S_vector pos = c->v_seats[s].pos * c->GetFrame()->GetMatrix();
    S_vector hit;
    if (gifCollision.TestLineV(pos + off + S_vector(0.0f, 1.0f, 0.0f), S_vector(0.0f, -2.0f, 0.0f), &hit, NULL,
        COLF_UNKNOWN_100 | COLF_UNKNOWN_2, 0))
        pos.y = hit.y + .7f;
    else
        pos.y += 1.0f;
    if (gifCollision.TestSphereB(pos, off, .3f, NULL, NULL, COLF_UNKNOWN_100 | COLF_UNKNOWN_2, 0)) {
        gifCollision.DynSetNextCollOwner(NULL, NULL);
        return false;
    }
    if (gifCollision.TestSphereB(pos + off, S_vector(0.0f, .3f, 0.0f), .3f, NULL, NULL,
        COLF_UNKNOWN_100 | COLF_UNKNOWN_2, 0)) {
        gifCollision.DynSetNextCollOwner(NULL, NULL);
        return false;
    }
    gifCollision.DynSetNextCollOwner(NULL, NULL);
    return true;
}


// Whether the human can get out of the car he is in from the seat (his own for -1, Use_Actor's): the ground found
// below a point 1.2 m to the seat's side of him, and room for a sphere there and on the way out, his own collisions
// and the car's left out
// FUNCTION: GAME 0x5805c0
bool C_human::Can_DropOutFromCar(int s)
{
    if (s < 0)
        s = seat;
    C_car *c = car;
    if (!c)
        return false;
    bool b1, b2, b3, b4;
    if (!c->GetSeatProperty(s, b1, b2, b3, b4))
        return false;
    S_vector off(1.2f, 0.0f, 0.0f);
    if (!b1)
        off.x *= -1.0f;
    S_matrix m = frame->GetMatrix();
    m(3).x = m(3).y = m(3).z = 0.0f;
    off *= m;
    off.y = 0.0f;
    next_coll_owners.clear();
    for (int i = num_colls; i--; )
        next_coll_owners.push_back(colls[i]);
    next_coll_owners.push_back((tDynamicCollObject *)c->dyn_coll);
    gifCollision.DynSetNextCollOwnerMult(&next_coll_owners[0], next_coll_owners.size(), NULL);
    S_vector pos = frame->GetWorldPos();
    S_vector hit;
    if (gifCollision.TestLineV(pos + off + S_vector(0.0f, 1.0f, 0.0f), S_vector(0.0f, -2.0f, 0.0f), &hit, NULL,
        COLF_UNKNOWN_100 | COLF_UNKNOWN_2, 0))
        pos.y = hit.y + .7f;
    else
        pos.y += 1.0f;
    if (gifCollision.TestSphereB(pos, off, .3f, NULL, NULL, COLF_UNKNOWN_100 | COLF_UNKNOWN_2, 0)) {
        gifCollision.DynSetNextCollOwner(NULL, NULL);
        return false;
    }
    if (gifCollision.TestSphereB(pos + off, S_vector(0.0f, .3f, 0.0f), .3f, NULL, NULL,
        COLF_UNKNOWN_100 | COLF_UNKNOWN_2, 0)) {
        gifCollision.DynSetNextCollOwner(NULL, NULL);
        return false;
    }
    gifCollision.DynSetNextCollOwner(NULL, NULL);
    return true;
}


// Whether the human, in no car, can get into the car's seat (Use_Actor, the entities' AI): the ground found below a
// point 1.3 m to the seat's side of the car, room for him there, and with test_line no part of the car between that
// point and a metre above his own position (the name is ours)
// FUNCTION: GAME 0x5808c0
bool C_human::Can_EnterToCar(C_car *c, int s, bool test_line)
{
    if (car)
        return false;
    bool b1, b2, b3, b4;
    if (!c->GetSeatProperty(s, b1, b2, b3, b4))
        return false;
    S_vector off(1.3f, 0.0f, 0.0f);
    if (!b1)
        off.x *= -1.0f;
    S_matrix m = c->frame->GetMatrix();
    m(3).x = m(3).y = m(3).z = 0.0f;
    off *= m;
    off.y = 0.0f;
    S_vector pos = c->v_seats[s].pos;
    pos *= c->frame->GetMatrix();
    next_coll_owners.clear();
    for (int i = num_colls; i--; )
        next_coll_owners.push_back(colls[i]);
    next_coll_owners.push_back((tDynamicCollObject *)c->dyn_coll);
    gifCollision.DynSetNextCollOwnerMult(&next_coll_owners[0], next_coll_owners.size(), NULL);
    S_vector hit;
    if (gifCollision.TestLineV(pos + off + S_vector(0.0f, 1.0f, 0.0f), S_vector(0.0f, -2.0f, 0.0f), &hit, NULL,
        COLF_UNKNOWN_100 | COLF_UNKNOWN_2, 0))
        pos.y = hit.y + .7f;
    else
        pos.y += 1.0f;
    if (gifCollision.TestLineH(pos, off, NULL, NULL, COLF_UNKNOWN_100 | COLF_UNKNOWN_2, 0)) {
        gifCollision.DynSetNextCollOwner(NULL, NULL);
        return false;
    }
    if (gifCollision.TestSphereB(pos + off, S_vector(0.0f, .5f, 0.0f), .3f, NULL, NULL,
        COLF_UNKNOWN_100 | COLF_UNKNOWN_2, 0)) {
        gifCollision.DynSetNextCollOwner(NULL, NULL);
        return false;
    }
    gifCollision.DynSetNextCollOwner(NULL, NULL);
    if (test_line) {
        // The line from there to a metre above him, in hit again
        pos += off;
        I3D_COLLISION col;
        hit = frame->GetWorldPos() + S_vector(0.0f, 1.0f, 0.0f) - pos;
        if (act_mission->scene->TestColHierarchy(pos, hit, c->GetFrame(), col, 0, NULL, 0, NULL))
            return false;
    }
    return true;
}


// Getting out of the car the human sits in, beside his seat (Czech: "the crappy climbing out of the car in the
// morning"): false where he isn't in a car's seat. A car on its roof or side, or a wall beside the seat, throws him out
// instead (ZasrateVybliknutiZAuta_ve3rano). Out, the car loses its owner, the human his place in it, and the player
// the car's indicators; the camera looks at him from where it is
// FUNCTION: GAME 0x580ca0
bool C_human::ZasrateLezeniZAuta_ve2rano()
{
    C_car *c = car;
    if (!c || c->type != ACTOR_CAR || seat == -1)
        return false;
    S_matrix m = c->frame->GetMatrix();
    m(3).x = m(3).y = m(3).z = 0.0f;
    S_vector v(0.0f, 1.0f, 0.0f);
    v *= m;
    // The car on its side or roof
    if (v.AngleTo(S_vector(0.0f, -1.0f, 0.0f)) > .55f) {
        if (v.AngleTo(S_vector(0.0f, 1.0f, 0.0f)) > .7f) {
            ZasrateVybliknutiZAuta_ve3rano();
            return true;
        }
        return false;
    }
    // Beside the seat, on its side of the car
    v = pos = c->v_seats[seat].pos;
    if (pos.x < 0.0f) {
        pos.x = -2.5f;
        dir = S_vector(-1.0f, 0.0f, 0.0f);
    } else {
        pos.x = 2.5f;
        dir = S_vector(1.0f, 0.0f, 0.0f);
    }
    v *= c->frame->GetMatrix();
    pos *= c->frame->GetMatrix();
    dir *= m;
    pos.y += _get_delta_y_(pos);
    S_vector d = pos - v;
    d.y = 0.0f;
    v.y = pos.y + .5f;
    if (gifCollision.TestLineH(v, d, NULL, NULL, 0, 0)) {
        ZasrateVybliknutiZAuta_ve3rano();
        return true;
    }
    if (!seat) {
        c->SetEngineOn(false, false);
        if (act_mission->game->shvestky.active)
            act_mission->GetGame()->shvestky.DriveCar(this, NULL);
    }
    car = NULL;
    using_car = NULL;
    AIManager.police.PanoveKokotPremistujePrdel(this, c, false);
    frame->SetPos(pos);
    frame->SetDir(dir, 0.0f);
    frame->Update();
    frame->LinkTo(act_mission->GetScene()->primary_sector, 0);
    if (base_frame)
        act_mission->scene->SetFrameSectorPos(frame, base_frame->GetWorldPos());
    c->SetOwner(NULL, seat, 0.0f);
    if (act_mission->GetGame()->initialized) {
        ActivateColls(HUMAN_COLLS_ALL);
        shadow = act_mission->GetGame()->AddFrameForShadow(frame, SHADOW_TYPE_0, SHADOW_TYPE_0, NULL, NULL);
    }
    if (act_mission->GetGame()->player == this) {
        ((C_player *)this)->IndicatorSetAimed();
        mafiaIndicators.show_flags &= ~(INDF_RADAR | INDF_TACHOMETR | INDF_SPEED_LIMITER | INDF_CAR_DAMAGE | INDF_MAP);
        act_mission->GetGame()->camera.PushMode();
        act_mission->GetGame()->camera.SetLookTo(act_mission->GetScene()->GetActiveCamera()->GetWorldPos(), frame);
    }
    std::vector<S_GameItem> dropped;
    inventory.SetInCar(false, &dropped);
    if (dropped.size())
        DropOutItems(dropped);
    ChangeWeaponModel();
    if (weapon_model)
        weapon_model->SetOn(true);
    if (weapon_model2)
        weapon_model2->SetOn(true);
    next_anim_id = ANIM_AUTO_LEZENI_Z_VRAKU;
    PersonAnim(0.0f, true, false);
    anim_machine.SetStageBlendingDir();
    uint time, length;
    anim_machine.GetActiveAnimTime(time, length);
    if (length > 250) {
        action_time = length - 250;
        last_velocity.Zero();
        velocity_ramp = 0.0f;
        action_state = HUMAN_ACTION_WAIT;
    }
    HideLegs((I3D_model *)frame, false);
    return true;
}


// Thrown out of the car the human sits in (Czech: "the crappy throwing up out of the car in the morning"): beside the
// car's front AI point, on the seat's side; where the spheres hit something there, beside the other point on the
// other side, and failing that on the car's roof above the seat. Out, the car loses its owner, the human his place in
// it, and the player the car's indicators and camera
// FUNCTION: GAME 0x581220
void C_human::ZasrateVybliknutiZAuta_ve3rano()
{
    S_vector d;
    C_car *c = car;
    if (!c || c->type != ACTOR_CAR || seat == -1)
        return;
    const S_vector &ai_front = c->GetAIPos(true);
    const S_vector &ai_back = c->GetAIPos(false);
    S_vector wpos = c->GetWorldPos();
    dir = c->GetWorldDir();
    d = ai_front - wpos;
    float len = d.Magnitude();
    d.Normalize();
    d *= len + .5f;
    pos = wpos + d;
    d.Normalize();
    d *= .4f;
    S_vector side = S_vector(-d.z, 0.0f, d.x);
    if (!c->IsSeatLeft(seat))
        side *= -1.0f;
    pos += side;
    pos.y += _get_delta_y_(pos);
    S_vector from = pos + S_vector(0.0f, .5f, 0.0f) - dir * 1.3f;
    int coll1 = gifCollision.TestSphereB(from, dir * 1.6f, .25f, NULL, NULL, COLF_UNKNOWN_2, 0);
    int coll2 = gifCollision.TestSphereB(pos + S_vector(0.0f, .5f, 0.0f), S_vector(0.0f, .5f, 0.0f), .3f, NULL, NULL,
        COLF_UNKNOWN_2, 0);
    if (coll1 || coll2) {
        // Beside the other point, on the other side
        d = ai_back - wpos;
        len = d.Magnitude();
        d.Normalize();
        d *= len + .5f;
        pos = wpos + d;
        d.Normalize();
        d *= .4f;
        S_vector side = S_vector(-d.z, 0.0f, d.x);
        if (c->IsSeatLeft(seat))
            side *= -1.0f;
        pos += side;
        pos.y += _get_delta_y_(pos);
        from = pos + S_vector(0.0f, .5f, 0.0f) + dir * 1.3f;
        coll1 = gifCollision.TestSphereB(from, dir * -1.6f, .25f, NULL, NULL, COLF_UNKNOWN_2, 0);
        coll2 = gifCollision.TestSphereB(pos + S_vector(0.0f, .5f, 0.0f), S_vector(0.0f, .5f, 0.0f), .3f, NULL, NULL,
            COLF_UNKNOWN_2, 0);
        if (coll1 || coll2) {
            // On the roof
            pos = c->v_seats[seat].pos;
            pos *= c->GetFrame()->GetMatrix();
            pos.y += 1.5f;
            pos.y += _get_delta_y_(pos);
        }
    }
    if (!seat) {
        c->SetEngineOn(false, false);
        if (act_mission->GetGame()->shvestky.active)
            act_mission->GetGame()->shvestky.DriveCar(this, NULL);
    }
    car = NULL;
    using_car = NULL;
    AIManager.police.PanoveKokotPremistujePrdel(this, c, false);
    GetFrame()->SetPos(pos);
    GetFrame()->SetDir(dir, 0.0f);
    GetFrame()->Update();
    GetFrame()->LinkTo(act_mission->GetScene()->primary_sector, 0);
    if (base_frame)
        act_mission->scene->SetFrameSectorPos(GetFrame(), base_frame->GetWorldPos());
    if (act_mission->GetGame()->player == this)
        mafiaIndicators.show_flags &= ~(INDF_TACHOMETR | INDF_SPEED_LIMITER | INDF_CAR_DAMAGE | INDF_MAP);
    c->SetOwner(NULL, seat, 0.0f);
    if (act_mission->GetGame()->initialized) {
        ActivateColls(HUMAN_COLLS_ALL);
        shadow = act_mission->GetGame()->AddFrameForShadow(GetFrame(), SHADOW_TYPE_0, SHADOW_TYPE_0, NULL, NULL);
    }
    if (act_mission->GetGame()->player == this) {
        ((C_player *)this)->IndicatorSetAimed();
        act_mission->GetGame()->camera.SetCar(NULL);
        mafiaIndicators.show_flags &= ~(INDF_RADAR | INDF_TACHOMETR | INDF_SPEED_LIMITER | INDF_CAR_DAMAGE | INDF_MAP);
    }
    std::vector<S_GameItem> dropped;
    inventory.SetInCar(false, &dropped);
    if (dropped.size())
        DropOutItems(dropped);
    ChangeWeaponModel();
    if (weapon_model)
        weapon_model->SetOn(true);
    if (weapon_model2)
        weapon_model2->SetOn(true);
    action_state = HUMAN_ACTION_STAND;
    Do_Breath();
    PersonAnim(0.0f, true, false);
    anim_machine.SetStageBlendingDir();
    HideLegs((I3D_model *)GetFrame(), false);
}


// Over to the seat beside his own in the car (the other of the pair, 0 and 1, 2 and 3), when it is free: false where
// he can't. Seated at once, he plays the climbing over (0x130 or 0x131), stretched along x by the seats' distance
// against the track's mixing point; the seat taken at the animation's end (HUMAN_VEHICLE_SEAT_CHANGE)
// FUNCTION: GAME 0x581eb0
bool C_human::Do_ClimbInCarLR()
{
    C_car *c = car;
    if (!c)
        return false;
    if (c->type != ACTOR_CAR)
        return false;
    if (seat < 0)
        return false;
    int other = seat & 1 ? seat - 1 : seat + 1;
    if (c->GetOwner(other))
        return false;
    if (!seat) {
        c->SetEngineOn(false, false);
        if (act_mission->game->shvestky.active)
            act_mission->game->shvestky.DriveCar(this, NULL);
        if (act_mission->game->player == this)
            mafiaIndicators.show_flags &= ~(INDF_RADAR | INDF_TACHOMETR | INDF_SPEED_LIMITER | INDF_CAR_DAMAGE | INDF_MAP);
    }
    dir = S_vector(0.0f, 0.0f, 1.0f);
    const S_vector &seat_pos = c->v_seats[seat].pos;
    pos = seat_pos;
    frame->SetPos(seat_pos);
    frame->SetDir(dir, 0.0f);
    frame->Update();
    c->SetOwner(NULL, seat, 0.0f);
    next_anim_id = seat & 1 | ANIM_AUTO_PRELEZ_L_TO_R;
    int index = GetAnimIndex(next_anim_id);
    if (index >= 0) {
        char buff[128];
        strcpy(buff, _anim_info_[index].name);
        char *ext = strrchr(buff, '.');
        if (ext)
            *ext = 0;
        strcat(buff, ".tck");
        S_vector mix;
        C_animations_machine_de_luxe::GetMixingPointOfTCK(buff, mix);
        float len = (float)fabs(mix.x);
        S_vector d = c->v_seats[seat].pos - c->v_seats[other].pos;
        if (len > .1f)
            climb_scale = (float)fabs(d.x) / len;
        else
            climb_scale = 1.0f;
    }
    PersonAnim(0.0f, true, false);
    vehicle_action = HUMAN_VEHICLE_SEAT_CHANGE;
    c->SetOwner(NULL, seat, 0.0f);
    c->SetOwner(this, other, properties[HUMAN_PROPERTY_MASS]);
    seat = other;
    if (weapon_kind == WEAPON_KIND_RIFLE || weapon_kind == WEAPON_KIND_SMG)
        ChangeWeaponModel();
    if (!seat && weapon_kind == WEAPON_KIND_PISTOL)
        ChangeWeaponModel();
    return true;
}


// The human uses the actor: a door opens or closes, a turnout switches; a car: into its seat b (how 1, the animation
// to the seat played, the human the seat's owner; with c he climbs over to the other seat of the pair once seated,
// next_car_action)
// or out of the seat he is in (another how: beside the car, thrown out, or over to the next seat); a rail vehicle: in
// or out through its nearest entry
// FUNCTION: GAME 0x582180
void C_human::Use_Actor(C_actor *act, int how, int b, int c)
{
    PoseNullRotate();
    switch (act->type) {
    case ACTOR_DOOR: {
        C_door *door = (C_door *)act;
        switch (door->state) {
        case C_door::STATE_CLOSED:
            door->SetState(C_door::STATE_OPENING, this, true, true);
            UniversalUse();
            break;
        case C_door::STATE_OPEN:
            door->SetState(C_door::STATE_CLOSING, this, true, true);
            UniversalUse();
            break;
        }
        car = NULL;
        break;
    }
    case ACTOR_TURNOUT:
        ((C_turnout *)act)->Start();
        UniversalUse();
        break;
    case ACTOR_CAR: {
        C_car *vehicle = (C_car *)act;
        if (act_mission->game->player == this)
            ((C_player *)this)->SetSniperMode(false);
        throw_swung = false;
        if (this == act_mission->game->player)
            mafiaIndicators.show_flags &= ~INDF_THIEF;
        if (how == HUMAN_USE_IN) {
            // Into the seat b
            if (!vehicle || box || dead_body)
                break;
            if (type != ACTOR_ENTITY && !Can_EnterToCar(vehicle, b, true))
                break;
            bool b1, b2, b3, door;
            vehicle->GetSeatProperty(b, b1, b2, b3, door);
            C_car *c2 = C_traffic_car::SebraliMiKaaaruSvine(vehicle);
            if (!c2)
                break;
            if (c2 != vehicle) {
                if (door)
                    c2->OpenDoor(b, 1.0f, true);
                act_mission->game->shvestky.DezoSebralGumyIRadio(c2);
            }
            vehicle = c2;
            act_mission->game->shvestky.RenamePoliceCar(c2);
            c2->GetSeatProperty(b, b1, b2, b3, door);
            seat = b;
            bool s1, s2, s3, s4;
            if (!c2->GetSeatProperty(seat, s1, s2, s3, s4))
                break;
            int id = GetAnimIDForUseCar(s1, s2, s3, s4, false, c2->seats.size() && !c2->seats[0].unknown_20);
            S_vector p, d;
            if (!c2->GetPlacementFromTCK(seat, id, p, d))
                break;
            if (this == act_mission->game->player) {
                C_traffic_generator::PlayerInTaxi(c2);
                mafiaIndicators.ConsoleAddText(mafiaCarDatabase.GetCarName(mafiaCarDatabase.GetIdxByModel(
                    c2->model_name)), 0xffffff);
            }
            next_car_action = c != 0;
            C_traffic_generator::VyfunOdTeMasiny(c2);
            anim_machine.DelExtendedAnim(4.0f);
            Do_Aimed(false);
            PoseNullRotate();
            PoseInterpolate(1000);
            DeactivateColls(false);
            fire_held = false;
            using_car = c2;
            last_car = c2;
            pos = p;
            dir = d;
            frame->SetPos(p);
            frame->SetDir(dir, 0.0f);
            frame->Update();
            next_anim_id = id;
            PersonAnim(0.0f, true, false);
            if (door) {
                do_snd_car_door = false;
                ShiftAnimation(NOTE_SKIP_DOOR);
                do_snd_car_door = true;
            }
            anim_machine.SetLandHeight(_get_delta_nodyn_y_(pos), true);
            c2->SetOwner(this, seat, properties[HUMAN_PROPERTY_MASS]);
            c2->LockVehicle(true);
            c2->SetEnterProtect1(seat, false);
            vehicle_action = HUMAN_VEHICLE_CAR_IN;
            if (act_mission->game->initialized) {
                act_mission->game->DelFrameForShadow(frame, SHADOW_TYPE_0);
                shadow = 0;
            }
            if (act_mission->game->player == this) {
                c2->unknown_65 = false;
                mafiaIndicators.show_flags &= ~INDF_CROSSHAIR;
                act_mission->game->camera.SetCar(c2);
                act_mission->game->camera.LezeDoToyoty(true);
                mafiaIndicators.RadarAddCar(c2, 0xffffffff);
                ((C_player *)this)->fade_alpha = 1.0f;
                ((C_player *)this)->SetAlpha(1.0f);
                mafiaLoadSave.CarIsStolen(c2);
                act_mission->game->shvestky.DelDoRiciActor(c2);
            }
            inventory.SetInCar(true, NULL);
            if ((weapon_kind == WEAPON_KIND_RIFLE || weapon_kind == WEAPON_KIND_SMG) && !c2->IsSeatLeft(seat))
                ChangeWeaponModel();
            if (!seat && weapon_kind == WEAPON_KIND_PISTOL && c2->IsSeatLeft(0))
                ChangeWeaponModel();
            if (weapon_model)
                weapon_model->SetOn(false);
            if (weapon_model2)
                weapon_model2->SetOn(false);
            if (!seat) {
                if (act_mission->game->player == this)
                    c2->auto_gearing = !((C_player *)this)->unknown_adb;
                else
                    c2->auto_gearing = true;
                if (act_mission->game->shvestky.active)
                    act_mission->game->shvestky.DriveCar(this, c2);
            }
        } else {
            // Out of the seat he is in
            if (this == act_mission->game->player)
                C_traffic_generator::PlayerInTaxi(NULL);
            if (!car || car->type != ACTOR_CAR || seat == -1)
                break;
            if (TozNaselJsemTenNejvyssiMost())
                break;
            if (act_mission->game->player == this && aimed) {
                Do_Aimed(false);
                ((C_player *)this)->unknown_af5 = false;
                act_mission->game->camera.EnableFreeLook(false, false);
            }
            if (ZasrateLezeniZAuta_ve2rano())
                break;
            if (Can_DropOutFromCar(-1)) {
                bool s1, s2, s3, s4;
                if (!vehicle->GetSeatProperty(seat, s1, s2, s3, s4))
                    break;
                action_state = HUMAN_ACTION_STAND;
                if (!seat) {
                    vehicle->SetEngineOn(false, false);
                    if (act_mission->game->shvestky.active)
                        act_mission->game->shvestky.DriveCar(this, NULL);
                }
                vehicle->SetOwner(NULL, seat, 0.0f);
                vehicle->unknown_2108 = 2000;
                vehicle->SetEnterProtect1(seat, false);
                car = NULL;
                AIManager.police.PanoveKokotPremistujePrdel(this, vehicle, false);
                using_car = vehicle;
                S_matrix m = vehicle->frame->GetMatrix();
                pos = frame->GetPos();
                dir = frame->GetDir();
                pos *= m;
                m(3).x = m(3).y = m(3).z = 0.0f;
                dir *= m;
                frame->SetPos(pos);
                frame->SetDir(dir, 0.0f);
                frame->Update();
                frame->LinkTo(act_mission->scene->primary_sector, 0);
                act_mission->scene->SetFrameSectorPos(frame, base_frame->GetWorldPos());
                next_anim_id = GetAnimIDForUseCar(s1, s2, s3, s4, true,
                    vehicle->seats.size() && !vehicle->seats[0].unknown_20);
                int index = GetAnimIndex(next_anim_id);
                PersonAnim(0.0f, true, false);
                vehicle_action = HUMAN_VEHICLE_CAR_OUT;
                if (act_mission->game->initialized) {
                    ActivateColls(HUMAN_COLLS_UPPER);
                    lower_coll_off = true;
                }
                if (index >= 0) {
                    // The landing's height by the track's end
                    char name[128];
                    strcpy(name, _anim_info_[index].name);
                    char *ext = strrchr(name, '.');
                    if (ext)
                        *ext = 0;
                    strcat(name, ".tck");
                    S_vector end = pos;
                    C_animations_machine_de_luxe::GetEndPointOfTCK(name, end);
                    end *= frame->GetMatrix();
                    anim_machine.SetLandHeight(_get_delta_nodyn_y_(end));
                }
                if (act_mission->game->player == this) {
                    mafiaIndicators.show_flags &= ~(INDF_RADAR | INDF_TACHOMETR | INDF_SPEED_LIMITER | INDF_CAR_DAMAGE |
                        INDF_MAP);
                    act_mission->game->camera.LezeDoToyoty(true);
                }
                HideLegs((I3D_model *)frame, false);
            } else {
                // Over to the next seat, if it is free and he can get out there, else thrown out
                C_car *c3 = car;
                int other = seat & 1 ? seat - 1 : seat + 1;
                if (c3->GetNumSeats() > other && !c3->GetOwner(other) && Can_DropOutFromCar(other)) {
                    Do_ClimbInCarLR();
                    next_car_action = HUMAN_CAR_ACTION_GET_OUT;
                    return;
                }
                ZasrateVybliknutiZAuta_ve3rano();
                return;
            }
        }
        break;
    }
    case ACTOR_RAILWAY: {
        C_railway *rail = (C_railway *)act;
        if (act_mission->game->player == this)
            ((C_player *)this)->SetSniperMode(false);
        throw_swung = false;
        if (this == act_mission->game->player)
            mafiaIndicators.show_flags &= ~INDF_THIEF;
        S_vector p;
        if (how == HUMAN_USE_IN) {
            // In through the nearest entry
            S_vector d;
            if (!rail->GetEntry(this, &p, &d, (uint *)&wagon))
                break;
            PoseNullRotate();
            anim_machine.DelExtendedAnim(4.0f);
            aimed = false;
            rail->Lock();
            using_car = (C_car *)rail;
            pos = p;
            d.y = 0.0f;
            d.Normalize();
            dir = d;
            frame->SetPos(p);
            frame->SetDir(dir, 0.0f);
            frame->Update();
            if (act_mission->game->initialized) {
                act_mission->game->DelFrameForShadow(frame, SHADOW_TYPE_0);
                shadow = 0;
            }
            if (rail->unknown_1d8) {
                next_anim_id = ANIM_WALK1;
                last_velocity = frame->GetDir();
                last_velocity.Magnitude();
                last_velocity *= 1.5f;
                action_time = 800;
                action_state = HUMAN_ACTION_WAIT;
            } else
                next_anim_id = ANIM_SALINA_NASTUP;
            PersonAnim(0.0f, true, false);
            vehicle_action = HUMAN_VEHICLE_RAILWAY_IN;
        } else {
            // Out through the nearest entry
            S_vector d;
            if (!rail->GetEntry(this, &p, &d, (uint *)&wagon))
                break;
            rail->Lock();
            pos = p;
            d.y = 0.0f;
            d.Normalize();
            dir = d;
            frame->SetPos(p);
            frame->SetDir(dir, 0.0f);
            frame->Update();
            if (rail->unknown_1d8) {
                next_anim_id = ANIM_WALK1;
                last_velocity = frame->GetDir();
                last_velocity.Magnitude();
                last_velocity *= 1.5f;
                action_time = 800;
                action_state = HUMAN_ACTION_WAIT;
            } else
                next_anim_id = ANIM_SALINA_VYSTUP;
            PersonAnim(0.0f, true, false);
            vehicle_action = HUMAN_VEHICLE_RAILWAY_OUT;
        }
        break;
    }
    }
}


// A shot or a strike started (b) or let go (PS2 name; the parameters' names are ours): the guns aim and then shoot
// (Shooting), the thrown weapons swing and throw on the release with a force by how long they were held, the fists and
// the melee weapons strike, the strikes of a series by the time between them; dir is the target, NULL to keep the last.
// False while the human carries a box or a body.
// FUNCTION: GAME 0x583590
bool C_human::Do_Shoot(bool b, S_vector *dir)
{
    if (box || dead_body)
        return false;
    if (b) {
        if (changing_weapon)
            ChangeWeaponModel();
        if (loading_shells)
            anim_machine.DelExtendedAnim(4.0f);
        loading_shells = false;
        switch (weapon_kind) {
        case WEAPON_KIND_PISTOL:
        case WEAPON_KIND_RIFLE:
        case WEAPON_KIND_SMG:
            if (dir)
                throw_dir = *dir;
            fire_held = true;
            if (!aimed) {
                aimed = true;
                fire_action = _fire_action_aimed_;
                fire_step = 0;
                if (this == act_mission->GetGame()->player)
                    ((C_player *)this)->aim_time = 3000;
            } else
                Shooting();
            break;
        case WEAPON_KIND_THROWN:
            if (!car || car->type != ACTOR_CAR || seat == -1) {
                if (dir)
                    throw_dir = *dir;
                if (!throw_swung) {
                    throw_time = act_mission->GetGame()->game_time;
                    I3D_animation_set *as = NULL;
                    // STRING: GAME 0x636c28
                    anim_cache.Create("hod vrchem mix75.i3d", &as, NULL, NULL);
                    if (as) {
                        anim_machine.SetExtendedAnim(as, .75f, 4.0f, false);
                        as->Release();
                    }
                }
                throw_swung = true;
            }
            break;
        case WEAPON_KIND_FISTS:
        case WEAPON_KIND_KNUCKLEDUSTER:
        case WEAPON_KIND_KNIFE:
        case WEAPON_KIND_BAT:
            if ((!car || car->type != ACTOR_CAR || seat == -1) && strike_anim_time < 0) {
                aimed = true;
                if (this == act_mission->GetGame()->player)
                    ((C_player *)this)->aim_time = 3000;
                if (!fire_held) {
                    uint dt = act_mission->GetGame()->game_time - strike_time;
                    strike_time = act_mission->GetGame()->game_time;
                    if (dt < 1200)
                        strike_index++;
                    else
                        strike_index = 0;
                    if (strike_index > 3)
                        strike_index = 0;
                }
                fire_held = true;
                if (act_mission->GetGame()->game_time - strike_time > 200) {
                    if (weapon_kind == WEAPON_KIND_BAT && !backswing) {
                        backswing = true;
                        I3D_animation_set *as = NULL;
                        // STRING: GAME 0x636c10
                        anim_cache.Create("boj basb naprah HPT.i3d", &as, NULL, NULL);
                        if (as) {
                            anim_machine.SetExtendedAnim(as, .75f, 4.0f, false);
                            as->Release();
                        }
                    }
                } else
                    backswing = false;
            }
            break;
        }
    } else {
        switch (weapon_kind) {
        case WEAPON_KIND_PISTOL:
        case WEAPON_KIND_RIFLE:
        case WEAPON_KIND_SMG:
            fire_held = false;
            break;
        case WEAPON_KIND_THROWN:
            if (throw_swung) {
                throw_swung = false;
                if (this == act_mission->GetGame()->player)
                    mafiaIndicators.show_flags &= ~INDF_THIEF;
                int dt = act_mission->GetGame()->game_time - throw_time;
                if (dt > 400) {
                    if (dt > 2000)
                        dt = 2000;
                    throw_dir.Normalize();
                    throw_dir *= (dt * .015f) * properties[HUMAN_PROPERTY_STRENGTH];
                    anim_machine.DelExtendedAnim(4.0f);
                    next_anim_id = ANIM_HOD_VRCHEM_HOD;
                    PersonAnim(0.0f, true, false);
                    last_fire_time = act_mission->GetGame()->game_time;
                } else
                    anim_machine.DelExtendedAnim(4.0f);
            }
            break;
        case WEAPON_KIND_FISTS:
        case WEAPON_KIND_KNUCKLEDUSTER:
            if (fire_held) {
                uint dt = act_mission->GetGame()->game_time - strike_time;
                strike_time = act_mission->GetGame()->game_time;
                if (dt <= 500 && (strike_index != 3 || action_state != HUMAN_ACTION_STAND)) {
                    if (action_state == HUMAN_ACTION_STAND) {
                        velocity.Zero();
                        velocity_ramp = 0.0f;
                        next_anim_id = strike_index + ANIM_BOJ_BOX_L_DIRECT;
                        anim_id = 0;
                        SetWaitAnim();
                        anim_machine.DelExtendedAnim(4.0f);
                    } else if (!anim_machine.IsBlocking())
                        SetFreeBoxingAnim(strike_index + ANIM_BOJ_HPT_BOX_L_DIRECT);
                } else {
                    velocity.Zero();
                    velocity_ramp = 0.0f;
                    strike_index = -1;
                    anim_machine.DelExtendedAnim(4.0f);
                    C_human *human = NULL;
                    if (!rnd(2)) {
                        human = Can_SpecialStroke();
                        if (human) {
                            if (!rnd(2)) {
                                if (!Do_Castrate(human))
                                    human = NULL;
                            } else if (!Do_HardHead(human))
                                human = NULL;
                            PersonAnim(0.0f, true, false);
                        }
                    }
                    if (!human) {
                        next_anim_id = ANIM_BOJ_BOX_L_HAK_SPODEK;
                        anim_id = 0;
                        SetWaitAnim();
                    }
                }
            }
            fire_held = false;
            break;
        case WEAPON_KIND_KNIFE:
            if (fire_held) {
                uint dt = act_mission->GetGame()->game_time - strike_time;
                strike_time = act_mission->GetGame()->game_time;
                if (dt <= 500 && strike_index != 3) {
                    if (action_state == HUMAN_ACTION_STAND) {
                        velocity.Zero();
                        velocity_ramp = 0.0f;
                        next_anim_id = strike_index + ANIM_BOJ_NUZ_BOD;
                        anim_id = 0;
                        SetWaitAnim();
                        anim_machine.DelExtendedAnim(4.0f);
                    } else if (!anim_machine.IsBlocking())
                        SetFreeBoxingAnim(strike_index + ANIM_BOJ_HPT_NUZ_BOD);
                } else {
                    velocity.Zero();
                    velocity_ramp = 0.0f;
                    strike_index = -1;
                    next_anim_id = ANIM_BOJ_NUZ_KOMBO;
                    anim_id = 0;
                    SetWaitAnim();
                    anim_machine.DelExtendedAnim(4.0f);
                }
            }
            fire_held = false;
            break;
        case WEAPON_KIND_BAT:
            if (fire_held) {
                uint dt = act_mission->GetGame()->game_time - strike_time;
                strike_time = act_mission->GetGame()->game_time;
                if (dt <= 500 && strike_index != 3) {
                    if (action_state == HUMAN_ACTION_STAND) {
                        velocity.Zero();
                        velocity_ramp = 0.0f;
                        next_anim_id = strike_index + ANIM_BOJ_BASB_Z_LH;
                        anim_id = 0;
                        SetWaitAnim();
                        anim_machine.DelExtendedAnim(4.0f);
                    } else if (!anim_machine.IsBlocking())
                        SetFreeBoxingAnim(strike_index + ANIM_BOJ_HPT_BASB_LH);
                } else {
                    velocity.Zero();
                    velocity_ramp = 0.0f;
                    strike_index = -1;
                    next_anim_id = ANIM_BOJ_BASB_Z_RH;
                    anim_id = 0;
                    SetWaitAnim();
                    anim_machine.DelExtendedAnim(4.0f);
                }
            }
            fire_held = false;
            break;
        }
    }
    return true;
}


// FUNCTION: GAME 0x583f40
bool C_human::Do_ThrowGrenade(S_vector &dir)
{
    if (!IsReady())
        return false;
    uint id = inventory.GetItemInRightHand(NULL);
    if (id != GAMEITEM_MOLOTOV && id != GAMEITEM_GRENADE)
        return false;
    throw_dir = dir;
    throw_swung = false;
    if (this == act_mission->GetGame()->player)
        mafiaIndicators.show_flags &= ~INDF_THIEF;
    anim_machine.DelExtendedAnim(4.0f);
    next_anim_id = ANIM_HOD_VRCHEM_HOD;
    PersonAnim(0.0f, true, false);
    last_fire_time = act_mission->GetGame()->game_time;
    return true;
}


// The human a special stroke can take (PS2 name; PS2's takes a bool): an active entity or player standing ready
// within a metre in front of this one, turned away from it, whose energy is above ten times this one's strength;
// NULL for none
// FUNCTION: GAME 0x584010
C_human *C_human::Can_SpecialStroke()
{
    for (int i = C_actor::global_actors.size(); i--;)
    {
        if (!C_actor::global_actors[i]->act_state &&
            (C_actor::global_actors[i]->type == ACTOR_ENTITY || C_actor::global_actors[i]->type == ACTOR_PLAYER))
        {
            C_human *human = (C_human *)C_actor::global_actors[i];
            if (human != this && !human->crouching && human->IsReady() &&
                properties[HUMAN_PROPERTY_STRENGTH] * 10.0f < human->properties[HUMAN_PROPERTY_ENERGY])
            {
                S_vector d = human->frame->GetWorldPos();
                d -= frame->GetWorldPos();
                if (d.Square() < 1.0f && d.AngleTo(frame->GetWorldDir()) < .3f &&
                    d.AngleTo(human->frame->GetWorldDir()) > 2.85f)
                    return human;
            }
        }
    }
    return NULL;
}


// FUNCTION: GAME 0x584480
bool C_human::Do_Castrate(C_human *human)
{
    if (!human)
        return false;
    if (!human->IsReady())
        return false;
    C_game *game = act_mission->GetGame();
    if (game->cheats & GAMEF_INVULNERABLE)
    {
        if (game->player == human)
            return false;
        if (human->human_type == HUMAN_TYPE_FRIEND)
            return false;
    }
    next_anim_id = ANIM_BOJ_BOX_ROZKROK;
    PersonAnim(0.0f, true, false);
    human->next_anim_id = ANIM_BOJ_ZASAH_ROZKROK_SM;
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
    if (game->cheats & GAMEF_INVULNERABLE)
    {
        if (game->player == human)
            return false;
        if (human->human_type == HUMAN_TYPE_FRIEND)
            return false;
    }
    next_anim_id = ANIM_BOJ_BOX_HLAVICKA;
    PersonAnim(0.0f, true, false);
    human->next_anim_id = ANIM_BOJ_ZASAH_HLAVICKA;
    human->PersonAnim(0.0f, true, false);
    human->properties[HUMAN_PROPERTY_ENERGY] -= properties[HUMAN_PROPERTY_STRENGTH] * 10.0f;
    return true;
}


// The strike's animation of the id as the hands' extended animation, for its length (PS2 name; inline on the PC, where
// Do_Shoot has it expanded)
inline void C_human::SetFreeBoxingAnim(uint id)
{
    int index = GetAnimIndex(id);
    if (index > -1) {
        I3D_animation_set *as = NULL;
        anim_cache.Create(_anim_info_[index].name, &as, NULL, NULL);
        if (as) {
            strike_anim_time = as->GetEndTime();
            anim_machine.SetExtendedAnim(as, 1.0f, 4.0f, true);
            anim_machine.unknown_10c = true;
            guarding = false;
            as->Release();
        }
    }
}


// The shot from a car's seat, to its side only (Shooting's; inline, as the starved nested calls of its vector
// operators show: the name is ours)
inline void C_human::ShootingInCar()
{
    if (car_stance_time < 0 && car_stance == HUMAN_CARSTANCE_AIMED) {
        if (fire_target)
            throw_dir = fire_target->GetWorldPos();
        S_vector d = throw_dir - shot_frame->GetWorldPos();
        S_vector2 d2(d.x, d.z);
        S_vector dir = frame->GetWorldDir();
        S_vector2 look2(dir.x, dir.z);
        float angle = AngleTo2(look2, d2);
        if (!car->IsSeatLeft(seat))
            angle *= -1.0f;
        if (angle < 0.0f)
            return;
        if (weapon_displaced)
            RepairWeaponPosition();
        if (act_mission->GetGame()->shvestky.GetActive())
            act_mission->GetGame()->shvestky.Firing(this);
        last_fire_time = act_mission->GetGame()->game_time;
        S_item_def *weapon = inventory.GetWeaponInRightHand();
        S_vector n = d;
        n.Normalize();
        S_vector up(n.z, n.y, -n.x);
        S_vector side = n.Cross(up);
        up = side.Cross(n);
        float a = rnd() * (PI * 2.0f);
        float r;
        if (type == ACTOR_PLAYER)
            r = rnd() * (1.0f - weapon->accuracy * properties[HUMAN_PROPERTY_SHOOTING]);
        else
            r = rnd() * (1.0f - weapon->accuracy * properties[HUMAN_PROPERTY_SHOOTING] * .95f);
        float c = (float)cos(a);
        float s = (float)sin(a);
        dir = n + side * s * r + up * c * r;
        dir.Normalize();
        dir *= weapon->range;
        act_mission->GetGame()->NewShoot(this, shot_frame->GetWorldPos(), dir, weapon->power, muzzle,
            shot_frame, 1);
        shots += 1.0f;
        velocity_ramp *= .3f;
        if (act_mission->GetGame()->player == this)
            ((C_player *)this)->ShootDeAimed(weapon);
        fire_action = weapon->fire;
        fire_step = 0;
        fire_time = 0;
        inventory.Vystrel();
        if (inventory.GetItemInRightHand(NULL) == GAMEITEM_SHOTGUN && !inventory.GetAmmo(NULL) &&
            inventory.MuzeNabitJeden())
            loading_shells = true;
        if (act_mission->GetGame()->player == this && ff_manager.HasEffects(true)) {
            ff_manager.Start(ff_shot);
            ff_manager.SetGain(ff_shot, mafiaLoadSave.settings.ff_player_movement);
        }
        if (weapon_sound) {
            weapon_sound->SetPos(S_vector(0.0f, 1.0f, 0.0f));
            weapon_sound->Update();
            if (weapon_sound->IsPlaying(false))
                weapon_sound->SetCurrTime(0);
            else
                weapon_sound->SetOn(true, true);
        }
    }
}


// The shot of the weapon in the right hand once its fire's last steps are over (PS2 name): without ammunition the
// player reloads; from a car's seat only to the seat's side, else unless the target is behind the human (or
// note_fire), the shot spread about the line to throw_dir by the weapon's accuracy and the human's shooting, the
// pellets of a close range weapon, the weapon's fire steps started, the round taken, the sound, the force feedback and
// the hands' fire animation
// FUNCTION: GAME 0x584620
void C_human::Shooting()
{
    if (fire_action[fire_step].action != FIRE_ACTION_END)
        return;
    if (changing_weapon)
        ChangeWeaponModel();
    if (!inventory.GetAmmo(NULL)) {
        if (type == ACTOR_PLAYER) {
            int spare;
            inventory.GetAmmo(&spare);
            if (spare)
                Do_Reload();
        }
        return;
    }
    if (!shot_frame)
        return;
    if (car && car->type == ACTOR_CAR && seat != -1) {
        // From the car's seat, to its side only
        ShootingInCar();
    } else {
        if (weapon_displaced)
            RepairWeaponPosition();
        idle_time = rnd(5000) + 5000;
        breath_time = -1;
        uint anim = anim_id & ANIM_BASE;
        if (anim >= ANIM_BREATH01B && anim <= ANIM_BREATH01D) {
            fire_action = _fire_action_aimed_;
            fire_step = 0;
            return;
        }
        // The cheat's Magnum fires grenades
        if (inventory.GetItemInRightHand(NULL) == GAMEITEM_SW_MAGNUM &&
            (bool)(act_mission->GetGame()->cheats & GAMEF_GRENADE_GUN)) {
            S_vector d = throw_dir - shot_frame->GetWorldPos();
            d.Normalize();
            S_vector pos = d + shot_frame->GetWorldPos();
            d *= 50.0f;
            act_mission->GetGame()->NewGrenade(NEWGRENADE_SHOT, pos, d, true, NULL);
            fire_action = inventory.GetWeaponInRightHand()->fire;
            fire_step = 0;
            fire_time = 0;
            return;
        }
        if (fire_target)
            throw_dir = fire_target->GetWorldPos();
        S_vector d = throw_dir - shot_frame->GetWorldPos();
        // Not behind the human, unless note_fire
        if (d.AngleTo(frame->GetWorldDir()) > PI / 2 && !note_fire)
            return;
        if (act_mission->GetGame()->shvestky.GetActive())
            act_mission->GetGame()->shvestky.Firing(this);
        last_fire_time = act_mission->GetGame()->game_time;
        S_item_def *weapon = inventory.GetWeaponInRightHand();
        S_vector n = d;
        n.Normalize();
        S_vector up(n.z, n.y, -n.x);
        S_vector side = n.Cross(up);
        up = side.Cross(n);
        float a = rnd() * (PI * 2.0f);
        float acc = weapon->accuracy;
        if (act_mission->GetGame()->player == this)
            acc = 1.0f - ((C_player *)this)->recoil * (1.0f - acc);
        float r = rnd() * (1.0f - acc * properties[HUMAN_PROPERTY_SHOOTING]);
        float c = (float)cos(a);
        float s = (float)sin(a);
        S_vector shot_dir = n + side * s * r + up * c * r;
        shot_dir.Normalize();
        shot_dir *= weapon->range;
        // The player in the railway's vehicle shoots from the camera
        I3D_frame *from;
        if (act_mission->GetGame()->player == this && (sniper_mode || car && car->type == ACTOR_RAILWAY))
            from = act_mission->GetScene()->GetActiveCamera();
        else
            from = shot_frame;
        S_vector from_pos = from->GetWorldPos();
        // In the railway's vehicle the shot hits its wagon first
        C_railway *rail = (C_railway *)car;
        int shots = (weapon->flags & ITEMF_CLOSE_RANGE) ? 10 : 1;
        I3D_frame *wagon_frm;
        if (!rail || rail->type != ACTOR_RAILWAY || !(wagon_frm = rail->GetVagon(wagon)) ||
            !rail->Hit(HIT_SHOT, shot_dir, from_pos, -shot_dir, weapon->power, this, 0, wagon_frm)) {
            act_mission->GetGame()->NewShoot(this, from_pos, shot_dir, weapon->power, muzzle, shot_frame,
                shots);
            if (!car && act_mission->options.game_effects >= OPTION_HIGH)
                act_mission->GetGame()->AddShootLight(from_pos, 7.0f, 50, 0);
        }
        shots += shots;
        velocity_ramp *= .3f;
        if (act_mission->GetGame()->player == this)
            ((C_player *)this)->ShootDeAimed(weapon);
        fire_action = weapon->fire;
        fire_step = 0;
        fire_time = 0;
        inventory.Vystrel();
        reload_forced = false;
        if (inventory.GetItemInRightHand(NULL) == GAMEITEM_SHOTGUN) {
            if (inventory.GetAmmo(NULL))
                pump_time = 500;
            else if (inventory.MuzeNabitJeden())
                loading_shells = true;
        }
        if (inventory.GetItemInRightHand(NULL) == GAMEITEM_COLT_1911) {
            if (inventory.GetAmmo(NULL))
                StartWeaponEffect(WEAPON_EFFECT_SHOT);
            else
                StartWeaponEffect(WEAPON_EFFECT_LAST_SHOT);
        }
        if (act_mission->GetGame()->player == this && ff_manager.human_effects) {
            ff_manager.Start(ff_shot);
            ff_manager.SetGain(ff_shot, mafiaLoadSave.settings.ff_player_movement);
        }
        if (weapon_sound) {
            if (car && car->type == ACTOR_CAR && seat == -1)
                weapon_sound->SetPos(S_vector(0.0f, 1.2f, 2.0f));
            else if (crouching)
                weapon_sound->SetPos(S_vector(0.0f, .6f, 1.0f));
            else
                weapon_sound->SetPos(S_vector(0.0f, 1.3f, 1.0f));
            weapon_sound->Update();
            weapon_sound->SetOn(false, true);
            weapon_sound->SetOn(true, true);
        }
        S_GameItem *item;
        uint id = inventory.GetItemInRightHand(&item);
        if (item && !item->ammo_loaded && item->ammo_spare)
            reload_effects = true;
        if (crouching && id == GAMEITEM_THOMPSON)
            return;
        // The hands' fire animation, sideways in the strafes
        int gun = weapon->fire_anim;
        uint walk = anim_id & ANIM_BASE;
        bool strafe = walk == ANIM_STRAFL1 || walk == ANIM_STRAFR1;
        char name[56];
        name[0] = 0;
        if (crouching) {
            if ((gun == GUN_ANIM_RIFLE || gun == GUN_ANIM_THOMPSON || gun == GUN_ANIM_SAWED_OFF) && strafe)
                // STRING: GAME 0x636c8c
                sprintf(name, "gun0%i drep Fire Straf.i3d", gun);
            else
                // STRING: GAME 0x636c74
                sprintf(name, "gun0%i drep Fire.i3d", gun);
        } else {
            if ((gun == GUN_ANIM_RIFLE || gun == GUN_ANIM_THOMPSON || gun == GUN_ANIM_SAWED_OFF ||
                gun == GUN_ANIM_SHOTGUN) && strafe)
                // STRING: GAME 0x636c58
                sprintf(name, "gun0%i stoj Fire Straf.i3d", gun);
            else
                // STRING: GAME 0x636c40
                sprintf(name, "gun0%i stoj Fire.i3d", gun);
        }
        if (strlen(name)) {
            I3D_animation_set *as = NULL;
            anim_cache.Create(name, &as, NULL, NULL);
            if (as) {
                anim_machine.SetExtendedAnim(as, .25f, 4.0f, true);
                as->Release();
            }
        }
    }
}


// FUNCTION: GAME 0x585b40
void C_human::Do_Reload()
{
    if (box || dead_body)
        return;
    if (fire_action[fire_step].action != FIRE_ACTION_END)
        return;
    if (weapon_kind <= WEAPON_KIND_FISTS || weapon_kind > WEAPON_KIND_SMG)
        return;
    int spare;
    inventory.GetAmmo(&spare);
    if (!spare)
        return;
    fire_action = inventory.GetWeaponInRightHand()->fire;
    fire_time = 0;
    fire_step = 0;
    while (fire_action[fire_step].action != FIRE_ACTION_END && fire_action[fire_step].action != FIRE_ACTION_RELOAD)
        fire_step++;
    // From the step before the reload's
    if (fire_action[fire_step].action == FIRE_ACTION_RELOAD && fire_step > 0)
    {
        fire_step--;
        fire_time = fire_action[fire_step].time;
    }
    reload_forced = true;
    reload_effects = true;
    // A shell at a time: the left hand lets go of the pump shotgun
    if (inventory.GetItemInRightHand(NULL) == GAMEITEM_SHOTGUN && inventory.MuzeNabitJeden())
    {
        loading_shells = true;
        if (left_hand_target)
        {
            left_hand_target->DeleteFromTarget(weapon_model);
            weapon_displaced = true;
        }
    }
}


// FUNCTION: GAME 0x585c60
void C_human::Do_Holster()
{
    if (box || dead_body)
    {
        Do_FuckingBoxDrop();
        return;
    }
    std::vector<S_GameItem> dropped;
    uint id = inventory.GetItemInRightHand(NULL);
    inventory.Holster(&dropped, false, act_mission->GetGame()->player == this);
    if (id != inventory.GetItemInRightHand(NULL))
    {
        if (act_mission->skip_frames > 0)
        {
            act_mission->skip_frames--;
            ChangeWeaponModel();
        }
        else
            Do_ChangeWeapon(HUMAN_WEAPON_ON_OFF, dropped.size() > 0);
    }
    if (dropped.size())
        DropOutItems(dropped);
}


// FUNCTION: GAME 0x585d90
void C_human::Do_WeaponDrop()
{
    if (box)
    {
        Do_FuckingBoxDrop();
        return;
    }
    if (dead_body)
    {
        Do_DeadBodyDrop();
        return;
    }
    std::vector<S_GameItem> dropped;
    S_GameItem *item;
    inventory.GetItemInRightHand(&item);
    if (item)
    {
        dropped.push_back(*item);
        inventory.Remove(item, NULL);
        if (dropped.size())
        {
            if (DropOutItems(dropped))
                Do_ChangeWeapon(HUMAN_WEAPON_THROW_AWAY, false);
        }
    }
}


// FUNCTION: GAME 0x585e80
int C_human::Do_PlayAnim(const char *anim, bool simple, bool unknown)
{
    idle_time = -1;
    breath_time = -1;
    velocity.Zero();
    velocity_ramp = 0.0f;
    test_ground = false;
    fall = 0.0f;
    throw_swung = false;
    if (this == act_mission->GetGame()->player)
        mafiaIndicators.show_flags &= ~INDF_THIEF;
    anim_machine.SetAnim(anim, false, false, simple, 4.0f, 0.0f, true, unknown, NULL, true, (uint)-1);
    if (anim_machine.track != -1)
        action_state = HUMAN_ACTION_TRACK;
    uint time, length;
    anim_machine.GetActiveAnimTime(time, length);
    return length;
}


// FUNCTION: GAME 0x585f40
bool C_human::GetBoundPoints2D(std::vector<S_vector2> &points, I3D_frame *frm)
{
    S_vector pos = frame->GetWorldPos();
    points.push_back(S_vector2(pos.x + .7f, pos.z));
    points.push_back(S_vector2(pos.x - .7f, pos.z));
    points.push_back(S_vector2(pos.x, pos.z + .7f));
    points.push_back(S_vector2(pos.x, pos.z - .7f));
    return true;
}


// A jump (PS2 name): from a run forward, from a stance or a walk up, from a side step aside; a wall within reach is
// climbed instead (Do_Climb). The player's joystick shakes in the jump's direction.
// FUNCTION: GAME 0x586190
void C_human::Do_Jump()
{
    if (box || dead_body || car)
        return;
    fire_held = false;
    uint id = anim_id & ANIM_BASE;
    if (id == ANIM_RUN1)
    {
        if (!crouching && !Do_Climb())
        {
            uint time, length;
            anim_machine.GetActiveAnimTime(time, length);
            anim_left = time < length / 2;
            next_anim_id = ANIM_JUMPSTAT;
            PersonAnim(0.0f, true, false);
            action_state = HUMAN_ACTION_JUMP;
            velocity.y = 6.0f;
            if (act_mission->GetGame()->player == this && ff_manager.human_effects)
            {
                ff_manager.SetDirection(ff_jump, 0.0f);
                ff_manager.Start(ff_jump);
                ff_manager.SetGain(ff_jump, mafiaLoadSave.settings.ff_player_movement);
            }
        }
    }
    else if (id >= ANIM_BREATH01A && id <= ANIM_BREATH01D || id == ANIM_WALK1 || id == ANIM_LEFT1 || id == ANIM_RIGHT1)
    {
        if (crouching)
            return;
        if (!Do_Climb())
        {
            next_anim_id = ANIM_JUMP1;
            if (act_mission->GetGame()->player == this && ff_manager.human_effects)
            {
                ff_manager.SetDirection(ff_jump, 0.0f);
                ff_manager.Start(ff_jump);
                ff_manager.SetGain(ff_jump, mafiaLoadSave.settings.ff_player_movement);
            }
        }
    }
    else if (id == ANIM_STRAFL1 || id == ANIM_STRAFRL1)
    {
        if (weapon_kind == WEAPON_KIND_RIFLE || weapon_kind == WEAPON_KIND_SMG)
            next_anim_id = ANIM_JUMPL3;
        else
            next_anim_id = ANIM_JUMPL1;
        if (act_mission->GetGame()->player == this && ff_manager.human_effects)
        {
            ff_manager.SetDirection(ff_jump, PI / 2);
            ff_manager.Start(ff_jump);
            ff_manager.SetGain(ff_jump, mafiaLoadSave.settings.ff_player_movement);
        }
    }
    else if (id == ANIM_STRAFR1 || id == ANIM_STRAFRR1)
    {
        if (weapon_kind == WEAPON_KIND_RIFLE || weapon_kind == WEAPON_KIND_SMG)
            next_anim_id = ANIM_JUMPR3;
        else
            next_anim_id = ANIM_JUMPR1;
        if (act_mission->GetGame()->player == this && ff_manager.human_effects)
        {
            ff_manager.SetDirection(ff_jump, PI * 3 / 2);
            ff_manager.Start(ff_jump);
            ff_manager.SetGain(ff_jump, mafiaLoadSave.settings.ff_player_movement);
        }
    }
    if (anim_id != next_anim_id)
    {
        throw_swung = false;
        idle_time = -1;
        breath_time = -1;
        if (this == act_mission->GetGame()->player)
            mafiaIndicators.show_flags &= ~INDF_THIEF;
        anim_machine.DelExtendedAnim(4.0f);
        ResetWeaponEffects();
        reload_effects = false;
        PersonAnim(0.0f, true, false);
    }
}


// A wall within reach climbed (Do_Jump's): a horizontal line ahead at the knees, else at the waist, finds the wall,
// none ahead at the head; a line down from above the wall's top finds its top, flat enough and 0.3 to 1.1 m above the
// feet, with room for the human above him and above the top; he turns to the wall where there is room there too, and
// climbs with the low or the high animation (0x68 below 0.8 m, else 0x69)
// FUNCTION: GAME 0x586400
bool C_human::Do_Climb()
{
    if (box || dead_body)
        return false;
    S_vector from = pos + S_vector(0.0f, .3f, 0.0f);
    S_vector wall_dir(0.0f, 0.0f, 0.0f);
    S_vector d = dir * .7f;
    S_vector hit_pos, normal, delta;
    d.Normalize();
    gifCollision.DynSetNextCollOwnerMult(colls, num_colls, NULL);
    g_collision_header *coll = (g_collision_header *)gifCollision.TestLineH(from, d, &hit_pos, &normal,
        COLF_UNKNOWN_2, 0);
    gifCollision.DynSetNextCollOwner(NULL, NULL);
    if (!coll) {
        S_vector from2 = pos + S_vector(0.0f, .7f, 0.0f);
        gifCollision.DynSetNextCollOwnerMult(colls, num_colls, NULL);
        coll = (g_collision_header *)gifCollision.TestLineH(from2, d, &hit_pos, &normal, COLF_UNKNOWN_2, 0);
        gifCollision.DynSetNextCollOwner(NULL, NULL);
        if (!coll)
            return false;
        delta = hit_pos - from2;
        wall_dir = -normal;
        wall_dir.y = 0.0f;
        wall_dir.Normalize();
    } else {
        // Written in both branches, as PS2 has them (VC6 merges the copies from the delta's copy on)
        delta = hit_pos - from;
        wall_dir = -normal;
        wall_dir.y = 0.0f;
        wall_dir.Normalize();
    }
    // Not onto a human
    if (coll->flags & COLLHF_UNKNOWN_40) {
        C_actor *owner = coll->owner;
        if (owner && (owner->type == ACTOR_PLAYER || owner->type == ACTOR_ENTITY))
            return false;
    }
    from = pos + S_vector(0.0f, 1.2f, 0.0f);
    d = dir * .5f;
    d.Normalize();
    gifCollision.DynSetNextCollOwnerMult(colls, num_colls, NULL);
    int hit = gifCollision.TestLineH(from, d, &hit_pos, &normal, COLF_UNKNOWN_2, 0);
    gifCollision.DynSetNextCollOwner(NULL, NULL);
    if (hit)
        return false;
    d.Normalize();
    from = pos + S_vector(0.0f, 1.2f, 0.0f) + d * .8f;
    d = S_vector(0.0f, -1.2f, 0.0f);
    gifCollision.DynSetNextCollOwnerMult(colls, num_colls, NULL);
    hit = gifCollision.TestLineV(from, d, &hit_pos, &normal, COLF_UNKNOWN_2, 0);
    gifCollision.DynSetNextCollOwner(NULL, NULL);
    if (!hit)
        return false;
    if (normal.AngleTo(S_vector(0.0f, 1.0f, 0.0f)) > .5f)
        return false;
    float height = hit_pos.y - pos.y;
    if (height < .3f)
        return false;
    if (height > 1.1f)
        return false;
    from = pos;
    from.y += .5f;
    d = S_vector(0.0f, height + 1.0f, 0.0f);
    gifCollision.DynSetNextCollOwnerMult(colls, num_colls, NULL);
    hit = gifCollision.TestSphereB(from, d, .3f, NULL, NULL, COLF_UNKNOWN_2, 0);
    gifCollision.DynSetNextCollOwner(NULL, NULL);
    if (hit)
        return false;
    from = dir * .5f + pos;
    from.y += height + .5f;
    d = S_vector(0.0f, 1.0f, 0.0f);
    gifCollision.DynSetNextCollOwnerMult(colls, num_colls, NULL);
    hit = gifCollision.TestSphereB(from, d, .3f, NULL, NULL, COLF_UNKNOWN_2, 0);
    gifCollision.DynSetNextCollOwner(NULL, NULL);
    if (hit) {
        // No room ahead: turned to the wall, if there is room that way
        from = wall_dir * .5f + pos;
        from.y += height + .5f;
        gifCollision.DynSetNextCollOwnerMult(colls, num_colls, NULL);
        hit = gifCollision.TestSphereB(from, d, .3f, NULL, NULL, COLF_UNKNOWN_2, 0);
        gifCollision.DynSetNextCollOwner(NULL, NULL);
        if (hit)
            return false;
        dir = wall_dir;
        frame->SetDir(wall_dir, 0.0f);
        frame->Update();
    }
    float dist = delta.Magnitude();
    climb_reach = dist < .5f ? 1.0f : dist * 2.5f;
    if (height < .8f) {
        next_anim_id = ANIM_NA_BEDNU_06M;
        climb_scale = height / .6f;
    } else {
        next_anim_id = ANIM_NA_BEDNU_1M;
        climb_scale = height;
    }
    if (anim_id != next_anim_id) {
        throw_swung = false;
        idle_time = -1;
        breath_time = -1;
        if (this == act_mission->GetGame()->player)
            mafiaIndicators.show_flags &= ~INDF_THIEF;
        anim_machine.DelExtendedAnim(4.0f);
        ResetWeaponEffects();
        reload_effects = false;
        if (reload_sound)
            reload_sound->SetOn(false);
        if (pump_sound)
            pump_sound->SetOn(false);
        velocity_ramp = 0.0f;
        last_velocity.Zero();
        velocity.Zero();
        PersonAnim(0.0f, true, false);
    }
    return true;
}


// FUNCTION: GAME 0x587040
void C_human::Do_SideJump(const S_vector &pos2, const S_vector &dir2)
{
    if (box || dead_body)
        return;
    if (anim_id >= ANIM_USKOK_RIGHT_FRONT && anim_id <= ANIM_USKOK_FRONT_RIGHT)
        return;
    float angle = dir.AngleTo(dir2);
    if (angle < PI / 4)
        next_anim_id = ANIM_USKOK_LEFT_BACK;
    else if (angle > PI * 3 / 4)
        next_anim_id = ANIM_USKOK_RIGHT_FRONT;
    else
    {
        S_vector2 d(dir.x, dir.z);
        S_vector2 d2(dir2.x, dir2.z);
        if (AngleTo2(d, d2) > 0.0f)
            next_anim_id = ANIM_USKOK_BACK_RIGHT;
        else
            next_anim_id = ANIM_USKOK_FRONT_LEFT;
    }
    // The side of the line through pos2 the human is on (the line's direction takes dir's x and dir2's z, as the
    // original's code has it)
    S_vector2 hp(pos.x, pos.z);
    if (hp.DistanceToLine(S_vector2(pos2.x, pos2.z), S_vector2(dir.x, dir2.z)) < 0.0f)
        next_anim_id++;
}


// The human's model changed for the file's (PS2 name; the bool's name is ours: GameDone and Deactivate put the
// original back with it set, which leaves the player and the weapon's model alone). The first change keeps the
// original's name; the poses, the hands' targets, the shadow and the collisions go with the old model and are made
// again for the new one.
// FUNCTION: GAME 0x587190
void C_human::_intern_ChangeModel(char *name, bool restore)
{
    I3D_model *model = (I3D_model *)frame;
    if (!model)
        return;
    if (!orig_model)
    {
        const char *s = strrchr(model->GetFileName(), '\\');
        if (s)
            s++;
        else
            s = model->GetFileName();
        orig_model = new char[strlen(s) + 1];
        strcpy(orig_model, s);
    }
    if (!restore)
    {
        // The player stays the player
        C_human *player = act_mission->GetGame()->player;
        act_mission->GetGame()->InvalidateActor(this, true);
        if (player == this)
            act_mission->GetGame()->player = player;
    }
    act_mission->GetGame()->DelFrameForShadow(frame, SHADOW_TYPE_0);
    anim_machine.StopAnims();
    EraseDynColls();
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
    I3D_frame *notify = frame->FindChildFrame("notify", ENUMF_ALL);
    if (notify)
        notify->SetCallback(NULL, LS3D_CALLBACK_NOTE);
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
    if (frame && frame->GetType() == FRAME_MODEL)
        ((I3D_model *)frame)->StopAnimation(0);
    if (base_frame)
        act_mission->GetGame()->LightnessDel(base_frame);
    act_mission->GetGame()->InvalidateFrame(frame);
    if (model_cache.Open((I3D_model *)frame, name, 0, NULL, NULL, NULL) != I3D_OK)
        return;

    if (act_mission->GetGame()->player == this)
        ((C_player *)this)->_intern_ChangeModel();
    if (type == ACTOR_ENTITY)
        ((C_entity *)this)->InvalidateSelf();
    target_e = frame->FindChildFrame("targetE", ENUMF_TARGET);
    target_n = frame->FindChildFrame("targetN", ENUMF_TARGET);
    if (target_e)
        target_e->SetOn(false);
    if (target_n)
        target_n->SetOn(false);
    // No projector falls on the model's visuals
    for (int i = frame->NumChildren(); i--;)
    {
        if (frame->GetChild(i)->GetType() == FRAME_VISUAL)
        {
            I3D_visual *vis = (I3D_visual *)frame->GetChild(i);
            vis->vis_flags &= ~VISF_PROJECTOR;
        }
    }
    anim_machine.Init(this);
    if (alive)
    {
        anim_id = 0;
        next_anim_id = ANIM_BREATH01A;
    }
    pose_moving = false;
    pose_back1 = driver->CreatePose();
    ((I3D_model *)frame)->SetPose("back1", pose_back1);
    pose_back2 = driver->CreatePose();
    ((I3D_model *)frame)->SetPose("back2", pose_back2);
    pose_neck = driver->CreatePose();
    ((I3D_model *)frame)->SetPose("neck", pose_neck);
    look_frame = NULL;
    base_frame = frame->FindChildFrame("base", ENUMF_ALL);
    if (alive)
        CreateDynColls();
    if (frame)
        shot_skeleton.Init(frame);
    notify = frame->FindChildFrame("notify", ENUMF_ALL);
    if (notify)
        notify->SetCallback(human_notify_callback, LS3D_CALLBACK_NOTE);
    shadow = act_mission->GetGame()->AddFrameForShadow(frame, SHADOW_TYPE_0, SHADOW_TYPE_0, NULL, NULL);
    right_hand = frame->FindChildFrame("gun1", ENUMF_ALL);
    left_hand = frame->FindChildFrame("gun2", ENUMF_ALL);
    if (left_hand)
    {
        left_hand_target = (I3D_target *)driver->CreateFrame(FRAME_TARGET);
        left_hand_target->SetPos(S_vector(0.0f, 0.0f, 0.0f));
        left_hand_target->LinkTo(left_hand, 0);
        left_hand_target->Update();
    }
    else
        left_hand_target = NULL;
    if (right_hand)
    {
        right_hand_target = (I3D_target *)driver->CreateFrame(FRAME_TARGET);
        right_hand_target->SetPos(S_vector(0.0f, 0.0f, 0.0f));
        right_hand_target->LinkTo(right_hand, 0);
        right_hand_target->Update();
    }
    else
        right_hand_target = NULL;
    if (!right_hand)
        right_hand = frame->FindChildFrame("r_hand", ENUMF_ALL);
    if (!left_hand)
        left_hand = frame->FindChildFrame("l_hand", ENUMF_ALL);
    if (base_frame)
        act_mission->GetGame()->LightnessAdd(this, base_frame, 0, S_vector(0.0f, 1.0f, 0.0f));
    if (!restore)
        ChangeWeaponModel();
    cutscene_channel_0 = true;
    cutscene_blending = false;
    cutscene_blend = 1.0f;
    tck_time = 0;
    tck_on = false;
}


// FUNCTION: GAME 0x5877a0
void C_human::InitFFEffects()
{
    ff_shot = ff_manager.CreateEffect(JOY_EFFECT_CONSTANT_FORCE, 0.0f, 120000, 1.0f, 30, true);
    ff_manager.SetConstantForce(ff_shot, 1.0f);
    ff_jump = ff_manager.CreateEffect(JOY_EFFECT_SINE, PI, 500000, 1.0f, 20, true);
    ff_manager.SetPeriodicForce(ff_jump, 1.0f, 0.0f, 0, 500000);
    ff_hit = ff_manager.CreateEffect(JOY_EFFECT_CONSTANT_FORCE, 0.0f, 200000, 1.0f, 50, true);
    ff_manager.SetConstantForce(ff_hit, 1.0f);
    ff_fall = ff_manager.CreateEffect(JOY_EFFECT_RAMP_FORCE, 0.0f, 400000, 1.0f, 40, true);
    ff_manager.SetRampForce(ff_fall, 1.0f, -1.0f);
}

// FUNCTION: GAME 0x587880
void C_human::DoneFFEffects()
{
    ff_manager.DeleteEffect(ff_shot);
    ff_manager.DeleteEffect(ff_jump);
    ff_manager.DeleteEffect(ff_hit);
    ff_manager.DeleteEffect(ff_fall);
}

// The human dies at once (PS2 name): one of the nine deaths standing, set to its last frame, his weapon dropped and
// his blood spilt one to three times
// FUNCTION: GAME 0x5878d0
void C_human::_intern_ForceDeath()
{
    anim_id = next_anim_id = rnd(9) + ANIM_SMRT_STOJ_ZADA_A;
    int index = GetAnimIndex(next_anim_id);
    S_anim_info *info = index > -1 ? &_anim_info_[index] : NULL;
    if (info)
    {
        I3D_animation_set *set = NULL;
        anim_cache.Create(info->name, &set, NULL, NULL);
        if (set)
        {
            set->SetOptions(I3DANIMOP_ONCE);
            I3D_model *model = (I3D_model *)frame;
            model->StopAnimation(0);
            model->StopAnimation(1);
            model->StopAnimation(2);
            model->SetAnimation(set, 0, 1.0f, 0);
            model->SetAnimBlend(0, 1.0f);
            model->SetAnimBlend(1, 0.0f);
            model->SetAnimBlend(2, 0.0f);
            model->SetAnimTime(0, set->GetEndTime() - 1);
            model->Tick(0);
            model->Update();
            model->StopAnimation(0);
            set->Release();
        }
    }
    properties[HUMAN_PROPERTY_ENERGY] = 0.0f;
    alive = false;
    anim_machine.unknown_110 = true;
    Death();
    SetActState(ACTSTATE_NONE);
    PoseValidate(false);
    InventoryTruncate();
    Do_WeaponDrop();
    S_vector p = frame->GetWorldPos() + S_vector(0.0f, 1.0f, 0.0f);
    for (int i = rnd(3) + 1; i; i--)
    {
        S_vector d(rnd() + .001f, rnd(), rnd());
        d.Normalize();
        if (act_mission->options.game_effects >= OPTION_MEDIUM)
            act_mission->GetGame()->blood_manager.AddBlood(p, d, 1.0f, 0, false);
    }
}


// FUNCTION: GAME 0x587bc0
void C_human::Do_ShockStart(int i)
{
    if (box || dead_body)
        return;
    if (i < 0)
        i = 0;
    else if (i > 3)
        i = 3;
    next_anim_id = (rnd(2) ? ANIM_PANIKA_LEKNUTI01L : ANIM_PANIKA_LEKNUTI02L) + i;
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
    anim_steps[0].anim_id = i * 3 + ANIM_X_PANIKA_DREP_DOLU;
    anim_steps[0].type = HUMAN_STEP_ANIM;
    anim_steps[1].type = HUMAN_STEP_ANIM;
    anim_steps[0].stance = false;
    anim_steps[0].flags = 0;
    anim_steps[1].anim_id = i * 3 + ANIM_X_PANIKA_DREP_STATIV;
    anim_steps[1].stance = false;
    anim_steps[1].flags = 0;
    anim_steps[2].type = HUMAN_STEP_END;
    action_state = HUMAN_ACTION_STEPS;
    anim_step = 0;
}

// FUNCTION: GAME 0x587cb0
void C_human::Do_ShockUp()
{
    if (box || dead_body)
        return;
    if (anim_id == ANIM_X_PANIKA_DREP_STATIV || anim_id == ANIM_X_PANIKA_NA4_STATIV ||
        anim_id == ANIM_X_PANIKA_LEH_STATIV)
    {
        next_anim_id = anim_id + 1;
        PersonAnim(0.0f, true, false);
    }
}

// FUNCTION: GAME 0x587cf0
void C_human::Do_ShockStumble()
{
    if (box || dead_body)
        return;
    if (anim_id == ANIM_PANIKA_RUN01A || anim_id == ANIM_PANIKA_RUN02A || anim_id == ANIM_PANIKA_RUN03A)
    {
        next_anim_id = anim_id + 1;
        PersonAnim(0.0f, true, false);
    }
}

// FUNCTION: GAME 0x587d30
void C_human::cbCollTester(tDynamicCollObject *obj)
{
    if (act_mission->options.game_effects >= OPTION_HIGH)
    {
        ushort v = track_steps;
        if (!v)
        {
            v = 10;
            track_texture = TRACK_TEXTURE_BFOOTPRINT;
            track_steps = v;
        }
        track_steps_max = v;
    }
}


// The human (the player) throws the driver out of the car's seat ("cocot", Czech slang for an idiot): placed by the
// seat's door, outside the car, turned to the side the door opens to, he plays the throw. A player's or an entity's
// driver is thrown out (an entity gets the event and its script), and screams unless dead; a car of the traffic gets a
// driver made of the traffic's model, an entity in the car's seat thrown out, dead if the car's driver was killed,
// else perhaps armed and swearing. False where the seat can't be reached.
// FUNCTION: GAME 0x587d70
bool C_human::Do_ThrowCocotFromCar(C_car *car, int seat)
{
    if (box || dead_body)
        return false;
    if (!Can_DropOutFromCar_Free(car, seat))
        return false;
    bool b1, b2, b3, b4;
    if (!car->GetSeatProperty(seat, b1, b2, b3, b4))
        return false;
    S_vector car_dir;
    car_dir = car->GetWorldDir();
    S_vector side;
    side = S_vector(car_dir.z, 0.0f, -car_dir.x);
    if (b1)
        side *= -1.0f;
    side.Normalize();
    // Beside the seat's door
    S_vector p;
    if (b2) {
        switch (seat) {
        case 1:
        case 3:
            p = S_vector(1.581f, 0.0f, .251f);
            break;
        case 2:
        default:
            p = S_vector(-1.2517f, 0.0f, -.6723f);
        }
    } else {
        switch (seat) {
        case 1:
        case 3:
            p = S_vector(1.343f, 0.0f, 1.148f);
            break;
        case 2:
        default:
            p = S_vector(-1.364f, 0.0f, .231f);
        }
    }
    p += car->v_seats[seat].pos;
    p *= car->frame->GetMatrix();
    pos = p;
    dir = side;
    pos.y += _get_delta_nodyn_y_(pos);
    frame->SetPos(pos);
    frame->SetDir(dir, 0.0f);
    frame->Update();
    aimed = false;
    anim_machine.DelExtendedAnim(4.0f);
    anim_id = 0;
    if (b1)
        next_anim_id = b2 ? ANIM_VYHOZ_MAN_BACK_L : ANIM_VYHOZ_MAN_FRONT_L;
    else
        next_anim_id = b2 ? ANIM_VYHOZ_MAN_FRONT_L : ANIM_VYHOZ_MAN_BACK_L;
    PersonAnim(0.0f, true, false);
    if (act_mission->game->shvestky.active)
        act_mission->game->shvestky.CarSteal(this, true, true);
    if (car->GetOwner(seat)) {
        // A player's or an entity's driver
        if (car->GetOwner(seat)->type == ACTOR_PLAYER || car->GetOwner(seat)->type == ACTOR_ENTITY) {
            C_human *owner = car->GetOwner(seat);
            act_mission->game->sledovacka.RemoveCar(car);
            if (car->GetOwner(seat)->type == ACTOR_ENTITY)
                ((C_entity *)car->GetOwner(seat))->ForceScript(false);
            if (car->GetOwner(seat)->type == ACTOR_ENTITY)
                ((C_entity *)car->GetOwner(seat))->Event(this, ENTITY_EVENT_THROWN_FROM_CAR, NULL);
            car->GetOwner(seat)->_intern_ThrowMeFromCar();
            if (!owner->dead)
                owner->NewScream(rnd(7) + VOICE_SWEAR);
        }
    } else {
        // A car of the traffic: its driver made an entity
        bool killed = car->driver_killed;
        I3D_frame *model = C_traffic_car::Corovacka(car);
        if (model) {
            bool male = true;
            char name[64];
            I3D_frame *back = model->FindChildFrame("back1", ENUMF_ALL);
            if (back) {
                strcpy(name, back->m_pProperty ? back->m_pProperty : "");
                if (strlen(name) >= 3 && (uchar)(name[0] - 'A') == 1)
                    male = false;
            }
            I3D_model *driver_model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
            driver_model->Duplicate(model);
            driver_model->SetPos(S_vector(0.0f, 0.0f, 0.0f));
            driver_model->SetDir(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
            driver_model->Update();
            driver_model->SetOn(true);
            act_mission->scene->SetFrameSector(driver_model);
            C_entity *entity = new C_entity;
            bool armed = false;
            if (male) {
                if (!rnd(3))
                    armed = true;
                entity->max_properties[HUMAN_PROPERTY_AGGRESIVITY] = armed ? .75f : .25f;
            } else
                entity->max_properties[HUMAN_PROPERTY_AGGRESIVITY] = .25f;
            if (entity->Init(driver_model)) {
                entity->voice_type = (male ? HUMAN_VOICE_TRAFFIC_MALE : HUMAN_VOICE_TRAFFIC_FEMALE) + rnd(3);
                entity->human_type = armed ? HUMAN_TYPE_UNKNOWN_40 : HUMAN_TYPE_UNKNOWN_80;
                act_mission->game->AddTemporaryActor(entity);
                entity->temp_driver = true;
                if (!entity->traffic)
                    entity->traffic = new S_human_traffic;
                memset(entity->traffic, 0, sizeof(S_human_traffic));
                entity->traffic->traffic_frame = true;
                entity->Intern_UseCar(car, seat);
                if (killed) {
                    // Dead in the seat: killed by the hit, his voice kept quiet
                    int priority = entity->voice_priority;
                    entity->voice_priority = 1000;
                    entity->Hit(HIT_IN_CAR, car->GetDir(), car->GetPos(), car->GetDir(),
                        entity->properties[HUMAN_PROPERTY_ENERGY] + 1.0f, NULL, HIT_PART_BODY, NULL);
                    entity->SetActState(ACTSTATE_NONE);
                    entity->anim_machine.Tick(250);
                    entity->frame->Tick(250);
                    entity->anim_machine.Reset();
                    entity->voice_priority = priority;
                }
                entity->_intern_ThrowMeFromCar();
                entity->Event(this, ENTITY_EVENT_THROWN_FROM_CAR, NULL);
                car->must_steal = false;
                act_mission->game->shvestky.AddDoRiciActor(entity);
                if (!killed) {
                    if (armed) {
                        // A Colt Detective Special with 5 rounds loaded and 30 spare, or a baseball bat
                        if (!rnd(5)) {
                            S_GameItem item;
                            item.id = GAMEITEM_COLT_DETECTIVE;
                            item.ammo_loaded = 5;
                            item.ammo_spare = 30;
                            item.using_object = NULL;
                            entity->inventory.Insert(&item, false);
                        } else if (!rnd(2)) {
                            S_GameItem item;
                            item.id = GAMEITEM_BASEBALL_BAT;
                            item.ammo_loaded = 0;
                            item.ammo_spare = 0;
                            item.using_object = NULL;
                            entity->inventory.Insert(&item, false);
                        }
                        std::vector<S_GameItem> dropped;
                        entity->inventory.SetCanHaveBigWeapon(true, &dropped);
                        entity->inventory.Holster(&dropped, false, false);
                        entity->ChangeWeaponModel();
                    }
                    // He swears: one of the seven voices from VOICE_SWEAR (NewScream's ids) at the driver's model
                    uint sound = gameVoices[rnd(7) + VOICE_SWEAR - 2].sounds[entity->voice_type];
                    if (sound) {
                        sprintf(name, "%08lu.wav", sound);
                        I3D_sound *snd = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
                        if (snd) {
                            if (sound_cache.Open(snd, name, 0, NULL, NULL, NULL) == I3D_OK) {
                                snd->SetLoop(false);
                                snd->SetPos(driver_model->GetWorldPos());
                                snd->Update();
                                snd->SetSoundType(I3DSOUND_POINT);
                                snd->SetRange(1.0f, 30.0f, .4f, .25f);
                                snd->SetVolume(mafiaLoadSave.settings.speech_volume /
                                    (mafiaLoadSave.settings.sound_volume + .01f) * .95f + .05f);
                                snd->SetOutVol(0.0f);
                                snd->SetOn(true, true);
                                act_mission->game->Play3DSound(snd);
                            } else
                                snd->Release();
                        }
                    }
                }
            } else {
                entity->Release();
                driver_model->Release();
            }
        }
    }
    return true;
}


// The human thrown out of his car (Do_ThrowCocotFromCar's): the seat emptied (the driver's leaves the car without power
// or brake), back in the scene where he sits, turned as he sits, and the throw's animation by the seat's side (dead
// humans their own, and the dead ones left to the game to tick); its track's end gives the landing's height; the
// player's car indicators and aim off, the police told, the items the car held dropped
// FUNCTION: GAME 0x588b40
void C_human::_intern_ThrowMeFromCar()
{
    C_car *c = car;
    if (!c || c->type != ACTOR_CAR)
        return;
    if (seat == -1)
        return;
    bool b1, b2, b3, b4;
    if (!c->GetSeatProperty(seat, b1, b2, b3, b4))
        return;
    action_state = HUMAN_ACTION_STAND;
    if (!seat) {
        c->SetPower(0.0f, true);
        c->SetBrake(0.0f);
        if (act_mission->GetGame()->shvestky.active)
            act_mission->GetGame()->shvestky.DriveCar(this, NULL);
    }
    c->SetOwner(NULL, seat, 0.0f);
    c->unknown_2108 = 2500;
    using_car = c;
    car = NULL;
    S_matrix m = c->frame->GetMatrix();
    pos = frame->GetPos();
    dir = frame->GetDir();
    pos *= m;
    m(3).x = m(3).y = m(3).z = 0.0f;
    dir *= m;
    frame->SetPos(pos);
    frame->SetDir(dir, 0.0f);
    frame->Update();
    frame->LinkTo(act_mission->scene->primary_sector, 0);
    if (base_frame)
        act_mission->scene->SetFrameSectorPos(frame, base_frame->GetWorldPos());
    anim_id = 0;
    if (alive) {
        if (act_mission->game->initialized)
            ActivateColls(HUMAN_COLLS_ALL);
        if (b1)
            next_anim_id = b2 ? ANIM_VYHOZ_FRONT_R : ANIM_VYHOZ_BACK_R;
        else
            next_anim_id = b2 ? ANIM_VYHOZ_FRONT_L : ANIM_VYHOZ_BACK_L;
    } else {
        act_mission->game->AddDeadHumanForTick(this);
        c->SetOwner(NULL, seat, 0.0f);
        car = NULL;
        if (b1)
            next_anim_id = b2 ? ANIM_VYHOZ_MRT_FRONT_R : ANIM_VYHOZ_MRT_BACK_R;
        else
            next_anim_id = b2 ? ANIM_VYHOZ_MRT_FRONT_L : ANIM_VYHOZ_MRT_BACK_L;
    }
    PersonAnim(0.0f, true, false);
    vehicle_action = HUMAN_VEHICLE_CAR_THROWN_OUT;
    int index = GetAnimIndex(next_anim_id);
    if (index >= 0) {
        // The landing's height at the end of the throw's track
        char name[128];
        strcpy(name, _anim_info_[index].name);
        char *ext = strrchr(name, '.');
        if (ext)
            *ext = 0;
        strcat(name, ".tck");
        S_vector end = pos;
        C_animations_machine_de_luxe::GetEndPointOfTCK(name, end);
        end *= frame->GetMatrix();
        anim_machine.SetLandHeight(_get_delta_nodyn_y_(end));
    }
    if (act_mission->game->player == this) {
        mafiaIndicators.show_flags &= ~(INDF_TACHOMETR | INDF_SPEED_LIMITER | INDF_MAP | INDF_CAR_DAMAGE | INDF_RADAR);
        act_mission->GetGame()->GetCamera().SetCar(NULL);
        if (aimed) {
            Do_Aimed(false);
            ((C_player *)this)->unknown_af5 = false;
            act_mission->GetGame()->GetCamera().EnableFreeLook(false, false);
        }
    }
    AIManager.police.PanoveKokotPremistujePrdel(this, c, false);
    HideLegs((I3D_model *)frame, false);
    std::vector<S_GameItem> dropped;
    inventory.SetInCar(false, &dropped);
    if (dropped.size())
        DropOutItems(dropped);
    ChangeWeaponModel();
}


// FUNCTION: GAME 0x589140
bool C_human::Do_FuckingBox(I3D_frame *frm)
{
    if (dead_body)
        return false;
    if (frm && box)
        return false;
    if (!frm && !box)
        return false;
    if (!right_hand)
        return false;
    if (box)
    {
        if (act_mission->GetGame()->GetPlayer() == this)
            act_mission->GetGame()->BoxPlacementDestSetOU(false, box);
        box = NULL;
        // STRING: GAME 0x632228
        ChangeAnim("breath01a.i3d", NULL);
        // STRING: GAME 0x632218
        ChangeAnim("breath01b.i3d", NULL);
        // STRING: GAME 0x632208
        ChangeAnim("breath01c.i3d", NULL);
        // STRING: GAME 0x6321f8
        ChangeAnim("breath01d.i3d", NULL);
        // STRING: GAME 0x6321e0
        ChangeAnim("walk1.i3d", NULL);
        // STRING: GAME 0x6321d4
        ChangeAnim("back1.i3d", NULL);
        // STRING: GAME 0x6321c8
        ChangeAnim("left1.i3d", NULL);
        // STRING: GAME 0x6321bc
        ChangeAnim("right1.i3d", NULL);
        // STRING: GAME 0x6321a4
        ChangeAnim("strafL1.i3d", NULL);
        // STRING: GAME 0x632198
        ChangeAnim("strafR1.i3d", NULL);
        // STRING: GAME 0x632154
        ChangeAnim("walkL1.i3d", NULL);
        // STRING: GAME 0x632148
        ChangeAnim("walkR1.i3d", NULL);
        // STRING: GAME 0x63213c
        ChangeAnim("backL1.i3d", NULL);
        // STRING: GAME 0x632130
        ChangeAnim("backR1.i3d", NULL);
        anim_id = 0;
        if (weapon_model)
            weapon_model->SetOn(true);
        if (weapon_model2)
            weapon_model2->SetOn(true);
        if (act_mission->GetGame()->GetPlayer() == this)
        {
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_GET_IN_FRONT);
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_GET_IN_BACK);
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_BOARD_TRAIN);
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_HIJACK_CAR);
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_PICK_UP_BOX);
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_SEARCH_DEAD_BODY);
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_MOVE_DEAD_BODY);
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_GET_IN_AS_DRIVER);
        }
    }
    else
    {
        if (act_mission->GetGame()->GetPlayer() == this)
            box_index = act_mission->GetGame()->BoxPlacementDestSetOU(true, frm);
        box = frm;
        box->SetPos(0.0f, 0.0f, 0.0f);
        box->SetDir(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
        box->Update();
        box->LinkTo(right_hand, 0);
        ChangeAnim("breath01a.i3d",
            // STRING: GAME 0x636d48
            "BednaStativ.i3d");
        ChangeAnim("breath01b.i3d", "BednaStativ.i3d");
        ChangeAnim("breath01c.i3d", "BednaStativ.i3d");
        ChangeAnim("breath01d.i3d", "BednaStativ.i3d");
        ChangeAnim("walk1.i3d",
            // STRING: GAME 0x636d38
            "BednaWalk.i3d");
        ChangeAnim("back1.i3d",
            // STRING: GAME 0x636d28
            "BednaBack.i3d");
        ChangeAnim("left1.i3d",
            // STRING: GAME 0x636d18
            "BednaLeft.i3d");
        ChangeAnim("right1.i3d",
            // STRING: GAME 0x636d08
            "BednaRight.i3d");
        ChangeAnim("strafL1.i3d",
            // STRING: GAME 0x636cf8
            "BednaStrafL.i3d");
        ChangeAnim("strafR1.i3d",
            // STRING: GAME 0x636ce8
            "BednaStrafR.i3d");
        ChangeAnim("walkL1.i3d",
            // STRING: GAME 0x636cd8
            "BednaWalkL.i3d");
        ChangeAnim("walkR1.i3d",
            // STRING: GAME 0x636cc8
            "BednaWalkR.i3d");
        ChangeAnim("backL1.i3d",
            // STRING: GAME 0x636cb8
            "BednaBackL.i3d");
        ChangeAnim("backR1.i3d",
            // STRING: GAME 0x636ca8
            "BednaBackR.i3d");
        anim_id = 0;
        if (weapon_model)
            weapon_model->SetOn(false);
        if (weapon_model2)
            weapon_model2->SetOn(false);
        throw_swung = false;
        if (this == act_mission->GetGame()->GetPlayer())
            mafiaIndicators.show_flags &= ~INDF_THIEF;
        anim_machine.DelExtendedAnim(4.0f);
        ResetWeaponEffects();
        reload_effects = false;
        Do_Aimed(false);
        fire_held = false;
        if (act_mission->GetGame()->GetPlayer() == this)
        {
            act_mission->GetGame()->GetUsingObject().AddInvalidItem(USING_ACTION_GET_IN_FRONT);
            act_mission->GetGame()->GetUsingObject().AddInvalidItem(USING_ACTION_GET_IN_BACK);
            act_mission->GetGame()->GetUsingObject().AddInvalidItem(USING_ACTION_BOARD_TRAIN);
            act_mission->GetGame()->GetUsingObject().AddInvalidItem(USING_ACTION_HIJACK_CAR);
            act_mission->GetGame()->GetUsingObject().AddInvalidItem(USING_ACTION_PICK_UP_BOX);
            act_mission->GetGame()->GetUsingObject().AddInvalidItem(USING_ACTION_SEARCH_DEAD_BODY);
            act_mission->GetGame()->GetUsingObject().AddInvalidItem(USING_ACTION_MOVE_DEAD_BODY);
            act_mission->GetGame()->GetUsingObject().AddInvalidItem(USING_ACTION_GET_IN_AS_DRIVER);
        }
    }
    if (act_mission->GetGame()->GetPlayer() == this)
        ((C_player *)this)->IndicatorSetAimed();
    if (!GetCar())
        act_mission->GetGame()->SetFrameForShadow(GetFrame(), SHADOW_TYPE_0, SHADOW_TYPE_0);
    return true;
}


// FUNCTION: GAME 0x589680
bool C_human::Do_FuckingBoxCancel()
{
    if (!box || !right_hand)
        return false;
    bool found = false;
    S_vector pos = box->GetWorldPos();
    std::vector<C_actor *> &actors = C_actor::global_actors;
    for (int i = actors.size(); i--; )
    {
        if (actors[i]->type != ACTOR_CAR)
            continue;
        if (actors[i]->act_state == ACTSTATE_NONE)
            continue;
        if ((actors[i]->GetFrame()->GetWorldPos() - pos).Square() < 100.0f)
        {
            I3D_bvolume vol = actors[i]->GetFrame()->GetHRBoundVolume();
            S_matrix m = actors[i]->GetFrame()->GetMatrix();
            m.Invert();
            S_vector p = pos;
            p *= m;
            if (vol.bbox.min.x - .4f <= p.x && vol.bbox.max.x + .4f >= p.x && vol.bbox.min.y - .4f <= p.y &&
                vol.bbox.max.y + .4f >= p.y && vol.bbox.min.z - .4f <= p.z && vol.bbox.max.z + .4f >= p.z)
            {
                found = true;
                break;
            }
        }
    }
    if (found)
    {
        I3D_frame *dest = act_mission->GetGame()->BoxPlacementGetEnemyDest(this, box_index);
        S_vector start;
        if (!act_mission->GetGame()->BoxPlacement_GetStartPoint(box, start))
            start = S_vector(0.0f, -10000.0f, 0.0f);
        S_vector d = pos - start;
        S_vector e = pos - dest->GetWorldPos();
        if (d.Square() > e.Square())
        {
            if (dest)
                act_mission->GetGame()->BoxPlacementGetNextDest(box, dest);
        }
        else
        {
            act_mission->GetGame()->BoxPlacement_RestoreInfo(box);
            act_mission->GetGame()->BoxPlacementAdd(box, box_index);
            act_mission->GetGame()->BoxPlacementRecompile();
        }
        if (act_mission->GetGame()->GetPlayer() == this)
            act_mission->GetGame()->BoxPlacementDestSetOU(false, box);
        box = NULL;
        ChangeAnim("breath01a.i3d", NULL);
        ChangeAnim("breath01b.i3d", NULL);
        ChangeAnim("breath01c.i3d", NULL);
        ChangeAnim("breath01d.i3d", NULL);
        ChangeAnim("walk1.i3d", NULL);
        ChangeAnim("back1.i3d", NULL);
        ChangeAnim("left1.i3d", NULL);
        ChangeAnim("right1.i3d", NULL);
        ChangeAnim("strafL1.i3d", NULL);
        ChangeAnim("strafR1.i3d", NULL);
        ChangeAnim("walkL1.i3d", NULL);
        ChangeAnim("walkR1.i3d", NULL);
        ChangeAnim("backL1.i3d", NULL);
        ChangeAnim("backR1.i3d", NULL);
        anim_id = 0;
        if (weapon_model)
            weapon_model->SetOn(true);
        if (weapon_model2)
            weapon_model2->SetOn(true);
        if (act_mission->GetGame()->GetPlayer() == this)
        {
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_GET_IN_FRONT);
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_GET_IN_BACK);
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_BOARD_TRAIN);
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_HIJACK_CAR);
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_PICK_UP_BOX);
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_SEARCH_DEAD_BODY);
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_MOVE_DEAD_BODY);
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_GET_IN_AS_DRIVER);
        }
        if (act_mission->GetGame()->GetPlayer() == this)
            ((C_player *)this)->IndicatorSetAimed();
        if (!GetCar())
            act_mission->GetGame()->SetFrameForShadow(GetFrame(), SHADOW_TYPE_0, SHADOW_TYPE_0);
        return true;
    }
    return false;
}


// FUNCTION: GAME 0x589c20
bool C_human::Do_DeadBody(I3D_frame *frm)
{
    if (box)
        return false;
    if (frm && dead_body)
        return false;
    if (!frm && !dead_body)
        return false;
    if (!right_hand)
        return false;
    if (dead_body)
    {
        dead_body->LinkTo(act_mission->GetScene()->GetPrimarySector(), 1);
        act_mission->GetScene()->SetFrameSector(dead_body);
        if (dead_body_human)
        {
            dead_body_human->SetPos(dead_body->GetPos());
            dead_body_human->SetDir(dead_body->GetDir());
            dead_body_human->using_objects[0].pos = dead_body->GetWorldPos();
            dead_body_human->using_objects[0].pos.y += .5f;
            dead_body_human->using_objects[1].pos = dead_body->GetWorldPos();
            dead_body_human->using_objects[1].pos.y += .5f;
            act_mission->GetGame()->GetUsingObject().AddObject(&dead_body_human->using_objects[0]);
            dead_body_time = dead_body_human->Do_PlayAnim("NoseniMrtvolyMrtShozeni.I3D", true, false);
        }
        dead_body = NULL;
        ChangeAnim("breath01a.i3d", NULL);
        ChangeAnim("breath01b.i3d", NULL);
        ChangeAnim("breath01c.i3d", NULL);
        ChangeAnim("breath01d.i3d", NULL);
        ChangeAnim("walk1.i3d", NULL);
        ChangeAnim("back1.i3d", NULL);
        ChangeAnim("left1.i3d", NULL);
        ChangeAnim("right1.i3d", NULL);
        ChangeAnim("strafL1.i3d", NULL);
        ChangeAnim("strafR1.i3d", NULL);
        ChangeAnim("walkL1.i3d", NULL);
        ChangeAnim("walkR1.i3d", NULL);
        ChangeAnim("backL1.i3d", NULL);
        ChangeAnim("backR1.i3d", NULL);
        anim_id = 0;
        if (weapon_model)
            weapon_model->SetOn(true);
        if (weapon_model2)
            weapon_model2->SetOn(true);
        if (act_mission->GetGame()->GetPlayer() == this)
        {
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_GET_IN_FRONT);
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_GET_IN_BACK);
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_BOARD_TRAIN);
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_HIJACK_CAR);
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_PICK_UP_BOX);
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_SEARCH_DEAD_BODY);
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_MOVE_DEAD_BODY);
            act_mission->GetGame()->GetUsingObject().DelInvalidItem(USING_ACTION_GET_IN_AS_DRIVER);
        }
    }
    else
    {
        dead_body = frm;
        dead_body->SetPos(0.0f, 0.0f, 0.0f);
        dead_body->SetDir(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
        dead_body->Update();
        dead_body->LinkTo(frame, 0);
        dead_body_human = NULL;
        dead_body_time = -1;
        C_actor **p = GetFrameActor(frm);
        if (p && *p && ((*p)->type == ACTOR_PLAYER || (*p)->type == ACTOR_ENTITY))
        {
            dead_body_human = (C_human *)*p;
            dead_body_human->anim_machine.Reset();
            act_mission->GetGame()->GetUsingObject().DelObject(&dead_body_human->using_objects[0]);
            act_mission->GetGame()->GetUsingObject().DelObject(&dead_body_human->using_objects[1]);
        }
        ChangeAnim("breath01a.i3d",
            // STRING: GAME 0x636e70
            "NoseniMrtvolyTomStat.I3D");
        ChangeAnim("breath01b.i3d", "NoseniMrtvolyTomStat.I3D");
        ChangeAnim("breath01c.i3d", "NoseniMrtvolyTomStat.I3D");
        ChangeAnim("breath01d.i3d", "NoseniMrtvolyTomStat.I3D");
        ChangeAnim("walk1.i3d",
            // STRING: GAME 0x636e54
            "NoseniMrtvolyTomWalk.I3D");
        ChangeAnim("back1.i3d",
            // STRING: GAME 0x636e38
            "NoseniMrtvolyTomBack.I3D");
        ChangeAnim("left1.i3d",
            // STRING: GAME 0x636e1c
            "NoseniMrtvolyTomLeft.I3D");
        ChangeAnim("right1.i3d",
            // STRING: GAME 0x636e00
            "NoseniMrtvolyTomRight.I3D");
        ChangeAnim("strafL1.i3d",
            // STRING: GAME 0x636de4
            "NoseniMrtvolyTomStrafL.I3D");
        ChangeAnim("strafR1.i3d",
            // STRING: GAME 0x636dc8
            "NoseniMrtvolyTomStrafR.I3D");
        ChangeAnim("walkL1.i3d",
            // STRING: GAME 0x636dac
            "NoseniMrtvolyTomWalkL.I3D");
        ChangeAnim("walkR1.i3d",
            // STRING: GAME 0x636d90
            "NoseniMrtvolyTomWalkR.I3D");
        ChangeAnim("backL1.i3d",
            // STRING: GAME 0x636d74
            "NoseniMrtvolyTomBackL.I3D");
        ChangeAnim("backR1.i3d",
            // STRING: GAME 0x636d58
            "NoseniMrtvolyTomBackR.I3D");
        anim_id = 0;
        next_anim_id = ANIM_NOSENI_MRTVOLY_TOM_NAHOZENI;
        PersonAnim(0.0f, true, false);
        if (weapon_model)
            weapon_model->SetOn(false);
        if (weapon_model2)
            weapon_model2->SetOn(false);
        S_vector d = act_mission->GetScene()->GetActiveCamera()->GetWorldPos() - GetWorldPos();
        if (act_mission->options.game_sounds >= OPTION_HIGH && d.Square() < 400.0f)
        {
            I3D_sound *sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
            if (sound)
            {
                sound->SetPos(pos);
                sound->Update();
                if (sound_table.ConstructSound(sound, SOUND_DEAD_BODY_PICKUP))
                    act_mission->GetGame()->Play3DSound(sound);
                else
                    sound->Release();
            }
        }
        throw_swung = false;
        if (this == act_mission->GetGame()->GetPlayer())
            mafiaIndicators.show_flags &= ~INDF_THIEF;
        anim_machine.DelExtendedAnim(4.0f);
        ResetWeaponEffects();
        reload_effects = false;
        Do_Aimed(false);
        fire_held = false;
        if (act_mission->GetGame()->GetPlayer() == this)
        {
            act_mission->GetGame()->GetUsingObject().AddInvalidItem(USING_ACTION_GET_IN_FRONT);
            act_mission->GetGame()->GetUsingObject().AddInvalidItem(USING_ACTION_GET_IN_BACK);
            act_mission->GetGame()->GetUsingObject().AddInvalidItem(USING_ACTION_BOARD_TRAIN);
            act_mission->GetGame()->GetUsingObject().AddInvalidItem(USING_ACTION_HIJACK_CAR);
            act_mission->GetGame()->GetUsingObject().AddInvalidItem(USING_ACTION_PICK_UP_BOX);
            act_mission->GetGame()->GetUsingObject().AddInvalidItem(USING_ACTION_SEARCH_DEAD_BODY);
            act_mission->GetGame()->GetUsingObject().AddInvalidItem(USING_ACTION_MOVE_DEAD_BODY);
            act_mission->GetGame()->GetUsingObject().AddInvalidItem(USING_ACTION_GET_IN_AS_DRIVER);
        }
    }
    if (act_mission->GetGame()->GetPlayer() == this)
        ((C_player *)this)->IndicatorSetAimed();
    if (!GetCar())
        act_mission->GetGame()->SetFrameForShadow(GetFrame(), SHADOW_TYPE_0, SHADOW_TYPE_0);
    return true;
}


// The car the human sits in blew up (PS2 name, "my little car in the air"; the parameter's name is ours): the human
// goes with the wreck, burnt and dead, the death's animation set to its last frame
// FUNCTION: GAME 0x58a5a0
void C_human::EineMeineKleineAutoInLuft(C_car *wreck)
{
    C_car *in = car;
    if (!in || in->GetType() != ACTOR_CAR)
        return;
    if (wreck->frame)
        frame->LinkTo(wreck->frame, 0);
    car = wreck;
    act_mission->GetGame()->DeathHumanTalk(this);
    // STRING: GAME 0x632418
    _intern_ChangeModel("ohorelec01.i3d", false);
    if (alive)
    {
        properties[HUMAN_PROPERTY_ENERGY] = 0.0f;
        alive = false;
        anim_machine.unknown_110 = true;
        Death();
        SetActState(ACTSTATE_NONE);
    }
    if (wingman > -1)
        mafiaIndicators.PlayerSetWingmanLives(wingman, 0);
    if (car_stance == HUMAN_CARSTANCE_AIMED)
        next_anim_id = in->IsSeatLeft(seat) ? ANIM_SMRT_AUTO_SPOL_OUTL : ANIM_SMRT_AUTO_SPOL_OUTR;
    else if (!seat)
        next_anim_id = ANIM_SMRT_AUTO_RIDIC01;
    else if (weapon_kind == WEAPON_KIND_RIFLE || weapon_kind == WEAPON_KIND_SMG)
        next_anim_id = ANIM_SMRT_AUTO_SPOL_IN02;
    else
        next_anim_id = ANIM_SMRT_AUTO_SPOL_IN01;
    int index = GetAnimIndex(next_anim_id);
    S_anim_info *info = index > -1 ? &_anim_info_[index] : NULL;
    if (info)
    {
        I3D_animation_set *set = NULL;
        anim_cache.Create(info->name, &set, NULL, NULL);
        if (set)
        {
            set->SetOptions(I3DANIMOP_ONCE);
            I3D_model *model = (I3D_model *)frame;
            model->StopAnimation(0);
            model->StopAnimation(1);
            model->StopAnimation(2);
            model->SetAnimation(set, 0, 1.0f, 0);
            model->SetAnimBlend(0, 1.0f);
            model->SetAnimBlend(1, 0.0f);
            model->SetAnimBlend(2, 0.0f);
            model->SetAnimTime(0, set->GetEndTime() - 1);
            model->Tick(0);
            model->Update();
            model->StopAnimation(0);
            set->Release();
        }
    }
    if (act_mission->GetGame()->player == this)
        mafiaIndicators.PlayerSetLives(0);
    action_state = HUMAN_ACTION_DEAD;
}


// FUNCTION: GAME 0x58a790
bool C_human::Can_Aimed()
{
    S_vector pos = frame->GetWorldPos();
    pos.y += 1.5f;
    S_vector dir = frame->GetWorldDir();
    return gifCollision.TestLineH(pos, dir, NULL, NULL, COLF_UNKNOWN_2, COLF2_UNKNOWN_80000) ? false : true;
}


// FUNCTION: GAME 0x58a960
bool C_human::Do_CornerBreath(E_corner_view view)
{
    if (box || dead_body)
        return false;
    switch (view)
    {
    case CORNER_0:
        next_anim_id = ANIM_ENM_ROH_STOJ_STAT_L;
        return true;
    case CORNER_1:
        next_anim_id = ANIM_ENM_ROH_STOJ_STAT_R;
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
        next_anim_id = ANIM_ENM_ROH_STOJ_NAHL_L;
        break;
    case CORNER_1:
        next_anim_id = ANIM_ENM_ROH_STOJ_NAHL_R;
        break;
    case CORNER_2:
        if (!crouching)
            return false;
        next_anim_id = ANIM_ENM_ROH_DREP_NAHL_T;
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
    if (weapon_kind == WEAPON_KIND_RIFLE || weapon_kind == WEAPON_KIND_FISTS)
        return false;
    switch (view)
    {
    case CORNER_0:
        next_anim_id = ANIM_ENM_ROH_FIRE01_L;
        if (!crouching && inventory.GetWeaponInRightHand()->fire_anim == GUN_ANIM_THOMPSON)
            next_anim_id = ANIM_ENM_ROH_SAP_STOJ_FIRE01_L;
        break;
    case CORNER_1:
        next_anim_id = ANIM_ENM_ROH_FIRE01_R;
        if (!crouching && inventory.GetWeaponInRightHand()->fire_anim == GUN_ANIM_THOMPSON)
            next_anim_id = ANIM_ENM_ROH_SAP_STOJ_FIRE01_R;
        break;
    case CORNER_2:
        if (!crouching)
            return false;
        next_anim_id = ANIM_ENM_ROH_DREP_FIRE01_T;
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
    if (weapon_kind == WEAPON_KIND_RIFLE || weapon_kind == WEAPON_KIND_FISTS)
        return false;
    switch (view)
    {
    case CORNER_0:
        next_anim_id = ANIM_ENM_ROH_FIRE02_L;
        if (!crouching && inventory.GetWeaponInRightHand()->fire_anim == GUN_ANIM_THOMPSON)
            next_anim_id = ANIM_ENM_ROH_SAP_STOJ_FIRE02_L;
        break;
    case CORNER_1:
        next_anim_id = ANIM_ENM_ROH_FIRE02_R;
        if (!crouching && inventory.GetWeaponInRightHand()->fire_anim == GUN_ANIM_THOMPSON)
            next_anim_id = ANIM_ENM_ROH_SAP_STOJ_FIRE02_R;
        break;
    case CORNER_2:
        if (!crouching)
            return false;
        next_anim_id = ANIM_ENM_ROH_DREP_FIRE02_T;
        break;
    default:
        return false;
    }
    PersonAnim(0.0f, true, false);
    aimed = true;
    return true;
}

// FUNCTION: GAME 0x58ac50
void C_human::Force_CanWorking()
{
    action_state = HUMAN_ACTION_STAND;
}

// FUNCTION: GAME 0x58ac60
void C_human::Turn_Turn()
{
    next_anim_id = ANIM_TURN1;
    velocity_ramp = 0.0f;
    last_velocity.Zero();
    PersonAnim(0.0f, true, false);
}

// FUNCTION: GAME 0x58ac90
void C_human::Activate()
{
    dead = false;
    alive = true;
    rendered = false;
    frame->SetOn(true);
    pos = frame->GetPos();
    dir = frame->GetDir();
    act_state = ACTSTATE_ACTIVE;
    vehicle_action = 0;
    action_state = HUMAN_ACTION_STAND;
    anim_machine.Init(this);
    next_anim_id = ANIM_BREATH01A;
    memcpy(properties, max_properties, sizeof(properties));
    anim_id = 0;
    anim_set = 0;
    car = NULL;
    using_car = NULL;
    velocity.Zero();
    last_velocity = S_vector(0.0f, 0.0f, 0.0f);
    turn_speed = 0;
    fall = 0.0f;
    velocity_ramp = 0.0f;
    idle_time = -1;
    move_pos = pos;
    breath_time = -1;
    guarding = false;
    striking = false;
    sniper_mode = false;
    dead_time = 1500;
    agony_anim = -1;
    wingman = -1;
    last_fire_time = 0;
    fire_target = NULL;
    track_test = 0;
    bleed_count = 0;
    track_steps = 0;
    traffic = NULL;
    box = NULL;
    human_flags = 0;
    move_time = 0;
    move_ratio = 1.0f;
    step_time = 0;
    move_coll = 0;
    no_anim_hit = false;
    note_fire = false;
    reloading = false;
    reload_forced = false;
    loading_shells = false;
    car_stance = HUMAN_CARSTANCE_SEATED;
    car_stance_time = 0;
    car_anim_index = 0;
    car_anim_length = 0;
    car_aim = .5f;
    pose_moving = false;
    anim_step = 0;
    anim_steps[0].type = HUMAN_STEP_END;
    last_hit_time = 0;
    shots = 1.0f;
    hits = 1.0f;
    prev_shots = 1.0f;
    prev_hits = 1.0f;
    last_hit_render = 0;
    idle_anims = false;
    test_ground = true;
    voice_priority = -1;
    dead_body = NULL;
    dead_body_human = 0;
    dead_body_time = -1;
    lower_coll_off = false;
    killed_by_shot = false;
    death_pos_set = false;
    stealing_car = NULL;
    freeze_time = -1;
    effect_step = 0;
    pump_time = -1;
    hand_clip = NULL;
    killer = NULL;
    drowned = false;
    changing_weapon = false;
    scream_areas = 1;
    unknown_b0 = 0;
    next_car_action = 0;
    car_action_time = -1;
    speed_limiter = false;
    strike_anim_time = -1;
    score_bonus = false;
    temp_driver = false;
    last_car = 0;
    weapon_displaced = false;
    reload_effects = false;
    shut_up = false;
    ab_created = true;
    strike_time = 0;
    locked_car = NULL;
    locked_vehicle = NULL;
    CreateDynColls();
    for (int i = anims.size(); i--;)
        delete[] anims[i].name;
    anims.clear();
    aimed = false;
    throw_swung = false;
    fire_held = false;
    crouching = false;
    strike_index = 0;
    weapon_kind = WEAPON_KIND_FISTS;
    ground_material = 0;
    tilt = .5f;
    shadow = act_mission->GetGame()->AddFrameForShadow(frame, SHADOW_TYPE_0, SHADOW_TYPE_0, NULL, NULL);
    pose_flags = 0;
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
    reload_effects = false;
    for (int i = anims.size(); i--;)
        delete[] anims[i].name;
    anims.clear();
    car = NULL;
    using_car = NULL;
    HideLegs((I3D_model *)frame, false);
}


// The human's voice says voices.def's entry id - 2 in his voice_type, unless he is shut up, out of the camera's
// scream areas, or says something of a higher priority already
// FUNCTION: GAME 0x58b470
void C_human::NewScream(uint id)
{
    if (shut_up || !voice)
        return;
    if (!(scream_areas & act_mission->GetGame()->camera_scream_areas))
        return;
    id -= 2;
    if (id >= numOfGameVoices)
        return;
    if (voice->IsPlaying(false) && gameVoices[id].priority <= voice_priority)
        return;
    if (voice_priority > 500)
        return;
    uint sound = gameVoices[id].sounds[voice_type];
    if (!sound)
        return;
    char name[16];
    voice_priority = gameVoices[id].priority;
    // STRING: GAME 0x6326ec
    sprintf(name, "%08lu.wav", sound);
    voice->SetOn(false, true);
    if (sound_cache.Open(voice, name, 0, NULL, NULL, NULL))
        return;
    voice->SetLoop(false);
    voice->SetPos(S_vector(0.0f, 1.7f, 0.0f));
    voice->LinkTo(frame, 0);
    voice->Update();
    voice->SetSoundType(I3DSOUND_POINT);
    voice->SetRange(1.0f, 30.0f, .4f, .3f);
    voice->SetVolume(mafiaLoadSave.settings.speech_volume / (mafiaLoadSave.settings.sound_volume + .01f) * .95f +
        .05f);
    voice->SetOutVol(0.0f);
    voice->SetOn(true, true);
}


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
        weapon->item.id = GAMEITEM_NONE;
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
    next_anim_id = ANIM_NOSENI_MRTVOLY_TOM_SHOZENI;
    PersonAnim(0.0f, true, false);
    SetWaitAnim();
}

// FUNCTION: GAME 0x58b870
void C_human::ResetActions()
{
    Do_Crouched(false);
    aimed = false;
    fire_held = false;
    if (alive)
    {
        anim_id = 0;
        next_anim_id = ANIM_BREATH01A;
        PersonAnim(0.0f, true, false);
    }
    PoseNullRotate();
    if (type == ACTOR_PLAYER)
    {
        ((C_player *)this)->fade_alpha = 1.0f;
        ((C_player *)this)->SetAlpha(1.0f);
    }
}


// RecompileDeathPos's helpers, static (PC only, the names are ours; PS2 has their code in RecompileDeathPos): VC6
// places each after its first caller
static void SettleOnGround(I3D_frame *frm, S_vector *pts, int num);
static bool PushOutOfWalls(I3D_frame *frm, S_vector *pts, int num);
static bool SlideOffEdge(I3D_frame *frm, S_vector neck, S_vector base, S_vector feet);

// The dead body settled onto the ground (PS2 name): the death's animation at its last frame gives the bones neck,
// r_foot, l_foot and base, a dummy frame in the human's place is moved and turned until they lie on the ground and
// out of the walls, and the human's frame takes the dummy's rotation and moves to its position in death_length ms
// (ProcessDeathPos), a third of the animation, at once without one
// FUNCTION: GAME 0x58b8d0
void C_human::RecompileDeathPos()
{
    static char buff[128];
    I3D_model *model = (I3D_model *)frame;
    death_pos_set = true;
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
    bool anim = false;
    if (af) {
        strcpy(buff, af->name);
        char *side = strchr(buff, '?');
        if (side)
            *side = anim_left ? 'L' : 'R';
        I3D_animation_set *set = NULL;
        anim_cache.Create(buff, &set, NULL, NULL);
        if (set) {
            set->SetOptions(I3DANIMOP_ONCE);
            int end = set->GetEndTime() - 1;
            death_length = end / 3;
            model->SetAnimation(set, 3, 1.0f, 0);
            model->SetAnimBlend(3, 1.0f);
            set->Release();
            model->SetAnimTime(3, end);
            model->Tick(0);
            model->Update();
            anim = true;
        }
    }
    bool ok = true;
    S_vector pts[4];
    I3D_frame *frm = frame->FindChildFrame("neck", ENUMF_ALL);
    if (frm)
        pts[0] = frm->GetWorldPos();
    else
        ok = false;
    frm = frame->FindChildFrame("r_foot", ENUMF_ALL);
    if (frm)
        pts[1] = frm->GetWorldPos();
    else
        ok = false;
    frm = frame->FindChildFrame("l_foot", ENUMF_ALL);
    if (frm)
        pts[2] = frm->GetWorldPos();
    else
        ok = false;
    frm = frame->FindChildFrame("base", ENUMF_ALL);
    if (frm)
        pts[3] = frm->GetWorldPos();
    else
        ok = false;
    S_matrix m = frame->GetMatrix();
    m.Invert();
    for (i = 4; i--; )
        pts[i] *= m;
    if (anim) {
        model->SetAnimBlend(3, 0.0f);
        model->StopAnimation(3);
        model->Tick(0);
        model->Update();
    }
    death_pos = frame->GetPos();
    death_move.Zero();
    death_time = 0;
    if (ok) {
        I3D_frame *dummy = driver->CreateFrame(FRAME_DUMMY);
        if (dummy) {
            dummy->SetPos(frame->GetPos());
            dummy->SetRot(frame->GetRot());
            dummy->Update();
            dummy->LinkTo(frame->GetParent(), 0);
            for (i = 2; i--; ) {
                int j;
                for (j = 0; j < 2; j++)
                    SettleOnGround(dummy, pts, 4);
                if (!PushOutOfWalls(dummy, pts, 4))
                    break;
                for (j = 0; j < 2; j++)
                    SettleOnGround(dummy, pts, 4);
            }
            S_vector feet = pts[1] + pts[2];
            feet *= .5f;
            if (SlideOffEdge(dummy, pts[0], pts[3], feet))
                SettleOnGround(dummy, pts, 4);
            death_move = dummy->GetPos() - death_pos;
            frame->SetRot(dummy->GetRot());
            frame->Update();
            dummy->Release();
            if (!anim) {
                death_length = 1;
                ProcessDeathPos(1);
            }
        }
    } else
        death_length = -1;
    if (death_move.Square() > 1.0f && death_move.AngleTo(S_vector(0.0f, 1.0f, 0.0f)) < PI * .2f)
        death_move.y = 0.0f;
}


// The frame lifted or lowered by the ground's mean height under the points (in its space, 1 above them to 2 below)
// and turned to the mean normal of the ground less than 63 degrees steep
// FUNCTION: GAME 0x58bf30
static void SettleOnGround(I3D_frame *frm, S_vector *pts, int num)
{
    S_vector normal(0.0f, 0.0f, 0.0f);
    float height = 0.0f;
    float count = 0.0f;
    for (int i = num; i--; ) {
        S_vector p = pts[i];
        p *= frm->GetMatrix();
        float y = p.y;
        p.y += 1.0f;
        S_vector pos, n;
        if (gifCollision.TestLineV(p, S_vector(0.0f, -3.0f, 0.0f), &pos, &n, COLF_ALL, 0)) {
            height += pos.y - y + pts[i].y;
            count += 1.0f;
            if (n.AngleTo(S_vector(0.0f, 1.0f, 0.0f)) < PI * .35f)
                normal += n;
        }
    }
    if (!(normal.Square() < 1e-16f)) {
        normal.Normalize();
        S_matrix m = frm->GetParent()->GetMatrix();
        m.Invert();
        m(3).Zero();
        normal *= m;
        S_vector dir = frm->GetDir();
        S_vector side = dir.Cross(normal);
        dir = normal.Cross(side);
        frm->m_LocalMatrix.SetDir3(dir, normal);
        frm->m_FrmFlags = (frm->m_FrmFlags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID |
            FRMFLAGS_QUAT_VALID | FRMFLAGS_ROT_VALID)) | FRMFLAGS_UPDATE | FRMFLAGS_MAT_VALID;
    }
    if (count > 0.0f)
        height /= count;
    S_vector p = frm->GetPos();
    p.y += height;
    frm->SetPos(p);
    frm->Update();
}


// The frame moved out of the walls (steeper than 45 degrees) between it and the points: back from the point deepest
// in one by its depth and 0.3 more; false where no point is in a wall
// FUNCTION: GAME 0x58c4f0
static bool PushOutOfWalls(I3D_frame *frm, S_vector *pts, int num)
{
    int best = -1;
    float best_dist = -1.0f;
    S_vector dir, hit;
    for (int i = num; i--; ) {
        S_vector p = pts[i];
        p *= frm->GetMatrix();
        S_vector from = frm->GetWorldPos();
        dir = p - from;
        from.y += .1f;
        S_vector pos, normal;
        if (gifCollision.TestLineH(from, dir, &pos, &normal, COLF_ALL, 0) &&
            normal.AngleTo(S_vector(0.0f, 1.0f, 0.0f)) > PI * .25f) {
            dir = pos - p;
            float sq = dir.Square();
            if (sq > best_dist) {
                best = i;
                best_dist = sq;
                hit = pos;
            }
        }
    }
    if (best > -1) {
        S_vector fpos = frm->GetPos();
        S_vector p = pts[best];
        p *= frm->GetMatrix();
        dir = hit - p;
        float len = dir.Magnitude();
        dir.Normalize();
        dir *= len + .3f;
        fpos += dir;
        frm->SetPos(fpos);
        frm->Update();
        return true;
    }
    return false;
}


// The body over an edge (ground within 0.5 under the neck, the base or the feet, but not under all): the frame moved
// from the side on the ground towards the other as far as a wall lets it; with no ground under any, lowered to the
// nearest ground within 50 (false for none, or nearer than 0.3); false where the neck and the feet are both on ground
// FUNCTION: GAME 0x58c980
static bool SlideOffEdge(I3D_frame *frm, S_vector neck, S_vector base, S_vector feet)
{
    neck *= frm->GetMatrix();
    base *= frm->GetMatrix();
    feet *= frm->GetMatrix();
    bool on_neck = gifCollision.TestLineV(neck, S_vector(0.0f, -.5f, 0.0f), NULL, NULL, COLF_ALL, 0) != 0;
    bool on_base = gifCollision.TestLineV(base, S_vector(0.0f, -.5f, 0.0f), NULL, NULL, COLF_ALL, 0) != 0;
    bool on_feet = gifCollision.TestLineV(feet, S_vector(0.0f, -.5f, 0.0f), NULL, NULL, COLF_ALL, 0) != 0;
    if (on_neck && on_base && on_feet)
        return false;
    if (on_neck && on_feet)
        return false;
    if (!on_neck && !on_base && !on_feet) {
        float best = 10000.0f;
        S_vector pos;
        if (gifCollision.TestLineV(neck, S_vector(0.0f, -50.0f, 0.0f), &pos, NULL, COLF_ALL, 0)) {
            float d = (neck - pos).Square();
            if (d < best)
                best = d;
        }
        if (gifCollision.TestLineV(base, S_vector(0.0f, -50.0f, 0.0f), &pos, NULL, COLF_ALL, 0)) {
            float d = (base - pos).Square();
            if (d < best)
                best = d;
        }
        if (gifCollision.TestLineV(feet, S_vector(0.0f, -50.0f, 0.0f), &pos, NULL, COLF_ALL, 0)) {
            float d = (feet - pos).Square();
            if (d < best)
                best = d;
        }
        if (best < 9999.0f && best > .1f) {
            S_vector p = frm->GetPos();
            p.y -= (float)sqrt(best);
            frm->SetPos(p);
            frm->Update();
            return true;
        }
        return false;
    }
    S_vector from, dir;
    if (on_neck) {
        from = neck;
        if (on_base)
            dir = base - feet;
        else
            dir = neck - feet;
    } else {
        from = feet;
        if (on_base)
            dir = base - neck;
        else
            dir = feet - neck;
    }
    S_vector pos;
    if (gifCollision.TestLineH(from, dir, &pos, NULL, COLF_ALL, 0))
        dir = pos - from;
    S_vector p = frm->GetPos();
    p += dir;
    frm->SetPos(p);
    frm->Update();
    return true;
}


// FUNCTION: GAME 0x58cec0
void C_human::ProcessDeathPos(uint time)
{
    int length = death_length;
    if (length < 0 || death_time >= length)
        return;
    death_time += time;
    if (death_time > length)
        death_time = length;
    float t = (float)death_time / length;
    frame->SetPos(death_pos + death_move * t);
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
    AddHandModels(frame->FindChildFrame("r_hand", ENUMF_ALL), models);
    AddHandModels(frame->FindChildFrame("l_hand", ENUMF_ALL), models);
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
        I3D_frame *frm = model->FindChildFrame("clip", ENUMF_ALL);
        if (frm && frm->GetType() == FRAME_VISUAL)
            frm->SetOn(false);
    }
}


// Starts the weapon's effect, the animations' note of the reload or the shot: the moves TickWeaponEffect runs, the
// clip dropped as a physical thing, the revolver's shells, the new clip in the left hand
// FUNCTION: GAME 0x58d2c0
void C_human::StartWeaponEffect(int effect)
{
    effect_step = 0;
    switch (effect) {
    case WEAPON_EFFECT_OPEN:
    {
        S_GameItem *item;
        switch (inventory.GetItemInRightHand(&item)) {
        case GAMEITEM_COLT_DETECTIVE:
        case GAMEITEM_SW_MAGNUM:
        case GAMEITEM_SW_MP:
            effect_frame = weapon_model->FindChildFrame("cylinder", ENUMF_ALL);
            if (effect_frame) {
                effect_pos = 0.0f;
                effect_speed = .01f;
                effect_step = WEAPON_EFFECT_OPEN;
            }
            break;
        case GAMEITEM_COLT_1911:
        case GAMEITEM_THOMPSON:
            // The clip falls out, a physical thing, once a reload
            if (!item || !item->clip_out) {
                I3D_frame *clip = weapon_model->FindChildFrame("clip", ENUMF_ALL);
                if (clip && clip->GetType() == FRAME_VISUAL) {
                    clip->SetOn(false);
                    I3D_visual *vis = driver->CreateVisual(I3D_VISUAL_OBJECT);
                    if (vis) {
                        vis->Duplicate(clip);
                        vis->LinkTo(act_mission->GetScene()->GetPrimarySector(), 0);
                        vis->SetPos(clip->GetWorldPos());
                        vis->SetRot(clip->GetWorldRot());
                        vis->Update();
                        act_mission->GetScene()->SetFrameSector(vis);
                        vis->SetOn(true);
                        C_public_physics *phys = new C_public_physics;
                        if (phys->Init(vis)) {
                            S_THING_BOX thing_box;
                            phys->unknown_348 = 8;
                            thing_box.weight = .5f;
                            thing_box.unknown_10 = .45f;
                            thing_box.velocity = S_vector(0.0f, -1.0f, 0.0f);
                            phys->InitBox(&thing_box);
                            phys->unknown_6d = false;
                            act_mission->GetGame()->AddTemporaryActor(phys);
                            phys->unknown_c0 = 0x80000;
                            if (item)
                                item->clip_out = 1;
                        } else
                            phys->Release();
                        vis->Release();
                    }
                }
            }
            break;
        case GAMEITEM_SAWED_OFF:
            effect_frame = weapon_model->FindChildFrame("hlavne", ENUMF_ALL);
            if (effect_frame) {
                effect_pos = 0.0f;
                effect_speed = .01f;
                effect_step = WEAPON_EFFECT_OPEN;
            }
            break;
        }
        break;
    }
    case WEAPON_EFFECT_CLOSE:
        switch (inventory.GetItemInRightHand(NULL)) {
        case GAMEITEM_COLT_DETECTIVE:
        case GAMEITEM_SW_MAGNUM:
        case GAMEITEM_SW_MP:
            effect_frame = weapon_model->FindChildFrame("cylinder", ENUMF_ALL);
            if (effect_frame) {
                effect_pos = 1.0f;
                effect_speed = -.01f;
                effect_step = WEAPON_EFFECT_CLOSE;
            }
            break;
        case GAMEITEM_COLT_1911:
            effect_frame = weapon_model->FindChildFrame("zaver1", ENUMF_ALL);
            if (effect_frame) {
                effect_pos = 1.0f;
                effect_speed = -.05f;
                effect_step = WEAPON_EFFECT_CLOSE;
            }
            break;
        case GAMEITEM_THOMPSON:
        {
            I3D_frame *clip = weapon_model->FindChildFrame("clip", ENUMF_ALL);
            if (clip)
                clip->SetOn(true);
            break;
        }
        case GAMEITEM_SAWED_OFF:
            effect_frame = weapon_model->FindChildFrame("hlavne", ENUMF_ALL);
            if (effect_frame) {
                effect_pos = 1.0f;
                effect_speed = -.01f;
                effect_step = WEAPON_EFFECT_CLOSE;
            }
            break;
        }
        // The new clip is in the weapon
        if (hand_clip) {
            hand_clip->LinkTo(NULL, 0);
            hand_clip->Release();
            hand_clip = NULL;
        }
        break;
    case WEAPON_EFFECT_PUMP:
        switch (inventory.GetItemInRightHand(NULL)) {
        case GAMEITEM_SHOTGUN:
            effect_frame = weapon_model->FindChildFrame("foregrip", ENUMF_ALL);
            if (effect_frame) {
                effect_pos = 0.0f;
                effect_speed = .01f;
                effect_step = WEAPON_EFFECT_PUMP;
            }
        case GAMEITEM_SPRINGFIELD:
        case GAMEITEM_MOSIN_NAGANT:
            if (pump_sound)
                pump_sound->SetOn(true);
            break;
        }
        break;
    case WEAPON_EFFECT_SHELLS:
    {
        // The six shells out of the cylinder, once a reload
        S_GameItem *item;
        uint id = inventory.GetItemInRightHand(&item);
        if (id >= GAMEITEM_COLT_DETECTIVE && id <= GAMEITEM_SW_MP && (!item || !item->clip_out)) {
            if (item)
                item->clip_out = 1;
            effect_frame = weapon_model->FindChildFrame("cylinder", ENUMF_ALL);
            if (effect_frame) {
                S_vector dir = effect_frame->GetMatrix()(1);
                dir *= .1f;
                for (int i = 0; i < 6; i++)
                    act_mission->GetGame()->NewCartidge(effect_frame, dir, false, true);
            }
        }
        break;
    }
    case WEAPON_EFFECT_NEW_CLIP:
    {
        // A copy of the clip in the left hand
        uint id = inventory.GetItemInRightHand(NULL);
        if (id == GAMEITEM_COLT_1911 || id == GAMEITEM_THOMPSON) {
            I3D_frame *clip = weapon_model->FindChildFrame("clip", ENUMF_ALL);
            if (clip && clip->GetType() == FRAME_VISUAL) {
                clip->SetOn(false);
                hand_clip = driver->CreateVisual(I3D_VISUAL_OBJECT);
                if (hand_clip) {
                    hand_clip->Duplicate(clip);
                    hand_clip->LinkTo(left_hand, 0);
                    switch (id) {
                    case GAMEITEM_THOMPSON:
                        hand_clip->SetPos(.01f, .068f, .071f);
                        hand_clip->SetRot(S_quat(-.7f, .5f, -.509f, 1.185f));
                        break;
                    case GAMEITEM_COLT_1911:
                        hand_clip->SetPos(0.0f, 0.0f, 0.0f);
                        S_vector dir(0.0f, 0.0f, 1.0f);
                        hand_clip->SetDir(dir, 0.0f);
                        break;
                    }
                    hand_clip->Update();
                    hand_clip->SetOn(true);
                }
            }
        }
        break;
    }
    case WEAPON_EFFECT_SHOT:
        if (inventory.GetItemInRightHand(NULL) == GAMEITEM_COLT_1911) {
            effect_frame = weapon_model->FindChildFrame("zaver1", ENUMF_ALL);
            if (effect_frame) {
                effect_pos = 0.0f;
                effect_speed = .03f;
                effect_step = WEAPON_EFFECT_SHOT;
            }
        }
        break;
    case WEAPON_EFFECT_LAST_SHOT:
        if (inventory.GetItemInRightHand(NULL) == GAMEITEM_COLT_1911) {
            effect_frame = weapon_model->FindChildFrame("zaver1", ENUMF_ALL);
            if (effect_frame) {
                effect_pos = 0.0f;
                effect_speed = .05f;
                effect_step = WEAPON_EFFECT_OPEN;
            }
        }
        break;
    }
}


// The weapon effect's move, of effect_speed a ms: the cylinder or the barrels open to 1 and close to 0, the slide and
// the foregrip go back and forth from 0 over 1 to 2
// FUNCTION: GAME 0x58da70
void C_human::TickWeaponEffect(uint time)
{
    switch (effect_step) {
    case WEAPON_EFFECT_OPEN:
        effect_pos += time * effect_speed;
        if (effect_pos >= 1.0f) {
            effect_pos = 1.0f;
            effect_step = 0;
        }
        switch (inventory.GetItemInRightHand(NULL)) {
        case GAMEITEM_COLT_DETECTIVE:
        case GAMEITEM_SW_MAGNUM:
        case GAMEITEM_SW_MP:
            if (effect_frame) {
                S_quat q;
                float angle = effect_pos * -.6f;
                q.Make(S_vector(0.0f, 0.0f, 1.0f), angle);
                effect_frame->SetRot(q);
                effect_frame->Update();
            } else
                effect_step = 0;
            break;
        case GAMEITEM_COLT_1911:
            if (effect_frame) {
                effect_frame->SetPos(0.0f, 0.0f, effect_pos * -.04f);
                effect_frame->Update();
            } else
                effect_step = 0;
            break;
        case GAMEITEM_SAWED_OFF:
            if (effect_frame) {
                S_quat q;
                q.Make(S_vector(1.0f, 0.0f, 0.0f), effect_pos * -1.57f);
                effect_frame->SetRot(q);
                effect_frame->Update();
            } else
                effect_step = 0;
            break;
        default:
            effect_step = 0;
        }
        break;
    case WEAPON_EFFECT_CLOSE:
        effect_pos += time * effect_speed;
        if (effect_pos <= 0.0f) {
            effect_pos = 0.0f;
            effect_step = 0;
        }
        switch (inventory.GetItemInRightHand(NULL)) {
        case GAMEITEM_COLT_DETECTIVE:
        case GAMEITEM_SW_MAGNUM:
        case GAMEITEM_SW_MP:
            if (effect_frame) {
                S_quat q;
                float angle = effect_pos * -.6f;
                q.Make(S_vector(0.0f, 0.0f, 1.0f), angle);
                effect_frame->SetRot(q);
                effect_frame->Update();
            } else
                effect_step = 0;
            break;
        case GAMEITEM_COLT_1911:
            if (effect_frame) {
                effect_frame->SetPos(0.0f, 0.0f, effect_pos * -.04f);
                effect_frame->Update();
            } else
                effect_step = 0;
            break;
        case GAMEITEM_SAWED_OFF:
            if (effect_frame) {
                S_quat q;
                q.Make(S_vector(1.0f, 0.0f, 0.0f), effect_pos * -1.57f);
                effect_frame->SetRot(q);
                effect_frame->Update();
            } else
                effect_step = 0;
            break;
        default:
            effect_step = 0;
        }
        break;
    case WEAPON_EFFECT_PUMP:
        if (inventory.GetItemInRightHand(NULL) == GAMEITEM_SHOTGUN) {
            if (effect_frame) {
                effect_pos += time * effect_speed;
                if (effect_pos >= 2.0f) {
                    effect_pos = 2.0f;
                    effect_step = 0;
                }
                float pos = effect_pos;
                if (pos > 1.0f)
                    pos = 2.0f - pos;
                effect_frame->SetPos(0.0f, 0.0f, -pos * .1f);
                effect_frame->Update();
            } else
                effect_step = 0;
        } else
            effect_step = 0;
        break;
    case WEAPON_EFFECT_SHOT:
        if (inventory.GetItemInRightHand(NULL) == GAMEITEM_COLT_1911) {
            if (effect_frame) {
                float step = time * effect_speed;
                if (step > 1.0f)
                    step = 1.0f;
                effect_pos += step;
                if (effect_pos >= 2.0f) {
                    effect_pos = 2.0f;
                    effect_step = 0;
                }
                float pos = effect_pos;
                if (pos > 1.0f)
                    pos = 2.0f - pos;
                effect_frame->SetPos(0.0f, 0.0f, -pos * .04f);
                effect_frame->Update();
            } else
                effect_step = 0;
        } else
            effect_step = 0;
        break;
    }
}


// The weapon's moving parts put back where they rest, the fire's steps back to ready
// FUNCTION: GAME 0x58df60
void C_human::ResetWeaponEffects()
{
    fire_action = _fire_action_ready_;
    fire_step = 0;
    fire_time = 0;
    switch (inventory.GetItemInRightHand(NULL)) {
    case GAMEITEM_COLT_DETECTIVE:
    case GAMEITEM_SW_MAGNUM:
    case GAMEITEM_SW_MP:
        // STRING: GAME 0x636eb0
        effect_frame = weapon_model->FindChildFrame("cylinder", ENUMF_ALL);
        if (effect_frame) {
            S_quat q;
            q.Make(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
            effect_frame->SetRot(q);
            effect_frame->Update();
        }
        break;
    case GAMEITEM_COLT_1911:
        effect_frame = weapon_model->FindChildFrame("zaver1", ENUMF_ALL);
        if (effect_frame) {
            effect_frame->SetPos(0.0f, 0.0f, 0.0f);
            effect_frame->Update();
        }
        break;
    case GAMEITEM_SHOTGUN:
        // STRING: GAME 0x636e94
        effect_frame = weapon_model->FindChildFrame("foregrip", ENUMF_ALL);
        if (effect_frame) {
            effect_frame->SetPos(0.0f, 0.0f, 0.0f);
            effect_frame->Update();
        }
        // and on into the clip's, which the shotgun's model doesn't have
    case GAMEITEM_THOMPSON:
        effect_frame = weapon_model->FindChildFrame("clip", ENUMF_ALL);
        if (effect_frame)
            effect_frame->SetOn(true);
        break;
    case GAMEITEM_SAWED_OFF:
        // STRING: GAME 0x636ea8
        effect_frame = weapon_model->FindChildFrame("hlavne", ENUMF_ALL);
        if (effect_frame) {
            S_quat q;
            q.Make(S_vector(1.0f, 0.0f, 0.0f), 0.0f);
            effect_frame->SetRot(q);
            effect_frame->Update();
        }
        break;
    }
    effect_step = 0;
    effect_frame = NULL;
    pump_time = -1;
    if (hand_clip) {
        hand_clip->LinkTo(NULL, 0);
        hand_clip->Release();
        hand_clip = NULL;
    }
}


// FUNCTION: GAME 0x58e210
void C_human::FakeWeaponEffect(I3D_frame *frm, int i, bool b)
{
    if (i == GAMEITEM_COLT_1911)
    {
        // STRING: GAME 0x636ea0
        I3D_frame *z = frm->FindChildFrame("zaver1", ENUMF_ALL);
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

// Empty on the PC: its calls go to the empty function the linker folded it into (0x4aabe0)
void C_human::UniversalUse()
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

#pragma auto_inline(on)
