#pragma once

#include "ls3df.h"

#include <mmsystem.h>
#include <dsound.h>

// The sound interfaces (PS2 files IS_3Dlistener.cpp, IS_3Dsound.cpp, IS_driver.cpp, IS_sound.cpp, IS_source.cpp).
// The virtual functions the game calls are __stdcall. Member names are guesses unless noted.

// From the DirectX 8.1 SDK: dxguid.lib's GUIDs
// GLOBAL: LS3DF 0x100a183c
// IID_IKsPropertySet
// GLOBAL: LS3DF 0x100a185c
// IID_IDirectSound3DBuffer
// GLOBAL: LS3DF 0x100a186c
// IID_IDirectSound3DListener
// GLOBAL: LS3DF 0x100a187c
// IID_IDirectSoundBuffer8
// GLOBAL: LS3DF 0x100a188c
// IID_IDirectSound8

// What ISND_driver::Init(ISND_INIT *) takes (PS2 name)
struct ISND_INIT {
    int freq;
    int channels;
    int bits;
    uint flags;                 // ISNDINIT_*
    void *hwnd;
};

// ISND_driver::Init's flags; the last two are the driver's own, in init_flags
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

// The other sound classes: only their size, constructor and what IS_driver.cpp uses so far. The rest of their
// layout isn't recovered yet.
class ISND_source {
    uchar unknown_00[0xc];
    C_str file_name;
    uchar unknown_10[0x3c];

public:
    ISND_source();

    // The out-of-line copy the linker kept is in I3D code. Its "" is the engine's at 0x100af7fc, which reccmp can't
    // pair: it names strings by their text, and the CRT's "" in .rdata already has that name.
    // FUNCTION: LS3DF 0x10059370
    const char *GetFileName() const
    {
        return file_name ? file_name : C_str("");
    }
};

class ISND_sound {
    uchar unknown_00[0x8];

protected:
    int unknown_08;
    int unknown_0c;
    int unknown_10;
    float unknown_14;
    float unknown_18;

public:
    ISND_sound();
};

class ISND_3Dsound : public ISND_sound {
    bool unknown_1c;
    int unknown_20;

public:
    ISND_3Dsound();
};

class ISND_3Dlistener {
    uchar unknown_00[0x8];

public:
    DS3DLISTENER params;

    ISND_3Dlistener();
};

// The DirectSound driver (PS2 IS_driver.cpp). The PS2 build declares the virtual functions in this order; MSVC groups
// each name's overloads at the first one's slot in reverse order, which gives the original's vtable.
// VTABLE: LS3DF 0x1009da88
class ISND_driver {
public:
    ISND_driver();
    ~ISND_driver();

    virtual LS3D_RESULT __stdcall Init(void *hwnd, int freq, int channels, int bits, uint flags);
    virtual LS3D_RESULT __stdcall Init(ISND_INIT *init);
    virtual void __stdcall Close();
    virtual bool __stdcall IsInit() const;
    virtual LS3D_RESULT __stdcall InitBuffer(int freq, int channels, int bits);
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
    bool InitDSound(void *hwnd, int freq, int channels, int bits, uint flags);

    void *hwnd;                             // 0x04
    int ref;                                // 0x08
    IDirectSound8 *lpDS8;                   // 0x0c
    IKsPropertySet *eax_property_set;       // 0x10, of eax_buffer
    IDirectSound3DBuffer *eax_buffer;       // 0x14, of a small static 3D buffer, for EAX
    HMODULE module;                         // 0x18
    int unknown_1c;                         // 0x1c
    void *unknown_20;                       // 0x20
    int unknown_24;                         // 0x24
    IDirectSoundBuffer *lpDSBPrimary;       // 0x28
    IDirectSound3DListener *lpListener;     // 0x2c
    uint init_flags;                        // 0x30, ISNDINIT_*
    float volume;                           // 0x34
    uint max_freq;                          // 0x38
    uint min_freq;                          // 0x3c
    C_list<ISND_source *> sources;          // 0x40
    int unknown_50;                         // 0x50
    int unknown_54;                         // 0x54
};

// PS2 names
extern bool ISndIsInit;
extern int MaxHWChannels;
