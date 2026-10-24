#pragma once

#include "Main.h"

class C_game;

// The game's options (the struct's and the members' names are ours, but for the first, which feeds the engine's
// RS_LOD_QUALITY). main hands each the value setup.exe saved, through a setter that clamps it, and saves them back when
// the player quits. PS2's C_mission has the floats at 0xcc0, without methods.
struct S_mission_options
{
    float lod_quality;                  // 0x0, 0.4 to 2
    float unknown_4;                    // 0x4, 0.3 to 1
    float unknown_8;                    // 0x8, the camera's far range (C_mission::UpdateSettings)
    int unknown_c;                      // 0xc, 0 to 2
    int unknown_10;                     // 0x10, 0 to 2
    int unknown_14;                     // 0x14, one of 0, 1, 3, 6, 9 and 15, set and read by its index
    int unknown_18;                     // 0x18, 0 to 2

    void SetLodQuality(float quality);
    void SetUnknown4(float value);
    void SetUnknownC(int value);
    void SetUnknown10(int value);
    void SetUnknown14(int index);
    int GetUnknown14();
    // Inline: main copies the floats into the settings through the FPU (fld, fstp), where the members read directly
    // are copied with integer moves
    float GetLodQuality() const { return lod_quality; }
    float GetUnknown4() const { return unknown_4; }
    void SetUnknown18(int value);
};

// The loaded mission (PS2 Mission.cpp): its scene, actors and game. Only what main uses so far.
class C_mission
{
public:
    uchar unknown_0[0x24];
    C_game *game;                       // 0x24 (PS2 0x20)
    uchar unknown_28[0x48];
    int unknown_70;                     // 0x70, cleared after each mission is opened (PS2 0xca0)
    int unknown_74;                     // 0x74 (PS2 0xca4)
    uchar unknown_78[0x18];
    S_mission_options options;          // 0x90 (PS2 0xcc0)

    // Opens the mission of that name, a folder in missions\; 0 if it succeeds
    int Open(const char *name, bool, uint, bool);
    void Close();
    // The name is ours. main clears both members through one load of act_mission, so through a method: written out,
    // VC6 reloads the pointer after the first store.
    void ResetUnknown70()
    {
        unknown_70 = 0;
        unknown_74 = 0;
    }
};

extern C_mission *act_mission;
