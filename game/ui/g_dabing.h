#pragma once

#include "Main.h"

#include <vector>

class I3D_sound;
class I3D_frame;
class I3D_visual;

// The dubbing, the characters' speech (PS2 g_dabing.cpp, "dabing" Czech for dubbing). Only what C_game's Init and Done
// call so far.

// The speaking channels (PS2's 16 of dabingChanel; the PC's are 0x44 bytes, PS2's 0x48)
#define NUM_DABING_CHANELS 16

// A channel's state that leaves its sound alone at DabingDone (the name is ours; PS2's DabingDone tests the same 5)
#define DABING_STATE_UNKNOWN_5 5

// A speaking channel (the name is ours, PS2's array is dabingChanel; the members' names are ours, after the speech at
// 0x5f5570), 0x44 bytes
struct S_dabing_chanel
{
    uint *data;                         // 0x0, tables\dat\<speech>.dat, new
    uchar unknown_4[8];
    int state;                          // 0xc, 2 while speaking
    uchar unknown_10[4];
    I3D_sound *sound;                   // 0x14, sounds\<speech>.wav, a sound frame of the driver's
    I3D_frame *unknown_18;              // 0x18, the frame the sound is linked to
    I3D_visual *face;                   // 0x1c, the speaker's face, whose weights DabingReset sets back
    uchar unknown_20[0x44 - 0x20];
};

// A morph of a speaking face (PS2's name, 0x10 bytes; its members are unknown, dwords on PC as its copies show)
struct G_DabingMorphObj
{
    uint unknown_0;
    uint unknown_4;
    uint unknown_8;
    uint unknown_c;
};

// PS2 names
extern S_dabing_chanel dabingChanel[NUM_DABING_CHANELS];
extern std::vector<G_DabingMorphObj> G_DabingMorphActive;

// PS2 names: the channels cleared, and their sounds released at the game's end
void DabingInit();
void DabingDone();
// Each channel's face gets its weights back, in each of three regions the first level 1, the other four 0; then the
// channels' data and sounds are freed (the data first, where DabingDone frees the sound first) and the channels
// cleared, as DabingDone does (PC only, the name is ours; C_cutscene::Close calls it)
void DabingReset();
