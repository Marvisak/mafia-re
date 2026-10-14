#pragma once

// What IShow.cpp uses of RAD Game Tools' Bink (binkw32.dll). RAD's bink.h isn't public, so this declares only that, with
// the SDK's names. The engine calls the functions through the import address table: RAD's header declares them
// __declspec(dllimport) __stdcall.

#include <types.h>

typedef unsigned int U32;
typedef int S32;

// The open video. The engine reads its size, frame count and current frame (the public members at the start) and two
// members further in, which SetCurrentPos and GetCurrentPos convert between frames and milliseconds with.
typedef struct BINK {
    U32 Width;                          // 0x00
    U32 Height;                         // 0x04
    U32 Frames;                         // 0x08
    U32 FrameNum;                       // 0x0c, the frame to be displayed (1 based)
    uchar unknown_10[0x2ac];
    U32 unknown_2bc;                    // 0x2bc
    U32 unknown_2c0;                    // 0x2c0
} BINK, *HBINK;

// BinkCopyToBufferRect's destination formats
#define BINKSURFACE32 3
#define BINKSURFACE555 9
#define BINKSURFACE565 10
// Copy the whole frame, not only the parts that changed
#define BINKCOPYALL 0x80000000L

typedef void *(__stdcall *BINKSNDSYSOPEN)(U32 param);

#define BINKEXPORT extern "C" __declspec(dllimport)

BINKEXPORT HBINK __stdcall BinkOpen(const char *name, U32 flags);
BINKEXPORT void __stdcall BinkClose(HBINK bink);
BINKEXPORT S32 __stdcall BinkDoFrame(HBINK bink);
BINKEXPORT void __stdcall BinkNextFrame(HBINK bink);
BINKEXPORT S32 __stdcall BinkWait(HBINK bink);
BINKEXPORT S32 __stdcall BinkPause(HBINK bink, S32 pause);
BINKEXPORT void __stdcall BinkGoto(HBINK bink, U32 frame, S32 flags);
BINKEXPORT S32 __stdcall BinkCopyToBufferRect(HBINK bink, void *dest, S32 dest_pitch, U32 dest_height, U32 dest_x,
    U32 dest_y, U32 src_x, U32 src_y, U32 src_w, U32 src_h, U32 flags);
BINKEXPORT void __stdcall BinkSetVolume(HBINK bink, U32 track_id, S32 volume);
BINKEXPORT S32 __stdcall BinkSetSoundSystem(BINKSNDSYSOPEN open, U32 param);
BINKEXPORT void *__stdcall BinkOpenDirectSound(U32 param);

// Bink plays its sound through the given DirectSound object
#define BinkSoundUseDirectSound(lpDS) BinkSetSoundSystem(BinkOpenDirectSound, (U32)(lpDS))
