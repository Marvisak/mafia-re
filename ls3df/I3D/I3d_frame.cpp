// The scene graph's node (PS2 I3d_frame.cpp), at 0x1001a7f0-0x1001d2e3 in the original: I3D_frame in PS2's order,
// with the PC's hierarchy bound volumes. The header's zero vector has its $E at 0x1001c280, between EnumSubtree and
// EnumFrames (ours comes after RemoveChild: see tools/autopch.py). Names are the PS2 build's unless noted.
#include "I3D/I3D.h"
#include "IGraph/IGraph.h"
#include "dta.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

// I3d_driver.cpp
extern I3D_frame *pFFRoot;
extern I3D_frame *pFFLastUp_;
extern I3D_frame **OFFD_pFrameList;
extern int OFFD_ListUsed;

// The loader's file and error, which LoadImage reads from and sets (another I3D file's)
extern int LoadImageHandle;
extern LS3D_RESULT ImageErrFlg;

// The frame the last destructor ran for, so that an enumeration doesn't go on into a frame its callback deleted
// GLOBAL: LS3DF 0x100c0028
I3D_frame *lastDestroyedFrame;

// OFFD_CountFrames' count
// GLOBAL: LS3DF 0x100c002c
int OFFD_NumFrames;

// FUNCTION: LS3DF 0x1001a7f0
I3D_frame::I3D_frame()
{
    ref = 1;
    type = FRAME_NULL;
    parent = NULL;
    child = NULL;
    prev = NULL;
    next = NULL;
    num_children = 0;
    f_matrix.Identity();
    matrix.Identity();
    update_count = 0;
    scale = S_vector(1.0f, 1.0f, 1.0f);
    rot.Identity();
    world_scale = 1.0f;
    owner = NULL;
    callback = NULL;
    user_data = 0;
    name = NULL;
    orig_name = NULL;
    property = NULL;
    bound.bbox.Invalidate();
    bound.bsphere.pos = S_vector(0.0f, 0.0f, 0.0f);
    bound.bsphere.radius = 0.0f;
    bound_w.bbox.Invalidate();
    bound_w.bsphere.pos = S_vector(0.0f, 0.0f, 0.0f);
    bound_w.bsphere.radius = 0.0f;
    frm_flags = FRMFLAGS_UPDATE | FRMFLAGS_WMAT_VALID | FRMFLAGS_MAT_VALID | FRMFLAGS_QUAT_VALID | FRMFLAGS_ROT_VALID |
        FRMFLAGS_ON;
    unknown_130 = 0;
    ff_left = NULL;
    ff_right = NULL;
    unknown_13c = -1;
}

// FUNCTION: LS3DF 0x1001a9a0
I3D_frame::~I3D_frame()
{
    if (frm_flags & FRMFLAGS_FFTREE)
        RemoveFromFFTree();
    DeleteFromOwner();
    if (frm_flags & FRMFLAGS_TARGET)
        I3D_target::DeleteFromAnyTarget(this);
    if (frm_flags & FRMFLAGS_SHADOW)
        I3D_shadow::DeleteFromAnyShadow(this);
    if (frm_flags & FRMFLAGS_TICK)
        pI3DDriver->DeleteFromSceneTickList(this);
    if (parent)
        parent->RemoveChild(this);
    for (I3D_frame *frm = child; frm; ) {
        if (frm->parent != this) {
            dbgPrintf("!! Bad child in list  I3D_frame destructor  this:0x%x\t%s   child:0x%x", this, GetName(), frm);
            break;
        }
        I3D_frame *frm_next = frm->next;
        if (frm->type == FRAME_SHADOW)
            ((I3D_shadow *)frm)->SetTo(NULL);
        RemoveChild(frm);
        frm = frm_next;
    }
    delete[] property;
    delete[] name;
    lastDestroyedFrame = this;
}

// FUNCTION: LS3DF 0x1001aae0
uint I3D_frame::Release()
{
    if (--ref > 0)
        return ref;
    delete this;
    return 0;
}

// FUNCTION: LS3DF 0x1001ab00
void I3D_frame::DeleteFromOwner()
{
    if (!owner)
        return;
    switch (owner->type) {
    case FRAME_MODEL:
        ((I3D_model *)owner)->DeleteFrame(this);
        break;
    case FRAME_SCENE:
        ((I3D_scene *)owner)->DeleteFrame(this);
        break;
    default:
        dbgPrintf("!! Invalid owner type:%d  m_pOwner:0x%x  this:0x%x  %s  I3D_frame::DeleteFromOwner()", owner->type,
            owner, this, GetName());
    }
}

