// The actors' animations (PS2 C_animations.cpp, C_animations_machine_de_luxe), the file at 0x40df70-0x411bdc, which the
// tracks' initializer starts (docs/game-files.md), written whole in PS2's order: the tracks, the machine's Init, Reset
// and the slots, SetAnim, Tick, the extended animation, the tracks' placements and StageBlendingTick; then the
// headers' first copies (the anim cache's Create and its items' helpers, the vectors' templates).
// The tracks' destructor (0x40dfb0, registered by 0x40dfa0) tests and sets a guard byte (0x63d2d4) of its own, as
// C_actor's static members' do. Open: reccmp scores the initializer's first half 50%, as nothing names the atexit
// registration it jumps to; the anim cache's Create (0x4104f0) is as far off as the other caches' (game/I3D_cache.h);
// the tracks' fill and copy_backward call the keys' size and capacity in the other order (the assignment inlined in
// them, as S_track::operator= notes).

#include "actors/C_animations.h"

#include <stdio.h>
#include <string.h>

#include "C_chunk.h"
#include "I3D_cache.h"
#include "actors/C_actor.h"
#include "actors/C_human.h"
#include "ai/C_person_traffic.h"

// GLOBAL: GAME 0x63d2d8
std::vector<C_animations_machine_de_luxe::S_track> C_animations_machine_de_luxe::tracks;
// GLOBAL: GAME 0x63d2e8
int C_animations_machine_de_luxe::base_track;

// The pointers only: Init sets the rest
// FUNCTION: GAME 0x40e010
C_animations_machine_de_luxe::C_animations_machine_de_luxe()
{
    actor = NULL;
    model = NULL;
}

// Empty: the linker folded it with the other empty functions of its kind (0x4aabe0), where C_traffic_generator::
// ClearElements calls it (PS2's is in this file, its place on PC isn't known)
C_animations_machine_de_luxe::~C_animations_machine_de_luxe()
{
}

// The tracks are searched from the last one. The failures share the chunk's destructor and return -1 at the end, PS2's
// too, and the track has a block of its own: the original destroys it before it counts the tracks. The first five reads
// are inlined, the budget leaves the others and the second vector's constructor out of line, as the original.
// FUNCTION: GAME 0x40e020
int C_animations_machine_de_luxe::LoadTrack(const char *name)
{
    for (int i = tracks.size(); i--;)
    {
        if (!_strcmpi(name, tracks[i].name))
            return i;
    }
    char path[128];
    // STRING: GAME 0x632280
    sprintf(path, "anims\\%s", name);
    C_chunk chunk;
    if (chunk.OpenFile(path, false))
    {
        int version;
        chunk.Read(&version, sizeof(version));
        if (version == TCK_VERSION)
        {
            {
                S_track t;
                strcpy(t.name, name);
                chunk.Read(&t.start_pos, sizeof(t.start_pos));
                chunk.Read(&t.end_pos, sizeof(t.end_pos));
                chunk.Read(&t.length, sizeof(t.length));
                chunk.Read(&t.key_time, sizeof(t.key_time));
                int num;
                chunk.Read(&num, sizeof(num));
                if (num)
                {
                    t.positions.resize(num, S_vector(0.0f, 0.0f, 0.0f));
                    chunk.Read(t.positions.begin(), num * sizeof(S_vector));
                }
                chunk.Read(&num, sizeof(num));
                if (num)
                {
                    t.directions.resize(num, S_vector(0.0f, 0.0f, 0.0f));
                    chunk.Read(t.directions.begin(), num * sizeof(S_vector));
                }
                t.has_directions = num > 0;
                tracks.push_back(t);
                chunk.Close();
            }
            return tracks.size() - 1;
        }
        chunk.Close();
    }
    return -1;
}

// A loaded track's index, -1 if it isn't loaded (PS2 name), which SetAnim calls for a track it doesn't load
// FUNCTION: GAME 0x40e4a0
int C_animations_machine_de_luxe::GetTrack(const char *name)
{
    for (int i = tracks.size(); i--;)
    {
        if (!_strcmpi(name, tracks[i].name))
            return i;
    }
    return -1;
}

