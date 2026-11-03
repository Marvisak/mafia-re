#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

class ITexture;
class I3D_camera;
class C_car;
class C_web_node;
class I3D_frame;
struct TLVERTEX;
struct tDynamicCollObject;

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

// G_IndicatorsClass::show_flags's bits, what DrawAll draws (the names are ours): a menu keeps INDF_WIDESCREEN, INDF_FADE
// and INDF_SNIPER while it runs. INDF_MENU and INDF_CUTSCENE hide most of the HUD.
#define INDF_MENU 1                     // set while GM_Menu::ExecuteMenu runs a menu
#define INDF_TACHOMETR 2                // the tachometer
#define INDF_PLAYER 4                   // the player's health and the wingmen's
#define INDF_TIMER 8                    // the clock
#define INDF_PROGRESS 0x10              // the scripts' PM_SHOWPROGRESS (the police's too): a bar for progress, under
                                        // INDF_SYMBOL
#define INDF_CROSSHAIR 0x20             // cleared while the camera is CAMERA_CP_SNIPER, where INDF_SNIPER is set
#define INDF_COMPASS 0x80              // the arrow to compass_destination, and its mark on the radar
// The tachometer's light at its top right: set with the player's car's speed limit (C_Vehicle::SetSpeedLimit, PS2's
// name) when CTRL_SPEED_LIMITER turns it on (0x591f4a)
#define INDF_SPEED_LIMITER 0x100
#define INDF_CUTSCENE 0x800             // set by C_recorder::RecLoadFull, cleared by C_recorder::Close
#define INDF_THIEF 0x1000               // PS2's ThiefIndicatorSetValue's bar
#define INDF_MAP 0x2000                 // the city map, where map_enabled
#define INDF_WIDESCREEN 0x4000          // WideScreenSetRatio's bars
#define INDF_FADE 0x8000                // PS2's FadeInOut sets it: the screen in fade_color
#define INDF_CAR_DAMAGE 0x10000         // the scripts' CARDAMAGEVISIBLE's bar for car_damage
#define INDF_SNIPER 0x20000             // the sniper's scope
#define INDF_REFULLING 0x40000          // PS2's RefullingIndicatorSetValue's bar
#define INDF_RADAR 0x80000
#define INDF_RACE 0x100000              // the race's texts
#define INDF_RACE_START 0x200000        // RaceSetStartFlag's flag shows
#define INDF_RACE_FLASH 0x400000        // PS2's RaceFlashText's text, race_flash_text
#define INDF_RACE_WRONG 0x800000        // racewrong.bmp
#define INDF_MISSION_HELP 0x1000000     // the mission's help texts, cleared by DrawAll when there are none
#define INDF_CREW 0x2000000             // the blood over the screen, crew_quads of _VtxCrew
// The scripts' PM_SHOWSYMBOL (the police's too): a part of 0textura.tga, symbol_index's, in symbol_color's colour
#define INDF_SYMBOL 0x4000000
#define INDF_AMMO 0x8000000             // the weapon's ammo, with INDF_PLAYER
#define INDF_WINGMAN1 0x10000000        // the wingmen, with INDF_PLAYER
#define INDF_WINGMAN2 0x20000000
#define INDF_WINGMAN3 0x40000000
#define INDF_WINGMAN4 0x80000000


// The lines of the HUD's console (the name is ours; PS2's has 3)
#define CONSOLE_LINES 5

// RaceSetStartFlag's flags that hide it (the names are ours): the countdown's step after its last light (0x5507c0
// counts the flags 0 to 3 a second each and starts the race at 4), and none
#define RACE_START_DONE 4
#define RACE_START_NONE 0xffffffff

// OutText's flags (the names are ours)
#define OUTTEXT_UNDERLINE 0x1           // a red line under the text
#define OUTTEXT_CENTER 0x2              // centered in the width
#define OUTTEXT_RIGHT 0x4               // aligned to the width's right end
#define OUTTEXT_INDENT 0x8              // half the size from the left
#define OUTTEXT_FIT 0x10                // narrowed to the width (less the indent)
#define OUTTEXT_UNKNOWN_20 0x20         // DrawAll passes it, OutText doesn't read it (PS2's neither)

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

// A subtitle's line (PS2's name; the members' names are ours), 0x10 bytes, as the mission's help texts have them:
// DrawAll writes the text up to end, unless hidden, the last one at the bottom
struct G_MISubtitleStruct
{
    const char *text;
    const char *end;                    // 0x4, OutText's stop
    uint hidden;                        // 0x8
    uint alpha;                         // 0xc, over 0xff opaque
};