// The rotation of a direction and a roll (PC: pitch from the direction's height, yaw from its ground direction)
// FUNCTION: LS3DF 0x1001ab50
void I3D_frame::SetDir(const S_vector &dir, float roll)
{
    float pitch = 0.0f, yaw = 0.0f;
    if (fabs(dir.x) > 1e-5f) {
        if (fabs(dir.y) > 1e-5f) {
            float s = dir.y / dir.Magnitude();
            if (fabs(s) < 1.0)
                pitch = (float)asin(s);
            else
                pitch = s > 0.0f ? PI * 0.5f : -PI * 0.5f;
        }
        if (fabs(dir.z) < 1e-5f) {
            if (dir.x > 0.0f)
                yaw = -PI * 0.5f;
            else if (dir.x < 0.0f)
                yaw = PI * 0.5f;
        } else {
            yaw = (float)atan(-(dir.x / dir.z));
            if (dir.z < 0.0f)
                yaw += PI;
        }
    } else {
        if (fabs(dir.y) > 1e-5f) {
            float s = dir.y / dir.Magnitude();
            if (fabs(s) < 1.0)
                pitch = (float)asin(s);
            else
                pitch = s > 0.0f ? PI * 0.5f : -PI * 0.5f;
        }
        if (dir.z < 0.0f)
            yaw = PI;
    }
    S_quat q_pitch, q_yaw, q_roll;
    q_pitch.Make(S_vector(1.0f, 0.0f, 0.0f), pitch);
    q_yaw.Make(S_vector(0.0f, 1.0f, 0.0f), yaw);
    q_roll.Make(S_vector(0.0f, 0.0f, 1.0f), roll);
    SetRot(q_roll * q_pitch * q_yaw);
}

// FUNCTION: LS3DF 0x1001b040
void I3D_frame::SetWorldDir(const S_vector &dir, float roll)
{
    if (!parent) {
        SetDir(dir, roll);
        return;
    }
    S_matrix m_inv;
    m_inv.Inverse(parent->GetMatrix());
    S_matrix m;
    m.SetDir(dir);
    if (fabs(roll) > MRG_ZERO)
        m = RotateZMatrix(roll) * m;
    m = m * m_inv;
    f_matrix.SetDir3(m(2), m(1));
    frm_flags = (frm_flags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID | FRMFLAGS_QUAT_VALID |
        FRMFLAGS_ROT_VALID)) | FRMFLAGS_UPDATE | FRMFLAGS_MAT_VALID;
}

// FUNCTION: LS3DF 0x1001b160
void I3D_frame::SetWorldPos(const S_vector &pos)
{
    if (!parent)
        SetPos(pos);
    else {
        S_matrix m_inv;
        m_inv.Inverse(parent->GetMatrix());
        SetPos(pos * m_inv);
    }
}

// FUNCTION: LS3DF 0x1001b230
void I3D_frame::SetWorldPosDir(const S_vector &pos, const S_vector &dir, float roll)
{
    if (!parent) {
        SetDir(dir, roll);
        SetPos(pos);
        return;
    }
    S_matrix m_inv;
    m_inv.Inverse(parent->GetMatrix());
    S_matrix m;
    m.SetDir(dir);
    if (fabs(roll) > MRG_ZERO)
        m = RotateZMatrix(roll) * m;
    m = m * m_inv;
    f_matrix.SetDir3(m(2), m(1));
    frm_flags = (frm_flags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID | FRMFLAGS_QUAT_VALID |
        FRMFLAGS_ROT_VALID)) | FRMFLAGS_UPDATE | FRMFLAGS_MAT_VALID;
    // pos isn't used: row 3 is SetDir's, moved by the parent's inverse
    SetPos(m(3));
}

// FUNCTION: LS3DF 0x1001b3c0
void I3D_frame::SetCallback(I3D_CALLBACK *cb, uint flags)
{
    flags &= FRMFLAGS_CALLBACK_MASK;
    if (!cb)
        frm_flags &= ~flags;
    else {
        callback = cb;
        frm_flags |= flags;
    }
}

// FUNCTION: LS3DF 0x1001b400
void I3D_frame::SetOn(bool on)
{
    if (on)
        frm_flags |= FRMFLAGS_ON;
    else
        frm_flags &= ~FRMFLAGS_ON;
}

// FUNCTION: LS3DF 0x1001b430
void I3D_frame::RemoveFromFFTree()
{
    frm_flags &= ~FRMFLAGS_FFTREE;
    if (!name)
        return;
    if (pI3DDriver->FindFrameFast_(name, this) == this) {
        I3D_frame *up = pFFLastUp_;
        I3D_frame **link;
        if (this == pFFRoot)
            link = &pFFRoot;
        else if (up->ff_left == this)
            link = &up->ff_left;
        else
            link = &up->ff_right;
        if (ff_right) {
            *link = ff_right;
            if (ff_left) {
                I3D_frame *frm = ff_right;
                while (frm->ff_left)
                    frm = frm->ff_left;
                frm->ff_left = ff_left;
            }
        } else
            *link = ff_left;
    } else
        dbgPrintf(" !! RemoveFromFFTree error! Can not remove frame %s !! ", name);
    ff_left = NULL;
    ff_right = NULL;
}

