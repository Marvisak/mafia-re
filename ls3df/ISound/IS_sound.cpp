// Sounds, at 0x1007c2a0-0x1007d42e in the original (PS2 IS_sound.cpp, same order). A sound plays a duplicate of its
// source's buffer. Unless the driver or the sound opts out, the sounds playing in hardware are kept in ChannelList, at
// most MaxHWChannels of them. A sound that finds no free channel takes the one of the lowest priority, and the sound
// it stops waits in TerminatedChannelList until UpdateAllTerminatedBuffs gives it a channel again. After the last
// function comes the header's zero vector (see IS_driver.cpp).
#include "isound.h"

// PS2's __sinit_IS_sound.cpp constructs the lists in this order

// GLOBAL: LS3DF 0x101c6300
C_list<CHANNEL_STRUCT> ChannelList(64);

// GLOBAL: LS3DF 0x101c62e4
C_list<TERMINATED_CHANNEL_STRUCT> TerminatedChannelList(512);

// FUNCTION: LS3DF 0x1007c340
void UpdateAllTerminatedBuffs()
{
    if (!MaxHWChannels || !(pISndDriver->init_flags & ISNDINIT_EAX))
        return;
    uint time = timeGetTime();

    // Free the channels of the sounds that have ended
    int i = ChannelList.size();
    while (i--) {
        if (ChannelList[i].loop)
            continue;
        DWORD status;
        ChannelList[i].buffer->GetStatus(&status);
        if (status & DSBSTATUS_PLAYING) {
            if (time - ChannelList[i].start_time < ChannelList[i].sound->GetSource()->GetPlayTime())
                continue;
            ChannelList[i].buffer->Stop();
        }
        ChannelList.erase(i);
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
ISND_sound::ISND_sound() : buffer(NULL), source(NULL)
{
    ref = 1;
    frequency = 1.0f;
    volume = 1.0f;
}

// FUNCTION: LS3DF 0x1007c960
ISND_sound::~ISND_sound()
{
    Close();
}

// FUNCTION: LS3DF 0x1007c9a0
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
LS3D_RESULT ISND_sound::Open(ISND_source *source, uint flags)
{
    Close();
    if (!source->buffer)
        return I3DERR_GENERIC;
    HRESULT hr = pISndDriver->lpDS8->DuplicateSoundBuffer(source->buffer, &buffer);
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
LS3D_RESULT ISND_sound::Close()
{
    Stop();
    if (buffer) {
        buffer->Release();
        buffer = NULL;
    }
    if (source) {
        source->Release();
        source = NULL;
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007ca70
LS3D_RESULT ISND_sound::Play(ISND_source *source, uint flags)
{
    LS3D_RESULT result = Open(source, flags);
    if (result == I3D_OK)
        result = Play(flags, 0);
    return result;
}

// FUNCTION: LS3DF 0x1007caa0
LS3D_RESULT ISND_sound::Play(uint flags, uint priority)
{
    if (!buffer)
        return I3DERR_GENERIC;
    if (!(flags & ISND_NOREWIND))
        buffer->SetCurrentPosition(0);
    DWORD play_flags, play_priority;
    if (!(pISndDriver->init_flags & ISNDINIT_NOHWCHANNELS) && !(this->flags & ISND_NOHWCHANNEL)) {
        if (pISndDriver->init_flags & ISNDINIT_EAX) {
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
                    t.buffer = buffer;
                    t.sound = this;
                    t.priority = priority;
                    buffer->GetCurrentPosition(&t.position, NULL);
                    t.stop_time = timeGetTime();
                    return I3D_OK;
                }
                TERMINATED_CHANNEL_STRUCT &t = TerminatedChannelList.push_back();
                t.loop = best->loop;
                t.buffer = best->buffer;
                t.sound = best->sound;
                t.priority = best->priority;
                // The new sound's position, not the stopped one's
                buffer->GetCurrentPosition(&t.position, NULL);
                t.stop_time = timeGetTime();
                best->buffer->Stop();
                best->sound = this;
                best->buffer = buffer;
                best->priority = priority;
                best->loop = flags & ISND_LOOP;
                best->start_time = t.stop_time;
            } else {
                CHANNEL_STRUCT &c = ChannelList.push_back();
                c.sound = this;
                c.buffer = buffer;
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
        if (FAILED(buffer->Play(0, play_priority, play_flags | DSBPLAY_LOOPING))
            && FAILED(buffer->Play(0, 0, DSBPLAY_LOOPING | DSBPLAY_LOCSOFTWARE)))
            return I3DERR_GENERIC;
    } else {
        if (FAILED(buffer->Play(0, play_priority, play_flags)) && FAILED(buffer->Play(0, 0, DSBPLAY_LOCSOFTWARE)))
            return I3DERR_GENERIC;
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007cf50
LS3D_RESULT ISND_sound::Stop()
{
    if (!buffer)
        return I3DERR_GENERIC;
    buffer->Stop();
    if (MaxHWChannels > 0 && (pISndDriver->init_flags & ISNDINIT_EAX)
        && !(pISndDriver->init_flags & ISNDINIT_NOHWCHANNELS) && !(flags & ISND_NOHWCHANNEL)) {
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
uint ISND_sound::GetCurrTime() const
{
    if (!buffer)
        return 0;
    DWORD position;
    buffer->GetCurrentPosition(&position, NULL);
    return (uint)(position / source->bytes_per_ms);
}

// FUNCTION: LS3DF 0x1007d090
LS3D_RESULT ISND_sound::SetCurrTime(uint time)
{
    if (!buffer)
        return I3DERR_GENERIC;
    // The start of the block before
    int position = ((int)(time * source->bytes_per_ms) / source->wave_format.nBlockAlign - 1)
        * source->wave_format.nBlockAlign;
    if (position < 0)
        position = 0;
    buffer->SetCurrentPosition(position);
    if (MaxHWChannels > 0 && (pISndDriver->init_flags & ISNDINIT_EAX)
        && !(pISndDriver->init_flags & ISNDINIT_NOHWCHANNELS) && !(flags & ISND_NOHWCHANNEL)) {
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
LS3D_RESULT ISND_sound::SetVolume(float volume)
{
    if ((flags & ISND_NOVOLUME) || !buffer)
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
    buffer->SetVolume(db);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007d1f0
LS3D_RESULT ISND_sound::SetVolumeLOG(float volume)
{
    this->volume = volume;
    if ((flags & ISND_NOVOLUME) || !buffer)
        return I3DERR_GENERIC;
    buffer->SetVolume((LONG)((1.0f - volume * pISndDriver->volume) * -10000.0f));
    return I3D_OK;
}

// In hundredths of a decibel
// FUNCTION: LS3DF 0x1007d240
LS3D_RESULT ISND_sound::SetVolumeLOG(int volume)
{
    if ((flags & ISND_NOVOLUME) || !buffer)
        return I3DERR_GENERIC;
    // The driver's attenuation. Without the cast to float, VC6 folds the two minus signs into an add.
    float attenuation = (float)(log(pISndDriver->volume) * -866.0);
    volume -= (int)attenuation;
    if (volume < DSBVOLUME_MIN)
        volume = DSBVOLUME_MIN;
    else if (volume > DSBVOLUME_MAX)
        volume = DSBVOLUME_MAX;
    buffer->SetVolume(volume);
    return I3D_OK;
}

// A factor of the source's sample rate
// FUNCTION: LS3DF 0x1007d2b0
LS3D_RESULT ISND_sound::SetFrequency(float frequency)
{
    if (!(flags & ISND_FREQUENCY) || !buffer)
        return I3DERR_GENERIC;
    uint freq = (uint)(frequency * source->format.samples_per_sec);
    if (freq > pISndDriver->max_freq)
        freq = pISndDriver->max_freq;
    if (freq < pISndDriver->min_freq)
        freq = pISndDriver->min_freq;
    this->frequency = (float)freq / source->format.samples_per_sec;
    buffer->SetFrequency(freq);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007d340
LS3D_RESULT ISND_sound::SetPanning(float panning)
{
    if ((flags & ISND_NOPANNING) || !buffer)
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
    buffer->SetPan(pan);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007d3e0
bool ISND_sound::IsPlaying() const
{
    if (!buffer)
        return false;
    DWORD status;
    buffer->GetStatus(&status);
    return status & DSBSTATUS_PLAYING ? true : false;
}
