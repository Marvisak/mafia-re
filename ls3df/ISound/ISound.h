#pragma once

#include "ls3df.h"
#include "I3D/I3d_math.h"

#include <mmsystem.h>
#include <dsound.h>

// The sound interfaces (PS2 files IS_3Dlistener.cpp, IS_3Dsound.cpp, IS_driver.cpp, IS_sound.cpp, IS_source.cpp).
// The virtual functions the game calls are __stdcall. Member names are guesses unless noted; Insanity3D's ISound2
// (the same lineage's later sound library, published with source) gives the ones noted as its own.

// From the DirectX 8.1 SDK: dxguid.lib's GUIDs
// GLOBAL: LS3DF 0x100a182c
// GLOBAL: LS3DF11 0x1009e82c
// DS3DALG_NO_VIRTUALIZATION
// GLOBAL: LS3DF 0x100a183c
// GLOBAL: LS3DF11 0x1009e83c
// IID_IKsPropertySet
// GLOBAL: LS3DF 0x100a184c
// GLOBAL: LS3DF11 0x1009e84c
// IID_IDirectSoundNotify
// GLOBAL: LS3DF 0x100a185c
// GLOBAL: LS3DF11 0x1009e85c
// IID_IDirectSound3DBuffer
// GLOBAL: LS3DF 0x100a186c
// GLOBAL: LS3DF11 0x1009e86c
// IID_IDirectSound3DListener
// GLOBAL: LS3DF 0x100a187c
// GLOBAL: LS3DF11 0x1009e87c
// IID_IDirectSoundBuffer8
// GLOBAL: LS3DF 0x100a188c
// GLOBAL: LS3DF11 0x1009e88c
// IID_IDirectSound8

// What ISND_driver::Init(ISND_INIT *) takes (PS2 name)
struct ISND_INIT {
    int mix_frequency;                  // the member names are Insanity3D's
    int num_channels;
    int bits_per_second;
    uint flags;                 // ISNDINIT_*
    void *hwnd;
};

// ISND_driver::Init's flags; the last two are the driver's own, in create_flags
enum {
    ISNDINIT_NOEAX = 0x10,              // DirectSoundCreate8 instead of EAX Unified's COM object
    ISNDINIT_NOHWCHANNELS = 0x20,       // neither count the free hardware 3D channels nor look for EAX 3.0
    ISNDINIT_HEADPHONES = 0x400,
    ISNDINIT_STEREO = 0x800,
    ISNDINIT_QUAD = 0x1000,
    ISNDINIT_SURROUND = 0x2000,         // 5.1
    ISNDINIT_EAX = 0x100000,            // a 3D buffer gave an IKsPropertySet
    ISNDINIT_EAX30 = 0x200000,          // it supports EAX 3.0's listener and buffer properties
};

// The flags of ISND_source::Open and ISND_sound::Open and Play. Insanity3D's ISound2 has these names for the same
// values (and ISND_STREAMING 1, ISND_PRELOAD 8, ISND_CARDMEM 0x20, which the code written so far doesn't use).
enum {
    ISND_NOVOLUME = 0x2,                // sound: SetVolume fails
    ISND_NOPANNING = 0x4,               // sound: SetPanning fails
    ISND_FREQUENCY = 0x10,              // sound: SetFrequency works
    ISND_SYSMEM = 0x40,                 // a software buffer: no hardware 3D channel management (Play, Stop)
    ISND_LOOP = 0x100,                  // Play
    ISND_3DBUFFER = 0x200,              // a 3D buffer (mono sources only)
    ISND_NOREWIND = 0x400,              // Play from the current position
};

// What ISND_source::GetFormat returns (PS2 name)
struct S_wave_format {
    uint num_channels;
    uint bytes_per_sample;
    uint samples_per_sec;
    uint size;                          // of the sound data
};

// A sound loaded from a WAV file into a DirectSound buffer (PS2 IS_source.cpp). It isn't polymorphic on PS2.
// VTABLE: LS3DF 0x1009db90
// VTABLE: LS3DF11 0x1009ab90
class ISND_source {
public:
    ISND_source();
    ~ISND_source();

    virtual int __stdcall Release();
    virtual LS3D_RESULT __stdcall Open(const char *name, uint flags);
    virtual void __stdcall Close();
    virtual bool __stdcall IsOpen() const;          // Insanity3D's name for this slot
    virtual uint __stdcall GetPlayTime() const;
    virtual LS3D_RESULT __stdcall GetFormat(S_wave_format *wf1);
    virtual LS3D_RESULT __stdcall Lock(void **data);
    virtual LS3D_RESULT __stdcall Unlock();