// FUNCTION: LS3DF 0x1001b500
void I3D_frame::AddToFFTree()
{
    static int addcounter;
    addcounter++;
    if (!pFFRoot) {
        pFFRoot = this;
    } else {
        I3D_frame *frm = pFFRoot;
        for (;;) {
            int cmp = strcmp(name, frm->name);
            if (cmp < 0) {
                if (!frm->ff_left) {
                    frm->ff_left = this;
                    break;
                }
                frm = frm->ff_left;
            } else if (!cmp) {
                while (frm->ff_right && !strcmp(name, frm->ff_right->name))
                    frm = frm->ff_right;
                ff_right = frm->ff_right;
                frm->ff_right = this;
                break;
            } else {
                if (!frm->ff_right) {
                    frm->ff_right = this;
                    break;
                }
                frm = frm->ff_right;
            }
        }
    }
    frm_flags |= FRMFLAGS_FFTREE;
}

// The fast find tree's counts and list. VC6 inlines OFFD_CountFrames_ one level into OFFD_CountFrames, and turns the
// recursions into the right branch into loops.
// FUNCTION: LS3DF 0x1001b600
int I3D_frame::OFFD_CountFrames()
{
    OFFD_NumFrames = 1;
    if (ff_left)
        ff_left->OFFD_CountFrames_();
    if (ff_right)
        ff_right->OFFD_CountFrames_();
    return OFFD_NumFrames;
}

// FUNCTION: LS3DF 0x1001b680
void I3D_frame::OFFD_CountFrames_()
{
    if (ff_left)
        ff_left->OFFD_CountFrames_();
    OFFD_NumFrames++;
    if (ff_right)
        ff_right->OFFD_CountFrames_();
}

// FUNCTION: LS3DF 0x1001b6b0
void I3D_frame::AlphabeticalListOfFrames_()
{
    if (ff_left)
        ff_left->AlphabeticalListOfFrames_();
    OFFD_pFrameList[OFFD_ListUsed++] = this;
    if (ff_right)
        ff_right->AlphabeticalListOfFrames_();
}

