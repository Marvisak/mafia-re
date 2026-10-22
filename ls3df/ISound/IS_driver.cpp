// The DirectSound driver, at 0x1007b690-0x1007c29e in the original. The PS2 build names the file and its functions;
// InitEAX and the EAX and DirectSound 8 code are PC only.
// The file ends with the dynamic initializer of a zero S_vector at 0x101c62d0 (0x1007c280). Every engine file has
// one, from a header (I3d_math.h's ZeroSVector; PS2's files construct a zero vector and an identity quaternion at the
// start of each __sinit). /YX puts it at the file's end, or mid-file in the long files (see tools/autopch.py).
#include "ISound/ISound.h"
#include "I3D/I3D.h"
#include "ISound/eax.h"

// Zero-initialized globals follow the ones VC6's hash places (the zero vector), in declaration order

// GLOBAL: LS3DF 0x101c62dc
// GLOBAL: LS3DF11 0x101c33dc
bool ISndIsInit = false;

// GLOBAL: LS3DF 0x101c62e0
// GLOBAL: LS3DF11 0x101c33e0
int MaxHWChannels = 0;

// FUNCTION: LS3DF 0x1007b690
// FUNCTION: LS3DF11 0x100784d0
ISND_driver::ISND_driver()
{
    ref = 1;
    lpDS8 = NULL;
    eax_property_set = NULL;
    ds_property = NULL;
    module = NULL;
    unknown_1c = 0;
    streams = NULL;
    lpDSP = NULL;
    ds_listener8 = NULL;
    volume = 0.0f;
    create_flags = 0;
    hwnd = NULL;
    unknown_50 = 0;
    unknown_54 = 0;
}

// FUNCTION: LS3DF 0x1007b6e0
// FUNCTION: LS3DF11 0x10078520
ISND_driver::~ISND_driver()
{
    if (ISndIsInit)
        dbgPrintf("!! ISND was initialised, but not closed");
}

// FUNCTION: LS3DF 0x1007b730
// FUNCTION: LS3DF11 0x10078570
bool ISND_driver::IsInit() const
{
    return ISndIsInit;
}

// FUNCTION: LS3DF 0x1007b740
// FUNCTION: LS3DF11 0x10078580
void ISND_driver::Close()
{
    if (sources.size() > 0) {
        dbgPrintf("Unreleased sound sources: %i", sources.size());
        for (int i = 0; i < sources.size(); i++)
            dbgPrintf("\t\t%s", sources[i]->GetFileName());
    }
    if (lpDS8) {
        if (streams) {
            delete streams;
            streams = NULL;
        }
        if (module) {
            FreeLibrary(module);
            module = NULL;
        }
        if (ds_listener8) {
            ds_listener8->Release();
            ds_listener8 = NULL;
        }
        if (eax_property_set) {
            eax_property_set->Release();
            eax_property_set = NULL;
        }
        if (ds_property) {
            ds_property->Release();
            ds_property = NULL;
        }
        if (lpDSP) {
            lpDSP->Release();
            lpDSP = NULL;
        }
        lpDS8->Release();
        lpDS8 = NULL;
    }
    ISndIsInit = false;
}

// FUNCTION: LS3DF 0x1007b810
// FUNCTION: LS3DF11 0x10078650
LS3D_RESULT ISND_driver::InitBuffer(int mix_freq, int num_chan, int bps)
{
    WAVEFORMATEX format;
    memset(&format, 0, sizeof(format));
    format.wBitsPerSample = bps;
    format.nChannels = num_chan;
    format.nSamplesPerSec = mix_freq;
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.cbSize = 0;
    format.nBlockAlign = format.wBitsPerSample / 8 * format.nChannels;
    format.nAvgBytesPerSec = format.nBlockAlign * format.nSamplesPerSec;
    return FAILED(lpDSP->SetFormat(&format)) ? I3DERR_INVALIDPARAMS : I3D_OK;
}

