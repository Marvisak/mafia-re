// The in-engine cutscenes (PC only, the file's name is ours, after its class's; the PC's file is the one at
// 0x5983e0-0x5a075c, docs/game-files.md), written whole: the keys' interpolation (Ease, InterpolateCamera,
// InterpolateTarget), the actors' records (CreateRecords to GetActors), Tick and the actors it plays and records, the
// recording files (RecLoadFull, RecSaveFull) and the saved games' buffers, the animations' names and the animations
// (LoadTCK), Close, the object's initializer (0x59cc20) and inline constructor mid-file, the recording, the show, and
// the seagulls after them (C_seagull, from 0x59e7e0). It starts with a zero vector's initializer (0x5983e0, nothing
// reads the vector; as files 9's and 92's) and ends with the std::ctype<unsigned short>::id initializer every game file
// ends with (0x5a0730). Open: RecLoadFull (the entries' three induction pointers), RecSaveFull, HasTCK (its frame is
// 0x18 bytes larger in the original, and it calls two vector destructors out of line), the seagulls' GameInit (the
// ring walk keeps GetConn's bound tests in the original, and its push_back's calls), registers in Tick, AI and the
// two interpolations; GetActors' atexit helper isn't annotated (C_recorder.h).

#include "mission/C_recorder.h"
#include "I3D_cache.h"
#include "IGraph/IGraph.h"
#include "C_chunk.h"
#include "actors/C_animations.h"
#include "ai/pathweb.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "ui/g_dabing.h"
#include "ui/g_indicators.h"
#include "rw_data.h"

#include <float.h>
#include <stdlib.h>
#include <string.h>

#include "I3D_cache.h"
#include "stub.h"

// GLOBAL: GAME 0x647a58
int cutscene_play_ticks;

// GLOBAL: GAME 0x647a4c
std::vector<S_cutscene_placement> *saved_placements;

// The name is ours, chosen for the .bss order (an IShow plays videos and music alike)
// GLOBAL: GAME 0x647a50
IShow *movie;

// The cutscene's time at which its show starts (the name is ours): SetShowStart sets it, OpenShow clears it
// GLOBAL: GAME 0x647a54
int cutscene_show_start;

// The time between two keys eased: accelerated over a, at constant speed, then slowed over b (a and b scaled down to
// a sum of 1); the time itself where they add up to nothing
// FUNCTION: GAME 0x598410
float Ease(float u, float a, float b)
{
    float s = a + b;
    if (s < .001)
        return u;
    if (s > 1.0f)
    {
        a = a / s;
        b = b / s;
    }
    float k = 1.0f / (2.0f - a - b);
    if (u < a)
        return k / a * u * u;
    if (u < 1.0f - b)
        return k * (2.0f * u - a);
    u = 1.0f - u;
    float r = 1.0f - k / b * u * u;
    return r;
}

// The camera's key between two keys for the time (the names are ours): the time between their key times eased,
// the position on the spline (Hermite's basis functions, as C_actor::PlayStep), the roll and the field of view in
// a straight line by the first two of them. Its time, flags and ease stay as they were.
// FUNCTION: GAME 0x5984e0
void C_recorder::InterpolateCamera(S_cutscene_camera_key *out, const S_cutscene_camera_key *k1,
    const S_cutscene_camera_key *k2, uint time)
{
    float f = 0.0f;
    if (k2->key_time != k1->key_time)
        f = (float)(time - k1->key_time) / (k2->key_time - k1->key_time);
    float s = Ease(f, k1->ease_out, k2->ease_in);
    float s2 = s * s;
    float s3 = s2 * s;
    float h1 = 2.0f * s3 - 3.0f * s2 + 1.0f;
    float h2 = 3.0f * s2 - 2.0f * s3;
    float h3 = s3 - 2.0f * s2 + s;
    float h4 = s3 - s2;
    out->pos = k1->pos * h1;
    out->pos += k2->pos * h2;
    out->pos += k1->tangent_out * h3;
    out->pos += k2->tangent_in * h4;
    out->roll = k1->roll * h1;
    out->roll += k2->roll * h2;
    out->fov = k1->fov * h1;
    out->fov += k2->fov * h2;
}

// The same for a key of the camera's target, its position only
// FUNCTION: GAME 0x598680
void C_recorder::InterpolateTarget(S_cutscene_target_key *out, const S_cutscene_target_key *k1,
    const S_cutscene_target_key *k2, uint time)
{
    float f = 0.0f;
    if (k2->key_time != k1->key_time)
        f = (float)(time - k1->key_time) / (k2->key_time - k1->key_time);
    float s = Ease(f, k1->ease_out, k2->ease_in);
    float s2 = s * s;
    float s3 = s2 * s;
    float h1 = 2.0f * s3 - 3.0f * s2 + 1.0f;
    float h2 = 3.0f * s2 - 2.0f * s3;
    float h3 = s3 - 2.0f * s2 + s;
    float h4 = s3 - s2;
    out->pos = k1->pos * h1;
    out->pos += k2->pos * h2;
    out->pos += k1->tangent_out * h3;
    out->pos += k2->tangent_in * h4;
}

// The counts of the records CreateRecords has named, by their names' prefixes (the names are ours): zero-initialized,
// so they follow the file's hashed globals in this order
// GLOBAL: GAME 0x647a5c
int records_act = 0;
// GLOBAL: GAME 0x647a60
int records_box = 0;
// GLOBAL: GAME 0x647a64
int records_car = 0;
// GLOBAL: GAME 0x647a68
int records_hum = 0;
// GLOBAL: GAME 0x647a6c
int records_drp = 0;
// GLOBAL: GAME 0x647a70
int records_unk = 0;

// An actor's records: none recorded yet, the sizes of the four types (a bare S_cutscene_record, then the three
// given), named by the actor's type and a count of that type's names (the names are ours)
// FUNCTION: GAME 0x5987f0
C_cutscene_records *C_recorder::CreateRecords(int size1, int size2, int size3, int type)
{
    C_cutscene_records *records = new C_cutscene_records;
    if (!records)
        return NULL;
    records->record_sizes[0] = sizeof(S_cutscene_record);
    records->record_sizes[1] = size1;
    records->record_sizes[2] = size2;
    records->record_sizes[3] = size3;
    records->cur = NULL;
    records->next = NULL;
    records->unknown_8 = NULL;
    records->unknown_c = NULL;
    records->unknown_10 = NULL;
    records->last_key_time = RECORDS_NO_KEY;
    records->first_block = NULL;
    records->last_block = NULL;
    switch (type)
    {
    case ACTOR_ACTOR:
        // STRING: GAME 0x636f20
        sprintf(records->name, "act%d", records_act++);
        break;
    case ACTOR_DYN_OBJECT:
        // STRING: GAME 0x636f18
        sprintf(records->name, "box%d", records_box++);
        break;
    case ACTOR_CAR:
        // STRING: GAME 0x636f10
        sprintf(records->name, "car%d", records_car++);
        break;
    case ACTOR_PLAYER:
    case ACTOR_HUMAN:
    case ACTOR_ENTITY:
        // STRING: GAME 0x636f08
        sprintf(records->name, "hum%d", records_hum++);
        break;
    case ACTOR_DROP_OUT:
        // STRING: GAME 0x636f00
        sprintf(records->name, "drp%d", records_drp++);
        break;
    default:
        // STRING: GAME 0x636ef8
        sprintf(records->name, "unk%d", records_unk++);
        break;
    }
    return records;
}

// Starts the records over: nothing recorded, no key yet (nothing for NULL)
// FUNCTION: GAME 0x598970
void C_recorder::ResetRecords(C_cutscene_records *records)
{
    if (records)
    {
        records->cur = NULL;
        records->next = NULL;
        records->unknown_8 = NULL;
        records->unknown_c = NULL;
        records->unknown_10 = NULL;
        records->last_key_time = RECORDS_NO_KEY;
        records->first_block = NULL;
        records->last_block = NULL;
    }
}

// Each case walks the second list, then the first one
// FUNCTION: GAME 0x5989a0
void C_recorder::SetState(int new_state)
{
    uint i;
    prev_state = state;
    state = new_state;
    if (prev_state == state)
        return;
    switch (state)
    {
    case CUTSCENE_STATE_0:
        for (i = 0; i < actors_2.size(); i++)
        {
            actors_2[i]->act_flags &= ~ACTF_CUTSCENE;
            actors_2[i]->SetCutsceneRecord(false);
            actors_2[i]->SetCutscenePlay(false);
        }
        for (i = 0; i < actors.size(); i++)
        {
            actors[i]->act_flags &= ~ACTF_CUTSCENE;
            actors[i]->SetCutscenePlay(false);
        }
        unknown_128 = false;
        break;
    case CUTSCENE_STATE_1:
        for (i = 0; i < actors_2.size(); i++)
        {
            actors_2[i]->act_flags |= ACTF_CUTSCENE;
            actors_2[i]->SetCutsceneRecord(false);
            actors_2[i]->SetCutscenePlay(true);
        }
        for (i = 0; i < actors.size(); i++)
        {
            actors[i]->act_flags |= ACTF_CUTSCENE;
            actors[i]->SetCutscenePlay(true);
        }
        break;
    case CUTSCENE_STATE_2:
        for (i = 0; i < actors_2.size(); i++)
        {
            actors_2[i]->act_flags &= ~ACTF_CUTSCENE;
            actors_2[i]->SetCutscenePlay(false);
            actors_2[i]->SetCutsceneRecord(true);
        }
        for (i = 0; i < actors.size(); i++)
        {
            actors[i]->act_flags |= ACTF_CUTSCENE;
            actors[i]->SetCutscenePlay(true);
        }
        break;
    }
}

// FUNCTION: GAME 0x598b70
int C_recorder::GetState()
{
    return state;
}

// The cutscene's time of a time (the name is ours): the time itself. SetTime calls it out of line, and the linker folded
// it with the other functions of its code (mov eax, ecx; ret) at 0x605560, G_LoadSaveClass::ProfileGetList's copy, so
// it has no address of its own and no marker.
int CutsceneTime(int time)
{
    return time;
}

// Jumps to a time: cur_time, and scaled_time and time_speed by the speed key it falls into (by cur_time itself
// without the speed keys), the subtitles past it skipped; in CUTSCENE_STATE_0 and CUTSCENE_STATE_3 every actor placed
// at the time, and those that can't place themselves (C_actor::PlaceAtTime's false) played there from their records'
// start
// FUNCTION: GAME 0x598b80
void C_recorder::SetTime(int time)
{
    cur_time = CutsceneTime(time);
    if (!speed_keys_on)
    {
        scaled_time = cur_time;
        time_speed = 1.0f;
    }
    else
    {
        if (speed_keys.num_keys)
        {
            speed_keys.cur_key = 0;
            while (speed_keys.cur_key < speed_keys.num_keys - 1 && speed_keys.keys[speed_keys.cur_key + 1].time < cur_time)
                speed_keys.cur_key++;
            S_cutscene_speed_key *k = &speed_keys.keys[speed_keys.cur_key];
            scaled_time = (int)((cur_time - k->time) * k->speed) + k->scaled_time;
            time_speed = k->speed;
        }
        else
        {
            scaled_time = cur_time;
            time_speed = 1.0f;
        }
        if (subtitles.next < subtitles.num_items)
        {
            while (scaled_time >= ((S_cutscene_subtitle *)subtitles.items)[subtitles.next].time)
                subtitles.next++;
        }
    }
    uint i;
    switch (state)
    {
    case CUTSCENE_STATE_0:
        for (i = 0; i < actors.size(); i++)
        {
            if (!actors[i]->PlaceAtTime(scaled_time))
                ReplayActor(actors[i], 0);
        }
        for (i = 0; i < actors_2.size(); i++)
        {
            if (!actors_2[i]->PlaceAtTime(scaled_time))
                ReplayActor(actors_2[i], 0);
        }
        break;
    case CUTSCENE_STATE_3:
        for (i = 0; i < actors.size(); i++)
        {
            if (!actors[i]->PlaceAtTime(scaled_time))
                ReplayActor(actors[i], 0);
        }
        for (i = 0; i < actors_2.size(); i++)
        {
            if (!actors_2[i]->PlaceAtTime(scaled_time))
                ReplayActor(actors_2[i], 0);
        }
        break;
    }
}