// The tracks after the base ones from the last one, or all of them before C_game::Init has counted the base ones
// FUNCTION: GAME 0x40e510
void C_animations_machine_de_luxe::ClearNoBaseTracks()
{
    if (base_track > 0)
    {
        while (tracks.size() > base_track)
            tracks.pop_back();
    }
    else
        ClearTracks();
}

// Reset's stores written out in each (the game's /Ob1 inlines no function not declared inline)
// FUNCTION: GAME 0x40e660
void C_animations_machine_de_luxe::Init(C_actor *act)
{
    actor = act;
    element = NULL;
    model = act ? (I3D_model *)act->frame : NULL;
    anim0_on = anim1_on = false;
    anim0_stop_blended = anim1_stop_blended = false;
    anim0_once = anim1_once = false;
    stage_blending = false;
    blend = 0.0f;
    blend_speed = 0.0f;
    anim_index = 0;
    track = -1;
    prev_track = -1;
    next_start = -1.0f;
    ext_on = false;
    disabled = false;
    unknown_10e = 0xffff;
    anim0_id = anim1_id = (uint)-1;
}

// FUNCTION: GAME 0x40e6d0
void C_animations_machine_de_luxe::Init(C_traffic_element *e)
{
    actor = NULL;
    element = e;
    model = e ? e->frame : NULL;
    anim0_on = anim1_on = false;
    anim0_stop_blended = anim1_stop_blended = false;
    anim0_once = anim1_once = false;
    stage_blending = false;
    blend = 0.0f;
    blend_speed = 0.0f;
    anim_index = 0;
    track = -1;
    prev_track = -1;
    next_start = -1.0f;
    ext_on = false;
    disabled = false;
    unknown_10e = 0xffff;
    anim0_id = anim1_id = (uint)-1;
}

// FUNCTION: GAME 0x40e740
void C_animations_machine_de_luxe::Init(I3D_model *m)
{
    actor = NULL;
    element = NULL;
    model = m;
    anim0_on = anim1_on = false;
    anim0_stop_blended = anim1_stop_blended = false;
    anim0_once = anim1_once = false;
    stage_blending = false;
    blend = 0.0f;
    blend_speed = 0.0f;
    anim_index = 0;
    track = -1;
    prev_track = -1;
    next_start = -1.0f;
    ext_on = false;
    disabled = false;
    unknown_10e = 0xffff;
    anim0_id = anim1_id = (uint)-1;
}

// FUNCTION: GAME 0x40e7a0
void C_animations_machine_de_luxe::Reset()
{
    anim0_on = anim1_on = false;
    anim0_stop_blended = anim1_stop_blended = false;
    anim0_once = anim1_once = false;
    stage_blending = false;
    blend = 0.0f;
    blend_speed = 0.0f;
    anim_index = 0;
    track = -1;
    prev_track = -1;
    next_start = -1.0f;
    ext_on = false;
    disabled = false;
    unknown_10e = 0xffff;
}

// FUNCTION: GAME 0x40e7f0
void C_animations_machine_de_luxe::EqualTime(int src, int dst)
{
    I3D_animation_set *set = model->GetAnimationSet(src);
    if (!set)
        return;
    uint length = set->GetEndTime();
    uint time = model->GetAnimTime(src);
    if (!length || time > length)
        return;
    set = model->GetAnimationSet(dst);
    if (!set)
        return;
    int t = (int)((uint)set->GetEndTime() * ((float)time / length));
    model->SetAnimTime(dst, t);
}

// FUNCTION: GAME 0x40e890
void C_animations_machine_de_luxe::GetActiveAnimTime(uint &time, uint &length)
{
    length = time = 0;
    if (model)
    {
        I3D_animation_set *set = model->GetAnimationSet(anim_index);
        if (set)
        {
            length = set->GetEndTime();
            time = model->GetAnimTime(anim_index);
        }
    }
}

// FUNCTION: GAME 0x40e8e0
void C_animations_machine_de_luxe::StopAnims()
{
    if (model)
    {
        for (int i = 0; i < 8; i++)
            model->StopAnimation(i);
    }
}