// FUNCTION: LS3DF 0x1007b8a0
// FUNCTION: LS3DF11 0x100786e0
LS3D_RESULT ISND_driver::SetVolume(float vol)
{
    if (ISndIsInit && !IsAbsMrgZero(vol - volume)) {
        // windows.h's macros: VC6 threads their repeated arguments, which puts the 1.0 before the 0.0 block
        volume = max(min(vol, 1.0f), 0.0f);
        for (int i = pI3DDriver->sounds.size(); i--; ) {
            I3D_sound *sound = pI3DDriver->sounds[i];
            if (sound->IsPlaying(false))
                sound->snd_flags |= SOUNDF_VOLUME;
        }
    }
    return I3D_OK;
}

// Its marker is in ISound.h, by name: the linker folds it
LS3D_RESULT ISND_driver::GetCaps(uint *caps) const
{
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007b960
// FUNCTION: LS3DF11 0x10078790
ISND_source *ISND_driver::CreateSource()
{
    return new ISND_source;
}

// FUNCTION: LS3DF 0x1007b9c0
// FUNCTION: LS3DF11 0x100787f0
LS3D_RESULT ISND_driver::CreateSource(ISND_source **source)
{
    if (!source)
        return I3DERR_INVALIDPARAMS;
    *source = CreateSource();
    return *source ? I3D_OK : I3DERR_OUTOFMEM;
}

// FUNCTION: LS3DF 0x1007ba00
// FUNCTION: LS3DF11 0x10078830
ISND_sound *ISND_driver::CreateSound()
{
    return new ISND_sound;
}

// FUNCTION: LS3DF 0x1007ba60
// FUNCTION: LS3DF11 0x10078890
LS3D_RESULT ISND_driver::CreateSound(ISND_sound **sound)
{
    if (!sound)
        return I3DERR_INVALIDPARAMS;
    *sound = CreateSound();
    return *sound ? I3D_OK : I3DERR_OUTOFMEM;
}

// FUNCTION: LS3DF 0x1007baa0
// FUNCTION: LS3DF11 0x100788d0
ISND_3Dsound *ISND_driver::Create3DSound()
{
    return new ISND_3Dsound;
}

// FUNCTION: LS3DF 0x1007bb00
// FUNCTION: LS3DF11 0x10078930
LS3D_RESULT ISND_driver::Create3DSound(ISND_3Dsound **sound)
{
    if (!sound)
        return I3DERR_INVALIDPARAMS;
    *sound = Create3DSound();
    return *sound ? I3D_OK : I3DERR_OUTOFMEM;
}

// FUNCTION: LS3DF 0x1007bb40
// FUNCTION: LS3DF11 0x10078970
ISND_3Dlistener *ISND_driver::CreateListener()
{
    ISND_3Dlistener *listener = new ISND_3Dlistener;
    if (!listener)
        return NULL;
    if (ds_listener8)
        ds_listener8->GetAllParameters(&listener->ds_data);
    return listener;
}

// FUNCTION: LS3DF 0x1007bbd0
// FUNCTION: LS3DF11 0x10078a00
LS3D_RESULT ISND_driver::CreateListener(ISND_3Dlistener **listener)
{
    if (!listener)
        return I3DERR_INVALIDPARAMS;
    *listener = CreateListener();
    return *listener ? I3D_OK : I3DERR_OUTOFMEM;
}

// Gets the IKsPropertySet of a small hardware 3D buffer and checks for EAX 3.0. When ds_property already exists, the
// original releases the two buffer pointers without having set them.
// FUNCTION: LS3DF 0x1007bc10
// FUNCTION: LS3DF11 0x10078a40
int ISND_driver::InitEAX()
{
    if (!lpDS8)
        return 0;
    if (!eax_property_set) {
        IDirectSoundBuffer *buffer;
        IDirectSoundBuffer8 *buffer8;
        if (!ds_property) {
            WAVEFORMATEX format;
            memset(&format, 0, sizeof(format));
            format.wFormatTag = WAVE_FORMAT_PCM;
            format.nChannels = 1;
            format.nSamplesPerSec = 22050;
            format.wBitsPerSample = 16;
            format.nBlockAlign = 2;
            format.nAvgBytesPerSec = 44100;
            DSBUFFERDESC desc;
            memset(&desc, 0, sizeof(desc));
            desc.dwSize = sizeof(desc);
            desc.dwFlags = DSBCAPS_STATIC | DSBCAPS_LOCHARDWARE | DSBCAPS_CTRL3D;
            desc.dwBufferBytes = 64;
            desc.lpwfxFormat = &format;
            if (lpDS8->CreateSoundBuffer(&desc, &buffer, NULL) != DS_OK)
                return 0;
            if (FAILED(buffer->QueryInterface(IID_IDirectSoundBuffer8, (void **)&buffer8))) {
                buffer->Release();
                return 0;
            }
            if (FAILED(buffer8->QueryInterface(IID_IDirectSound3DBuffer, (void **)&ds_property))) {
                buffer->Release();
                buffer8->Release();
                return 0;
            }
        }
        buffer->Release();
        buffer8->Release();
        if (FAILED(ds_property->QueryInterface(IID_IKsPropertySet, (void **)&eax_property_set))) {
            ds_property->Release();
            ds_property = NULL;
            return 0;
        }
    }
    create_flags |= ISNDINIT_EAX;
    if (!(create_flags & ISNDINIT_NOEAX)) {
        ULONG support;
        if (SUCCEEDED(eax_property_set->QuerySupport(DSPROPSETID_EAX30_ListenerProperties,
                DSPROPERTY_EAXLISTENER_ALLPARAMETERS, &support))
            && (support & KSPROPERTY_SUPPORT_GET) && (support & KSPROPERTY_SUPPORT_SET)
            && SUCCEEDED(eax_property_set->QuerySupport(DSPROPSETID_EAX30_BufferProperties,
                DSPROPERTY_EAXBUFFER_ALLPARAMETERS, &support))
            && (support & KSPROPERTY_SUPPORT_GET) && (support & KSPROPERTY_SUPPORT_SET)) {
            create_flags |= ISNDINIT_EAX30;
            dbgPrintf("*** FULL EAX 3.0 available ***");
            long room = -10000;
            eax_property_set->Set(DSPROPSETID_EAX30_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOM, NULL, 0, &room,
                sizeof(room));
        }
    }
    return 1;
}

// FUNCTION: LS3DF 0x1007be20
// FUNCTION: LS3DF11 0x10078c50
bool ISND_driver::InitDSound(void *hwnd, int mix_freq, int num_chan, int bps, uint flags)
{
    create_flags = flags;
    if (flags & ISNDINIT_NOEAX) {
        if (FAILED(DirectSoundCreate8(NULL, &lpDS8, NULL)))
            return false;
    } else {
        CoInitialize(NULL);
        if (FAILED(CoCreateInstance(CLSID_EAXDirectSound8, NULL, CLSCTX_INPROC_SERVER, IID_IDirectSound8,
                (void **)&lpDS8))) {
            dbgPrintf("WARNING! EAX Unified was NOT FOUND !");
            if (FAILED(DirectSoundCreate8(NULL, &lpDS8, NULL)))
                return false;
        } else if (FAILED(lpDS8->Initialize(NULL))) {
            if (FAILED(DirectSoundCreate8(NULL, &lpDS8, NULL)))
                return false;
        }
    }

    DSCAPS caps;
    memset(&caps, 0, sizeof(caps));
    caps.dwSize = sizeof(caps);
    lpDS8->GetCaps(&caps);
    max_freq = caps.dwMaxSecondarySampleRate;
    min_freq = caps.dwMinSecondarySampleRate;
    if (min_freq == max_freq || min_freq > 10000 || !min_freq || max_freq < 40000) {
        dbgPrintf("Your soundcard claims WRONG frequency bounds !! Minimal frequency: %iHz, Maximal frequency: %iHz",
            min_freq, max_freq);
        min_freq = 10000;
    }
    if (min_freq < 8000)
        min_freq = 8000;
    if (max_freq < 44000)
        max_freq = 44000;

    if (!(create_flags & ISNDINIT_NOHWCHANNELS)) {
        MaxHWChannels = caps.dwFreeHw3DAllBuffers;
        dbgPrintf("*** Sound management: %i free 3D channels claimed.", MaxHWChannels);
        if (MaxHWChannels > 35)
            MaxHWChannels = 30;
    }
    if (!(create_flags & ISNDINIT_NOHWCHANNELS))
        InitEAX();

    if (FAILED(lpDS8->SetCooperativeLevel((HWND)hwnd, DSSCL_PRIORITY)))
        return false;
    DSBUFFERDESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRL3D;
    if (FAILED(lpDS8->CreateSoundBuffer(&desc, &lpDSP, NULL)))
        return false;
    if (InitBuffer(mix_freq, num_chan, bps) != I3D_OK)
        return false;
    if (FAILED(lpDSP->QueryInterface(IID_IDirectSound3DListener, (void **)&ds_listener8)))
        return false;

    if (eax_property_set && (create_flags & ISNDINIT_EAX30)) {
        // EAX 3.0's generic preset without the room. VC6 stores these in statement order, and this is the original's.
        EAXLISTENERPROPERTIES props;
        props.ulEnvironment = EAX_ENVIRONMENT_UNDEFINED;
        props.ulFlags = EAXLISTENERFLAGS_DEFAULT;
        props.flEnvironmentSize = 7.5f;
        props.lRoomLF = 0;
        props.lRoomHF = -100;
        props.flDecayTime = 1.49f;
        props.flDecayHFRatio = 0.83f;
        props.lReflections = -2602;
        props.flReflectionsDelay = 0.007f;
        props.lReverb = 200;
        props.flReverbDelay = 0.011f;
        props.flEnvironmentDiffusion = 1.0f;
        props.flDecayLFRatio = 1.0f;
        props.lRoom = -10000;
        props.flEchoTime = 0.25f;
        props.flEchoDepth = 0.0f;
        props.flModulationTime = 0.25f;
        props.flModulationDepth = 0.0f;
        props.flRoomRolloffFactor = 0.0f;
        props.flAirAbsorptionHF = -5.0f;
        props.flHFReference = 5000.0f;
        props.flLFReference = 250.0f;
        props.vReflectionsPan.x = 0.0f;
        props.vReflectionsPan.y = 0.0f;
        props.vReflectionsPan.z = 0.0f;
        props.vReverbPan.x = 0.0f;
        props.vReverbPan.y = 0.0f;
        props.vReverbPan.z = 0.0f;
        eax_property_set->Set(DSPROPSETID_EAX30_ListenerProperties, DSPROPERTY_EAXLISTENER_ALLPARAMETERS, NULL, 0,
            &props, sizeof(props));
    }

    if (flags & ISNDINIT_HEADPHONES) {
        lpDS8->SetSpeakerConfig(DSSPEAKER_HEADPHONE);
        dbgPrintf("*** Speaker configuration: STEREO HEADPHONES");
    } else if (flags & ISNDINIT_QUAD) {
        lpDS8->SetSpeakerConfig(DSSPEAKER_QUAD);
        dbgPrintf("*** Speaker configuration: QUADRAPHONIC SPEAKERS");
    } else if (flags & ISNDINIT_SURROUND) {
        lpDS8->SetSpeakerConfig(DSSPEAKER_5POINT1);
        dbgPrintf("*** Speaker configuration: 5.1 SURROUND SOUND SPEAKERS");
    } else if (flags & ISNDINIT_STEREO) {
        lpDS8->SetSpeakerConfig(DSSPEAKER_STEREO);
        dbgPrintf("*** Speaker configuration: STEREO SPEAKERS");
    } else
        dbgPrintf("*** Speaker configuration: CURRENT WINDOWS SPEAKER CONFIGURATION");
    return true;
}

// FUNCTION: LS3DF 0x1007c1f0
// FUNCTION: LS3DF11 0x10079020
LS3D_RESULT ISND_driver::Init(void *hwnd, int mix_freq, int num_chan, int bps, uint flags)
{
    if (!InitDSound(hwnd, mix_freq, num_chan, bps, flags))
        return I3DERR_GENERIC;
    this->hwnd = hwnd;
    ISndIsInit = true;
    volume = 1.0f;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007c240
// FUNCTION: LS3DF11 0x10079070
LS3D_RESULT ISND_driver::Init(ISND_INIT *init)
{
    if (!init)
        return I3DERR_INVALIDPARAMS;
    return Init(init->hwnd, init->mix_frequency, init->num_channels, init->bits_per_second, init->flags);
}