// FUNCTION: GAME 0x598d90
int C_recorder::GetCurTime()
{
    return cur_time;
}

// FUNCTION: GAME 0x598da0
int C_recorder::GetScaledTime()
{
    return scaled_time;
}

// Adds the actor to the cutscene's played actors, with records of its own (CreateRecords', named by its type) and an
// entry of the recording, unless it is in a list already: the actor, NULL where its records can't be made
// FUNCTION: GAME 0x598db0
C_actor *C_recorder::AddActor(C_actor *actor, int size1, int size2, int size3)
{
    if (!HasActor(actor))
    {
        actor->records = CreateRecords(size1, size2, size3, actor->GetType());
        if (!actor->records)
            return NULL;
        actor->records->actor = actor;
        actor->act_flags |= ACTF_CUTSCENE;
        actors.push_back(actor);
        recording.AddActor(actor);
    }
    return actor;
}

// Adds the actor to the cutscene's played actors without records, unless it is in a list already (the name is ours).
// GetActFlags is the inline call still to come that leaves the push_back's insert out of line, as the original calls it.
// FUNCTION: GAME 0x598fb0
C_actor *C_recorder::AddPlayedActor(C_actor *actor)
{
    if (!HasActor(actor))
    {
        actors.push_back(actor);
        actor->records = NULL;
        actor->act_flags = actor->GetActFlags() | ACTF_CUTSCENE;
        actor->SetCutscenePlay(true);
        actor->SetCutsceneRecord(false);
    }
    return actor;
}

// FUNCTION: GAME 0x599010
bool C_recorder::RemoveActor(C_actor *actor, bool unknown)
{
    recording.RemoveActor(actor, unknown);
    uint i;
    for (i = 0; i < actors.size(); i++)
    {
        if (actor == actors[i])
        {
            actor->act_flags &= ~ACTF_CUTSCENE;
            actors.erase(actors.begin() + i);
            return true;
        }
    }
    for (i = 0; i < actors_2.size(); i++)
    {
        if (actor == actors_2[i])
        {
            actors_2.erase(actors_2.begin() + i);
            return true;
        }
    }
    return false;
}

// FUNCTION: GAME 0x5990e0
void C_recorder::RemoveActors()
{
    uint i;
    if (records_data == (uchar *)1)
    {
        for (i = 0; i < actors.size(); i++)
            RemoveActor(actors[i], true);
        for (i = 0; i < actors_2.size(); i++)
            RemoveActor(actors_2[i], true);
    }
    else
    {
        for (i = 0; i < actors.size(); i++)
        {
            delete actors[i]->records;
            actors[i]->act_flags &= ~ACTF_CUTSCENE;
            actors[i]->records = NULL;
        }
        for (i = 0; i < actors_2.size(); i++)
        {
            delete actors_2[i]->records;
            actors_2[i]->act_flags &= ~ACTF_CUTSCENE;
            actors_2[i]->records = NULL;
        }
        recording.Clear();
    }
}

// Whether the actor is in either list
// FUNCTION: GAME 0x5991e0
bool C_recorder::HasActor(C_actor *actor)
{
    uint i;
    for (i = 0; i < actors.size(); i++)
    {
        if (actor == actors[i])
            return true;
    }
    for (i = 0; i < actors_2.size(); i++)
    {
        if (actor == actors_2[i])
            return true;
    }
    return false;
}

// Moves an actor of the cutscene from the played actors to the recorded ones (actors_2), or back (the name is ours)
// FUNCTION: GAME 0x599240
void C_recorder::SetActorRecord(C_actor *actor, bool record)
{
    uint i;
    switch (record)
    {
    case true:
        if (HasActor(actor))
        {
            actor->SetCutsceneRecord(true);
            actor->act_flags &= ~ACTF_CUTSCENE;
            for (i = 0; i < actors.size(); i++)
            {
                if (actor == actors[i])
                {
                    actors.erase(actors.begin() + i);
                    actors_2.push_back(actor);
                    break;
                }
            }
        }
        break;
    case false:
        if (HasActor(actor))
        {
            actor->SetCutsceneRecord(false);
            actor->act_flags |= ACTF_CUTSCENE;
            for (i = 0; i < actors_2.size(); i++)
            {
                if (actor == actors_2[i])
                {
                    actors_2.erase(actors_2.begin() + i);
                    actors.push_back(actor);
                    break;
                }
            }
        }
        break;
    }
}

// Whether the actor is among the recorded actors (actors_2), and not among the played ones
// FUNCTION: GAME 0x599370
bool C_recorder::HasActor2Only(C_actor *actor)
{
    uint i;
    for (i = 0; i < actors.size(); i++)
    {
        if (actor == actors[i])
            return false;
    }
    for (i = 0; i < actors_2.size(); i++)
    {
        if (actor == actors_2[i])
            return true;
    }
    return false;
}

// Both lists' actors in one vector, a function static the next call fills again (the name is ours)
// FUNCTION: GAME 0x5993d0
std::vector<C_actor *> *C_recorder::GetActors()
{
    static std::vector<C_actor *> all_actors;
    all_actors.clear();
    uint i;
    for (i = 0; i < actors.size(); i++)
        all_actors.push_back(actors[i]);
    for (i = 0; i < actors_2.size(); i++)
        all_actors.push_back(actors_2[i]);
    return &all_actors;
}

// The cutscene's tick: the camera from its keys and the target's (a key of CUTSCENE_END_FOV whose time has come stops
// the cutscene), the overlay, the global programs and the sound frames whose times have come, the actors played or
// recorded by the state, then the next item of the dubbing, the subtitles and the morphs, the show and the dubbing's
// frames. The state's cases are in the order of the original's jump table targets. The overlay's last key index is a
// local of the overlay's block (in the branch that uses it, every register of the function comes out otherwise).
// FUNCTION: GAME 0x599610
void C_recorder::Tick(uint time)
{
    if (state == CUTSCENE_STOPPED)
        return;
    if (cutscene_play_ticks)
        cutscene_play_ticks--;
    if (state != CUTSCENE_STATE_0 && state != CUTSCENE_STATE_2 && camera.num_keys && camera.num_targets)
    {
        const S_cutscene_camera_key *key;
        S_cutscene_camera_key key_at;
        uint i;
        unknown_128 = false;
        if (camera.keys[0].time >= cur_time)
        {
            key = &camera.keys[0];
            camera.cur_key = 0;
            if (key->fov == CUTSCENE_END_FOV)
            {
                SetState(CUTSCENE_STATE_0);
                return;
            }
        }
        else if (camera.keys[camera.num_keys - 1].time <= cur_time)
        {
            key = &camera.keys[camera.num_keys - 1];
            camera.cur_key = camera.num_keys - 1;
            if (key->fov == CUTSCENE_END_FOV)
            {
                SetState(CUTSCENE_STATE_0);
                return;
            }
        }
        else
        {
            uint next;
            for (i = camera.cur_key; i < camera.num_keys; i++)
            {
                if (camera.keys[i].fov == CUTSCENE_END_FOV && camera.keys[i].time <= cur_time)
                {
                    SetState(CUTSCENE_STATE_0);
                    return;
                }
                if (camera.keys[i].flags & (CUTKEYF_UNKNOWN_1 | CUTKEYF_UNKNOWN_2) && camera.keys[i].time <= cur_time)
                {
                    camera.cur_key = i;
                    next = i;
                }
            }
            for (i = camera.cur_key; i < camera.num_keys; i++)
            {
                if (camera.keys[i].flags & (CUTKEYF_UNKNOWN_1 | CUTKEYF_UNKNOWN_2) && camera.keys[i].time > cur_time)
                {
                    next = i;
                    break;
                }
            }
            uint from = camera.keys[camera.cur_key].flags & CUTKEYF_UNKNOWN_2 ? camera.cur_key + 2 : camera.cur_key;
            uint to = camera.keys[next].flags & CUTKEYF_UNKNOWN_2 ? next + 1 : next;
            key = &key_at;
            InterpolateCamera(&key_at, &camera.keys[from], &camera.keys[to], cur_time);
        }
        const S_cutscene_target_key *target;
        S_cutscene_target_key target_at;
        if (camera.targets[0].time >= cur_time)
        {
            camera.cur_target = 0;
            target = &camera.targets[0];
        }
        else if (camera.targets[camera.num_targets - 1].time <= cur_time)
        {
            target = &camera.targets[camera.num_targets - 1];
            camera.cur_target = camera.num_targets - 1;
        }
        else
        {
            uint next;
            for (i = camera.cur_target; i < camera.num_targets; i++)
            {
                if (camera.targets[i].flags & (CUTKEYF_UNKNOWN_1 | CUTKEYF_UNKNOWN_2) &&
                    camera.targets[i].time <= cur_time)
                {
                    camera.cur_target = i;
                    next = i;
                }
            }
            for (i = camera.cur_target; i < camera.num_targets; i++)
            {
                if (camera.targets[i].flags & (CUTKEYF_UNKNOWN_1 | CUTKEYF_UNKNOWN_2) &&
                    camera.targets[i].time > cur_time)
                {
                    next = i;
                    break;
                }
            }
            uint from = camera.targets[camera.cur_target].flags & CUTKEYF_UNKNOWN_2 ? camera.cur_target + 2 :
                camera.cur_target;
            uint to = camera.targets[next].flags & CUTKEYF_UNKNOWN_2 ? next + 1 : next;
            target = &target_at;
            InterpolateTarget(&target_at, &camera.targets[from], &camera.targets[to], cur_time);
        }
        if (!unknown_128)
        {
            I3D_camera *cam = act_mission->scene->GetActiveCamera();
            S_vector dir;
            dir = target->pos - key->pos;
            dir.Normalize();
            cam->SetPos(key->pos);
            cam->SetDir(dir, key->roll);
            mafiaIndicators.SetCameraFOV(cam, key->fov);
            cam->Update();
            act_mission->scene->SetFrameSector(cam);
        }
    }
    if (overlay.num_keys)
    {
        float fade = 0.0f;
        uint color = 0;
        const char *texture = NULL;
        bool opaque = false;
        S_cutscene_overlay_key *keys = overlay.keys;
        uint first = keys[0].flags;
        uint last = overlay.num_keys - 1;
        if (cur_time > (first & OVERLAYF_TIME))
        {
            while (overlay.cur_key < last &&
                (keys[overlay.cur_key + 1].flags & OVERLAYF_TIME) <= cur_time)
                overlay.cur_key++;
            if (overlay.cur_key < last)
            {
                S_cutscene_overlay_key *k = &keys[overlay.cur_key];
                uint flags = k->flags;
                if (flags & OVERLAYF_FULL)
                {
                    fade = 1.0f;
                    color = k->color;
                    if (flags & OVERLAYF_TEXTURE)
                        texture = k->texture;
                    opaque = flags & OVERLAYF_OPAQUE ? true : false;
                }
                else
                {
                    float in = 0.0f;
                    float out = 0.0f;
                    if (flags & OVERLAYF_FADE_OUT && k->fade_out > 0.0f)
                        out = 1.0f - (cur_time - (flags & OVERLAYF_TIME)) / k->fade_out;
                    uint next_flags = keys[overlay.cur_key + 1].flags;
                    if (next_flags & OVERLAYF_FADE_IN && k[1].fade_in > 0.0f)
                        in = 1.0f - ((next_flags & OVERLAYF_TIME) - cur_time) / k[1].fade_in;
                    if (in > out)
                    {
                        fade = in;
                        color = k[1].color;
                        if (next_flags & OVERLAYF_TEXTURE)
                            texture = k[1].texture;
                    }
                    else if (out > 0.0f)
                    {
                        fade = out;
                        color = k->color;
                        if (k->flags & OVERLAYF_TEXTURE)
                            texture = k->texture;
                    }
                    if (k->flags & OVERLAYF_OPAQUE)
                    {
                        opaque = true;
                        if (k->flags & OVERLAYF_TEXTURE)
                            texture = k->texture;
                        else
                            texture = NULL;
                    }
                }
            }
            else
            {
                uint flags = keys[overlay.cur_key].flags;
                S_cutscene_overlay_key *k = &keys[overlay.cur_key];
                if (!(flags & OVERLAYF_FULL) && cur_time < (flags & OVERLAYF_TIME))
                {
                    if (flags & OVERLAYF_FADE_OUT && k->fade_out > 0.0f)
                    {
                        color = k->color;
                        fade = 1.0f - (cur_time - (flags & OVERLAYF_TIME)) / k->fade_out;
                        if (flags & OVERLAYF_TEXTURE)
                            texture = k->texture;
                        opaque = flags & OVERLAYF_OPAQUE ? true : false;
                    }
                }
                else
                {
                    fade = 1.0f;
                    color = k->color;
                    if (flags & OVERLAYF_TEXTURE)
                        texture = k->texture;
                    opaque = flags & OVERLAYF_OPAQUE ? true : false;
                }
            }
        }
        else if (first & OVERLAYF_FADE_IN && keys[0].fade_in > 0.0f)
        {
            color = keys[0].color;
            fade = 1.0f - ((first & OVERLAYF_TIME) - cur_time) / keys[0].fade_in;
            if (first & OVERLAYF_TEXTURE)
                texture = keys[0].texture;
            opaque = first & OVERLAYF_OPAQUE ? true : false;
        }
        if (fade < 0.0f)
            fade = 0.0f;
        mafiaIndicators.SetOverlayTexture(texture, opaque, false);
        mafiaIndicators.SetOverlayFade(fade, color);
    }
    else
    {
        mafiaIndicators.SetOverlayTexture(NULL, false, false);
        mafiaIndicators.SetOverlayFade(0.0f, 0);
    }
    if (programs.num_items)
    {
        while (programs.last < programs.num_items - 1 &&
            ((S_cutscene_program *)programs.items)[programs.last + 1].time < cur_time)
        {
            programs.last++;
            act_mission->GlobalProgramRun(((S_cutscene_program *)programs.items)[programs.last].name);
        }
    }
    if (sounds.num_items)
    {
        while (sounds.last < sounds.num_items - 1 &&
            ((S_cutscene_sound *)sounds.items)[sounds.last + 1].time < cur_time)
        {
            sounds.last++;
            I3D_frame *frm = act_mission->scene->FindFrame(((S_cutscene_sound *)sounds.items)[sounds.last].frame_name,
                ENUMF_SOUND);
            if (frm)
            {
                switch (((S_cutscene_sound *)sounds.items)[sounds.last].mode)
                {
                case CUTSCENE_SOUND_OFF:
                    frm->SetOn(false);
                    break;
                case CUTSCENE_SOUND_ON:
                    frm->SetOn(true);
                    break;
                }
            }
        }
    }
    uint i;
    switch (state)
    {
    case CUTSCENE_STATE_1:
        if (prev_state != CUTSCENE_STATE_1)
        {
            for (i = 0; i < actors_2.size(); i++)
                actors_2[i]->SetCutscenePlay(true);
            for (i = 0; i < actors.size(); i++)
                actors[i]->SetCutscenePlay(true);
            for (i = 0; i < actors.size(); i++)
                ReplayActor(actors[i], scaled_time + time);
            for (i = 0; i < actors_2.size(); i++)
                ReplayActor(actors_2[i], scaled_time + time);
        }
        else
        {
            for (i = 0; i < actors.size(); i++)
            {
                actors[i]->UpdateCutsceneFlags();
                PlayActor(actors[i], time);
            }
            for (i = 0; i < actors_2.size(); i++)
            {
                actors_2[i]->UpdateCutsceneFlags();
                PlayActor(actors_2[i], time);
            }
        }
        break;
    case CUTSCENE_STATE_3:
        if (prev_state != CUTSCENE_STATE_3)
        {
            for (i = 0; i < actors_2.size(); i++)
                actors_2[i]->SetCutscenePlay(true);
            for (i = 0; i < actors.size(); i++)
                actors[i]->SetCutscenePlay(true);
        }
        for (i = 0; i < actors.size(); i++)
        {
            actors[i]->UpdateCutsceneFlags();
            PlayActor(actors[i], time);
        }
        for (i = 0; i < actors_2.size(); i++)
        {
            actors_2[i]->UpdateCutsceneFlags();
            PlayActor(actors_2[i], time);
        }
        break;
    case CUTSCENE_STATE_2:
        if (prev_state != CUTSCENE_STATE_2)
        {
            for (i = 0; i < actors_2.size(); i++)
                actors_2[i]->SetCutscenePlay(false);
            for (i = 0; i < actors.size(); i++)
                actors[i]->SetCutscenePlay(true);
        }
        for (i = 0; i < actors.size(); i++)
        {
            actors[i]->UpdateCutsceneFlags();
            PlayActor(actors[i], time);
        }
        for (i = 0; i < actors_2.size(); i++)
        {
            actors_2[i]->UpdateCutsceneFlags();
            RecordActor(actors_2[i], time);
        }
        break;
    case CUTSCENE_STATE_0:
        prev_state = state;
        return;
    }
    prev_state = state;
    if (dabing.next < dabing.num_items)
    {
        S_cutscene_dabing *item = &((S_cutscene_dabing *)dabing.items)[dabing.next];
        if (scaled_time >= item->time)
        {
            DabingCutscene(item);
            dabing.next++;
        }
    }
    if (subtitles.next < subtitles.num_items)
    {
        S_cutscene_subtitle *item = &((S_cutscene_subtitle *)subtitles.items)[subtitles.next];
        if (scaled_time >= item->time)
        {
            mafiaIndicators.SubtitlesAddCutscene(item);
            subtitles.next++;
        }
    }
    if (morphs.next < morphs.num_items)
    {
        S_cutscene_morph *item = &((S_cutscene_morph *)morphs.items)[morphs.next];
        if (scaled_time >= item->time)
        {
            DabingCutsceneMorph(item, scaled_time);
            morphs.next++;
        }
    }
    TickShow(cur_time);
    DabingUpdateFrames();
}