// The translation cleared through the matrix's element accessor: its three pending inline calls leave GetLocalMatrix's
// UpdateMatrix out of line, as the original calls it (the airplane file's copy, 0x40de20)
// FUNCTION: GAME 0x40e900
void C_animations_machine_de_luxe::RecompileMatrix(S_matrix &m)
{
    track_pos *= m;
    prev_track_pos *= m;
    track_matrix = model->GetLocalMatrix();
    track_matrix(3, 0) = track_matrix(3, 1) = track_matrix(3, 2) = 0.0f;
}

// Starts the animation name in the slot the active one doesn't use (slot 0 when neither plays), blending to it at speed
// per second, with the track of its name (.tck) if use_track; keep_track keeps the old track going on (prev_track) while
// the blend lasts, and the correction then moves the actor from the old track's end to the new one's start.
// The count of the old track's directions is a local: tested directly, VC6 orders the operands of the inlined
// UpdateMatrix's products the other way round in the correction's GetLocalMatrix (any local declared there gives the
// original's, I3D_model *mod = model; or a const reference to the matrix as well).
// FUNCTION: GAME 0x40e950
void C_animations_machine_de_luxe::SetAnim(const char *name, bool stop_blended, bool equal_time, bool once, float speed,
    float start, bool use_track, bool load_track, S_vector *scale, bool keep_track, uint id)
{
    if (disabled || !model)
        return;
    // The old track goes on while the blend lasts, unless less than 100 ms of it are left
    if (keep_track)
    {
        if (track != -1 && tracks[track].length - track_time > 10)
            prev_track = track;
        else
            prev_track = -1;
        if (prev_track != -1)
        {
            int rest = tracks[prev_track].length - track_time;
            if (rest >= 100)
            {
                prev_blend = 0.0f;
                prev_blend_speed = 1.0f / rest;
                prev_track_time = track_time;
                prev_track_pos = track_pos;
                prev_track_pos.y += land_height;
                prev_track_matrix = track_matrix;
            }
            else
                prev_track = -1;
        }
    }
    else
        prev_track = -1;
    // The track: the animation's name with the extension .tck, from the old track's end or the model's position. An
    // animation with a track plays once.
    if (use_track)
    {
        char buff[64];
        strcpy(buff, name);
        char *ext = strchr(buff, '.');
        if (ext)
            *ext = 0;
        strcat(buff, ".tck");
        track = load_track ? LoadTrack(buff) : GetTrack(buff);
        if (track != -1)
        {
            track_offset.Zero();
            land_height = 0.0f;
            once = true;
            if (prev_track == -1)
                track_pos = model->GetPos();
            else
            {
                track_pos = tracks[prev_track].positions[tracks[prev_track].positions.size() - 1];
                track_pos *= prev_track_matrix;
                track_pos += prev_track_pos;
            }
            track_time = 0;
            track_matrix = model->GetLocalMatrix();
            track_matrix(3, 0) = track_matrix(3, 1) = track_matrix(3, 2) = 0.0f;
            if (scale)
            {
                track_matrix(0, 0) *= scale->x;
                track_matrix(1, 1) *= scale->y;
                track_matrix(2, 2) *= scale->z;
            }
        }
    }
    else
        track = -1;
    I3D_animation_set *set = NULL;
    anim_cache.Create(name, &set, NULL, NULL);
    if (!set)
        return;
    set->SetOptions(once ? I3DANIMOP_ONCE : I3DANIMOP_LOOP);
    if (!anim0_on && !anim1_on)
    {
        // Nothing plays: slot 0 at once
        anim0_on = true;
        anim0_stop_blended = stop_blended;
        model->SetAnimation(set, 0, 1.0f, 0);
        model->SetAnimBlend(0, 1.0f);
        model->SetAnimBlend(1, 0.0f);
        stage_blending = false;
        anim_index = 0;
        anim0_id = id;
    }
    else
    {
        // Both play: the one blended out more stops
        if (anim0_on && anim1_on)
        {
            if (blend > .5f)
            {
                anim1_on = false;
                model->StopAnimation(1);
            }
            else
            {
                anim0_on = false;
                model->StopAnimation(0);
            }
        }
        if (anim0_on)
        {
            anim1_on = true;
            anim1_stop_blended = stop_blended;
            anim1_once = once;
            stage_blending = true;
            blend = .95f;
            blend_speed = -speed;
            anim_index = 1;
            model->SetAnimation(set, 1, 0.0f, 0);
            model->SetAnimBlend(0, .95f);
            model->SetAnimBlend(1, .05f);
            if (equal_time)
                EqualTime(0, 1);
            anim1_id = id;
        }
        else
        {
            anim0_on = true;
            anim0_stop_blended = stop_blended;
            anim0_once = once;
            stage_blending = true;
            blend = .05f;
            blend_speed = speed;
            anim_index = 0;
            model->SetAnimation(set, 0, 0.0f, 0);
            model->SetAnimBlend(0, .05f);
            model->SetAnimBlend(1, .95f);
            if (equal_time)
                EqualTime(1, 0);
            anim0_id = id;
        }
    }
    // The start, a part of the animation's length: the one set for the next animation first
    if (next_start > 0.0f)
    {
        start = next_start;
        next_start = -1.0f;
    }
    if (!equal_time && start >= 0.0f)
    {
        uint time = (uint)((uint)set->GetEndTime() * start);
        model->Tick(0);
        model->SetAnimTime(anim_index, time);
    }
    // The correction over the old track's rest: from its end to the new track's start, or to the animation's base
    // frame without a track, in the model's directions where the old track has its own
    if (prev_track != -1)
    {
        correcting = true;
        float rest = (float)(tracks[prev_track].length - prev_track_time);
        if (rest > 0.0f)
        {
            correction_speed = 1.0f / rest;
            correction = 0.0f;
            if (track != -1)
                correction_offset = tracks[prev_track].end_pos - tracks[track].start_pos;
            else
                correction_offset = tracks[prev_track].end_pos - GetAnimBaseDelta(set);
            int num_dirs = tracks[prev_track].directions.size();
            if (num_dirs)
            {
                S_matrix m = model->GetLocalMatrix();
                m(3, 0) = m(3, 1) = m(3, 2) = 0.0f;
                correction_offset *= m;
            }
            else
                correction_offset *= track_matrix;
        }
        else
            correcting = false;
    }
    else
        correcting = false;
    set->Release();
}

