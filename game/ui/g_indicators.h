#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

class ITexture;
struct TLVERTEX;

// The HUD's fonts (the name is ours)
#define NUM_FONTS 4

// The wingmen PlayerSetWingmanName and PlayerSetWingmanLives take (the name is ours)
#define MAX_WINGMEN 4

// BonusSetCount's count that shows no bonus (the name is ours)
#define BONUS_NONE 0x7fffffff

// parheliaIndex: Matrox Parhelia's screens, told from the screen's shape (the names are ours)
#define PARHELIA_NONE 0
#define PARHELIA_DUAL 1                 // wider than 2:1
#define PARHELIA_TRIPLE 2               // wider than 3:1: the HUD is on the middle screen

// G_IndicatorsClass::unknown_40a4's bits (the names are ours, by value where the meaning isn't known yet): a menu keeps
// the three unknown ones while it runs
#define INDF_MENU 1                     // set while GM_Menu::ExecuteMenu runs a menu
#define INDF_UNKNOWN_4000 0x4000
#define INDF_UNKNOWN_8000 0x8000
#define INDF_UNKNOWN_20000 0x20000

// A needle of the HUD's dials (PS2's name; the members' names are ours), 0x2c bytes: the angles in radians
struct G_RafickaDef
{
    float angle;                        // at the scale's start
    float angle_range;                  // 0x4
    float min;                          // 0x8, the value at the scale's start
    float range;                        // 0xc
    float x, y;                         // 0x10, the needle's axis on the dial
    float tip, tail;                    // 0x18, its ends' distances from the axis, the tail's negative
    float tip_width, tail_width;        // 0x20, its half widths there
    uint color;                         // 0x28
};

// A needle's value and the corners _SpocitejRafiku2 computes from it on the screen, left and right as the needle points
// up (PS2's name; the members' names are ours). Not an array: the original's constructor of the HUD constructs none.
struct G_RafickaValue
{
    float value;
    S_vector2 tail_left;                // 0x4
    S_vector2 tip_left;                 // 0xc
    S_vector2 tail_right;               // 0x14
    S_vector2 tip_right;                // 0x1c
};

// A subtitle's line (PS2's name; its members are unknown), 0x10 bytes, as the mission's help texts have them
struct G_MISubtitleStruct
{
    uchar unknown_0[0x10];
};

// An intro's subtitle (PS2's name; its members are unknown), 0x1c bytes
struct G_MIIntroSubtitles
{
    uchar unknown_0[0x1c];
};

// One waiting to be shown (PS2's name; its members are unknown), 8 bytes
struct G_MIIntroSubtitlesWaiting
{
    uchar unknown_0[8];
};

// The HUD (PS2 G_IndicatorsClass, g_indicators.cpp; mafiaIndicators is 0x4e38 bytes there). Only what main and the
// profiles use so far; the textures' names are ours, after their files, and so are the other members' (PS2's offsets in
// the comments where its functions show them). Its destructor is the implicit one, the copy after the global's
// initializers (0x5f6a80), as PS2's is the implicit one at its file's end. Open: the next global after it
// (fontTranstable) is 0x10 bytes past the flag at 0x46f4.
class G_IndicatorsClass
{
public:
    // A car and a thing on the radar (PS2's names; their members are unknown), 8 bytes each
    struct G_RadarCar
    {
        uchar unknown_0[8];
    };
    struct G_RadarItem
    {
        uchar unknown_0[8];
    };