// Starts the actor's records over (AttachActor, ReplayActor): the current and the next record their first one, none
// without records, and not ended
// FUNCTION: GAME 0x59a240
void C_recorder::RewindActor(C_actor *actor)
{
    if (actor && actor->records)
    {
        if (actor->records->first_block)
            actor->records->cur = actor->records->unknown_8 = actor->records->unknown_10 =
                actor->records->first_block + 8;
        else
            actor->records->cur = actor->records->unknown_8 = actor->records->unknown_10 = NULL;
        actor->records->next = actor->records->cur;
        actor->records->ended = 0;
    }
}

// Plays the actor's records up to the cutscene's time (Tick's, the name is ours): each record whose time has come
// becomes the current one (PlayRecord), the next one after it, then the step (PlayStep); the records end once the time
// passes their last key
// FUNCTION: GAME 0x59a2a0
void C_recorder::PlayActor(C_actor *actor, uint time)
{
    if (!actor->records->first_block || !actor->records->cur || !actor->records->next)
        return;
    C_cutscene_records *rec = actor->records;
    if (!rec->ended)
    {
        while (scaled_time >= ((S_cutscene_record *)rec->next)->time)
        {
            rec->cur = rec->next;
            if (((S_cutscene_record *)rec->cur)->time < rec->last_key_time)
                rec->next = rec->cur + rec->record_sizes[((S_cutscene_record *)rec->cur)->type];
            if (((S_cutscene_record *)rec->cur)->type & 2)
            {
                rec->unknown_8 = rec->cur;
                if (((S_cutscene_record *)rec->cur)->type & 3)
                    rec->unknown_10 = rec->cur;
            }
            actor->PlayRecord(scaled_time);
            if (((S_cutscene_record *)rec->cur)->time >= rec->last_key_time)
                break;
        }
    }
    actor->PlayStep(time);
    if (scaled_time >= rec->last_key_time)
        rec->ended = 1;
}

// The actor's records from their start, played up to the cutscene's time (SetTime's and Tick's, the name is ours)
// FUNCTION: GAME 0x59a360
void C_recorder::ReplayActor(C_actor *actor, uint time)
{
    RewindActor(actor);
    PlayActor(actor, time);
}

// Records a key of the actor at the cutscene's time where it makes one (Tick's, the name is ours): the key's time is
// the last one, the record the current one
// FUNCTION: GAME 0x59a380
void C_recorder::RecordActor(C_actor *actor, uint time)
{
    if (actor->RecordKey(scaled_time) != -1)
    {
        actor->records->last_key_time = scaled_time;
        actor->records->cur = actor->records->next;
    }
}

// RECLOADFULL: loads a recording file (CUTSCENE_FILE_MAGIC, C_program's records\<name>.rep), false where it can't
// be opened or isn't one. The cutscene is cleared first; the file gives the animations' names, the actors' records
// (one buffer, which the recording keeps), each given to every mission actor whose frame has the entry's frame name, or
// to none for "<none>", the two tracks of keys, the speed keys, the overlay's keys, the programs and the sounds, then
// the other tracks (LoadTracks). The cutscene then starts (CUTSCENE_STATE_1) and keeps the placements of its humans,
// entities (their poses off) and doors for Close; the camera's range, the HUD in cutscene mode.
// FUNCTION: GAME 0x59a3c0
bool C_recorder::RecLoadFull(const char *name)
{
    std::vector<C_actor *> &mission_actors = act_mission->GetActors();
    S_cutscene_placement placement;
    int handle = dtaOpen(name, false);
    if (handle < 0)
        return false;
    S_cutscene_file_header header;
    dtaRead(handle, &header, sizeof(header));
    if (header.magic != CUTSCENE_FILE_MAGIC)
    {
        dtaClose(handle);
        return false;
    }
    recording.Clear();
    RemoveActors();
    anim_names.LoadDta(handle);
    uint lists_size = header.lists_size;
    uint tracks_size = header.tracks_size;
    uint keys_size = header.num_keys * sizeof(S_cutscene_camera_key) +
        header.num_targets * sizeof(S_cutscene_target_key);
    S_cutscene_file_actor *entries = new S_cutscene_file_actor[header.num_actors];
    records_data = new uchar[header.data_size];
    keys_data = keys_size ? new uchar[keys_size] : NULL;
    lists_data = lists_size ? new uchar[lists_size] : NULL;
    tracks_data = tracks_size ? new uchar[tracks_size] : NULL;
    dtaRead(handle, entries, header.num_actors * sizeof(S_cutscene_file_actor));
    dtaRead(handle, records_data, header.data_size);
    if (keys_size)
        dtaRead(handle, keys_data, keys_size);
    if (lists_size)
        dtaRead(handle, lists_data, lists_size);
    if (tracks_size)
        dtaRead(handle, tracks_data, tracks_size);
    dtaClose(handle);
    uchar *data = records_data;
    bool shared = recording.SetData(data) ? true : false;
    for (uint i = 0; i < header.num_actors; i++)
    {
        S_cutscene_file_actor &e = entries[i];
        // STRING: GAME 0x636148
        if (!strcmp(e.frame_name, "<none>"))
        {
            C_cutscene_records *rec = new C_cutscene_records;
            rec->ended = e.ended;
            rec->last_key_time = e.last_key_time;
            for (int j = 0; j < 4; j++)
                rec->record_sizes[j] = e.record_sizes[j];
            rec->cur = rec->next = rec->unknown_8 = rec->unknown_c = rec->unknown_10 = NULL;
            rec->first_block = data;
            rec->last_block = data;
            strcpy(rec->name, e.name);
            recording.AddEntry(true, shared, rec->name, NULL, rec, e.type);
            data += e.size;
        }
        else
        {
            for (uint j = 0; j < mission_actors.size(); j++)
            {
                C_actor *act = mission_actors[j];
                if (!strcmp(act->frame->GetName(), e.frame_name))
                {
                    C_cutscene_records *rec = new C_cutscene_records;
                    rec->ended = e.ended;
                    rec->last_key_time = e.last_key_time;
                    for (int k = 0; k < 4; k++)
                        rec->record_sizes[k] = e.record_sizes[k];
                    rec->cur = rec->next = rec->unknown_8 = rec->unknown_c = rec->unknown_10 = NULL;
                    rec->first_block = data;
                    rec->last_block = data;
                    strcpy(rec->name, e.name);
                    recording.AddEntry(true, shared, rec->name, act, rec, e.type);
                }
            }
            data += e.size;
        }
    }
    if (records_data == (uchar *)1)
    {
        if (camera.num_keys && camera.keys)
            delete[] camera.keys;
        if (camera.num_targets && camera.targets)
            delete[] camera.targets;
    }
    camera.num_keys = header.num_keys;
    camera.num_targets = header.num_targets;
    camera.targets = (S_cutscene_target_key *)(keys_data + header.num_keys * sizeof(S_cutscene_camera_key));
    camera.cur_key = 0;
    camera.cur_target = 0;
    camera.keys = (S_cutscene_camera_key *)keys_data;
    if (lists_size)
    {
        uchar *p = lists_data;
        uint pos = 0x10;
        uint speed_keys_size = ((uint *)p)[0];
        uint overlay_size = ((uint *)p)[1];
        uint programs_size = ((uint *)p)[2];
        uint sounds_size = ((uint *)p)[3];
        speed_keys.num_keys = speed_keys_size / sizeof(S_cutscene_speed_key);
        overlay.num_keys = overlay_size / sizeof(S_cutscene_overlay_key);
        programs.num_items = programs_size / sizeof(S_cutscene_program);
        sounds.num_items = sounds_size / sizeof(S_cutscene_sound);
        speed_keys.keys = NULL;
        overlay.keys = NULL;
        programs.items = NULL;
        sounds.items = NULL;
        if (speed_keys_size)
        {
            speed_keys.keys = (S_cutscene_speed_key *)(p + pos);
            pos += speed_keys_size;
        }
        if (overlay_size)
        {
            overlay.keys = (S_cutscene_overlay_key *)(p + pos);
            pos += overlay_size;
        }
        if (programs_size)
        {
            programs.items = p + pos;
            pos += programs_size;
        }
        if (sounds_size)
        {
            sounds.items = p + pos;
            pos += sounds_size;
        }
    }
    programs.last = -1;
    sounds.last = -1;
    speed_keys.cur_key = 0;
    overlay.cur_key = 0;
    LoadTracks(tracks_data);
    if (entries)
        delete[] entries;
    CreateAnims();
    cutscene_play_ticks = 5;
    scaled_time = 0;
    cur_time = 0;
    SetState(CUTSCENE_STATE_0);
    SetState(CUTSCENE_STATE_1);
    saved_placements = new std::vector<S_cutscene_placement>;
    for (C_actor **p = actors.begin(); p < actors.end(); p++)
    {
        switch ((*p)->type)
        {
        case ACTOR_DOOR:
            placement.actor = *p;
            placement.frame = (*p)->frame;
            placement.pos = placement.frame->GetPos();
            placement.rot = placement.frame->GetRot();
            saved_placements->push_back(placement);
            break;
        case ACTOR_HUMAN:
        case ACTOR_ENTITY:
            placement.actor = *p;
            placement.frame = (*p)->frame;
            saved_placements->push_back(placement);
            ((I3D_model *)(*p)->GetFrame())->SetPoseBlend(0.0f);
            break;
        }
    }
    act_mission->GetGame()->EntityBlock(true, true);
    act_mission->GetScene()->GetActiveCamera()->SetRange(.1f, 300.0f);
    mafiaIndicators.SubtitlesClear();
    mafiaIndicators.AddFlags(INDF_CUTSCENE);
    SetWBuffer(false);
    mafiaIndicators.ParheliaSetCenter(true);
    return true;
}

