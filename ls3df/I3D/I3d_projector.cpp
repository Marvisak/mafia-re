// The projectors (PS2 I3d_projector.cpp), at 0x10045e30-0x10048060 in the original. Not written yet: only the
// receivers' array, which the shadows' visual (I3d_shadow.cpp) shares with the projectors. The driver has the
// constructor's stub.
#include "I3D/I3D.h"

// The visuals a projector or shadow falls on (PC only; the name is a guess)
// GLOBAL: LS3DF 0x101b4630
I3D_visual *ShadowReceivers[512];
