// The dubbing (PS2 g_dabing.cpp; the PC's file is most likely the one at 0x5f4b60-0x5f6a1c, between g_camera.cpp and
// g_indicators.cpp as the names sort, docs/game-files.md). Written so far: the globals, DabingInit, DabingDone,
// DabingGetFreeChanel, DabingReset (PC only), which C_recorder::Close calls, and DabingTick, which C_game::Tick
// calls; stubs of the rest the scripts call.
//
// Open: the original's .bss has G_DabingMorphActive (0x6bdb40) before dabingChanel (0x6bdb50), the reverse of PS2's
// definition order, where their buckets (tools/bsshash.py: 885 and 135) give the other order; as g_indicators.cpp's,
// what an object compiled with another file's precompiled header (/Yu) gives.

#include "ui/g_dabing.h"

#include <string.h>

#include "I3D/I3D.h"
#include "stub.h"

// PS2 names, in PS2's order; the vector's initializer starts the file
// GLOBAL: GAME 0x6bdb50
S_dabing_chanel dabingChanel[NUM_DABING_CHANELS];
// GLOBAL: GAME 0x6bdb40
std::vector<G_DabingMorphObj> G_DabingMorphActive;

// FUNCTION: GAME 0x5f4bc0
void DabingInit()
{
    memset(dabingChanel, 0, sizeof(dabingChanel));
    G_DabingMorphActive.clear();
}

// PS2's frees no data
// FUNCTION: GAME 0x5f4c10
void DabingDone()
{
    for (int i = 0; i < NUM_DABING_CHANELS; i++)
    {
        if (dabingChanel[i].sound && dabingChanel[i].state != DABING_STATE_SCENE_SOUND)
            dabingChanel[i].sound->Release();
        dabingChanel[i].sound = NULL;
        if (dabingChanel[i].data)
            delete dabingChanel[i].data;
    }
    memset(dabingChanel, 0, sizeof(dabingChanel));
    G_DabingMorphActive.clear();
}

// FUNCTION: GAME 0x5f4ca0
int DabingGetFreeChanel()
{
    for (int i = NUM_DABING_CHANELS - 1; i >= 0; i--)
        if (!dabingChanel[i].state)
            return i;
    return NUM_DABING_CHANELS - 1;
}

// STUB: GAME 0x5f4cd0
void DabingSetHuman(int chanel, C_human *human)
{
    StubLog("DabingSetHuman");
}

// FUNCTION: GAME 0x5f5060
void DabingReset()
{
    for (int i = 0; i < NUM_DABING_CHANELS; i++)
    {
        if (dabingChanel[i].face)
        {
            dabingChanel[i].face->SetWeight(0, 0, 1.0f);
            dabingChanel[i].face->SetWeight(0, 1, 0.0f);
            dabingChanel[i].face->SetWeight(0, 2, 0.0f);
            dabingChanel[i].face->SetWeight(0, 3, 0.0f);
            dabingChanel[i].face->SetWeight(0, 4, 0.0f);
            dabingChanel[i].face->SetWeight(1, 0, 1.0f);
            dabingChanel[i].face->SetWeight(1, 1, 0.0f);
            dabingChanel[i].face->SetWeight(1, 2, 0.0f);
            dabingChanel[i].face->SetWeight(1, 3, 0.0f);
            dabingChanel[i].face->SetWeight(1, 4, 0.0f);
            dabingChanel[i].face->SetWeight(2, 0, 1.0f);
            dabingChanel[i].face->SetWeight(2, 1, 0.0f);
            dabingChanel[i].face->SetWeight(2, 2, 0.0f);
            dabingChanel[i].face->SetWeight(2, 3, 0.0f);
            dabingChanel[i].face->SetWeight(2, 4, 0.0f);
        }
        if (dabingChanel[i].data)
            delete dabingChanel[i].data;
        if (dabingChanel[i].sound && dabingChanel[i].state != DABING_STATE_SCENE_SOUND)
            dabingChanel[i].sound->Release();
        dabingChanel[i].sound = NULL;
    }
    memset(dabingChanel, 0, sizeof(dabingChanel));
    G_DabingMorphActive.clear();
}

// STUB: GAME 0x5f53d0
void DabingInvalidate(C_human *human)
{
    StubLog("DabingInvalidate");
}

// STUB: GAME 0x5f5570
void DabingSpeak(int chanel, uint speech, bool subtitle, uint kind)
{
    StubLog("DabingSpeak");
}

// STUB: GAME 0x5f5c40
bool DabingIsSpeaking(int chanel)
{
    StubLog("DabingIsSpeaking");
    return false;
}

// STUB: GAME 0x5f5cc0
void DabingStop(int chanel)
{
    StubLog("DabingStop");
}