// Saves the cutscene as a recording file RecLoadFull loads (the name is ours): the header, the animations' names, an
// entry for each of the recording's actors, their records one after another, the camera's keys and its target's, the speed
// keys, the overlay's keys, the programs and the sounds, then the other tracks (SaveTracks); false where the file can't be made
// FUNCTION: GAME 0x59ac30
bool C_recorder::RecSaveFull(const char *name)
{
    S_cutscene_file_header header;
    header.magic = CUTSCENE_FILE_MAGIC;
    header.names_size = anim_names.GetSaveSize();
    header.data_size = 0;
    header.num_actors = 0;
    header.num_keys = camera.num_keys;
    header.num_targets = camera.num_targets;
    header.lists_size = 0;
    header.tracks_size = GetTracksSize();
    memset(header.reserved, 0, sizeof(header.reserved));
    uint i;
    for (i = 0; i < recording.actors.size(); i++)
    {
        header.num_actors++;
        header.data_size += recording.actors[i].records->GetRecordsSize() + 8;
    }
    uint keys_size = camera.num_keys * sizeof(S_cutscene_camera_key) +
        camera.num_targets * sizeof(S_cutscene_target_key);
    S_cutscene_file_actor *entries = new S_cutscene_file_actor[header.num_actors];
    uchar *data = new uchar[header.data_size];
    uchar *keys_buf = new uchar[keys_size];
    uchar *p = data;
    for (i = 0; i < recording.actors.size(); i++)
    {
        entries[i].type = recording.actors[i].type;
        entries[i].ended = recording.actors[i].records->ended;
        entries[i].last_key_time = recording.actors[i].records->last_key_time;
        entries[i].size = recording.actors[i].records->GetRecordsSize() + 8;
        entries[i].offset = i > 0 ? entries[i - 1].size + entries[i - 1].offset : 0;
        for (int j = 0; j < 4; j++)
            entries[i].record_sizes[j] = recording.actors[i].records->record_sizes[j];
        strcpy(entries[i].frame_name, recording.actors[i].actor ? recording.actors[i].actor->frame->GetName() :
            "<none>");
        strcpy(entries[i].name, recording.actors[i].records->name);
        memcpy(p, recording.actors[i].records->first_block, entries[i].size);
        p += entries[i].size;
    }
    uchar *q = keys_buf;
    for (i = 0; i < camera.num_keys; i++)
    {
        memcpy(q, &camera.keys[i], sizeof(S_cutscene_camera_key));
        q += sizeof(S_cutscene_camera_key);
    }
    for (i = 0; i < camera.num_targets; i++)
    {
        memcpy(q, &camera.targets[i], sizeof(S_cutscene_target_key));
        q += sizeof(S_cutscene_target_key);
    }
    uint speed_keys_size = speed_keys.num_keys * sizeof(S_cutscene_speed_key);
    uint overlay_size = overlay.num_keys * sizeof(S_cutscene_overlay_key);
    uint programs_size = programs.num_items * sizeof(S_cutscene_program);
    uint sounds_size = sounds.num_items * sizeof(S_cutscene_sound);
    uint lists_size = speed_keys_size + overlay_size + programs_size + sounds_size + 0x10;
    uchar *lists = new uchar[lists_size];
    q = lists;
    *(uint *)q = speed_keys_size;
    q += 4;
    *(uint *)q = overlay_size;
    q += 4;
    *(uint *)q = programs_size;
    q += 4;
    *(uint *)q = sounds_size;
    q += 4;
    memcpy(q, speed_keys.keys, speed_keys_size);
    q += speed_keys_size;
    memcpy(q, overlay.keys, overlay_size);
    q += overlay_size;
    memcpy(q, programs.items, programs_size);
    q += programs_size;
    memcpy(q, sounds.items, sounds_size);
    header.lists_size = lists_size;
    uint tracks_size = GetTracksSize();
    uchar *tracks = new uchar[tracks_size];
    SaveTracks(tracks);
    FILE *f = fopen(name, "wb");
    if (!f)
    {
        if (entries)
            delete[] entries;
        if (data)
            delete[] data;
        if (keys_buf)
            delete[] keys_buf;
        delete[] lists;
        if (tracks)
            delete[] tracks;
        return false;
    }
    fwrite(&header, sizeof(header), 1, f);
    anim_names.SaveFile(f);
    anim_names.GetSaveSize();
    fwrite(entries, sizeof(S_cutscene_file_actor), header.num_actors, f);
    fwrite(data, header.data_size, 1, f);
    fwrite(keys_buf, keys_size, 1, f);
    fwrite(lists, lists_size, 1, f);
    fwrite(tracks, tracks_size, 1, f);
    fclose(f);
    if (entries)
        delete[] entries;
    if (data)
        delete[] data;
    if (keys_buf)
        delete[] keys_buf;
    delete[] lists;
    if (tracks)
        delete[] tracks;
    return true;
}

// The frame's time through the speed keys: cur_time advances by it, and scaled_time follows the speed key cur_time has
// reached (its scaled_time and the time since it at its speed), which scales the frame's time too; without the speed
// keys scaled_time is cur_time, at the speed 1
// FUNCTION: GAME 0x59b130
void C_recorder::ScaleTime(uint &time)
{
    if (state == CUTSCENE_STOPPED)
        return;
    cur_time += time;
    if (speed_keys_on && speed_keys.num_keys)
    {
        while (speed_keys.cur_key < speed_keys.num_keys - 1 && speed_keys.keys[speed_keys.cur_key + 1].time < cur_time)
            speed_keys.cur_key++;
        S_cutscene_speed_key *key = &speed_keys.keys[speed_keys.cur_key];
        scaled_time = (int)((cur_time - key->time) * key->speed) + key->scaled_time;
        time_speed = key->speed;
        time = (int)((int)time * time_speed);
    }
    else
    {
        scaled_time = cur_time;
        time_speed = 1.0f;
    }
}

// The size SaveBuffer saves (the name is ours, after the animations' names'): a count, and for each of the recording's
// actors without one ("<NONE>") its records and their data
// FUNCTION: GAME 0x59b210
int C_recorder::GetSaveSize()
{
    int size = 4;
    for (uint i = 0; i < recording.actors.size(); i++)
    {
        S_cutscene_actor &a = recording.actors[i];
        // STRING: GAME 0x636150
        if (!strcmp(a.frame_name, "<NONE>"))
            size += a.records->GetRecordsSize() + 0x4c;
    }
    return size;
}

// Saves the recording's actors without one ("<NONE>") to memory (the name is ours): their count, then each one's
// records' size, name, sizes of the record types, ended flag, last key's time, actor type and the records
// FUNCTION: GAME 0x59b2a0
void C_recorder::SaveBuffer(void *buf)
{
    int count = 0;
    GetSaveSize();
    uint i;
    for (i = 0; i < recording.actors.size(); i++)
    {
        if (!strcmp(recording.actors[i].frame_name, "<NONE>"))
            count++;
    }
    uchar *p = (uchar *)buf;
    *(int *)p = count;
    p += 4;
    for (i = 0; i < recording.actors.size(); i++)
    {
        S_cutscene_actor &a = recording.actors[i];
        if (!strcmp(a.frame_name, "<NONE>"))
        {
            uint size = a.records->GetRecordsSize() + 8;
            *(uint *)p = size;
            p += 4;
            memcpy(p, a.name, sizeof(a.name));
            p += sizeof(a.name);
            *(int *)p = a.records->record_sizes[0];
            p += 4;
            *(int *)p = a.records->record_sizes[1];
            p += 4;
            *(int *)p = a.records->record_sizes[2];
            p += 4;
            *(int *)p = a.records->record_sizes[3];
            p += 4;
            *(int *)p = a.records->ended;
            p += 4;
            *(int *)p = a.records->last_key_time;
            p += 4;
            *(int *)p = a.type;
            p += 4;
            memcpy(p, a.records->first_block, size);
            p += size;
        }
    }
}

// Loads what SaveBuffer saved (the name is ours): records for each actor, added to the recording without an actor
// FUNCTION: GAME 0x59b430
void C_recorder::LoadBuffer(void *buf)
{
    uchar *p = (uchar *)buf;
    uint count = *(uint *)p;
    p += 4;
    for (uint i = 0; i < count; i++)
    {
        uint size = *(uint *)p;
        p += 4;
        char name[0x24];
        memcpy(name, p, sizeof(name));
        p += sizeof(name);
        int record_sizes[4];
        record_sizes[0] = *(int *)p;
        p += 4;
        record_sizes[1] = *(int *)p;
        p += 4;
        record_sizes[2] = *(int *)p;
        p += 4;
        record_sizes[3] = *(int *)p;
        p += 4;
        int ended = *(int *)p;
        p += 4;
        int last_key_time = *(int *)p;
        p += 4;
        int type = *(int *)p;
        p += 4;
        uchar *data = new uchar[size];
        memcpy(data, p, size);
        p += size;
        C_cutscene_records *rec = new C_cutscene_records;
        rec->ended = ended;
        rec->last_key_time = last_key_time;
        rec->record_sizes[0] = record_sizes[0];
        rec->record_sizes[1] = record_sizes[1];
        rec->record_sizes[2] = record_sizes[2];
        rec->record_sizes[3] = record_sizes[3];
        rec->first_block = data;
        rec->last_block = data;
        rec->cur = rec->next = rec->unknown_8 = rec->unknown_c = rec->unknown_10 = NULL;
        strcpy(rec->name, name);
        recording.AddEntry(true, false, rec->name, NULL, rec, type);
    }
}

