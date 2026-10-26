#pragma once

#include "Main.h"

// The HUD (PS2 G_IndicatorsClass, g_indicators.cpp; mafiaIndicators is 0x4e38 bytes there). Only what main and the
// profiles use so far. Open: the original's object has a constructor (0x5f6b70) and a destructor (0x5f6a80), which ours
// doesn't have yet, and the next global after it (language_credits) is 0x10 bytes past the flag at 0x46f4.
class G_IndicatorsClass
{
public:
    uchar unknown_0[0x40a4];
    uint unknown_40a4;                  // 0x40a4, cleared by G_LoadSaveClass::Init after ResolutionChanged, as PS2's 0x3094
    uchar unknown_40a8[0x46f4 - 0x40a8];// seven vectors among it, which the destructor frees
    bool parhelia_center;               // 0x46f4, ParheliaSetCenter's flag

    // Creates the HUD at the start (PS2's names, as Destroy's and ResolutionChanged's); G_LoadSaveClass::Init calls it
    void Create();
    // Frees what Create made, at CloseSystem
    void Destroy();
    // Lays the HUD out for the screen's resolution
    void ResolutionChanged();
    // Reads the language's tables\lang_<language>.def into language_primary ('en' for 0) and tables\lang_cz.def into
    // language_credits
    void LanguageSelect(uint language);
    // PC only, the name is ours. Stores the flag at 0x46f4 and, with Matrox Parhelia's two or three screens (the global
    // at 0x6c4508, 1 or 2), confines the scene's viewport and camera to the middle one while it is set; main sets it
    // around the car encyclopedia.
    void ParheliaSetCenter(bool center);
};

extern G_IndicatorsClass mafiaIndicators;

// A language's table (lang_xx.def, 0x238 bytes; PS2's names): "Lang" and the language's two letters, its name ("English"),
// a font's suffix ("_lat") and the font's character table, ushorts.
extern uchar language_primary[0x238];
// The Czech table, whatever the language (PS2's name has it for the credits)
extern uchar language_credits[0x238];
