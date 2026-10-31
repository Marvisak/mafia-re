#pragma once

#include "Main.h"

#include <vector>

class I3D_sound;
class I3D_frame;
class I3D_visual;
class C_human;

// The dubbing, the characters' speech (PS2 g_dabing.cpp, "dabing" Czech for dubbing). Only what C_game's Init, Done and
// Tick and the scripts call so far.

// The speaking channels (PS2's 16 of dabingChanel; the PC's are 0x44 bytes, PS2's 0x48)
#define NUM_DABING_CHANELS 16

// S_dabing_chanel::state (the names are ours)
#define DABING_STATE_FREE 0             // DabingGetFreeChanel takes it
#define DABING_STATE_SPEAKING 2         // the face follows the speech's frames
#define DABING_STATE_FADING 3           // the speech ended (DabingTick, PS2's DabingStop): the mouth closes over
                                        // fade_time
#define DABING_STATE_UNKNOWN_4 4        // waits out fade_time, the face left alone
// A channel's state that leaves its sound alone at DabingDone (the name is ours; PS2's DabingDone tests the same 5),
// whose time DabingTick takes from the sound's position, the speech never ending by it
#define DABING_STATE_UNKNOWN_5 5

// A speech's frames are 40 ms apart, and the mouth closes over 200 ms after it (the names are ours)
#define DABING_FRAME_TIME 40
#define DABING_FADE_TIME 200

// A frame of a speech's data, tables\dat\<speech>.dat (the name is ours): the weights of the face's levels, 0 to
// 255, in another order than S_dabing_chanel::weight. The data starts with a header the size of a frame, whose
// first dword counts the frames and the header.
struct S_dabing_frame
{
    uchar weight[6];
    uchar unknown_6[2];
};

// A speaking channel (the name is ours, PS2's array is dabingChanel; the members' names are ours, after the speech at
// 0x5f5570), 0x44 bytes
struct S_dabing_chanel
{
    uint *data;                         // 0x0, tables\dat\<speech>.dat, new
    uint time;                          // 0x4, the speech's time
    uint fade_time;                     // 0x8, the time left of DABING_STATE_FADING or DABING_STATE_UNKNOWN_4
    int state;                          // 0xc, DABING_STATE_*
    uchar unknown_10[4];
    I3D_sound *sound;                   // 0x14, sounds\<speech>.wav, a sound frame of the driver's
    I3D_frame *unknown_18;              // 0x18, the frame the sound is linked to
    I3D_visual *face;                   // 0x1c, the speaker's face, whose weights DabingReset sets back
    uchar unknown_20[8];
    float weight[6];                    // 0x28, the face's last weights, which the next speech blends from
    float blend;                        // 0x40, how far the speech has blended from them, 0 to 1
};

// The levels of a morph's regions that DabingTick clears (the name is ours)
#define DABING_MORPH_LEVELS 8

// A morph of a speaking face (PS2's name, 0x10 bytes; the members' names are ours): in regions 0 and 1 the face
// goes from one level to another over the duration
struct G_DabingMorphObj
{
    I3D_visual *visual;                 // 0x0
    uint duration;                      // 0x4
    uint time;                          // 0x8
    uchar level[2][2];                  // 0xc, for each region the level it goes from and the one it goes to
};

// PS2 names
extern S_dabing_chanel dabingChanel[NUM_DABING_CHANELS];
extern std::vector<G_DabingMorphObj> G_DabingMorphActive;

// PS2 names: the channels cleared, and their sounds released at the game's end
void DabingInit();
void DabingDone();
// PS2 names, what the scripts' speech calls: the last channel that isn't speaking (the last one if all are), the
// speaker of a channel, its speech (the parameters' meaning isn't known), and whether the channel still speaks
int DabingGetFreeChanel();
void DabingSetHuman(int chanel, C_human *human);
void DabingSpeak(int chanel, uint speech, bool unknown, uint unknown2);
bool DabingIsSpeaking(int chanel);
// The channels' speech and the faces' morphs moved on by the time passed, which C_game::Tick calls (PS2 name)
void DabingTick(uint time);
// Each channel's face gets its weights back, in each of three regions the first level 1, the other four 0; then the
// channels' data and sounds are freed (the data first, where DabingDone frees the sound first) and the channels
// cleared, as DabingDone does (PC only, the name is ours; C_cutscene::Close calls it)
void DabingReset();
