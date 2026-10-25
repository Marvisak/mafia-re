// The scene graph's node (PS2 I3d_frame.cpp), at 0x1001a7f0-0x1001d2e3 in the original: I3D_frame in PS2's order,
// with the PC's hierarchy bound volumes. The header's zero vector has its $E at 0x1001c280, between EnumSubtree and
// EnumFrames (see tools/autopch.py). Names are the PS2 build's unless noted.
#include "I3D/I3D.h"
#include "IGraph/IGraph.h"
#include "rw_data.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

// I3d_driver.cpp
extern I3D_frame *pFFRoot;
extern I3D_frame *pFFLastUp_;
extern I3D_frame **OFFD_pFrameList;
extern int OFFD_ListUsed;

// The frame the last destructor ran for, so that an enumeration doesn't go on into a frame its callback deleted
// GLOBAL: LS3DF 0x100c0028
// GLOBAL: LS3DF11 0x100bd148
I3D_frame *lastDestroyedFrame;

// OFFD_CountFrames' count
// GLOBAL: LS3DF 0x100c002c
// GLOBAL: LS3DF11 0x100bd14c
int OFFD_NumFrames;

// FUNCTION: LS3DF 0x1001a7f0
// FUNCTION: LS3DF11 0x1001a310
I3D_frame::I3D_frame()
{
    m_RefCount = 1;
    m_Type = FRAME_NULL;
    m_pParent = NULL;
    m_pChild = NULL;
    m_pPrev = NULL;
    m_pNext = NULL;
    m_numChildren = 0;
    m_LocalMatrix.Identity();
    m_Matrix.Identity();
    m_UpdateCount = 0;
    m_Scale = S_vector(1.0f, 1.0f, 1.0f);
    m_Rot.Identity();
    m_WorldScale = 1.0f;
    m_pOwner = NULL;
    m_pCallback = NULL;
    m_UserData = 0;
    m_pName = NULL;
    m_pOrigName = NULL;
    m_pProperty = NULL;
    m_HRBoundVol.bbox.Invalidate();
    m_HRBoundVol.bsphere.pos = S_vector(0.0f, 0.0f, 0.0f);
    m_HRBoundVol.bsphere.radius = 0.0f;
    m_HRBoundVolW.bbox.Invalidate();
    m_HRBoundVolW.bsphere.pos = S_vector(0.0f, 0.0f, 0.0f);
    m_HRBoundVolW.bsphere.radius = 0.0f;
    m_FrmFlags = FRMFLAGS_UPDATE | FRMFLAGS_WMAT_VALID | FRMFLAGS_MAT_VALID | FRMFLAGS_QUAT_VALID | FRMFLAGS_ROT_VALID |
        FRMFLAGS_ON;
    m_pNextVisual = NULL;
    m_pFFLeft = NULL;
    m_pFFRight = NULL;
    m_RenderNumber = -1;
}

// FUNCTION: LS3DF 0x1001a9a0
// FUNCTION: LS3DF11 0x1001a4c0
I3D_frame::~I3D_frame()
{
    if (m_FrmFlags & FRMFLAGS_FFTREE)
        RemoveFromFFTree();
    DeleteFromOwner();
    if (m_FrmFlags & FRMFLAGS_TARGET)
        I3D_target::DeleteFromAnyTarget(this);
    if (m_FrmFlags & FRMFLAGS_SHADOW)
        I3D_shadow::DeleteFromAnyShadow(this);
    if (m_FrmFlags & FRMFLAGS_TICK)
        pI3DDriver->DeleteFromSceneTickList(this);
    if (m_pParent)
        m_pParent->RemoveChild(this);
    for (I3D_frame *frm = m_pChild; frm; ) {
        if (frm->m_pParent != this) {
            dbgPrintf("!! Bad child in list  I3D_frame destructor  this:0x%x\t%s   child:0x%x", this, GetName(), frm);
            break;
        }
        I3D_frame *frm_next = frm->m_pNext;
        if (frm->m_Type == FRAME_SHADOW)
            ((I3D_shadow *)frm)->SetTo(NULL);
        RemoveChild(frm);
        frm = frm_next;
    }
    delete[] m_pProperty;
    delete[] m_pName;
    lastDestroyedFrame = this;
}

// FUNCTION: LS3DF 0x1001aae0
// FUNCTION: LS3DF11 0x1001a600
uint I3D_frame::Release()
{
    if (--m_RefCount > 0)
        return m_RefCount;
    delete this;
    return 0;
}

// FUNCTION: LS3DF 0x1001ab00
// FUNCTION: LS3DF11 0x1001a620
void I3D_frame::DeleteFromOwner()
{
    if (!m_pOwner)
        return;
    switch (m_pOwner->m_Type) {
    case FRAME_MODEL:
        ((I3D_model *)m_pOwner)->DeleteFrame(this);
        break;
    case FRAME_SCENE:
        ((I3D_scene *)m_pOwner)->DeleteFrame(this);
        break;
    default:
        dbgPrintf("!! Invalid owner type:%d  m_pOwner:0x%x  this:0x%x  %s  I3D_frame::DeleteFromOwner()",
            m_pOwner->m_Type, m_pOwner, this, GetName());
    }
}

