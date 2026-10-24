#pragma once

#include "types.h"

#define SETTINGS_MAGIC 0x14

#pragma pack(push, 1)
struct S_settings
{
    // Signed: Steam's Czech setup, built without optimization, sign-extends it for LoadSettings' check
    char magic;
    char adapter;
    int width;
    int height;
    int bit_depth;
    int refresh_rate;
    char antialiasing;
    bool multipass_rendering;
    bool clip_always;
    bool fullscreen;
    bool hardware_transform;
    bool vsync;
    bool texture_low_detail;
    bool triple_buffering;
    bool lightmap_true_color;
    bool texture_compressed;
    bool texture_true_color;
    bool disable_sound;
    bool disable_eax;
    bool suspend_when_inactive;
    bool software_mixing_only;
    // Signed: both setups sign-extend it
    char sound_type;
    int language;
    bool w_buffer;

    // The game's video options, which setup only initializes: the game hands them to its mission's options at its start
    // and saves them back when the player quits (main). Named after the game's Audio & Video menu (tables\menu\
    // optionsa.mnu, and the texts of textdb_en.def), whose choices the ints are: Low, Medium and High, and for the
    // shadows Very Low, Low, Medium, High, Very High and Highest.
    float level_of_detail;          // the engine's RS_LOD_QUALITY, 0.4 to 2
    float draw_distance;            // the part of the scene's camera range, 0.3 to 1
    int game_effects;               // "In Game Effects"
    int game_sounds;                // "In Game Sounds"
    int shadows;
    int particles;

    // The defaults, in declaration order. setup.exe 1.2 constructs its global S_settings and every local
    // copy with them: the global has a dynamic initializer, and no InitSettings function exists (each local
    // gets one set of default stores, never a second). Game.exe's main calls an out-of-line copy (for LoadSettings'
    // buffer), and Main.cpp's global gets the stores inlined in its dynamic initializer.
    // FUNCTION: SETUPSTEAM 0x401000
    // FUNCTION: GAME 0x5bf520
    S_settings()
    {
        magic = SETTINGS_MAGIC;
        adapter = 0;
        width = 800;
        height = 600;
        bit_depth = 32;
        refresh_rate = 0;
        antialiasing = 0;
        multipass_rendering = false;
        clip_always = false;
        fullscreen = false;
        hardware_transform = true;
        vsync = false;
        texture_low_detail = false;
        triple_buffering = false;
        lightmap_true_color = true;
        texture_compressed = true;
        texture_true_color = false;
        disable_sound = false;
        disable_eax = true;
        suspend_when_inactive = true;
        software_mixing_only = false;
        sound_type = 1;
        language = 0;
        w_buffer = true;
        level_of_detail = 1.0;
        draw_distance = 1.0;
        game_effects = 2;
        game_sounds = 2;
        shadows = 3;
        particles = 2;
    }
};
#pragma pack(pop)