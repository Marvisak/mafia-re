// The collision volume (PS2 I3d_volume.cpp), at 0x1005a7a0-0x1005aad0 in the original: I3D_volume in PS2's order, the
// destructor after the constructor. The header's zero vector has its $E at the end (0x1005aab0). Names are the PS2
// build's unless noted.
#include "I3D/I3D.h"

// The members before the box are zeroed before the vtable is set: they are initialized, the others assigned
// FUNCTION: LS3DF 0x1005a7a0
I3D_volume::I3D_volume() :
    volume_type(0),
    unknown_144(0),
    unknown_148(0),
    unknown_14c(0),
    unknown_150(0)
{
    type = FRAME_VOLUME;
    bbox.Invalidate();
    unknown_16c = 0;
    unknown_170 = 0;
}

// FUNCTION: LS3DF 0x1005a840
I3D_volume::~I3D_volume()
{
}

// FUNCTION: LS3DF 0x1005a850
uint I3D_volume::GetVolumeType() const
{
    return volume_type;
}

// A moved volume's scale is made to fit its type; the world matrix is the local one, whatever the parent.
// Not matching yet: the inline budget. The original calls S_vector::operator*= out of line for all six rows
// UpdateMatrix scales (its copy at 0x10056e60), ours inlines them, so the original had less budget left here. Empty
// inline calls after UpdateMatrix leave five of them out of line, but never the first branch's first row.
// FUNCTION: LS3DF 0x1005a860
void I3D_volume::UpdateV()
{
    if (frm_flags & FRMFLAGS_UPDATE) {
        switch (volume_type) {
        case 1:
            SetScale(S_vector(1.0f, 1.0f, 1.0f));
            SetPos(GetWorldPos());
            break;
        case 5:
        case 6:
            SetScale(S_vector(scale.x, scale.x, scale.x));
            break;
        case 4:
            {
                S_vector s = scale;
                s.y = 1.0f;
                SetScale(s);
            }
            break;
        }
    }
    UpdateMatrix();
    matrix = f_matrix;
    frm_flags |= FRMFLAGS_WMAT_VALID;
}