// An intro's subtitle (PS2's name; the members' names are ours), 0x1c bytes: placed for 800x600 from the HUD's left
// edge, as transparent as the less opaque of its two alphas
struct G_MIIntroSubtitles
{
    float x, y;
    float size;                         // 0x8, the characters' height
    const char *text;                   // 0xc
    const char *end;                    // 0x10, OutText's stop
    float alpha_in;                     // 0x14
    float alpha_out;                    // 0x18
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
    // The radar's car, the player's (PS2 name)
    void RadarSetPlayerCar(C_car *car);
    // A car and a thing on the radar (PS2's names; their members are unknown), 8 bytes each
    struct G_RadarCar
    {
        C_car *car;                     // RadarAddCar's (the names are ours)
        uint color;                     // 0x4
    };
    struct G_RadarItem
    {
        tDynamicCollObject *object;     // PS2's RadarAddObject's (the names are ours)
        uint color;                     // 0x4
    };

    G_Font fonts[NUM_FONTS];            // 0x0, LoadFont's tables\font<n><suffix>.gmf
    ITexture *font_textures[NUM_FONTS]; // 0x4070, font<n><suffix>.tga
    ITexture *texture_int;              // 0x4080, 1int.tga
    // 0x4084, the dials: 2int.tga, 2intmph.tga with the profile's speedometer in miles, at night 2intnoc.tga and
    // 2intnocmph.tga (GameInit's); cleared by Create (PS2's too), released by Destroy
    ITexture *texture_int2;
    ITexture *texture_numbers;          // 0x4088, 1numbers.tga
    ITexture *texture_textura;          // 0x408c, 0textura.tga
    // 0x4090, the city map's, which PS2's NahrajMapu ("load the map") loads; cleared by Create (PS2's too), released
    // by Destroy
    ITexture *texture_map;
    ITexture *texture_logo;             // 0x4094, logoplay.bmp
    ITexture *texture_sight;            // 0x4098, 2sight.bmp with its alpha
    ITexture *texture_race_start;       // 0x409c, racestart.bmp
    ITexture *texture_race_wrong;       // 0x40a0, racewrong.bmp
    // INDF_*, what the HUD shows. Cleared by ResetValues and by G_LoadSaveClass::Init after ResolutionChanged, as PS2's
    // 0x3094
    uint show_flags;                    // 0x40a4
    // PushFlags pushes show_flags here, as GM_Menu::ExecuteMenu does while its menu runs
    std::vector<uint> show_flags_stack; // 0x40a8, PS2's vector<unsigned int>
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
    float display_column;               // 0x41e4, the tachometer's small display's, as gear_column (the name is ours)
    float gear_column;                  // 0x41e8, TachometrSetGear's: the gear's column in the texture (PS2 0x41e4)
    S_vector2 hud_scale;                // 0x41ec, the same as screen_scale
    S_vector2 tachometr_pos;            // 0x41f4, its dial's
    S_vector2 revs_pos;                 // 0x41fc, the revs' dial's
    // 0x4204, TachometrSetGear's flag (PS2 0x41f8), which picks the icon's half: the player's car's gearing, manual or
    // automatic (which is which isn't known), C_Vehicle::auto_gearing (0x4cc), which CTRL_GEARING toggles (0x591ece)
    bool gearing_mode;
    bool fuel_low;                      // 0x4205, set by TachometrSetValues while the fuel is under 2%
    // 0x4208 (PS2 0x41fc; the name is a guess: nothing written yet counts it up), cleared by ResetValues and by
    // TachometrSetValues: the low fuel light shows while it is under .25
    float fuel_light_phase;
    // 0x420c, INDF_CAR_DAMAGE's bar, 0 to 1: one less the player's car's C_car::ShowCarEnergy, while
    // CARDAMAGEVISIBLE is on (0x591ea2)
    float car_damage;
    // The player's (PS2 0x4204 to 0x4230)
    uint lives;                         // 0x4210
    uint ammo;                          // 0x4214, the weapon's rounds
    uint ammo_spare;                    // 0x4218, the others
    char lives_text[0x10];              // 0x421c
    char ammo_text[0x10];               // 0x422c, "<ammo>/<spare>"
    float accuracy;                     // 0x423c, the crosshair's spread
    uchar unknown_4240[4];
    // 0x4244, the alpha of the dials' texture's icon above the panel, none for 0 (the name is ours)
    uint panel_icon_alpha;
    uchar unknown_4248[4];
    // The wingmen's (PS2 0x4244 and 0x4254)
    uint wingman_names[MAX_WINGMEN];    // 0x424c, 0 to 7
    uint wingman_lives[MAX_WINGMEN];    // 0x425c
    S_vector2 screen_center;            // 0x426c
    uchar unknown_4274[4];
    // 0x4278, PS2's CompassSetDestination's (PS2 0x4270): the frame INDF_COMPASS's arrow, the radar and the map point to,
    // cleared by ResetValues
    I3D_frame *compass_destination;
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
    uchar unknown_442c[0x28];
    uint crew_quads;                    // 0x4454, the quads of _VtxCrew INDF_CREW draws
    float thief_indicator;              // 0x4458, PS2's ThiefIndicatorSetValue's, .5 from ResetValues (PS2 0x4b98)
    float refulling_indicator;          // 0x445c, PS2's RefullingIndicatorSetValue's (PS2 0x4b9c)
    uchar unknown_4460[0x18];
    uint fade_color;                    // 0x4478, PS2's FadeInOut's (PS2 0x4bb8), with its alpha
    uchar unknown_447c[4];
    // 0x4480, cleared by ResetValues, as the three below (PS2 0x4bc0 to 0x4bd0): INDF_FADE's texture
    ITexture *fade_texture;
    // The cutscenes' fade and picture over the scene, which their track's keys set (0x599d78): SetOverlayFade's and
    // SetOverlayTexture's (the names are ours)
    float overlay_fade;                 // 0x4484, 0 to 1
    uint overlay_color;                 // 0x4488, the fade's colour (0xrrggbb)
    const char *overlay_name;           // 0x448c, the picture's name, the caller's pointer
    ITexture *overlay_texture;          // 0x4490
    // 0x4494, false without a texture: the texture drawn as it is with the fade over it, otherwise in the fade's
    // colour
    bool overlay_opaque;
    uchar unknown_4495[3];
    std::vector<G_RadarCar> radar_cars; // 0x4498 (PS2 0x4bd8)
    C_car *radar_player_car;            // 0x44a8, PS2's RadarSetPlayerCar's, cleared by ResetValues (PS2 0x4be4)
    std::vector<G_RadarItem> radar_items;   // 0x44ac (PS2 0x4be8)
    uint unknown_44bc;                  // 0x44bc, cleared by ResetValues (PS2 0x4bf4)
    float radar_scale;                  // 0x44c0, .9 from ResetValues (PS2 0x4bf8)
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
    // PS2's RaceFlashText's text and alpha (PS2 0x4d4c and 0x4dcc), which INDF_RACE_FLASH shows
    char race_flash_text[0x80];         // 0x4614
    float race_flash_alpha;             // 0x4694
    int bonus_count;                    // 0x4698, BonusSetCount's (PS2 0x4dd0)
    char bonus_text[0x40];              // 0x469c, the count as text 0xb3 ("$%d") formats it
    uint unknown_46dc;                  // 0x46dc, cleared by ResetValues (PS2 0x4e14)
    // SetSymbol's, the scripts' PM_SHOWSYMBOL's, and SetProgress's, PM_SETPROGRESS's
    uint symbol_index;                  // 0x46e0, INDF_SYMBOL's part of 0textura.tga, 1 to 4
    int symbol_color;                   // 0x46e4, its colour, -1 from ResetValues
    float progress;                     // 0x46e8, INDF_PROGRESS's bar, 0 to 1
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
    // The interval in seconds (PS2 name)
    void TimerGetInterval(uint *interval);
    // A needle's corners for its value, on the dial at pos (PS2's name: "compute the needle")
    void _SpocitejRafiku2(G_RafickaDef *def, G_RafickaValue *value, S_vector2 &pos, TLVERTEX *verts);
    // The player's panel, quads added to verts from *count on, which they advance (PS2's names: "add the wingman's
    // background", "the ammo's background", "the wingman's health", "the ammo"): the background of the player (index 0)
    // or a wingman's row, with the name's picture, and their health; the ammo's
    void _PridejWingPodklad(TLVERTEX *verts, uint *count, uint index, uint name);
    void _PridejAmmoPodklad(TLVERTEX *verts, uint *count);
    void _PridejWingLives(TLVERTEX *verts, uint *count, uint index, uint lives);
    void _PridejAmmo(TLVERTEX *verts, uint *count, uint ammo, uint spare);
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
    // A blood stain on the screen at x, y of that size, in 800x600 (PC only, 0x5f9e10; the name is ours), which
    // C_human::Hit adds for each shot on the player
    void AddBloodSplash(float x, float y, float size);
    // An intro's subtitle, the text of that id (PS2 names; the parameters' meaning isn't known)
    void IntroSubtitlesAdd(uint a, uint b);
    void IntroSubtitlesAddSimple(uint id);
    // Forgets the intro's subtitles
    void IntroSubtitlesClear();
    // A line of the HUD's console in the colour (0xrrggbb), the text database's text of that id, if it has one (PS2
    // name)
    void ConsoleAddTextByID(uint id, uint color);
    // The frame INDF_COMPASS points to (PS2 name)
    void CompassSetDestination(I3D_frame *frm);
    // The black bars above and below the picture, the picture's height a ratio of the screen's width
    void WideScreenSetRatio(float ratio);
    // Shows or hides the black bars over time milliseconds, at once for 0 (the name is ours)
    void WideScreenSetOn(bool on, uint time);
    // Fades the screen in fade_color in or out over time milliseconds, at once for 0 (PS2 names): FadeInOutGetState is
    // false while a fade runs, FadeInOutIsDark whether INDF_FADE is set
    void FadeInOut(bool in, uint time, uint color);
    bool FadeInOutGetState();
    bool FadeInOutIsDark();
    // PC only, the names are ours: the cutscenes' fade over the scene, and their picture, a texture loaded by name with
    // the flag (opaque: drawn as it is under the fade), unless the same pointer's is there already (reload loads it
    // again), released for NULL; C_recorder::Close clears both
    void SetOverlayFade(float fade, uint color);
    void SetOverlayTexture(const char *name, bool opaque, bool reload);
    // The bonus the HUD shows, BONUS_NONE for none
    void BonusSetCount(int count);
    // Draws the HUD over the frame (PS2 name), which the scene's callback does after each frame
    void DrawAll();
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
    // The race's start lights, RACE_START_DONE and RACE_START_NONE hiding them (PS2 name)
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
    // The same for a text of several lines (PS2 name)
    void OutTextMultiLine(const char *text, float x, float y, float sx, float sy, uint color, uint flags, uint font);
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
    // (parheliaIndex), confines the scene's viewport and camera to the middle one while it is set (with two the left
    // half), after clearing the screens; otherwise the whole screen. main sets it around the car encyclopedia,
    // C_recorder::Close clears it.
    void ParheliaSetCenter(bool center);

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours): the INDF_*
    // flags set, added, removed and tested, pushed (as ExecuteMenu does) and popped, each returning the flags
    // FUNCTION: GAME 0x47a070
    void SetFlags(uint flags)
    {
        show_flags = flags;
    }
    // FUNCTION: GAME 0x47a080
    void AddFlags(uint flags)
    {
        show_flags |= flags;
    }
    // FUNCTION: GAME 0x47a0a0
    void RemoveFlags(uint flags)
    {
        show_flags &= ~flags;
    }
    // FUNCTION: GAME 0x47a0c0
    bool IsShown(uint flags) const
    {
        return flags & show_flags ? true : false;
    }
    // FUNCTION: GAME 0x47a0e0
    uint PushFlags()
    {
        show_flags_stack.push_back(show_flags);
        return show_flags;
    }
    // FUNCTION: GAME 0x47a2a0
    uint PopFlags()
    {
        if (show_flags_stack.size())
        {
            show_flags = show_flags_stack.back();
            show_flags_stack.pop_back();
        }
        return show_flags;
    }
    // INDF_SYMBOL's part of the texture and its opacity, 0 to 1
    // FUNCTION: GAME 0x47a300
    void SetSymbol(uint symbol, float alpha)
    {
        symbol_index = symbol;
        uint a = (uint)(alpha * 255.0f);
        if (a > 0xff)
            a = 0xff;
        symbol_color = a << 24 | 0xffffff;
    }
    // FUNCTION: GAME 0x47a340
    void SetProgress(float value)
    {
        progress = value;
    }
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

// The engine's S_vector2 constructor and operators (I3d_math.h), which ResolutionChanged and DrawAll call out of line
// where their budgets run out
// LIBRARY: GAME 0x402530
// ??GS_vector2@@QBE?AU0@ABU0@@Z

// LIBRARY: GAME 0x47a350
// ??0S_vector2@@QAE@MM@Z

// LIBRARY: GAME 0x49b7a0
// ??DS_vector2@@QBE?AU0@M@Z

// LIBRARY: GAME 0x5f7640
// ??DS_vector2@@QBE?AU0@ABU0@@Z

// LIBRARY: GAME 0x5f7660
// ??HS_vector2@@QBE?AU0@ABU0@@Z

// LIBRARY: GAME 0x6025d0
// ??KS_vector2@@QBE?AU0@M@Z