// The rotation of a direction and a roll (PC: pitch from the direction's height, yaw from its ground direction)
// FUNCTION: LS3DF 0x1001ab50
// FUNCTION: LS3DF11 0x1001a670
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
// FUNCTION: LS3DF11 0x1001ab60
void I3D_frame::SetWorldDir(const S_vector &dir, float roll)
{
    if (!m_pParent) {
        SetDir(dir, roll);
        return;
    }
    S_matrix m_inv;
    m_inv.Inverse(m_pParent->GetMatrix());
    S_matrix m;
    m.SetDir(dir);
    if (fabs(roll) > MRG_ZERO)
        m = RotateZMatrix(roll) * m;
    m = m * m_inv;
    m_LocalMatrix.SetDir3(m(2), m(1));
    m_FrmFlags = (m_FrmFlags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID | FRMFLAGS_QUAT_VALID |
        FRMFLAGS_ROT_VALID)) | FRMFLAGS_UPDATE | FRMFLAGS_MAT_VALID;
}

// FUNCTION: LS3DF 0x1001b160
// FUNCTION: LS3DF11 0x1001ac80
void I3D_frame::SetWorldPos(const S_vector &pos)
{
    if (!m_pParent)
        SetPos(pos);
    else {
        S_matrix m_inv;
        m_inv.Inverse(m_pParent->GetMatrix());
        SetPos(pos * m_inv);
    }
}

// FUNCTION: LS3DF 0x1001b230
// FUNCTION: LS3DF11 0x1001ad50
void I3D_frame::SetWorldPosDir(const S_vector &pos, const S_vector &dir, float roll)
{
    if (!m_pParent) {
        SetDir(dir, roll);
        SetPos(pos);
        return;
    }
    S_matrix m_inv;
    m_inv.Inverse(m_pParent->GetMatrix());
    S_matrix m;
    m.SetDir(dir);
    if (fabs(roll) > MRG_ZERO)
        m = RotateZMatrix(roll) * m;
    m = m * m_inv;
    m_LocalMatrix.SetDir3(m(2), m(1));
    m_FrmFlags = (m_FrmFlags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID | FRMFLAGS_QUAT_VALID |
        FRMFLAGS_ROT_VALID)) | FRMFLAGS_UPDATE | FRMFLAGS_MAT_VALID;
    // pos isn't used (the original's bug): row 3 is SetDir's, moved by the parent's inverse
    SetPos(m(3));
}

// FUNCTION: LS3DF 0x1001b3c0
// FUNCTION: LS3DF11 0x1001aee0
void I3D_frame::SetCallback(I3D_CALLBACK *cb, uint flags)
{
    flags &= FRMFLAGS_CALLBACK_MASK;
    if (!cb)
        m_FrmFlags &= ~flags;
    else {
        m_pCallback = cb;
        m_FrmFlags |= flags;
    }
}

// FUNCTION: LS3DF 0x1001b400
// FUNCTION: LS3DF11 0x1001af20
void I3D_frame::SetOn(bool on)
{
    if (on)
        m_FrmFlags |= FRMFLAGS_ON;
    else
        m_FrmFlags &= ~FRMFLAGS_ON;
}

// FUNCTION: LS3DF 0x1001b430
// FUNCTION: LS3DF11 0x1001af50
void I3D_frame::RemoveFromFFTree()
{
    m_FrmFlags &= ~FRMFLAGS_FFTREE;
    if (!m_pName)
        return;
    if (pI3DDriver->FindFrameFast_(m_pName, this) == this) {
        I3D_frame *up = pFFLastUp_;
        I3D_frame **link;
        if (this == pFFRoot)
            link = &pFFRoot;
        else if (up->m_pFFLeft == this)
            link = &up->m_pFFLeft;
        else
            link = &up->m_pFFRight;
        if (m_pFFRight) {
            *link = m_pFFRight;
            if (m_pFFLeft) {
                I3D_frame *frm = m_pFFRight;
                while (frm->m_pFFLeft)
                    frm = frm->m_pFFLeft;
                frm->m_pFFLeft = m_pFFLeft;
            }
        } else
            *link = m_pFFLeft;
    } else
        dbgPrintf(" !! RemoveFromFFTree error! Can not remove frame %s !! ", m_pName);
    m_pFFLeft = NULL;
    m_pFFRight = NULL;
}

// FUNCTION: LS3DF 0x1001b500
// FUNCTION: LS3DF11 0x1001b020
void I3D_frame::AddToFFTree()
{
    static int addcounter;
    addcounter++;
    if (!pFFRoot) {
        pFFRoot = this;
    } else {
        I3D_frame *frm = pFFRoot;
        for (;;) {
            int cmp = strcmp(m_pName, frm->m_pName);
            if (cmp < 0) {
                if (!frm->m_pFFLeft) {
                    frm->m_pFFLeft = this;
                    break;
                }
                frm = frm->m_pFFLeft;
            } else if (!cmp) {
                while (frm->m_pFFRight && !strcmp(m_pName, frm->m_pFFRight->m_pName))
                    frm = frm->m_pFFRight;
                m_pFFRight = frm->m_pFFRight;
                frm->m_pFFRight = this;
                break;
            } else {
                if (!frm->m_pFFRight) {
                    frm->m_pFFRight = this;
                    break;
                }
                frm = frm->m_pFFRight;
            }
        }
    }
    m_FrmFlags |= FRMFLAGS_FFTREE;
}