// The position of the animation's "blnd" frame at its first key, zero without one (PS2 name; its own function there,
// inlined in SetAnim on PC)
inline S_vector C_animations_machine_de_luxe::GetAnimBaseDelta(I3D_animation_set *set)
{
    S_vector delta(0.0f, 0.0f, 0.0f);
    if (set)
    {
        // STRING: GAME 0x63228c
        int link = set->GetAnimIndex("blnd");
        if (link >= 0)
        {
            int time;
            set->GetPosKey(link, 0, time, delta);
        }
    }
    return delta;
}

// FUNCTION: GAME 0x40f170
bool C_animations_machine_de_luxe::IsActive()
{
    return !disabled && (stage_blending || track != -1 || (ext_fading || ext_once) && ext_on);
}

// The height the jump has reached: land_height times the part of the slot's animation played, up to 240 ms before
// its end (the rest of it with land_reverse), none without the model or the animation. Tick has the code twice, and
// tests the model again: an inline helper (PS2 inlines it too).
inline float C_animations_machine_de_luxe::GetLandOffset()
{
    if (!model)
        return 0.0f;
    I3D_animation_set *set = model->GetAnimationSet(land_slot);
    if (!set)
        return 0.0f;
    uint length = set->GetEndTime() - 240;
    float f = (float)(uint)model->GetAnimTime(land_slot) / length;
    if (f > 1.0f)
        f = 1.0f;
    if (land_reverse)
        f = 1.0f - f;
    return f * land_height;
}