    uchar fonts[NUM_FONTS][0x101c];     // 0x0, LoadFont's tables\font<n><suffix>.gmf
    ITexture *font_textures[NUM_FONTS]; // 0x4070, font<n><suffix>.tga
    ITexture *texture_int;              // 0x4080, 1int.tga
    ITexture *unknown_4084;             // 0x4084, cleared by Create (PS2's too), released by Destroy
    ITexture *texture_numbers;          // 0x4088, 1numbers.tga
    ITexture *texture_textura;          // 0x408c, 0textura.tga
    ITexture *unknown_4090;             // 0x4090, cleared by Create (PS2's too), released by Destroy
    ITexture *texture_logo;             // 0x4094, logoplay.bmp
    ITexture *texture_sight;            // 0x4098, 2sight.bmp with its alpha
    ITexture *texture_race_start;       // 0x409c, racestart.bmp
    ITexture *texture_race_wrong;       // 0x40a0, racewrong.bmp
    // INDF_*. Cleared by ResetValues and by G_LoadSaveClass::Init after ResolutionChanged, as PS2's 0x3094
    uint unknown_40a4;                  // 0x40a4
    // GM_Menu::ExecuteMenu pushes unknown_40a4 here while its menu runs
    std::vector<uint> unknown_40a8;     // 0x40a8, PS2's vector<unsigned int>
    S_vector2 screen_size;              // 0x40b8
    S_vector2 screen_scale;             // 0x40c0, the HUD's screen relative to 800x600
    S_vector2 hud_size;                 // 0x40c8, the size of the HUD's screen: the middle one with Parhelia's three
    uchar unknown_40d0[0xc];
    S_vector2 timer_pos;                // 0x40dc, the clock's dial
    uchar unknown_40e4[8];
    // The clock's hands; TimerSetTime and TimerSetInterval set their values to -1, which no time has, so the hands are
    // computed again
    G_RafickaValue timer_hours;         // 0x40ec
    G_RafickaValue timer_minutes;       // 0x4110
    G_RafickaValue timer_seconds;       // 0x4134
    uint timer_hour;                    // 0x4158, TimerSetTime's time (PS2 0x4154)
    uint timer_minute;                  // 0x415c
    uint timer_second;                  // 0x4160
    // TimerSetInterval's: where the minute and the second hand are at the interval's end, as fractions of a turn
    float interval_minute;              // 0x4164 (PS2 0x4160)
    float interval_second;              // 0x4168
    uchar unknown_416c[4];
    uint interval;                      // 0x4170, TimerSetInterval's, in milliseconds (PS2 0x416c)
    G_RafickaValue tachometr_speed;     // 0x4174, in km/h
    G_RafickaValue tachometr_revs;      // 0x4198
    G_RafickaValue tachometr_fuel;      // 0x41bc
    uchar unknown_41e0[4];
    float unknown_41e4;                 // 0x41e4, the texture's column of the tachometer's small display
    float gear_column;                  // 0x41e8, TachometrSetGear's: the gear's column in the texture (PS2 0x41e4)
    S_vector2 hud_scale;                // 0x41ec, the same as screen_scale
    S_vector2 tachometr_pos;            // 0x41f4, its dial's
    S_vector2 revs_pos;                 // 0x41fc, the revs' dial's
    bool unknown_4204;                  // 0x4204, TachometrSetGear's flag (PS2 0x41f8)
    bool fuel_low;                      // 0x4205, set by TachometrSetValues while the fuel is under 2%
    uint unknown_4208;                  // 0x4208, cleared by ResetValues and by TachometrSetValues (PS2 0x41fc)
    uchar unknown_420c[4];
    // The player's (PS2 0x4204 to 0x4230)
    uint lives;                         // 0x4210
    uint ammo;                          // 0x4214, the weapon's rounds
    uint ammo_spare;                    // 0x4218, the others
    char lives_text[0x10];              // 0x421c
    char ammo_text[0x10];               // 0x422c, "<ammo>/<spare>"
    float accuracy;                     // 0x423c
    uchar unknown_4240[0xc];
    // The wingmen's (PS2 0x4244 and 0x4254)
    uint wingman_names[MAX_WINGMEN];    // 0x424c, 0 to 7
    uint wingman_lives[MAX_WINGMEN];    // 0x425c
    S_vector2 screen_center;            // 0x426c
    uchar unknown_4274[4];
    uint unknown_4278;                  // 0x4278, cleared by ResetValues
    uchar unknown_427c[0x43e4 - 0x427c];
    uint unknown_43e4;                  // 0x43e4, cleared by ResetValues
    uint unknown_43e8;                  // 0x43e8
    std::vector<G_MISubtitleStruct> subtitles;      // 0x43ec (PS2 0x4b38), SubtitlesClear's (0x5f8990)
    std::vector<G_MISubtitleStruct> mission_help;   // 0x43fc (PS2 0x4b44), MissionHelpClear's (0x5f99b0)
    std::vector<G_MIIntroSubtitles> intro_subtitles;    // 0x440c (PS2 0x4b50)
    std::vector<G_MIIntroSubtitlesWaiting> intro_subtitles_waiting; // 0x441c (PS2 0x4b5c)
    uchar unknown_442c[0x2c];
    float unknown_4458;                 // 0x4458, .5 from ResetValues (PS2 0x4b98)
    uchar unknown_445c[0x24];
    uint unknown_4480;                  // 0x4480, cleared by ResetValues, as the three below (PS2 0x4bc0 to 0x4bd0)
    uint unknown_4484;                  // 0x4484
    uchar unknown_4488[4];
    uint unknown_448c;                  // 0x448c
    uint unknown_4490;                  // 0x4490
    uchar unknown_4494[4];
    std::vector<G_RadarCar> radar_cars; // 0x4498 (PS2 0x4bd8)
    uint unknown_44a8;                  // 0x44a8, cleared by ResetValues (PS2 0x4be4)
    std::vector<G_RadarItem> radar_items;   // 0x44ac (PS2 0x4be8)
    uint unknown_44bc;                  // 0x44bc, cleared by ResetValues (PS2 0x4bf4)
    float unknown_44c0;                 // 0x44c0, .9 from ResetValues (PS2 0x4bf8)
    uchar unknown_44c4[8];
    uint unknown_44cc;                  // 0x44cc, cleared by ResetValues (PS2 0x4c04)
    uint unknown_44d0;                  // 0x44d0
    // The race's texts (PS2 0x4c0c to 0x4d0c): RaceSetTime's, RaceSetLapTime's, RaceSetBestTime's, RaceSetPosition's
    // and RaceSetLaps'
    char race_time[0x40];               // 0x44d4
    char race_lap_time[0x40];           // 0x4514
    char race_best_time[0x40];          // 0x4554
    char race_position[0x40];           // 0x4594
    char race_laps[0x40];               // 0x45d4
    uchar unknown_4614[0x4698 - 0x4614];
    int bonus_count;                    // 0x4698, BonusSetCount's (PS2 0x4dd0)
    char bonus_text[0x40];              // 0x469c, the count as text 0xb3 ("$%d") formats it
    uint unknown_46dc;                  // 0x46dc, cleared by ResetValues (PS2 0x4e14)
    uint unknown_46e0;                  // 0x46e0
    int unknown_46e4;                   // 0x46e4, -1 from ResetValues
    uchar unknown_46e8[4];
    bool unknown_46ec;                  // 0x46ec, cleared by ResetValues (PS2 0x4e24)
    char *text_out_of_city;             // 0x46f0, text 0x5f, "Out of City Limits" (PS2 0x4e28)
    bool parhelia_center;               // 0x46f4, ParheliaSetCenter's flag

