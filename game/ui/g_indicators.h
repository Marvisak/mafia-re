#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

class ITexture;
class I3D_camera;
class C_car;
class C_web_node;
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
// 0x4000, 0x8000 and 0x20000 while it runs
#define INDF_MENU 1                     // set while GM_Menu::ExecuteMenu runs a menu
#define INDF_UNKNOWN_20 0x20            // cleared while the camera is CAMERA_CP_SNIPER, where INDF_UNKNOWN_20000 is set
#define INDF_UNKNOWN_800 0x800          // cleared by C_cutscene::Close
#define INDF_UNKNOWN_4000 0x4000
#define INDF_UNKNOWN_8000 0x8000
#define INDF_UNKNOWN_20000 0x20000
#define INDF_RACE_START 0x200000        // RaceSetStartFlag's flag shows

// The lines of the HUD's console (the name is ours; PS2's has 3)
#define CONSOLE_LINES 5

// RaceSetStartFlag's flags that hide it (the names are ours, by value where the meaning isn't known yet)
#define RACE_START_UNKNOWN_4 4
#define RACE_START_NONE 0xffffffff

// OutText's flags (the names are ours)
#define OUTTEXT_UNDERLINE 0x1           // a red line under the text
#define OUTTEXT_CENTER 0x2              // centered in the width
#define OUTTEXT_RIGHT 0x4               // aligned to the width's right end
#define OUTTEXT_INDENT 0x8              // half the size from the left
#define OUTTEXT_FIT 0x10                // narrowed to the width (less the indent)

// The quads OutText draws in one call (the name is ours)
#define OUTTEXT_MAX_CHARS 100

// A character of a HUD font (the names are ours), 0x10 bytes: its rectangle in the font's texture, the bottom
// G_Font::tex_height below the top, and its width relative to the font's
struct G_FontChar
{
    float u, v;                         // the top left corner
    float u1;                           // 0x8, the right edge
    float width;                        // 0xc, under .001 for no character
};

