// The dubbing (PS2 g_dabing.cpp; the PC's file is most likely the one at 0x5f4b60-0x5f6a1c, between g_camera.cpp and
// g_indicators.cpp as the names sort, docs/game-files.md), written whole: the vector's initializer, then the functions
// in the original's order. Unlike PS2's, the PC's loads a speech's data from the archives when it speaks (PS2 has
// DabingPreLoad and DabingPreUnload, the PC neither), and adds the speaker's targets and the cutscenes' functions.
//
// Open: the original's .bss has G_DabingMorphActive (0x6bdb40) before dabingChanel (0x6bdb50), the reverse of PS2's
// definition order, where their buckets (tools/bsshash.py: 885 and 135) give the other order; as g_indicators.cpp's,
// what an object compiled with another file's precompiled header (/Yu) gives.

#include "ui/g_dabing.h"
#include "actors/C_human.h"
#include "mission/C_recorder.h"
#include "mission/Mission.h"
#include "ui/g_indicators.h"
#include "ui/g_loadsave.h"
#include "ui/g_textdb.h"

#include <stdio.h>
#include <string.h>

#include "I3D/I3D.h"
#include "rw_data.h"

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

// The face is the human's "base" where it is a single morph: reset, then given the weights of a closed mouth
// FUNCTION: GAME 0x5f4cd0
void DabingSetHuman(int chanel, C_human *human)
{
    if (chanel < 0 || chanel >= NUM_DABING_CHANELS)
        return;
    S_dabing_chanel *ch = &dabingChanel[chanel];
    if (!human)
        return;
    ch->speaker_frame = human->frame;
    ch->target_e = human->target_e;
    ch->target_n = human->target_n;
    I3D_frame *base = human->base_frame;
    if (base && base->GetType() == FRAME_VISUAL &&
        ((I3D_visual *)base)->GetVisualType() == I3D_VISUAL_SINGLEMORPH)
    {
        ch->face = (I3D_visual *)base;
        ch->face->SetWeight(0, 0, 1.0f);
        ch->face->SetWeight(0, 1, 0.0f);
        ch->face->SetWeight(0, 2, 0.0f);
        ch->face->SetWeight(0, 3, 0.0f);
        ch->face->SetWeight(0, 4, 0.0f);
        ch->face->SetWeight(1, 0, 1.0f);
        ch->face->SetWeight(1, 1, 0.0f);
        ch->face->SetWeight(1, 2, 0.0f);
        ch->face->SetWeight(1, 3, 0.0f);
        ch->face->SetWeight(1, 4, 0.0f);
        ch->face->SetWeight(2, 0, 1.0f);
        ch->face->SetWeight(2, 1, 0.0f);
        ch->face->SetWeight(2, 2, 0.0f);
        ch->face->SetWeight(2, 3, 0.0f);
        ch->face->SetWeight(2, 4, 0.0f);
        // in the order DabingTick reads a frame's bytes into them (PS2 the same)
        ch->weight[1] = 0.0f;
        ch->weight[0] = .46f;
        ch->weight[2] = .53f;
        ch->weight[4] = .11f;
        ch->weight[3] = 1.0f;
        ch->weight[5] = .6f;
        ch->face->SetWeight(0, 0, 0.0f);
        ch->face->SetWeight(0, 1, ch->weight[1] * ch->weight[0]);
        ch->face->SetWeight(0, 2, (1.0f - ch->weight[0]) * ch->weight[1]);
        ch->face->SetWeight(0, 3, (1.0f - ch->weight[2]) * (1.0f - ch->weight[1]));
        ch->face->SetWeight(0, 4, (1.0f - ch->weight[1]) * ch->weight[2]);
        ch->face->SetWeight(1, 0, 0.0f);
        ch->face->SetWeight(1, 1, ch->weight[4] * ch->weight[3]);
        ch->face->SetWeight(1, 2, (1.0f - ch->weight[4]) * ch->weight[3]);
        ch->face->SetWeight(1, 3, 1.0f - ch->weight[3]);
        ch->face->SetWeight(2, 0, 0.0f);
        ch->face->SetWeight(2, 1, ch->weight[5] * ch->weight[3]);
        ch->face->SetWeight(2, 2, (1.0f - ch->weight[5]) * ch->weight[3]);
        ch->face->SetWeight(2, 3, 1.0f - ch->weight[3]);
    }
    else
        ch->face = NULL;
    ch->state = DABING_STATE_READY;
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

// FUNCTION: GAME 0x5f5210
void DabingSetModel(int chanel, I3D_model *model)
{
    if (chanel < 0 || chanel >= NUM_DABING_CHANELS)
        return;
    S_dabing_chanel *ch = &dabingChanel[chanel];
    if (!model)
        return;
    ch->speaker_frame = model;
    I3D_frame *base = model->FindChildFrame("base", ENUMF_ALL);
    if (base && base->GetType() == FRAME_VISUAL &&
        ((I3D_visual *)base)->GetVisualType() == I3D_VISUAL_SINGLEMORPH)
    {
        ch->face = (I3D_visual *)base;
        ch->target_e = NULL;
        ch->target_n = NULL;
        ch->face->SetWeight(0, 0, 1.0f);
        ch->face->SetWeight(0, 1, 0.0f);
        ch->face->SetWeight(0, 2, 0.0f);
        ch->face->SetWeight(0, 3, 0.0f);
        ch->face->SetWeight(0, 4, 0.0f);
        ch->face->SetWeight(1, 0, 1.0f);
        ch->face->SetWeight(1, 1, 0.0f);
        ch->face->SetWeight(1, 2, 0.0f);
        ch->face->SetWeight(1, 3, 0.0f);
        ch->face->SetWeight(1, 4, 0.0f);
        ch->face->SetWeight(2, 0, 1.0f);
        ch->face->SetWeight(2, 1, 0.0f);
        ch->face->SetWeight(2, 2, 0.0f);
        ch->face->SetWeight(2, 3, 0.0f);
        ch->face->SetWeight(2, 4, 0.0f);
    }
    else
        ch->face = NULL;
    ch->state = DABING_STATE_READY;
}

// FUNCTION: GAME 0x5f53d0
void DabingInvalidate(C_human *human)
{
    if (!human->frame)
        return;
    for (int i = 0; i < NUM_DABING_CHANELS; i++)
    {
        if (dabingChanel[i].speaker_frame != human->frame)
            continue;
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
        memset(&dabingChanel[i], 0, sizeof(dabingChanel[i]));
    }
}

// The speech's data (tables\dat\<speech>.dat) and sound (sounds\<speech>.wav, while the speech volume isn't all but
// off), its sound placed at the face's "neck" or above the speaker, its type and range by kind; a speech without
// either shows its subtitle only. The subtitle shows where the speaker is near enough the camera (by kind; always for
// kind 3, the cutscenes'), for as long as the sound plays and half a second.
// FUNCTION: GAME 0x5f5570
void DabingSpeak(int chanel, uint speech, bool subtitle, uint kind)
{
    char name[256];
    if (chanel < 0 || chanel >= NUM_DABING_CHANELS)
        return;
    S_dabing_chanel *ch = &dabingChanel[chanel];
    // the square of the subtitle's distance
    float range = 144.0f;
    if (ch->data)
        delete ch->data;
    ch->data = NULL;
    // STRING: GAME 0x638ba0
    sprintf(name, "tables\\dat\\%08d.dat", speech);
    int file = dtaOpen(name, 0);
    if (file >= 0)
    {
        uint size = dtaSeek(file, 0, SEEK_END);
        if (size)
        {
            dtaSeek(file, 0, SEEK_SET);
            ch->data = (uint *)new char[size];
            *ch->data = 0;
            dtaRead(file, ch->data, size);
        }
        dtaClose(file);
    }
    if (mafiaLoadSave.settings.speech_volume > .01f)
    {
        if (!ch->sound)
            ch->sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
        // STRING: GAME 0x638b90
        sprintf(name, "sounds\\%08d.wav", speech);
        LS3D_RESULT result;
        if (kind == 4)
            result = ch->sound->Open(name, 0x40, NULL, NULL);
        else
            result = ch->sound->Open(name, 0, NULL, NULL);
        if (result != I3D_OK)
        {
            ch->sound->Release();
            ch->sound = NULL;
        }
    }
    else if (ch->sound)
    {
        ch->sound->Release();
        ch->sound = NULL;
    }
    ch->state = DABING_STATE_SPEAKING;
    if (ch->sound)
    {
        ch->sound->priority = 0x80;
        // the speech's volume against the sounds' (the sounds' is the sound driver's, so this sets it back)
        float volume =
            mafiaLoadSave.settings.speech_volume / (mafiaLoadSave.settings.sound_volume + .01f) * .95f + .05f;
        I3D_frame *neck;
        if (ch->face && (neck = ch->face->FindChildFrame("neck", ENUMF_ALL)))
        {
            ch->sound->LinkTo(neck, 0);
            ch->sound->SetPos(0.0f, 0.0f, .1f);
        }
        else
        {
            ch->sound->LinkTo(ch->speaker_frame, 0);
            ch->sound->SetPos(0.0f, 1.6f, 0.0f);
        }
        ch->sound->SetDir(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
        switch (kind)
        {
        case 1:
            ch->sound->SetSoundType(I3DSOUND_POINT);
            ch->sound->SetRange(1.0f, 80.0f, .3f, .25f);
            ch->sound->SetOutVol(volume);
            ch->sound->SetVolume(volume);
            range = 625.0f;
            break;
        case 2:
            ch->sound->SetSoundType(I3DSOUND_AMBIENT);
            ch->sound->SetOutVol(volume);
            ch->sound->SetVolume(volume);
            range = 6.4e9f;
            break;
        case 3:
            ch->sound->SetSoundType(I3DSOUND_AMBIENT);
            ch->sound->SetOutVol(volume);
            ch->sound->SetVolume(volume);
            break;
        case 4:
            ch->sound->SetSoundType(I3DSOUND_AMBIENT);
            ch->sound->SetOutVol(volume);
            ch->sound->SetVolume(volume);
            break;
        case 6:
            ch->sound->SetSoundType(I3DSOUND_POINT);
            ch->sound->SetRange(1.0f, 80.0f, .3f, .25f);
            ch->sound->SetOutVol(volume);
            ch->sound->SetVolume(volume);
            range = 6400.0f;
            break;
        default:
            ch->sound->SetSoundType(I3DSOUND_SPOT);
            ch->sound->SetRange(2.5f, 30.0f, .35f, .2f);
            ch->sound->SetCone(140.0f * PI / 180.0f, PI);
            ch->sound->SetOutVol(volume * .85f);
            ch->sound->SetVolume(volume);
            break;
        }
        ch->sound->SetOn(false);
        ch->sound->SetCurrTime(0);
        ch->sound->Update();
        ch->sound->SetOn(true, true);
    }
    else if (kind == 5)
    {
        // STRING: GAME 0x638b84
        ch->sound = (I3D_sound *)act_mission->scene->FindFrame("14_Coucou", ENUMF_SOUND);
        if (ch->sound)
        {
            ch->sound->priority = 0x80;
            if (!ch->sound->IsPlaying(true))
            {
                ch->sound->SetOn(false);
                ch->sound->SetOn(true);
            }
        }
        ch->state = DABING_STATE_SCENE_SOUND;
    }
    ch->time = 0;
    ch->fade_time = DABING_FADE_TIME;
    if (subtitle && !ch->sound && !ch->data)
    {
        ch->fade_time = strlen(textDatabase.GetText(speech)) * 30 + 1000;
        ch->state = DABING_STATE_SUBTITLE;
    }
    ch->blend = 0.0f;
    if (!subtitle)
        return;
    S_vector dir;
    if (kind != 4 && ch->speaker_frame && act_mission->scene->GetActiveCamera())
        dir = ch->speaker_frame->GetWorldPos() - act_mission->scene->GetActiveCamera()->GetWorldPos();
    else
        dir = S_vector(0.0f, 1.0f, 0.0f);
    if (dir.Square() < range || kind == 3)
    {
        if (ch->sound)
            mafiaIndicators.SubtitlesAdd(speech, ch->sound->GetPlayTime() + 500, true);
        else
            mafiaIndicators.SubtitlesAdd(speech, 0, true);
    }
}

// FUNCTION: GAME 0x5f5b20
void DabingLinkTargets(int chanel, I3D_frame *frame, uint which)
{
    if (chanel < 0 || chanel >= NUM_DABING_CHANELS)
        return;
    S_dabing_chanel *ch = &dabingChanel[chanel];
    if (!ch->speaker_frame)
        return;
    if (ch->target_e && which & DABING_TARGET_E)
    {
        if (frame)
        {
            ch->target_e->SetOn(true);
            ch->target_e->LinkTo(frame, 0);
            ch->target_e->SetPos(0.0f, 0.0f, 0.0f);
        }
        else
            ch->target_e->SetOn(false);
        ch->target_e->Update();
    }
    if (ch->target_n && which & DABING_TARGET_N)
    {
        if (frame)
        {
            ch->target_n->SetOn(true);
            ch->target_n->LinkTo(frame, 0);
            ch->target_n->SetPos(0.0f, 0.0f, 0.0f);
        }
        else
            ch->target_n->SetOn(false);
        ch->target_n->Update();
    }
}

// Speaking while the sound plays short of its end (where DabingTick ends the speech), or in DABING_STATE_SPEAKING or
// DABING_STATE_SUBTITLE (PS2's also in DABING_STATE_FADING, and without the sound's test)
// FUNCTION: GAME 0x5f5c40
bool DabingIsSpeaking(int chanel)
{
    if (chanel < 0 || chanel >= NUM_DABING_CHANELS)
        return false;
    S_dabing_chanel *ch = &dabingChanel[chanel];
    if (ch->sound && ch->sound->IsPlaying(false) &&
        ch->sound->GetCurrTime() > ch->time &&
        ch->time + 300 > ch->sound->GetPlayTime())
        return true;
    if (ch->state == DABING_STATE_SPEAKING)
        return true;
    return ch->state == DABING_STATE_SUBTITLE ? true : false;
}

// FUNCTION: GAME 0x5f5cc0
void DabingStop(int chanel)
{
    if (chanel < 0 || chanel >= NUM_DABING_CHANELS)
        return;
    S_dabing_chanel *ch = &dabingChanel[chanel];
    if (ch->state == DABING_STATE_SCENE_SOUND)
        return;
    if (ch->sound)
        ch->sound->SetOn(false, true);
    ch->fade_time = 1;
    ch->state = DABING_STATE_FADING;
}

// Each target set to its own position, which marks it to update
// FUNCTION: GAME 0x5f5d10
void DabingUpdateFrames()
{
    for (int i = 0; i < NUM_DABING_CHANELS; i++)
    {
        if (dabingChanel[i].target_n && dabingChanel[i].target_n->IsOn())
        {
            dabingChanel[i].target_n->SetPos(dabingChanel[i].target_n->GetPos());
            dabingChanel[i].target_n->Update();
        }
        if (dabingChanel[i].target_e && dabingChanel[i].target_e->IsOn())
        {
            dabingChanel[i].target_e->SetPos(dabingChanel[i].target_e->GetPos());
            dabingChanel[i].target_e->Update();
        }
    }
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

// FUNCTION: GAME 0x5f65e0
void DabingPauseSounds(bool pause, bool unknown)
{
    for (int i = 0; i < NUM_DABING_CHANELS; i++)
    {
        if (pause)
        {
            if (dabingChanel[i].sound && dabingChanel[i].sound->IsPlaying(false) &&
                dabingChanel[i].state != DABING_STATE_SCENE_SOUND)
            {
                dabingChanel[i].sound->SetOn(false, true);
                dabingChanel[i].sound->Update();
            }
        }
        else if (dabingChanel[i].state == DABING_STATE_SPEAKING && dabingChanel[i].sound)
        {
            dabingChanel[i].sound->SetOn(true);
            dabingChanel[i].sound->SetOn(true, true);
            dabingChanel[i].sound->SetCurrTime(dabingChanel[i].time);
            dabingChanel[i].sound->Update();
        }
    }
}

// FUNCTION: GAME 0x5f6670
void DabingCutscene(const S_cutscene_dabing *item)
{
    switch (item->speech)
    {
    case DABING_CUTSCENE_NONE:
        break;
    case DABING_CUTSCENE_SPEAKER:
    {
        I3D_frame *frm = act_mission->scene->FindFrame(item->frame_name, ENUMF_MODEL);
        if (!frm)
            break;
        C_actor **actor = GetFrameActor(frm);
        if (!actor)
            break;
        switch ((*actor)->type)
        {
        case ACTOR_PLAYER:
        case ACTOR_HUMAN:
        case ACTOR_UNKNOWN_7:
        case ACTOR_ENTITY:
            DabingSetHuman(item->chanel, (C_human *)*actor);
            break;
        case ACTOR_ACTOR:
            if ((*actor)->frame->GetType() == FRAME_MODEL)
                DabingSetModel(item->chanel, (I3D_model *)(*actor)->frame);
            break;
        }
        break;
    }
    case DABING_CUTSCENE_TARGETS:
    case DABING_CUTSCENE_TARGET_N:
    case DABING_CUTSCENE_TARGET_E:
        if (item->frame_name[0])
            DabingLinkTargets(item->chanel, act_mission->scene->FindFrame(item->frame_name, ENUMF_ALL), item->speech);
        else
            DabingLinkTargets(item->chanel, NULL, item->speech);
        break;
    default:
        DabingSpeak(item->chanel, item->speech, true, 3);
        break;
    }
}

// The morph starts at the item's time, time being the cutscene's now
// FUNCTION: GAME 0x5f6760
void DabingCutsceneMorph(const S_cutscene_morph *item, uint time)
{
    I3D_frame *frm = act_mission->scene->FindFrame(item->frame_name, ENUMF_VISUAL);
    if (!frm || ((I3D_visual *)frm)->GetVisualType() != I3D_VISUAL_MORPH)
        return;
    G_DabingMorphObj obj;
    obj.visual = (I3D_visual *)frm;
    obj.duration = item->duration;
    obj.time = time - item->time;
    obj.level[0][0] = item->level[0][0];
    obj.level[0][1] = item->level[0][1];
    obj.level[1][0] = item->level[1][0];
    obj.level[1][1] = item->level[1][1];
    G_DabingMorphActive.push_back(obj);
}
