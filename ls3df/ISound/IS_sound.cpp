// Sounds (PS2 IS_sound.cpp). On PC it starts at 0x1007c2a0 with the hardware channel lists ChannelList and
// TerminatedChannelList, UpdateAllTerminatedBuffs and TraceDSErrors: PS2's __sinit_IS_sound.cpp constructs the two
// lists. Not written yet: only what other files need to link.
#include "isound.h"

// Without its vtable and reference count
// STUB: LS3DF 0x1007c8e0
ISND_sound::ISND_sound()
{
    unknown_08 = 0;
    unknown_0c = 0;
    unknown_14 = 1.0f;
    unknown_18 = 1.0f;
}
