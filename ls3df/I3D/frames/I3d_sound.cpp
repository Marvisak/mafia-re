// The sounds (PS2 I3d_sound.cpp), at 0x10057650-0x10059440 in the original: I3D_sound in PS2's order from its
// constructor, without the out-of-line m_Pause (inlined everywhere) and with LoadImage folded into I3D_area's. The
// header's zero vector has its $E after m_UpdateSound (0x10059230), before Duplicate and m_SetState. Names are the
// PS2 build's unless noted. Open: m_UpdateSound's fade-out (see there).
//
// The sound driver's header comes before the engine's: the file's last two functions are the out-of-line copies of
// ISND_source::GetFileName and I3D_sound::SetOn(bool), the deferred items of the precompiled header that never came
// due, in header order. IGraph.h, which nothing here needs, comes first: without its inline functions GetFileName's
// copy comes due after Duplicate.
#include "IGraph/IGraph.h"
#include "ISound/ISound.h"
#include "I3D/I3D.h"

// <new> declares operator delete throw(), so VC6 sets no EH state before the lists' destructors in the destructor
#include <new>

// render.cpp's
extern C_list<I3D_area *> CurrentActiveAreaList;

// The distance's volume: 1 up to the near range, falling to range_mid_vol at the middle and to 0 at the far range
// (inline: the point and spot sounds have a copy each, and PS2's has no function of its own; the name is a guess)
inline float I3D_sound::GetRangeVolume(float dist) const
{
    if (dist <= scaled_range_n)
        return 1.0f;
    if (dist <= scaled_range_mid)
        return range_mid_vol + (1.0 - range_mid_vol) * (1.0 - (dist - scaled_range_n) /
            (scaled_range_mid - scaled_range_n));
    return range_mid_vol * (1.0 - (dist - scaled_range_mid) / (scaled_range_f - scaled_range_mid));
}

// FUNCTION: LS3DF 0x10057650
I3D_sound::I3D_sound()
{
    type = FRAME_SOUND;
    snd_flags = SOUNDF_POS | SOUNDF_DIR | SOUNDF_RANGE | SOUNDF_VOLUME | SOUNDF_SCALE | SOUNDF_VELOCITY;
    m_SetState(SOUNDSTATE_STOP);
    unknown_14c = 0;
    pause_time = 0;
    start_time = 0;
    pause_pos = 0;
    voice = NULL;
    snd_src = NULL;
    loop = false;
    volume_out = 0.0f;
    volume = 1.0f;
    virtual_volume = 1.0f;
    frequency = 1.0f;
    open_flags = 0;
    range_n = 1.0f;
    scaled_range_n = 1.0f;
    range_f = 10.0f;
    scaled_range_f = 10.0f;
    scaled_range_f_2 = 100.0f;
    range_mid = 1.0f;
    scaled_range_mid = 0.0f;
    range_mid_vol = 0.0f;
    snd_type = I3DSOUND_NULL;
    cone_in = PI / 3;
    cone_out = PI / 2;
    velocity = S_vector(0.0f, 0.0f, 0.0f);
    cos_cone_in = (float)cos(cone_in * .5);
    cos_cone_out = (float)cos(cone_out * .5);
    fade_sector = NULL;
    fade_time = 0;
    fade = -1.0f;
    fade_length = 0;
    priority = 0;
}

// FUNCTION: LS3DF 0x100577e0
I3D_sound::~I3D_sound()
{
    SetOn(false);
    I3D_sector **sp = &sectors[0];
    int i;
    for (i = sectors.size(); i--; sp++)
        (*sp)->DeleteSound(this);
    I3D_area **ap = &areas[0];
    for (i = areas.size(); i--; ap++)
        (*ap)->DeleteSound(this);
    pI3DDriver->sounds.remove(this);
    if (snd_src)
        snd_src->Release();
    if (voice)
        voice->Release();
}

