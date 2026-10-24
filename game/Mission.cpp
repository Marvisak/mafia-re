// The mission (PS2 Mission.cpp), the file at 0x53efb0-0x54913c (docs/game-files.md). Not written yet: only what main
// needs to link.

#include "Mission.h"

#include "ls3df.h"

#pragma auto_inline(off)

// STUB: GAME 0x5405e0
void C_mission::Close()
{
    dbgPrintf("STUB: C_mission::Close");
}

// STUB: GAME 0x5409d0
int C_mission::Open(const char *name, bool, uint, bool)
{
    dbgPrintf("STUB: C_mission::Open %s", name);
    return 1;
}

// STUB: GAME 0x545b20
void S_mission_options::SetLodQuality(float quality)
{
    dbgPrintf("STUB: S_mission_options::SetLodQuality");
    lod_quality = quality;
}

// STUB: GAME 0x545b60
void S_mission_options::SetUnknown4(float value)
{
    dbgPrintf("STUB: S_mission_options::SetUnknown4");
    unknown_4 = value;
}

// STUB: GAME 0x545ba0
void S_mission_options::SetUnknownC(int value)
{
    dbgPrintf("STUB: S_mission_options::SetUnknownC");
    unknown_c = value;
}

// STUB: GAME 0x545bd0
void S_mission_options::SetUnknown10(int value)
{
    dbgPrintf("STUB: S_mission_options::SetUnknown10");
    unknown_10 = value;
}

// STUB: GAME 0x545c00
void S_mission_options::SetUnknown14(int index)
{
    dbgPrintf("STUB: S_mission_options::SetUnknown14");
    unknown_14 = index;
}

// STUB: GAME 0x545c60
int S_mission_options::GetUnknown14()
{
    dbgPrintf("STUB: S_mission_options::GetUnknown14");
    return unknown_14;
}

// STUB: GAME 0x545cb0
void S_mission_options::SetUnknown18(int value)
{
    dbgPrintf("STUB: S_mission_options::SetUnknown18");
    unknown_18 = value;
}

#pragma auto_inline(on)