    int AddRef()
    {
        return ++ref;
    }

    // The out-of-line copy the linker kept is in I3D code. Its "" is the engine's, in .data like VC6's other literals.
    // reccmp's string search skips empty strings, so the original's needs a marker; the CRT's in .rdata is "\x00".
    // FUNCTION: LS3DF 0x10059370
    // FUNCTION: LS3DF11 0x10056a30
    const char *GetFileName() const
    {
        // STRING: LS3DF 0x100af7fc
        // STRING: LS3DF11 0x100ac91c
        return filename ? filename : C_str("");
    }

    // open_flags, filename, wf, lpDSB8 and the lock members are Insanity3D's names, in its order
    int ref;                            // 0x04
    uint open_flags;                    // 0x08, ISND_*
    C_str filename;                     // 0x0c
    S_wave_format wf;                   // 0x10
    IDirectSoundBuffer8 *lpDSB8;        // 0x20
    float bytes_per_ms;                 // 0x24
    // Lock's two areas, in Insanity3D's order (and its typo); the PC's Lock asks only for the first
    void *save_lock_ptr1;               // 0x28
    void *save_lock_ptr2;               // 0x2c
    DWORD save_lick_size1;              // 0x30
    DWORD save_lick_size2;              // 0x34
    WAVEFORMATEX wave_format;           // 0x38
};

// A sound playing a source (PS2 IS_sound.cpp)
// VTABLE: LS3DF 0x1009db08
// VTABLE: LS3DF11 0x1009ab08
class ISND_sound {
public:
    ISND_sound();

    virtual int __stdcall Release();
    virtual LS3D_RESULT __stdcall Open(ISND_source *source, uint flags);
    virtual LS3D_RESULT __stdcall Close();
    virtual LS3D_RESULT __stdcall Play(ISND_source *source, uint flags);
    virtual LS3D_RESULT __stdcall Play(uint flags, uint priority);
    virtual LS3D_RESULT __stdcall Stop();
    virtual uint __stdcall GetCurrTime() const;
    virtual LS3D_RESULT __stdcall SetCurrTime(uint time);
    virtual LS3D_RESULT __stdcall SetVolume(float volume);
    virtual LS3D_RESULT __stdcall SetVolumeLOG(float volume);
    virtual LS3D_RESULT __stdcall SetVolumeLOG(int volume);
    virtual LS3D_RESULT __stdcall SetFrequency(float frequency);
    virtual LS3D_RESULT __stdcall SetPanning(float panning);
    virtual bool __stdcall IsPlaying() const;

    // The out-of-line copy the linker kept is in IS_3Dsound.cpp, after its last function
    // FUNCTION: LS3DF 0x1007b680
    // FUNCTION: LS3DF11 0x100784c0
    virtual ISND_source *__stdcall GetSource()
    {
        return source;
    }

    virtual ~ISND_sound();

    // IS_sound.cpp reads the volume and the frequency factor (SetFrequency's) through inline functions. The operand
    // of a float compare or division is then loaded before the other one where a plain member read isn't: the volume
    // compares load both onto the FPU stack (fld, fld, fcompp), and the frequency is loaded before the call that
    // gives the dividend.
    float GetVolume() const
    {
        return volume;
    }

    float GetFrequency() const
    {
        return freq_mult;
    }

    // lpDSB8 and freq_mult are Insanity3D's names
    int ref;                            // 0x04
    IDirectSoundBuffer *lpDSB8;         // 0x08, a duplicate of the source's
    ISND_source *source;                // 0x0c
    uint flags;                         // 0x10, ISND_*
    float freq_mult;                    // 0x14, of the source's frequency
    float volume;                       // 0x18
};

// SYNTHETIC: LS3DF 0x1007c910
// SYNTHETIC: LS3DF11 0x10079740
// ISND_sound::`scalar deleting destructor'

// A sound with a 3D buffer (PS2 IS_3Dsound.cpp)
// VTABLE: LS3DF 0x1009d9e0
// VTABLE: LS3DF11 0x1009a9e0
class ISND_3Dsound : public ISND_sound {
public:
    ISND_3Dsound();

