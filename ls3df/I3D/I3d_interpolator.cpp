// The interpolators (PS2 I3d_interpolator.cpp), at 0x1001d2f0-0x1001ed30 in the original: I3D_interpolator, then
// I3D_IPolSet, each in PS2's order. The header's zero vector has its $E after I3D_IPolSet::SetPose(const char *,
// I3D_pose *) (0x1001e840); ours comes two functions earlier, after SetAnimation (the original's headers had more
// inline code before the vector, see autopch.py). Names are the PS2 build's unless noted.
#include "I3D/I3D.h"
#include "IGraph/IGraph.h"

#include <string.h>
// <new> declares operator delete throw(), so VC6 sets no EH state before the list's destructor in the set's
#include <new>

// The constructor and destructor are defined first, as on PS2, where they are out of line. VC6 inlines them wherever
// they are called and the linker drops the copies, but compiling them first puts the vtable before Update's constant
// in .rdata, as in the original (VC6 emits a vtable with the first function that uses it).
I3D_interpolator::I3D_interpolator(I3D_IPolSet *set)
{
    frame = NULL;
    pose = NULL;
    pose_blend = 1.0f;
    ipol_set = set;
    memset(keys, 0, sizeof(keys));
}

I3D_interpolator::~I3D_interpolator()
{
    if (pose)
        pose->Release();
}

// FUNCTION: LS3DF 0x1001d2f0
void I3D_interpolator::SetPoseBlend(float blend)
{
    pose_blend = blend;
}

// FUNCTION: LS3DF 0x1001d300
LS3D_RESULT I3D_interpolator::ClearAnimation(uint index)
{
    keys[index].flags = 0;
    return I3D_OK;
}

// The key time of an animation time (the keys are 1/25 s apart) and a track's values, after its key times, aligned to
// 4 bytes (the names are guesses). Update reads every track through them: their calls after the pose code cut the
// inline share of the pose's quaternion products to the original's (see Update).
static inline ushort KeyTime(int time)
{
    return time / 40;
}

static inline const void *KeyValues(const ushort *times, int n)
{
    return n & 1 ? times + n : times + n + 1;
}