// The fast find tree's counts and list. VC6 inlines OFFD_CountFrames_ one level into OFFD_CountFrames, and turns the
// recursions into the right branch into loops.
// FUNCTION: LS3DF 0x1001b600
// FUNCTION: LS3DF11 0x1001b120
int I3D_frame::OFFD_CountFrames()
{
    OFFD_NumFrames = 1;
    if (m_pFFLeft)
        m_pFFLeft->OFFD_CountFrames_();
    if (m_pFFRight)
        m_pFFRight->OFFD_CountFrames_();
    return OFFD_NumFrames;
}

// FUNCTION: LS3DF 0x1001b680
// FUNCTION: LS3DF11 0x1001b1a0
void I3D_frame::OFFD_CountFrames_()
{
    if (m_pFFLeft)
        m_pFFLeft->OFFD_CountFrames_();
    OFFD_NumFrames++;
    if (m_pFFRight)
        m_pFFRight->OFFD_CountFrames_();
}

// FUNCTION: LS3DF 0x1001b6b0
// FUNCTION: LS3DF11 0x1001b1d0
void I3D_frame::AlphabeticalListOfFrames_()
{
    if (m_pFFLeft)
        m_pFFLeft->AlphabeticalListOfFrames_();
    OFFD_pFrameList[OFFD_ListUsed++] = this;
    if (m_pFFRight)
        m_pFFRight->AlphabeticalListOfFrames_();
}

