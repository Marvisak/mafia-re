#pragma once

// The parts of Creative's EAX 3.0 API (eax.h of the EAX SDK, which isn't in the repository) that the engine uses.
// The original defines all of these GUIDs in one file, in this order (0x100a1980-0x100a1a2f), so that file included
// <initguid.h> before them.

#include <dsound.h>

// GLOBAL: LS3DF 0x100a1980
// DSPROPSETID_EAX_ReverbProperties
// GLOBAL: LS3DF 0x100a1990
// DSPROPSETID_EAXBUFFER_ReverbProperties
// GLOBAL: LS3DF 0x100a19a0
// DSPROPSETID_EAX20_ListenerProperties
// GLOBAL: LS3DF 0x100a19b0
// DSPROPSETID_EAX20_BufferProperties
// GLOBAL: LS3DF 0x100a19c0
// DSPROPSETID_EAX30_ListenerProperties
// GLOBAL: LS3DF 0x100a19d0
// DSPROPSETID_EAX30_BufferProperties
// GLOBAL: LS3DF 0x100a19e0
// DSPROPSETID_I3DL2_ListenerProperties
// GLOBAL: LS3DF 0x100a19f0
// DSPROPSETID_I3DL2_BufferProperties
// GLOBAL: LS3DF 0x100a1a00
// EAX_GUID_UNKNOWN
// GLOBAL: LS3DF 0x100a1a10
// CLSID_EAXDirectSound
// GLOBAL: LS3DF 0x100a1a20
// CLSID_EAXDirectSound8

DEFINE_GUID(DSPROPSETID_EAX_ReverbProperties, 0x4a4e6fc1, 0xc341, 0x11d1, 0xb7, 0x3a, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
DEFINE_GUID(DSPROPSETID_EAXBUFFER_ReverbProperties, 0x4a4e6fc0, 0xc341, 0x11d1, 0xb7, 0x3a, 0x44, 0x45, 0x53, 0x54, 0x00,
    0x00);
DEFINE_GUID(DSPROPSETID_EAX20_ListenerProperties, 0x0306a6a8, 0xb224, 0x11d2, 0x99, 0xe5, 0x00, 0x00, 0xe8, 0xd8, 0xc7,
    0x22);
DEFINE_GUID(DSPROPSETID_EAX20_BufferProperties, 0x0306a6a7, 0xb224, 0x11d2, 0x99, 0xe5, 0x00, 0x00, 0xe8, 0xd8, 0xc7, 0x22);
DEFINE_GUID(DSPROPSETID_EAX30_ListenerProperties, 0xa8fa6882, 0xb476, 0x11d3, 0xbd, 0xb9, 0x00, 0xc0, 0xf0, 0x2d, 0xdf,
    0x87);
DEFINE_GUID(DSPROPSETID_EAX30_BufferProperties, 0xa8fa6881, 0xb476, 0x11d3, 0xbd, 0xb9, 0x00, 0xc0, 0xf0, 0x2d, 0xdf, 0x87);
DEFINE_GUID(DSPROPSETID_I3DL2_ListenerProperties, 0x60b721a1, 0xf7c8, 0x11d2, 0xa0, 0x2e, 0x00, 0x50, 0x04, 0x06, 0x18,
    0xb8);
DEFINE_GUID(DSPROPSETID_I3DL2_BufferProperties, 0x60b721a2, 0xf7c8, 0x11d2, 0xa0, 0x2e, 0x00, 0x50, 0x04, 0x06, 0x18, 0xb8);
// Not identified: in the block, but nothing in the engine uses it
DEFINE_GUID(EAX_GUID_UNKNOWN, 0xab6a8d23, 0x8356, 0x11d4, 0x88, 0x13, 0x00, 0x50, 0x04, 0x06, 0x2f, 0x83);
DEFINE_GUID(CLSID_EAXDirectSound, 0x4ff53b81, 0x1ce0, 0x11d3, 0xaa, 0xb8, 0x00, 0xa0, 0xc9, 0x59, 0x49, 0xd5);
DEFINE_GUID(CLSID_EAXDirectSound8, 0xca503b60, 0xb176, 0x11d4, 0xa0, 0x94, 0xd0, 0xc0, 0xbf, 0x3a, 0x56, 0x0c);

enum {
    DSPROPERTY_EAXLISTENER_NONE,
    DSPROPERTY_EAXLISTENER_ALLPARAMETERS,
    DSPROPERTY_EAXLISTENER_ENVIRONMENT,
    DSPROPERTY_EAXLISTENER_ENVIRONMENTSIZE,
    DSPROPERTY_EAXLISTENER_ENVIRONMENTDIFFUSION,
    DSPROPERTY_EAXLISTENER_ROOM,
};

enum {
    DSPROPERTY_EAXBUFFER_NONE,
    DSPROPERTY_EAXBUFFER_ALLPARAMETERS,
};

#define EAX_ENVIRONMENT_UNDEFINED 26
#define EAXLISTENERFLAGS_DEFAULT 0x3f

struct EAXVECTOR {
    float x, y, z;
};

struct EAXLISTENERPROPERTIES {
    unsigned long ulEnvironment;
    float flEnvironmentSize;
    float flEnvironmentDiffusion;
    long lRoom;
    long lRoomHF;
    long lRoomLF;
    float flDecayTime;
    float flDecayHFRatio;
    float flDecayLFRatio;
    long lReflections;
    float flReflectionsDelay;
    EAXVECTOR vReflectionsPan;
    long lReverb;
    float flReverbDelay;
    EAXVECTOR vReverbPan;
    float flEchoTime;
    float flEchoDepth;
    float flModulationTime;
    float flModulationDepth;
    float flAirAbsorptionHF;
    float flHFReference;
    float flLFReference;
    float flRoomRolloffFactor;
    unsigned long ulFlags;
};
