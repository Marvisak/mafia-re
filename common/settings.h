#pragma once

#include "types.h"

#define SETTINGS_MAGIC 0x14

// S_settings::sound_type: setup's sound box, in its order (the names are ours)
#define SOUND_TYPE_HEADPHONES 0
#define SOUND_TYPE_STEREO 1
#define SOUND_TYPE_QUAD 2
#define SOUND_TYPE_SURROUND 3           // 5.1

// S_settings::language: the item data of setup's language box (the names are ours). The game takes its language from
// tables\version.txt instead.
#define LANGUAGE_ENGLISH 0
#define LANGUAGE_CZECH 1
#define LANGUAGE_GERMAN 2

// The game's options game_effects, game_sounds and particles: the Audio & Video menu's Low, Medium and High (the names
// are ours)
enum {
    OPTION_LOW,
    OPTION_MEDIUM,
    OPTION_HIGH,
};

// S_settings::shadows: the menu's choices (the names are ours). S_mission_options::shadows holds the level each one
// stands for (SetShadows).
enum {
    SHADOWS_VERY_LOW,
    SHADOWS_LOW,
    SHADOWS_MEDIUM,
    SHADOWS_HIGH,
    SHADOWS_VERY_HIGH,
    SHADOWS_HIGHEST,
    NUM_SHADOWS,
};

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
    char antialiasing;              // 0, or the D3DMULTISAMPLE_TYPE setup checked (2 or 4 samples)
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
    char sound_type;                // SOUND_TYPE_*
    int language;                   // LANGUAGE_*
    bool w_buffer;

    // The game's video options, which setup only initializes: the game hands them to its mission's options at its start
    // and saves them back when the player quits (main). Named after the game's Audio & Video menu (tables\menu\
    // optionsa.mnu, and the texts of textdb_en.def), whose choices the ints are: Low, Medium and High, and for the
    // shadows Very Low, Low, Medium, High, Very High and Highest.
    float level_of_detail;          // the engine's RS_LOD_QUALITY, 0.4 to 2
    float draw_distance;            // the part of the scene's camera range, 0.3 to 1
    int game_effects;               // "In Game Effects", OPTION_*
    int game_sounds;                // "In Game Sounds", OPTION_*
    int shadows;                    // SHADOWS_*
    int particles;                  // OPTION_*

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
        sound_type = SOUND_TYPE_STEREO;
        language = LANGUAGE_ENGLISH;
        w_buffer = true;
        level_of_detail = 1.0;
        draw_distance = 1.0;
        game_effects = OPTION_HIGH;
        game_sounds = OPTION_HIGH;
        shadows = SHADOWS_HIGH;
        particles = OPTION_HIGH;
    }
};
#pragma pack(pop)