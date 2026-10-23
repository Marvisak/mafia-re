// The models (PS2 I3d_model.cpp), at 0x10033410-0x10034350 in the original (1.1's 0x100323e0-0x100333d0): I3D_model in
// PS2's order. The duplication's globals are defined before duplicateChildren, where their $Es are
// (0x10033b50-0x10033bef). The header's zero vector has its $E at the end (0x10034330). Names are the PS2 build's
// unless noted. 1.1 prints its errors with dbgPrintf (out of memory, a frame or target not found, a failed
// duplicate), where 1.2 is silent, and its SetAnimTime leaves last_time.
#include "I3D/I3D.h"
#include "IGraph/IGraph.h"

#include <string.h>
// <new> declares operator delete throw(), so VC6 sets no EH state before the frame list's destructor in the model's
// (as IGraph.cpp's)
#include <new>

// FUNCTION: LS3DF 0x10033410
// FUNCTION: LS3DF11 0x100323e0
I3D_model::I3D_model()
{
    file_name = NULL;
    ipol_set = NULL;
    m_Type = FRAME_MODEL;
}

// FUNCTION: LS3DF 0x10033480
// FUNCTION: LS3DF11 0x10032450
I3D_model::~I3D_model()
{
    Close();
}

// FUNCTION: LS3DF 0x10033500
// FUNCTION: LS3DF11 0x100324d0
void I3D_model::Close()
{
    delete ipol_set;
    ipol_set = NULL;
    I3D_frame **p = &frames[0];
    for (int i = frames.size(); i--; p++) {
        (*p)->m_pOwner = NULL;
        (*p)->Release();
    }
    frames.reset();
    if (file_name) {
        int *ref = (int *)file_name - 1;
        if (--*ref <= 0)
            delete[] (char *)ref;
        file_name = NULL;
    }
    DiscardHRBoundVol();
}

// Named frames go into the fast find tree under the model's name (SetName), not their own
// FUNCTION: LS3DF 0x100335a0
// FUNCTION: LS3DF11 0x10032570
LS3D_RESULT I3D_model::Open(const char *name, uint flags, I3D_LOAD_CB_PROC *cb, void *context)
{
    Close();
    int fast_find = renderState[RS_FAST_FIND];
    if (*GetName())
        renderState[RS_FAST_FIND] = 0;
    LS3D_RESULT r = LoadHierarchy(name, flags, cb, context, this);
    renderState[RS_FAST_FIND] = fast_find;
    if (r < 0)
        return r;
    int *ref = (int *)new char[strlen(name) + 1 + sizeof(int)];
    if (!ref) {
#if ENGINE_VERSION < 395
        dbgPrintf("!! Out of memory  I3D_model::Open()");
#endif
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
        return I3DERR_OUTOFMEM;
    }
    *ref = 1;
    file_name = (char *)(ref + 1);
    strcpy(file_name, name);
    if (!*GetName())
        return I3D_OK;
    return SetName();
}

// FUNCTION: LS3DF 0x10033680
// FUNCTION: LS3DF11 0x10032660
void I3D_model::AddFrame(I3D_frame *frame)
{
    frame->m_RefCount++;
    frame->DeleteFromOwner();
    frames.push_back(frame);
    frame->m_pOwner = this;
}

// FUNCTION: LS3DF 0x10033720
// FUNCTION: LS3DF11 0x10032700
LS3D_RESULT I3D_model::DeleteFrame(I3D_frame *frame)
{
    if (frames.remove(frame)) {
        frame->m_pOwner = NULL;
        if (frame->m_RefCount > 0)
            frame->Release();
        return I3D_OK;
    }
#if ENGINE_VERSION < 395
    dbgPrintf("!! frame not found in the list  pFrame:0x%x  %s   I3D_model::DeleteFrame()", frame, frame->GetName());
#endif
    return I3DERR_OBJECTNOTFOUND;
}

// FUNCTION: LS3DF 0x100337b0
// FUNCTION: LS3DF11 0x100327b0
LS3D_RESULT I3D_model::SetAnimation(I3D_animation_set *set, uint index, float blend, uint options)
{
    if (!ipol_set) {
        ipol_set = new I3D_IPolSet(this);
        if (!ipol_set) {
#if ENGINE_VERSION < 395
            dbgPrintf("!! Out of memory  I3D_model::SetAnimation()");
#endif
            ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
            return I3DERR_OUTOFMEM;
        }
    }
    return ipol_set->SetAnimation(set, index, blend, options);
}