// FUNCTION: LS3DF 0x1001b6f0
LS3D_RESULT I3D_frame::SetName(const char *new_name)
{
    if (name == new_name)
        return I3D_OK;
    if (name && new_name && !strcmp(name, new_name))
        return I3D_OK;
    if (frm_flags & FRMFLAGS_FFTREE)
        RemoveFromFFTree();
    delete[] name;
    if (!new_name) {
        name = NULL;
        orig_name = NULL;
        return I3D_OK;
    }
    name = new char[strlen(new_name) + 1];
    if (!name) {
        dbgPrintf("!! Out of memory  I3D_frame::SetName()");
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
        return I3DERR_OUTOFMEM;
    }
    strcpy(name, new_name);
    orig_name = name;
    if (renderState[RS_FAST_FIND])
        AddToFFTree();
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1001b800
LS3D_RESULT I3D_frame::SetModelName(const char *model_name)
{
    if (!orig_name)
        orig_name = "";
    char *full_name = new char[strlen(model_name) + strlen(orig_name) + 2];
    if (!full_name) {
        dbgPrintf("!! Out of memory  I3D_frame::SetModelName()");
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
        return I3DERR_OUTOFMEM;
    }
    sprintf(full_name, "%s.%s", model_name, orig_name);
    if (frm_flags & FRMFLAGS_FFTREE)
        RemoveFromFFTree();
    delete[] name;
    name = full_name;
    orig_name = &full_name[strlen(model_name) + 1];
    if (renderState[RS_FAST_FIND])
        AddToFFTree();
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1001b8f0
LS3D_RESULT I3D_frame::ResetModelName()
{
    if (orig_name == name)
        return I3D_OK;
    if (frm_flags & FRMFLAGS_FFTREE)
        RemoveFromFFTree();
    if (!orig_name) {
        delete[] name;
        name = NULL;
        return I3D_OK;
    }
    int len = strlen(orig_name) + 1;
    char *new_name = new char[len];
    if (!new_name) {
        dbgPrintf("!! Out of memory  I3D_frame::ResetModelName()");
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
        return I3DERR_OUTOFMEM;
    }
    memcpy(new_name, orig_name, len);
    delete[] name;
    name = new_name;
    orig_name = new_name;
    if (renderState[RS_FAST_FIND])
        AddToFFTree();
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1001b9d0
LS3D_RESULT I3D_frame::LinkTo(I3D_frame *frame, uint flags)
{
    // The sectors, occluders and areas register in their scene, which changes with the sector the frame is in
    struct EnumHelp {
        // FUNCTION: LS3DF 0x1001be40
        static I3DENUMRET __stdcall I3D_EnumProc(I3D_frame *frame, uint user)
        {
            ((I3D_sector *)frame)->AddToRemoveFromSceneRing((I3D_scene *)user);
            return I3DENUMRET_OK;
        }
    };
    struct EnumHelp1 {
        // FUNCTION: LS3DF 0x1001be60
        static I3DENUMRET __stdcall I3D_EnumProc(I3D_frame *frame, uint user)
        {
            ((I3D_occluder *)frame)->AddToRemoveFromSceneRing((I3D_scene *)user);
            return I3DENUMRET_OK;
        }
    };
    struct EnumHelp2 {
        // FUNCTION: LS3DF 0x1001be80
        static I3DENUMRET __stdcall I3D_EnumProc(I3D_frame *frame, uint user)
        {
            ((I3D_area *)frame)->AddToRemoveFromSceneRing((I3D_scene *)user);
            return I3DENUMRET_OK;
        }
    };

    if (frame == parent)
        return I3D_OK;
    I3D_frame *old_parent = parent;
    uint keep_world = flags & 1;
    if (keep_world)
        UpdateWMatrix();
    if (parent) {
        I3D_frame *frm = parent;
        frm->RemoveChild(this);
        if (flags & 2)
            frm->DiscardHRBoundVol();
    }
    if (frame)
        frame->AddChild(this);
    if (keep_world) {
        S_matrix m;
        if (frame) {
            m = frame->GetMatrix();
            m.Inverse(S_matrix(m));
            m = GetMatrix() * m;
        } else
            m = GetMatrix();
        SetPos(m(3));
        scale = m.GetScale();
        if (!(frm_flags & FRMFLAGS_QUAT_VALID))
            rot.Make(f_matrix);
        frm_flags = (frm_flags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID | FRMFLAGS_MAT_VALID |
            FRMFLAGS_ROT_VALID)) | FRMFLAGS_UPDATE | FRMFLAGS_QUAT_VALID;
        f_matrix.SetDir3(m(2), m(1));
        frm_flags = (frm_flags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID | FRMFLAGS_QUAT_VALID |
            FRMFLAGS_ROT_VALID)) | FRMFLAGS_UPDATE | FRMFLAGS_MAT_VALID;
    } else
        frm_flags = (frm_flags & ~FRMFLAGS_WMAT_VALID) | FRMFLAGS_UPDATE;
    if (flags & 4)
        Update();
    else if (flags & 2)
        DiscardHRBoundVol();

    uint hr_flags = frm_flags & (FRMFLAGS_HR_SECTORS | FRMFLAGS_HR_OCCLUDERS | FRMFLAGS_HR_AREAS);
    if (hr_flags) {
        I3D_frame *frm;
        for (frm = frame; frm; frm = frm->parent)
            frm->frm_flags |= hr_flags;
        for (frm = old_parent; frm; frm = frm->parent) {
            switch (frm->type) {
            case FRAME_SECTOR:
                frm->frm_flags &= ~(FRMFLAGS_HR_SECTORS | FRMFLAGS_HR_AREAS);
                break;
            case FRAME_OCCLUDER:
                frm->frm_flags &= ~(FRMFLAGS_HR_SECTORS | FRMFLAGS_HR_OCCLUDERS);
                break;
            case FRAME_AREA:
                frm->frm_flags &= ~(FRMFLAGS_HR_OCCLUDERS | FRMFLAGS_HR_AREAS);
                break;
            default:
                frm->frm_flags &= ~(FRMFLAGS_HR_SECTORS | FRMFLAGS_HR_OCCLUDERS | FRMFLAGS_HR_AREAS);
            }
            // The children's flags back: the mask is 0 (& for |), so only the walk and a store of the same flags stay,
            // in PS2's code too
            uint f = 0;
            for (I3D_frame *c = frm->child; c; c = c->next)
                f |= c->frm_flags & FRMFLAGS_HR_SECTORS & FRMFLAGS_HR_OCCLUDERS & FRMFLAGS_HR_AREAS;
            frm->frm_flags |= f;
        }
        I3D_scene *old_scene = NULL, *new_scene = NULL;
        for (frm = old_parent; frm; frm = frm->parent) {
            if (frm->type == FRAME_SECTOR) {
                old_scene = ((I3D_sector *)frm)->scene;
                break;
            }
        }
        for (frm = frame; frm; frm = frm->parent) {
            if (frm->type == FRAME_SECTOR) {
                new_scene = ((I3D_sector *)frm)->scene;
                break;
            }
        }
        if (old_scene != new_scene) {
            if (type == FRAME_SECTOR)
                ((I3D_sector *)this)->AddToRemoveFromSceneRing(new_scene);
            EnumFrames(EnumHelp::I3D_EnumProc, (uint)new_scene, ENUMF_SECTOR, NULL);
            if (type == FRAME_OCCLUDER)
                ((I3D_occluder *)this)->AddToRemoveFromSceneRing(new_scene);
            EnumFrames(EnumHelp1::I3D_EnumProc, (uint)new_scene, ENUMF_OCCLUDER, NULL);
            if (type == FRAME_AREA)
                ((I3D_area *)this)->AddToRemoveFromSceneRing(new_scene);
            EnumFrames(EnumHelp2::I3D_EnumProc, (uint)new_scene, ENUMF_AREA, NULL);
        }
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1001bea0
I3D_frame *I3D_frame::AddChild(I3D_frame *frame)
{
    frm_flags &= ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_HR_BOUND_VALID);
    if (!child) {
        child = frame;
        frame->next = NULL;
        frame->prev = NULL;
        frame->parent = this;
        num_children = 1;
    } else {
        frame->next = child;
        frame->prev = NULL;
        child->prev = frame;
        child = frame;
        frame->parent = this;
        num_children++;
    }
    return frame;
}

// FUNCTION: LS3DF 0x1001bf20
I3D_frame *I3D_frame::RemoveChild(I3D_frame *frame)
{
    if (frame->parent != this)
        return frame;
    num_children--;
    if (frame->prev) {
        frame->prev->next = frame->next;
        if (frame->next) {
            frame->next->prev = frame->prev;
            frame->next = NULL;
        }
        frame->prev = NULL;
    } else if (frame->next) {
        child = frame->next;
        frame->next->prev = NULL;
        frame->next = NULL;
    } else {
        child = NULL;
        num_children = 0;
    }
    frame->prev = NULL;
    frame->next = NULL;
    frame->parent = NULL;
    frm_flags &= ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_HR_BOUND_VALID);
    return frame;
}

// FUNCTION: LS3DF 0x1001bfd0
I3D_frame *I3D_frame::GetChild(int index) const
{
    if (index < num_children) {
        for (I3D_frame *frm = child; frm; frm = frm->next) {
            if (!index--)
                return frm;
        }
        dbgPrintf("!! Child frame not found  I3D_frame::GetChild()");
    }
    return NULL;
}

static inline int I3Dtoupper(int c)
{
    return c >= 'a' && c <= 'z' ? c - ('a' - 'A') : c;
}

// FUNCTION: LS3DF 0x1001c020
LS3D_RESULT I3D_frame::EnumSubtree(I3D_ENUMPROC *proc, uint user, uint flags, const char *mask) const
{
    uint type_flags = flags & ENUMF_ALL;
    if (type_flags != ENUMF_ALL) {
        switch (type) {
        case FRAME_VISUAL: type_flags &= ENUMF_VISUAL; break;
        case FRAME_LIGHT: type_flags &= ENUMF_LIGHT; break;
        case FRAME_CAMERA: type_flags &= ENUMF_CAMERA; break;
        case FRAME_SOUND: type_flags &= ENUMF_SOUND; break;
        case FRAME_SECTOR: type_flags &= ENUMF_SECTOR; break;
        case FRAME_DUMMY: type_flags &= ENUMF_DUMMY; break;
        case FRAME_MODEL: type_flags &= ENUMF_MODEL; break;
        case FRAME_USER: type_flags &= ENUMF_USER; break;
        case FRAME_VOLUME: type_flags &= ENUMF_VOLUME; break;
        case FRAME_JOINT: type_flags &= ENUMF_JOINT; break;
        case FRAME_SHADOW: type_flags &= ENUMF_SHADOW; break;
        case FRAME_TARGET: type_flags &= ENUMF_TARGET; break;
        case FRAME_AREA: type_flags &= ENUMF_AREA; break;
        default: type_flags = 0;
        }
        if (!type_flags)
            goto children;
    }
    if (mask) {
        if (!orig_name)
            goto children;
        if (!(flags & ENUMF_WILDMASK)) {
            if (flags & ENUMF_CASESENS) {
                if (strcmp(mask, orig_name))
                    goto children;
            } else if (stricmp(mask, orig_name))
                goto children;
        } else if (flags & ENUMF_CASESENS) {
            int len = min(strlen(mask), strlen(orig_name)) + 1;
            for (const char *cp = mask, *cp1 = orig_name; len--; cp++, cp1++) {
                if (*cp == '*')
                    break;
                if (*cp != '?' && *cp != *cp1)
                    goto children;
            }
        } else {
            int len = min(strlen(mask), strlen(orig_name)) + 1;
            for (const char *cp = mask, *cp1 = orig_name; len--; cp++, cp1++) {
                if (*cp == '*')
                    break;
                if (*cp != '?' && I3Dtoupper(*cp) != I3Dtoupper(*cp1))
                    goto children;
            }
        }
    }
    {
        I3DENUMRET ret = proc((I3D_frame *)this, user);
        if (ret == I3DENUMRET_CANCEL)
            return I3DERR_CANCELED;
        if (ret == I3DENUMRET_SKIPCHILDREN)
            return I3D_OK;
    }
children:
    if (lastDestroyedFrame != this) {
        for (I3D_frame *frm = child; frm; ) {
            I3D_frame *frm_next = frm->next;
            if (frm->EnumSubtree(proc, user, flags, mask) == I3DERR_CANCELED)
                return I3DERR_CANCELED;
            frm = frm_next;
        }
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1001c2a0
LS3D_RESULT I3D_frame::EnumFrames(I3D_ENUMPROC *proc, uint user, uint flags, const char *mask) const
{
    if (child) {
        lastDestroyedFrame = NULL;
        for (I3D_frame *frm = child; frm; ) {
            I3D_frame *frm_next = frm->next;
            if (frm->EnumSubtree(proc, user, flags, mask) == I3DERR_CANCELED)
                return I3DERR_CANCELED;
            frm = frm_next;
        }
    }
    return I3D_OK;
}

// PS2 binds it globally: not static, so VC6 emits it here, not after FindChildFrame
// FUNCTION: LS3DF 0x1001c310
I3DENUMRET __stdcall FindChildCallBack(I3D_frame *frame, uint user)
{
    *(I3D_frame **)user = frame;
    return I3DENUMRET_CANCEL;
}

// FUNCTION: LS3DF 0x1001c330
I3D_frame *I3D_frame::FindChildFrame(const char *frame_name, uint flags) const
{
    I3D_frame *frm = NULL;
    EnumFrames(FindChildCallBack, (uint)&frm, flags, frame_name);
    return frm;
}

// FUNCTION: LS3DF 0x1001c360
I3D_frame *I3D_frame::GetChildFrame(const char *frame_name, uint flags)
{
    uint type_flags = flags & ENUMF_ALL;
    if (type_flags != ENUMF_ALL) {
        switch (type) {
        case FRAME_VISUAL: type_flags &= ENUMF_VISUAL; break;
        case FRAME_LIGHT: type_flags &= ENUMF_LIGHT; break;
        case FRAME_CAMERA: type_flags &= ENUMF_CAMERA; break;
        case FRAME_SOUND: type_flags &= ENUMF_SOUND; break;
        case FRAME_SECTOR: type_flags &= ENUMF_SECTOR; break;
        case FRAME_DUMMY: type_flags &= ENUMF_DUMMY; break;
        case FRAME_MODEL: type_flags &= ENUMF_MODEL; break;
        case FRAME_USER: type_flags &= ENUMF_USER; break;
        case FRAME_VOLUME: type_flags &= ENUMF_VOLUME; break;
        case FRAME_SHADOW: type_flags &= ENUMF_SHADOW; break;
        case FRAME_TARGET: type_flags &= ENUMF_TARGET; break;
        case FRAME_AREA: type_flags &= ENUMF_AREA; break;
        default: type_flags = 0;
        }
        if (!type_flags)
            goto children;
    }
    if (frame_name) {
        if (!name)
            goto children;
        if (!(flags & ENUMF_WILDMASK)) {
            if (flags & ENUMF_CASESENS) {
                if (strcmp(frame_name, name))
                    goto children;
            } else if (stricmp(frame_name, name))
                goto children;
        } else if (flags & ENUMF_CASESENS) {
            int len = min(strlen(frame_name), strlen(name)) + 1;
            for (const char *cp = frame_name, *cp1 = name; len--; cp++, cp1++) {
                if (*cp == '*')
                    break;
                if (*cp != '?' && *cp != *cp1)
                    goto children;
            }
        } else {
            int len = min(strlen(frame_name), strlen(name)) + 1;
            for (const char *cp = frame_name, *cp1 = name; len--; cp++, cp1++) {
                if (*cp == '*')
                    break;
                if (*cp != '?' && toupper(*cp) != toupper(*cp1))
                    goto children;
            }
        }
    }
    return this;
children:
    for (I3D_frame *frm = child; frm; frm = frm->next) {
        I3D_frame *found = frm->GetChildFrame(frame_name, flags);
        if (found)
            return found;
    }
    return NULL;
}

// FUNCTION: LS3DF 0x1001c5b0
LS3D_RESULT I3D_frame::Duplicate(I3D_frame *frame)
{
    if (!frame)
        return I3DERR_INVALIDPARAMS;
    if (frm_flags & FRMFLAGS_FFTREE)
        RemoveFromFFTree();
    delete[] name;
    uint flags = frm_flags;
    memcpy(&f_matrix, &frame->f_matrix, sizeof(S_matrix) + sizeof(S_vector) + sizeof(S_quat));
    frm_flags = (frame->frm_flags & 0xfa73e9f) | (flags & (FRMFLAGS_TICK | FRMFLAGS_SHADOW | FRMFLAGS_TARGET)) |
        FRMFLAGS_UPDATE;
    callback = frame->callback;
    update_count++;
    SetProperty(frame->property);
    if (!frame->name) {
        name = NULL;
        orig_name = NULL;
    } else {
        name = new char[strlen(frame->name) + 1];
        if (!name) {
            orig_name = NULL;
            dbgPrintf("!! Out of memory  I3D_frame::Duplicate()");
            ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
            return I3DERR_OUTOFMEM;
        }
        strcpy(name, frame->name);
        if (renderState[RS_FAST_FIND])
            AddToFFTree();
        orig_name = frame->orig_name - frame->name + name;
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1001c700
LS3D_RESULT I3D_frame::SetProperty(const char *prop)
{
    delete[] property;
    if (!prop) {
        property = NULL;
        return I3D_OK;
    }
    int len = strlen(prop);
    if (!len) {
        property = NULL;
        return I3D_OK;
    }
    property = new char[len + 1];
    if (!property)
        return I3DERR_OUTOFMEM;
    strcpy(property, prop);
    return I3D_OK;
}

// The targets to update after the frames (I3D_target's), defined here: its $E is between SetProperty and
// UpdateWMatrixProc
// GLOBAL: LS3DF 0x100c0034
C_list<I3D_target *> UpdateTargetList;

// The original calls it out of line from this file too, where VC6 inlines ours; why isn't known (writing
// UpdateMatrix's code out keeps it out of line too, but then VC6 merges more of the two branches' scaling)
#pragma auto_inline(off)

// FUNCTION: LS3DF 0x1001c7d0
void I3D_frame::UpdateWMatrixProc()
{
    UpdateMatrix();
    if (parent) {
        parent->UpdateWMatrix();
        matrix = f_matrix * parent->matrix;
        world_scale = (scale.x + scale.y + scale.z) * parent->world_scale * (1.0f / 3.0f);
    } else {
        matrix = f_matrix;
        world_scale = (scale.x + scale.y + scale.z) * (1.0f / 3.0f);
    }
    frm_flags |= FRMFLAGS_WMAT_VALID;
}

#pragma auto_inline(on)

// Inline: Update inlines it, the recursion needs the copy (/Ob2 alone takes only small functions). Our copy comes
// after Update, the original's before it: open
// FUNCTION: LS3DF 0x1001c9a0
inline void I3D_frame::UpdateChildren()
{
    uint changed = frm_flags & (FRMFLAGS_UPDATE | FRMFLAGS_CHANGED);
    if (changed) {
        update_count++;
        if (type == FRAME_VISUAL && !(((I3D_visual *)this)->vis_flags & 0x40))
            ((I3D_visual *)this)->vis_flags &= ~0x40000;
    }
    for (I3D_frame *frm = child; frm; frm = frm->next) {
        if (changed)
            frm->frm_flags = (frm->frm_flags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID)) | FRMFLAGS_CHANGED;
        frm->UpdateChildren();
        frm->UpdateV();
        if (frm->frm_flags & (FRMFLAGS_UPDATE | FRMFLAGS_CHANGED)) {
            frm->frm_flags &= ~(FRMFLAGS_UPDATE | FRMFLAGS_CHANGED);
            frm_flags = (frm_flags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_HR_BOUND_VALID)) | FRMFLAGS_CHANGED;
        }
    }
}

// FUNCTION: LS3DF 0x1001ca60
void I3D_frame::Update()
{
    UpdateChildren();
    UpdateV();
    DiscardHRBoundVol();
    if (frm_flags & (FRMFLAGS_UPDATE | FRMFLAGS_CHANGED) && type == FRAME_VISUAL &&
        !(((I3D_visual *)this)->vis_flags & 0x40))
        ((I3D_visual *)this)->vis_flags &= ~0x40000;
    frm_flags &= ~(FRMFLAGS_UPDATE | FRMFLAGS_CHANGED);
    if (UpdateTargetList.size()) {
        static bool updating;
        if (!updating) {
            updating = true;
            I3D_target **target = &UpdateTargetList[0];
            for (int i = UpdateTargetList.size(); i--; target++)
                (*target)->UpdateFrames();
            UpdateTargetList.reset();
            updating = false;
        }
    }
}

// FUNCTION: LS3DF 0x1001cc00
void I3D_frame::DiscardHRBoundVol()
{
    if (type == FRAME_SECTOR)
        return;
    frm_flags &= ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_HR_BOUND_VALID);
    for (I3D_frame *frm = parent; frm; frm = frm->parent) {
        if (!(frm->frm_flags & FRMFLAGS_HR_BOUND_VALID))
            break;
        if (frm->type == FRAME_SECTOR)
            break;
        frm->frm_flags &= ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_HR_BOUND_VALID);
    }
}

// FUNCTION: LS3DF 0x1001cc50
void I3D_frame::UpdateHRBoundVol()
{
    if (type == FRAME_SECTOR) {
        frm_flags |= FRMFLAGS_HR_BOUND_VALID;
        return;
    }
    if (type == FRAME_VISUAL) {
        I3D_visual *vis = (I3D_visual *)this;
        if (!(vis->vis_flags & 0x20000))
            vis->UpdateBoundProc();
        bound.bbox = vis->bbox;
    } else
        bound.bbox.Invalidate();
    for (I3D_frame *frm = child; frm; frm = frm->next) {
        if (frm->type == FRAME_VOLUME)
            continue;
        if (!(frm->frm_flags & FRMFLAGS_ON))
            continue;
        if (!(frm->frm_flags & FRMFLAGS_HR_BOUND_VALID))
            frm->UpdateHRBoundVol();
        if (frm->bound.bbox.IsValid()) {
            const S_vector *v = BBoxVertices(frm->bound.bbox);
            // UpdateMatrix stays out of line: a call inside an inline function gets only a share of the budget
            const S_matrix &m = frm->GetLocalMatrix();
            for (int i = 8; i--; v++)
                bound.bbox.Absorb(*v * m);
        }
    }
    if (bound.bbox.IsValid()) {
        S_vector half = (bound.bbox.max - bound.bbox.min) * 0.5f;
        bound.bsphere.pos = bound.bbox.min + half;
        bound.bsphere.radius = (float)sqrt(half.Square());
    } else
        bound.bsphere.radius = 0.0f;
    frm_flags |= FRMFLAGS_HR_BOUND_VALID;
}

// The local bound volume of a box (PC only, the name is a guess)
// FUNCTION: LS3DF 0x1001ce80
void I3D_frame::SetBoundVolume(const I3D_bbox &bbox)
{
    if (bbox.min.x <= bbox.max.x) {
        bound.bbox = bbox;
        S_vector half = (bbox.max - bbox.min) * 0.5f;
        bound.bsphere.pos = bbox.min + half;
        bound.bsphere.radius = (float)sqrt(half.Square());
    } else {
        bound.bbox.Invalidate();
        bound.bsphere.radius = 0.0f;
    }
    frm_flags = (frm_flags & ~FRMFLAGS_HR_BOUND_W_VALID) | FRMFLAGS_HR_BOUND_VALID;
}

// FUNCTION: LS3DF 0x1001cf90
void I3D_frame::UpdateHRBoundVolW()
{
    if (!(frm_flags & FRMFLAGS_HR_BOUND_VALID))
        UpdateHRBoundVol();
    bound_w.bbox.Invalidate();
    if (bound.bbox.min.x <= bound.bbox.max.x) {
        const S_vector *v = BBoxVertices(bound.bbox);
        bound_w.bbox.min = bound_w.bbox.max = *v * GetMatrix();
        for (int i = 7; i--; ) {
            v++;
            bound_w.bbox.Absorb(*v * GetMatrix());
        }
        S_vector half = (bound_w.bbox.max - bound_w.bbox.min) * 0.5f;
        bound_w.bsphere.pos = bound_w.bbox.min + half;
        bound_w.bsphere.radius = (float)sqrt(half.Square());
    } else {
        bound_w.bsphere.pos = S_vector(0.0f, 0.0f, 0.0f);
        bound_w.bsphere.radius = 0.0f;
    }
    frm_flags |= FRMFLAGS_HR_BOUND_W_VALID;
}

// FUNCTION: LS3DF 0x1001d1c0
void I3D_frame::LoadImage()
{
    dtaRead(LoadImageHandle, &f_matrix(3), sizeof(S_vector));
    dtaRead(LoadImageHandle, &scale, 0x1d);
    int len = 0;
    dtaRead(LoadImageHandle, &len, 1);
    if (len) {
        name = new char[len + 1];
        if (!name)
            goto fail;
        orig_name = name;
        dtaRead(LoadImageHandle, name, len);
        name[len] = 0;
        if (renderState[RS_FAST_FIND])
            AddToFFTree();
    }
    dtaRead(LoadImageHandle, &len, 1);
    if (len) {
        property = new char[len + 1];
        if (!property)
            goto fail;
        dtaRead(LoadImageHandle, property, len);
        property[len] = 0;
    }
    frm_flags = (frm_flags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_HR_BOUND_VALID | FRMFLAGS_WMAT_VALID |
        FRMFLAGS_MAT_VALID | FRMFLAGS_ROT_VALID)) | FRMFLAGS_UPDATE | FRMFLAGS_QUAT_VALID;
    return;
fail:
    dbgPrintf("!! Out of memory  I3D_frame::LoadImage()");
    ImageErrFlg = I3DERR_OUTOFMEM;
    ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
}

// The loader's (another I3D file's), until that file is written
// GLOBAL: LS3DF 0x101c3bfc
int LoadImageHandle;

// GLOBAL: LS3DF 0x101c3be4
LS3D_RESULT ImageErrFlg;

// The functions of other files the frames call, until those are written. Their bodies differ, or the linker would
// fold them; auto_inline(off) keeps VC6 from inlining them here.
#pragma auto_inline(off)

void I3D_area::AddToRemoveFromSceneRing(I3D_scene *scene)
{
    unknown_140[0] = 1;
}

// STUB: LS3DF 0x1003a650
void I3D_occluder::AddToRemoveFromSceneRing(I3D_scene *scene)
{
    unknown_140[0] = 2;
}

// STUB: LS3DF 0x1004f260
void I3D_sector::AddToRemoveFromSceneRing(I3D_scene *scene)
{
    unknown_140[0] = 3;
}

// STUB: LS3DF 0x10050d80
void I3D_shadow::SetTo(I3D_frame *frame)
{
    unknown_140[0] = 4;
}

// STUB: LS3DF 0x100599b0
void I3D_target::UpdateFrames()
{
    unknown_140[0] = 5;
}

#pragma auto_inline(on)