// The animations' frame, in PS2's order and shapes: the slots played once stop at their end, the stage blending goes on
// (StageBlendingTick's code without its model tick), the actor moves along the track, blended with the previous track
// while its blend lasts, or along the previous track alone, the correction spreads over the old track's rest, and the
// extended animation fades. ANIMTICK_END in the track's last 250 ms, ANIMTICK_NO_MODEL without a model.
// The keys' interpolation is written out three times, as PS2's: key + 1 is a local (computed before size()'s ?:, VC6
// sends an empty list straight to the else), so is the left part of the key's time (its conversion is stored once for
// the positions and the directions), and the sum is scaled by the reciprocal written out (`*= 1.0f / key_time`, the
// temporary kept on the FPU stack). pos is the function's, dir the track's block's: each moves the frame slots of the
// interpolations' temporaries and the correction's position to the original's, and the order of the correction's
// operand loads.
// FUNCTION: GAME 0x40f1b0
uint C_animations_machine_de_luxe::Tick(uint time)
{
    if (disabled)
        return 0;
    uint flags = 0;
    if (!model)
        return ANIMTICK_NO_MODEL;
    if (anim0_on && anim0_once && !model->AnimIsOn(0))
    {
        model->StopAnimation(0);
        anim0_on = false;
        stage_blending = false;
        prev_track = -1;
        anim0_once = false;
    }
    if (anim1_on && anim1_once && !model->AnimIsOn(1))
    {
        model->StopAnimation(1);
        anim1_on = false;
        stage_blending = false;
        prev_track = -1;
        anim1_once = false;
    }
    if (stage_blending)
    {
        // The parentheses keep the constant's product first (fmul .001, then the speed), as the original
        blend += (time * .001f) * blend_speed;
        if (blend >= 1.0f)
        {
            blend = 1.0f;
            stage_blending = false;
            prev_track = -1;
            if (anim1_on)
            {
                model->StopAnimation(1);
                anim1_on = false;
            }
            if (anim0_on && anim0_stop_blended)
            {
                model->StopAnimation(0);
                anim0_on = false;
            }
        }
        else if (blend <= 0.0f)
        {
            blend = 0.0f;
            stage_blending = false;
            prev_track = -1;
            if (anim0_on)
            {
                model->StopAnimation(0);
                anim0_on = false;
            }
            if (anim1_on && anim1_stop_blended)
            {
                model->StopAnimation(1);
                anim1_on = false;
            }
        }
        float blend1 = 1.0f - blend;
        model->SetAnimBlend(0, blend);
        model->SetAnimBlend(1, blend1);
    }
    S_vector pos;
    if (track != -1)
    {
        S_vector dir;
        track_time += time;
        if (track_time > tracks[track].length)
            track_time = tracks[track].length;
        uint key = track_time / tracks[track].key_time;
        uint next = key + 1;
        if (next < tracks[track].positions.size())
        {
            int rest = track_time - key * tracks[track].key_time;
            int left = tracks[track].key_time - rest;
            pos = tracks[track].positions[key] * (float)left + tracks[track].positions[key + 1] * (float)rest;
            pos *= 1.0f / tracks[track].key_time;
            if (tracks[track].has_directions)
            {
                dir = tracks[track].directions[key] * (float)left + tracks[track].directions[key + 1] * (float)rest;
                dir *= 1.0f / tracks[track].key_time;
                dir *= track_matrix;
            }
        }
        else
        {
            pos = tracks[track].positions[key];
            if (tracks[track].has_directions)
            {
                dir = tracks[track].directions[key];
                dir *= track_matrix;
            }
        }
        pos *= track_matrix;
        pos += track_pos;
        pos += track_offset;
        if (land_height != 0.0f)
            pos.y += GetLandOffset();
        if (prev_track != -1)
        {
            bool prev_ended = false;
            prev_blend += time * prev_blend_speed;
            if (prev_blend >= 1.0f)
            {
                prev_ended = true;
                prev_blend = 1.0f;
            }
            float prev_weight = 1.0f - prev_blend;
            prev_track_time += time;
            if (prev_track_time > tracks[prev_track].length)
                prev_track_time = tracks[prev_track].length;
            uint prev_key = prev_track_time / tracks[prev_track].key_time;
            S_vector prev_pos;
            uint prev_next = prev_key + 1;
            if (prev_next < tracks[prev_track].positions.size())
            {
                int rest = prev_track_time - prev_key * tracks[prev_track].key_time;
                int left = tracks[prev_track].key_time - rest;
                prev_pos = tracks[prev_track].positions[prev_key] * (float)left +
                    tracks[prev_track].positions[prev_key + 1] * (float)rest;
                prev_pos *= 1.0f / tracks[prev_track].key_time;
            }
            else
                prev_pos = tracks[prev_track].positions[prev_key];
            prev_pos *= prev_track_matrix;
            prev_pos += prev_track_pos;
            prev_pos += track_offset;
            if (land_height != 0.0f)
                prev_pos.y += GetLandOffset();
            pos = pos * prev_blend + prev_pos * prev_weight;
            if (prev_ended)
                prev_track = -1;
        }
        // The actor's setters take the vector by value (all three loaded, then stored)
        if (tracks[track].has_directions)
        {
            if (actor)
                actor->SetDir(dir);
            else if (element)
                element->dir = dir;
            else
                model->SetDir(dir, 0.0f);
        }
        if (actor)
            actor->SetPos(pos);
        else if (element)
            element->pos = pos;
        else
        {
            model->SetPos(pos);
            model->Update();
        }
        if (track_time > tracks[track].length - 250)
            flags |= ANIMTICK_END;
    }
    else if (prev_track != -1)
    {
        prev_track_time += time;
        bool prev_ended = false;
        if (prev_track_time > tracks[prev_track].length)
        {
            prev_track_time = tracks[prev_track].length;
            prev_ended = true;
        }
        uint key = prev_track_time / tracks[prev_track].key_time;
        uint next = key + 1;
        if (next < tracks[prev_track].positions.size())
        {
            int rest = prev_track_time - key * tracks[prev_track].key_time;
            int left = tracks[prev_track].key_time - rest;
            pos = tracks[prev_track].positions[key] * (float)left + tracks[prev_track].positions[key + 1] * (float)rest;
            pos *= 1.0f / tracks[prev_track].key_time;
        }
        else
            pos = tracks[prev_track].positions[key];
        pos *= prev_track_matrix;
        pos += prev_track_pos;
        pos += track_offset;
        if (prev_ended)
            prev_track = -1;
        if (actor)
            actor->SetPos(pos);
        else if (element)
            element->pos = pos;
        else
        {
            model->SetPos(pos);
            model->Update();
        }
    }
    if (correcting)
    {
        correction += time * correction_speed;
        if (correction >= 1.0f)
        {
            correction = 1.0f;
            track_pos += correction_offset;
            correcting = false;
            // Track 0, not -1 (PS2's tests 0 too)
            if (!track)
                prev_track = -1;
        }
        S_vector corrected = actor ? actor->GetPos() : element ? element->GetPos() : model->GetPos();
        corrected += correction_offset * correction;
        if (actor)
            actor->SetPos(corrected);
        else if (element)
            element->pos = corrected;
        else
        {
            model->SetPos(corrected);
            model->Update();
        }
    }
    if (ext_fading && ext_on)
    {
        // The flag is threaded away (PS2 keeps it)
        bool set_blend = true;
        ext_blend += (time * .001f) * ext_speed;
        if (ext_speed < 0.0f)
        {
            if (ext_blend <= ext_target)
            {
                ext_on = false;
                ext_fading = false;
                model->StopAnimation(2);
                set_blend = false;
                if (actor && (actor->type == ACTOR_ENTITY || actor->type == ACTOR_PLAYER) &&
                    ((C_human *)actor)->weapon_displaced)
                    ((C_human *)actor)->RepairWeaponPosition();
            }
        }
        else if (ext_blend >= ext_target)
        {
            ext_blend = ext_target;
            ext_fading = false;
        }
        if (set_blend)
            model->SetAnimBlend(2, ext_blend);
    }
    if (ext_once && ext_on)
    {
        ext_end -= time;
        if (ext_end <= 0)
            DelExtendedAnim(4.0f);
    }
    return flags;
}