// Blends the animations' keys at their times into the frame's position, rotation and scale, each animation by its
// blend, then the pose by pose_blend, and tells the frame's callback of the note keys passed since the last update. A
// track's value between two keys is interpolated (linearly, the rotations spherically); before the first key it is the
// first, after the last the last. The first animation that moves a part replaces the frame's, the following ones blend
// into it. The three tracks' loops are written out, the rotation's with a result pointer of its own. The interpolated
// values are block locals, assigned: the rotation's shares its area with the pose code's quaternion, as the original's.
// The inline budget: the original's full pose leaves the vector operators of its quaternion product out of line, the
// blended one only the constructors. The pose's accessors and the key helpers' calls give that; without them ours
// inlines more.
// Not matching yet: registers and a few stack slots all through. The original keeps t_last in bx and tests the keys'
// flags from memory twice where ours keeps them in bx, and merges the blended position's and scale's stores with the
// copies' at the join.
// FUNCTION: LS3DF 0x1001d320
void I3D_interpolator::Update()
{
    if (!frame)
        return;
    S_vector pos, scale;
    S_quat rot;
    int i;

    int num_pos = 0;
    for (i = 0; i < 8; i++) {
        if (keys[i].flags & ANIMF_POS) {
            if (!(ipol_set->anims[i].blend <= 0.0f) && ipol_set->anims[i].on) {
                S_vector v_ipol;
                ushort t = KeyTime(ipol_set->anims[i].time);
                const ushort *times = keys[i].pos;
                int n = keys[i].num_pos;
                const S_vector *v = (const S_vector *)KeyValues(times, n);
                if (times[0] <= t) {
                    if (times[--n] <= t)
                        v += n;
                    else {
                        int t0, t1;
                        const S_vector *v0, *v1;
                        if (n == 1) {
                            t1 = times[1];
                            v0 = v;
                            v1 = v + 1;
                            t0 = times[0];
                        } else {
                            int j;
                            for (j = 1, times++; j < n; times++, j++) {
                                if (*times > t)
                                    break;
                            }
                            t0 = times[-1];
                            t1 = times[0];
                            v1 = v + j;
                            v0 = v1 - 1;
                        }
                        float f = (ipol_set->anims[i].time * 0.025f - t0) / (t1 - t0);
                        v_ipol = (*v1 - *v0) * f + *v0;
                        v = &v_ipol;
                    }
                }
                if (num_pos && !(ipol_set->anims[i].blend >= 1.0f)) {
                    float b = ipol_set->anims[i].blend;
                    pos = *v * b + pos * (1.0f - b);
                } else
                    pos = *v;
                num_pos++;
            }
        }
    }

    int num_rot = 0;
    for (i = 0; i < 8; i++) {
        if (ipol_set->anims[i].on && (keys[i].flags & ANIMF_ROT) && !(ipol_set->anims[i].blend <= 0.0f)) {
            S_quat q_ipol;
            ushort t = KeyTime(ipol_set->anims[i].time);
            const ushort *times = keys[i].rot;
            int n = keys[i].num_rot;
            const S_quat *q = (const S_quat *)KeyValues(times, n);
            const S_quat *p;
            if (times[0] > t)
                p = q;
            else if (times[--n] <= t)
                p = q + n;
            else {
                int t0, t1;
                const S_quat *q0, *q1;
                if (n == 1) {
                    t1 = times[1];
                    q0 = q;
                    q1 = q + 1;
                    t0 = times[0];
                } else {
                    int j;
                    for (j = 1, times++; j < n; times++, j++) {
                        if (*times > t)
                            break;
                    }
                    t0 = times[-1];
                    t1 = times[0];
                    q1 = q + j;
                    q0 = q1 - 1;
                }
                q_ipol = q0->Slerp(*q1, (ipol_set->anims[i].time * 0.025f - t0) / (t1 - t0), false);
                p = &q_ipol;
            }
            if (num_rot && !(ipol_set->anims[i].blend >= 1.0f)) {
                rot = rot.Slerp(*p, ipol_set->anims[i].blend, false);
                num_rot++;
            } else {
                rot = *p;
                num_rot = 1;
            }
        }
    }

    int num_scale = 0;
    for (i = 0; i < 8; i++) {
        if (keys[i].flags & ANIMF_SCALE) {
            if (!(ipol_set->anims[i].blend <= 0.0f) && ipol_set->anims[i].on) {
                S_vector v_ipol;
                ushort t = KeyTime(ipol_set->anims[i].time);
                const ushort *times = keys[i].scale;
                int n = keys[i].num_scale;
                const S_vector *v = (const S_vector *)KeyValues(times, n);
                if (times[0] <= t) {
                    if (times[--n] <= t)
                        v += n;
                    else {
                        int t0, t1;
                        const S_vector *v0, *v1;
                        if (n == 1) {
                            t1 = times[1];
                            v0 = v;
                            v1 = v + 1;
                            t0 = times[0];
                        } else {
                            int j;
                            for (j = 1, times++; j < n; times++, j++) {
                                if (*times > t)
                                    break;
                            }
                            t0 = times[-1];
                            t1 = times[0];
                            v1 = v + j;
                            v0 = v1 - 1;
                        }
                        float f = (ipol_set->anims[i].time * 0.025f - t0) / (t1 - t0);
                        v_ipol = (*v1 - *v0) * f + *v0;
                        v = &v_ipol;
                    }
                }
                if (num_scale && !(ipol_set->anims[i].blend >= 1.0f)) {
                    float b = ipol_set->anims[i].blend;
                    scale = *v * b + scale * (1.0f - b);
                } else
                    scale = *v;
                num_scale++;
            }
        }
    }

    // The pose: in full it replaces only what an animation moves (a rotation always), blended it adds to it
    if (pose && pose_blend > 0.0f) {
        uint flags = pose->pose_flags;
        if (pose_blend >= 1.0f) {
            if (flags & POSEF_POS)
                pos = pose->GetPos1();
            if (flags & POSEF_SCALE)
                scale = pose->GetScale1();
            if (flags & POSEF_ROT_ADD) {
                if (num_rot) {
                    num_rot++;
                    rot = pose->GetRot1() * rot;
                } else {
                    rot = pose->GetRot1();
                    num_rot = 1;
                }
            } else if (flags & POSEF_ROT) {
                rot = pose->GetRot1();
                num_rot = 1;
            }
        } else {
            if (flags & POSEF_POS) {
                if (num_pos)
                    pos = pose->GetPos1() * pose_blend + pos * (1.0f - pose_blend);
                else
                    pos = pose->GetPos1();
                num_pos++;
            }
            if (flags & POSEF_SCALE) {
                if (num_scale)
                    scale = pose->GetScale1() * pose_blend + scale * (1.0f - pose_blend);
                else
                    scale = pose->GetScale1();
                num_scale++;
            }
            if (flags & POSEF_ROT_ADD) {
                if (num_rot) {
                    num_rot++;
                    S_quat q = pose->GetRot1();
                    q.s *= pose_blend;
                    rot = q * rot;
                } else {
                    rot = pose->GetRot1();
                    num_rot = 1;
                }
            } else if (flags & POSEF_ROT) {
                if (num_rot)
                    rot = rot.Slerp(pose->GetRot1(), pose_blend, false);
                else
                    rot = pose->GetRot1();
                num_rot++;
            }
        }
    }

    if (num_pos)
        frame->SetPos(pos);
    if (num_rot) {
        if (num_rot > 1)
            rot.Normalize();
        I3D_frame *frm = frame;
        frm->rot = rot;
        frm->rot.Normalize();
        frm->frm_flags = (frm->frm_flags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID | FRMFLAGS_MAT_VALID |
            FRMFLAGS_ROT_VALID)) | FRMFLAGS_UPDATE | FRMFLAGS_QUAT_VALID;
    }
    if (num_scale)
        frame->SetScale(scale);

    // The note keys after the last update's time up to this one's, in keys, or past an end when the time went around.
    // Played backwards, only the latter: the tests are the forward ones.
    if (!(frame->frm_flags & (LS3D_CALLBACK_NOTE | LS3D_CALLBACK_NOTE_STRING)) || !frame->callback)
        return;
    for (i = 0; i < 8; i++) {
        if (!(keys[i].flags & (ANIMF_NOTE | ANIMF_NOTE_STRING)))
            continue;
        if (!ipol_set->anims[i].on)
            continue;
        ushort t = KeyTime(ipol_set->anims[i].time);
        ushort t_last = KeyTime(ipol_set->anims[i].last_time);
        if (t == t_last)
            continue;
        if (keys[i].flags & ANIMF_NOTE_STRING) {
            if (!(frame->frm_flags & LS3D_CALLBACK_NOTE_STRING))
                continue;
            const ushort *times = keys[i].note;
            int n = keys[i].num_note;
            const char *const *notes = (const char *const *)KeyValues(times, n);
            bool wrap = ipol_set->anims[i].delta >= 0 ? t < t_last : t >= t_last;
            for (int j = 0; j < n; j++, times++) {
                ushort kt = *times;
                if (wrap ? kt > t_last || kt <= t : kt > t_last && kt <= t)
                    frame->callback(frame, LS3D_CALLBACK_NOTE_STRING, (uint)notes[j], kt * 40);
            }
        } else {
            if (!(frame->frm_flags & LS3D_CALLBACK_NOTE))
                continue;
            const ushort *times = keys[i].note;
            int n = keys[i].num_note;
            const ushort *notes = times + n;
            bool wrap = ipol_set->anims[i].delta >= 0 ? t < t_last : t >= t_last;
            for (int j = 0; j < n; j++, times++) {
                ushort kt = *times;
                if (wrap ? kt > t_last || kt <= t : kt > t_last && kt <= t)
                    frame->callback(frame, LS3D_CALLBACK_NOTE, notes[j], kt * 40);
            }
        }
    }
}

