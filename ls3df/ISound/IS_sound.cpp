// Sounds, at 0x1007c2a0-0x1007d42e in the original (PS2 IS_sound.cpp, same order). A sound plays a duplicate of its
// source's buffer. Unless the driver or the sound opts out, the sounds playing in hardware are kept in ChannelList, at
// most MaxHWChannels of them. A sound that finds no free channel takes the one of the lowest priority, and the sound
// it stops waits in TerminatedChannelList until UpdateAllTerminatedBuffs gives it a channel again. After the last
// function comes the header's zero vector (see IS_driver.cpp).
#include "ISound/ISound.h"

// PS2's __sinit_IS_sound.cpp constructs the lists in this order

// GLOBAL: LS3DF 0x101c6300
// GLOBAL: LS3DF11 0x101c3400
C_list<CHANNEL_STRUCT> ChannelList(64);

// GLOBAL: LS3DF 0x101c62e4
// GLOBAL: LS3DF11 0x101c33e4
C_list<TERMINATED_CHANNEL_STRUCT> TerminatedChannelList(512);

// Open: the original spills the frequency factor of both divisions with fld/fstp, in a slot of its own; ours copies it
// with integer moves, into the status's slot (a frame 4 bytes smaller). GetFrequency with a local, a reference or no
// const, and inline helpers taking the factor as a parameter don't give it.
// FUNCTION: LS3DF 0x1007c340
// FUNCTION: LS3DF11 0x10079170
void UpdateAllTerminatedBuffs()
{
    if (!MaxHWChannels || !(pISndDriver->create_flags & ISNDINIT_EAX))
        return;
    uint time = timeGetTime();

    // Free the channels of the sounds that have ended. An erase in each branch, which VC6 merges: one erase after the
    // playing branch's Stop (a continue for a sound still playing) gives the same instructions with time in ebx and i
    // in a stack slot, the original's the other way round.
    int i = ChannelList.size();
    while (i--) {
        if (ChannelList[i].loop)
            continue;
        DWORD status;
        ChannelList[i].buffer->GetStatus(&status);
        if (!(status & DSBSTATUS_PLAYING))
            ChannelList.erase(i);
        else if (time - ChannelList[i].start_time >= ChannelList[i].sound->GetSource()->GetPlayTime()) {
            ChannelList[i].buffer->Stop();
            ChannelList.erase(i);
        }
    }
    if (!TerminatedChannelList.size())
        return;

    // Forget the terminated sounds that would have ended by now (all but the last)
    i = TerminatedChannelList.size() - 1;
    while (i--) {
        if (TerminatedChannelList[i].loop)
            continue;
        if (TerminatedChannelList[i].position - TerminatedChannelList[i].stop_time + time
            >= TerminatedChannelList[i].sound->GetSource()->GetPlayTime() / TerminatedChannelList[i].sound->GetFrequency())
            TerminatedChannelList.erase(i);
    }

    // Give a free channel to the one of the highest priority, the loudest of equal ones
    if (MaxHWChannels > ChannelList.size()) {
        int best = TerminatedChannelList.size() - 1;
        i = TerminatedChannelList.size() - 1;
        while (i--) {
            if (TerminatedChannelList[i].priority > TerminatedChannelList[best].priority
                || TerminatedChannelList[i].priority == TerminatedChannelList[best].priority
                    && TerminatedChannelList[i].sound->GetVolume() > TerminatedChannelList[best].sound->GetVolume())
                best = i;
        }
        CHANNEL_STRUCT &c = ChannelList.push_back();
        c.sound = TerminatedChannelList[best].sound;
        c.buffer = TerminatedChannelList[best].buffer;
        c.priority = TerminatedChannelList[best].priority;
        c.loop = TerminatedChannelList[best].loop;
        c.start_time = time;
        // It resumes where it would be now
        if (TerminatedChannelList[best].loop) {
            uint position = (TerminatedChannelList[best].position - TerminatedChannelList[best].stop_time + time)
                % (uint)(TerminatedChannelList[best].sound->GetSource()->GetPlayTime()
                    / TerminatedChannelList[best].sound->GetFrequency());
            TerminatedChannelList[best].sound->SetCurrTime(position);
            c.start_time -= position;
            TerminatedChannelList[best].buffer->Play(0, 0, DSBPLAY_LOOPING);
        } else {
            uint position = TerminatedChannelList[best].position - TerminatedChannelList[best].stop_time + time;
            TerminatedChannelList[best].sound->SetCurrTime(position);
            c.start_time -= position;
            TerminatedChannelList[best].buffer->Play(0, 0, 0);
        }
        TerminatedChannelList.erase(best);
    }
}

