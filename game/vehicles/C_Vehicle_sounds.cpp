// The vehicles' sounds (PS2 C_Vehicle_sounds.cpp; the PC's file is the one at 0x4eac70-0x4efb3c, docs/game-files.md,
// whose functions follow PS2's from InitSounds to SoundOff and then SoundInit). Written so far: the sounds' volume,
// which the profiles set.

#include "vehicles/C_Vehicle.h"

// GLOBAL: GAME 0x634b00
float C_Vehicle::sound_volume = 1.0f;

// Between SoundOff (0x4eec80) and SoundInit (0x4ef110), where PS2 has OpenSnd
// FUNCTION: GAME 0x4ef0d0
void C_Vehicle::SetSoundVolume(float volume)
{
    sound_volume = volume;
}