// The set is told of the change; a pose leaving is released, one coming kept
// FUNCTION: LS3DF 0x1001e230
void I3D_interpolator::SetPose(I3D_pose *p)
{
    if (p == pose)
        return;
    ipol_set->changed = true;
    if (!p) {
        pose->Release();
        pose = NULL;
        return;
    }
    if (pose)
        pose->Release();
    pose = p;
    p->ref++;
}

// FUNCTION: LS3DF 0x1001e280
I3D_IPolSet::~I3D_IPolSet()
{
    Clear();
}

// The original calls it from the destructor: VC6 inlines it automatically unless it is a little larger than it is
// without the local in the first loop
// FUNCTION: LS3DF 0x1001e2e0
void I3D_IPolSet::Clear()
{
    I3D_interpolator **p = &interpolators[0];
    for (int i = interpolators.size(); i--; p++) {
        I3D_interpolator *ip = *p;
        delete ip;
    }
    interpolators.reset();
    for (int j = 0; j < 8; j++) {
        if (anims[j].set)
            anims[j].set->Release();
    }
    memset(anims, 0, sizeof(anims));
}

// Advances each animation that is on by its time, then updates the interpolators. An animation that reaches an end
// tells the callback of the first frame that has one for it, and starts again at the other end if it loops, else
// stops.
// FUNCTION: LS3DF 0x1001e360
void I3D_IPolSet::Tick(const int *times)
{
    int i;
    for (i = 0; i < 8; i++) {
        if (times[i]) {
            changed = true;
            if (anims[i].on) {
                anims[i].time += times[i];
                anims[i].delta = times[i];
            }
        }
    }
    if (!changed)
        return;
    changed = false;
    I3D_interpolator **p = &interpolators[0];
    for (int j = interpolators.size(); j--; p++)
        (*p)->Update();
    for (i = 0; i < 8; i++) {
        if (!anims[i].on)
            continue;
        anims[i].last_time = anims[i].time;
        if (anims[i].time >= 0 && anims[i].time < anims[i].set->GetEndTime())
            continue;
        p = &interpolators[0];
        for (int k = interpolators.size(); k--; p++) {
            I3D_frame *frm = (*p)->frame;
            if (frm && (frm->frm_flags & LS3D_CALLBACK_ANIM_END)) {
                frm->callback(frm, LS3D_CALLBACK_ANIM_END, i, 0);
                break;
            }
        }
        switch (anims[i].options) {
        case I3DANIMOP_LOOP:
            if (anims[i].time < 0)
                anims[i].time += anims[i].set->GetEndTime();
            else
                anims[i].time -= anims[i].set->GetEndTime();
            break;
        default:
            anims[i].on = false;
        }
    }
}