// FUNCTION: LS3DF 0x1001b6f0
// FUNCTION: LS3DF11 0x1001b210
LS3D_RESULT I3D_frame::SetName(const char *new_name)
{
    if (m_pName == new_name)
        return I3D_OK;
    if (m_pName && new_name && !strcmp(m_pName, new_name))
        return I3D_OK;
    if (m_FrmFlags & FRMFLAGS_FFTREE)
        RemoveFromFFTree();
    delete[] m_pName;
    if (!new_name) {
        m_pName = NULL;
        m_pOrigName = NULL;
        return I3D_OK;
    }
    m_pName = new char[strlen(new_name) + 1];
    if (!m_pName) {
        dbgPrintf("!! Out of memory  I3D_frame::SetName()");
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
        return I3DERR_OUTOFMEM;
    }
    strcpy(m_pName, new_name);
    m_pOrigName = m_pName;
    if (renderState[RS_FAST_FIND])
        AddToFFTree();
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1001b800
// FUNCTION: LS3DF11 0x1001b320
LS3D_RESULT I3D_frame::SetModelName(const char *model_name)
{
    if (!m_pOrigName)
        m_pOrigName = "";
    int len = strlen(model_name) + strlen(m_pOrigName) + 2;
    char *full_name = new char[len];
    if (!full_name) {
        dbgPrintf("!! Out of memory  I3D_frame::SetModelName()");
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
        return I3DERR_OUTOFMEM;
    }
    sprintf(full_name, "%s.%s", model_name, m_pOrigName);
    if (m_FrmFlags & FRMFLAGS_FFTREE)
        RemoveFromFFTree();
    delete[] m_pName;
    m_pName = full_name;
    m_pOrigName = &full_name[strlen(model_name) + 1];
    if (renderState[RS_FAST_FIND])
        AddToFFTree();
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1001b8f0
// FUNCTION: LS3DF11 0x1001b410
LS3D_RESULT I3D_frame::ResetModelName()
{
    if (m_pOrigName == m_pName)
        return I3D_OK;
    if (m_FrmFlags & FRMFLAGS_FFTREE)
        RemoveFromFFTree();
    if (!m_pOrigName) {
        delete[] m_pName;
        m_pName = NULL;
        return I3D_OK;
    }
    int len = strlen(m_pOrigName) + 1;
    char *new_name = new char[len];
    if (!new_name) {
        dbgPrintf("!! Out of memory  I3D_frame::ResetModelName()");
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
        return I3DERR_OUTOFMEM;
    }
    memcpy(new_name, m_pOrigName, len);
    delete[] m_pName;
    m_pName = new_name;
    m_pOrigName = new_name;
    if (renderState[RS_FAST_FIND])
        AddToFFTree();
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1001b9d0
// FUNCTION: LS3DF11 0x1001b4f0
LS3D_RESULT I3D_frame::LinkTo(I3D_frame *frame, uint flags)
{
    // The sectors, occluders and areas register in their scene, which changes with the sector the frame is in
    struct EnumHelp {
        // FUNCTION: LS3DF 0x1001be40
        // FUNCTION: LS3DF11 0x1001b960
        static I3DENUMRET __stdcall I3D_EnumProc(I3D_frame *frame, uint user)
        {
            ((I3D_sector *)frame)->AddToRemoveFromSceneRing((I3D_scene *)user);
            return I3DENUMRET_OK;
        }
    };
    struct EnumHelp1 {
        // FUNCTION: LS3DF 0x1001be60
        // FUNCTION: LS3DF11 0x1001b980
        static I3DENUMRET __stdcall I3D_EnumProc(I3D_frame *frame, uint user)
        {
            ((I3D_occluder *)frame)->AddToRemoveFromSceneRing((I3D_scene *)user);
            return I3DENUMRET_OK;
        }
    };
    struct EnumHelp2 {
        // FUNCTION: LS3DF 0x1001be80
        // FUNCTION: LS3DF11 0x1001b9a0
        static I3DENUMRET __stdcall I3D_EnumProc(I3D_frame *frame, uint user)
        {
            ((I3D_area *)frame)->AddToRemoveFromSceneRing((I3D_scene *)user);
            return I3DENUMRET_OK;
        }
    };

    if (frame == m_pParent)
        return I3D_OK;
    I3D_frame *old_parent = m_pParent;
    uint keep_world = flags & I3DLINK_UPDMATRIX;
    if (keep_world)
        UpdateWMatrix();
    if (m_pParent) {
        I3D_frame *frm = m_pParent;
        frm->RemoveChild(this);
        if (flags & I3DLINK_DISCARD_BOUND)
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
        m_Scale = m.GetScale();
        if (!(m_FrmFlags & FRMFLAGS_QUAT_VALID))
            m_Rot.Make(m_LocalMatrix);
        m_FrmFlags = (m_FrmFlags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID | FRMFLAGS_MAT_VALID |
            FRMFLAGS_ROT_VALID)) | FRMFLAGS_UPDATE | FRMFLAGS_QUAT_VALID;
        m_LocalMatrix.SetDir3(m(2), m(1));
        m_FrmFlags = (m_FrmFlags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID | FRMFLAGS_QUAT_VALID |
            FRMFLAGS_ROT_VALID)) | FRMFLAGS_UPDATE | FRMFLAGS_MAT_VALID;
    } else
        m_FrmFlags = (m_FrmFlags & ~FRMFLAGS_WMAT_VALID) | FRMFLAGS_UPDATE;
    if (flags & I3DLINK_UPDATE)
        Update();
    else if (flags & I3DLINK_DISCARD_BOUND)
        DiscardHRBoundVol();

    uint hr_flags = m_FrmFlags & (FRMFLAGS_HR_SECTORS | FRMFLAGS_HR_OCCLUDERS | FRMFLAGS_HR_AREAS);
    if (hr_flags) {
        I3D_frame *frm;
        for (frm = frame; frm; frm = frm->m_pParent)
            frm->m_FrmFlags |= hr_flags;
        for (frm = old_parent; frm; frm = frm->m_pParent) {
            switch (frm->m_Type) {
            case FRAME_SECTOR:
                frm->m_FrmFlags &= ~(FRMFLAGS_HR_AREAS | FRMFLAGS_HR_OCCLUDERS);
                break;
            case FRAME_OCCLUDER:
                frm->m_FrmFlags &= ~(FRMFLAGS_HR_AREAS | FRMFLAGS_HR_SECTORS);
                break;
            case FRAME_AREA:
                frm->m_FrmFlags &= ~(FRMFLAGS_HR_SECTORS | FRMFLAGS_HR_OCCLUDERS);
                break;
            default:
                frm->m_FrmFlags &= ~(FRMFLAGS_HR_SECTORS | FRMFLAGS_HR_OCCLUDERS | FRMFLAGS_HR_AREAS);
            }
            // The children's flags back: the mask is 0 (& for |), so only the walk and a store of the same flags stay,
            // in PS2's code too
            uint f = 0;
            for (I3D_frame *c = frm->m_pChild; c; c = c->m_pNext)
                f |= c->m_FrmFlags & FRMFLAGS_HR_SECTORS & FRMFLAGS_HR_OCCLUDERS & FRMFLAGS_HR_AREAS;
            frm->m_FrmFlags |= f;
        }
        I3D_scene *old_scene = NULL, *new_scene = NULL;
        for (frm = old_parent; frm; frm = frm->m_pParent) {
            if (frm->m_Type == FRAME_SECTOR) {
                old_scene = ((I3D_sector *)frm)->scene;
                break;
            }
        }
        for (frm = frame; frm; frm = frm->m_pParent) {
            if (frm->m_Type == FRAME_SECTOR) {
                new_scene = ((I3D_sector *)frm)->scene;
                break;
            }
        }
        if (old_scene != new_scene) {
            if (m_Type == FRAME_SECTOR)
                ((I3D_sector *)this)->AddToRemoveFromSceneRing(new_scene);
            EnumFrames(EnumHelp::I3D_EnumProc, (uint)new_scene, ENUMF_SECTOR, NULL);
            if (m_Type == FRAME_OCCLUDER)
                ((I3D_occluder *)this)->AddToRemoveFromSceneRing(new_scene);
            EnumFrames(EnumHelp1::I3D_EnumProc, (uint)new_scene, ENUMF_OCCLUDER, NULL);
            if (m_Type == FRAME_AREA)
                ((I3D_area *)this)->AddToRemoveFromSceneRing(new_scene);
            EnumFrames(EnumHelp2::I3D_EnumProc, (uint)new_scene, ENUMF_AREA, NULL);
        }
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1001bea0
// FUNCTION: LS3DF11 0x1001b9c0
I3D_frame *I3D_frame::AddChild(I3D_frame *frame)
{
    m_FrmFlags &= ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_HR_BOUND_VALID);
    if (!m_pChild) {
        m_pChild = frame;
        frame->m_pNext = NULL;
        frame->m_pPrev = NULL;
        frame->m_pParent = this;
        m_numChildren = 1;
    } else {
        frame->m_pNext = m_pChild;
        frame->m_pPrev = NULL;
        m_pChild->m_pPrev = frame;
        m_pChild = frame;
        frame->m_pParent = this;
        m_numChildren++;
    }
    return frame;
}

// FUNCTION: LS3DF 0x1001bf20
// FUNCTION: LS3DF11 0x1001ba40
I3D_frame *I3D_frame::RemoveChild(I3D_frame *frame)
{
    if (frame->m_pParent != this)
        return frame;
    m_numChildren--;
    if (frame->m_pPrev) {
        frame->m_pPrev->m_pNext = frame->m_pNext;
        if (frame->m_pNext) {
            frame->m_pNext->m_pPrev = frame->m_pPrev;
            frame->m_pNext = NULL;
        }
        frame->m_pPrev = NULL;
    } else if (frame->m_pNext) {
        m_pChild = frame->m_pNext;
        frame->m_pNext->m_pPrev = NULL;
        frame->m_pNext = NULL;
    } else {
        m_pChild = NULL;
        m_numChildren = 0;
    }
    frame->m_pPrev = NULL;
    frame->m_pNext = NULL;
    frame->m_pParent = NULL;
    m_FrmFlags &= ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_HR_BOUND_VALID);
    return frame;
}

// FUNCTION: LS3DF 0x1001bfd0
// FUNCTION: LS3DF11 0x1001baf0
I3D_frame *I3D_frame::GetChild(int index) const
{
    if (index < m_numChildren) {
        for (I3D_frame *frm = m_pChild; frm; frm = frm->m_pNext) {
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
// FUNCTION: LS3DF11 0x1001bb40
LS3D_RESULT I3D_frame::EnumSubtree(I3D_ENUMPROC *proc, uint user, uint flags, const char *mask) const
{
    uint type_flags = flags & ENUMF_ALL;
    if (type_flags != ENUMF_ALL) {
        switch (m_Type) {
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
        if (!m_pOrigName)
            goto children;
        if (!(flags & ENUMF_WILDMASK)) {
            if (flags & ENUMF_CASESENS) {
                if (strcmp(mask, m_pOrigName))
                    goto children;
            } else if (stricmp(mask, m_pOrigName))
                goto children;
        } else if (flags & ENUMF_CASESENS) {
            int len = min(strlen(mask), strlen(m_pOrigName)) + 1;
            for (const char *cp = mask, *cp1 = m_pOrigName; len--; cp++, cp1++) {
                if (*cp == '*')
                    break;
                if (*cp != '?' && *cp != *cp1)
                    goto children;
            }
        } else {
            int len = min(strlen(mask), strlen(m_pOrigName)) + 1;
            for (const char *cp = mask, *cp1 = m_pOrigName; len--; cp++, cp1++) {
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
        for (I3D_frame *frm = m_pChild; frm; ) {
            I3D_frame *frm_next = frm->m_pNext;
            if (frm->EnumSubtree(proc, user, flags, mask) == I3DERR_CANCELED)
                return I3DERR_CANCELED;
            frm = frm_next;
        }
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1001c2a0
// FUNCTION: LS3DF11 0x1001bdc0
LS3D_RESULT I3D_frame::EnumFrames(I3D_ENUMPROC *proc, uint user, uint flags, const char *mask) const
{
    if (m_pChild) {
        lastDestroyedFrame = NULL;
        for (I3D_frame *frm = m_pChild; frm; ) {
            I3D_frame *frm_next = frm->m_pNext;
            if (frm->EnumSubtree(proc, user, flags, mask) == I3DERR_CANCELED)
                return I3DERR_CANCELED;
            frm = frm_next;
        }
    }
    return I3D_OK;
}

// PS2 binds it globally: not static, so VC6 emits it here, not after FindChildFrame
// FUNCTION: LS3DF 0x1001c310
// FUNCTION: LS3DF11 0x1001be30
I3DENUMRET __stdcall FindChildCallBack(I3D_frame *frame, uint user)
{
    *(I3D_frame **)user = frame;
    return I3DENUMRET_CANCEL;
}

// FUNCTION: LS3DF 0x1001c330
// FUNCTION: LS3DF11 0x1001be50
I3D_frame *I3D_frame::FindChildFrame(const char *frame_name, uint flags) const
{
    I3D_frame *frm = NULL;
    EnumFrames(FindChildCallBack, (uint)&frm, flags, frame_name);
    return frm;
}

// FUNCTION: LS3DF 0x1001c360
// FUNCTION: LS3DF11 0x1001be80
I3D_frame *I3D_frame::GetChildFrame(const char *frame_name, uint flags)
{
    uint type_flags = flags & ENUMF_ALL;
    uint ok;
    if (type_flags != ENUMF_ALL) {
        switch (m_Type) {
        case FRAME_VISUAL: ok = type_flags & ENUMF_VISUAL; break;
        case FRAME_LIGHT: ok = type_flags & ENUMF_LIGHT; break;
        case FRAME_CAMERA: ok = type_flags & ENUMF_CAMERA; break;
        case FRAME_SOUND: ok = type_flags & ENUMF_SOUND; break;
        case FRAME_SECTOR: ok = type_flags & ENUMF_SECTOR; break;
        case FRAME_DUMMY: ok = type_flags & ENUMF_DUMMY; break;
        case FRAME_MODEL: ok = type_flags & ENUMF_MODEL; break;
        case FRAME_USER: ok = type_flags & ENUMF_USER; break;
        case FRAME_VOLUME: ok = type_flags & ENUMF_VOLUME; break;
        case FRAME_SHADOW: ok = type_flags & ENUMF_SHADOW; break;
        case FRAME_TARGET: ok = type_flags & ENUMF_TARGET; break;
        case FRAME_AREA: ok = type_flags & ENUMF_AREA; break;
        default: ok = 0;
        }
    } else
        ok = 1;
    if (ok && frame_name) {
        if (!m_pName)
            ok = 0;
        else if (!(flags & ENUMF_WILDMASK)) {
            if (flags & ENUMF_CASESENS) {
                if (strcmp(frame_name, m_pName))
                    ok = 0;
            } else if (stricmp(frame_name, m_pName))
                ok = 0;
        } else if (flags & ENUMF_CASESENS) {
            int len = min(strlen(frame_name), strlen(m_pName)) + 1;
            for (const char *cp = frame_name, *cp1 = m_pName; len--; cp++, cp1++) {
                if (*cp == '*')
                    break;
                if (*cp != '?' && *cp != *cp1) {
                    ok = 0;
                    break;
                }
            }
        } else {
            int len = min(strlen(frame_name), strlen(m_pName)) + 1;
            for (const char *cp = frame_name, *cp1 = m_pName; len--; cp++, cp1++) {
                if (*cp == '*')
                    break;
                if (*cp != '?' && toupper(*cp) != toupper(*cp1)) {
                    ok = 0;
                    break;
                }
            }
        }
    }
    if (ok)
        return this;
    for (I3D_frame *frm = m_pChild; frm; frm = frm->m_pNext) {
        I3D_frame *found = frm->GetChildFrame(frame_name, flags);
        if (found)
            return found;
    }
    return NULL;
}

// FUNCTION: LS3DF 0x1001c5b0
// FUNCTION: LS3DF11 0x1001c0d0
LS3D_RESULT I3D_frame::Duplicate(I3D_frame *frame)
{
    if (!frame)
        return I3DERR_INVALIDPARAMS;
    if (m_FrmFlags & FRMFLAGS_FFTREE)
        RemoveFromFFTree();
    delete[] m_pName;
    uint flags = m_FrmFlags;
    memcpy(&m_LocalMatrix, &frame->m_LocalMatrix, sizeof(S_matrix) + sizeof(S_vector) + sizeof(S_quat));
    m_FrmFlags = (frame->m_FrmFlags & FRMFLAGS_DUPLICATE_MASK) |
        (flags & (FRMFLAGS_TICK | FRMFLAGS_SHADOW | FRMFLAGS_TARGET)) | FRMFLAGS_UPDATE;
    m_pCallback = frame->m_pCallback;
    m_UpdateCount++;
    SetProperty(frame->m_pProperty);
    if (!frame->m_pName) {
        m_pName = NULL;
        m_pOrigName = NULL;
    } else {
        m_pName = new char[strlen(frame->m_pName) + 1];
        if (!m_pName) {
            m_pOrigName = NULL;
            dbgPrintf("!! Out of memory  I3D_frame::Duplicate()");
            ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
            return I3DERR_OUTOFMEM;
        }
        strcpy(m_pName, frame->m_pName);
        if (renderState[RS_FAST_FIND])
            AddToFFTree();
        m_pOrigName = frame->m_pOrigName - frame->m_pName + m_pName;
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1001c700
// FUNCTION: LS3DF11 0x1001c220
LS3D_RESULT I3D_frame::SetProperty(const char *prop)
{
    delete[] m_pProperty;
    if (!prop) {
        m_pProperty = NULL;
        return I3D_OK;
    }
    int len = strlen(prop);
    if (!len) {
        m_pProperty = NULL;
        return I3D_OK;
    }
    m_pProperty = new char[len + 1];
    if (!m_pProperty)
        return I3DERR_OUTOFMEM;
    strcpy(m_pProperty, prop);
    return I3D_OK;
}

// The targets to update after the frames (I3D_target's), defined here: its $E is between SetProperty and
// UpdateWMatrixProc
// GLOBAL: LS3DF 0x100c0034
// GLOBAL: LS3DF11 0x100bd154
C_list<I3D_target *> UpdateTargetList;

// The original calls it out of line from this file too, where VC6 inlines ours; why isn't known (writing
// UpdateMatrix's code out keeps it out of line too, but then VC6 merges more of the two branches' scaling)
#pragma auto_inline(off)

// FUNCTION: LS3DF 0x1001c7d0
// FUNCTION: LS3DF11 0x1001c2f0
void I3D_frame::UpdateWMatrixProc()
{
    UpdateMatrix();
    if (m_pParent) {
        m_pParent->UpdateWMatrix();
        m_Matrix = m_LocalMatrix * m_pParent->m_Matrix;
        m_WorldScale = (m_Scale.x + m_Scale.y + m_Scale.z) * m_pParent->m_WorldScale * (1.0f / 3.0f);
    } else {
        m_Matrix = m_LocalMatrix;
        m_WorldScale = (m_Scale.x + m_Scale.y + m_Scale.z) * (1.0f / 3.0f);
    }
    m_FrmFlags |= FRMFLAGS_WMAT_VALID;
}

#pragma auto_inline(on)

// Inline: Update inlines it, the recursion needs the copy (/Ob2 alone takes only small functions). Our copy comes
// after Update, the original's before it: open
// FUNCTION: LS3DF 0x1001c9a0
// FUNCTION: LS3DF11 0x1001c4c0
inline void I3D_frame::UpdateChildren()
{
    uint changed = m_FrmFlags & (FRMFLAGS_UPDATE | FRMFLAGS_CHANGED);
    if (changed) {
        m_UpdateCount++;
        if (m_Type == FRAME_VISUAL && !(((I3D_visual *)this)->vis_flags & VISF_WORLD_SPACE))
            ((I3D_visual *)this)->vis_flags &= ~VISF_WBOUND_VALID;
    }
    for (I3D_frame *frm = m_pChild; frm; frm = frm->m_pNext) {
        if (changed)
            frm->m_FrmFlags = (frm->m_FrmFlags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID)) | FRMFLAGS_CHANGED;
        frm->UpdateChildren();
        frm->UpdateV();
        if (frm->m_FrmFlags & (FRMFLAGS_UPDATE | FRMFLAGS_CHANGED)) {
            frm->m_FrmFlags &= ~(FRMFLAGS_UPDATE | FRMFLAGS_CHANGED);
            m_FrmFlags = (m_FrmFlags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_HR_BOUND_VALID)) | FRMFLAGS_CHANGED;
        }
    }
}

// FUNCTION: LS3DF 0x1001ca60
// FUNCTION: LS3DF11 0x1001c580
void I3D_frame::Update()
{
    UpdateChildren();
    UpdateV();
    DiscardHRBoundVol();
    if (m_FrmFlags & (FRMFLAGS_UPDATE | FRMFLAGS_CHANGED) && m_Type == FRAME_VISUAL &&
        !(((I3D_visual *)this)->vis_flags & VISF_WORLD_SPACE))
        ((I3D_visual *)this)->vis_flags &= ~VISF_WBOUND_VALID;
    m_FrmFlags &= ~(FRMFLAGS_UPDATE | FRMFLAGS_CHANGED);
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
// FUNCTION: LS3DF11 0x1001c720
void I3D_frame::DiscardHRBoundVol()
{
    if (m_Type == FRAME_SECTOR)
        return;
    m_FrmFlags &= ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_HR_BOUND_VALID);
    for (I3D_frame *frm = m_pParent; frm; frm = frm->m_pParent) {
        if (!(frm->m_FrmFlags & FRMFLAGS_HR_BOUND_VALID))
            break;
        if (frm->m_Type == FRAME_SECTOR)
            break;
        frm->m_FrmFlags &= ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_HR_BOUND_VALID);
    }
}

// Empty for a frame of no other type, folded with the other out-of-line empty functions (I3D.h)
void I3D_frame::UpdateV()
{
}

// FUNCTION: LS3DF 0x1001cc50
// FUNCTION: LS3DF11 0x1001c770
void I3D_frame::UpdateHRBoundVol()
{
    if (m_Type == FRAME_SECTOR) {
        m_FrmFlags |= FRMFLAGS_HR_BOUND_VALID;
        return;
    }
    if (m_Type == FRAME_VISUAL) {
        I3D_visual *vis = (I3D_visual *)this;
        if (!(vis->vis_flags & VISF_BOUND_VALID))
            vis->UpdateBoundProc();
        m_HRBoundVol.bbox = vis->vis_bound.bbox;
    } else
        m_HRBoundVol.bbox.Invalidate();
    for (I3D_frame *frm = m_pChild; frm; frm = frm->m_pNext) {
        if (frm->m_Type == FRAME_VOLUME)
            continue;
        if (!(frm->m_FrmFlags & FRMFLAGS_ON))
            continue;
        if (!(frm->m_FrmFlags & FRMFLAGS_HR_BOUND_VALID))
            frm->UpdateHRBoundVol();
        if (frm->m_HRBoundVol.bbox.IsValid()) {
            const S_vector *v = BBoxVertices(frm->m_HRBoundVol.bbox);
            // UpdateMatrix stays out of line: a call inside an inline function gets only a share of the budget
            const S_matrix &m = frm->GetLocalMatrix();
            for (int i = 8; i--; v++)
                m_HRBoundVol.bbox.Absorb(*v * m);
        }
    }
    if (m_HRBoundVol.bbox.IsValid()) {
        S_vector half = (m_HRBoundVol.bbox.max - m_HRBoundVol.bbox.min) * 0.5f;
        m_HRBoundVol.bsphere.pos = m_HRBoundVol.bbox.min + half;
        m_HRBoundVol.bsphere.radius = (float)sqrt(half.Square());
    } else
        m_HRBoundVol.bsphere.radius = 0.0f;
    m_FrmFlags |= FRMFLAGS_HR_BOUND_VALID;
}

// The local bound volume of a box (PC only, the name is a guess)
// FUNCTION: LS3DF 0x1001ce80
// FUNCTION: LS3DF11 0x1001c9a0
void I3D_frame::SetBoundVolume(const I3D_bbox &bbox)
{
    if (bbox.min.x <= bbox.max.x) {
        m_HRBoundVol.bbox = bbox;
        S_vector half = (bbox.max - bbox.min) * 0.5f;
        m_HRBoundVol.bsphere.pos = bbox.min + half;
        m_HRBoundVol.bsphere.radius = (float)sqrt(half.Square());
    } else {
        m_HRBoundVol.bbox.Invalidate();
        m_HRBoundVol.bsphere.radius = 0.0f;
    }
    m_FrmFlags = (m_FrmFlags & ~FRMFLAGS_HR_BOUND_W_VALID) | FRMFLAGS_HR_BOUND_VALID;
}

// FUNCTION: LS3DF 0x1001cf90
// FUNCTION: LS3DF11 0x1001cab0
void I3D_frame::UpdateHRBoundVolW()
{
    if (!(m_FrmFlags & FRMFLAGS_HR_BOUND_VALID))
        UpdateHRBoundVol();
    m_HRBoundVolW.bbox.Invalidate();
    if (m_HRBoundVol.bbox.min.x <= m_HRBoundVol.bbox.max.x) {
        const S_vector *v = BBoxVertices(m_HRBoundVol.bbox);
        m_HRBoundVolW.bbox.min = m_HRBoundVolW.bbox.max = *v * GetMatrix();
        for (int i = 7; i--; ) {
            v++;
            m_HRBoundVolW.bbox.Absorb(*v * GetMatrix());
        }
        S_vector half = (m_HRBoundVolW.bbox.max - m_HRBoundVolW.bbox.min) * 0.5f;
        m_HRBoundVolW.bsphere.pos = m_HRBoundVolW.bbox.min + half;
        m_HRBoundVolW.bsphere.radius = (float)sqrt(half.Square());
    } else {
        m_HRBoundVolW.bsphere.pos = S_vector(0.0f, 0.0f, 0.0f);
        m_HRBoundVolW.bsphere.radius = 0.0f;
    }
    m_FrmFlags |= FRMFLAGS_HR_BOUND_W_VALID;
}

// Empty, as UpdateV
void I3D_frame::SaveImage()
{
}

// FUNCTION: LS3DF 0x1001d1c0
// FUNCTION: LS3DF11 0x1001cce0
void I3D_frame::LoadImage()
{
    dtaRead(LoadImageHandle, &m_LocalMatrix(3), sizeof(S_vector));
    // The scale, the rotation and the file's flags byte, which lands in m_FrmFlags' low byte
    dtaRead(LoadImageHandle, &m_Scale, 0x1d);
    int len = 0;
    dtaRead(LoadImageHandle, &len, 1);
    if (len) {
        m_pName = new char[len + 1];
        if (!m_pName)
            goto fail;
        m_pOrigName = m_pName;
        dtaRead(LoadImageHandle, m_pName, len);
        m_pName[len] = 0;
        if (renderState[RS_FAST_FIND])
            AddToFFTree();
    }
    dtaRead(LoadImageHandle, &len, 1);
    if (len) {
        m_pProperty = new char[len + 1];
        if (!m_pProperty)
            goto fail;
        dtaRead(LoadImageHandle, m_pProperty, len);
        m_pProperty[len] = 0;
    }
    m_FrmFlags = (m_FrmFlags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_HR_BOUND_VALID | FRMFLAGS_WMAT_VALID |
        FRMFLAGS_MAT_VALID | FRMFLAGS_ROT_VALID)) | FRMFLAGS_UPDATE | FRMFLAGS_QUAT_VALID;
    return;
fail:
    dbgPrintf("!! Out of memory  I3D_frame::LoadImage()");
    ImageErrFlg = I3DERR_OUTOFMEM;
    ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
}