// The animations' names and SaveBuffer's data to a file (the name is ours): SaveBuffer gets the file, not a buffer,
// and the file isn't closed
// FUNCTION: GAME 0x59b570
void C_recorder::SaveFile(const char *name)
{
    FILE *f = fopen(name, "wb");
    if (f)
    {
        anim_names.SaveFile(f);
        SaveBuffer(f);
    }
}

// The same for the loads (the name is ours): the file opened as SaveFile opens it ("wb"), LoadBuffer given the file
// FUNCTION: GAME 0x59b5b0
void C_recorder::LoadFile(const char *name)
{
    FILE *f = fopen(name, "wb");
    if (f)
    {
        anim_names.LoadFile(f);
        LoadBuffer(f);
    }
}

// FUNCTION: GAME 0x59b5f0
C_cutscene_anim_names::C_cutscene_anim_names()
{
    last_id = 0x100;
    num_items = 0;
    items = NULL;
}

// The next id: one more than the last one given
// FUNCTION: GAME 0x59b610
int C_cutscene_anim_names::NewId()
{
    return ++last_id;
}

// Adds an item of the id and the name, its value 0: the array reallocated one larger, the old items copied
// FUNCTION: GAME 0x59b620
bool C_cutscene_anim_names::AddItem(int id, const char *name)
{
    if (!items)
    {
        items = new S_cutscene_anim_name[1];
        num_items = 1;
        strcpy(items[0].name, name);
        items[0].id = id;
        items[0].value = 0;
        return true;
    }
    S_cutscene_anim_name *n = new S_cutscene_anim_name[num_items + 1];
    for (int i = 0; i < num_items; i++)
    {
        strcpy(n[i].name, items[i].name);
        n[i].id = items[i].id;
        n[i].value = items[i].value;
    }
    strcpy(n[num_items].name, name);
    n[num_items].id = id;
    n[num_items].value = 0;
    num_items++;
    delete[] items;
    items = n;
    return true;
}

// The id of the item of the name, -1 if there is none
// FUNCTION: GAME 0x59b740
int C_cutscene_anim_names::FindId(const char *name)
{
    for (int i = 0; i < num_items; i++)
    {
        if (!strcmp(items[i].name, name))
            return items[i].id;
    }
    return -1;
}

// FUNCTION: GAME 0x59b7c0
const char *C_cutscene_anim_names::GetName(int id)
{
    for (int i = 0; i < num_items; i++)
    {
        if (items[i].id == id)
            return items[i].name;
    }
    return NULL;
}

// FUNCTION: GAME 0x59b800
int C_cutscene_anim_names::GetValue(int id)
{
    for (int i = 0; i < num_items; i++)
    {
        if (items[i].id == id)
            return items[i].value;
    }
    return 0;
}

// Every item of the id
// FUNCTION: GAME 0x59b840
void C_cutscene_anim_names::SetValue(int id, int value)
{
    for (int i = 0; i < num_items; i++)
    {
        if (items[i].id == id)
            items[i].value = value;
    }
}

// FUNCTION: GAME 0x59b880
int C_cutscene_anim_names::GetNumItems()
{
    return num_items;
}

// The array freed where it has items, and dropped (the recorder's constructor calls it)
// FUNCTION: GAME 0x59b890
void C_cutscene_anim_names::Reset()
{
    if (items && num_items)
        delete[] items;
    num_items = 0;
    items = NULL;
}

// The vtable's loads and saves: the count and last_id, then each item's id and name (the values aren't saved, the
// loads clear them). The buffer's load allocates the array even for no items, the others not; the archive's frees the
// old one first.
// FUNCTION: GAME 0x59b8c0
bool C_cutscene_anim_names::LoadBuffer(void *buf)
{
    int *p = (int *)buf;
    num_items = *p++;
    last_id = *p++;
    items = new S_cutscene_anim_name[num_items];
    for (int i = 0; i < num_items; i++)
    {
        items[i].id = *p++;
        memcpy(items[i].name, p, sizeof(items[i].name));
        p += sizeof(items[i].name) / sizeof(*p);
        items[i].value = 0;
    }
    return true;
}

// FUNCTION: GAME 0x59b940
bool C_cutscene_anim_names::SaveBuffer(void *buf)
{
    int *p = (int *)buf;
    *p++ = num_items;
    *p++ = last_id;
    for (int i = 0; i < num_items; i++)
    {
        *p++ = items[i].id;
        memcpy(p, items[i].name, sizeof(items[i].name));
        p += sizeof(items[i].name) / sizeof(*p);
    }
    return true;
}

// FUNCTION: GAME 0x59b9a0
bool C_cutscene_anim_names::LoadFile(FILE *file)
{
    fread(&num_items, 1, sizeof(num_items), file);
    fread(&last_id, 1, sizeof(last_id), file);
    if (num_items)
        items = new S_cutscene_anim_name[num_items];
    else
        items = NULL;
    for (int i = 0; i < num_items; i++)
    {
        fread(&items[i].id, 1, sizeof(items[i].id), file);
        fread(items[i].name, sizeof(items[i].name), 1, file);
        items[i].value = 0;
    }
    return true;
}

// FUNCTION: GAME 0x59ba40
bool C_cutscene_anim_names::SaveFile(FILE *file)
{
    fwrite(&num_items, 1, sizeof(num_items), file);
    fwrite(&last_id, 1, sizeof(last_id), file);
    for (int i = 0; i < num_items; i++)
    {
        fwrite(&items[i].id, 1, sizeof(items[i].id), file);
        fwrite(items[i].name, sizeof(items[i].name), 1, file);
    }
    return true;
}

// FUNCTION: GAME 0x59bab0
bool C_cutscene_anim_names::LoadDta(int handle)
{
    dtaRead(handle, &num_items, sizeof(num_items));
    dtaRead(handle, &last_id, sizeof(last_id));
    if (items)
        delete[] items;
    if (num_items)
        items = new S_cutscene_anim_name[num_items];
    else
        items = NULL;
    for (int i = 0; i < num_items; i++)
    {
        dtaRead(handle, &items[i].id, sizeof(items[i].id));
        dtaRead(handle, items[i].name, sizeof(items[i].name));
        items[i].value = 0;
    }
    return true;
}

// FUNCTION: GAME 0x59bb50
void C_cutscene_anim_names::Clear()
{
    if (items)
        delete[] items;
    items = NULL;
    num_items = 0;
}

// The saves' size: the count and last_id, each item's id and name
// FUNCTION: GAME 0x59bb70
int C_cutscene_anim_names::GetSaveSize()
{
    int size = 0;
    for (int i = 0; i < num_items; i++)
        size += sizeof(items[i].id) + sizeof(items[i].name);
    return size + sizeof(num_items) + sizeof(last_id);
}

// The id of the name's item, or a new animation (with its track if tck) at the end of anims and an item for it, whose id
// is its index; -1 if the animation doesn't load
// FUNCTION: GAME 0x59bb90
int C_recorder::AddAnim(const char *name, bool tck)
{
    int id = anim_names.FindId(name);
    if (id != -1)
        return id;
    S_cutscene_anim *anim = CreateAnim(name, tck);
    if (!anim)
        return -1;
    anims.push_back(anim);
    id = anims.size() - 1;
    anim_names.AddItem(id, name);
    return id;
}

// The animation set of anims\<name>, looped, and if tck, its track: the file of the name with the extension .tck
// (none where it doesn't load, length 0); NULL where the set doesn't load
// FUNCTION: GAME 0x59bc20
S_cutscene_anim *C_recorder::CreateAnim(const char *name, bool tck)
{
    char path[256];
    char tck_path[1024];
    S_cutscene_anim *anim = new S_cutscene_anim;
    anim->tck.length = 0;
    anim->anim_set = driver->CreateAnimationSet();
    sprintf(path, "anims\\%s", name);
    if (anim->anim_set->Open(path, 0, NULL, NULL) < I3D_OK)
    {
        anim->anim_set->Release();
        delete anim;
        return NULL;
    }
    anim->anim_set->SetOptions(I3DANIMOP_LOOP);
    if (tck)
    {
        // STRING: GAME 0x636f28
        sprintf(tck_path, "%s", path);
        char *dot = strchr(tck_path, '.');
        if (dot)
        {
            *dot = 0;
            strcat(tck_path, ".tck");
        }
        if (!LoadTCK(tck_path, &anim->tck))
            anim->tck.length = 0;
    }
    return anim;
}

// FUNCTION: GAME 0x59bd80
S_cutscene_anim *C_recorder::GetAnimByIndex(int index)
{
    if (index >= 0 && index < anims.size())
        return anims[index];
    return NULL;
}

// The animation of the name's item, as GetAnimByIndex written out
// FUNCTION: GAME 0x59bdb0
S_cutscene_anim *C_recorder::GetAnim(const char *name)
{
    int index = anim_names.FindId(name);
    if (index >= 0 && index < anims.size())
        return anims[index];
    return NULL;
}

// An animation's track (the file's name as given): as C_animations_machine_de_luxe::LoadTrack reads it, with
// messages, its keys added to the track's vectors; true once the file opened, whatever its version. The locals are
// declared first (their slots are the original's that way). Open: the original calls the chunk stack's constructor
// out of line (ours inlines it and calls the allocator's copy out of line): chunk.ROpen(name) gives that, but then
// inlines the failure's destructor, which the original calls; some 2 units of budget spent near the end (a Spend(0)
// probe) give 100% with ROpen, no source found for them yet
// FUNCTION: GAME 0x59bdf0
bool C_recorder::LoadTCK(const char *name, S_cutscene_tck *tck)
{
    C_chunk chunk;
    int version;
    int num;
    int i;
    // STRING: GAME 0x636fc0
    dbgPrintf("C_recorder:LoadTCK(%s)", name);
    if (chunk.OpenFile(name, false))
    {
        // STRING: GAME 0x636fa4
        dbgPrintf("C_recorder:tck opened...");
        chunk.Read(&version, sizeof(version));
        if (version == TCK_VERSION)
        {
            // STRING: GAME 0x636f8c
            dbgPrintf("C_recorder:version = 4");
            chunk.Read(&tck->start_pos, sizeof(tck->start_pos));
            chunk.Read(&tck->end_pos, sizeof(tck->end_pos));
            chunk.Read(&tck->length, sizeof(tck->length));
            chunk.Read(&tck->key_time, sizeof(tck->key_time));
            chunk.Read(&num, sizeof(num));
            for (i = 0; i < num; i++)
            {
                S_vector v;
                chunk.Read(&v, sizeof(v));
                tck->positions.push_back(v);
            }
            chunk.Read(&num, sizeof(num));
            if (num)
            {
                tck->has_directions = true;
                for (i = 0; i < num; i++)
                {
                    S_vector v;
                    chunk.Read(&v, sizeof(v));
                    tck->directions.push_back(v);
                }
            }
            else
                tck->has_directions = false;
        }
        else
        {
            // STRING: GAME 0x636f70
            dbgPrintf("C_recorder:wrong version");
        }
        // STRING: GAME 0x636f50
        dbgPrintf("C_recorder:LoadTCK() done ok");
        return true;
    }
    // STRING: GAME 0x636f2c
    dbgPrintf("C_recorder:LoadTCK() done failed");
    return false;
}

// Whether the track of the id's item loads: the item's name with the extension .tck (no anims\ before it), loaded into a
// track of its own that is dropped again
// FUNCTION: GAME 0x59c170
bool C_recorder::HasTCK(int id)
{
    S_cutscene_tck tck;
    char path[1024];
    const char *name = vfunc_19(id);
    if (name)
    {
        sprintf(path, "%s", name);
        char *dot = strchr(path, '.');
        if (dot)
        {
            *dot = 0;
            strcat(path, ".tck");
            if (LoadTCK(path, &tck))
                return true;
        }
    }
    return false;
}