    virtual LS3D_RESULT __stdcall Open(ISND_source *source, uint flags);
    virtual LS3D_RESULT __stdcall Close();
    virtual ~ISND_3Dsound();
    virtual bool __stdcall Is3DInit() const;
    virtual LS3D_RESULT __stdcall SetPos(const S_vector &pos);
    virtual LS3D_RESULT __stdcall SetDir(const S_vector &dir);
    virtual LS3D_RESULT __stdcall SetVelocity(const S_vector &velocity);
    virtual LS3D_RESULT __stdcall SetDistances(float min, float max);
    virtual LS3D_RESULT __stdcall SetCone(float in_angle, float out_angle);
    virtual LS3D_RESULT __stdcall SetOutVolume(float out_vol);

    bool is3d_init;                     // 0x1c
    IDirectSound3DBuffer *lpDSB3D8;     // 0x20, Insanity3D's name
};

// SYNTHETIC: LS3DF 0x1007b390
// SYNTHETIC: LS3DF11 0x100781d0
// ISND_3Dsound::`scalar deleting destructor'

// The listener (PS2 IS_3Dlistener.cpp). Its destructor isn't virtual: I3D code calls it directly.
// VTABLE: LS3DF 0x1009d968
// VTABLE: LS3DF11 0x1009a968
class ISND_3Dlistener {
public:
    ISND_3Dlistener();
    ~ISND_3Dlistener();

    virtual int __stdcall AddRef();
    virtual int __stdcall Release();
    virtual LS3D_RESULT __stdcall SetPos(const S_vector &pos);
    virtual LS3D_RESULT __stdcall SetDir(const S_vector &dir, const S_vector &up);
    virtual LS3D_RESULT __stdcall SetVelocity(const S_vector &velocity);
    virtual LS3D_RESULT __stdcall SetDistanceFactor(float factor);
    virtual LS3D_RESULT __stdcall SetDopplerFactor(float factor);
    virtual LS3D_RESULT __stdcall SetRolloffFactor(float factor);
    virtual LS3D_RESULT __stdcall SetEAXProperty(int property, uint value);
    virtual LS3D_RESULT __stdcall SetEAX3Property(uint property, void *data, uint size);
    virtual LS3D_RESULT __stdcall GetEAX3Property(uint property, void *data, uint size, uint *returned);
    virtual LS3D_RESULT __stdcall Update();
    virtual LS3D_RESULT __stdcall CommitDeferredSettings();

    int ref;                            // 0x04
    DS3DLISTENER ds_data;               // 0x08, Insanity3D's name
};

// SetDistanceFactor is folded with I3D_material::SetEnvAlpha, the same code. The original keeps the listener file's
// copy; which one our linker keeps changes with the link, so the marker is by the name our PDB keeps (tools/foldfix.py
// updates it).
// FUNCTION: LS3DF 0x1007b180
// ?SetDistanceFactor@ISND_3Dlistener@@UAG?AW4LS3D_RESULT@@M@Z

// FUNCTION: LS3DF11 0x1002a480
// ?SetEnvAlpha@I3D_material@@UAG?AW4LS3D_RESULT@@M@Z

// The hardware 3D channel lists (IS_sound.cpp; PS2 names)
struct CHANNEL_STRUCT {
    ISND_sound *sound;
    IDirectSoundBuffer *buffer;
    uint priority;
    uint loop;                          // ISND_LOOP
    uint start_time;                    // timeGetTime
};

struct TERMINATED_CHANNEL_STRUCT {
    ISND_sound *sound;
    IDirectSoundBuffer *buffer;
    uint priority;
    DWORD position;
    uint stop_time;                     // timeGetTime
    uint loop;                          // ISND_LOOP
};

// The destructors IS_sound.cpp's thunks register for ChannelList and TerminatedChannelList: the last $E5 and $E10 in
// link order, after ITexture.cpp's
// SYNTHETIC: LS3DF 0x1007c2d0
// SYNTHETIC: LS3DF11 0x10079100
// $E5

// SYNTHETIC: LS3DF 0x1007c320
// SYNTHETIC: LS3DF11 0x10079150
// $E10

