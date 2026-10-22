// The user frame (PS2 I3d_user.cpp), at 0x1005a140-0x1005a200 in the original: I3D_user in PS2's order, the destructor
// after the constructor. The header's zero vector has its $E at the end (0x1005a1e0). Names are the PS2 build's.
#include "I3D/I3D.h"

// FUNCTION: LS3DF 0x1005a140
// FUNCTION: LS3DF11 0x10057800
I3D_user::I3D_user()
{
    type = FRAME_USER;
    add_data = 0;
}

// Declared, though empty: VC6's implicit destructor wouldn't set the vtable again
// FUNCTION: LS3DF 0x1005a190
// FUNCTION: LS3DF11 0x10057850
I3D_user::~I3D_user()
{
}

// FUNCTION: LS3DF 0x1005a1a0
// FUNCTION: LS3DF11 0x10057860
LS3D_RESULT I3D_user::Duplicate(I3D_frame *frame)
{
    if (!frame)
        return I3DERR_INVALIDPARAMS;
    if (frame->GetType() == FRAME_USER)
        add_data = ((I3D_user *)frame)->add_data;
    return I3D_frame::Duplicate(frame);
}