// The speaking channels: a channel's time runs on by the time passed, or by its sound's position while the sound plays
// (the speech ends when the sound stopped, or went back to its start near its end), and its data's frame at that time
// gives the face's weights, blended in from the last weights over 125 ms. After the speech the mouth closes from the
// last frame, whose weights the next speech blends from. Then the morphs: each moves its face from one level to another
// in regions 0 and 1 over its duration, and is dropped at its end. PS2's also ends a speech by a second clock, 2.6 s
// past its data's end or 1.8 s past its sound's; the PC's has neither.
// The weights' variables are named by their index in weight[], which the frame's bytes fill in another order. Each case
// declares them first, in that order, with an initializer the frame overwrites: the original's operand order needs them
// first seen in that order (VC6's live-range ids), and PS2's registers follow it too.
// FUNCTION: GAME 0x5f5dd0
void DabingTick(uint time)
{
    uint frame;
    for (int i = 0; i < NUM_DABING_CHANELS; i++)
    {
        if (!dabingChanel[i].state)
            continue;
        if (!dabingChanel[i].sound)
            dabingChanel[i].time += time;
        if (dabingChanel[i].state == DABING_STATE_SPEAKING)
        {
            if (dabingChanel[i].sound)
            {
                if (dabingChanel[i].sound->IsPlaying(false))
                {
                    uint cur = dabingChanel[i].sound->GetCurrTime();
                    if (dabingChanel[i].time > cur
                        && dabingChanel[i].time + 300 > dabingChanel[i].sound->GetPlayTime())
                    {
                        dabingChanel[i].fade_time = DABING_FADE_TIME;
                        dabingChanel[i].state = DABING_STATE_FADING;
                    }
                    else
                    {
                        dabingChanel[i].time = cur;
                        frame = dabingChanel[i].time / DABING_FRAME_TIME;
                    }
                }
                else
                {
                    dabingChanel[i].fade_time = DABING_FADE_TIME;
                    dabingChanel[i].state = DABING_STATE_FADING;
                }
            }
            else
            {
                frame = dabingChanel[i].time / DABING_FRAME_TIME;
                if (dabingChanel[i].data)
                {
                    if (frame >= *dabingChanel[i].data - 1)
                    {
                        dabingChanel[i].fade_time = DABING_FADE_TIME;
                        dabingChanel[i].state = DABING_STATE_FADING;
                    }
                }
                else
                {
                    dabingChanel[i].fade_time = DABING_FADE_TIME;
                    dabingChanel[i].state = DABING_STATE_FADING;
                }
            }
        }
        switch (dabingChanel[i].state)
        {
        case DABING_STATE_SCENE_SOUND:
            // the sound's position, whether it plays or not
            if (!dabingChanel[i].sound)
                break;
            dabingChanel[i].time = dabingChanel[i].sound->GetCurrTime();
            frame = dabingChanel[i].time / DABING_FRAME_TIME;
        case DABING_STATE_SPEAKING:
        {
            float w0 = 0.0f, w1 = 0.0f, w2 = 0.0f, w3 = 0.0f, w4 = 0.0f, w5 = 0.0f;
            if (!dabingChanel[i].data)
                break;
            if (!dabingChanel[i].face)
                break;
            if (frame >= *dabingChanel[i].data)
                frame = *dabingChanel[i].data - 2;
            const uchar *fr = ((S_dabing_frame *)dabingChanel[i].data)[frame + 1].weight;
            w1 = fr[0] * (1.0f / 255);
            w0 = fr[1] * (1.0f / 255);
            w2 = fr[2] * (1.0f / 255);
            w4 = fr[3] * (1.0f / 255);
            w3 = fr[4] * (1.0f / 255);
            w5 = fr[5] * (1.0f / 255);
            if (dabingChanel[i].blend < 1.0f)
            {
                dabingChanel[i].blend += time * .008f;
                if (dabingChanel[i].blend < 1.0f)
                {
                    float f = dabingChanel[i].blend;
                    w0 = w0 * f + dabingChanel[i].weight[0] * (1.0f - dabingChanel[i].blend);
                    w1 = w1 * f + dabingChanel[i].weight[1] * (1.0f - dabingChanel[i].blend);
                    w2 = w2 * f + dabingChanel[i].weight[2] * (1.0f - dabingChanel[i].blend);
                    w3 = w3 * f + dabingChanel[i].weight[3] * (1.0f - dabingChanel[i].blend);
                    w4 = w4 * f + dabingChanel[i].weight[4] * (1.0f - dabingChanel[i].blend);
                    w5 = w5 * f + dabingChanel[i].weight[5] * (1.0f - dabingChanel[i].blend);
                }
            }
            dabingChanel[i].face->SetWeight(0, 0, 0.0f);
            dabingChanel[i].face->SetWeight(0, 1, w1 * w0);
            dabingChanel[i].face->SetWeight(0, 2, (1.0f - w0) * w1);
            dabingChanel[i].face->SetWeight(0, 3, (1.0f - w2) * (1.0f - w1));
            dabingChanel[i].face->SetWeight(0, 4, (1.0f - w1) * w2);
            dabingChanel[i].face->SetWeight(1, 0, 0.0f);
            dabingChanel[i].face->SetWeight(1, 1, w4 * w3);
            dabingChanel[i].face->SetWeight(1, 2, (1.0f - w4) * w3);
            dabingChanel[i].face->SetWeight(1, 3, 1.0f - w3);
            dabingChanel[i].face->SetWeight(2, 0, 0.0f);
            dabingChanel[i].face->SetWeight(2, 1, w5 * w3);
            dabingChanel[i].face->SetWeight(2, 2, (1.0f - w5) * w3);
            dabingChanel[i].face->SetWeight(2, 3, 1.0f - w3);
            break;
        }
        case DABING_STATE_FADING:
        {
            // the last frame, its first two weights scaled down to 0 with the time left
            float w0 = 0.0f, w1 = 0.0f, w2 = 0.0f, w3 = 0.0f, w4 = 0.0f, w5 = 0.0f;
            if (dabingChanel[i].fade_time > time)
                dabingChanel[i].fade_time -= time;
            else
            {
                dabingChanel[i].fade_time = 0;
                dabingChanel[i].state = DABING_STATE_FREE;
            }
            if (!dabingChanel[i].data)
                break;
            if (!dabingChanel[i].face)
                break;
            w1 = ((S_dabing_frame *)dabingChanel[i].data)[*dabingChanel[i].data - 1].weight[0] * (1.0f / 255);
            w0 = ((S_dabing_frame *)dabingChanel[i].data)[*dabingChanel[i].data - 1].weight[1] * (1.0f / 255);
            w2 = ((S_dabing_frame *)dabingChanel[i].data)[*dabingChanel[i].data - 1].weight[2] * (1.0f / 255);
            w4 = ((S_dabing_frame *)dabingChanel[i].data)[*dabingChanel[i].data - 1].weight[3] * (1.0f / 255);
            w3 = ((S_dabing_frame *)dabingChanel[i].data)[*dabingChanel[i].data - 1].weight[4] * (1.0f / 255);
            w5 = ((S_dabing_frame *)dabingChanel[i].data)[*dabingChanel[i].data - 1].weight[5] * (1.0f / 255);
            float fade = dabingChanel[i].fade_time * (1.0f / DABING_FADE_TIME);
            w1 *= fade;
            w0 *= fade;
            dabingChanel[i].weight[0] = w0;
            dabingChanel[i].weight[1] = w1;
            dabingChanel[i].weight[2] = w2;
            dabingChanel[i].weight[3] = w3;
            dabingChanel[i].weight[4] = w4;
            dabingChanel[i].weight[5] = w5;
            dabingChanel[i].face->SetWeight(0, 0, 0.0f);
            dabingChanel[i].face->SetWeight(0, 1, w1 * w0);
            dabingChanel[i].face->SetWeight(0, 2, (1.0f - w0) * w1);
            dabingChanel[i].face->SetWeight(0, 3, (1.0f - w2) * (1.0f - w1));
            dabingChanel[i].face->SetWeight(0, 4, (1.0f - w1) * w2);
            dabingChanel[i].face->SetWeight(1, 0, 0.0f);
            dabingChanel[i].face->SetWeight(1, 1, w4 * w3);
            dabingChanel[i].face->SetWeight(1, 2, (1.0f - w4) * w3);
            dabingChanel[i].face->SetWeight(1, 3, 1.0f - w3);
            dabingChanel[i].face->SetWeight(2, 0, 0.0f);
            dabingChanel[i].face->SetWeight(2, 1, w5 * w3);
            dabingChanel[i].face->SetWeight(2, 2, (1.0f - w5) * w3);
            dabingChanel[i].face->SetWeight(2, 3, 1.0f - w3);
            break;
        }
        case DABING_STATE_SUBTITLE:
            if (dabingChanel[i].fade_time > time)
                dabingChanel[i].fade_time -= time;
            else
            {
                dabingChanel[i].fade_time = 0;
                dabingChanel[i].state = DABING_STATE_FREE;
            }
            break;
        }
    }

    if (G_DabingMorphActive.size())
    {
        for (G_DabingMorphObj *obj = G_DabingMorphActive.begin(); obj < G_DabingMorphActive.end(); obj++)
        {
            for (int j = 0; j < DABING_MORPH_LEVELS; j++)
            {
                obj->visual->SetWeight(0, j, 0.0f);
                obj->visual->SetWeight(1, j, 0.0f);
            }
            obj->time += time;
            if (obj->time > obj->duration)
            {
                obj->visual->SetWeight(0, obj->level[0][1], 1.0f);
                obj->visual->SetWeight(1, obj->level[1][1], 1.0f);
                G_DabingMorphActive.erase(obj);
                obj--;
            }
            else
            {
                float f = (float)obj->time / obj->duration;
                obj->visual->SetWeight(0, obj->level[0][0], 1.0f - f);
                obj->visual->SetWeight(1, obj->level[1][0], 1.0f - f);
                obj->visual->SetWeight(0, obj->level[0][1], f);
                obj->visual->SetWeight(1, obj->level[1][1], f);
            }
        }
    }
}

// STUB: GAME 0x5f65e0
void DabingPauseSounds(bool pause, bool unknown)
{
    StubLog("DabingPauseSounds");
}