// The animations anew: for each of the names' ids from 0 up to their count, CreateAnim's (with its track) of the
// item's name, NULL where no item has the id. One local for both: the original's two push_backs share its slot (the
// first one's insert left out of line by the budget, the second inlined)
// FUNCTION: GAME 0x59c2f0
bool C_recorder::CreateAnims()
{
    anims.clear();
    for (int i = 0; i < anim_names.GetNumItems(); i++)
    {
        const char *name = anim_names.GetName(i);
        S_cutscene_anim *anim;
        if (name)
        {
            anim = CreateAnim(name, true);
            anims.push_back(anim);
        }
        else
        {
            anim = NULL;
            anims.push_back(anim);
        }
    }
    return true;
}

// FUNCTION: GAME 0x59c4e0
void C_recorder::ReleaseAnims()
{
    for (S_cutscene_anim **p = anims.begin(); p < anims.end(); p++)
    {
        if (*p)
        {
            if ((*p)->anim_set)
                (*p)->anim_set->Release();
            delete *p;
        }
    }
    anims.clear();
}

// FUNCTION: GAME 0x59c580
int C_recorder::AddAnimName(const char *name)
{
    int id = anim_names.FindId(name);
    if (id != -1)
        return id;
    int new_id = anim_names.NewId();
    if (anim_names.AddItem(new_id, name))
        return new_id;
    return -1;
}

// The two have the same code: the original's slots point to one copy
// FUNCTION: GAME 0x59c5c0 FOLDED
const char *C_recorder::vfunc_19(int id)
{
    return anim_names.GetName(id);
}

// FUNCTION: GAME 0x59c5c0 FOLDED
const char *C_recorder::GetAnimName(int id)
{
    return anim_names.GetName(id);
}

// FUNCTION: GAME 0x59c5d0
int C_recorder::GetAnimValue(int id)
{
    return anim_names.GetValue(id);
}

// FUNCTION: GAME 0x59c5e0
void C_recorder::SetAnimValue(int id, int value)
{
    anim_names.SetValue(id, value);
}

// FUNCTION: GAME 0x59c5f0
void C_recorder::ResetSpeedKeys()
{
    speed_keys.cur_key = 0;
}

// FUNCTION: GAME 0x59c600
void C_recorder::SetSpeedKeysOn(bool on)
{
    speed_keys_on = on;
}

// FUNCTION: GAME 0x59c610
bool C_recorder::GetSpeedKeysOn()
{
    return speed_keys_on;
}

// FUNCTION: GAME 0x59c620
void C_recorder::RecClear()
{
    recording.RecClear();
}

// FUNCTION: GAME 0x59c630
void C_recorder::RecAddActor(const char *name, C_actor *actor)
{
    recording.RecAddActor(name, actor);
}

// FUNCTION: GAME 0x59c640
void C_recorder::RecLoad(const char *name)
{
    recording.RecLoad(name);
}

// FUNCTION: GAME 0x59c650
void C_recorder::DetachActor(C_actor *actor)
{
    recording.DetachActor(actor);
}

// FUNCTION: GAME 0x59c660
void C_recorder::AttachActor(C_actor *actor, const char *name)
{
    recording.AttachActor(actor, name);
    RewindActor(actor);
}

// FUNCTION: GAME 0x59c690
void C_recorder::RecUpdateActors()
{
    recording.UpdateActors();
}

// The actors' placements back where they still exist: a human's or an entity's model gets its pose back, a door its
// position and rotation (SetRot with a normalization, written out as C_actor::GameDone's). The accessors after the
// two clears (GetType, GetGame, SetFlags) are the inline calls still to come that leave both clears' _Destroy out
// of line, as the original calls it.
// FUNCTION: GAME 0x59c6a0
void C_recorder::Close()
{
    if (act_mission->skip_frames < 2)
        act_mission->skip_frames = 2;
    ReleaseAnims();
    anim_names.Clear();
    SetState(CUTSCENE_STATE_0);
    SetState(CUTSCENE_STOPPED);
    recording.Clear();
    RemoveActors();
    unknown_128 = true;
    if (keys_data)
        delete[] keys_data;
    if (lists_data)
        delete[] lists_data;
    if (tracks_data)
        delete[] tracks_data;
    keys_data = NULL;
    lists_data = NULL;
    tracks_data = NULL;
    if (dabing.items)
        delete[] dabing.items;
    if (subtitles.items)
        delete[] subtitles.items;
    if (morphs.items)
        delete[] morphs.items;
    memset(&camera, 0, sizeof(camera));
    memset(&overlay, 0, sizeof(overlay));
    memset(&speed_keys, 0, sizeof(speed_keys));
    memset(&dabing, 0, sizeof(dabing));
    memset(&subtitles, 0, sizeof(subtitles));
    memset(&programs, 0, sizeof(programs));
    memset(&sounds, 0, sizeof(sounds));
    memset(&morphs, 0, sizeof(morphs));
    cutscene_play_ticks = 0;
    mafiaIndicators.SetOverlayTexture(NULL, false, false);
    mafiaIndicators.SetOverlayFade(0.0f, 0);
    actors.clear();
    actors_2.clear();
    if (saved_placements)
    {
        for (S_cutscene_placement *p = saved_placements->begin(); p < saved_placements->end(); p++)
        {
            if (C_actor::IsGlobalActor(p->actor))
            {
                switch (p->actor->GetType())
                {
                case ACTOR_HUMAN:
                case ACTOR_ENTITY:
                    ((I3D_model *)p->frame)->SetPoseBlend(1.0f);
                    break;
                case ACTOR_DOOR:
                    {
                        p->frame->SetPos(p->pos);
                        I3D_frame *frm = p->frame;
                        frm->m_Rot = p->rot;
                        frm->m_Rot.Normalize();
                        frm->m_FrmFlags = (frm->m_FrmFlags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID |
                            FRMFLAGS_MAT_VALID | FRMFLAGS_ROT_VALID)) | FRMFLAGS_UPDATE | FRMFLAGS_QUAT_VALID;
                        p->frame->Update();
                    }
                    break;
                }
            }
        }
        delete saved_placements;
        saved_placements = NULL;
    }
    DabingReset();
    act_mission->GetGame()->EntityBlock(false, true);
    SetWBuffer(true);
    mafiaIndicators.ParheliaSetCenter(false);
    mafiaIndicators.SubtitlesClear();
    mafiaIndicators.SetFlags(mafiaIndicators.show_flags & ~INDF_CUTSCENE);
}

// The dubbing's, the subtitles' and the morphs' tracks into a buffer and back out of it, and the size they take there:
// the three counts, then the three arrays of items (the names are ours). RecLoadFull loads them, RecSaveFull saves them.
// FUNCTION: GAME 0x59c9c0
void C_recorder::SaveTracks(void *buf)
{
    if (!buf)
        return;
    S_cutscene_tracks_head head;
    head.num_dabing = dabing.num_items;
    head.num_subtitles = subtitles.num_items;
    head.num_morphs = morphs.num_items;
    *(S_cutscene_tracks_head *)buf = head;
    uchar *p = (uchar *)buf + sizeof(head);
    if (head.num_dabing > 0)
    {
        memcpy(p, dabing.items, dabing.num_items * 0x24);
        p += head.num_dabing * 0x24;
    }
    if (head.num_subtitles > 0)
    {
        memcpy(p, subtitles.items, subtitles.num_items * 0xc);
        p += head.num_subtitles * 0xc;
    }
    if (head.num_morphs > 0)
        memcpy(p, morphs.items, morphs.num_items * 0x2c);
}

// FUNCTION: GAME 0x59ca90
void C_recorder::LoadTracks(void *buf)
{
    if (!buf)
        return;
    S_cutscene_tracks_head head = *(S_cutscene_tracks_head *)buf;
    uchar *p = (uchar *)buf + sizeof(head);
    dabing.num_items = head.num_dabing;
    subtitles.num_items = head.num_subtitles;
    morphs.num_items = head.num_morphs;
    dabing.items = NULL;
    subtitles.items = NULL;
    morphs.items = NULL;
    if (head.num_dabing > 0)
        dabing.items = new uchar[dabing.num_items * 0x24];
    if (head.num_subtitles > 0)
        subtitles.items = new uchar[subtitles.num_items * 0xc];
    if (head.num_morphs > 0)
        morphs.items = new uchar[morphs.num_items * 0x2c];
    if (head.num_dabing > 0)
    {
        memcpy(dabing.items, p, dabing.num_items * 0x24);
        p += head.num_dabing * 0x24;
    }
    if (head.num_subtitles > 0)
    {
        memcpy(subtitles.items, p, subtitles.num_items * 0xc);
        p += head.num_subtitles * 0xc;
    }
    if (head.num_morphs > 0)
        memcpy(morphs.items, p, morphs.num_items * 0x2c);
    dabing.next = 0;
    subtitles.next = 0;
    morphs.next = 0;
}

// FUNCTION: GAME 0x59cbf0
int C_recorder::GetTracksSize()
{
    return sizeof(S_cutscene_tracks_head) + dabing.num_items * 0x24 + subtitles.num_items * 0xc +
        morphs.num_items * 0x2c;
}

// The cutscene stopped with nothing loaded: the lists empty, the tracks without items, the time 0 at the speed 1 and
// the speed keys on (the vectors and the two objects constructed first). Defined inline before the object, as the
// original has it: its copy follows the object's initializer, the recording's destructor's copy (its unwinding's)
// follows it, and ~C_recorder, compiled after that, inlines the recording's destructor without unwind states of its own
// FUNCTION: GAME 0x59cc40
inline C_recorder::C_recorder()
{
    state = CUTSCENE_STOPPED;
    prev_state = CUTSCENE_STATE_0;
    actors.clear();
    actors_2.clear();
    unknown_24.clear();
    anims.clear();
    programs.last = -1;
    sounds.last = -1;
    morphs.next = -1;
    scaled_time = 0;
    cur_time = 0;
    time_speed = 1.0f;
    camera.num_targets = 0;
    camera.num_keys = 0;
    camera.keys = NULL;
    camera.targets = NULL;
    speed_keys.num_keys = 0;
    speed_keys.cur_key = 0;
    dabing.num_items = 0;
    subtitles.num_items = 0;
    programs.num_items = 0;
    programs.items = NULL;
    sounds.num_items = 0;
    sounds.items = NULL;
    morphs.num_items = 0;
    morphs.items = NULL;
    unknown_128 = true;
    speed_keys_on = true;
    records_data = (uchar *)1;
    anim_names.Reset();
    overlay.num_keys = 0;
    overlay.cur_key = 0;
    overlay.keys = NULL;
    tracks_data = lists_data = keys_data = NULL;
    camera_placed = false;
}

// The one object: its initializer (0x59cc20) constructs it with the inline constructor, whose copy follows
// GLOBAL: GAME 0x647900
C_recorder cutscene;

// No data and no actors
// FUNCTION: GAME 0x59cf70
C_cutscene_recording::C_cutscene_recording()
{
    data = NULL;
    actors.clear();
    rec_actors.clear();
}

// FUNCTION: GAME 0x59cff0
bool C_cutscene_recording::HasData()
{
    return data ? true : false;
}

// FUNCTION: GAME 0x59d000
bool C_cutscene_recording::SetData(uchar *data)
{
    if (!data)
        return false;
    if (HasData())
        return false;
    this->data = data;
    return true;
}

// The entry of the records' name: true and the entry, where there is one
// FUNCTION: GAME 0x59d030
bool C_cutscene_recording::FindActor(const char *name, S_cutscene_actor **entry)
{
    for (uint i = 0; i < actors.size(); i++)
    {
        if (!strcmp(name, actors[i].name))
        {
            if (entry)
                *entry = &actors[i];
            return true;
        }
    }
    return false;
}

// The same where the entry also has the frame's name (any frame for NULL): the first entry of the name decides
// FUNCTION: GAME 0x59d0e0
bool C_cutscene_recording::FindActor(const char *name, const char *frame_name, S_cutscene_actor **entry)
{
    for (uint i = 0; i < actors.size(); i++)
    {
        if (!strcmp(name, actors[i].name))
        {
            if (!frame_name)
            {
                if (entry)
                    *entry = &actors[i];
                return true;
            }
            S_cutscene_actor *a = &actors[i];
            if (strcmp(frame_name, a->frame_name))
                return false;
            if (entry)
                *entry = a;
            return true;
        }
    }
    return false;
}

