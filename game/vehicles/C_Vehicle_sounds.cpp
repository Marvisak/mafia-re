// The vehicles' sounds (PS2 C_Vehicle_sounds.cpp; the PC's file is the one at 0x4eac70-0x4efb3c, docs/game-files.md,
// whose functions follow PS2's from InitSounds to SoundOff, then OpenSnd and SoundInit). Written so far: the sounds'
// volume, which the profiles set, and OpenSnd, the class's virtual function.

#include "vehicles/C_Vehicle.h"
#include "I3D_cache.h"

// GLOBAL: GAME 0x634b00
float C_Vehicle::sound_volume = 1.0f;

// After SoundOff (0x4eec80), before OpenSnd
// FUNCTION: GAME 0x4ef0d0
void C_Vehicle::SetSoundVolume(float volume)
{
    sound_volume = volume;
}

// The PC's only passes the call on to the sound cache (PS2's is 1156 bytes long)
// FUNCTION: GAME 0x4ef0e0
LS3D_RESULT C_Vehicle::OpenSnd(I3D_sound *snd, const char *name, uint flags)
{
    return sound_cache.Open(snd, name, flags, NULL, NULL, NULL);
}