    // The vectors and the quads' indices; the textures are set by Create
    G_IndicatorsClass();
    // Its state at the start of a game (PS2's name)
    void ResetValues();
    // Loads one of the fonts for the primary language or, with credits, the Czech one (PS2's name)
    void LoadFont(bool credits, uint font);
    // Reads the language's tables\lang_<language>.def into language_primary ('en' for 0) and tables\lang_cz.def into
    // language_credits
    void LanguageSelect(uint language);
    // Creates the HUD at the start (PS2's names, as Destroy's and ResolutionChanged's): the textures and the fonts;
    // G_LoadSaveClass::Init calls it
    void Create();
    // Frees what Create made, at CloseSystem
    void Destroy();
    // The clock's time, hours, minutes and seconds
    void TimerSetTime(uint hour, uint minute, uint second);
    // The interval to show on the clock, in seconds, from its time
    void TimerSetInterval(uint interval);
    // A needle's corners for its value, on the dial at pos (PS2's name: "compute the needle")
    void _SpocitejRafiku2(G_RafickaDef *def, G_RafickaValue *value, S_vector2 &pos, TLVERTEX *verts);
    // The gear the tachometer's small display shows
    void TachometrSetGear(int gear, bool flag);
    // The needles and the odometer: the speed in metres per second (shown in km/h), the revs and the fuel from 0 to 1,
    // the odometer's distance
    void TachometrSetValues(float speed, float revs, float fuel, float odometer);
    // The player's health, ammo and accuracy
    void PlayerSetLives(uint lives);
    void PlayerSetAmmo(uint ammo, uint spare);
    void PlayerSetAccuracy(float accuracy);
    // A wingman's name (0 to 7) and health
    void PlayerSetWingmanName(uint wingman, uint name);
    void PlayerSetWingmanLives(uint wingman, uint lives);
    // Forgets the intro's subtitles
    void IntroSubtitlesClear();
    // The black bars above and below the picture, the picture's height a ratio of the screen's width
    void WideScreenSetRatio(float ratio);
    // The bonus the HUD shows, BONUS_NONE for none
    void BonusSetCount(int count);
    // Lays the HUD out for the screen's resolution: Parhelia's screens, the scale and the dials' vertices
    void ResolutionChanged();
    // PC only, the name is ours. Stores the flag at 0x46f4 and, with Matrox Parhelia's two or three screens
    // (parheliaIndex), confines the scene's viewport and camera to the middle one while it is set; main sets it around
    // the car encyclopedia.
    void ParheliaSetCenter(bool center);
};