// FUNCTION: LS3DF 0x1007c6d0
// FUNCTION: LS3DF11 0x10079500
void TraceDSErrors(HRESULT hr)
{
    switch (hr) {
    case DSERR_ACCESSDENIED:
        dbgPrintf("TraceDSErrors() - DSERR_ACCESSDENIED");
        break;
    case DSERR_ALLOCATED:
        dbgPrintf("TraceDSErrors() - DSERR_ALLOCATED");
        break;
    case DSERR_ALREADYINITIALIZED:
        dbgPrintf("TraceDSErrors() - DSERR_ALREADYINITIALIZED");
        break;
    case DSERR_BADFORMAT:
        dbgPrintf("TraceDSErrors() - DSERR_BADFORMAT");
        break;
    case DSERR_BUFFERLOST:
        dbgPrintf("TraceDSErrors() - DSERR_BUFFERLOST");
        break;
    case DSERR_CONTROLUNAVAIL:
        dbgPrintf("TraceDSErrors() - DSERR_CONTROLUNAVAIL");
        break;
    case DSERR_GENERIC:
        dbgPrintf("TraceDSErrors() - DSERR_GENERIC");
        break;
    case DSERR_INVALIDCALL:
        dbgPrintf("TraceDSErrors() - DSERR_INVALIDCALL");
        break;
    case DSERR_INVALIDPARAM:
        dbgPrintf("TraceDSErrors() - DSERR_INVALIDPARAM");
        break;
    case DSERR_NOAGGREGATION:
        dbgPrintf("TraceDSErrors() - DSERR_NOAGGREGATION");
        break;
    case DSERR_NODRIVER:
        dbgPrintf("TraceDSErrors() - DSERR_NODRIVER");
        break;
    case DSERR_NOINTERFACE:
        dbgPrintf("TraceDSErrors() - DSERR_NOINTERFACE");
        break;
    case DSERR_OTHERAPPHASPRIO:
        dbgPrintf("TraceDSErrors() - DSERR_OTHERAPPHASPRIO");
        break;
    case DSERR_OUTOFMEMORY:
        dbgPrintf("TraceDSErrors() - DSERR_OUTOFMEMORY");
        break;
    case DSERR_PRIOLEVELNEEDED:         // the message has its own spelling
        dbgPrintf("TraceDSErrors() - DSERR_PRIORLEVELNEEDED");
        break;
    case DSERR_UNINITIALIZED:
        dbgPrintf("TraceDSErrors() - DSERR_UNINITIALIZED");
        break;
    case DSERR_UNSUPPORTED:
        dbgPrintf("TraceDSErrors() - DSERR_UNSUPPORTED");
        break;
    }
}

// FUNCTION: LS3DF 0x1007c8e0
// FUNCTION: LS3DF11 0x10079710
ISND_sound::ISND_sound() : lpDSB8(NULL), source(NULL)
{
    ref = 1;
    freq_mult = 1.0f;
    volume = 1.0f;
}

// FUNCTION: LS3DF 0x1007c960
// FUNCTION: LS3DF11 0x10079790
ISND_sound::~ISND_sound()
{
    Close();
}