// Opens the file's source, which the sound driver's sources share, and a voice for it. A source without a 3D buffer
// (a stereo one) can only be ambient.
// FUNCTION: LS3DF 0x10057940
LS3D_RESULT I3D_sound::Open(const char *file_name, uint flags, I3D_LOAD_CB_PROC *cb, void *context)
{
    if (!pISndDriver->IsInit())
        return I3DERR_NOTINITIALIZED;
    if (!file_name)
        return I3DERR_NOFILE;
    flags |= ISND_3DBUFFER;
    if (voice) {
        if (!strcmp(file_name, snd_src->GetFileName()))
            return I3D_OK;
        if (snd_src)
            snd_src->Release();
        if (voice)
            voice->Release();
        voice = NULL;
        snd_src = NULL;
    }
    for (int i = pISndDriver->sources.size(); i--; ) {
        ISND_source *src = pISndDriver->sources[i];
        if (!stricmp(file_name, src->GetFileName())) {
            snd_src = src;
            snd_src->AddRef();
            break;
        }
    }
    if (!snd_src) {
        ISND_source *src = NULL;
        if (pISndDriver->CreateSource(&src) < 0)
            return I3DERR_GENERIC;
        if (flags & I3DLOAD_SOUND_FREQUENCY)
            flags |= ISND_FREQUENCY;
        if (src->Open(file_name, flags) < 0) {
            src->Release();
            return I3DERR_GENERIC;
        }
        pISndDriver->sources.push_back(src);
        snd_src = src;
    }
    open_flags = snd_src->open_flags;
    if (pISndDriver->Create3DSound(&voice) < 0)
        return I3DERR_GENERIC;
    if (voice->Open(snd_src, open_flags) < 0)
        return I3DERR_GENERIC;
    if (open_flags & ISND_3DBUFFER) {
        voice->SetDistances(1e9f, 1e9f);
        SetCone(PI / 3, PI / 2);
    } else if (snd_type && snd_type != I3DSOUND_AMBIENT) {
        S_wave_format wf;
        snd_src->GetFormat(&wf);
        if (wf.num_channels > 1)
            dbgPrintf("WARNING ! I3D_sound::Open() - STEREO sound[%s] can be ambient only. Forcing to ambient type!",
                snd_src->GetFileName());
        else
            dbgPrintf("WARNING ! I3D_sound::Open() - Sound[%s], which isn't 3D, can be ambient only. Forcing to "
                "ambient type!", snd_src ? snd_src->GetFileName() : "???");
        SetSoundType(I3DSOUND_AMBIENT);
    }
    if (frm_flags & FRMFLAGS_ON)
        SetOn(true);
    SetCurrTime(0);
    snd_flags = SOUNDF_POS | SOUNDF_DIR | SOUNDF_RANGE | SOUNDF_VOLUME | SOUNDF_OUTVOL | SOUNDF_FREQUENCY |
        SOUNDF_CONE | SOUNDF_SCALE | SOUNDF_TYPE | SOUNDF_VELOCITY;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10057c40
bool I3D_sound::IsPlaying(bool real) const
{
    if (!voice)
        return false;
    if (real)
        return voice->IsPlaying();
    if (state == SOUNDSTATE_STOP)
        return false;
    // The playing case ends in a return of its own: as an if/else the early returns join the shared compare's false
    if (state != SOUNDSTATE_PAUSE && state != SOUNDSTATE_RESUME) {
        if (!loop) {
            uint elapsed = timeGetTime() - start_time;
            if (pause_pos + elapsed >= (uint)(GetPlayTime() / frequency))
                return false;
        }
        return true;
    }
    if (!loop) {
        uint elapsed = timeGetTime() - pause_time;
        if (pause_pos + elapsed >= (uint)(GetPlayTime() / frequency))
            return false;
    }
    return true;
}

// FUNCTION: LS3DF 0x10057d30
void I3D_sound::SetSoundType(I3D_SOUNDTYPE type)
{
    if (snd_type == type)
        return;
    if (snd_src && !(open_flags & ISND_3DBUFFER) && type && type != I3DSOUND_AMBIENT) {
        S_wave_format wf;
        snd_src->GetFormat(&wf);
        if (wf.num_channels > 1)
            dbgPrintf("WARNING ! I3D_sound::SetSoundType() - STEREO sound[%s] can be ambient only. Forcing to "
                "ambient type!", snd_src->GetFileName());
        else
            dbgPrintf("WARNING ! I3D_sound::SetSoundType() - Sound[%s], which isn't 3D, can be ambient only. Forcing "
                "to ambient type!", snd_src ? snd_src->GetFileName() : "???");
        snd_type = I3DSOUND_AMBIENT;
    } else
        snd_type = type;
    snd_flags |= SOUNDF_TYPE;
}

// FUNCTION: LS3DF 0x10057dd0
void I3D_sound::SetRange(float near_range, float far_range, float mid, float mid_vol)
{
    if (IsAbsMrgZero(range_n - near_range) && IsAbsMrgZero(range_f - far_range) && IsAbsMrgZero(range_mid - mid) &&
        IsAbsMrgZero(range_mid_vol - mid_vol))
        return;
    range_n = near_range;
    range_f = far_range;
    range_mid = mid;
    range_mid_vol = mid_vol;
    snd_flags |= SOUNDF_RANGE;
}

// The angles are at most PI
// FUNCTION: LS3DF 0x10057eb0
void I3D_sound::SetCone(float in_angle, float out_angle)
{
    if (IsAbsMrgZero(cone_in - in_angle) && IsAbsMrgZero(cone_out - out_angle))
        return;
    cone_in = in_angle;
    cone_out = out_angle;
    if (in_angle > PI)
        cone_in = PI;
    if (out_angle > PI)
        cone_out = PI;
    snd_flags |= SOUNDF_CONE;
    cos_cone_in = (float)cos(cone_in * .5);
    cos_cone_out = (float)cos(cone_out * .5);
}

// FUNCTION: LS3DF 0x10057f90
void I3D_sound::SetOutVol(float vol)
{
    if (IsAbsMrgZero(volume_out - vol))
        return;
    volume_out = vol;
    snd_flags |= SOUNDF_OUTVOL;
}

// FUNCTION: LS3DF 0x10057fd0
void I3D_sound::SetVolume(float vol)
{
    if (IsAbsMrgZero(volume - vol))
        return;
    volume = vol;
    snd_flags |= SOUNDF_VOLUME;
}

// FUNCTION: LS3DF 0x10058010
void I3D_sound::SetLoop(bool loop)
{
    this->loop = loop;
    if (loop)
        open_flags |= ISND_LOOP;
    else
        open_flags &= ~ISND_LOOP;
    if (frm_flags & FRMFLAGS_ON)
        SetOn(true);
}

// FUNCTION: LS3DF 0x10058060
LS3D_RESULT I3D_sound::SetCurrTime(uint time)
{
    if (!voice)
        return I3D_OK;
    start_time = timeGetTime() - time;
    if (state == SOUNDSTATE_PAUSE || state == SOUNDSTATE_RESUME) {
        pause_time = timeGetTime();
        pause_pos = time;
        if (state == SOUNDSTATE_PAUSE)
            return I3D_OK;
    }
    return voice->SetCurrTime(time) < 0 ? I3DERR_GENERIC : I3D_OK;
}

// FUNCTION: LS3DF 0x100580e0
void I3D_sound::SetFrequency(float freq)
{
    if (IsAbsMrgZero(frequency - freq))
        return;
    frequency = freq;
    snd_flags |= SOUNDF_FREQUENCY;
}

// FUNCTION: LS3DF 0x10058120
void I3D_sound::SetVelocity(const S_vector &v)
{
    if (IsAbsMrgZero(velocity.x - v.x) && IsAbsMrgZero(velocity.y - v.y) && IsAbsMrgZero(velocity.z - v.z))
        return;
    velocity = v;
    snd_flags |= SOUNDF_VELOCITY;
}

// FUNCTION: LS3DF 0x100581d0
void I3D_sound::GetRange(float &near_range, float &far_range, float &mid, float &mid_vol) const
{
    near_range = range_n;
    far_range = range_f;
    mid = range_mid;
    mid_vol = range_mid_vol;
}

// FUNCTION: LS3DF 0x10058210
void I3D_sound::GetCone(float &in_angle, float &out_angle) const
{
    in_angle = cone_in;
    out_angle = cone_out;
}

// FUNCTION: LS3DF 0x10058230
uint I3D_sound::GetCurrTime() const
{
    if (!voice)
        return 0;
    return voice->GetCurrTime();
}

// FUNCTION: LS3DF 0x10058250
uint I3D_sound::GetPlayTime() const
{
    if (!voice)
        return 0;
    return snd_src->GetPlayTime();
}

// Switching on plays the sound if the camera is in one of its sectors and one of its areas is active, from where it
// would be by now if it paused; update gives the voice its parameters at once.
// FUNCTION: LS3DF 0x10058280
void I3D_sound::SetOn(bool on, bool update)
{
    I3D_frame::SetOn(on);
    if (!voice)
        return;
    if (!snd_type) {
        voice->Stop();
        m_SetState(SOUNDSTATE_STOP);
        return;
    }
    if (on) {
        if (!pCurrentCamera) {
            m_SetState(SOUNDSTATE_NOCAMERA);
            return;
        }
        // The size tested apart from the loop's count: the sectors' search then tests its count after the loop (no
        // threading), the areas' keeps its loop guard and its flag in a register
        if (sectors.size() > 0) {
            I3D_sector **sp = &sectors[0];
            int i;
            for (i = sectors.size(); i--; sp++) {
                if (pCurrentCamera->sector == *sp)
                    break;
            }
            if (i < 0) {
                m_Pause();
                return;
            }
            snd_flags |= SOUNDF_CAMERA_SECTOR;
        }
        if (areas.size() > 0) {
            I3D_area **ap = &areas[0];
            bool found = false;
            for (int n = areas.size(); n--; ap++) {
                if (CurrentActiveAreaList.find(*ap)) {
                    found = true;
                    n = 0;
                }
            }
            if (!found) {
                m_Pause();
                return;
            }
            snd_flags = (snd_flags & ~SOUNDF_FADE_OUT) | SOUNDF_FADE_IN;
        }
        if (state != SOUNDSTATE_PAUSE && state != SOUNDSTATE_RESUME)
            pause_pos = 0;
        else {
            uint elapsed = timeGetTime() - pause_time;
            uint len = (uint)(GetPlayTime() / frequency);
            if (loop || pause_pos + elapsed < len) {
                int pos = (pause_pos + elapsed) % len;
                pos = (int)(pos * frequency);
                SetCurrTime(pos);
                pause_pos = pos;
            } else {
                m_SetState(SOUNDSTATE_STOP);
                return;
            }
        }
        snd_flags |= SOUNDF_PLAY;
        m_SetState(SOUNDSTATE_PLAY);
        start_time = timeGetTime();
        if (update)
            m_UpdateSound();
    } else {
        voice->Stop();
        snd_flags &= ~SOUNDF_PLAY;
        m_SetState(SOUNDSTATE_STOP);
    }
}

// Stops the voice, keeping the time and position, as the sound goes out of range
inline void I3D_sound::m_Pause()
{
    if (state == SOUNDSTATE_PAUSE)
        return;
    m_SetState(SOUNDSTATE_PAUSE);
    snd_flags &= ~SOUNDF_PLAY;
    pause_time = timeGetTime();
    pause_pos = GetCurrTime();
    if (voice)
        voice->Stop();
}

// FUNCTION: LS3DF 0x10058530
void I3D_sound::ForceUpdate()
{
    m_UpdateSound();
}

// Gives the voice the volume for the camera's distance (and direction, for a spot sound), the fading and what changed,
// pauses the sound out of range and plays it when it comes back.
// - The near range's clamp is an if: windows.h's min stores through a compare that keeps the value (fcom, jnp).
// - The spot sound's direction is written out: through GetWorldDir() its nested Normalize call doesn't get the inline
//   budget the original has for it. SetDir's argument is GetWorldDir(), whose result is copied into a temporary.
// - The fade step's (float) keeps 1.0 / fade_length a factor (VC6 turns a double product into one division), which
//   the time multiplies with fimul.
// - Open: the original's fade-out loads fade before the step (fld, fsubp), ours subtracts it from memory (fsubr), two
//   bytes shorter, which moves the padding before the jump table. Double operands load it first, but then multiply
//   the time as a double (fild qword); no spelling found has both.
// FUNCTION: LS3DF 0x10058540
void I3D_sound::m_UpdateSound()
{
    if (state == SOUNDSTATE_STOP || !(frm_flags & FRMFLAGS_ON) || !pCurrentCamera || !voice)
        return;
    if (state == SOUNDSTATE_NOCAMERA)
        SetOn(true);
    if (snd_flags & SOUNDF_TYPE) {
        snd_flags &= ~SOUNDF_TYPE;
        switch (snd_type) {
        case I3DSOUND_NULL:
            if (open_flags & ISND_3DBUFFER)
                voice->lpDSB3D8->SetMode(DS3DMODE_DISABLE, DS3D_IMMEDIATE);
            SetOn(false);
            return;
        case I3DSOUND_AMBIENT:
            if (open_flags & ISND_3DBUFFER)
                voice->lpDSB3D8->SetMode(DS3DMODE_DISABLE, DS3D_IMMEDIATE);
            break;
        case I3DSOUND_POINT:
        case I3DSOUND_SPOT:
            voice->lpDSB3D8->SetMode(DS3DMODE_NORMAL, DS3D_IMMEDIATE);
            break;
        }
    }
    if (!snd_type) {
        SetOn(false);
        return;
    }
    if (!IsPlaying(false)) {
        m_SetState(SOUNDSTATE_STOP);
        snd_flags &= ~SOUNDF_PLAY;
        return;
    }
    if (snd_flags & (SOUNDF_RANGE | SOUNDF_SCALE)) {
        scaled_range_n = range_n * world_scale;
        scaled_range_f = range_f * world_scale;
        scaled_range_f_2 = scaled_range_f * scaled_range_f;
        if (scaled_range_n >= scaled_range_f - MRG_ZERO)
            scaled_range_n = scaled_range_f - MRG_ZERO;
        scaled_range_mid = scaled_range_n + range_mid * (scaled_range_f - scaled_range_n);
        snd_flags &= ~(SOUNDF_RANGE | SOUNDF_SCALE);
    }
    S_vector cam_pos = pCurrentCamera->GetWorldPos();
    S_vector d = cam_pos - GetWorldPos();
    float dist_2 = d.Square();
    if (snd_type != I3DSOUND_AMBIENT && dist_2 >= scaled_range_f_2) {
        m_Pause();
        return;
    }
    if (state == SOUNDSTATE_PAUSE && (snd_flags & SOUNDF_CAMERA_SECTOR || !sectors.size())) {
        if (snd_flags & SOUNDF_FADE_IN)
            m_SetState(SOUNDSTATE_RESUME);
        else if (snd_flags & SOUNDF_FADE_OUT && !IsMrgZeroLess(fade))
            m_SetState(SOUNDSTATE_RESUME);
        else if (!areas.size())
            m_SetState(SOUNDSTATE_RESUME);
    }
    if (state == SOUNDSTATE_PAUSE)
        return;

    float dist = (float)sqrt(dist_2);
    float vol;
    switch (snd_type) {
    case I3DSOUND_POINT:
        vol = GetRangeVolume(dist);
        break;
    case I3DSOUND_SPOT: {
        d.Normalize();
        S_vector dir = GetMatrix()(2);
        dir.Normalize();
        float a = dir.Dot(d);
        float cone_vol;
        if (a <= cos_cone_out) {
            if (IsAbsMrgZero(volume_out)) {
                m_Pause();
                return;
            }
            cone_vol = volume_out;
        } else if (a > cos_cone_in)
            cone_vol = 1.0f;
        else
            cone_vol = volume_out + (1.0f - volume_out) * (1.0 - (a - cos_cone_in) / (cos_cone_out - cos_cone_in));
        vol = cone_vol * GetRangeVolume(dist);
    } break;
    case I3DSOUND_AMBIENT:
        vol = 1.0f;
        break;
    }
    vol *= volume;

    // The sector's blend into its sound environment
    if (fade_sector) {
        if (fade_sector->sct_flags & SCTF_SOUND_FADE_IN && IsMrgZeroLess(fade_sector->unknown_1e8)) {
            fade_sector = NULL;
            m_Pause();
            return;
        }
        if (!(fade_sector->sct_flags & SCTF_SOUND_FADE_IN) && IsMrgZeroLess(1.0 - fade_sector->unknown_1e8)) {
            fade_sector = NULL;
            return;
        }
        vol *= fade_sector->unknown_1e8;
    }

    // The fading in its areas
    if (areas.size() > 0) {
        if (fade >= 0.0f) {
            if (snd_flags & SOUNDF_FADE_IN && !IsMrgZeroLess(1.0 - fade)) {
                uint time = timeGetTime();
                fade = min(fade + (float)(1.0 / fade_length) * (time - fade_time), 1.0);
                fade_time = time;
                vol *= fade;
            } else if (snd_flags & SOUNDF_FADE_OUT && !IsMrgZeroLess(fade)) {
                uint time = timeGetTime();
                fade = max(fade - (float)(1.0 / fade_length) * (time - fade_time), 0.0);
                fade_time = time;
                vol *= fade;
            }
        }
        if (!(snd_flags & SOUNDF_FADE_IN) && IsMrgZeroLess(fade)) {
            snd_flags &= ~SOUNDF_FADE_OUT;
            fade = -1.0f;
            m_Pause();
            return;
        }
        if (!(snd_flags & SOUNDF_FADE_OUT) && IsMrgZeroLess(1.0 - fade))
            fade = -1.0f;
    }

    if (snd_flags & SOUNDF_VOLUME || fabs(virtual_volume - vol) > .01f) {
        virtual_volume = vol;
        voice->SetVolume(vol);
    }
    if (snd_flags & SOUNDF_FREQUENCY)
        voice->SetFrequency(frequency);
    if (snd_flags & SOUNDF_POS)
        voice->SetPos(GetWorldPos());
    if (snd_flags & SOUNDF_DIR)
        voice->SetDir(GetWorldDir());
    if (snd_flags & SOUNDF_VELOCITY)
        voice->SetVelocity(velocity);
    snd_flags &= ~(SOUNDF_POS | SOUNDF_DIR | SOUNDF_RANGE | SOUNDF_VOLUME | SOUNDF_OUTVOL | SOUNDF_FREQUENCY |
        SOUNDF_CONE | SOUNDF_VELOCITY);
    if (state == SOUNDSTATE_RESUME)
        SetOn(true);
    if (snd_flags & SOUNDF_PLAY) {
        voice->SetVolume(virtual_volume);
        // Two calls, as PS2's: their common loads and the priority's push come before the test (a ?: argument is
        // computed before them)
        if (pause_pos > 0)
            voice->Play(open_flags | ISND_NOREWIND, priority);
        else
            voice->Play(open_flags, priority);
        snd_flags &= ~SOUNDF_PLAY;
    }
}

// Takes the other sound's file and parameters. The state it takes is the low byte of the other's flags, which PS2's
// does too.
// FUNCTION: LS3DF 0x10059250
LS3D_RESULT I3D_sound::Duplicate(I3D_frame *frame)
{
    if (type == frame->type) {
        I3D_sound *snd = (I3D_sound *)frame;
        SetOn(false);
        if (voice)
            voice->Release();
        Open(snd->snd_src ? snd->snd_src->GetFileName() : "", snd->open_flags, NULL, NULL);
        loop = snd->loop;
        volume_out = snd->volume_out;
        volume = snd->volume;
        frequency = snd->frequency;
        open_flags = snd->open_flags;
        range_n = snd->range_n;
        range_f = snd->range_f;
        range_mid = snd->range_mid;
        range_mid_vol = snd->range_mid_vol;
        snd_type = snd->snd_type;
        cone_in = snd->cone_in;
        cone_out = snd->cone_out;
        m_SetState((uchar)snd->snd_flags);
    }
    return I3D_frame::Duplicate(frame);
}

// FUNCTION: LS3DF 0x10059360
void I3D_sound::m_SetState(uchar new_state)
{
    state = new_state;
}

// The sounds aren't loaded ("nepodporovany typ": unsupported type). The linker folds it with I3D_area::LoadImage, whose
// copy the original's vtable points at.
void I3D_sound::LoadImage()
{
    dbgPrintf("** nepodporovany typ:%s  LoadImage()  %s", pI3DDriver->FrameType2String(type), GetName());
}