// The same with one time for all the animations
// FUNCTION: LS3DF 0x1001e470
void I3D_IPolSet::Tick(int time)
{
    int i;
    if (time) {
        changed = true;
        for (i = 0; i < 8; i++) {
            if (anims[i].on) {
                anims[i].time += time;
                anims[i].delta = time;
            }
        }
    }
    if (!changed)
        return;
    changed = false;
    I3D_interpolator **p = &interpolators[0];
    for (int j = interpolators.size(); j--; p++)
        (*p)->Update();
    for (i = 0; i < 8; i++) {
        if (!anims[i].on)
            continue;
        anims[i].last_time = anims[i].time;
        if (anims[i].time >= 0 && anims[i].time < anims[i].set->GetEndTime())
            continue;
        p = &interpolators[0];
        for (int k = interpolators.size(); k--; p++) {
            I3D_frame *frm = (*p)->frame;
            if (frm && (frm->frm_flags & LS3D_CALLBACK_ANIM_END)) {
                frm->callback(frm, LS3D_CALLBACK_ANIM_END, i, 0);
                break;
            }
        }
        switch (anims[i].options) {
        case I3DANIMOP_LOOP:
            if (anims[i].time < 0)
                anims[i].time += anims[i].set->GetEndTime();
            else
                anims[i].time -= anims[i].set->GetEndTime();
            break;
        default:
            anims[i].on = false;
        }
    }
}