// FUNCTION: GAME 0x410060
void C_animations_machine_de_luxe::SetExtendedAnim(I3D_animation_set *set, float target, float speed, bool once)
{
    if (disabled || !set)
        return;
    unknown_10c = false;
    unknown_10e = 0xffff;
    set->SetOptions(once ? I3DANIMOP_ONCE : I3DANIMOP_LOOP);
    if (speed > 0.0f)
    {
        model->SetAnimation(set, 2, 0.0f, 0);
        ext_fading = true;
        ext_blend = 0.0f;
        ext_target = target;
        ext_speed = speed;
    }
    else
    {
        model->SetAnimation(set, 2, target, 0);
        ext_fading = false;
    }
    ext_once = once;
    ext_on = true;
    if (once)
    {
        uint length = 0;
        I3D_animation_set *s = model->GetAnimationSet(2);
        if (s)
            length = s->GetEndTime();
        ext_end = length - 250;
        if (ext_end < 0)
            ext_end = length / 2;
    }
}

// FUNCTION: GAME 0x410150
void C_animations_machine_de_luxe::DelExtendedAnim(float speed)
{
    if (disabled || !ext_on)
        return;
    unknown_10e = 0xffff;
    if (speed > 0.0f)
    {
        ext_speed = -speed;
        ext_fading = true;
        ext_target = 0.0f;
        ext_once = false;
    }
    else
    {
        ext_on = false;
        ext_fading = false;
        model->StopAnimation(2);
        ext_once = false;
    }
}

