#pragma once

#include "Main.h"

class C_game;

// The game's video options (the struct's name is ours), the controls of the Audio & Video menu (tables\menu\
// optionsa.mnu), which give the members their names. main hands each the value setup.exe saved, through a setter that
// clamps it, and saves them back when the player quits. PS2's C_mission has its draw distance and far range at 0xcc0,
// without methods.
struct S_mission_options
{
    float level_of_detail;              // 0x0, 0.4 to 2: the engine's RS_LOD_QUALITY
    float draw_distance;                // 0x4, 0.3 to 1: the part of far_range the camera sees
    float far_range;                    // 0x8, the scene's camera range, which Open stores
    int game_effects;                   // 0xc, "In Game Effects": Low, Medium or High (0 to 2)
    int game_sounds;                    // 0x10, "In Game Sounds": Low, Medium or High
    int shadows;                        // 0x14, 0, 1, 3, 6, 9 or 15 for the menu's Very Low to Highest
    int particles;                      // 0x18, Low, Medium or High

    void SetLevelOfDetail(float level);
    void SetDrawDistance(float distance);
    void SetGameEffects(int level);
    void SetGameSounds(int level);
    // By the menu's choice, 0 to 5
    void SetShadows(int index);
    // The menu's choice; the game's code compares it too
    int GetShadows();
    // Inline: main copies the floats into the settings through the FPU (fld, fstp), where the members read directly
    // are copied with integer moves
    float GetLevelOfDetail() const { return level_of_detail; }
    float GetDrawDistance() const { return draw_distance; }
    void SetParticles(int level);
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
