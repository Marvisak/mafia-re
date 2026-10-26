#pragma once

#include "Main.h"

#include "I3D/I3d_math.h"

class ITexture;
struct TLVERTEX;

// The HUD's fonts (the name is ours)
#define NUM_FONTS 4

// parheliaIndex: Matrox Parhelia's screens, told from the screen's shape (the names are ours)
#define PARHELIA_NONE 0
#define PARHELIA_DUAL 1                 // wider than 2:1
#define PARHELIA_TRIPLE 2               // wider than 3:1: the HUD is on the middle screen

// A needle of the HUD's dials (PS2's name; the members' names are ours), 0x2c bytes: the angles in radians
struct G_RafickaDef
{
    float angle;                        // at the scale's start
    float angle_range;                  // 0x4
    float min;                          // 0x8, the value at the scale's start
    float range;                        // 0xc
    float x, y;                         // 0x10, the needle's axis on the dial
    float unknown_18[4];                // its shape
    uint color;                         // 0x28
};

// A needle's value and the corners _SpocitejRafiku2 computes from it (PS2's name; the members' names are ours)
struct G_RafickaValue
{
    float value;
    float corners[4][2];                // 0x4
};

// The HUD (PS2 G_IndicatorsClass, g_indicators.cpp; mafiaIndicators is 0x4e38 bytes there). Only what main and the
// profiles use so far; the textures' names are ours, after their files. Open: the original's object has a constructor
// (0x5f6b70) and a destructor (0x5f6a80), which ours doesn't have yet, and the next global after it (fontTranstable) is
// 0x10 bytes past the flag at 0x46f4.
class G_IndicatorsClass
{
public:
    uchar fonts[NUM_FONTS][0x101c];     // 0x0, LoadFont's tables\font<n><suffix>.gmf
    ITexture *font_textures[NUM_FONTS]; // 0x4070, font<n><suffix>.tga
    ITexture *texture_int;              // 0x4080, 1int.tga
    uint unknown_4084;                  // 0x4084, cleared by Create (PS2's too)
    ITexture *texture_numbers;          // 0x4088, 1numbers.tga
    ITexture *texture_textura;          // 0x408c, 0textura.tga
    uint unknown_4090;                  // 0x4090, cleared by Create (PS2's too)
    ITexture *texture_logo;             // 0x4094, logoplay.bmp
    ITexture *texture_sight;            // 0x4098, 2sight.bmp with its alpha
    ITexture *texture_race_start;       // 0x409c, racestart.bmp
    ITexture *texture_race_wrong;       // 0x40a0, racewrong.bmp
    uint unknown_40a4;                  // 0x40a4, cleared by G_LoadSaveClass::Init after ResolutionChanged, as PS2's 0x3094
    uchar unknown_40a8[0x10];           // a vector
    S_vector2 screen_size;              // 0x40b8
    S_vector2 screen_scale;             // 0x40c0, the HUD's screen relative to 800x600
    S_vector2 hud_size;                 // 0x40c8, the size of the HUD's screen: the middle one with Parhelia's three
    uchar unknown_40d0[0xc];
    S_vector2 timer_pos;                // 0x40dc, the clock's dial
    uchar unknown_40e4[8];
    G_RafickaValue timer_hours;         // 0x40ec
    G_RafickaValue timer_minutes;       // 0x4110
    G_RafickaValue timer_seconds;       // 0x4134
    uchar unknown_4158[0x1c];
    G_RafickaValue tachometr_speed;     // 0x4174
    G_RafickaValue tachometr_revs;      // 0x4198
    G_RafickaValue tachometr_fuel;      // 0x41bc
    uchar unknown_41e0[4];
    float unknown_41e4;                 // 0x41e4, the texture's column of the tachometer's small display
    uchar unknown_41e8[4];
    S_vector2 hud_scale;                // 0x41ec, the same as screen_scale
    S_vector2 tachometr_pos;            // 0x41f4, its dial's
    S_vector2 revs_pos;                 // 0x41fc, the revs' dial's
    uchar unknown_4204[0x68];
    S_vector2 screen_center;            // 0x426c
    uchar unknown_4274[0x46f4 - 0x4274];// seven vectors among it, which the destructor frees
    bool parhelia_center;               // 0x46f4, ParheliaSetCenter's flag

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
    // A needle's corners for its value, on the dial at pos (PS2's name: "compute the needle")
    void _SpocitejRafiku2(G_RafickaDef *def, G_RafickaValue *value, S_vector2 &pos, TLVERTEX *verts);
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