// A HUD font, tables\font<n><suffix>.gmf (the names are ours), 0x101c bytes: the scales relative to the text's size
struct G_Font
{
    uchar unknown_0[8];
    float height;                       // 0x8, the characters' height
    float width;                        // 0xc, the scale of their widths, relative to the height
    float tex_height;                   // 0x10, their height in the texture
    float spacing;                      // 0x14, what each character overlaps the next
    float top;                          // 0x18, the characters' top above the text's position
    G_FontChar chars[0x100];            // 0x1c, by the font's character code (fontTranstable's)
};

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
        C_car *car;                     // RadarAddCar's (the names are ours)
        uint color;                     // 0x4
    };
    struct G_RadarItem
    {
        uchar unknown_0[8];
    };

    G_Font fonts[NUM_FONTS];            // 0x0, LoadFont's tables\font<n><suffix>.gmf
    ITexture *font_textures[NUM_FONTS]; // 0x4070, font<n><suffix>.tga
    ITexture *texture_int;              // 0x4080, 1int.tga
    // 0x4084, the dials: 2int.tga, 2intmph.tga with the profile's speedometer in miles, at night 2intnoc.tga and
    // 2intnocmph.tga (GameInit's); cleared by Create (PS2's too), released by Destroy
    ITexture *texture_int2;
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
    // The HUD's console (the names are ours, after ConsoleAddText at 0x5f9d50; PS2's has three lines at 0x4278): a ring
    // of lines, the oldest at console_first
    char console_texts[CONSOLE_LINES][0x40];    // 0x427c
    uint console_colors[CONSOLE_LINES]; // 0x43bc, 0xrrggbb
    float console_times[CONSOLE_LINES]; // 0x43d0, 5 when added
    uint console_first;                 // 0x43e4, cleared by ResetValues
    uint console_count;                 // 0x43e8, cleared by ResetValues and ConsoleClear
    std::vector<G_MISubtitleStruct> subtitles;      // 0x43ec (PS2 0x4b38), SubtitlesClear's (0x5f8990)
    std::vector<G_MISubtitleStruct> mission_help;   // 0x43fc (PS2 0x4b44), MissionHelpClear's (0x5f99b0)
    std::vector<G_MIIntroSubtitles> intro_subtitles;    // 0x440c (PS2 0x4b50)
    std::vector<G_MIIntroSubtitlesWaiting> intro_subtitles_waiting; // 0x441c (PS2 0x4b5c)
    uchar unknown_442c[0x2c];
    float unknown_4458;                 // 0x4458, .5 from ResetValues (PS2 0x4b98)
    uchar unknown_445c[0x24];
    uint unknown_4480;                  // 0x4480, cleared by ResetValues, as the three below (PS2 0x4bc0 to 0x4bd0)
    uint unknown_4484;                  // 0x4484
    uint unknown_4488;                  // 0x4488, SetUnknown4484's second value
    uint unknown_448c;                  // 0x448c, SetUnknownTexture's name
    uint unknown_4490;                  // 0x4490, SetUnknownTexture's texture
    uchar unknown_4494[4];
    std::vector<G_RadarCar> radar_cars; // 0x4498 (PS2 0x4bd8)
    uint unknown_44a8;                  // 0x44a8, cleared by ResetValues (PS2 0x4be4)
    std::vector<G_RadarItem> radar_items;   // 0x44ac (PS2 0x4be8)
    uint unknown_44bc;                  // 0x44bc, cleared by ResetValues (PS2 0x4bf4)
    float unknown_44c0;                 // 0x44c0, .9 from ResetValues (PS2 0x4bf8)
    // RaceSetStartFlag's (PS2 0x4bfc to 0x4c08): the flag before and its float, the flag and its float, 1 when set
    uint race_start_flag_last;          // 0x44c4
    uint race_start_flag;               // 0x44c8
    float race_start_last_time;         // 0x44cc, cleared by ResetValues
    float race_start_time;              // 0x44d0
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
    bool map_enabled;                   // 0x46ec, MapEnable's (PS2 0x4e24), cleared by ResetValues
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
    // language_primary's two letters, the first in the low byte (PC only, the name is ours)
    uint GetLanguage();
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
    // PC only, the names are ours: the two values at 0x4484, and a texture loaded by name into 0x4490 unless that name's
    // is there already (reload loads it again), released for NULL; C_cutscene::Close clears both
    void SetUnknown4484(uint a, uint b);
    void SetUnknownTexture(const char *name, bool unknown, bool reload);
    // The bonus the HUD shows, BONUS_NONE for none
    void BonusSetCount(int count);
    // PS2 names, what C_game's Init and Done call (PS2's GameDone takes a bool). GameInit loads the dials' textures for
    // the game's day or night, GameDone frees what the game used.
    void GameInit();
    void GameDone();
    // Forgets the radar's cars and things
    void RadarClear();
    void RadarAddCar(C_car *car, uint color);
    void RadarRemoveCar(C_car *car);
    // The road's edges the radar draws, from a vector of web nodes; NULL for none (PS2 names)
    void RadarSetLeftEdge(std::vector<C_web_node *> *edge);
    void RadarSetRightEdge(std::vector<C_web_node *> *edge);
    // The race's start lights, RACE_START_UNKNOWN_4 and RACE_START_NONE hiding them (PS2 name)
    void RaceSetStartFlag(uint flag);
    void SubtitlesClear();
    // Empties the HUD's console
    void ConsoleClear();
    // Whether the city map may be shown (PS2 name), which also takes text 0x5f again
    void MapEnable(bool enable);
    // A text's width in a font at a size, up to stop if not NULL (PS2 name)
    float TextSize(const char *text, float size, uint font, const char *stop);
    // Writes a text at x, y in a font, up to stop if not NULL, the characters sy high: OUTTEXT_* place it in the width
    // sx (PS2 name). Returns where it ends.
    float OutText(const char *text, float x, float y, float sx, float sy, uint color, uint flags, uint font,
        const char *stop);
    // The name is ours: its copy is among the accessors at the end of the file at 0x460a50
    void SetUnknown40a4(uint flags)
    {
        unknown_40a4 = flags;
    }
    // Lays the HUD out for the screen's resolution: Parhelia's screens, the scale and the dials' vertices
    void ResolutionChanged();
    // The mouse's cursor, which GM_Menu::Draw draws over the active menu: a square of 15 pixels from 1int.tga (PC only,
    // the name is ours)
    void DrawCursor(float x, float y);
    // Gives the camera its field of view and the screen's aspect (PC only, the name is ours): with Parhelia's two or
    // three screens, the view of all of them (twice or 1.6 times the angle, at most 0.98 PI) or, with
    // ParheliaSetCenter, of the middle one
    void SetCameraFOV(I3D_camera *cam, float fov);
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