// FUNCTION: GAME 0x59d1f0
void C_cutscene_recording::RemoveActor(C_actor *actor, bool unknown)
{
    for (uint i = 0; i < actors.size(); i++)
    {
        if (actors[i].actor == actor)
        {
            if (unknown)
            {
                if (!actors[i].in_data)
                    actors[i].records->Reset();
                actors[i].records = NULL;
                actors[i].actor->records = NULL;
            }
            actors[i].actor = NULL;
            actors.erase(actors.begin() + i);
            return;
        }
    }
}

// FUNCTION: GAME 0x59d2c0
void C_cutscene_recording::Clear()
{
    for (uint i = 0; i < actors.size(); i++)
    {
        if (!actors[i].in_data)
            actors[i].records->Reset();
        if (actors[i].actor)
            actors[i].actor->records->first_block = NULL;
    }
    actors.clear();
    ReleaseData();
}

// FUNCTION: GAME 0x59d350
void C_cutscene_recording::ReleaseData()
{
    int count = 0;
    if (data)
    {
        for (uint i = 0; i < actors.size(); i++)
        {
            if (actors[i].in_data == 1)
                count++;
        }
        if (!count)
        {
            delete data;
            data = NULL;
        }
    }
}

// The entries that have an actor dropped, then the actors of the cutscene's two lists whose records have a block taken
// in again, each its own (RecUpdateActors')
// FUNCTION: GAME 0x59d3c0
void C_cutscene_recording::UpdateActors()
{
    uint i;
    for (i = 0; i < actors.size(); i++)
    {
        if (actors[i].actor)
        {
            actors.erase(actors.begin() + i);
            i = -1;
        }
    }
    for (i = 0; i < cutscene.actors.size(); i++)
    {
        C_actor *actor = cutscene.actors[i];
        if (actor && actor->records && actor->records->first_block)
            AddEntry(true, false, actor->records->name, actor, actor->records, actor->GetType());
    }
    for (i = 0; i < cutscene.actors_2.size(); i++)
    {
        C_actor *actor = cutscene.actors_2[i];
        if (actor && actor->records && actor->records->first_block)
            AddEntry(true, false, actor->records->name, actor, actor->records, actor->GetType());
    }
}

// RECCLEAR: no actors named for RecLoad
// FUNCTION: GAME 0x59d4d0
void C_cutscene_recording::RecClear()
{
    rec_actors.clear();
}

// RECADDACTOR: the actor named for the records of that name, which RecLoad gives it
// FUNCTION: GAME 0x59d500
void C_cutscene_recording::RecAddActor(const char *name, C_actor *actor)
{
    S_cutscene_rec_actor ra;
    strcpy(ra.name, name);
    ra.actor = actor;
    rec_actors.push_back(ra);
}

// With add, the entry of the name gets the records (its own reset unless they are in the data or the same ones), and so
// does its actor; without such an entry a new one, with the actor's frame's name ("<NONE>" without an actor), the
// actor taken into the cutscene with these records and started over
// FUNCTION: GAME 0x59d740
void C_cutscene_recording::AddEntry(bool add, bool shared, const char *name, C_actor *actor,
    C_cutscene_records *records, int type)
{
    S_cutscene_actor *a = NULL;
    switch (add)
    {
    case true:
        if (FindActor(name, NULL, &a))
        {
            if (!a->in_data && a->records != records)
                a->records->Reset();
            a->records = records;
            a->in_data = shared ? 1 : 0;
            a->type = type;
            if (a->actor)
                a->actor->records = a->records;
        }
        else
        {
            if (actor)
                cutscene.AddPlayedActor(actor);
            S_cutscene_actor e;
            strcpy(e.name, name);
            strcpy(e.frame_name, "<NONE>");
            if (actor)
                strcpy(e.frame_name, actor->frame->GetName());
            e.actor = actor;
            e.records = records;
            e.in_data = shared ? 1 : 0;
            e.type = type;
            if (actor)
                actor->records = records;
            actors.push_back(e);
            cutscene.RewindActor(actor);
        }
        break;
    }
}

// An entry for the actor: its records' name, its frame's, its type, not in the data (C_recorder::AddActor's)
// FUNCTION: GAME 0x59da60
void C_cutscene_recording::AddActor(C_actor *actor)
{
    if (actor)
    {
        S_cutscene_actor e;
        strcpy(e.name, actor->records->name);
        strcpy(e.frame_name, actor->frame->GetName());
        e.type = actor->GetType();
        e.records = actor->records;
        e.actor = actor;
        e.in_data = 0;
        actors.push_back(e);
    }
}

// RECLOAD: the records of a recording file for the actors RecAddActor named, by the records' name and the actor's type,
// each a copy of its own (not in the data), then the names cleared and the cutscene played; false where the file
// can't be read
// FUNCTION: GAME 0x59dcd0
bool C_cutscene_recording::RecLoad(const char *name)
{
    int handle = dtaOpen(name, false);
    if (handle < 0)
        return false;
    S_cutscene_file_header header;
    dtaRead(handle, &header, sizeof(header));
    if (header.magic != CUTSCENE_FILE_MAGIC)
    {
        dtaClose(handle);
        return false;
    }
    dtaSeek(handle, header.names_size, SEEK_CUR);
    S_cutscene_file_actor *entries =
        (S_cutscene_file_actor *)new uchar[header.num_actors * sizeof(S_cutscene_file_actor)];
    uchar *file_data = new uchar[header.data_size];
    dtaRead(handle, entries, header.num_actors * sizeof(S_cutscene_file_actor));
    dtaRead(handle, file_data, header.data_size);
    dtaClose(handle);
    for (uint i = 0; i < rec_actors.size(); i++)
    {
        S_cutscene_rec_actor *ra = &rec_actors[i];
        for (uint j = 0; j < header.num_actors; j++)
        {
            S_cutscene_file_actor *fa = &entries[j];
            if (!strcmp(ra->name, fa->name) && ra->actor && fa->type == ra->actor->GetType())
            {
                C_cutscene_records *rec = new C_cutscene_records;
                rec->ended = fa->ended;
                rec->last_key_time = fa->last_key_time;
                for (int k = 0; k < 4; k++)
                    rec->record_sizes[k] = fa->record_sizes[k];
                rec->cur = rec->next = rec->unknown_8 = rec->unknown_c = rec->unknown_10 = NULL;
                rec->first_block = new uchar[fa->size];
                memcpy(rec->first_block, file_data + fa->offset, fa->size);
                rec->last_block = rec->first_block;
                AddEntry(true, false, ra->name, ra->actor, rec, fa->type);
            }
        }
    }
    if (entries)
        delete entries;
    if (file_data)
        delete file_data;
    RecClear();
    cutscene.SetState(CUTSCENE_STATE_1);
    return true;
}

// The actor taken off its entries, their frame's name "<NONE>"
// FUNCTION: GAME 0x59df40
void C_cutscene_recording::DetachActor(C_actor *actor)
{
    for (uint i = 0; i < actors.size(); i++)
    {
        if (actors[i].actor == actor)
        {
            strcpy(actors[i].frame_name, "<NONE>");
            actors[i].actor = NULL;
        }
    }
}

// The actor onto the entry of the records' name where its type is the entry's: the entry takes its frame's name, the
// actor the entry's records, and the cutscene the actor
// FUNCTION: GAME 0x59dfb0
void C_cutscene_recording::AttachActor(C_actor *actor, const char *name)
{
    S_cutscene_actor *a;
    if (actor && FindActor(name, &a) && a->type == actor->GetType())
    {
        strcpy(a->frame_name, actor->frame->GetName());
        a->actor = actor;
        cutscene.AddPlayedActor(actor);
        actor->records = a->records;
    }
}

// The end time of an animation of anim_names by its id, from CUTSCENE_ANIM_ID_BASE on: its set opened from the first of
// the animation cache's folders that has it, 0 where none has
// FUNCTION: GAME 0x59e020
int C_recorder::GetAnimEndTime(uint id)
{
    char buf[256];
    if (id < CUTSCENE_ANIM_ID_BASE)
        return 0;
    const char *name = vfunc_19(id - CUTSCENE_ANIM_ID_BASE);
    if (!name)
        return 0;
    I3D_animation_set *set = driver->CreateAnimationSet();
    bool found = false;
    for (uint i = 0; i < anim_cache.dirs.size() && !found; i++)
    {
        // STRING: GAME 0x636fd8
        sprintf(buf, "%s%s", (const char *)anim_cache.dirs[i], name);
        if (set->Open(buf, 0, NULL, NULL) >= I3D_OK)
            found = true;
    }
    int time = found ? set->GetEndTime() : 0;
    set->Release();
    return time;
}

// FUNCTION: GAME 0x59e100
int C_recorder::vfunc_46()
{
    return 5;
}

// The HUD's overlay: the picture (reloaded) and its fade
// FUNCTION: GAME 0x59e110
void C_recorder::SetOverlay(float fade, uint color, const char *name, bool opaque)
{
    mafiaIndicators.SetOverlayTexture(name, opaque, true);
    mafiaIndicators.SetOverlayFade(fade, color);
}

// A speech of the channel by the actor's frame, with its subtitle (kind 3)
// FUNCTION: GAME 0x59e140
void C_recorder::Speak(int chanel, C_actor *actor, uint speech)
{
    DabingStop(chanel);
    DabingSetModel(chanel, (I3D_model *)actor->frame);
    DabingSpeak(chanel, speech, true, 3);
}

// FUNCTION: GAME 0x59e170
void C_recorder::vfunc_49(uint time)
{
    DabingTick(time);
}

// The show of that name opened (none for NULL), played and paused at once, its start at the time 0
// FUNCTION: GAME 0x59e180
void C_recorder::OpenShow(const char *name)
{
    if (movie)
        movie->Release();
    movie = NULL;
    if (name)
    {
        movie = igraph->CreateIShow();
        movie->Open(name, 0);
        movie->Play();
        movie->Pause();
    }
    cutscene_show_start = 0;
}

// FUNCTION: GAME 0x59e1e0
void C_recorder::SetShowStart(int unknown)
{
    cutscene_show_start = unknown;
}

// The show kept at the cutscene's time from its start: stopped before it, paused but in the states 1 to 3, put at the
// time where it is more than 300 ms off, and played again after an event. The calls to Stop and Pause are tail calls in
// the original (jmp), which the early returns' spelling doesn't give; the states compare unsigned (state is a uint)
// FUNCTION: GAME 0x59e1f0
void C_recorder::TickShow(int time)
{
    if (movie)
    {
        int t = time > cutscene_show_start ? time - cutscene_show_start : -1;
        if (t == -1)
            movie->Stop();
        else if (state > CUTSCENE_STATE_0 && state <= CUTSCENE_STATE_3)
        {
            uint pos;
            movie->GetCurrentPos(&pos);
            if (abs((int)(pos / 1000) - t) > 300)
                movie->SetCurrentPos(t * 1000);
            if (movie->GetEvent())
                movie->Play();
        }
        else
            movie->Pause();
    }
}

// FUNCTION: GAME 0x59e2a0
void C_recorder::Stop()
{
    cutscene_play_ticks = 0;
    SetState(CUTSCENE_STATE_0);
    if (act_mission->skip_frames < 4)
        act_mission->skip_frames = 4;
}

// FUNCTION: GAME 0x59e2d0
bool C_recorder::IsPlaying()
{
    if (cutscene_play_ticks)
        return true;
    if (state == CUTSCENE_STATE_0)
        return false;
    return state != CUTSCENE_STOPPED ? true : false;
}

// The vtable's last function: the linker's folded return of 0 of the out-of-line kind (0x4aa430, by name in
// actors/C_actor.h)
int C_recorder::vfunc_53()
{
    return 0;
}

// The members' constructors do the rest: the collision's memset and the vector's
// FUNCTION: GAME 0x59e7e0
C_seagull::C_seagull()
{
    type = ACTOR_SEAGULL;
}

// Declared, empty: it stores the vtable again before the vector's destructor
// FUNCTION: GAME 0x59e850
C_seagull::~C_seagull()
{
}

// The linker's folded empty function of the out-of-line kind (0x46ca70)
void C_seagull::vfunc_38(int a, int b)
{
}

// The original's is C_pumper's, folded: it logs that name, so ours folds too
// STUB: GAME 0x47c760 FOLDED
bool C_seagull::Init(I3D_frame *frm)
{
    StubLog("C_pumper::Init");
    return C_actor::Init(frm);
}