extern G_IndicatorsClass mafiaIndicators;

// A language's table (lang_xx.def; the global's names are PS2's, the members' ours), 0x238 bytes
struct S_language
{
    char magic[4];                      // "Lang"
    char language[4];                   // 0x4, its two letters
    char name[0x20];                    // 0x8, "English"
    char font_suffix[8];                // 0x28, of the fonts' files, "_lat"
    uchar unknown_30[8];
    ushort chars[0x100];                // 0x38, the fonts' character for each byte of a text
};

extern S_language language_primary;
// The Czech table, whatever the language (PS2's name has it for the credits)
extern S_language language_credits;
// Each font's character table, language_primary's or language_credits' chars (PS2's name)
extern ushort *fontTranstable[NUM_FONTS];

// The HUD's textures that aren't members (PS2's names)
extern ITexture *textureCrew;           // 2blood.bmp with its alpha
extern ITexture *textureRadarClip;      // radarclp.bmp
extern ITexture *textureBenzin;         // 2intbenal.tga
extern ITexture *textureIcons;          // PS2's ukakri.bmp, which the PC doesn't load

// The screens' layout (PS2's names): PARHELIA_*, and the HUD's screen's right and left edges (with its bottom and top)
extern int parheliaIndex;
extern S_vector2 pravyOkraj;
extern S_vector2 levyOkraj;

// The engine's S_vector2 operators (I3d_math.h), which ResolutionChanged calls out of line where its budget runs out
// LIBRARY: GAME 0x5f7640
// ??DS_vector2@@QBE?AU0@ABU0@@Z

// LIBRARY: GAME 0x5f7660
// ??HS_vector2@@QBE?AU0@ABU0@@Z

// LIBRARY: GAME 0x6025d0
// ??KS_vector2@@QBE?AU0@M@Z