// Plays the set's animation as the index-th from its start, its frames found among the model's or scene's by their own
// names; NULL stops the index-th. The interpolators keep pointers into the set's keys. The options are the set's
// where none are given.
// FUNCTION: LS3DF 0x1001e580
LS3D_RESULT I3D_IPolSet::SetAnimation(I3D_animation_set *set, uint index, float blend, uint options)
{
    if (anims[index].set != set) {
        if (anims[index].set) {
            I3D_interpolator **p = &interpolators[0];
            for (int i = interpolators.size(); i--; p++)
                (*p)->keys[index].flags = 0;
            anims[index].set->Release();
        }
        anims[index].set = set;
        if (!set) {
            anims[index].on = false;
            m_releaseUnusedInterpolators();
            return I3D_OK;
        }
        set->AddRef();
    } else if (!set)
        return I3D_OK;
    anims[index].on = true;
    anims[index].time = 0;
    anims[index].last_time = -40;
    if (!options)
        options = set->options;
    anims[index].options = options;
    anims[index].blend = blend;
    const S_frame_anim *anim = set->GetAnims();
    for (int i = set->NumAnimations(); i; anim++, i--) {
        I3D_frame **fp;
        int n;
        if (frame->GetType() == FRAME_MODEL) {
            fp = &((I3D_model *)frame)->frames[0];
            n = ((I3D_model *)frame)->frames.size();
        } else {
            fp = &((I3D_scene *)frame)->owned_frames[0];
            n = ((I3D_scene *)frame)->owned_frames.size();
        }
        for (; n--; fp++) {
            if (strcmp((*fp)->GetOrigName(), anim->frame_name))
                continue;
            I3D_interpolator *ip = GetFrameInterpolator(*fp);
            if (!ip)
                return I3DERR_OUTOFMEM;
            S_anim_keys &k = ip->keys[index];
            const ushort *data = anim->keys;
            k.flags = *data;
            data += 2;
            if (k.flags & ANIMF_ROT) {
                k.num_rot = *data++;
                k.rot = data;
                data += k.num_rot * 9;
                if (!(k.num_rot & 1))
                    data++;
            } else
                k.num_rot = 0;
            if (k.flags & ANIMF_POS) {
                k.num_pos = *data++;
                k.pos = data;
                data += k.num_pos * 7;
                if (!(k.num_pos & 1))
                    data++;
            } else
                k.num_pos = 0;
            if (k.flags & ANIMF_SCALE) {
                k.num_scale = *data++;
                k.scale = data;
                data += k.num_scale * 7;
                if (!(k.num_scale & 1))
                    data++;
            } else
                k.num_scale = 0;
            if (k.flags & (ANIMF_NOTE | ANIMF_NOTE_STRING)) {
                k.num_note = *data++;
                k.note = data;
            } else
                k.num_note = 0;
            break;
        }
    }
    m_releaseUnusedInterpolators();
    changed = true;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1001e7e0
LS3D_RESULT I3D_IPolSet::SetPose(I3D_frame *frm, I3D_pose *pose)
{
    I3D_interpolator *ip = GetFrameInterpolator(frm);
    if (!ip)
        return I3DERR_OBJECTNOTFOUND;
    ip->SetPose(pose);
    changed = true;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1001e810
LS3D_RESULT I3D_IPolSet::SetPose(const char *frame_name, I3D_pose *pose)
{
    I3D_interpolator *ip = GetFrameInterpolator(frame_name);
    if (!ip)
        return I3DERR_OBJECTNOTFOUND;
    ip->SetPose(pose);
    changed = true;
    return I3D_OK;
}

// Each pose to the frame of its name
// FUNCTION: LS3DF 0x1001e860
LS3D_RESULT I3D_IPolSet::SetPose(I3D_pose_set *set)
{
    I3D_pose **p = &set->poses[0];
    for (int i = set->poses.size(); i--; p++) {
        I3D_interpolator *ip = GetFrameInterpolator((*p)->GetName());
        if (!ip)
            return I3DERR_OBJECTNOTFOUND;
        ip->SetPose(*p);
    }
    changed = true;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1001e8c0
void I3D_IPolSet::SetPoseBlend(float blend)
{
    I3D_interpolator **p = &interpolators[0];
    for (int i = interpolators.size(); i--; p++)
        (*p)->SetPoseBlend(blend);
    changed = true;
}

// The blend of the frames of the set's poses
// FUNCTION: LS3DF 0x1001e900
LS3D_RESULT I3D_IPolSet::SetPoseBlend(float blend, I3D_pose_set *set)
{
    changed = true;
    I3D_pose **p = &set->poses[0];
    for (int i = set->poses.size(); i--; p++) {
        I3D_interpolator *ip = GetFrameInterpolator((*p)->GetName());
        if (!ip)
            return I3DERR_OBJECTNOTFOUND;
        ip->SetPoseBlend(blend);
    }
    return I3D_OK;
}

// The first interpolator's with a pose
// FUNCTION: LS3DF 0x1001e960
float I3D_IPolSet::GetPoseBlend() const
{
    I3D_interpolator *const *p = &interpolators[0];
    for (int i = interpolators.size(); i--; p++) {
        if ((*p)->pose)
            return (*p)->pose_blend;
    }
    return 0.0f;
}

// FUNCTION: LS3DF 0x1001e9a0
void I3D_IPolSet::ResetAllPoses()
{
    I3D_interpolator **p = &interpolators[0];
    for (int i = interpolators.size(); i--; p++) {
        if ((*p)->pose)
            (*p)->SetPose(NULL);
    }
    changed = true;
}

// Deletes the interpolators without a pose that none of the animations playing moves. The list is cut at the deleted
// one's index, not closed up as PS2's: the ones after it are lost. Like Clear, it is at the size VC6 inlines
// automatically: written with || (the same code) SetAnimation inlines both calls.
// FUNCTION: LS3DF 0x1001e9e0
void I3D_IPolSet::m_releaseUnusedInterpolators()
{
    int i = interpolators.size();
    while (i--) {
        I3D_interpolator *ip = interpolators[i];
        if (ip->pose)
            continue;
        bool used = false;
        for (int j = 8; j--; ) {
            if (anims[j].on)
                used = true;
            else if (ip->keys[j].flags)
                used = true;
        }
        if (used)
            continue;
        delete ip;
        interpolators.used_size = i;
    }
}

// The frame's interpolator, made if it has none
// FUNCTION: LS3DF 0x1001ea60
I3D_interpolator *I3D_IPolSet::GetFrameInterpolator(I3D_frame *frm)
{
    I3D_interpolator **p = &interpolators[0];
    for (int i = interpolators.size(); i--; p++) {
        if ((*p)->frame == frm)
            return *p;
    }
    I3D_interpolator *ip = new I3D_interpolator(this);
    if (!ip) {
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
        return NULL;
    }
    interpolators.push_back(ip);
    ip->frame = frm;
    frm->frm_flags |= FRMFLAGS_INTERPOLATOR;
    return ip;
}

// The interpolator of the frame of that name (its own, without the model's), made if it has none; NULL if the model or
// scene has no such frame
// FUNCTION: LS3DF 0x1001eba0
I3D_interpolator *I3D_IPolSet::GetFrameInterpolator(const char *frame_name)
{
    I3D_interpolator **p = &interpolators[0];
    for (int i = interpolators.size(); i--; p++) {
        if (!strcmp((*p)->frame->GetOrigName(), frame_name))
            return *p;
    }
    I3D_frame *frm = frame->FindChildFrame(frame_name, ENUMF_ALL | ENUMF_CASESENS);
    if (!frm)
        return NULL;
    I3D_interpolator *ip = new I3D_interpolator(this);
    if (!ip) {
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
        return NULL;
    }
    interpolators.push_back(ip);
    ip->frame = frm;
    frm->frm_flags |= FRMFLAGS_INTERPOLATOR;
    return ip;
}

// The functions of other files the interpolators call, until those are written. They come after the file's functions
// (see I3d_driver.cpp); their markers are in I3D.h, by name, as their addresses come before the file's.
#pragma auto_inline(off)

const S_frame_anim *I3D_animation_set::GetAnims() const
{
    return data ? (const S_frame_anim *)((char *)data + 4) : NULL;
}

#pragma auto_inline(on)