// The placement a track leaves the actor in, relative to its start: the move from its last key to its first and the
// direction (0, 0, 1) turned by the angle between its first and last directions, the move turned with it
// FUNCTION: GAME 0x4101d0
bool C_animations_machine_de_luxe::GetPlacementFromTCK(const char *name, S_vector &pos, S_vector &dir)
{
    int i = LoadTrack(name);
    if (i == -1)
        return false;
    pos = tracks[i].positions[0] - tracks[i].positions.end()[-1];
    if (tracks[i].has_directions)
    {
        S_vector2 first(tracks[i].directions[0].x, tracks[i].directions[0].z);
        S_vector2 last(tracks[i].directions.end()[-1].x, tracks[i].directions.end()[-1].z);
        float angle = AngleTo2(last, first);
        dir = S_vector(0.0f, 0.0f, 1.0f);
        RotateDir(dir, angle);
        RotateDirNoNorm(pos, angle);
        return true;
    }
    dir = S_vector(0.0f, 0.0f, 1.0f);
    return true;
}

// FUNCTION: GAME 0x410310
bool C_animations_machine_de_luxe::GetEndPointOfTCK(const char *name, S_vector &pos)
{
    int i = LoadTrack(name);
    if (i == -1)
        return false;
    pos = tracks[i].positions.end()[-1];
    return true;
}

// FUNCTION: GAME 0x410350
bool C_animations_machine_de_luxe::GetMixingPointOfTCK(const char *name, S_vector &pos)
{
    int i = LoadTrack(name);
    if (i != -1)
    {
        S_track &t = tracks[i];
        if ((uint)t.length > 250)
        {
            pos = t.positions[t.positions.size() - 250 / t.key_time];
            return true;
        }
    }
    return false;
}

// FUNCTION: GAME 0x4103d0
void C_animations_machine_de_luxe::StageBlendingTick(uint time)
{
    if (disabled || !model || !stage_blending)
        return;
    blend += (time * .001f) * blend_speed;
    if (blend >= 1.0f)
    {
        blend = 1.0f;
        stage_blending = false;
        prev_track = -1;
        if (anim1_on)
        {
            model->StopAnimation(1);
            anim1_on = false;
        }
        if (anim0_on && anim0_stop_blended)
        {
            model->StopAnimation(0);
            anim0_on = false;
        }
    }
    else if (blend <= 0.0f)
    {
        blend = 0.0f;
        stage_blending = false;
        prev_track = -1;
        if (anim0_on)
        {
            model->StopAnimation(0);
            anim0_on = false;
        }
        if (anim1_on && anim1_stop_blended)
        {
            model->StopAnimation(1);
            anim1_on = false;
        }
    }
    float blend1 = 1.0f - blend;
    model->SetAnimBlend(0, blend);
    model->SetAnimBlend(1, blend1);
    model->Tick(0);
}