// FUNCTION: LS3DF 0x10033860
// FUNCTION: LS3DF11 0x10032870
I3D_animation_set *I3D_model::GetAnimationSet(uint index) const
{
    if (index >= 8)
        return NULL;
    if (!ipol_set)
        return NULL;
    return ipol_set->anims[index].set;
}

// FUNCTION: LS3DF 0x10033890
// FUNCTION: LS3DF11 0x100328a0
void I3D_model::StopAnimation(uint index)
{
    if (index >= 8)
        return;
    if (!ipol_set)
        return;
    ipol_set->anims[index].on = false;
}

// FUNCTION: LS3DF 0x100338c0
// FUNCTION: LS3DF11 0x100328d0
void I3D_model::StartAnimation(uint index)
{
    if (index >= 8)
        return;
    if (!ipol_set)
        return;
    ipol_set->anims[index].on = true;
}

// FUNCTION: LS3DF 0x100338f0
// FUNCTION: LS3DF11 0x10032900
LS3D_RESULT I3D_model::SetAnimBlend(uint index, float blend)
{
    if (index >= 8)
        return I3DERR_INVALIDPARAMS;
    I3D_IPolSet *ips = ipol_set;
    if (!ips)
        return I3DERR_OBJECTNOTFOUND;
    float b = blend;
    if (blend < 0.0f)
        b = 0.0f;
    else if (blend > 1.0f)
        b = 1.0f;
    ips->anims[index].blend = b;
    ips->changed = true;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10033970
// FUNCTION: LS3DF11 0x10032980
float I3D_model::GetAnimBlend(uint index) const
{
    if (index >= 8)
        return 0.0f;
    if (!ipol_set)
        return 0.0f;
    return ipol_set->anims[index].blend;
}

// FUNCTION: LS3DF 0x100339b0
// FUNCTION: LS3DF11 0x100329c0
LS3D_RESULT I3D_model::SetAnimTime(uint index, int time)
{
    if (index >= 8)
        return I3DERR_INVALIDPARAMS;
    I3D_IPolSet *ips = ipol_set;
    if (!ips)
        return I3DERR_OBJECTNOTFOUND;
    ips->anims[index].time = time;
#if ENGINE_VERSION >= 395
    ips->anims[index].last_time = time;
#endif
    ips->changed = true;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x100339f0
// FUNCTION: LS3DF11 0x10032a00
int I3D_model::GetAnimTime(uint index) const
{
    if (index >= 8)
        return 0;
    if (!ipol_set)
        return 0;
    return ipol_set->anims[index].time;
}

// FUNCTION: LS3DF 0x10033a20
// FUNCTION: LS3DF11 0x10032a30
LS3D_RESULT I3D_model::SetAnimOptions(uint index, uint options)
{
    if (index >= 8)
        return I3DERR_INVALIDPARAMS;
    I3D_IPolSet *ips = ipol_set;
    if (!ips)
        return I3DERR_OBJECTNOTFOUND;
    ips->anims[index].options = options;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10033a60
// FUNCTION: LS3DF11 0x10032a70
uint I3D_model::GetAnimOptions(uint index) const
{
    if (index >= 8)
        return 0;
    if (!ipol_set)
        return 0;
    return ipol_set->anims[index].options;
}

// FUNCTION: LS3DF 0x10033a90
// FUNCTION: LS3DF11 0x10032aa0
bool I3D_model::AnimIsOn(uint index) const
{
    if (index >= 8)
        return false;
    if (!ipol_set)
        return false;
    return ipol_set->anims[index].on;
}

// FUNCTION: LS3DF 0x10033ac0
// FUNCTION: LS3DF11 0x10032ad0
LS3D_RESULT I3D_model::SetName()
{
    I3D_frame **p = &frames[0];
    const char *model_name = GetName();
    if (*model_name) {
        for (int i = frames.size(); i > 0; i--, p++) {
            LS3D_RESULT r = (*p)->SetModelName(model_name);
            if (r < 0)
                return r;
        }
    } else {
        for (int i = frames.size(); i > 0; i--, p++) {
            LS3D_RESULT r = (*p)->ResetModelName();
            if (r < 0)
                return r;
        }
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10033b30
// FUNCTION: LS3DF11 0x10032b40
LS3D_RESULT I3D_model::SetName(const char *new_name)
{
    LS3D_RESULT r = I3D_frame::SetName(new_name);
    if (r < 0)
        return r;
    return SetName();
}

// The duplication's state: the copies of the frames that had a target with their targets, the copies of the shadows,
// the model the copies go into and the one copied
// GLOBAL: LS3DF 0x101b4368
// GLOBAL: LS3DF11 0x101b1488
static C_list<TARGET_ITEM> DuplTargetList;

// GLOBAL: LS3DF 0x101b4378
// GLOBAL: LS3DF11 0x101b1498
TARGET_ITEM DuplTargetItem;

// GLOBAL: LS3DF 0x101b4380
// GLOBAL: LS3DF11 0x101b14a0
static C_list<I3D_shadow *> DuplShadowList;

// GLOBAL: LS3DF 0x101b4390
// GLOBAL: LS3DF11 0x101b14b0
static I3D_model *pSrcOwner;

// GLOBAL: LS3DF 0x101b4394
// GLOBAL: LS3DF11 0x101b14b4
static I3D_model *pDuplOwner;

// Copies src's children under dst, and theirs under the copies; a model's own models aren't copied
// FUNCTION: LS3DF 0x10033bf0
// FUNCTION: LS3DF11 0x10032c00
LS3D_RESULT duplicateChildren(I3D_frame *dst, I3D_frame *src)
{
    for (I3D_frame *frm = src->m_pChild; frm; frm = frm->m_pNext) {
        I3D_frame *dup;
        I3D_FRAME_TYPE t = frm->GetType();
        if (t == FRAME_VISUAL)
            dup = pI3DDriver->CreateVisual(((I3D_visual *)frm)->visual_type);
        else if (t != FRAME_MODEL)
            dup = pI3DDriver->CreateFrame(t);
        else
            continue;
        if (!dup) {
#if ENGINE_VERSION < 395
            dbgPrintf("!! Out of memory  I3D_model::Duplicate()");
#endif
            return I3DERR_OUTOFMEM;
        }
        LS3D_RESULT r = dup->Duplicate(frm);
        if (r < 0) {
#if ENGINE_VERSION < 395
            dbgPrintf("!! Error duplicating frame:0x%x %s  I3D_model::Duplicate()", frm, frm->GetName());
#endif
            return r;
        }
        if (frm->m_FrmFlags & FRMFLAGS_TARGET) {
            DuplTargetItem.target = I3D_target::GetTarget(frm);
            DuplTargetItem.frame = dup;
            DuplTargetList.push_back(DuplTargetItem);
        }
        if (dup->GetType() == FRAME_SHADOW)
            DuplShadowList.push_back((I3D_shadow *)dup);
        dst->AddChild(dup);
        pDuplOwner->AddFrame(dup);
        dup->Release();
        r = duplicateChildren(dup, frm);
        if (frm->GetType() == FRAME_VISUAL) {
            I3D_VISUAL_TYPE vt = ((I3D_visual *)frm)->visual_type;
            if (vt == I3D_VISUAL_SINGLEMESH || vt == I3D_VISUAL_SINGLEMORPH)
                ((I3D_singlemesh *)dup)->DBuild();
        }
        if (r < 0)
            return r;
    }
    return I3D_OK;
}

// The copies of the frames that had a target get the copy of that target, found by its name, and the shadows are set
// to their parents again
// FUNCTION: LS3DF 0x10033e10
// FUNCTION: LS3DF11 0x10032e50
LS3D_RESULT I3D_model::DuplicateHierarchy(I3D_model *model)
{
    if (!model)
        return I3DERR_INVALIDPARAMS;
    Close();
    if (model->file_name) {
        file_name = model->file_name;
        ((int *)file_name)[-1]++;
    }
    int fast_find = renderState[RS_FAST_FIND];
    renderState[RS_FAST_FIND] = 0;
    DuplTargetList.reset();
    DuplShadowList.reset();
    pDuplOwner = this;
    pSrcOwner = model;
    LS3D_RESULT r = duplicateChildren(this, model);
    renderState[RS_FAST_FIND] = fast_find;
    if (r < 0)
        return r;
    r = SetName();
    TARGET_ITEM *ti = &DuplTargetList[0];
    int i;
    for (i = DuplTargetList.size(); i--; ti++) {
        const char *target_name = ti->target->GetOrigName();
        I3D_frame *frm = NULL;
        I3D_frame **p = &frames[0];
        for (int j = frames.size(); j--; p++) {
            if (!strcmp((*p)->GetOrigName(), target_name)) {
                frm = *p;
                break;
            }
        }
#if ENGINE_VERSION >= 395
        if (frm)
            ((I3D_target *)frm)->AddToTarget(ti->frame);
#else
        if (!frm)
            dbgPrintf("!! Target:%s not found for %s  I3D_model::Duplicate(%s)", target_name, ti->frame->GetName(),
                model->GetName());
        else
            ((I3D_target *)frm)->AddToTarget(ti->frame);
#endif
    }
    I3D_shadow **sp = &DuplShadowList[0];
    for (i = DuplShadowList.size(); i--; sp++) {
        if ((*sp)->m_pParent)
            (*sp)->SetTo((*sp)->m_pParent);
    }
    Update();
    if (model->ipol_set)
        SetAnimation(model->ipol_set->anims[0].set, 0, 1.0f, 0);
    return r;
}

// FUNCTION: LS3DF 0x10033fd0
// FUNCTION: LS3DF11 0x10033050
LS3D_RESULT I3D_model::Duplicate(I3D_frame *frame)
{
    if (!frame)
        return I3DERR_INVALIDPARAMS;
    LS3D_RESULT r = I3D_frame::Duplicate(frame);
    if (r < 0)
        return r;
    if (frame->GetType() == FRAME_MODEL)
        return DuplicateHierarchy((I3D_model *)frame);
    return r;
}

// FUNCTION: LS3DF 0x10034010
// FUNCTION: LS3DF11 0x10033090
void I3D_model::Tick(int time)
{
    if (ipol_set && ipol_set->interpolators.size()) {
        ipol_set->Tick(time);
        Update();
    }
}

// FUNCTION: LS3DF 0x10034040
// FUNCTION: LS3DF11 0x100330c0
void I3D_model::Tick(const int *times)
{
    if (ipol_set) {
        ipol_set->Tick(times);
        Update();
    }
}

// FUNCTION: LS3DF 0x10034070
// FUNCTION: LS3DF11 0x100330f0
void I3D_model::ResetAllPoses()
{
    if (ipol_set)
        ipol_set->ResetAllPoses();
}

// FUNCTION: LS3DF 0x10034090
// FUNCTION: LS3DF11 0x10033110
int I3D_model::NumInterpolators() const
{
    if (!ipol_set)
        return 0;
    return ipol_set->interpolators.size();
}

// FUNCTION: LS3DF 0x100340b0
// FUNCTION: LS3DF11 0x10033130
I3D_interpolator **I3D_model::GetInterpolators() const
{
    if (!ipol_set)
        return NULL;
    return &ipol_set->interpolators[0];
}

// FUNCTION: LS3DF 0x100340d0
// FUNCTION: LS3DF11 0x10033150
LS3D_RESULT I3D_model::SetPose(I3D_pose_set *set)
{
    if (!ipol_set) {
        ipol_set = new I3D_IPolSet(this);
        if (!ipol_set) {
#if ENGINE_VERSION >= 395
            ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
#else
            dbgPrintf("!! Out of memory  I3D_model::SetPose()");
#endif
            return I3DERR_OUTOFMEM;
        }
    }
    return ipol_set->SetPose(set);
}

// FUNCTION: LS3DF 0x10034170
// FUNCTION: LS3DF11 0x100331f0
LS3D_RESULT I3D_model::SetPose(I3D_frame *frame, I3D_pose *pose)
{
    if (!ipol_set) {
        ipol_set = new I3D_IPolSet(this);
        if (!ipol_set) {
#if ENGINE_VERSION >= 395
            ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
#else
            dbgPrintf("!! Out of memory  I3D_model::SetPose()");
#endif
            return I3DERR_OUTOFMEM;
        }
    }
    return ipol_set->SetPose(frame, pose);
}

// FUNCTION: LS3DF 0x10034210
// FUNCTION: LS3DF11 0x10033290
LS3D_RESULT I3D_model::SetPose(const char *frame_name, I3D_pose *pose)
{
    if (!ipol_set) {
        ipol_set = new I3D_IPolSet(this);
        if (!ipol_set) {
#if ENGINE_VERSION >= 395
            ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
#else
            dbgPrintf("!! Out of memory  I3D_model::SetPose()");
#endif
            return I3DERR_OUTOFMEM;
        }
    }
    return ipol_set->SetPose(frame_name, pose);
}

// FUNCTION: LS3DF 0x100342b0
// FUNCTION: LS3DF11 0x10033330
LS3D_RESULT I3D_model::SetPoseBlend(float blend)
{
    if (!ipol_set)
        return I3DERR_OBJECTNOTFOUND;
    ipol_set->SetPoseBlend(blend);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x100342e0
// FUNCTION: LS3DF11 0x10033360
LS3D_RESULT I3D_model::SetPoseBlend(float blend, I3D_pose_set *set)
{
    if (!ipol_set)
        return I3DERR_OBJECTNOTFOUND;
    return ipol_set->SetPoseBlend(blend, set);
}

// FUNCTION: LS3DF 0x10034310
// FUNCTION: LS3DF11 0x10033390
float I3D_model::GetPoseBlend() const
{
    if (!ipol_set)
        return 0.0f;
    return ipol_set->GetPoseBlend();
}