// The linker's folded copy (0x4aa490)
bool C_seagull::SaveGameSave(void **buf)
{
    return true;
}

// The linker's folded copy (0x4aa490)
bool C_seagull::SaveGameLoad(void **buf)
{
    return true;
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_seagull::Update(uint time)
{
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_seagull::NetDirect(uint time)
{
}

// The box around the ring of the walking paths' nodes nearest the frame (the WEBF_UNKNOWN_8 ones, each linked to two)
// with a wall through each pair of the ring's neighbours, its floor and its ceiling; a box collision of it; and five
// seagulls at random places in it, each flying its random way. The walls go into planes from the first on, so the
// floor and the ceiling replace the first two (as the original does).
// Open: VC6 knows num_conns is 2 after the walk's test and drops GetConn's tests, which the original keeps (no loop
// shape, accessor or GetConn spelling tried keeps them); the push_back's nested calls, which the original calls out of
// line (size, _Ucopy, _Ufill, _Destroy), as one more inline call after it would give; and registers.
// FUNCTION: GAME 0x59e890
void C_seagull::GameInit()
{
    C_actor::GameInit();
    bbox.min = S_vector(FLT_MAX, FLT_MAX, FLT_MAX);
    bbox.max = S_vector(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    unknown_65 = true;
    num_planes = 0;
    int first = act_mission->web_path->GetNearNode(frame->GetWorldPos(), -1.0f, WEBF_UNKNOWN_8);
    int cur = first;
    int prev = act_mission->web_path->nodes[first].num_conns ? act_mission->web_path->nodes[first].GetConn(0)->node :
        first;
    int next = -2;
    S_vector p1, p2;
    while (true)
    {
        if (next == first)
            break;
        C_web_node &node = act_mission->web_path->nodes[cur];
        if (node.num_conns != 2)
            break;
        next = node.GetConn(0)->node == prev ? node.GetConn(1)->node : node.GetConn(0)->node;
        p1 = node.GetPos();
        p2 = act_mission->web_path->nodes[next].GetPos();
        S_plane pl;
        pl.ComputePlane(p1, p2, S_vector(p2.x, p2.y + 1.0f, p2.z));
        planes[num_planes] = pl;
        num_planes++;
        if (p1.x < bbox.min.x)
            bbox.min.x = p1.x;
        if (p1.x > bbox.max.x)
            bbox.max.x = p1.x;
        if (p2.x < bbox.min.x)
            bbox.min.x = p2.x;
        if (p2.x > bbox.max.x)
            bbox.max.x = p2.x;
        if (p1.y < bbox.min.y)
            bbox.min.y = p1.y;
        if (p1.y > bbox.max.y)
            bbox.max.y = p1.y;
        if (p2.y < bbox.min.y)
            bbox.min.y = p2.y;
        if (p2.y > bbox.max.y)
            bbox.max.y = p2.y;
        if (p1.z < bbox.min.z)
            bbox.min.z = p1.z;
        if (p1.z > bbox.max.z)
            bbox.max.z = p1.z;
        if (p2.z < bbox.min.z)
            bbox.min.z = p2.z;
        if (p2.z > bbox.max.z)
            bbox.max.z = p2.z;
        prev = cur;
        cur = next;
    }
    planes[0] = S_plane(S_vector(0.0f, 1.0f, 0.0f), -S_vector(0.0f, 1.0f, 0.0f).Dot(bbox.min));
    planes[1] = S_plane(S_vector(0.0f, -1.0f, 0.0f), -S_vector(0.0f, -1.0f, 0.0f).Dot(bbox.max));
    memset(&coll, 0, sizeof(coll));
    coll.type = DYNCOLL_UNKNOWN_40;
    ((I3D_bbox &)coll.bbox).min = bbox.min;
    coll.owner = this;
    ((I3D_bbox &)coll.bbox).max = bbox.max;
    matrix.Identity();
    coll.matrix = &matrix;
    coll.inv_matrix = &matrix;
    ((I3D_bsphere &)coll.bsphere).radius = (bbox.max - bbox.min).Magnitude() * .5f;
    coll.flags = DYNCOLLF_UPPER | DYNCOLLF_UNKNOWN_4 | DYNCOLLF_UNKNOWN_40000;
    ((I3D_bsphere &)coll.bsphere).pos = (bbox.max + bbox.min) * .5f;
    for (int i = 0; i < 5; i++)
    {
        S_seagull gull;
        gull.state = SEAGULL_FLYING;
        gull.unknown_58 = 50 + i * 100;
        gull.model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
        gull.model->SetPos(0.0f, 0.0f, (float)i);
        gull.model->SetDir(S_vector((float)(5 - i), 0.0f, 0.0f), 0.0f);
        gull.model->LinkTo(act_mission->scene->GetPrimarySector(), 0);
        gull.model->Update();
        act_mission->scene->SetFrameSector(gull.model);
        // STRING: GAME 0x636fe0
        model_cache.Open(gull.model, "racek.i3d", 0, NULL, NULL, NULL);
        gull.unknown_30 = S_vector(1.0f, 0.0f, 0.0f);
        gull.unknown_48 = S_vector(0.0f, 0.0f, 1.0f);
        S_vector pos;
        pos.x = (rand() * (1.0f / RAND_MAX)) * (bbox.max.x - bbox.min.x) + bbox.min.x;
        pos.y = (rand() * (1.0f / RAND_MAX)) * (bbox.max.y - bbox.min.y) + bbox.min.y;
        pos.z = (rand() * (1.0f / RAND_MAX)) * (bbox.max.z - bbox.min.z) + bbox.min.z;
        gull.target = pos;
        gull.dir = S_vector((rand() % 100 - 100) * .01f, 0.0f, (rand() % 100 - 100) * .01f);
        gull.dir.Normalize();
        gull.turn_time = 1.0f;
        gull.next_dir = S_vector((rand() % 100 - 100) * .01f, 0.0f, (rand() % 100 - 100) * .01f);
        gull.next_dir.Normalize();
        gull.model->SetPos(S_vector(pos.x, pos.y + 1.0f, pos.z));
        gull.model->Update();
        gulls.push_back(gull);
    }
}

// The seagulls' models released and the flock emptied
// FUNCTION: GAME 0x59f6a0
void C_seagull::GameDone()
{
    C_actor::GameDone();
    for (S_seagull *g = gulls.begin(); g < gulls.end(); g++)
        g->model->Release();
    gulls.clear();
}

// A collision with the seagulls' box releases them all, as GameDone, and isn't one
// FUNCTION: GAME 0x59f710
bool C_seagull::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    I3D_frame *frm)
{
    for (S_seagull *g = gulls.begin(); g < gulls.end(); g++)
        g->model->Release();
    gulls.clear();
    return false;
}

// A shot (v2 the line's start, v1 its direction): the seagull nearest the line within 1 falls. The original writes the
// 2000 through its loop pointer, which by then points past the last seagull.
// FUNCTION: GAME 0x59f770
bool C_seagull::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
    uint u, I3D_frame *frm)
{
    float best = 1.0f;
    S_seagull *hit = NULL;
    if (type == HIT_SHOT)
    {
        S_seagull *g;
        for (g = gulls.begin(); g < gulls.end(); g++)
        {
            if (g->state != SEAGULL_STATE_0)
            {
                float dist = g->model->GetWorldPos().DistanceToLine(v2, v1);
                if (dist < best)
                {
                    best = dist;
                    hit = g;
                }
            }
        }
        if (hit)
        {
            hit->state = SEAGULL_FALLING;
            g->unknown_58 = 2000;
            return true;
        }
    }
    return false;
}

// Each seagull's step of the time (in ms): a falling one falls faster and moves along its speed, and where its way meets
// the level it is placed there and explodes (no damage) and comes back flying; a flying one moves along its frame's
// direction and turns to its target, which moves on along dir; where the target's time to the next wall runs out it
// turns to next_dir, and the next wall it will meet sets its new time and the direction after it (the wall's normal
// turned back, with a little at random); a respawned one gets a random place in the box, inside every plane, and
// random directions. The walls tested are planes 2 to 5, with a branch for the floor and the ceiling (0 and 1) the
// loop never reaches, as the original.
// Open: two pairs of stack slots, and the target's distance to a wall loads target.x before the normal's x
// FUNCTION: GAME 0x59f920
void C_seagull::AI(uint time)
{
    float t = time * .001f;
    for (S_seagull *g = gulls.begin(); g < gulls.end(); g++)
    {
        switch (g->state)
        {
        case SEAGULL_RESPAWN:
        {
            g->unknown_30 = S_vector(1.0f, 0.0f, 0.0f);
            g->unknown_48 = S_vector(0.0f, 0.0f, 1.0f);
            S_vector pos;
            bool ok;
            do
            {
            pos.x = (rand() * (1.0f / RAND_MAX)) * (bbox.max.x - bbox.min.x) + bbox.min.x;
            pos.y = (rand() * (1.0f / RAND_MAX)) * (bbox.max.y - bbox.min.y) + bbox.min.y;
            pos.z = (rand() * (1.0f / RAND_MAX)) * (bbox.max.z - bbox.min.z) + bbox.min.z;
            ok = true;
            for (uint i = 0; i < num_planes; i++)
            {
                if (pos.Dot(planes[i].normal) + planes[i].d < 0.0f)
                {
                    ok = false;
                    break;
                }
            }
            } while (!ok);
            g->target = pos;
            g->dir = S_vector((rand() % 100 - 100) * .01f, 0.0f, (rand() % 100 - 100) * .01f);
            g->dir.Normalize();
            g->turn_time = 1.0f;
            g->next_dir = S_vector((rand() % 100 - 100) * .01f, 0.0f, (rand() % 100 - 100) * .01f);
            g->next_dir.Normalize();
            g->model->SetPos(S_vector(pos.x, pos.y + 1.0f, pos.z));
            g->model->Update();
            g->state = SEAGULL_FLYING;
            break;
        }
        case SEAGULL_FLYING:
        {
            S_vector to;
            to = g->target - g->model->GetPos();
            S_vector dir = g->model->GetDir();
            g->model->SetPos(g->model->GetPos() + dir * t);
            g->model->SetDir(to, 0.0f);
            g->model->Update();
            if (t < g->turn_time)
            {
                g->target += g->dir * t;
                g->target.y = 15.0f;
                g->turn_time -= t;
            } else
            {
                g->target += g->dir * g->turn_time;
                float best = 1000.0f;
                g->dir = g->next_dir;
                g->target += g->dir * (t - g->turn_time);
                for (int i = 2; i < 6; i++)
                {
                    float dot = planes[i].normal.Dot(g->dir);
                    if (dot > MRG_ZERO)
                    {
                        float dist = (g->target.Dot(planes[i].normal) + planes[i].d) / dot;
                        if (dist < best)
                        {
                            best = dist;
                            if (i < 2)
                            {
                                g->next_dir = g->dir;
                                g->next_dir.y *= -1.0f;
                                g->next_dir += S_vector((rand() % 100 - 100) * .001f, (rand() % 100 - 100) * .001f,
                                    (rand() % 100 - 100) * .001f);
                                g->next_dir.Normalize();
                            } else
                            {
                                g->next_dir = planes[i].normal * -2.0f;
                                g->next_dir += S_vector((rand() % 100 - 100) * .001f, (rand() % 100 - 100) * .001f,
                                    (rand() % 100 - 100) * .001f);
                                g->next_dir.Normalize();
                            }
                        }
                    }
                }
                if (best > 100.0f)
                    best = 100.0f;
                else if (best < 0.0f)
                    best = 0.0f;
                g->turn_time = best;
            }
            break;
        }
        case SEAGULL_FALLING:
        {
            g->dir.y -= t * 10.0f;
            S_vector pos = g->model->GetPos();
            S_vector step = g->dir * t;
            S_vector hit_pos;
            if (gifCollision.TestLineV(pos, step, &hit_pos, NULL, COLF_ALL, 0))
            {
                g->model->SetPos(hit_pos);
                g->model->Update();
                act_mission->GetGame()->NewExplosion(this, hit_pos, 5.0f, 5.0f, false, true, true, SOUND_EXPLOSION);
                g->state = SEAGULL_RESPAWN;
            } else
            {
                g->model->SetPos(pos + step);
                g->model->Update();
            }
            break;
        }
        }
    }
}
