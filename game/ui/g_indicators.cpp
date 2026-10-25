// The HUD (PS2 g_indicators.cpp; the PC's file is the one at 0x5f6a20-0x604e5c, docs/game-files.md). Written so far:
// LanguageSelect; the rest of what main calls is stubs.

#include "ui/g_indicators.h"

#include "rw_data.h"
#include "stub.h"

// GLOBAL: GAME 0x6bf980
G_IndicatorsClass mafiaIndicators;

// Open: the original has language_credits right before language_primary, both after mafiaIndicators, where their
// buckets (168, 190; mafiaIndicators' is 450) would put them before it (tools/bsshash.py)
// GLOBAL: GAME 0x6c42c0
uchar language_primary[0x238];
// GLOBAL: GAME 0x6c4088
uchar language_credits[0x238];

// FUNCTION: GAME 0x5f7000
void G_IndicatorsClass::LanguageSelect(uint language)
{
    // STRING: GAME 0x63920c
    char name[64] = "tables\\lang_en.def";
    if (language)
    {
        name[12] = language >> 8;
        name[13] = language;
    }
    else
    {
        name[12] = 'e';
        name[13] = 'n';
    }
    int file = dtaOpen(name, false);
    if (file >= 0)
    {
        dtaRead(file, language_primary, sizeof(language_primary));
        dtaClose(file);
    }
    // STRING: GAME 0x6391f8
    file = dtaOpen("tables\\lang_cz.def", false);
    if (file >= 0)
    {
        dtaRead(file, language_credits, sizeof(language_credits));
        dtaClose(file);
    }
}

#pragma auto_inline(off)

// STUB: GAME 0x604a80
void G_IndicatorsClass::ParheliaSetCenter(bool center)
{
    StubLog("G_IndicatorsClass::ParheliaSetCenter");
}

#pragma auto_inline(on)
