// The dummy frame (PS2 I3d_dummy.cpp), at 0x1001a1e0-0x1001a6e0 in the original: I3D_dummy in PS2's order, with the
// PC's UpdateV after the destructor and Duplicate. The header's zero vector has its $E at the end (0x1001a6c0), and
// after it comes the copy of the frame's inline UpdateMatrix the linker kept (see tools/autopch.py). Names are the PS2
// build's unless noted.
#include "I3D/I3D.h"
#include "dta.h"

// FUNCTION: LS3DF 0x1001a1e0
I3D_dummy::I3D_dummy()
{
    type = FRAME_DUMMY;
    bbox.Invalidate();
    dummy_flags = 0;
}

// FUNCTION: LS3DF 0x1001a260
I3D_dummy::~I3D_dummy()
{
    if (frm_flags & FRMFLAGS_WEATHER) {
        for (int i = pI3DDriver->scenes.size(); i--; )
            pI3DDriver->scenes[i]->WeatherSystemDelDummy(this);
    }
}

// FUNCTION: LS3DF 0x1001a2e0
LS3D_RESULT I3D_dummy::Duplicate(I3D_frame *frame)
{
    if (!frame)
        return I3DERR_INVALIDPARAMS;
    if (frame->GetType() == FRAME_DUMMY)
        bbox = ((I3D_dummy *)frame)->bbox;
    return I3D_frame::Duplicate(frame);
}

// FUNCTION: LS3DF 0x1001a330
void I3D_dummy::UpdateV()
{
    if (frm_flags & (FRMFLAGS_UPDATE | FRMFLAGS_CHANGED))
        dummy_flags &= ~DUMMYF_INV_MAT_VALID;
}

// Which of S_vector's constructors VC6 leaves out of line here is the inline budget's choice: the GetScale() calls
// make the difference, the second branch's before its size (after it, the size's constructor stays out of line).
// FUNCTION: LS3DF 0x1001a350
void I3D_dummy::Normalize(uint mode)
{
    if (mode == 2) {
        S_vector pos = (bbox.max + bbox.min) * 0.5f;
        SetPos(pos * GetLocalMatrix());
        S_vector size = (bbox.max - bbox.min) * 0.5f;
        const S_vector &s = GetScale();
        SetScale(S_vector(size.x * s.x, size.y * s.y, size.z * s.z));
        bbox.min = S_vector(-1.0f, -1.0f, -1.0f);
        bbox.max = S_vector(1.0f, 1.0f, 1.0f);
        dummy_flags = (dummy_flags & ~(DUMMYF_INV_MAT_VALID | DUMMYF_NORMALIZED)) | DUMMYF_NORMALIZED_CENTER;
    } else {
        S_vector pos = bbox.min;
        SetPos(pos * GetLocalMatrix());
        const S_vector &s = GetScale();
        S_vector size = bbox.max - bbox.min;
        SetScale(S_vector(size.x * s.x, size.y * s.y, size.z * s.z));
        bbox.min.Zero();
        bbox.max = S_vector(1.0f, 1.0f, 1.0f);
        dummy_flags = (dummy_flags & ~(DUMMYF_INV_MAT_VALID | DUMMYF_NORMALIZED_CENTER)) | DUMMYF_NORMALIZED;
    }
}

// FUNCTION: LS3DF 0x1001a6a0
void I3D_dummy::LoadImage()
{
    I3D_frame::LoadImage();
    dtaRead(LoadImageHandle, &bbox, sizeof(bbox));
}