// FUNCTION: LS3DF 0x1007c9a0
// FUNCTION: LS3DF11 0x100797d0
int ISND_sound::Release()
{
    if (--ref < 0) {
        dbgPrintf("!! RefCount<0  this:0x%x  ISND_sound", this);
        return 0;
    }
    if (!ref) {
        delete this;
        return 0;
    }
    return ref;
}

// FUNCTION: LS3DF 0x1007c9d0
// FUNCTION: LS3DF11 0x10079800
LS3D_RESULT ISND_sound::Open(ISND_source *source, uint flags)
{
    Close();
    if (!source->lpDSB8)
        return I3DERR_GENERIC;
    HRESULT hr = pISndDriver->lpDS8->DuplicateSoundBuffer(source->lpDSB8, &lpDSB8);
    if (FAILED(hr)) {
        TraceDSErrors(hr);
        return I3DERR_GENERIC;
    }
    this->source = source;
    source->AddRef();
    this->flags = flags;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007ca30
// FUNCTION: LS3DF11 0x10079860
LS3D_RESULT ISND_sound::Close()
{
    Stop();
    if (lpDSB8) {
        lpDSB8->Release();
        lpDSB8 = NULL;
    }
    if (source) {
        source->Release();
        source = NULL;
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007ca70
// FUNCTION: LS3DF11 0x100798a0
LS3D_RESULT ISND_sound::Play(ISND_source *source, uint flags)
{
    LS3D_RESULT result = Open(source, flags);
    if (result == I3D_OK)
        result = Play(flags, 0);
    return result;
}

// FUNCTION: LS3DF 0x1007caa0
// FUNCTION: LS3DF11 0x100798d0
LS3D_RESULT ISND_sound::Play(uint flags, uint priority)
{
    if (!lpDSB8)
        return I3DERR_GENERIC;
    if (!(flags & ISND_NOREWIND))
        lpDSB8->SetCurrentPosition(0);
    DWORD play_flags, play_priority;
    if (!(pISndDriver->create_flags & ISNDINIT_NOHWCHANNELS) && !(this->flags & ISND_SYSMEM)) {
        if (pISndDriver->create_flags & ISNDINIT_EAX) {
            play_flags = 0;
            play_priority = 0;
        } else {
            play_flags = DSBPLAY_TERMINATEBY_TIME | DSBPLAY_TERMINATEBY_PRIORITY;
            play_priority = priority;
        }
        if (MaxHWChannels > 0) {
            // Playing already
            int i = ChannelList.size();
            while (i--) {
                if (ChannelList[i].sound == this) {
                    ChannelList[i].priority = priority;
                    ChannelList[i].loop = flags & ISND_LOOP;
                    ChannelList[i].start_time = timeGetTime();
                    goto play;
                }
            }
            i = TerminatedChannelList.size();
            while (i--) {
                if (TerminatedChannelList[i].sound == this) {
                    TerminatedChannelList.erase(i);
                    break;
                }
            }
            if (MaxHWChannels <= ChannelList.size()) {
                // Take a stopped channel, or the one of the lowest priority, the loudest of equal ones
                CHANNEL_STRUCT *best = NULL;
                uint lowest = priority;
                i = ChannelList.size();
                while (i--) {
                    DWORD status;
                    ChannelList[i].buffer->GetStatus(&status);
                    if (!(status & DSBSTATUS_PLAYING)) {
                        best = &ChannelList[i];
                        break;
                    }
                    if (ChannelList[i].priority < lowest
                        || ChannelList[i].priority == lowest && best
                            && ChannelList[i].sound->GetVolume() > best->sound->GetVolume()) {
                        lowest = ChannelList[i].priority;
                        best = &ChannelList[i];
                    }
                }
                if (!best) {
                    // No channel: the sound waits for one
                    TERMINATED_CHANNEL_STRUCT &t = TerminatedChannelList.push_back();
                    t.loop = flags & ISND_LOOP;
                    t.buffer = lpDSB8;
                    t.sound = this;
                    t.priority = priority;
                    lpDSB8->GetCurrentPosition(&t.position, NULL);
                    t.stop_time = timeGetTime();
                    return I3D_OK;
                }
                TERMINATED_CHANNEL_STRUCT &t = TerminatedChannelList.push_back();
                t.loop = best->loop;
                t.buffer = best->buffer;
                t.sound = best->sound;
                t.priority = best->priority;
                // The new sound's position, not the stopped one's
                lpDSB8->GetCurrentPosition(&t.position, NULL);
                t.stop_time = timeGetTime();
                best->buffer->Stop();
                best->sound = this;
                best->buffer = lpDSB8;
                best->priority = priority;
                best->loop = flags & ISND_LOOP;
                best->start_time = t.stop_time;
            } else {
                CHANNEL_STRUCT &c = ChannelList.push_back();
                c.sound = this;
                c.buffer = lpDSB8;
                c.priority = priority;
                c.loop = flags & ISND_LOOP;
                c.start_time = timeGetTime();
            }
        }
    } else {
        play_flags = 0;
        play_priority = 0;
    }
play:
    // In software when the hardware can't
    if (flags & ISND_LOOP) {
        if (FAILED(lpDSB8->Play(0, play_priority, play_flags | DSBPLAY_LOOPING))
            && FAILED(lpDSB8->Play(0, 0, DSBPLAY_LOOPING | DSBPLAY_LOCSOFTWARE)))
            return I3DERR_GENERIC;
    } else {
        if (FAILED(lpDSB8->Play(0, play_priority, play_flags)) && FAILED(lpDSB8->Play(0, 0, DSBPLAY_LOCSOFTWARE)))
            return I3DERR_GENERIC;
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007cf50
// FUNCTION: LS3DF11 0x10079d80
LS3D_RESULT ISND_sound::Stop()
{
    if (!lpDSB8)
        return I3DERR_GENERIC;
    lpDSB8->Stop();
    if (MaxHWChannels > 0 && (pISndDriver->create_flags & ISNDINIT_EAX)
        && !(pISndDriver->create_flags & ISNDINIT_NOHWCHANNELS) && !(flags & ISND_SYSMEM)) {
        int i = ChannelList.size();
        while (i--) {
            if (ChannelList[i].sound == this) {
                ChannelList.erase(i);
                break;
            }
        }
        i = TerminatedChannelList.size();
        while (i--) {
            if (TerminatedChannelList[i].sound == this) {
                TerminatedChannelList.erase(i);
                break;
            }
        }
    }
    return I3D_OK;
}

// In milliseconds
// FUNCTION: LS3DF 0x1007d040
// FUNCTION: LS3DF11 0x10079e70
uint ISND_sound::GetCurrTime() const
{
    if (!lpDSB8)
        return 0;
    DWORD position;
    lpDSB8->GetCurrentPosition(&position, NULL);
    return (uint)(position / source->bytes_per_ms);
}

// FUNCTION: LS3DF 0x1007d090
// FUNCTION: LS3DF11 0x10079ec0
LS3D_RESULT ISND_sound::SetCurrTime(uint time)
{
    if (!lpDSB8)
        return I3DERR_GENERIC;
    // The start of the block before
    int position = ((int)(time * source->bytes_per_ms) / source->wave_format.nBlockAlign - 1)
        * source->wave_format.nBlockAlign;
    if (position < 0)
        position = 0;
    lpDSB8->SetCurrentPosition(position);
    if (MaxHWChannels > 0 && (pISndDriver->create_flags & ISNDINIT_EAX)
        && !(pISndDriver->create_flags & ISNDINIT_NOHWCHANNELS) && !(flags & ISND_SYSMEM)) {
        int i = ChannelList.size();
        while (i--) {
            if (ChannelList[i].sound == this) {
                ChannelList[i].start_time = timeGetTime() - time;
                break;
            }
        }
    }
    return I3D_OK;
}

// From linear to DirectSound's hundredths of a decibel, scaled by the driver's volume
// FUNCTION: LS3DF 0x1007d160
// FUNCTION: LS3DF11 0x10079f90
LS3D_RESULT ISND_sound::SetVolume(float volume)
{
    if ((flags & ISND_NOVOLUME) || !lpDSB8)
        return I3DERR_GENERIC;
    this->volume = volume;
    volume *= pISndDriver->volume;
    int db;
    if (volume < MRG_ZERO)
        db = DSBVOLUME_MIN;
    else if (volume >= 1.0f)
        db = DSBVOLUME_MAX;
    else {
        db = (int)(log(volume) * 866.0);
        if (db < DSBVOLUME_MIN)
            db = DSBVOLUME_MIN;
    }
    lpDSB8->SetVolume(db);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007d1f0
// FUNCTION: LS3DF11 0x1007a020
LS3D_RESULT ISND_sound::SetVolumeLOG(float volume)
{
    this->volume = volume;
    if ((flags & ISND_NOVOLUME) || !lpDSB8)
        return I3DERR_GENERIC;
    lpDSB8->SetVolume((LONG)((1.0f - volume * pISndDriver->volume) * -10000.0f));
    return I3D_OK;
}

// In hundredths of a decibel
// FUNCTION: LS3DF 0x1007d240
// FUNCTION: LS3DF11 0x1007a070
LS3D_RESULT ISND_sound::SetVolumeLOG(int volume)
{
    if ((flags & ISND_NOVOLUME) || !lpDSB8)
        return I3DERR_GENERIC;
    // The driver's attenuation. Without the cast to float, VC6 folds the two minus signs into an add.
    volume -= (int)(float)(log(pISndDriver->volume) * -866.0);
    if (volume < DSBVOLUME_MIN)
        volume = DSBVOLUME_MIN;
    else if (volume > DSBVOLUME_MAX)
        volume = DSBVOLUME_MAX;
    lpDSB8->SetVolume(volume);
    return I3D_OK;
}

// A factor of the source's sample rate
// FUNCTION: LS3DF 0x1007d2b0
// FUNCTION: LS3DF11 0x1007a0e0
LS3D_RESULT ISND_sound::SetFrequency(float frequency)
{
    if (!(flags & ISND_FREQUENCY) || !lpDSB8)
        return I3DERR_GENERIC;
    uint freq = (uint)(frequency * source->wf.samples_per_sec);
    if (freq > pISndDriver->max_freq)
        freq = pISndDriver->max_freq;
    if (freq < pISndDriver->min_freq)
        freq = pISndDriver->min_freq;
    freq_mult = (float)freq / source->wf.samples_per_sec;
    lpDSB8->SetFrequency(freq);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007d340
// FUNCTION: LS3DF11 0x1007a170
LS3D_RESULT ISND_sound::SetPanning(float panning)
{
    if ((flags & ISND_NOPANNING) || !lpDSB8)
        return I3DERR_GENERIC;
    int pan;
    if (IsAbsMrgZero(panning))
        pan = DSBPAN_CENTER;
    else {
        pan = (int)(log(panning) * 866.0);
        if (pan > DSBPAN_RIGHT)
            pan = DSBPAN_RIGHT;
        else if (pan < DSBPAN_LEFT)
            pan = DSBPAN_LEFT;
    }
    lpDSB8->SetPan(pan);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007d3e0
// FUNCTION: LS3DF11 0x1007a210
bool ISND_sound::IsPlaying() const
{
    if (!lpDSB8)
        return false;
    DWORD status;
    lpDSB8->GetStatus(&status);
    return status & DSBSTATUS_PLAYING ? true : false;
}