// The stream functions the driver keeps (PC only), through which IShow plays music. Nothing in LS3DF fills the table in:
// the driver only deletes it when it closes, and frees a library it never loads (module), presumably the one that
// provided it. The functions are __fastcall, the engine's default; the names are guesses.
struct ISND_STREAM_FUNCS {
    void *unknown_00[4];
    HRESULT(__fastcall *Init)(int unknown);                                 // 0x10
    HRESULT(__fastcall *Close)(int unknown);                                // 0x14
    void *unknown_18;
    HRESULT(__fastcall *SetPan)(int stream, int unknown, int pan);          // 0x1c
    HRESULT(__fastcall *SetVolume)(int stream, int unknown, int volume);    // 0x20
    HRESULT(__fastcall *Open)(int unknown, int *stream, void(__stdcall *callback)(int stream, int event),
        const char *name, int unknown1);                                    // 0x24
    HRESULT(__fastcall *CloseStream)(int stream);                           // 0x28
    void *unknown_2c;
    HRESULT(__fastcall *Play)(int stream, int loop);                        // 0x30
    HRESULT(__fastcall *Stop)(int stream);                                  // 0x34
    HRESULT(__fastcall *Pause)(int stream);                                 // 0x38
    HRESULT(__fastcall *Resume)(int stream);                                // 0x3c
};

// The DirectSound driver (PS2 IS_driver.cpp). The PS2 build declares the virtual functions in this order; MSVC groups
// each name's overloads at the first one's slot in reverse order, which gives the original's vtable.
// VTABLE: LS3DF 0x1009da88
// VTABLE: LS3DF11 0x1009aa88
class ISND_driver {
public:
    ISND_driver();
    ~ISND_driver();

    virtual LS3D_RESULT __stdcall Init(void *hwnd, int mix_freq, int num_chan, int bps, uint flags);
    virtual LS3D_RESULT __stdcall Init(ISND_INIT *init);
    virtual void __stdcall Close();
    virtual bool __stdcall IsInit() const;
    virtual LS3D_RESULT __stdcall InitBuffer(int mix_freq, int num_chan, int bps);
    virtual LS3D_RESULT __stdcall SetVolume(float volume);
    virtual LS3D_RESULT __stdcall GetCaps(uint *caps) const;
    virtual ISND_source *__stdcall CreateSource();
    virtual ISND_sound *__stdcall CreateSound();
    virtual ISND_3Dsound *__stdcall Create3DSound();
    virtual ISND_3Dlistener *__stdcall CreateListener();
    virtual LS3D_RESULT __stdcall CreateSource(ISND_source **source);
    virtual LS3D_RESULT __stdcall CreateSound(ISND_sound **sound);
    virtual LS3D_RESULT __stdcall Create3DSound(ISND_3Dsound **sound);
    virtual LS3D_RESULT __stdcall CreateListener(ISND_3Dlistener **listener);

private:
    int InitEAX();
    bool InitDSound(void *hwnd, int mix_freq, int num_chan, int bps, uint flags);

public:
    // lpDS8, eax_property_set, ds_property, lpDSP, ds_listener8 (its listener's), create_flags and volume are
    // Insanity3D's names
    void *hwnd;                             // 0x04
    int ref;                                // 0x08
    IDirectSound8 *lpDS8;                   // 0x0c
    IKsPropertySet *eax_property_set;       // 0x10, of ds_property
    IDirectSound3DBuffer *ds_property;      // 0x14, of a small static 3D buffer, for EAX
    HMODULE module;                         // 0x18
    int unknown_1c;                         // 0x1c
    ISND_STREAM_FUNCS *streams;             // 0x20
    int unknown_24;                         // 0x24
    IDirectSoundBuffer *lpDSP;              // 0x28, the primary buffer
    IDirectSound3DListener *ds_listener8;   // 0x2c
    uint create_flags;                      // 0x30, ISNDINIT_*
    float volume;                           // 0x34
    uint max_freq;                          // 0x38
    uint min_freq;                          // 0x3c
    C_list<ISND_source *> sources;          // 0x40
    int unknown_50;                         // 0x50
    int unknown_54;                         // 0x54
};

// GetCaps (return I3D_OK) is folded with other functions that return 0 the same way. The original kept its copy, our
// build its own or one of DirectShow's base classes' (CBasePin::Run's until I3d_singlemesh.cpp, from I3d_singlemorph.cpp
// until i3d_collision.cpp, from I3d_projector.cpp to render.cpp and since I3d_portal.cpp), so the marker is by the name
// our PDB keeps (tools/foldfix.py updates it).
// FUNCTION: LS3DF 0x1007b950
// ?GetCaps@ISND_driver@@UBG?AW4LS3D_RESULT@@PAI@Z

// PS2 names
extern bool ISndIsInit;
extern int MaxHWChannels;

// Restarts the terminated sounds that have a hardware channel again (IS_sound.cpp)
void UpdateAllTerminatedBuffs();
