// The HUD (PS2 g_indicators.cpp; the PC's file is the one at 0x5f6a20-0x604e5c, docs/game-files.md). Written so far:
// the constructor, ResetValues and the setters it calls, LoadFont, LanguageSelect, GetLanguage, Create, Destroy,
// GameInit, GameDone, the clock's setters, _SpocitejRafiku2, the player's panel's helpers, the tachometer's and the
// player's setters, the clears C_game's Init and Done call (SubtitlesClear, IntroSubtitlesClear, ConsoleClear,
// RadarClear), MapEnable, WideScreenSetRatio, SetUnknown4484, SetUnknownTexture, RadarRemoveCar, RaceSetStartFlag,
// BonusSetCount, DrawAll, ResolutionChanged (with the dials' vertex helpers), TextSize, OutText, OutTextMultiLine, the
// menus' cursor, the camera's field of view and ParheliaSetCenter; the rest of what main, the scene's callback, the
// profiles and the game call is stubs (RadarAddCar, the radar's edges).

#include "ui/g_indicators.h"
#include "collision/g_colls.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "ui/g_loadsave.h"
#include "ui/g_textdb.h"
#include "vehicles/C_car.h"

#include <stdio.h>

#include "IGraph/IGraph.h"
#include "I3D/I3D.h"
#include "rw_data.h"
#include "stub.h"

// The needles' angles were written in degrees, with pi as 3.1415 (the name is ours)
#define DEGREES(a) ((a) * 3.1415f / 180.0f)

// GLOBAL: GAME 0x6bf980
G_IndicatorsClass mafiaIndicators;

// The dials' needles (PS2's tachometrDefSpeed and tachometrDefPalivo, with PS2's angles; the other names are ours): the
// tachometer's speed, revs and fuel, the clock's hands
// GLOBAL: GAME 0x638bb8
G_RafickaDef tachometrDefSpeed = {DEGREES(207), DEGREES(306), 0.0f, 240.0f, 90.5f, 91.5f, 80.0f, -10.0f, .9f, 1.8f, 0xff74130d};
// GLOBAL: GAME 0x638be4
G_RafickaDef tachometrDefRevs = {DEGREES(200), DEGREES(320), 0.0f, 1.0f, 37.5f, 37.5f, 27.0f, -8.0f, .9f, 1.8f, 0xff74130d};
// GLOBAL: GAME 0x638c10
G_RafickaDef tachometrDefPalivo = {DEGREES(-45), DEGREES(90), 0.0f, 1.0f, 90.5f, 163.5f, 20.0f, -2.0f, 1.0f, 2.0f, 0xff74130d};
// GLOBAL: GAME 0x638c3c
G_RafickaDef timerDefMinutes = {0.0f, DEGREES(360), 0.0f, 60.0f, 42.0f, 62.0f, 32.0f, -2.0f, 1.2f, 1.8f, 0xff000000};
// GLOBAL: GAME 0x638c68
G_RafickaDef timerDefHours = {0.0f, DEGREES(720), 0.0f, 24.0f, 42.0f, 62.0f, 23.0f, -2.0f, 1.3f, 2.0f, 0xff000000};
// GLOBAL: GAME 0x638c94
G_RafickaDef timerDefSeconds = {0.0f, DEGREES(360), 0.0f, 60.0f, 42.0f, 80.5f, 10.0f, -5.0f, .9f, 1.4f, 0xff000000};

// The quads' indices, six for each quad's two triangles (PS2's name and size): the constructor computes 102 quads' of
// them, the first three are initialized data
// GLOBAL: GAME 0x638cc0
static ushort __idx[650] = {0, 1, 2, 1, 3, 2, 4, 5, 6, 5, 7, 6, 8, 9, 10, 9, 11, 10};

// Open: the original's order of the uninitialized globals is the reverse of a definition order close to PS2's
// (parheliaIndex, levyOkraj and pravyOkraj, language_primary, language_credits, fontTranstable, mafiaIndicators,
// textureCrew, textureRadarClip, textureIcons and textureBenzin, the vertices), as an object compiled with another
// file's precompiled header (/Yu) gives, where their buckets (tools/bsshash.py) give another order. And it has one empty
// $E at the file's start (0x5f6a20) where the two S_vector2 globals give two.
// GLOBAL: GAME 0x6c4508
int parheliaIndex;
// GLOBAL: GAME 0x6c4500
S_vector2 levyOkraj;
// GLOBAL: GAME 0x6c44f8
S_vector2 pravyOkraj;
// GLOBAL: GAME 0x6c42c0
S_language language_primary;
// GLOBAL: GAME 0x6c4088
S_language language_credits;
// GLOBAL: GAME 0x6c4078
ushort *fontTranstable[NUM_FONTS];
// GLOBAL: GAME 0x6bf97c
ITexture *textureCrew;
// GLOBAL: GAME 0x6bf978
ITexture *textureRadarClip;
// GLOBAL: GAME 0x6bf974
ITexture *textureIcons;
// GLOBAL: GAME 0x6bf970
ITexture *textureBenzin;

// The dials' vertices, quads of six (PS2's names, whose sizes differ): the clock's dial and hands, the clock's interval,
// the tachometer's dials and displays, its needles, three quads of the player's, and the black bars of
// WideScreenSetRatio
// GLOBAL: GAME 0x6bf670
TLVERTEX _VtxTimerCifernik[24];
// GLOBAL: GAME 0x6bf490
TLVERTEX _VtxTimerInterval[15];
// GLOBAL: GAME 0x6bed10
TLVERTEX _VtxTachometrCiselnik[54];
// GLOBAL: GAME 0x6bead0
TLVERTEX _VtxTachometrRafiky[18];
// GLOBAL: GAME 0x6be890
TLVERTEX _VtxPlayer[18];
// GLOBAL: GAME 0x6be710
TLVERTEX _VtxWideScreen[12];

// The blood over the screen, crew_quads of them (PS2's name)
// GLOBAL: GAME 0x6bdf90
TLVERTEX _VtxCrew[60];

// Zero-initialized, so they come after the others in their definition order (PS2's names): the clock's interval's
// vertices, and the radar's road edges, the positions RadarSetLeftEdge and RadarSetRightEdge take and their vertices
// GLOBAL: GAME 0x6c450c
uint _VtxTimerIntervalNumOfVertexes = 0;
// GLOBAL: GAME 0x6c4510
S_vector *_LeftEdge = NULL;
// GLOBAL: GAME 0x6c4514
S_vector *_RightEdge = NULL;
// GLOBAL: GAME 0x6c4518
TLVERTEX *_VtxLeftEdge = NULL;
// GLOBAL: GAME 0x6c451c
TLVERTEX *_VtxRightEdge = NULL;
// GLOBAL: GAME 0x6c4520
uint _VtxNumOfLeftEdge = 0;
// GLOBAL: GAME 0x6c4524
uint _VtxNumOfRightEdge = 0;

// The original's loop stops at 610 (PS2's the same), short of the table's 650
// FUNCTION: GAME 0x5f6b70
G_IndicatorsClass::G_IndicatorsClass()
{
    for (int i = 0, n = 0; i < 610; i += 6, n += 4)
    {
        __idx[i] = n;
        __idx[i + 1] = n + 1;
        __idx[i + 2] = n + 2;
        __idx[i + 3] = n + 1;
        __idx[i + 4] = n + 3;
        __idx[i + 5] = n + 2;
    }
    memset(&language_primary, 0, sizeof(language_primary));
    memset(&language_credits, 0, sizeof(language_credits));
    font_textures[0] = NULL;
    font_textures[1] = NULL;
    font_textures[2] = NULL;
    font_textures[3] = NULL;
    ResetValues();
}

// In PS2's order; PS2's also resets a second text and two members of its own
// FUNCTION: GAME 0x5f6cf0
void G_IndicatorsClass::ResetValues()
{
    unknown_40a4 = 0;
    unknown_40a8.clear();
    TachometrSetGear(0, false);
    TachometrSetValues(0.0f, 0.0f, 0.0f, 0.0f);
    unknown_4208 = 0.0f;
    PlayerSetLives(100);
    PlayerSetAmmo(16, 30);
    PlayerSetAccuracy(1.0f);
    PlayerSetWingmanName(0, 0);
    PlayerSetWingmanName(1, 0);
    PlayerSetWingmanName(2, 0);
    PlayerSetWingmanName(3, 0);
    PlayerSetWingmanLives(0, 100);
    PlayerSetWingmanLives(1, 100);
    PlayerSetWingmanLives(2, 100);
    PlayerSetWingmanLives(3, 100);
    compass_destination = NULL;
    thief_indicator = .5f;
    console_first = 0;
    console_count = 0;
    SYSTEMTIME time;
    GetLocalTime(&time);
    TimerSetTime(time.wHour, time.wMinute, time.wSecond);
    TimerSetInterval(130);
    WideScreenSetRatio(.5f);
    memset(_VtxTimerCifernik, 0, sizeof(_VtxTimerCifernik));
    memset(_VtxTimerInterval, 0, sizeof(_VtxTimerInterval));
    radar_player_car = NULL;
    radar_cars.clear();
    unknown_44bc = 0;
    radar_items.clear();
    unknown_44c0 = .9f;
    race_start_last_time = 0.0f;
    race_start_time = 0.0f;
    race_best_time[0] = 0;
    race_lap_time[0] = 0;
    race_time[0] = 0;
    race_position[0] = 0;
    race_laps[0] = 0;
    unknown_46dc = 0;
    unknown_4484 = 0.0f;
    unknown_4490 = NULL;
    unknown_448c = NULL;
    unknown_4480 = NULL;
    unknown_46e0 = 0;
    unknown_46e4 = -1;
    map_enabled = false;
    text_out_of_city = textDatabase.GetText(0x5f);
    parhelia_center = false;
    IntroSubtitlesClear();
    BonusSetCount(BONUS_NONE);
}

// The PC's version doesn't adjust the fonts for other languages or make PS2's fake fonts
// FUNCTION: GAME 0x5f6f30
void G_IndicatorsClass::LoadFont(bool credits, uint font)
{
    S_language *language = credits ? &language_credits : &language_primary;
    if (font_textures[font])
        font_textures[font]->Release();
    font_textures[font] = NULL;
    char name[64];
    // STRING: GAME 0x6391e8
    sprintf(name, "font%d%s.tga", font, language->font_suffix);
    igraph->CreateITexture(name, NULL, ITC_ALPHA, &font_textures[font]);
    // STRING: GAME 0x6391d4
    sprintf(name, "tables\\font%d%s.gmf", font, language->font_suffix);
    int file = dtaOpen(name, false);
    if (file >= 0)
    {
        dtaRead(file, &fonts[font], sizeof(fonts[font]));
        dtaClose(file);
    }
    fontTranstable[font] = language->chars;
}

// FUNCTION: GAME 0x5f7000
void G_IndicatorsClass::LanguageSelect(uint language)
{
    // STRING: GAME 0x63920c
    char name[64] = "tables\\lang_en.def";
    if (language)                       // its two letters in place of the "en"
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
        dtaRead(file, &language_primary, sizeof(language_primary));
        dtaClose(file);
    }
    // STRING: GAME 0x6391f8
    file = dtaOpen("tables\\lang_cz.def", false);
    if (file >= 0)
    {
        dtaRead(file, &language_credits, sizeof(language_credits));
        dtaClose(file);
    }
}

// FUNCTION: GAME 0x5f70d0
uint G_IndicatorsClass::GetLanguage()
{
    return *(uint *)language_primary.language;
}

// PS2's loads the fonts first, after preloading the menus' textures, and the icons, which the PC leaves unloaded
// FUNCTION: GAME 0x5f70e0
void G_IndicatorsClass::Create()
{
    unknown_4090 = NULL;
    // STRING: GAME 0x6392a4
    igraph->CreateITexture("logoplay.bmp", NULL, 0, &texture_logo);
    igraph->CreateITexture(
        // STRING: GAME 0x63928c
        "2sight.bmp",
        // STRING: GAME 0x639298
        "2sight+.bmp",
        ITC_TRUECOLOR | ITC_USE_ALPHA, &texture_sight);
    // STRING: GAME 0x63927c
    igraph->CreateITexture("racestart.bmp", NULL, ITC_ALPHA, &texture_race_start);
    // STRING: GAME 0x63926c
    igraph->CreateITexture("racewrong.bmp", NULL, 0, &texture_race_wrong);
    igraph->CreateITexture(
        // STRING: GAME 0x6372d0
        "2blood.bmp",
        // STRING: GAME 0x6372dc
        "2blood+.bmp",
        ITC_USE_ALPHA, &textureCrew);
    // STRING: GAME 0x639260
    igraph->CreateITexture("1int.tga", NULL, ITC_COLORKEY, &texture_int);
    texture_int2 = NULL;
    // STRING: GAME 0x639250
    igraph->CreateITexture("1numbers.tga", NULL, ITC_ALPHA, &texture_numbers);
    // STRING: GAME 0x639240
    igraph->CreateITexture("0textura.tga", NULL, ITC_ALPHA, &texture_textura);
    // STRING: GAME 0x639230
    igraph->CreateITexture("radarclp.bmp", NULL, ITC_COLORKEY, &textureRadarClip);
    textureIcons = NULL;
    // STRING: GAME 0x639220
    igraph->CreateITexture("2intbenal.tga", NULL, ITC_COLORKEY, &textureBenzin);
    LoadFont(false, 0);
    LoadFont(false, 1);
    LoadFont(false, 2);
    LoadFont(false, 3);
    ResetValues();
}

// Create's textures and the fonts', in another order (PS2's the same)
// FUNCTION: GAME 0x5f7250
void G_IndicatorsClass::Destroy()
{
    if (font_textures[0])
        font_textures[0]->Release();
    font_textures[0] = NULL;
    if (font_textures[1])
        font_textures[1]->Release();
    font_textures[1] = NULL;
    if (font_textures[2])
        font_textures[2]->Release();
    font_textures[2] = NULL;
    if (font_textures[3])
        font_textures[3]->Release();
    font_textures[3] = NULL;
    if (texture_int)
        texture_int->Release();
    texture_int = NULL;
    if (texture_int2)
        texture_int2->Release();
    texture_int2 = NULL;
    if (texture_numbers)
        texture_numbers->Release();
    texture_numbers = NULL;
    if (texture_textura)
        texture_textura->Release();
    texture_textura = NULL;
    if (textureRadarClip)
        textureRadarClip->Release();
    textureRadarClip = NULL;
    if (textureIcons)
        textureIcons->Release();
    textureIcons = NULL;
    if (textureBenzin)
        textureBenzin->Release();
    textureBenzin = NULL;
    if (unknown_4090)
        unknown_4090->Release();
    unknown_4090 = NULL;
    if (texture_logo)
        texture_logo->Release();
    texture_logo = NULL;
    if (texture_sight)
        texture_sight->Release();
    texture_sight = NULL;
    if (texture_race_start)
        texture_race_start->Release();
    texture_race_start = NULL;
    if (texture_race_wrong)
        texture_race_wrong->Release();
    texture_race_wrong = NULL;
    if (textureCrew)
        textureCrew->Release();
    textureCrew = NULL;
}

// The PC's: PS2's loads much more (its map among it)
// FUNCTION: GAME 0x5f73c0
void G_IndicatorsClass::GameInit()
{
    if (act_mission->game->unknown_2d44)
    {
        if (texture_int2)
            texture_int2->Release();
        if (mafiaLoadSave.settings.speedometer_mph)
            // STRING: GAME 0x6392f8
            igraph->CreateITexture("2intnocmph.tga", NULL, ITC_COLORKEY, &texture_int2);
        else
            // STRING: GAME 0x6392ec
            igraph->CreateITexture("2intnoc.tga", NULL, ITC_COLORKEY, &texture_int2);
        if (textureIcons)
            textureIcons->Release();
        // STRING: GAME 0x6392dc
        igraph->CreateITexture("2intauton.tga", NULL, ITC_COLORKEY, &textureIcons);
    }
    else
    {
        if (texture_int2)
            texture_int2->Release();
        if (mafiaLoadSave.settings.speedometer_mph)
            // STRING: GAME 0x6392d0
            igraph->CreateITexture("2intmph.tga", NULL, ITC_COLORKEY, &texture_int2);
        else
            // STRING: GAME 0x6392c4
            igraph->CreateITexture("2int.tga", NULL, ITC_COLORKEY, &texture_int2);
        if (textureIcons)
            textureIcons->Release();
        // STRING: GAME 0x6392b4
        igraph->CreateITexture("2intauto.tga", NULL, ITC_COLORKEY, &textureIcons);
    }
}

// PS2's releases the dials' texture too unless asked not to, and resets more
// FUNCTION: GAME 0x5f74a0
void G_IndicatorsClass::GameDone()
{
    if (unknown_4090)
        unknown_4090->Release();
    unknown_4090 = NULL;
    RadarSetLeftEdge(NULL);
    RadarSetRightEdge(NULL);
    BonusSetCount(BONUS_NONE);
    IntroSubtitlesClear();
    RaceSetStartFlag(RACE_START_NONE);
    TimerSetInterval(0);
}

// FUNCTION: GAME 0x5f7500
void G_IndicatorsClass::TimerSetTime(uint hour, uint minute, uint second)
{
    timer_hour = hour;
    timer_minute = minute;
    timer_second = second;
    timer_minutes.value = -1.0f;
    timer_hours.value = -1.0f;
    timer_seconds.value = -1.0f;
}

// The hands' places at the interval's end from the clock's time; the minute hand's past a whole turn starts again
// FUNCTION: GAME 0x5f7540
void G_IndicatorsClass::TimerSetInterval(uint interval)
{
    this->interval = interval * 1000;
    interval_minute = (timer_minute + interval / 60) * (1.0f / 60);
    if (interval_minute > 1.0f)
        interval_minute -= 1.0f;
    interval_second = ((float)timer_second + interval % 60) * (1.0f / 60);
    timer_minutes.value = -1.0f;
    timer_hours.value = -1.0f;
    timer_seconds.value = -1.0f;
}

// The needle at the value's angle, scaled from 800x600, and its quad, the texture's u along it from the tail
// FUNCTION: GAME 0x5f7680
void G_IndicatorsClass::_SpocitejRafiku2(G_RafickaDef *def, G_RafickaValue *value, S_vector2 &pos, TLVERTEX *verts)
{
    float t = (value->value - def->min) / def->range;
    if (t < 0.0f)
        t = 0.0f;
    else if (t > 1.0f)
        t = 1.0f;
    float angle = t * def->angle_range + def->angle;
    float s = (float)sin(angle);
    float c = (float)cos(angle);
    S_vector2 axis(def->x, def->y);
    S_vector2 dir(s, -c);               // up at 0, clockwise on the screen
    S_vector2 side(c, s);
    value->tip_right = (axis + side * def->tip_width + dir * def->tip) * hud_scale + pos;
    value->tail_right = (axis + side * def->tail_width + dir * def->tail) * hud_scale + pos;
    value->tip_left = (axis - side * def->tip_width + dir * def->tip) * hud_scale + pos;
    value->tail_left = (axis - side * def->tail_width + dir * def->tail) * hud_scale + pos;
    uint color = def->color;
    verts[0].x = value->tail_left.x;
    verts[0].y = value->tail_left.y;
    verts[0].z = 0.0f;
    verts[0].rhw = 1.0f;
    verts[0].color = color;
    verts[0].u = 0.0f;
    verts[0].v = 0.0f;
    verts[1].x = value->tip_left.x;
    verts[1].y = value->tip_left.y;
    verts[1].z = 0.0f;
    verts[1].rhw = 1.0f;
    verts[1].color = color;
    verts[1].u = 1.0f;
    verts[1].v = 0.0f;
    verts[2].x = value->tail_right.x;
    verts[2].y = value->tail_right.y;
    verts[2].z = 0.0f;
    verts[2].rhw = 1.0f;
    verts[2].color = color;
    verts[2].u = 0.0f;
    verts[2].v = 1.0f;
    verts[3].x = value->tip_right.x;
    verts[3].y = value->tip_right.y;
    verts[3].z = 0.0f;
    verts[3].rhw = 1.0f;
    verts[3].color = color;
    verts[3].u = 1.0f;
    verts[3].v = 1.0f;
    verts[4] = verts[2];
    verts[5] = verts[1];
}

// The player's panel, which DrawAll builds for 800x600 from the HUD's left edge: each helper adds its quads from
// verts[*count * 2] on, *count counting the triangles, each quad's vertices in the order of the quads' indices
// (__idx). The rows are 26 apart, the player's at the bottom (index 0).
// The row's background, with the name's picture from 1int.tga's pictures 23 texels apart
// FUNCTION: GAME 0x5f7990
void G_IndicatorsClass::_PridejWingPodklad(TLVERTEX *verts, uint *count, uint index, uint name)
{
    verts += *count * 2;
    float row = (float)(name * 23);
    float x = screen_scale.x * 7.0f + levyOkraj.x;
    float y = (float)(568 - index * 26) * screen_scale.y;
    float sx = screen_scale.x * 51.0f;
    float sy = screen_scale.y * 23.0f;
    verts[0].x = x;
    verts[0].y = y;
    verts[0].z = 0.0f;
    verts[0].rhw = 1.0f;
    verts[0].color = 0xffffffff;
    verts[0].u = 0.0f;
    verts[0].v = (row + 73.0f) / 256.0f;
    verts[1].x = x + sx;
    verts[1].y = y;
    verts[1].z = 0.0f;
    verts[1].rhw = 1.0f;
    verts[1].color = 0xffffffff;
    verts[1].u = 204 / 256.0f;
    verts[1].v = (row + 73.0f) / 256.0f;
    verts[2].x = x;
    verts[2].y = y + sy;
    verts[2].z = 0.0f;
    verts[2].rhw = 1.0f;
    verts[2].color = 0xffffffff;
    verts[2].u = 0.0f;
    verts[2].v = (row + 96.0f) / 256.0f;
    verts[3].x = x + sx;
    verts[3].y = y + sy;
    verts[3].z = 0.0f;
    verts[3].rhw = 1.0f;
    verts[3].color = 0xffffffff;
    verts[3].u = 204 / 256.0f;
    verts[3].v = (row + 96.0f) / 256.0f;
    *count += 2;
}

// FUNCTION: GAME 0x5f7ad0
void G_IndicatorsClass::_PridejAmmoPodklad(TLVERTEX *verts, uint *count)
{
    verts += *count * 2;
    float x = screen_scale.x * 62.0f + levyOkraj.x;
    float y = screen_scale.y * 568.0f;
    float sx = screen_scale.x * 65.0f;
    float sy = screen_scale.y * 23.0f;
    verts[0].x = x;
    verts[0].y = y;
    verts[0].z = 0.0f;
    verts[0].rhw = 1.0f;
    verts[0].color = 0xffffffff;
    verts[0].u = 0.0f;
    verts[0].v = 1 / 256.0f;
    verts[1].x = x + sx;
    verts[1].y = y;
    verts[1].z = 0.0f;
    verts[1].rhw = 1.0f;
    verts[1].color = 0xffffffff;
    verts[1].u = 252 / 256.0f;
    verts[1].v = 1 / 256.0f;
    verts[2].x = x;
    verts[2].y = y + sy;
    verts[2].z = 0.0f;
    verts[2].rhw = 1.0f;
    verts[2].color = 0xffffffff;
    verts[2].u = 0.0f;
    verts[2].v = 24 / 256.0f;
    verts[3].x = x + sx;
    verts[3].y = y + sy;
    verts[3].z = 0.0f;
    verts[3].rhw = 1.0f;
    verts[3].color = 0xffffffff;
    verts[3].u = 252 / 256.0f;
    verts[3].v = 24 / 256.0f;
    *count += 2;
}

// The health's digits from the right, up to three, from 1numbers.tga's digits 11 texels apart, the next .9 of a digit's
// width to the left, .5 after a 1
// FUNCTION: GAME 0x5f7bc0
void G_IndicatorsClass::_PridejWingLives(TLVERTEX *verts, uint *count, uint index, uint lives)
{
    verts += *count * 2;
    float x = screen_scale.x * 42.0f + levyOkraj.x;
    float y = (float)(572 - index * 26) * screen_scale.y;
    float sx = screen_scale.x * 11.0f;
    float sy = screen_scale.y * 16.0f;
    uint digit = lives % 10;
    float u = (float)(digit * 11);
    verts[0].x = x;
    verts[0].y = y;
    verts[0].z = 0.0f;
    verts[0].rhw = 1.0f;
    verts[0].color = 0xffffffff;
    verts[0].u = u / 128.0f;
    verts[0].v = 0.0f;
    verts[1].x = x + sx;
    verts[1].y = y;
    verts[1].z = 0.0f;
    verts[1].rhw = 1.0f;
    verts[1].color = 0xffffffff;
    verts[1].u = (u + 11.0f) / 128.0f;
    verts[1].v = 0.0f;
    verts[2].x = x;
    verts[2].y = y + sy;
    verts[2].z = 0.0f;
    verts[2].rhw = 1.0f;
    verts[2].color = 0xffffffff;
    verts[2].u = u / 128.0f;
    verts[2].v = 254 / 256.0f;
    verts[3].x = x + sx;
    verts[3].y = y + sy;
    verts[3].z = 0.0f;
    verts[3].rhw = 1.0f;
    verts[3].color = 0xffffffff;
    verts[3].u = (u + 11.0f) / 128.0f;
    verts[3].v = 254 / 256.0f;
    *count += 2;
    if (digit == 1)
        x -= sx * .5f;
    else
        x -= sx * .9f;
    if (lives < 10)
        return;
    digit = lives / 10 % 10;
    u = (float)(digit * 11);
    verts[4].x = x;
    verts[4].y = y;
    verts[4].z = 0.0f;
    verts[4].rhw = 1.0f;
    verts[4].color = 0xffffffff;
    verts[4].u = u / 128.0f;
    verts[4].v = 0.0f;
    verts[5].x = x + sx;
    verts[5].y = y;
    verts[5].z = 0.0f;
    verts[5].rhw = 1.0f;
    verts[5].color = 0xffffffff;
    verts[5].u = (u + 11.0f) / 128.0f;
    verts[5].v = 0.0f;
    verts[6].x = x;
    verts[6].y = y + sy;
    verts[6].z = 0.0f;
    verts[6].rhw = 1.0f;
    verts[6].color = 0xffffffff;
    verts[6].u = u / 128.0f;
    verts[6].v = 254 / 256.0f;
    verts[7].x = x + sx;
    verts[7].y = y + sy;
    verts[7].z = 0.0f;
    verts[7].rhw = 1.0f;
    verts[7].color = 0xffffffff;
    verts[7].u = (u + 11.0f) / 128.0f;
    verts[7].v = 254 / 256.0f;
    *count += 2;
    if (digit == 1)
        x -= sx * .5f;
    else
        x -= sx * .9f;
    if (lives < 100)
        return;
    digit = lives / 100;
    u = (float)(digit * 11);
    verts[8].x = x;
    verts[8].y = y;
    verts[8].z = 0.0f;
    verts[8].rhw = 1.0f;
    verts[8].color = 0xffffffff;
    verts[8].u = u / 128.0f;
    verts[8].v = 0.0f;
    verts[9].x = x + sx;
    verts[9].y = y;
    verts[9].z = 0.0f;
    verts[9].rhw = 1.0f;
    verts[9].color = 0xffffffff;
    verts[9].u = (u + 11.0f) / 128.0f;
    verts[9].v = 0.0f;
    verts[10].x = x;
    verts[10].y = y + sy;
    verts[10].z = 0.0f;
    verts[10].rhw = 1.0f;
    verts[10].color = 0xffffffff;
    verts[10].u = u / 128.0f;
    verts[10].v = 254 / 256.0f;
    verts[11].x = x + sx;
    verts[11].y = y + sy;
    verts[11].z = 0.0f;
    verts[11].rhw = 1.0f;
    verts[11].color = 0xffffffff;
    verts[11].u = (u + 11.0f) / 128.0f;
    verts[11].v = 254 / 256.0f;
    *count += 2;
}

// "<ammo>/<spare>" from the right as _PridejWingLives writes a number: the spare rounds' digits, up to three, the slash,
// then the weapon's, the next digit .8 of a digit's width to the left, .45 after a 1
// FUNCTION: GAME 0x5f7f90
void G_IndicatorsClass::_PridejAmmo(TLVERTEX *verts, uint *count, uint ammo, uint spare)
{
    verts += *count * 2;
    float x = screen_scale.x * 112.0f + levyOkraj.x;
    float y = screen_scale.y * 571.0f;
    float sx = screen_scale.x * 11.0f;
    float sy = screen_scale.y * 16.0f;
    uint digit = spare % 10;
    float u = (float)(digit * 11);
    verts[0].x = x;
    verts[0].y = y;
    verts[0].z = 0.0f;
    verts[0].rhw = 1.0f;
    verts[0].color = 0xffffffff;
    verts[0].u = u / 128.0f;
    verts[0].v = 0.0f;
    verts[1].x = x + sx;
    verts[1].y = y;
    verts[1].z = 0.0f;
    verts[1].rhw = 1.0f;
    verts[1].color = 0xffffffff;
    verts[1].u = (u + 11.0f) / 128.0f;
    verts[1].v = 0.0f;
    verts[2].x = x;
    verts[2].y = y + sy;
    verts[2].z = 0.0f;
    verts[2].rhw = 1.0f;
    verts[2].color = 0xffffffff;
    verts[2].u = u / 128.0f;
    verts[2].v = 254 / 256.0f;
    verts[3].x = x + sx;
    verts[3].y = y + sy;
    verts[3].z = 0.0f;
    verts[3].rhw = 1.0f;
    verts[3].color = 0xffffffff;
    verts[3].u = (u + 11.0f) / 128.0f;
    verts[3].v = 254 / 256.0f;
    *count += 2;
    verts += 4;
    if (digit == 1)
        x -= sx * .45f;
    else
        x -= sx * .8f;
    if (spare >= 10)
    {
        digit = spare / 10 % 10;
        u = (float)(digit * 11);
        verts[0].x = x;
        verts[0].y = y;
        verts[0].z = 0.0f;
        verts[0].rhw = 1.0f;
        verts[0].color = 0xffffffff;
        verts[0].u = u / 128.0f;
        verts[0].v = 0.0f;
        verts[1].x = x + sx;
        verts[1].y = y;
        verts[1].z = 0.0f;
        verts[1].rhw = 1.0f;
        verts[1].color = 0xffffffff;
        verts[1].u = (u + 11.0f) / 128.0f;
        verts[1].v = 0.0f;
        verts[2].x = x;
        verts[2].y = y + sy;
        verts[2].z = 0.0f;
        verts[2].rhw = 1.0f;
        verts[2].color = 0xffffffff;
        verts[2].u = u / 128.0f;
        verts[2].v = 254 / 256.0f;
        verts[3].x = x + sx;
        verts[3].y = y + sy;
        verts[3].z = 0.0f;
        verts[3].rhw = 1.0f;
        verts[3].color = 0xffffffff;
        verts[3].u = (u + 11.0f) / 128.0f;
        verts[3].v = 254 / 256.0f;
        *count += 2;
        verts += 4;
        if (digit == 1)
            x -= sx * .45f;
        else
            x -= sx * .8f;
        if (spare >= 100)
        {
            digit = spare / 100 % 10;
            u = (float)(digit * 11);
            verts[0].x = x;
            verts[0].y = y;
            verts[0].z = 0.0f;
            verts[0].rhw = 1.0f;
            verts[0].color = 0xffffffff;
            verts[0].u = u / 128.0f;
            verts[0].v = 0.0f;
            verts[1].x = x + sx;
            verts[1].y = y;
            verts[1].z = 0.0f;
            verts[1].rhw = 1.0f;
            verts[1].color = 0xffffffff;
            verts[1].u = (u + 11.0f) / 128.0f;
            verts[1].v = 0.0f;
            verts[2].x = x;
            verts[2].y = y + sy;
            verts[2].z = 0.0f;
            verts[2].rhw = 1.0f;
            verts[2].color = 0xffffffff;
            verts[2].u = u / 128.0f;
            verts[2].v = 254 / 256.0f;
            verts[3].x = x + sx;
            verts[3].y = y + sy;
            verts[3].z = 0.0f;
            verts[3].rhw = 1.0f;
            verts[3].color = 0xffffffff;
            verts[3].u = (u + 11.0f) / 128.0f;
            verts[3].v = 254 / 256.0f;
            *count += 2;
            verts += 4;
            if (digit == 1)
                x -= sx * .45f;
            else
                x -= sx * .8f;
        }
    }
    verts[0].x = x;
    verts[0].y = y;
    verts[0].z = 0.0f;
    verts[0].rhw = 1.0f;
    verts[0].color = 0xffffffff;
    verts[0].u = 220 / 256.0f;
    verts[0].v = 0.0f;
    verts[1].x = x + sx;
    verts[1].y = y;
    verts[1].z = 0.0f;
    verts[1].rhw = 1.0f;
    verts[1].color = 0xffffffff;
    verts[1].u = 242 / 256.0f;
    verts[1].v = 0.0f;
    verts[2].x = x;
    verts[2].y = y + sy;
    verts[2].z = 0.0f;
    verts[2].rhw = 1.0f;
    verts[2].color = 0xffffffff;
    verts[2].u = 220 / 256.0f;
    verts[2].v = 254 / 256.0f;
    verts[3].x = x + sx;
    verts[3].y = y + sy;
    verts[3].z = 0.0f;
    verts[3].rhw = 1.0f;
    verts[3].color = 0xffffffff;
    verts[3].u = 242 / 256.0f;
    verts[3].v = 254 / 256.0f;
    verts += 4;
    *count += 2;
    x -= sx * .8f;
    digit = ammo % 10;
    u = (float)(digit * 11);
    verts[0].x = x;
    verts[0].y = y;
    verts[0].z = 0.0f;
    verts[0].rhw = 1.0f;
    verts[0].color = 0xffffffff;
    verts[0].u = u / 128.0f;
    verts[0].v = 0.0f;
    verts[1].x = x + sx;
    verts[1].y = y;
    verts[1].z = 0.0f;
    verts[1].rhw = 1.0f;
    verts[1].color = 0xffffffff;
    verts[1].u = (u + 11.0f) / 128.0f;
    verts[1].v = 0.0f;
    verts[2].x = x;
    verts[2].y = y + sy;
    verts[2].z = 0.0f;
    verts[2].rhw = 1.0f;
    verts[2].color = 0xffffffff;
    verts[2].u = u / 128.0f;
    verts[2].v = 254 / 256.0f;
    verts[3].x = x + sx;
    verts[3].y = y + sy;
    verts[3].z = 0.0f;
    verts[3].rhw = 1.0f;
    verts[3].color = 0xffffffff;
    verts[3].u = (u + 11.0f) / 128.0f;
    verts[3].v = 254 / 256.0f;
    *count += 2;
    if (digit == 1)
        x -= sx * .45f;
    else
        x -= sx * .8f;
    if (ammo < 10)
        return;
    digit = ammo / 10 % 10;
    u = (float)(digit * 11);
    verts[4].x = x;
    verts[4].y = y;
    verts[4].z = 0.0f;
    verts[4].rhw = 1.0f;
    verts[4].color = 0xffffffff;
    verts[4].u = u / 128.0f;
    verts[4].v = 0.0f;
    verts[5].x = x + sx;
    verts[5].y = y;
    verts[5].z = 0.0f;
    verts[5].rhw = 1.0f;
    verts[5].color = 0xffffffff;
    verts[5].u = (u + 11.0f) / 128.0f;
    verts[5].v = 0.0f;
    verts[6].x = x;
    verts[6].y = y + sy;
    verts[6].z = 0.0f;
    verts[6].rhw = 1.0f;
    verts[6].color = 0xffffffff;
    verts[6].u = u / 128.0f;
    verts[6].v = 254 / 256.0f;
    verts[7].x = x + sx;
    verts[7].y = y + sy;
    verts[7].z = 0.0f;
    verts[7].rhw = 1.0f;
    verts[7].color = 0xffffffff;
    verts[7].u = (u + 11.0f) / 128.0f;
    verts[7].v = 254 / 256.0f;
    *count += 2;
    if (digit == 1)
        x -= sx * .45f;
    else
        x -= sx * .8f;
    if (ammo < 100)
        return;
    digit = ammo / 100;
    u = (float)(digit * 11);
    verts[8].x = x;
    verts[8].y = y;
    verts[8].z = 0.0f;
    verts[8].rhw = 1.0f;
    verts[8].color = 0xffffffff;
    verts[8].u = u / 128.0f;
    verts[8].v = 0.0f;
    verts[9].x = x + sx;
    verts[9].y = y;
    verts[9].z = 0.0f;
    verts[9].rhw = 1.0f;
    verts[9].color = 0xffffffff;
    verts[9].u = (u + 11.0f) / 128.0f;
    verts[9].v = 0.0f;
    verts[10].x = x;
    verts[10].y = y + sy;
    verts[10].z = 0.0f;
    verts[10].rhw = 1.0f;
    verts[10].color = 0xffffffff;
    verts[10].u = u / 128.0f;
    verts[10].v = 254 / 256.0f;
    verts[11].x = x + sx;
    verts[11].y = y + sy;
    verts[11].z = 0.0f;
    verts[11].rhw = 1.0f;
    verts[11].color = 0xffffffff;
    verts[11].u = (u + 11.0f) / 128.0f;
    verts[11].v = 254 / 256.0f;
    *count += 2;
}

// The gears are 11 texels apart in the texture
// FUNCTION: GAME 0x5f8690
void G_IndicatorsClass::TachometrSetGear(int gear, bool flag)
{
    gear_column = gear * 11.0f;
    unknown_4204 = flag;
}

// The needles move only when their values change. The odometer shows five digits of a hundredth of the distance (a
// tenth of it, 999999 added below 0, without its last digit), each digit's u from the texture's strip of digits 13
// texels apart: the lowest rolls with the last digit, the next ones only while the digits below them are at 9. PS2's
// has no revs and divides by 10 and 255 where the PC's multiplies. The digit through a uint local, as the original
// converts it; open: the original keeps n's loop value in eax and copies it to edi in the body, ours the other way
// round, which no spelling of the loop, its counter, the divisions or the locals' first appearances moved.
// FUNCTION: GAME 0x5f86b0
void G_IndicatorsClass::TachometrSetValues(float speed, float revs, float fuel, float odometer)
{
    speed *= 3.6f;
    if (speed != tachometr_speed.value)
    {
        tachometr_speed.value = speed;
        _SpocitejRafiku2(&tachometrDefSpeed, &tachometr_speed, tachometr_pos, &_VtxTachometrRafiky[0]);
    }
    if (tachometr_revs.value != revs)
    {
        tachometr_revs.value = revs;
        _SpocitejRafiku2(&tachometrDefRevs, &tachometr_revs, revs_pos, &_VtxTachometrRafiky[6]);
    }
    if (tachometr_fuel.value != fuel)
    {
        tachometr_fuel.value = fuel;
        _SpocitejRafiku2(&tachometrDefPalivo, &tachometr_fuel, tachometr_pos, &_VtxTachometrRafiky[12]);
    }
    uint n = odometer < 0.0f ? (uint)(odometer * .1f + 999999.0f) : (uint)(odometer * .1f);
    float roll = (float)(n % 10);
    n /= 10;
    TLVERTEX *v = &_VtxTachometrCiselnik[12];
    for (int i = 0; i < 5; i++)
    {
        uint d = n % 10;
        n /= 10;
        float digit = (float)d;
        float u = 130.0f - (digit * 13.0f + roll * 1.3f);
        float u1 = (u + 125.0f - 1.0f) / 256.0f;
        float u0 = (u + 113.0f) / 256.0f;
        v[0].u = u1;
        v[0].v = 243 / 256.0f;
        v[0].color = 0x70ffffff;
        v[1].u = u1;
        v[1].v = 255 / 256.0f;
        v[1].color = 0x70ffffff;
        v[2].u = u0;
        v[2].v = 243 / 256.0f;
        v[2].color = 0x70ffffff;
        v[3].u = u0;
        v[3].v = 255 / 256.0f;
        v[3].color = 0x70ffffff;
        v[4].u = u0;
        v[4].v = 243 / 256.0f;
        v[4].color = 0x70ffffff;
        v[5].u = u1;
        v[5].v = 255 / 256.0f;
        v[5].color = 0x70ffffff;
        if (digit < 9.0f)
            roll = 0.0f;
        v += 6;
    }
    fuel_low = false;
    if (fuel < .02f)
    {
        fuel_low = true;
        if (revs < .01f)
            unknown_4208 = 0.0f;
    }
}

// FUNCTION: GAME 0x5f88e0
void G_IndicatorsClass::PlayerSetLives(uint lives)
{
    this->lives = lives;
    // STRING: GAME 0x63814c
    sprintf(lives_text, "%d", lives);
}

// FUNCTION: GAME 0x5f8910
void G_IndicatorsClass::PlayerSetAmmo(uint ammo, uint spare)
{
    this->ammo = ammo;
    ammo_spare = spare;
    // STRING: GAME 0x639308
    sprintf(ammo_text, "%d/%d", ammo, spare);
}

// FUNCTION: GAME 0x5f8940
void G_IndicatorsClass::PlayerSetAccuracy(float accuracy)
{
    this->accuracy = accuracy;
}

// FUNCTION: GAME 0x5f8950
void G_IndicatorsClass::PlayerSetWingmanName(uint wingman, uint name)
{
    if (wingman <= MAX_WINGMEN - 1)
        wingman_names[wingman] = name & 7;
}

// FUNCTION: GAME 0x5f8970
void G_IndicatorsClass::PlayerSetWingmanLives(uint wingman, uint lives)
{
    if (wingman <= MAX_WINGMEN - 1)
        wingman_lives[wingman] = lives;
}

// FUNCTION: GAME 0x5f8990
void G_IndicatorsClass::SubtitlesClear()
{
    subtitles.clear();
}

// FUNCTION: GAME 0x5f9940
void G_IndicatorsClass::IntroSubtitlesClear()
{
    intro_subtitles.clear();
    intro_subtitles_waiting.clear();
}

// FUNCTION: GAME 0x5f9d10
void G_IndicatorsClass::MapEnable(bool enable)
{
    map_enabled = enable;
    text_out_of_city = textDatabase.GetText(0x5f);
}

// PS2's also stops its console's timer, which the PC's lines have each
// FUNCTION: GAME 0x5f9d40
void G_IndicatorsClass::ConsoleClear()
{
    console_count = 0;
}

// Two quads in the colour 0xaf0000ff over the whole width, a few pixels past the screen's edges: from the top to where
// the picture starts and from where it ends to the bottom, the picture's height the scaled 801 times the ratio. Each
// quad's vertices left before right (PS2's the same), the edges written at each use.
// FUNCTION: GAME 0x5fa050
void G_IndicatorsClass::WideScreenSetRatio(float ratio)
{
    float x = levyOkraj.x;
    float width = screen_scale.x * 801.0f;
    float height = screen_scale.y * 601.0f;
    ratio *= width;
    float top = (height - ratio) * .5f;
    float bottom = ratio + top;
    _VtxWideScreen[0].x = x - 2.0f;
    _VtxWideScreen[0].y = -2.0f;
    _VtxWideScreen[0].z = 0.0f;
    _VtxWideScreen[0].rhw = 1.0f;
    _VtxWideScreen[0].color = 0xaf0000ff;
    _VtxWideScreen[0].u = 0.0f;
    _VtxWideScreen[0].v = 0.0f;
    _VtxWideScreen[2].x = x - 2.0f;
    _VtxWideScreen[2].y = top;
    _VtxWideScreen[2].z = 0.0f;
    _VtxWideScreen[2].rhw = 1.0f;
    _VtxWideScreen[2].color = 0xaf0000ff;
    _VtxWideScreen[2].u = 0.0f;
    _VtxWideScreen[2].v = 1.0f;
    _VtxWideScreen[1].x = width + x + 3.0f;
    _VtxWideScreen[1].y = -2.0f;
    _VtxWideScreen[1].z = 0.0f;
    _VtxWideScreen[1].rhw = 1.0f;
    _VtxWideScreen[1].color = 0xaf0000ff;
    _VtxWideScreen[1].u = 1.0f;
    _VtxWideScreen[1].v = 0.0f;
    _VtxWideScreen[3].x = width + x + 3.0f;
    _VtxWideScreen[3].y = top;
    _VtxWideScreen[3].z = 0.0f;
    _VtxWideScreen[3].rhw = 1.0f;
    _VtxWideScreen[3].color = 0xaf0000ff;
    _VtxWideScreen[3].u = 1.0f;
    _VtxWideScreen[3].v = 1.0f;
    _VtxWideScreen[4] = _VtxWideScreen[2];
    _VtxWideScreen[5] = _VtxWideScreen[1];
    _VtxWideScreen[6].x = x - 2.0f;
    _VtxWideScreen[6].y = bottom;
    _VtxWideScreen[6].z = 0.0f;
    _VtxWideScreen[6].rhw = 1.0f;
    _VtxWideScreen[6].color = 0xaf0000ff;
    _VtxWideScreen[6].u = 0.0f;
    _VtxWideScreen[6].v = 0.0f;
    _VtxWideScreen[8].x = x - 2.0f;
    _VtxWideScreen[8].y = height + 3.0f;
    _VtxWideScreen[8].z = 0.0f;
    _VtxWideScreen[8].rhw = 1.0f;
    _VtxWideScreen[8].color = 0xaf0000ff;
    _VtxWideScreen[8].u = 0.0f;
    _VtxWideScreen[8].v = 1.0f;
    _VtxWideScreen[7].x = width + x + 3.0f;
    _VtxWideScreen[7].y = bottom;
    _VtxWideScreen[7].z = 0.0f;
    _VtxWideScreen[7].rhw = 1.0f;
    _VtxWideScreen[7].color = 0xaf0000ff;
    _VtxWideScreen[7].u = 1.0f;
    _VtxWideScreen[7].v = 0.0f;
    _VtxWideScreen[9].x = width + x + 3.0f;
    _VtxWideScreen[9].y = height + 3.0f;
    _VtxWideScreen[9].z = 0.0f;
    _VtxWideScreen[9].rhw = 1.0f;
    _VtxWideScreen[9].color = 0xaf0000ff;
    _VtxWideScreen[9].u = 1.0f;
    _VtxWideScreen[9].v = 1.0f;
    _VtxWideScreen[10] = _VtxWideScreen[8];
    _VtxWideScreen[11] = _VtxWideScreen[7];
}

// FUNCTION: GAME 0x5fa410
void G_IndicatorsClass::SetUnknown4484(float fade, uint color)
{
    unknown_4484 = fade;
    unknown_4488 = color;
}

// FUNCTION: GAME 0x5fa430
void G_IndicatorsClass::SetUnknownTexture(const char *name, bool unknown, bool reload)
{
    if (name == unknown_448c && !reload)
        return;
    if (unknown_4490)
        unknown_4490->Release();
    unknown_4490 = NULL;
    unknown_448c = name;
    unknown_4494 = unknown;
    if (name)
        igraph->CreateITexture(name, NULL, 0, &unknown_4490);
    else
        unknown_4494 = false;
}

// FUNCTION: GAME 0x5fa4a0
void G_IndicatorsClass::RadarClear()
{
    radar_cars.clear();
    radar_items.clear();
}

#pragma auto_inline(off)

// STUB: GAME 0x5fa510
void G_IndicatorsClass::RadarAddCar(C_car *car, uint color)
{
    StubLog("G_IndicatorsClass::RadarAddCar");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x5fa720
void G_IndicatorsClass::RadarRemoveCar(C_car *car)
{
    for (G_RadarCar *p = radar_cars.begin(); p < radar_cars.end(); p++)
    {
        if (p->car == car)
        {
            radar_cars.erase(p);
            return;
        }
    }
}

#pragma auto_inline(off)

// STUB: GAME 0x5fa9f0
void G_IndicatorsClass::RadarSetLeftEdge(std::vector<C_web_node *> *edge)
{
    StubLog("G_IndicatorsClass::RadarSetLeftEdge");
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x5fabf0
void G_IndicatorsClass::RadarSetRightEdge(std::vector<C_web_node *> *edge)
{
    StubLog("G_IndicatorsClass::RadarSetRightEdge");
}

#pragma auto_inline(on)

// PS2's shows any flag under 16. The hiding case assigns in PS2's order, which lets VC6 share its end with case 0's.
// FUNCTION: GAME 0x5fadf0
void G_IndicatorsClass::RaceSetStartFlag(uint flag)
{
    switch (flag)
    {
    case 0:
        unknown_40a4 |= INDF_RACE_START;
        race_start_flag_last = 0xff;
        race_start_flag = 0;
        race_start_last_time = 1.0f;
        race_start_time = 1.0f;
        break;
    case RACE_START_UNKNOWN_4:
    case RACE_START_NONE:
        unknown_40a4 &= ~INDF_RACE_START;
        race_start_flag_last = 0xff;
        race_start_last_time = 1.0f;
        race_start_flag = 0xff;
        race_start_time = 1.0f;
        break;
    default:
        unknown_40a4 |= INDF_RACE_START;
        race_start_flag_last = race_start_flag;
        race_start_last_time = race_start_time;
        race_start_flag = flag;
        race_start_time = 1.0f;
    }
}

// FUNCTION: GAME 0x5fb010
void G_IndicatorsClass::BonusSetCount(int count)
{
    bonus_count = count;
    if (count != BONUS_NONE)
        sprintf(bonus_text, textDatabase.GetText(0xb3), count);
    else
        bonus_text[0] = 0;
}

// Whether the camera has CAMPF_UNKNOWN_8 (the name is ours): the original tests the flag through an inline function,
// which keeps its shift (PS2 has no such function)
static inline bool CameraUnknown8(const G_Camera &camera)
{
    return (camera.flags >> 3) & 1;
}

// The whole HUD, block by block in the flags' order, each block's quads written into one vertex array: the frame shows
// 80 vertices at its bottom (esp + 0x8c), each block's other aggregates above them. The player's panel writes up to 88
// into it, over the unused space above. Open: the original's frame has two more dwords among the scalars, which are in
// another order, and its inline budget runs out where ours doesn't: it calls S_vector2's constructor and operators
// out of line in the map and from the race's start lights on, and the vectors' begin(), end() and size() at the end.
// The function is past the size where the budget depends on where source sits: 53 dead statements anywhere in it
// (`verts[0].x = verts[0].y * verts[0].z + verts[0].u;` in an `if (0)`) give exactly the original's out-of-line calls
// and 96% with registers and stack offsets masked, so the original had about that much more source compiling to
// nothing. No block's respellings that keep its code account for more than a few of them.
// FUNCTION: GAME 0x5fb060
void G_IndicatorsClass::DrawAll()
{
    TLVERTEX verts[80];
    igraph->SetState(DXS_ALPHA, ALPHASTATE_BLEND);
    driver->SetState(RS_USEZB, 0);
    // SetUnknownTexture's picture over the HUD's screen, a few pixels past its edges: drawn as it is with the fade over
    // it, or in the fade's colour
    if (unknown_4484 > 0.0f || unknown_4494)
    {
        if (unknown_4494)
        {
            verts[0].x = levyOkraj.x - 2.0f;
            verts[0].y = -2.0f;
            verts[0].z = 0.0f;
            verts[0].rhw = 1.0f;
            verts[0].color = 0xffffffff;
            verts[0].u = 0.0f;
            verts[0].v = 0.0f;
            verts[2].x = levyOkraj.x - 2.0f;
            verts[2].y = hud_size.y + 3.0f;
            verts[2].z = 0.0f;
            verts[2].rhw = 1.0f;
            verts[2].color = 0xffffffff;
            verts[2].u = 0.0f;
            verts[2].v = 1.0f;
            verts[1].x = levyOkraj.x + hud_size.x + 3.0f;
            verts[1].y = -2.0f;
            verts[1].z = 0.0f;
            verts[1].rhw = 1.0f;
            verts[1].color = 0xffffffff;
            verts[1].u = 1.0f;
            verts[1].v = 0.0f;
            verts[3].x = levyOkraj.x + hud_size.x + 3.0f;
            verts[3].y = hud_size.y + 3.0f;
            verts[3].z = 0.0f;
            verts[3].rhw = 1.0f;
            verts[3].color = 0xffffffff;
            verts[3].u = 1.0f;
            verts[3].v = 1.0f;
            igraph->SetTexture(unknown_4490);
            igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
            uint color = (uint)(unknown_4484 * 255.0f) << 24 | unknown_4488;
            verts[0].x = levyOkraj.x - 2.0f;
            verts[0].y = -2.0f;
            verts[0].z = 0.0f;
            verts[0].rhw = 1.0f;
            verts[0].color = color;
            verts[0].u = 0.0f;
            verts[0].v = 0.0f;
            verts[2].x = levyOkraj.x - 2.0f;
            verts[2].y = hud_size.y + 3.0f;
            verts[2].z = 0.0f;
            verts[2].rhw = 1.0f;
            verts[2].color = color;
            verts[2].u = 0.0f;
            verts[2].v = 1.0f;
            verts[1].x = levyOkraj.x + hud_size.x + 3.0f;
            verts[1].y = -2.0f;
            verts[1].z = 0.0f;
            verts[1].rhw = 1.0f;
            verts[1].color = color;
            verts[1].u = 1.0f;
            verts[1].v = 0.0f;
            verts[3].x = levyOkraj.x + hud_size.x + 3.0f;
            verts[3].y = hud_size.y + 3.0f;
            verts[3].z = 0.0f;
            verts[3].rhw = 1.0f;
            verts[3].color = color;
            verts[3].u = 1.0f;
            verts[3].v = 1.0f;
            igraph->SetTexture(NULL);
            igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
        }
        else
        {
            uint color = (uint)(unknown_4484 * 255.0f) << 24 | unknown_4488;
            verts[0].x = levyOkraj.x - 2.0f;
            verts[0].y = -2.0f;
            verts[0].z = 0.0f;
            verts[0].rhw = 1.0f;
            verts[0].color = color;
            verts[0].u = 0.0f;
            verts[0].v = 0.0f;
            verts[2].x = levyOkraj.x - 2.0f;
            verts[2].y = hud_size.y + 3.0f;
            verts[2].z = 0.0f;
            verts[2].rhw = 1.0f;
            verts[2].color = color;
            verts[2].u = 0.0f;
            verts[2].v = 1.0f;
            verts[1].x = levyOkraj.x + hud_size.x + 3.0f;
            verts[1].y = -2.0f;
            verts[1].z = 0.0f;
            verts[1].rhw = 1.0f;
            verts[1].color = color;
            verts[1].u = 1.0f;
            verts[1].v = 0.0f;
            verts[3].x = levyOkraj.x + hud_size.x + 3.0f;
            verts[3].y = hud_size.y + 3.0f;
            verts[3].z = 0.0f;
            verts[3].rhw = 1.0f;
            verts[3].color = color;
            verts[3].u = 1.0f;
            verts[3].v = 1.0f;
            igraph->SetTexture(unknown_4490);
            igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
        }
    }
    if (unknown_40a4 & INDF_WIDESCREEN)
    {
        igraph->SetTexture(NULL);
        igraph->DrawPrimitiveList(PT_TRIANGLELIST, 4, _VtxWideScreen, LS3D_STREAM_TLVERTEX);
    }
    if (unknown_40a4 & INDF_CREW && crew_quads)
    {
        igraph->SetTexture(textureCrew);
        igraph->DrawPrimitiveList(PT_TRIANGLELIST, crew_quads * 2, _VtxCrew, LS3D_STREAM_TLVERTEX);
    }
    if (unknown_40a4 & INDF_SNIPER)
    {
        // The scope over the whole screen, then its black lines from the left and right edges and from the bottom to the
        // middle. The scope's top right vertex is at 0, not -2 (PS2's too).
        verts[0].x = -2.0f;
        verts[0].y = -2.0f;
        verts[0].z = 0.0f;
        verts[0].rhw = 1.0f;
        verts[0].color = 0xffffffff;
        verts[0].u = 0.0f;
        verts[0].v = 32 / 256.0f;
        verts[2].x = -2.0f;
        verts[2].y = screen_size.y + 3.0f;
        verts[2].z = 0.0f;
        verts[2].rhw = 1.0f;
        verts[2].color = 0xffffffff;
        verts[2].u = 0.0f;
        verts[2].v = 224 / 256.0f;
        verts[1].x = screen_size.x + 3.0f;
        verts[1].y = 0.0f;
        verts[1].z = 0.0f;
        verts[1].rhw = 1.0f;
        verts[1].color = 0xffffffff;
        verts[1].u = 1.0f;
        verts[1].v = 32 / 256.0f;
        verts[3].x = screen_size.x + 3.0f;
        verts[3].y = screen_size.y + 3.0f;
        verts[3].z = 0.0f;
        verts[3].rhw = 1.0f;
        verts[3].color = 0xffffffff;
        verts[3].u = 1.0f;
        verts[3].v = 224 / 256.0f;
        igraph->SetTexture(texture_sight);
        if (texture_sight)
            igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
        float w = screen_scale.y * 1.5f;
        float cy = screen_size.y * .5f;
        float x = screen_size.x * .3f;
        float cx = screen_size.x * .5f;
        verts[0].x = 1.0f;
        verts[0].y = cy - w;
        verts[0].z = 0.0f;
        verts[0].rhw = 1.0f;
        verts[0].color = 0xff000000;
        verts[0].u = 1.0f;
        verts[0].v = 0.0f;
        verts[2].x = 1.0f;
        verts[2].y = cy + w;
        verts[2].z = 0.0f;
        verts[2].rhw = 1.0f;
        verts[2].color = 0xff000000;
        verts[2].u = 0.0f;
        verts[2].v = 0.0f;
        verts[1].x = x;
        verts[1].y = cy - w;
        verts[1].z = 0.0f;
        verts[1].rhw = 1.0f;
        verts[1].color = 0xff000000;
        verts[1].u = 1.0f;
        verts[1].v = 1.0f;
        verts[3].x = x;
        verts[3].y = cy + w;
        verts[3].z = 0.0f;
        verts[3].rhw = 1.0f;
        verts[3].color = 0xff000000;
        verts[3].u = 0.0f;
        verts[3].v = 1.0f;
        verts[4] = verts[2];
        verts[5] = verts[1];
        verts[6].x = screen_size.x - x;
        verts[6].y = cy - w;
        verts[6].z = 0.0f;
        verts[6].rhw = 1.0f;
        verts[6].color = 0xff000000;
        verts[6].u = 1.0f;
        verts[6].v = 0.0f;
        verts[8].x = screen_size.x - x;
        verts[8].y = cy + w;
        verts[8].z = 0.0f;
        verts[8].rhw = 1.0f;
        verts[8].color = 0xff000000;
        verts[8].u = 0.0f;
        verts[8].v = 0.0f;
        verts[7].x = screen_size.x - 2.0f;
        verts[7].y = cy - w;
        verts[7].z = 0.0f;
        verts[7].rhw = 1.0f;
        verts[7].color = 0xff000000;
        verts[7].u = 1.0f;
        verts[7].v = 1.0f;
        verts[9].x = screen_size.x - 2.0f;
        verts[9].y = cy + w;
        verts[9].z = 0.0f;
        verts[9].rhw = 1.0f;
        verts[9].color = 0xff000000;
        verts[9].u = 0.0f;
        verts[9].v = 1.0f;
        verts[10] = verts[8];
        verts[11] = verts[7];
        verts[12].x = cx - w;
        verts[12].y = cy;
        verts[12].z = 0.0f;
        verts[12].rhw = 1.0f;
        verts[12].color = 0xff000000;
        verts[12].u = 1.0f;
        verts[12].v = 0.0f;
        verts[14].x = cx - w;
        verts[14].y = screen_size.y - 2.0f;
        verts[14].z = 0.0f;
        verts[14].rhw = 1.0f;
        verts[14].color = 0xff000000;
        verts[14].u = 0.0f;
        verts[14].v = 0.0f;
        verts[13].x = cx + w;
        verts[13].y = cy;
        verts[13].z = 0.0f;
        verts[13].rhw = 1.0f;
        verts[13].color = 0xff000000;
        verts[13].u = 1.0f;
        verts[13].v = 1.0f;
        verts[15].x = cx + w;
        verts[15].y = screen_size.y - 2.0f;
        verts[15].z = 0.0f;
        verts[15].rhw = 1.0f;
        verts[15].color = 0xff000000;
        verts[15].u = 0.0f;
        verts[15].v = 1.0f;
        verts[16] = verts[14];
        verts[17] = verts[13];
        igraph->SetTexture(NULL);
        igraph->DrawPrimitiveList(PT_TRIANGLELIST, 6, verts, LS3D_STREAM_TLVERTEX);
    }
    if (!(unknown_40a4 & (INDF_MENU | INDF_UNKNOWN_800)))
    {
        if (unknown_40a4 & INDF_UNKNOWN_4000000)
        {
            // A part of 0textura.tga at the screen's top, centered; another value draws the vertices as they are
            float w, h, x, y;
            uint color = unknown_46e4;
            switch (unknown_46e0)
            {
            case 2:
                w = screen_scale.x * 117.0f;
                h = screen_scale.x * 59.0f;
                y = screen_size.y * .02;
                x = (screen_size.x - w) * .5;
                verts[0].x = x;
                verts[0].y = y;
                verts[0].z = 0.0f;
                verts[0].rhw = 1.0f;
                verts[0].color = color;
                verts[0].u = 0.0f;
                verts[0].v = 0.0f;
                verts[1].x = x + w;
                verts[1].y = y;
                verts[1].z = 0.0f;
                verts[1].rhw = 1.0f;
                verts[1].color = color;
                verts[1].u = 234 / 256.0f;
                verts[1].v = 0.0f;
                verts[2].x = x;
                verts[2].y = y + h;
                verts[2].z = 0.0f;
                verts[2].rhw = 1.0f;
                verts[2].color = color;
                verts[2].u = 0.0f;
                verts[2].v = 59 / 256.0f;
                verts[3].x = x + w;
                verts[3].y = y + h;
                verts[3].z = 0.0f;
                verts[3].rhw = 1.0f;
                verts[3].color = color;
                verts[3].u = 234 / 256.0f;
                verts[3].v = 59 / 256.0f;
                break;
            case 3:
                w = screen_scale.x * 64.0f;
                h = screen_scale.x * 71.0f;
                y = screen_size.y * .02;
                x = (screen_size.x - w) * .5;
                verts[0].x = x;
                verts[0].y = y;
                verts[0].z = 0.0f;
                verts[0].rhw = 1.0f;
                verts[0].color = color;
                verts[0].u = 0.0f;
                verts[0].v = 61 / 256.0f;
                verts[1].x = x + w;
                verts[1].y = y;
                verts[1].z = 0.0f;
                verts[1].rhw = 1.0f;
                verts[1].color = color;
                verts[1].u = 128 / 256.0f;
                verts[1].v = 61 / 256.0f;
                verts[2].x = x;
                verts[2].y = y + h;
                verts[2].z = 0.0f;
                verts[2].rhw = 1.0f;
                verts[2].color = color;
                verts[2].u = 0.0f;
                verts[2].v = 132 / 256.0f;
                verts[3].x = x + w;
                verts[3].y = y + h;
                verts[3].z = 0.0f;
                verts[3].rhw = 1.0f;
                verts[3].color = color;
                verts[3].u = 128 / 256.0f;
                verts[3].v = 132 / 256.0f;
                break;
            case 4:
                w = screen_scale.x * 58.0f;
                h = screen_scale.x * 77.0f;
                y = screen_size.y * .02;
                x = (screen_size.x - w) * .5;
                verts[0].x = x;
                verts[0].y = y;
                verts[0].z = 0.0f;
                verts[0].rhw = 1.0f;
                verts[0].color = color;
                verts[0].u = 130 / 256.0f;
                verts[0].v = 62 / 256.0f;
                verts[1].x = x + w;
                verts[1].y = y;
                verts[1].z = 0.0f;
                verts[1].rhw = 1.0f;
                verts[1].color = color;
                verts[1].u = 246 / 256.0f;
                verts[1].v = 62 / 256.0f;
                verts[2].x = x;
                verts[2].y = y + h;
                verts[2].z = 0.0f;
                verts[2].rhw = 1.0f;
                verts[2].color = color;
                verts[2].u = 130 / 256.0f;
                verts[2].v = 139 / 256.0f;
                verts[3].x = x + w;
                verts[3].y = y + h;
                verts[3].z = 0.0f;
                verts[3].rhw = 1.0f;
                verts[3].color = color;
                verts[3].u = 246 / 256.0f;
                verts[3].v = 139 / 256.0f;
                break;
            case 1:
                w = screen_scale.x * 79.0f;
                h = screen_scale.x * 70.0f;
                y = screen_size.y * .02;
                x = (screen_size.x - w) * .5;
                verts[0].x = x;
                verts[0].y = y;
                verts[0].z = 0.0f;
                verts[0].rhw = 1.0f;
                verts[0].color = color;
                verts[0].u = 48 / 256.0f;
                verts[0].v = 149 / 256.0f;
                verts[1].x = x + w;
                verts[1].y = y;
                verts[1].z = 0.0f;
                verts[1].rhw = 1.0f;
                verts[1].color = color;
                verts[1].u = 206 / 256.0f;
                verts[1].v = 149 / 256.0f;
                verts[2].x = x;
                verts[2].y = y + h;
                verts[2].z = 0.0f;
                verts[2].rhw = 1.0f;
                verts[2].color = color;
                verts[2].u = 48 / 256.0f;
                verts[2].v = 219 / 256.0f;
                verts[3].x = x + w;
                verts[3].y = y + h;
                verts[3].z = 0.0f;
                verts[3].rhw = 1.0f;
                verts[3].color = color;
                verts[3].u = 206 / 256.0f;
                verts[3].v = 219 / 256.0f;
                break;
            }
            igraph->SetTexture(texture_textura);
            igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
            if (unknown_40a4 & INDF_UNKNOWN_10)
            {
                // A bar under it, its frame and then its part for unknown_46e8 from 3% to 98%
                float x = screen_scale.x * 336.0f;
                float y = screen_scale.y * 92.0f;
                float w = screen_scale.x * 128.0f;
                float h = screen_scale.y * 16.0f;
                verts[0].x = x;
                verts[0].y = y;
                verts[0].z = 0.0f;
                verts[0].rhw = 1.0f;
                verts[0].color = 0xffffffff;
                verts[0].u = 0.0f;
                verts[0].v = 239 / 256.0f;
                verts[1].x = x + w;
                verts[1].y = y;
                verts[1].z = 0.0f;
                verts[1].rhw = 1.0f;
                verts[1].color = 0xffffffff;
                verts[1].u = 254 / 256.0f;
                verts[1].v = 239 / 256.0f;
                verts[2].x = x;
                verts[2].y = y + h;
                verts[2].z = 0.0f;
                verts[2].rhw = 1.0f;
                verts[2].color = 0xffffffff;
                verts[2].u = 0.0f;
                verts[2].v = 255 / 256.0f;
                verts[3].x = x + w;
                verts[3].y = y + h;
                verts[3].z = 0.0f;
                verts[3].rhw = 1.0f;
                verts[3].color = 0xffffffff;
                verts[3].u = 254 / 256.0f;
                verts[3].v = 255 / 256.0f;
                igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
                verts[0].x = x;
                verts[0].y = y;
                verts[0].z = 0.0f;
                verts[0].rhw = 1.0f;
                verts[0].color = 0xffffffff;
                verts[0].u = 0.0f;
                verts[0].v = 222 / 256.0f;
                verts[1].x = x + (float)(unknown_46e8 * .95 + .03) * screen_scale.x * 128.0f;
                verts[1].y = y;
                verts[1].z = 0.0f;
                verts[1].rhw = 1.0f;
                verts[1].color = 0xffffffff;
                verts[1].u = (float)(unknown_46e8 * .95 + .03) * 127.0f / 128.0f;
                verts[1].v = 222 / 256.0f;
                verts[2].x = x;
                verts[2].y = y + h;
                verts[2].z = 0.0f;
                verts[2].rhw = 1.0f;
                verts[2].color = 0xffffffff;
                verts[2].u = 0.0f;
                verts[2].v = 238 / 256.0f;
                verts[3].x = x + (float)(unknown_46e8 * .95 + .03) * screen_scale.x * 128.0f;
                verts[3].y = y + h;
                verts[3].z = 0.0f;
                verts[3].rhw = 1.0f;
                verts[3].color = 0xffffffff;
                verts[3].u = (float)(unknown_46e8 * .95 + .03) * 127.0f / 128.0f;
                verts[3].v = 238 / 256.0f;
                igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
            }
        }
        if (unknown_40a4 & INDF_CROSSHAIR && !CameraUnknown8(act_mission->game->camera))
        {
            // Lines around screen_center, spread by the accuracy: the profile's cross, the point or both
            float size = screen_scale.x * 10.0f;
            float spread = accuracy * 4.0f * screen_scale.x;
            switch (mafiaLoadSave.settings.cursor)
            {
            case CURSOR_CROSSHAIR:
                verts[0].x = screen_center.x + spread;
                verts[0].y = screen_center.y;
                verts[0].z = 0.0f;
                verts[0].rhw = 1.0f;
                verts[0].color = 0xffffffff;
                verts[0].u = 0.0f;
                verts[0].v = 0.0f;
                verts[1].x = (screen_center.x + size) + spread;
                verts[1].y = screen_center.y;
                verts[1].z = 0.0f;
                verts[1].rhw = 1.0f;
                verts[1].color = 0xffffffff;
                verts[1].u = 0.0f;
                verts[1].v = 0.0f;
                verts[2].x = screen_center.x - spread;
                verts[2].y = screen_center.y;
                verts[2].z = 0.0f;
                verts[2].rhw = 1.0f;
                verts[2].color = 0xffffffff;
                verts[2].u = 0.0f;
                verts[2].v = 0.0f;
                verts[3].x = screen_center.x - spread - size;
                verts[3].y = screen_center.y;
                verts[3].z = 0.0f;
                verts[3].rhw = 1.0f;
                verts[3].color = 0xffffffff;
                verts[3].u = 0.0f;
                verts[3].v = 0.0f;
                verts[4].x = screen_center.x;
                verts[4].y = screen_center.y - spread;
                verts[4].z = 0.0f;
                verts[4].rhw = 1.0f;
                verts[4].color = 0xffffffff;
                verts[4].u = 0.0f;
                verts[4].v = 0.0f;
                verts[5].x = screen_center.x;
                verts[5].y = screen_center.y - spread - size;
                verts[5].z = 0.0f;
                verts[5].rhw = 1.0f;
                verts[5].color = 0xffffffff;
                verts[5].u = 0.0f;
                verts[5].v = 0.0f;
                verts[6].x = screen_center.x;
                verts[6].y = screen_center.y + spread;
                verts[6].z = 0.0f;
                verts[6].rhw = 1.0f;
                verts[6].color = 0xffffffff;
                verts[6].u = 0.0f;
                verts[6].v = 0.0f;
                verts[7].x = screen_center.x;
                verts[7].y = (screen_center.y + size) + spread;
                verts[7].z = 0.0f;
                verts[7].rhw = 1.0f;
                verts[7].color = 0xffffffff;
                verts[7].u = 0.0f;
                verts[7].v = 0.0f;
                GetIGraph()->SetTexture(NULL);
                igraph->DrawPrimitiveList(PT_LINELIST, 4, verts, LS3D_STREAM_TLVERTEX);
                break;
            case CURSOR_CENTER_POINT:
                verts[0].x = screen_center.x;
                verts[0].y = screen_center.y;
                verts[0].z = 0.0f;
                verts[0].rhw = 1.0f;
                verts[0].color = 0xffffffff;
                verts[0].u = 0.0f;
                verts[0].v = 0.0f;
                verts[1].x = screen_center.x + 1.0f;
                verts[1].y = screen_center.y + 1.0f;
                verts[1].z = 0.0f;
                verts[1].rhw = 1.0f;
                verts[1].color = 0xffffffff;
                verts[1].u = 0.0f;
                verts[1].v = 0.0f;
                GetIGraph()->SetTexture(NULL);
                igraph->DrawPrimitiveList(PT_LINELIST, 1, verts, LS3D_STREAM_TLVERTEX);
                break;
            default:
                verts[0].x = screen_center.x + spread;
                verts[0].y = screen_center.y;
                verts[0].z = 0.0f;
                verts[0].rhw = 1.0f;
                verts[0].color = 0xffffffff;
                verts[0].u = 0.0f;
                verts[0].v = 0.0f;
                verts[1].x = (screen_center.x + size) + spread;
                verts[1].y = screen_center.y;
                verts[1].z = 0.0f;
                verts[1].rhw = 1.0f;
                verts[1].color = 0xffffffff;
                verts[1].u = 0.0f;
                verts[1].v = 0.0f;
                verts[2].x = screen_center.x - spread;
                verts[2].y = screen_center.y;
                verts[2].z = 0.0f;
                verts[2].rhw = 1.0f;
                verts[2].color = 0xffffffff;
                verts[2].u = 0.0f;
                verts[2].v = 0.0f;
                verts[3].x = screen_center.x - spread - size;
                verts[3].y = screen_center.y;
                verts[3].z = 0.0f;
                verts[3].rhw = 1.0f;
                verts[3].color = 0xffffffff;
                verts[3].u = 0.0f;
                verts[3].v = 0.0f;
                verts[4].x = screen_center.x;
                verts[4].y = screen_center.y - spread;
                verts[4].z = 0.0f;
                verts[4].rhw = 1.0f;
                verts[4].color = 0xffffffff;
                verts[4].u = 0.0f;
                verts[4].v = 0.0f;
                verts[5].x = screen_center.x;
                verts[5].y = screen_center.y - spread - size;
                verts[5].z = 0.0f;
                verts[5].rhw = 1.0f;
                verts[5].color = 0xffffffff;
                verts[5].u = 0.0f;
                verts[5].v = 0.0f;
                verts[6].x = screen_center.x;
                verts[6].y = screen_center.y + spread;
                verts[6].z = 0.0f;
                verts[6].rhw = 1.0f;
                verts[6].color = 0xffffffff;
                verts[6].u = 0.0f;
                verts[6].v = 0.0f;
                verts[7].x = screen_center.x;
                verts[7].y = (screen_center.y + size) + spread;
                verts[7].z = 0.0f;
                verts[7].rhw = 1.0f;
                verts[7].color = 0xffffffff;
                verts[7].u = 0.0f;
                verts[7].v = 0.0f;
                verts[8].x = screen_center.x;
                verts[8].y = screen_center.y;
                verts[8].z = 0.0f;
                verts[8].rhw = 1.0f;
                verts[8].color = 0xffffffff;
                verts[8].u = 0.0f;
                verts[8].v = 0.0f;
                verts[9].x = screen_center.x + 1.0f;
                verts[9].y = screen_center.y + 1.0f;
                verts[9].z = 0.0f;
                verts[9].rhw = 1.0f;
                verts[9].color = 0xffffffff;
                verts[9].u = 0.0f;
                verts[9].v = 0.0f;
                GetIGraph()->SetTexture(NULL);
                igraph->DrawPrimitiveList(PT_LINELIST, 5, verts, LS3D_STREAM_TLVERTEX);
            }
        }
        if (unknown_40a4 & INDF_TIMER)
        {
            igraph->SetTexture(texture_int2);
            igraph->DrawPrimitiveList(PT_TRIANGLELIST, 2, _VtxTimerCifernik, LS3D_STREAM_TLVERTEX);
            igraph->SetTexture(NULL);
            igraph->DrawPrimitiveList(PT_TRIANGLELIST, 6, &_VtxTimerCifernik[6], LS3D_STREAM_TLVERTEX);
            // The interval's sweep, a fan of both sides
            if (_VtxTimerIntervalNumOfVertexes && interval)
            {
                uint cull = igraph->GetState(DXS_CULL);
                igraph->SetState(DXS_CULL, D3DCULL_NONE);
                igraph->DrawPrimitiveList(PT_TRIANGLEFAN, _VtxTimerIntervalNumOfVertexes - 2, _VtxTimerInterval,
                    LS3D_STREAM_TLVERTEX);
                igraph->SetState(DXS_CULL, cull);
            }
        }
        if (unknown_40a4 & INDF_TACHOMETR)
        {
            // The dials and the digits, with INDF_UNKNOWN_100 the light; the small display, the needles, the gear's icon
            igraph->SetTexture(texture_int2);
            if (unknown_40a4 & INDF_UNKNOWN_100)
                igraph->DrawPrimitiveList(PT_TRIANGLELIST, 16, _VtxTachometrCiselnik, LS3D_STREAM_TLVERTEX);
            else
                igraph->DrawPrimitiveList(PT_TRIANGLELIST, 14, _VtxTachometrCiselnik, LS3D_STREAM_TLVERTEX);
            igraph->SetTexture(texture_int);
            igraph->DrawPrimitiveList(PT_TRIANGLELIST, 2, &_VtxTachometrCiselnik[48], LS3D_STREAM_TLVERTEX);
            igraph->SetTexture(NULL);
            igraph->DrawPrimitiveList(PT_TRIANGLELIST, 6, _VtxTachometrRafiky, LS3D_STREAM_TLVERTEX);
            float x = pravyOkraj.x - screen_scale.x * 42.0f;
            float y = pravyOkraj.y - screen_scale.y * 42.0f;
            float w = screen_scale.x * 32.0f;
            float h = screen_scale.y * 32.0f;
            if (unknown_4204)
            {
                verts[0].x = x;
                verts[0].y = y;
                verts[0].z = 0.0f;
                verts[0].rhw = 1.0f;
                verts[0].color = 0xffffffff;
                verts[0].u = 0.0f;
                verts[0].v = 0.0f;
                verts[1].x = x + w;
                verts[1].y = y;
                verts[1].z = 0.0f;
                verts[1].rhw = 1.0f;
                verts[1].color = 0xffffffff;
                verts[1].u = .5f;
                verts[1].v = 0.0f;
                verts[2].x = x;
                verts[2].y = y + h;
                verts[2].z = 0.0f;
                verts[2].rhw = 1.0f;
                verts[2].color = 0xffffffff;
                verts[2].u = 0.0f;
                verts[2].v = 1.0f;
                verts[3].x = x + w;
                verts[3].y = y + h;
                verts[3].z = 0.0f;
                verts[3].rhw = 1.0f;
                verts[3].color = 0xffffffff;
                verts[3].u = .5f;
                verts[3].v = 1.0f;
            }
            else
            {
                verts[0].x = x;
                verts[0].y = y;
                verts[0].z = 0.0f;
                verts[0].rhw = 1.0f;
                verts[0].color = 0xffffffff;
                verts[0].u = .5f;
                verts[0].v = 0.0f;
                verts[1].x = x + w;
                verts[1].y = y;
                verts[1].z = 0.0f;
                verts[1].rhw = 1.0f;
                verts[1].color = 0xffffffff;
                verts[1].u = 1.0f;
                verts[1].v = 0.0f;
                verts[2].x = x;
                verts[2].y = y + h;
                verts[2].z = 0.0f;
                verts[2].rhw = 1.0f;
                verts[2].color = 0xffffffff;
                verts[2].u = .5f;
                verts[2].v = 1.0f;
                verts[3].x = x + w;
                verts[3].y = y + h;
                verts[3].z = 0.0f;
                verts[3].rhw = 1.0f;
                verts[3].color = 0xffffffff;
                verts[3].u = 1.0f;
                verts[3].v = 1.0f;
            }
            igraph->SetTexture(textureIcons);
            igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
            if (fuel_low && unknown_4208 < .25f)
            {
                float x = pravyOkraj.x - screen_scale.x * 132.0f;
                float y = pravyOkraj.y - screen_scale.y * 41.0f;
                float w = screen_scale.x * 16.0f;
                float h = screen_scale.y * 16.0f;
                verts[0].x = x;
                verts[0].y = y;
                verts[0].z = 0.0f;
                verts[0].rhw = 1.0f;
                verts[0].color = 0xffffffff;
                verts[0].u = 0.0f;
                verts[0].v = 0.0f;
                verts[1].x = x + w;
                verts[1].y = y;
                verts[1].z = 0.0f;
                verts[1].rhw = 1.0f;
                verts[1].color = 0xffffffff;
                verts[1].u = 1.0f;
                verts[1].v = 0.0f;
                verts[2].x = x;
                verts[2].y = y + h;
                verts[2].z = 0.0f;
                verts[2].rhw = 1.0f;
                verts[2].color = 0xffffffff;
                verts[2].u = 0.0f;
                verts[2].v = 1.0f;
                verts[3].x = x + w;
                verts[3].y = y + h;
                verts[3].z = 0.0f;
                verts[3].rhw = 1.0f;
                verts[3].color = 0xffffffff;
                verts[3].u = 1.0f;
                verts[3].v = 1.0f;
                igraph->SetTexture(textureBenzin);
                igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
            }
        }
        if (unknown_40a4 & INDF_UNKNOWN_10000)
        {
            // A bar at the bottom's middle from 1int.tga: its frame, then its part for unknown_420c from 3% to 98%
            float x = (levyOkraj.x + pravyOkraj.x) * .5f - screen_scale.x * 32.0f;
            float y = pravyOkraj.y - screen_scale.y * 32.0f;
            float w = screen_scale.x * 63.0f;
            float h = screen_scale.y * 23.0f;
            verts[0].x = x;
            verts[0].y = y;
            verts[0].z = 0.0f;
            verts[0].rhw = 1.0f;
            verts[0].color = 0xffffffff;
            verts[0].u = 0.0f;
            verts[0].v = 48 / 256.0f;
            verts[1].x = x + w;
            verts[1].y = y;
            verts[1].z = 0.0f;
            verts[1].rhw = 1.0f;
            verts[1].color = 0xffffffff;
            verts[1].u = 252 / 256.0f;
            verts[1].v = 48 / 256.0f;
            verts[2].x = x;
            verts[2].y = y + h;
            verts[2].z = 0.0f;
            verts[2].rhw = 1.0f;
            verts[2].color = 0xffffffff;
            verts[2].u = 0.0f;
            verts[2].v = 72 / 256.0f;
            verts[3].x = x + w;
            verts[3].y = y + h;
            verts[3].z = 0.0f;
            verts[3].rhw = 1.0f;
            verts[3].color = 0xffffffff;
            verts[3].u = 252 / 256.0f;
            verts[3].v = 72 / 256.0f;
            igraph->SetTexture(texture_int);
            igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
            verts[0].x = x;
            verts[0].y = y;
            verts[0].z = 0.0f;
            verts[0].rhw = 1.0f;
            verts[0].color = 0xffffffff;
            verts[0].u = 0.0f;
            verts[0].v = 24 / 256.0f;
            verts[1].x = x + (float)(unknown_420c * .95 + .03) * screen_scale.x * 63.0f;
            verts[1].y = y;
            verts[1].z = 0.0f;
            verts[1].rhw = 1.0f;
            verts[1].color = 0xffffffff;
            verts[1].u = (float)(unknown_420c * .95 + .03) * 63.0f / 64.0f;
            verts[1].v = 24 / 256.0f;
            verts[2].x = x;
            verts[2].y = y + h;
            verts[2].z = 0.0f;
            verts[2].rhw = 1.0f;
            verts[2].color = 0xffffffff;
            verts[2].u = 0.0f;
            verts[2].v = 48 / 256.0f;
            verts[3].x = x + (float)(unknown_420c * .95 + .03) * screen_scale.x * 63.0f;
            verts[3].y = y + h;
            verts[3].z = 0.0f;
            verts[3].rhw = 1.0f;
            verts[3].color = 0xffffffff;
            verts[3].u = (float)(unknown_420c * .95 + .03) * 63.0f / 64.0f;
            verts[3].v = 48 / 256.0f;
            igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
        }
        if (bonus_count != BONUS_NONE)
            OutText(bonus_text, levyOkraj.x + screen_scale.x * 760.0f, screen_scale.y * 20.0f, screen_scale.x * 20.0f,
                screen_scale.y * 40.0f, 0xffffffff, OUTTEXT_RIGHT, 1, NULL);
        // The player's panel: the rows' backgrounds, then their numbers, the player's at the bottom and a row above it
        // for each wingman shown; rows counts them for what goes above the panel
        uint rows = 0;
        if (unknown_40a4 & INDF_PLAYER)
        {
            uint count = 0;
            _PridejWingPodklad(verts, &count, rows, 0);
            if (unknown_40a4 & INDF_AMMO)
                _PridejAmmoPodklad(verts, &count);
            rows++;
            if (unknown_40a4 & INDF_WINGMAN1)
                _PridejWingPodklad(verts, &count, rows++, wingman_names[0]);
            if (unknown_40a4 & INDF_WINGMAN2)
                _PridejWingPodklad(verts, &count, rows++, wingman_names[1]);
            if (unknown_40a4 & INDF_WINGMAN3)
                _PridejWingPodklad(verts, &count, rows++, wingman_names[2]);
            if (unknown_40a4 & INDF_WINGMAN4)
                _PridejWingPodklad(verts, &count, rows++, wingman_names[3]);
            igraph->SetTexture(texture_int);
            igraph->DrawIndexedPrimitiveList(PT_TRIANGLELIST, count * 3, __idx, count, verts, LS3D_STREAM_TLVERTEX);
            count = 0;
            rows = 0;
            _PridejWingLives(verts, &count, rows, lives);
            if (unknown_40a4 & INDF_AMMO)
                _PridejAmmo(verts, &count, ammo, ammo_spare);
            rows++;
            if (unknown_40a4 & INDF_WINGMAN1)
                _PridejWingLives(verts, &count, rows++, wingman_lives[0]);
            if (unknown_40a4 & INDF_WINGMAN2)
                _PridejWingLives(verts, &count, rows++, wingman_lives[1]);
            if (unknown_40a4 & INDF_WINGMAN3)
                _PridejWingLives(verts, &count, rows++, wingman_lives[2]);
            if (unknown_40a4 & INDF_WINGMAN4)
                _PridejWingLives(verts, &count, rows++, wingman_lives[3]);
            igraph->SetTexture(texture_numbers);
            igraph->DrawIndexedPrimitiveList(PT_TRIANGLELIST, count * 3, __idx, count, verts, LS3D_STREAM_TLVERTEX);
        }
        // An icon of the dials' texture above the panel, 33 pixels square, unknown_4244 its alpha
        if (!(unknown_40a4 & INDF_MENU) && unknown_4244)
        {
            uint color = unknown_4244 << 24 | 0xffffff;
            float x = screen_scale.x * 9.0f + levyOkraj.x;
            float y = (float)(552 - rows * 26) * screen_scale.y;
            float sx = screen_scale.x * 33.0f;
            float sy = screen_scale.y * 33.0f;
            verts[0].x = x;
            verts[0].y = y;
            verts[0].z = 0.0f;
            verts[0].rhw = 1.0f;
            verts[0].color = color;
            verts[0].u = 1 / 256.0f;
            verts[0].v = 113 / 256.0f;
            verts[1].x = x + sx;
            verts[1].y = y;
            verts[1].z = 0.0f;
            verts[1].rhw = 1.0f;
            verts[1].color = color;
            verts[1].u = 34 / 256.0f;
            verts[1].v = 113 / 256.0f;
            verts[2].x = x;
            verts[2].y = y + sy;
            verts[2].z = 0.0f;
            verts[2].rhw = 1.0f;
            verts[2].color = color;
            verts[2].u = 1 / 256.0f;
            verts[2].v = 146 / 256.0f;
            verts[3].x = x + sx;
            verts[3].y = y + sy;
            verts[3].z = 0.0f;
            verts[3].rhw = 1.0f;
            verts[3].color = color;
            verts[3].u = 34 / 256.0f;
            verts[3].v = 146 / 256.0f;
            igraph->SetTexture(texture_int2);
            igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
        }
        // The console's lines above the panel, the oldest at the top, each as opaque as its time left
        if (console_count && !(unknown_40a4 & INDF_MENU))
        {
            float y = (547 - rows * 26 - console_count * 20 * .8f) * screen_scale.y;
            for (uint i = 0; i < console_count; i++)
            {
                uint line = (console_first + i) % CONSOLE_LINES;
                uint alpha = (uint)(console_times[line] * 255.0f);
                if (alpha > 0xff)
                    alpha = 0xff;
                OutText(console_texts[line], screen_scale.x * 9.0f + levyOkraj.x, y, 10.0f, screen_scale.y * 20.0f,
                    console_colors[line] | alpha << 24, 0, 3, NULL);
                y += screen_scale.y * 16.0f;
            }
        }
        // PS2's thief indicator: a bar in a white frame, red from its left end to thief_indicator's, more so further on
        if (unknown_40a4 & INDF_THIEF)
        {
            S_vector2 pos = levyOkraj + S_vector2(250.0f, 500.0f) * screen_scale;
            S_vector2 size = S_vector2(300.0f, 10.0f) * screen_scale;
            verts[0].x = pos.x - 2.0f;
            verts[0].y = pos.y - 2.0f;
            verts[0].z = 0.0f;
            verts[0].rhw = 1.0f;
            verts[0].color = 0xffffffff;
            verts[0].u = 0.0f;
            verts[0].v = 0.0f;
            verts[1].x = pos.x + size.x + 1.0f;
            verts[1].y = pos.y - 2.0f;
            verts[1].z = 0.0f;
            verts[1].rhw = 1.0f;
            verts[1].color = 0xffffffff;
            verts[1].u = 0.0f;
            verts[1].v = 0.0f;
            verts[2].x = pos.x + size.x + 1.0f;
            verts[2].y = pos.y + size.y + 1.0f;
            verts[2].z = 0.0f;
            verts[2].rhw = 1.0f;
            verts[2].color = 0xffffffff;
            verts[2].u = 0.0f;
            verts[2].v = 0.0f;
            verts[3].x = pos.x - 2.0f;
            verts[3].y = pos.y + size.y + 1.0f;
            verts[3].z = 0.0f;
            verts[3].rhw = 1.0f;
            verts[3].color = 0xffffffff;
            verts[3].u = 0.0f;
            verts[3].v = 0.0f;
            verts[4].x = pos.x - 2.0f;
            verts[4].y = pos.y - 2.0f;
            verts[4].z = 0.0f;
            verts[4].rhw = 1.0f;
            verts[4].color = 0xffffffff;
            verts[4].u = 0.0f;
            verts[4].v = 0.0f;
            igraph->SetTexture(NULL);
            igraph->DrawPrimitiveList(PT_LINESTRIP, 4, verts, LS3D_STREAM_TLVERTEX);
            size.x *= thief_indicator;
            uint color = 0xdf000000 | (uchar)(thief_indicator * 255.0f) << 16;
            verts[0].x = pos.x;
            verts[0].y = pos.y;
            verts[0].z = 0.0f;
            verts[0].rhw = 1.0f;
            verts[0].color = 0xdf3f0000;
            verts[0].u = 0.0f;
            verts[0].v = 0.0f;
            verts[1].x = pos.x + size.x;
            verts[1].y = pos.y;
            verts[1].z = 0.0f;
            verts[1].rhw = 1.0f;
            verts[1].color = color;
            verts[1].u = 1.0f;
            verts[1].v = 0.0f;
            verts[2].x = pos.x;
            verts[2].y = pos.y + size.y;
            verts[2].z = 0.0f;
            verts[2].rhw = 1.0f;
            verts[2].color = 0xdf3f0000;
            verts[2].u = 0.0f;
            verts[2].v = 1.0f;
            verts[3].x = pos.x + size.x;
            verts[3].y = pos.y + size.y;
            verts[3].z = 0.0f;
            verts[3].rhw = 1.0f;
            verts[3].color = color;
            verts[3].u = 1.0f;
            verts[3].v = 1.0f;
            igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
        }
        // PS2's refulling indicator, the same as the thief's
        if (unknown_40a4 & INDF_REFULLING)
        {
            S_vector2 pos = levyOkraj + S_vector2(350.0f, 20.0f) * screen_scale;
            S_vector2 size = S_vector2(100.0f, 8.0f) * screen_scale;
            verts[0].x = pos.x - 2.0f;
            verts[0].y = pos.y - 2.0f;
            verts[0].z = 0.0f;
            verts[0].rhw = 1.0f;
            verts[0].color = 0xffffffff;
            verts[0].u = 0.0f;
            verts[0].v = 0.0f;
            verts[1].x = pos.x + size.x + 1.0f;
            verts[1].y = pos.y - 2.0f;
            verts[1].z = 0.0f;
            verts[1].rhw = 1.0f;
            verts[1].color = 0xffffffff;
            verts[1].u = 0.0f;
            verts[1].v = 0.0f;
            verts[2].x = pos.x + size.x + 1.0f;
            verts[2].y = pos.y + size.y + 1.0f;
            verts[2].z = 0.0f;
            verts[2].rhw = 1.0f;
            verts[2].color = 0xffffffff;
            verts[2].u = 0.0f;
            verts[2].v = 0.0f;
            verts[3].x = pos.x - 2.0f;
            verts[3].y = pos.y + size.y + 1.0f;
            verts[3].z = 0.0f;
            verts[3].rhw = 1.0f;
            verts[3].color = 0xffffffff;
            verts[3].u = 0.0f;
            verts[3].v = 0.0f;
            verts[4].x = pos.x - 2.0f;
            verts[4].y = pos.y - 2.0f;
            verts[4].z = 0.0f;
            verts[4].rhw = 1.0f;
            verts[4].color = 0xffffffff;
            verts[4].u = 0.0f;
            verts[4].v = 0.0f;
            igraph->SetTexture(NULL);
            igraph->DrawPrimitiveList(PT_LINESTRIP, 4, verts, LS3D_STREAM_TLVERTEX);
            size.x *= refulling_indicator;
            uint color = 0xdf000000 | (uchar)(refulling_indicator * 255.0f) << 16;
            verts[0].x = pos.x;
            verts[0].y = pos.y;
            verts[0].z = 0.0f;
            verts[0].rhw = 1.0f;
            verts[0].color = 0xdf3f0000;
            verts[0].u = 0.0f;
            verts[0].v = 0.0f;
            verts[1].x = pos.x + size.x;
            verts[1].y = pos.y;
            verts[1].z = 0.0f;
            verts[1].rhw = 1.0f;
            verts[1].color = color;
            verts[1].u = 1.0f;
            verts[1].v = 0.0f;
            verts[2].x = pos.x;
            verts[2].y = pos.y + size.y;
            verts[2].z = 0.0f;
            verts[2].rhw = 1.0f;
            verts[2].color = 0xdf3f0000;
            verts[2].u = 0.0f;
            verts[2].v = 1.0f;
            verts[3].x = pos.x + size.x;
            verts[3].y = pos.y + size.y;
            verts[3].z = 0.0f;
            verts[3].rhw = 1.0f;
            verts[3].color = color;
            verts[3].u = 1.0f;
            verts[3].v = 1.0f;
            igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
        }
    }
    // The mission's help texts, centred on the HUD's screen, the last at the bottom; the flag goes once there are none
    if (!(unknown_40a4 & INDF_MENU) && unknown_40a4 & INDF_MISSION_HELP)
    {
        if (!mission_help.empty())
        {
            float cx = screen_size.x / 2;
            if (parheliaIndex == PARHELIA_DUAL)
                cx = hud_size.x / 2;
            float y = screen_scale.y * 190.0f;
            float h = screen_scale.y * 22.0f;
            y += mission_help.size() * h * .5f;
            for (G_MISubtitleStruct *s = mission_help.end(); s > mission_help.begin();)
            {
                s--;
                uint color = s->alpha > 0xff ? 0xffffffff : s->alpha << 24 | 0xffffff;
                if (!s->hidden)
                {
                    OutText(s->text, cx - 10.0f, y, 20.0f, h, color, OUTTEXT_CENTER, 0, s->end);
                    y -= h;
                }
            }
        }
        else
            unknown_40a4 &= ~INDF_MISSION_HELP;
    }
    if (!(unknown_40a4 & (INDF_MENU | INDF_UNKNOWN_800)))
    {
        // The radar: the cars, the things and the road's edges around the player's car (or the camera), turned with the
        // camera, inside radarclp.bmp's frame
        if (unknown_40a4 & INDF_RADAR)
        {
            LS3D_VIEWPORT vp = *igraph->GetViewPort();
            int use_zb = driver->GetState(RS_USEZB);
            driver->SetState(RS_USEZB, 1);
            igraph->SetState(DXS_ZFUNC, 0);
            S_vector2 pos = levyOkraj + screen_scale * 29.0f;
            S_vector2 size = screen_scale * 144.0f;
            verts[0].x = pos.x;
            verts[0].y = pos.y;
            verts[0].z = .5f;
            verts[0].rhw = .2f;
            verts[0].color = 0xbfffffff;
            verts[0].u = 1 / 256.0f;
            verts[0].v = 148 / 256.0f;
            verts[1].x = pos.x + size.x;
            verts[1].y = pos.y;
            verts[1].z = .5f;
            verts[1].rhw = .2f;
            verts[1].color = 0xbfffffff;
            verts[1].u = 109 / 256.0f;
            verts[1].v = 148 / 256.0f;
            verts[2].x = pos.x;
            verts[2].y = pos.y + size.y;
            verts[2].z = .5f;
            verts[2].rhw = .2f;
            verts[2].color = 0xbfffffff;
            verts[2].u = 1 / 256.0f;
            verts[2].v = 255 / 256.0f;
            verts[3].x = pos.x + size.x;
            verts[3].y = pos.y + size.y;
            verts[3].z = .5f;
            verts[3].rhw = .2f;
            verts[3].color = 0xbfffffff;
            verts[3].u = 109 / 256.0f;
            verts[3].v = 255 / 256.0f;
            igraph->SetTexture(texture_int2);
            igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
            // The frame, at the front of the z-buffer: what is drawn behind it doesn't show
            verts[0].x = pos.x;
            verts[0].y = pos.y;
            verts[0].z = 0.0f;
            verts[0].rhw = 1.0f;
            verts[0].color = 0x1000000;
            verts[0].u = 0.0f;
            verts[0].v = 0.0f;
            verts[1].x = pos.x + size.x;
            verts[1].y = pos.y;
            verts[1].z = 0.0f;
            verts[1].rhw = 1.0f;
            verts[1].color = 0x1000000;
            verts[1].u = 1.0f;
            verts[1].v = 0.0f;
            verts[2].x = pos.x;
            verts[2].y = pos.y + size.y;
            verts[2].z = 0.0f;
            verts[2].rhw = 1.0f;
            verts[2].color = 0x1000000;
            verts[2].u = 0.0f;
            verts[2].v = 1.0f;
            verts[3].x = pos.x + size.x;
            verts[3].y = pos.y + size.y;
            verts[3].z = 0.0f;
            verts[3].rhw = 1.0f;
            verts[3].color = 0x1000000;
            verts[3].u = 1.0f;
            verts[3].v = 1.0f;
            igraph->SetTexture(textureRadarClip);
            igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
            LS3D_VIEWPORT radar_vp;
            radar_vp.x = (int)(pos.x + 2.0f);
            radar_vp.y = (int)(pos.y + 2.0f);
            radar_vp.width = (int)(size.x - 4.0f);
            radar_vp.height = (int)(size.y - 4.0f);
            radar_vp.min_z = 0.0f;
            radar_vp.max_z = 1.0f;
            igraph->SetViewPort(&radar_vp);
            // The world turned with the camera's direction in the ground plane, around the player's car
            S_vector dir = act_mission->GetScene()->current_camera->GetWorldDir();
            dir.y = 0.0f;
            S_matrix m;
            m.Identity();
            m.SetDir(dir);
            if (radar_player_car)
                m(3) = radar_player_car->GetWorldPos();
            m.Inverse(S_matrix(m));
            S_vector center(pos.x + size.x * .5, 0.0f, pos.y + size.y * .5);
            igraph->SetTexture(NULL);
            igraph->SetState(DXS_ZFUNC, 1);
            S_matrix mat;
            S_vector corners[8];
            // Each car's box from above: the player's, and the others while their frames are on
            for (G_RadarCar *rc = radar_cars.begin(); rc < radar_cars.end(); rc++)
            {
                uint color = rc->color;
                if (radar_player_car == rc->car || rc->car->frame->m_FrmFlags & FRMFLAGS_ON)
                {
                    mat = rc->car->frame->GetLocalMatrix() * m;
                    S_vector min = -rc->car->GetVehiclePivotPos(false);
                    S_vector max = rc->car->GetVehicleSize(false) - rc->car->GetVehiclePivotPos(false);
                    S_vector *c = corners;
                    for (int i = 0; i < 2; i++)
                    {
                        for (int j = 0; j < 2; j++)
                        {
                            c->x = i ? min.x : max.x;
                            c->y = j ? min.y : max.y;
                            c->z = min.z;
                            c++;
                            c->x = i ? min.x : max.x;
                            c->y = j ? min.y : max.y;
                            c->z = max.z;
                            c++;
                        }
                    }
                    for (int k = 0; k < 8; k++)
                        corners[k] = center - (corners[k] * mat) * S_vector(-unknown_44c0, 0.0f, unknown_44c0);
                    verts[0].x = corners[0].x;
                    verts[0].y = corners[0].z;
                    verts[0].z = .2f;
                    verts[0].rhw = .5f;
                    verts[0].color = color;
                    verts[0].u = 0.0f;
                    verts[0].v = 0.0f;
                    verts[2].x = corners[1].x;
                    verts[2].y = corners[1].z;
                    verts[2].z = .2f;
                    verts[2].rhw = .5f;
                    verts[2].color = color;
                    verts[2].u = 1.0f;
                    verts[2].v = 0.0f;
                    verts[1].x = corners[4].x;
                    verts[1].y = corners[4].z;
                    verts[1].z = .2f;
                    verts[1].rhw = .5f;
                    verts[1].color = color;
                    verts[1].u = 1.0f;
                    verts[1].v = 1.0f;
                    verts[3].x = corners[5].x;
                    verts[3].y = corners[5].z;
                    verts[3].z = .2f;
                    verts[3].rhw = .5f;
                    verts[3].color = color;
                    verts[3].u = 0.0f;
                    verts[3].v = 1.0f;
                    igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
                }
            }
            // The things' boxes
            for (G_RadarItem *ri = radar_items.begin(); ri < radar_items.end(); ri++)
            {
                uint color = ri->color;
                mat = *ri->object->matrix * m;
                const I3D_bbox &bb = (const I3D_bbox &)ri->object->bbox;
                const S_vector &min = bb.min, &max = bb.max;
                S_vector *c = corners;
                for (int i = 0; i < 2; i++)
                {
                    for (int j = 0; j < 2; j++)
                    {
                        c->x = i ? min.x : max.x;
                        c->y = j ? min.y : max.y;
                        c->z = min.z;
                        c++;
                        c->x = i ? min.x : max.x;
                        c->y = j ? min.y : max.y;
                        c->z = max.z;
                        c++;
                    }
                }
                for (int k = 0; k < 8; k++)
                    corners[k] = center - (corners[k] * mat) * S_vector(-unknown_44c0, 0.0f, unknown_44c0);
                verts[0].x = corners[0].x;
                verts[0].y = corners[0].z;
                verts[0].z = .2f;
                verts[0].rhw = .5f;
                verts[0].color = color;
                verts[0].u = 0.0f;
                verts[0].v = 0.0f;
                verts[2].x = corners[1].x;
                verts[2].y = corners[1].z;
                verts[2].z = .2f;
                verts[2].rhw = .5f;
                verts[2].color = color;
                verts[2].u = 1.0f;
                verts[2].v = 0.0f;
                verts[1].x = corners[4].x;
                verts[1].y = corners[4].z;
                verts[1].z = .2f;
                verts[1].rhw = .5f;
                verts[1].color = color;
                verts[1].u = 1.0f;
                verts[1].v = 1.0f;
                verts[3].x = corners[5].x;
                verts[3].y = corners[5].z;
                verts[3].z = .2f;
                verts[3].rhw = .5f;
                verts[3].color = color;
                verts[3].u = 0.0f;
                verts[3].v = 1.0f;
                igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
            }
            // The compass destination's red cross. The original transforms the last thing's corners once more first,
            // for nothing.
            if (unknown_40a4 & INDF_COMPASS && compass_destination)
            {
                for (int k = 0; k < 8; k++)
                    corners[k] = center - (corners[k] * mat) * S_vector(-unknown_44c0, 0.0f, unknown_44c0);
                S_vector p = center - (compass_destination->GetWorldPos() * m) *
                    S_vector(-unknown_44c0, 0.0f, unknown_44c0);
                verts[0].x = p.x - screen_scale.x * 5.0f;
                verts[0].y = p.z;
                verts[0].z = .2f;
                verts[0].rhw = .5f;
                verts[0].color = 0xffff0000;
                verts[0].u = 0.0f;
                verts[0].v = 0.0f;
                verts[1].x = p.x + screen_scale.x * 5.0f + 1.0f;
                verts[1].y = p.z;
                verts[1].z = .2f;
                verts[1].rhw = .5f;
                verts[1].color = 0xffff0000;
                verts[1].u = 1.0f;
                verts[1].v = 0.0f;
                verts[2].x = p.x;
                verts[2].y = p.z - screen_scale.x * 5.0f;
                verts[2].z = .2f;
                verts[2].rhw = .5f;
                verts[2].color = 0xffff0000;
                verts[2].u = 1.0f;
                verts[2].v = 1.0f;
                verts[3].x = p.x;
                verts[3].y = p.z + screen_scale.x * 5.0f + 1.0f;
                verts[3].z = .2f;
                verts[3].rhw = .5f;
                verts[3].color = 0xffff0000;
                verts[3].u = 0.0f;
                verts[3].v = 1.0f;
                igraph->DrawPrimitiveList(PT_LINELIST, 2, verts, LS3D_STREAM_TLVERTEX);
            }
            // The road's edges, RadarSetLeftEdge's and RadarSetRightEdge's
            if (_VtxLeftEdge && _VtxRightEdge)
            {
                uint i;
                for (i = 0; i < _VtxNumOfLeftEdge; i++)
                {
                    S_vector p = center - (_LeftEdge[i] * m) * S_vector(-unknown_44c0, 0.0f, unknown_44c0);
                    _VtxLeftEdge[i].x = p.x;
                    _VtxLeftEdge[i].y = p.z;
                }
                igraph->DrawPrimitiveList(PT_LINESTRIP, _VtxNumOfLeftEdge - 1, _VtxLeftEdge, LS3D_STREAM_TLVERTEX);
                for (i = 0; i < _VtxNumOfRightEdge; i++)
                {
                    S_vector p = center - (_RightEdge[i] * m) * S_vector(-unknown_44c0, 0.0f, unknown_44c0);
                    _VtxRightEdge[i].x = p.x;
                    _VtxRightEdge[i].y = p.z;
                }
                igraph->DrawPrimitiveList(PT_LINESTRIP, _VtxNumOfRightEdge - 1, _VtxRightEdge, LS3D_STREAM_TLVERTEX);
            }
            igraph->SetViewPort(&vp);
            driver->SetState(RS_USEZB, use_zb);
        }
        // The compass: a dial with a needle pointing to compass_destination, as seen in the camera's direction
        if (unknown_40a4 & INDF_COMPASS && compass_destination)
        {
            S_vector dir;
            dir = compass_destination->GetWorldPos() - act_mission->GetScene()->current_camera->GetWorldPos();
            S_vector cam_dir = act_mission->GetScene()->current_camera->GetWorldDir();
            dir.y = 0.0f;
            cam_dir.y = 0.0f;
            float angle = dir.AngleTo(cam_dir);
            S_vector side(dir.z, dir.y, -dir.x);
            if (cam_dir.Dot(side) > 0.0f)
                angle = -angle;
            S_vector2 pos = levyOkraj + S_vector2(0.0f, 0.0f) * screen_scale;
            S_vector2 size = screen_scale * 50.0f;
            verts[0].x = pos.x;
            verts[0].y = pos.y;
            verts[0].z = 0.0f;
            verts[0].rhw = 1.0f;
            verts[0].color = 0xffffffff;
            verts[0].u = 187 / 256.0f;
            verts[0].v = 172 / 256.0f;
            verts[1].x = pos.x + size.x;
            verts[1].y = pos.y;
            verts[1].z = 0.0f;
            verts[1].rhw = 1.0f;
            verts[1].color = 0xffffffff;
            verts[1].u = 255 / 256.0f;
            verts[1].v = 172 / 256.0f;
            verts[2].x = pos.x;
            verts[2].y = pos.y + size.y;
            verts[2].z = 0.0f;
            verts[2].rhw = 1.0f;
            verts[2].color = 0xffffffff;
            verts[2].u = 187 / 256.0f;
            verts[2].v = 241 / 256.0f;
            verts[3].x = pos.x + size.x;
            verts[3].y = pos.y + size.y;
            verts[3].z = 0.0f;
            verts[3].rhw = 1.0f;
            verts[3].color = 0xffffffff;
            verts[3].u = 255 / 256.0f;
            verts[3].v = 241 / 256.0f;
            igraph->SetTexture(texture_int2);
            igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
            // The needle, 20 long and 5 wide on 800x600, turned by the angle around the dial's centre
            S_vector2 center(pos.x + size.x * .5, pos.y + size.y * .5);
            float s = (float)sin(angle), c = (float)cos(angle);
            S_vector2 along = S_vector2(s, -c) * screen_scale.x;
            S_vector2 across = S_vector2(c, s) * screen_scale.x;
            S_vector2 p0 = center - along * 20.0f + across * 5.0f;
            S_vector2 p1 = center - along * 20.0f - across * 5.0f;
            S_vector2 p2 = center + along * 20.0f + across * 5.0f;
            S_vector2 p3 = center + along * 20.0f - across * 5.0f;
            verts[0].x = p0.x;
            verts[0].y = p0.y;
            verts[0].z = 0.0f;
            verts[0].rhw = 1.0f;
            verts[0].color = 0xffffffff;
            verts[0].u = 12 / 256.0f;
            verts[0].v = 212 / 256.0f;
            verts[1].x = p1.x;
            verts[1].y = p1.y;
            verts[1].z = 0.0f;
            verts[1].rhw = 1.0f;
            verts[1].color = 0xffffffff;
            verts[1].u = 12 / 256.0f;
            verts[1].v = 228 / 256.0f;
            verts[2].x = p2.x;
            verts[2].y = p2.y;
            verts[2].z = 0.0f;
            verts[2].rhw = 1.0f;
            verts[2].color = 0xffffffff;
            verts[2].u = 252 / 256.0f;
            verts[2].v = 212 / 256.0f;
            verts[3].x = p3.x;
            verts[3].y = p3.y;
            verts[3].z = 0.0f;
            verts[3].rhw = 1.0f;
            verts[3].color = 0xffffffff;
            verts[3].u = 252 / 256.0f;
            verts[3].v = 228 / 256.0f;
            igraph->SetTexture(texture_int);
            igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
        }
        // The city map (PS2's DrawMap): the part of the map around the player, framed in black, with the player's arrow
        // or, outside the map, text 0x5f; with INDF_COMPASS the destination's marker, or an arrow at the window's edge
        // pointing to it
        if (unknown_40a4 & INDF_MAP && map_enabled)
        {
            S_vector dir, pos;
            C_car *car = act_mission->GetGame()->camera.car;
            if (car)
            {
                dir = car->GetDir();
                pos = car->GetPos();
            }
            else
            {
                dir = act_mission->GetScene()->current_camera->GetWorldDir();
                pos = act_mission->GetScene()->current_camera->GetWorldPos();
            }
            // The world's x and z on the map texture, 0 to 1
            pos *= S_vector(.00022986f, 0.0f, -.00050728f);
            pos += S_vector(.734642386f, 0.0f, .470245987f);
            dir.y = 0.0f;
            dir.Normalize();
            S_vector2 map_pos;
            map_pos = S_vector2(pos.x, pos.z);
            bool inside = true;
            if (map_pos.x < 0.0f)
            {
                map_pos.x = 0.0f;
                inside = false;
            }
            if (map_pos.y < 0.0f)
            {
                map_pos.y = 0.0f;
                inside = false;
            }
            if (map_pos.x > 1.0f)
            {
                map_pos.x = 1.0f;
                inside = false;
            }
            if (map_pos.y > 1.0f)
            {
                map_pos.y = 1.0f;
                inside = false;
            }
            S_vector dest_pos;
            S_vector2 dest;
            if (unknown_40a4 & INDF_COMPASS && compass_destination)
            {
                dest_pos = compass_destination->GetWorldPos();
                dest_pos *= S_vector(.00022986f, 0.0f, -.00050728f);
                dest_pos += S_vector(.734642386f, 0.0f, .470245987f);
                dest = S_vector2(dest_pos.x, dest_pos.z);
            }
            // The window of the map shown, .4 by .55 of it, centered on the player as far as the map goes
            S_vector2 corner = map_pos - S_vector2(.2f, .275f);
            if (corner.x < 0.0f)
                corner.x = 0.0f;
            if (corner.y < 0.0f)
                corner.y = 0.0f;
            if (corner.x + .4f > 1.0f)
                corner.x = 1.0f - .4f;
            if (corner.y + .55f > 1.0f)
                corner.y = 1.0f - .55f;
            // Drawn over the HUD's screen from .1 to .9 of its width and from .13 to .87 of its height
            S_vector2 size = pravyOkraj - levyOkraj;
            S_vector2 tl = levyOkraj + S_vector2(size.x * .1f, size.y * .13f);
            S_vector2 br = levyOkraj + S_vector2(size.x * .9f, size.y * .87f);
            verts[0].x = tl.x;
            verts[0].y = tl.y;
            verts[0].z = 0.0f;
            verts[0].rhw = 1.0f;
            verts[0].color = 0x7fffffff;
            verts[0].u = corner.x;
            verts[0].v = corner.y;
            verts[1].x = br.x;
            verts[1].y = tl.y;
            verts[1].z = 0.0f;
            verts[1].rhw = 1.0f;
            verts[1].color = 0x7fffffff;
            verts[1].u = corner.x + .4f;
            verts[1].v = corner.y;
            verts[2].x = tl.x;
            verts[2].y = br.y;
            verts[2].z = 0.0f;
            verts[2].rhw = 1.0f;
            verts[2].color = 0x7fffffff;
            verts[2].u = corner.x;
            verts[2].v = corner.y + .55f;
            verts[3].x = br.x;
            verts[3].y = br.y;
            verts[3].z = 0.0f;
            verts[3].rhw = 1.0f;
            verts[3].color = 0x7fffffff;
            verts[3].u = corner.x + .4f;
            verts[3].v = corner.y + .55f;
            igraph->SetTexture(unknown_4090);
            igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
            // Its frame, two black rectangles one pixel apart
            verts[0].x = tl.x - 1.0f;
            verts[0].y = tl.y - 1.0f;
            verts[0].z = 0.0f;
            verts[0].rhw = 1.0f;
            verts[0].color = 0xff000000;
            verts[0].u = 0.0f;
            verts[0].v = 0.0f;
            verts[1].x = tl.x - 1.0f;
            verts[1].y = br.y;
            verts[1].z = 0.0f;
            verts[1].rhw = 1.0f;
            verts[1].color = 0xff000000;
            verts[1].u = 0.0f;
            verts[1].v = 1.0f;
            verts[2].x = br.x;
            verts[2].y = br.y;
            verts[2].z = 0.0f;
            verts[2].rhw = 1.0f;
            verts[2].color = 0xff000000;
            verts[2].u = 1.0f;
            verts[2].v = 1.0f;
            verts[3].x = br.x;
            verts[3].y = tl.y - 1.0f;
            verts[3].z = 0.0f;
            verts[3].rhw = 1.0f;
            verts[3].color = 0xff000000;
            verts[3].u = 1.0f;
            verts[3].v = 0.0f;
            verts[4].x = tl.x - 1.0f;
            verts[4].y = tl.y - 1.0f;
            verts[4].z = 0.0f;
            verts[4].rhw = 1.0f;
            verts[4].color = 0xff000000;
            verts[4].u = 0.0f;
            verts[4].v = 0.0f;
            verts[5].x = tl.x - 2.0f;
            verts[5].y = tl.y - 2.0f;
            verts[5].z = 0.0f;
            verts[5].rhw = 1.0f;
            verts[5].color = 0xff000000;
            verts[5].u = 0.0f;
            verts[5].v = 0.0f;
            verts[6].x = tl.x - 2.0f;
            verts[6].y = br.y + 1.0f;
            verts[6].z = 0.0f;
            verts[6].rhw = 1.0f;
            verts[6].color = 0xff000000;
            verts[6].u = 0.0f;
            verts[6].v = 1.0f;
            verts[7].x = br.x + 1.0f;
            verts[7].y = br.y + 1.0f;
            verts[7].z = 0.0f;
            verts[7].rhw = 1.0f;
            verts[7].color = 0xff000000;
            verts[7].u = 1.0f;
            verts[7].v = 1.0f;
            verts[8].x = br.x + 1.0f;
            verts[8].y = tl.y - 2.0f;
            verts[8].z = 0.0f;
            verts[8].rhw = 1.0f;
            verts[8].color = 0xff000000;
            verts[8].u = 1.0f;
            verts[8].v = 0.0f;
            verts[9].x = tl.x - 2.0f;
            verts[9].y = tl.y - 2.0f;
            verts[9].z = 0.0f;
            verts[9].rhw = 1.0f;
            verts[9].color = 0xff000000;
            verts[9].u = 0.0f;
            verts[9].v = 0.0f;
            igraph->SetTexture(NULL);
            igraph->DrawPrimitiveList(PT_LINESTRIP, 9, verts, LS3D_STREAM_TLVERTEX);
            if (inside)
            {
                // The player's arrow, a triangle 16 by 10 pixels (at 800x600) with a dark outline; the window's .4 by
                // .55 of the map are .8 by .74 of the screen
                S_vector2 d(dir.x, -dir.z), n(dir.z, dir.x);
                S_vector2 a = d * (screen_scale.y * 8.0f);
                S_vector2 b = n * (screen_scale.y * 5.0f);
                S_vector2 c = levyOkraj + size * .5f
                    + (map_pos - corner - S_vector2(.2f, .275f)) * size * S_vector2(.8f / .4f, .74f / .55f);
                S_vector2 p0 = c + b - a;
                S_vector2 p1 = c - b - a;
                S_vector2 p2 = c + a + a;
                verts[0].x = p0.x;
                verts[0].y = p0.y;
                verts[0].z = 0.0f;
                verts[0].rhw = 1.0f;
                verts[0].color = 0xffffff3f;
                verts[0].u = 0.0f;
                verts[0].v = 0.0f;
                verts[1].x = p1.x;
                verts[1].y = p1.y;
                verts[1].z = 0.0f;
                verts[1].rhw = 1.0f;
                verts[1].color = 0xffffff3f;
                verts[1].u = 0.0f;
                verts[1].v = 1.0f;
                verts[2].x = p2.x;
                verts[2].y = p2.y;
                verts[2].z = 0.0f;
                verts[2].rhw = 1.0f;
                verts[2].color = 0xffffff3f;
                verts[2].u = 1.0f;
                verts[2].v = 0.0f;
                igraph->SetTexture(NULL);
                igraph->DrawPrimitiveList(PT_TRIANGLELIST, 1, verts, LS3D_STREAM_TLVERTEX);
                verts[0].x = p0.x;
                verts[0].y = p0.y;
                verts[0].z = 0.0f;
                verts[0].rhw = 1.0f;
                verts[0].color = 0x7f000000;
                verts[0].u = 0.0f;
                verts[0].v = 0.0f;
                verts[1].x = p1.x;
                verts[1].y = p1.y;
                verts[1].z = 0.0f;
                verts[1].rhw = 1.0f;
                verts[1].color = 0x7f000000;
                verts[1].u = 0.0f;
                verts[1].v = 1.0f;
                verts[2].x = p2.x;
                verts[2].y = p2.y;
                verts[2].z = 0.0f;
                verts[2].rhw = 1.0f;
                verts[2].color = 0x7f000000;
                verts[2].u = 1.0f;
                verts[2].v = 0.0f;
                verts[3].x = p0.x;
                verts[3].y = p0.y;
                verts[3].z = 0.0f;
                verts[3].rhw = 1.0f;
                verts[3].color = 0x7f000000;
                verts[3].u = 0.0f;
                verts[3].v = 0.0f;
                igraph->SetTexture(NULL);
                igraph->DrawPrimitiveList(PT_LINESTRIP, 3, verts, LS3D_STREAM_TLVERTEX);
            }
            else
            {
                OutText(text_out_of_city, levyOkraj.x + screen_scale.x * 350.0f, screen_scale.y * 290.0f,
                    screen_scale.x * 100.0f, screen_scale.y * 40.0f, 0xffff0000, OUTTEXT_CENTER, 1, NULL);
            }
            if (unknown_40a4 & INDF_COMPASS && compass_destination)
            {
                // The destination relative to the window's center, -.5 to .5 inside it; outside, moved along the line
                // to the center onto the window's edge
                bool clipped = false;
                S_vector2 e;
                e = (dest - corner - S_vector2(.2f, .275f)) * S_vector2(1.0f / .4f, 1.0f / .55f);
                if (e.x < -.5f)
                {
                    e.y *= -.5f / e.x;
                    e.x = -.5f;
                    clipped = true;
                }
                if (e.y < -.5f)
                {
                    e.x *= -.5f / e.y;
                    e.y = -.5f;
                    clipped = true;
                }
                if (e.x > .5f)
                {
                    e.y *= .5f / e.x;
                    e.x = .5f;
                    clipped = true;
                }
                if (e.y > .5f)
                {
                    e.x *= .5f / e.y;
                    e.y = .5f;
                    clipped = true;
                }
                S_vector2 m = levyOkraj + size * .5f + e * size * S_vector2(.8f, .74f);
                if (!clipped)
                {
                    // The marker, a square of 20 pixels from the dials' texture
                    float r = screen_scale.y * 10.0f;
                    verts[0].x = m.x - r;
                    verts[0].y = m.y - r;
                    verts[0].z = 0.0f;
                    verts[0].rhw = 1.0f;
                    verts[0].color = 0xbfffffff;
                    verts[0].u = 64 / 256.0f;
                    verts[0].v = 116 / 256.0f;
                    verts[1].x = m.x + r;
                    verts[1].y = m.y - r;
                    verts[1].z = 0.0f;
                    verts[1].rhw = 1.0f;
                    verts[1].color = 0xbfffffff;
                    verts[1].u = 91 / 256.0f;
                    verts[1].v = 116 / 256.0f;
                    verts[2].x = m.x - r;
                    verts[2].y = m.y + r;
                    verts[2].z = 0.0f;
                    verts[2].rhw = 1.0f;
                    verts[2].color = 0xbfffffff;
                    verts[2].u = 64 / 256.0f;
                    verts[2].v = 143 / 256.0f;
                    verts[3].x = m.x + r;
                    verts[3].y = m.y + r;
                    verts[3].z = 0.0f;
                    verts[3].rhw = 1.0f;
                    verts[3].color = 0xbfffffff;
                    verts[3].u = 91 / 256.0f;
                    verts[3].v = 143 / 256.0f;
                    igraph->SetTexture(texture_int2);
                    igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
                }
                else
                {
                    // An arrow at the edge, pointing from the player to the destination
                    dir = dest_pos - pos;
                    dir.y = 0.0f;
                    dir.Normalize();
                    S_vector2 d(dir.x, dir.z), n(dir.z, -dir.x);
                    S_vector2 a = d * (screen_scale.y * 8.0f);
                    S_vector2 b = n * (screen_scale.y * 5.0f);
                    S_vector2 q0 = m - b - a;
                    S_vector2 q1 = m + b - a;
                    S_vector2 q2 = m + a + a;
                    verts[0].x = q0.x;
                    verts[0].y = q0.y;
                    verts[0].z = 0.0f;
                    verts[0].rhw = 1.0f;
                    verts[0].color = 0xff40aef9;
                    verts[0].u = 0.0f;
                    verts[0].v = 0.0f;
                    verts[1].x = q1.x;
                    verts[1].y = q1.y;
                    verts[1].z = 0.0f;
                    verts[1].rhw = 1.0f;
                    verts[1].color = 0xff40aef9;
                    verts[1].u = 0.0f;
                    verts[1].v = 1.0f;
                    verts[2].x = q2.x;
                    verts[2].y = q2.y;
                    verts[2].z = 0.0f;
                    verts[2].rhw = 1.0f;
                    verts[2].color = 0xff40aef9;
                    verts[2].u = 1.0f;
                    verts[2].v = 0.0f;
                    igraph->SetTexture(NULL);
                    igraph->DrawPrimitiveList(PT_TRIANGLELIST, 1, verts, LS3D_STREAM_TLVERTEX);
                    verts[0].x = q0.x;
                    verts[0].y = q0.y;
                    verts[0].z = 0.0f;
                    verts[0].rhw = 1.0f;
                    verts[0].color = 0x7f000000;
                    verts[0].u = 0.0f;
                    verts[0].v = 0.0f;
                    verts[1].x = q1.x;
                    verts[1].y = q1.y;
                    verts[1].z = 0.0f;
                    verts[1].rhw = 1.0f;
                    verts[1].color = 0x7f000000;
                    verts[1].u = 0.0f;
                    verts[1].v = 1.0f;
                    verts[2].x = q2.x;
                    verts[2].y = q2.y;
                    verts[2].z = 0.0f;
                    verts[2].rhw = 1.0f;
                    verts[2].color = 0x7f000000;
                    verts[2].u = 1.0f;
                    verts[2].v = 0.0f;
                    verts[3].x = q0.x;
                    verts[3].y = q0.y;
                    verts[3].z = 0.0f;
                    verts[3].rhw = 1.0f;
                    verts[3].color = 0x7f000000;
                    verts[3].u = 0.0f;
                    verts[3].v = 0.0f;
                    igraph->SetTexture(NULL);
                    igraph->DrawPrimitiveList(PT_LINESTRIP, 3, verts, LS3D_STREAM_TLVERTEX);
                }
            }
        }
        // The start lights, the flag set and the one before it: a quarter of racestart.bmp each, chosen by the flag's
        // two low bits, sized and faded by its time; flags from 10 up show nothing (PS2's from 16 up). The quads'
        // vertices go 0, 1, 3, 2.
        if (unknown_40a4 & INDF_RACE_START)
        {
            if (race_start_flag < 10)
            {
                uint color = (uint)(race_start_time * 255.0f) << 24 | 0xffffff;
                S_vector2 size = S_vector2(256.0f, 256.0f) * screen_scale * race_start_time;
                S_vector2 pos = levyOkraj + S_vector2(400.0f, 20.0f) * screen_scale;
                pos.x -= size.x * .5;
                float u = race_start_flag & 1 ? .5f : 0.0f;
                float v = race_start_flag & 2 ? .5f : 0.0f;
                verts[0].x = pos.x;
                verts[0].y = pos.y;
                verts[0].z = 0.0f;
                verts[0].rhw = 1.0f;
                verts[0].color = color;
                verts[0].u = u;
                verts[0].v = v;
                verts[1].x = pos.x + size.x;
                verts[1].y = pos.y;
                verts[1].z = 0.0f;
                verts[1].rhw = 1.0f;
                verts[1].color = color;
                verts[1].u = u + .5f;
                verts[1].v = v;
                verts[3].x = pos.x + size.x;
                verts[3].y = pos.y + size.y;
                verts[3].z = 0.0f;
                verts[3].rhw = 1.0f;
                verts[3].color = color;
                verts[3].u = u + .5f;
                verts[3].v = v + .5f;
                verts[2].x = pos.x;
                verts[2].y = pos.y + size.y;
                verts[2].z = 0.0f;
                verts[2].rhw = 1.0f;
                verts[2].color = color;
                verts[2].u = u;
                verts[2].v = v + .5f;
                igraph->SetTexture(texture_race_start);
                igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
            }
            if (race_start_flag_last < 10)
            {
                uint color = (uint)(race_start_last_time * 255.0f) << 24 | 0xffffff;
                S_vector2 size = S_vector2(256.0f, 256.0f) * screen_scale * race_start_last_time;
                S_vector2 pos = levyOkraj + S_vector2(400.0f, 20.0f) * screen_scale;
                pos.x -= size.x * .5;
                float u = race_start_flag_last & 1 ? .5f : 0.0f;
                float v = race_start_flag_last & 2 ? .5f : 0.0f;
                verts[0].x = pos.x;
                verts[0].y = pos.y;
                verts[0].z = 0.0f;
                verts[0].rhw = 1.0f;
                verts[0].color = color;
                verts[0].u = u;
                verts[0].v = v;
                verts[1].x = pos.x + size.x;
                verts[1].y = pos.y;
                verts[1].z = 0.0f;
                verts[1].rhw = 1.0f;
                verts[1].color = color;
                verts[1].u = u + .5f;
                verts[1].v = v;
                verts[3].x = pos.x + size.x;
                verts[3].y = pos.y + size.y;
                verts[3].z = 0.0f;
                verts[3].rhw = 1.0f;
                verts[3].color = color;
                verts[3].u = u + .5f;
                verts[3].v = v + .5f;
                verts[2].x = pos.x;
                verts[2].y = pos.y + size.y;
                verts[2].z = 0.0f;
                verts[2].rhw = 1.0f;
                verts[2].color = color;
                verts[2].u = u;
                verts[2].v = v + .5f;
                igraph->SetTexture(texture_race_start);
                igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
            }
        }
        // The race's texts at the right edge, one under the other
        if (unknown_40a4 & INDF_RACE)
        {
            float x = levyOkraj.x + screen_scale.x * 780.0f;
            float h = screen_scale.y * 30.0f;
            OutText(race_time, x, h, 10.0f, h, 0xffffffff, OUTTEXT_RIGHT, 1, NULL);
            OutText(race_lap_time, x, h * 2.0f, 10.0f, h, 0xffffffff, OUTTEXT_RIGHT, 1, NULL);
            OutText(race_best_time, x, h * 3.0f, 10.0f, h, 0xffffffff, OUTTEXT_RIGHT, 1, NULL);
            OutText(race_position, x, h * 4.0f, 10.0f, h, 0xffffffff, OUTTEXT_RIGHT, 1, NULL);
            OutText(race_laps, x, h * 5.0f, 10.0f, h, 0xffffffff, OUTTEXT_RIGHT, 1, NULL);
        }
        // unknown_4614's lines, centred across the screen from a quarter of its height down, in unknown_4694's alpha
        if (unknown_40a4 & INDF_UNKNOWN_400000)
        {
            S_vector2 pos = levyOkraj + S_vector2(390.0f, 150.0f) * screen_scale;
            float sy = screen_scale.y * 30.0f;
            uint alpha = (uint)(unknown_4694 * 255.0f);
            if (alpha > 0xff)
                alpha = 0xff;
            OutTextMultiLine(unknown_4614, pos.x, pos.y, 20.0f, sy, alpha << 24 | 0xffffff,
                OUTTEXT_CENTER | OUTTEXT_UNKNOWN_20, 0);
        }
        // racewrong.bmp, half transparent
        if (unknown_40a4 & INDF_RACE_WRONG)
        {
            S_vector2 pos = levyOkraj + S_vector2(272.0f, 100.0f) * screen_scale;
            S_vector2 size = S_vector2(256.0f, 128.0f) * screen_scale;
            verts[0].x = pos.x;
            verts[0].y = pos.y;
            verts[0].z = 0.0f;
            verts[0].rhw = 1.0f;
            verts[0].color = 0x7fffffff;
            verts[0].u = 0.0f;
            verts[0].v = 0.0f;
            verts[1].x = pos.x + size.x;
            verts[1].y = pos.y;
            verts[1].z = 0.0f;
            verts[1].rhw = 1.0f;
            verts[1].color = 0x7fffffff;
            verts[1].u = 1.0f;
            verts[1].v = 0.0f;
            verts[3].x = pos.x + size.x;
            verts[3].y = pos.y + size.y;
            verts[3].z = 0.0f;
            verts[3].rhw = 1.0f;
            verts[3].color = 0x7fffffff;
            verts[3].u = 1.0f;
            verts[3].v = 1.0f;
            verts[2].x = pos.x;
            verts[2].y = pos.y + size.y;
            verts[2].z = 0.0f;
            verts[2].rhw = 1.0f;
            verts[2].color = 0x7fffffff;
            verts[2].u = 0.0f;
            verts[2].v = 1.0f;
            igraph->SetTexture(texture_race_wrong);
            igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
        }
    }
    // The logo at the top right, but in a menu
    if (unknown_40a4 & ~INDF_MENU && texture_logo)
    {
        S_vector2 pos = S_vector2(687.0f, 20.0f) * screen_scale;
        S_vector2 size = S_vector2(93.0f, 128.0f) * screen_scale;
        verts[0].x = pos.x;
        verts[0].y = pos.y;
        verts[0].z = 0.0f;
        verts[0].rhw = 1.0f;
        verts[0].color = 0xffffffff;
        verts[0].u = 0.0f;
        verts[0].v = 0.0f;
        verts[1].x = pos.x + size.x;
        verts[1].y = pos.y;
        verts[1].z = 0.0f;
        verts[1].rhw = 1.0f;
        verts[1].color = 0xffffffff;
        verts[1].u = 1.0f;
        verts[1].v = 0.0f;
        verts[3].x = pos.x + size.x;
        verts[3].y = pos.y + size.y;
        verts[3].z = 0.0f;
        verts[3].rhw = 1.0f;
        verts[3].color = 0xffffffff;
        verts[3].u = 1.0f;
        verts[3].v = 1.0f;
        verts[2].x = pos.x;
        verts[2].y = pos.y + size.y;
        verts[2].z = 0.0f;
        verts[2].rhw = 1.0f;
        verts[2].color = 0xffffffff;
        verts[2].u = 0.0f;
        verts[2].v = 1.0f;
        igraph->SetTexture(texture_logo);
        igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
    }
    // The whole screen and a pixel more in fade_color, through INDF_FADE's texture. The quad's vertices go 0, 2, 1, 3
    // (PS2's the same), the others' 0, 1, 3, 2.
    if (unknown_40a4 & INDF_FADE)
    {
        verts[0].x = 0.0f;
        verts[0].y = 0.0f;
        verts[0].z = 0.0f;
        verts[0].rhw = 1.0f;
        verts[0].color = fade_color;
        verts[0].u = 0.0f;
        verts[0].v = 0.0f;
        verts[2].x = 0.0f;
        verts[2].y = screen_size.y + 1.0f;
        verts[2].z = 0.0f;
        verts[2].rhw = 1.0f;
        verts[2].color = fade_color;
        verts[2].u = 0.0f;
        verts[2].v = 1.0f;
        verts[1].x = screen_size.x + 1.0f;
        verts[1].y = 0.0f;
        verts[1].z = 0.0f;
        verts[1].rhw = 1.0f;
        verts[1].color = fade_color;
        verts[1].u = 1.0f;
        verts[1].v = 0.0f;
        verts[3].x = screen_size.x + 1.0f;
        verts[3].y = screen_size.y + 1.0f;
        verts[3].z = 0.0f;
        verts[3].rhw = 1.0f;
        verts[3].color = fade_color;
        verts[3].u = 1.0f;
        verts[3].v = 1.0f;
        igraph->SetTexture(unknown_4480);
        igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
    }
    // The subtitles centred at the bottom (of the left screen with Parhelia's two), the last line lowest, all opaque
    // (PS2's take their alpha)
    if (!(unknown_40a4 & INDF_MENU) && subtitles.size() && mafiaLoadSave.settings.subtitles)
    {
        float x = screen_size.x * .5f;
        if (parheliaIndex == PARHELIA_DUAL)
            x = hud_size.x * .5f;
        float y = screen_scale.y * 550.0f;
        float h = screen_scale.y * 22.0f;
        G_MISubtitleStruct *p = subtitles.end();
        while (p > subtitles.begin())
        {
            p--;
            if (!p->hidden)
            {
                OutText(p->text, x - 10.0f, y, 20.0f, h, 0xffffffff, OUTTEXT_CENTER, 0, p->end);
                y -= h;
            }
        }
    }
    if (intro_subtitles.size())
    {
        for (G_MIIntroSubtitles *p = intro_subtitles.begin(); p < intro_subtitles.end(); p++)
        {
            uint alpha = (uint)(min(p->alpha_out, p->alpha_in) * 255.0f);
            if (alpha > 0xff)
                alpha = 0xff;
            OutText(p->text, p->x * screen_scale.x + levyOkraj.x, p->y * screen_scale.y, 0.0f,
                p->size * screen_scale.y, alpha << 24 | 0xffffff, 0, 0, p->end);
        }
    }
    driver->SetState(RS_USEZB, 1);
}

// A quad's two triangles: the rectangle at pos, of size, and its texture's coordinates (PC only, the name is ours)
inline void SetRectVertices(const S_vector2 &pos, const S_vector2 &size, float u0, float v0, float u1, float v1,
    uint color, TLVERTEX *verts);
// The same for the diamond whose corners are size away from pos (PC only, the name is ours)
inline void SetDiamondVertices(const S_vector2 &pos, const S_vector2 &size, float u0, float v0, float u1, float v1,
    uint color, TLVERTEX *verts);

// The HUD was laid out for 800x600: the scale is the screen's (the middle screen's with Parhelia's three) relative to
// it, and the tachometer and the clock are placed from the screen's right edge
// FUNCTION: GAME 0x601520
void G_IndicatorsClass::ResolutionChanged()
{
    S_vector2 screen((float)igraph->Scrn_sx(), (float)igraph->Scrn_sy());
    parheliaIndex = PARHELIA_NONE;
    if (screen.x > screen.y * 2.0f)
        parheliaIndex = PARHELIA_DUAL;
    if (screen.x > screen.y * 3.0f)
        parheliaIndex = PARHELIA_TRIPLE;
    switch (parheliaIndex)
    {
    case PARHELIA_DUAL:
        levyOkraj = S_vector2(0.0f, 0.0f);
        pravyOkraj = S_vector2((float)igraph->Scrn_sx(), (float)igraph->Scrn_sy());
        break;
    case PARHELIA_TRIPLE:
        levyOkraj = S_vector2((float)(igraph->Scrn_sx() / 3), 0.0f);
        pravyOkraj = S_vector2((float)(igraph->Scrn_sx() * 2 / 3), (float)igraph->Scrn_sy());
        break;
    default:
        levyOkraj = S_vector2(0.0f, 0.0f);
        pravyOkraj = S_vector2((float)igraph->Scrn_sx(), (float)igraph->Scrn_sy());
    }
    hud_size = screen_size = S_vector2((float)igraph->Scrn_sx(), (float)igraph->Scrn_sy());
    switch (parheliaIndex)
    {
    case PARHELIA_DUAL:
        screen_scale = S_vector2((float)igraph->Scrn_sx() * (1.0f / 1600), (float)igraph->Scrn_sy() * (1.0f / 600));
        hud_scale = S_vector2((float)igraph->Scrn_sx() * (1.0f / 1600), (float)igraph->Scrn_sy() * (1.0f / 600));
        hud_size.x *= .5f;
        break;
    case PARHELIA_TRIPLE:
        screen_scale = S_vector2((float)igraph->Scrn_sx() * (1.0f / 2400), (float)igraph->Scrn_sy() * (1.0f / 600));
        hud_scale = S_vector2((float)igraph->Scrn_sx() * (1.0f / 2400), (float)igraph->Scrn_sy() * (1.0f / 600));
        hud_size.x *= 1.0f / 3;
        break;
    default:
        screen_scale = S_vector2((float)igraph->Scrn_sx() * (1.0f / 800), (float)igraph->Scrn_sy() * (1.0f / 600));
        hud_scale = S_vector2((float)igraph->Scrn_sx() * (1.0f / 800), (float)igraph->Scrn_sy() * (1.0f / 600));
    }
    tachometr_pos = pravyOkraj - hud_scale * 189.0f;
    revs_pos = pravyOkraj - S_vector2(259.0f, 84.0f) * hud_scale;
    revs_pos.x += hud_scale.x * 11.0f;
    // The tachometer's two dials
    SetRectVertices(tachometr_pos, hud_scale * 180.0f, 95 / 256.0f, 2 / 256.0f, 255 / 256.0f, 162 / 256.0f, 0xffffffff,
        &_VtxTachometrCiselnik[0]);
    SetRectVertices(revs_pos, hud_scale * 76.0f, 109 / 256.0f, 165 / 256.0f, 185 / 256.0f, 241 / 256.0f, 0xffffffff,
        &_VtxTachometrCiselnik[6]);
    // Its small display, the texture's column from unknown_41e4
    S_vector2 pos, size;
    pos = tachometr_pos + S_vector2(78.0f, 63.0f) * hud_scale;
    size = tachometr_pos + S_vector2(103.0f, 77.0f) * hud_scale - pos;
    SetRectVertices(pos, size, (unknown_41e4 + 22.0f) / 256.0f, 199 / 256.0f, (unknown_41e4 + 43.0f - 2.0f) / 256.0f,
        210 / 256.0f, 0x70ffffff, &_VtxTachometrCiselnik[48]);
    // Its row of five digits
    SetRectVertices(tachometr_pos + S_vector2(62.0f, 104.0f) * hud_scale, hud_scale * 12.0f, 1 / 256.0f, 1 / 256.0f,
        180 / 256.0f, 180 / 256.0f, 0x70ffffff, &_VtxTachometrCiselnik[36]);
    SetRectVertices(tachometr_pos + S_vector2(73.0f, 104.0f) * hud_scale, hud_scale * 12.0f, 1 / 256.0f, 1 / 256.0f,
        180 / 256.0f, 180 / 256.0f, 0x70ffffff, &_VtxTachometrCiselnik[30]);
    SetRectVertices(tachometr_pos + S_vector2(84.0f, 104.0f) * hud_scale, hud_scale * 12.0f, 1 / 256.0f, 1 / 256.0f,
        180 / 256.0f, 180 / 256.0f, 0x70ffffff, &_VtxTachometrCiselnik[24]);
    SetRectVertices(tachometr_pos + S_vector2(95.0f, 105.0f) * hud_scale, hud_scale * 12.0f, 1 / 256.0f, 1 / 256.0f,
        180 / 256.0f, 180 / 256.0f, 0x70ffffff, &_VtxTachometrCiselnik[18]);
    SetRectVertices(tachometr_pos + S_vector2(106.0f, 105.0f) * hud_scale, hud_scale * 12.0f, 1 / 256.0f, 1 / 256.0f,
        180 / 256.0f, 180 / 256.0f, 0x70ffffff, &_VtxTachometrCiselnik[12]);
    SetDiamondVertices(pravyOkraj + S_vector2(-40.0f, -220.0f) * hud_scale, S_vector2(20.0f, 20.0f) * hud_scale,
        35 / 256.0f, 116 / 256.0f, 63 / 256.0f, 144 / 256.0f, 0xffffffff, &_VtxTachometrCiselnik[42]);
    _SpocitejRafiku2(&tachometrDefSpeed, &tachometr_speed, tachometr_pos, &_VtxTachometrRafiky[0]);
    _SpocitejRafiku2(&tachometrDefRevs, &tachometr_revs, revs_pos, &_VtxTachometrRafiky[6]);
    _SpocitejRafiku2(&tachometrDefPalivo, &tachometr_fuel, tachometr_pos, &_VtxTachometrRafiky[12]);
    // The clock
    timer_pos = S_vector2(-106.0f, 20.0f) * hud_scale;
    timer_pos.x += pravyOkraj.x;
    SetRectVertices(timer_pos, S_vector2(86.0f, 105.0f) * hud_scale, 1 / 256.0f, 1 / 256.0f, 90 / 256.0f, 112 / 256.0f,
        0xffffffff, &_VtxTimerCifernik[0]);
    _SpocitejRafiku2(&timerDefHours, &timer_hours, timer_pos, &_VtxTimerCifernik[6]);
    _SpocitejRafiku2(&timerDefMinutes, &timer_minutes, timer_pos, &_VtxTimerCifernik[12]);
    _SpocitejRafiku2(&timerDefSeconds, &timer_seconds, timer_pos, &_VtxTimerCifernik[18]);
    screen_center = screen / 2.0f;
    SetRectVertices(S_vector2(145.0f, 569.0f) * hud_scale, S_vector2(10.0f, 24.0f) * hud_scale, 212 / 256.0f,
        79 / 256.0f, 223 / 256.0f, 104 / 256.0f, 0xffffffff, &_VtxPlayer[0]);
    SetRectVertices(S_vector2(55.0f, 567.0f) * hud_scale, S_vector2(16.0f, 25.0f) * hud_scale, 226 / 256.0f,
        79 / 256.0f, 242 / 256.0f, 104 / 256.0f, 0xffffffff, &_VtxPlayer[6]);
    SetRectVertices(S_vector2(15.0f, 500.0f) * hud_scale, S_vector2(30.0f, 30.0f) * hud_scale, 181 / 256.0f,
        113 / 256.0f, 212 / 256.0f, 144 / 256.0f, 0xffffffff, &_VtxPlayer[12]);
}

// FUNCTION: GAME 0x6025f0
inline void SetRectVertices(const S_vector2 &pos, const S_vector2 &size, float u0, float v0, float u1, float v1,
    uint color, TLVERTEX *verts)
{
    verts[0].x = pos.x;
    verts[0].y = pos.y;
    verts[0].z = 0.0f;
    verts[0].rhw = 1.0f;
    verts[0].color = color;
    verts[0].u = u0;
    verts[0].v = v0;
    verts[1].x = pos.x + size.x;
    verts[1].y = pos.y;
    verts[1].z = 0.0f;
    verts[1].rhw = 1.0f;
    verts[1].color = color;
    verts[1].u = u1;
    verts[1].v = v0;
    verts[2].x = pos.x;
    verts[2].y = pos.y + size.y;
    verts[2].z = 0.0f;
    verts[2].rhw = 1.0f;
    verts[2].color = color;
    verts[2].u = u0;
    verts[2].v = v1;
    verts[3].x = pos.x + size.x;
    verts[3].y = pos.y + size.y;
    verts[3].z = 0.0f;
    verts[3].rhw = 1.0f;
    verts[3].color = color;
    verts[3].u = u1;
    verts[3].v = v1;
    verts[4] = verts[2];
    verts[5] = verts[1];
}

// FUNCTION: GAME 0x6026d0
inline void SetDiamondVertices(const S_vector2 &pos, const S_vector2 &size, float u0, float v0, float u1, float v1,
    uint color, TLVERTEX *verts)
{
    verts[0].x = pos.x - size.x;
    verts[0].y = pos.y;
    verts[0].z = 0.0f;
    verts[0].rhw = 1.0f;
    verts[0].color = color;
    verts[0].u = u0;
    verts[0].v = v0;
    verts[1].x = pos.x;
    verts[1].y = pos.y - size.y;
    verts[1].z = 0.0f;
    verts[1].rhw = 1.0f;
    verts[1].color = color;
    verts[1].u = u1;
    verts[1].v = v0;
    verts[2].x = pos.x;
    verts[2].y = pos.y + size.y;
    verts[2].z = 0.0f;
    verts[2].rhw = 1.0f;
    verts[2].color = color;
    verts[2].u = u0;
    verts[2].v = v1;
    verts[3].x = pos.x + size.x;
    verts[3].y = pos.y;
    verts[3].z = 0.0f;
    verts[3].rhw = 1.0f;
    verts[3].color = color;
    verts[3].u = u1;
    verts[3].v = v1;
    verts[4] = verts[2];
    verts[5] = verts[1];
}

// A text's width in the font, at its size (the characters' height): each character's width less what it overlaps the
// next, a space's .3 of the size for a code the font has no character for; the last one's overlap counts
// FUNCTION: GAME 0x6036d0
float G_IndicatorsClass::TextSize(const char *text, float size, uint font, const char *stop)
{
    if (!text)
        return 0.0f;
    ushort *trans = fontTranstable[font];
    G_Font *f = &fonts[font];
    float spacing = size * f->spacing;
    size *= f->width;
    float width = 0.0f;
    for (; *text; text++)
    {
        if (stop && text >= stop)
            break;
        uchar c = (uchar)trans[(uchar)*text];
        if (f->chars[c].width < .001)
            width += size * .3f;
        else
            width += size * f->chars[c].width - spacing;
    }
    return width + spacing;
}

// The PC's: PS2's also draws the icons of the codes under 0x20, the text's background and skips texts off the screen.
// A code above 0xff that fontTranstable gives is two characters, an accent (the high byte, lowered with 0x8000)
// centered over a letter (the low byte, none for 0). The characters are quads of the font's texture, OUTTEXT_MAX_CHARS
// of them at most.
// FUNCTION: GAME 0x603880
float G_IndicatorsClass::OutText(const char *text, float x, float y, float sx, float sy, uint color, uint flags,
    uint font, const char *stop)
{
    if (!text)
        return x;
    ushort *trans = fontTranstable[font];
    y -= sy * fonts[font].top;
    G_Font *f = &fonts[font];
    float tex_height = f->tex_height;
    float size = sy * f->height;
    float spacing = sy * f->spacing;
    float char_width = size * f->width;
    if (flags & (OUTTEXT_CENTER | OUTTEXT_RIGHT))
    {
        float text_size = TextSize(text, size, font, stop);
        if (flags & OUTTEXT_CENTER)
            x = x + sx * .5f - text_size * .5f;
        else
            x = x + sx - text_size;
    }
    else
    {
        if (flags & OUTTEXT_INDENT)
            x += sy * .5f;
        if (flags & OUTTEXT_FIT)
        {
            float text_size = TextSize(text, size, font, stop);
            if (flags & OUTTEXT_INDENT)
            {
                if (text_size > sx - sy)
                {
                    float scale = (sx - sy) / text_size;
                    char_width *= scale;
                    spacing *= scale;
                }
            }
            else if (text_size > sx)
            {
                float scale = sx / text_size;
                char_width *= scale;
                spacing *= scale;
            }
        }
    }
    float cur_x = x;
    int num = 0;
    float ch_width;
    TLVERTEX verts[450];
    for (; *text; text++)
    {
        if (stop && text >= stop)
            break;
        uchar c = *text;
        ushort code = trans[c];
        if (!c)
            continue;
        if (code > 0xff)
        {
            uchar accent_code = (code >> 8) & 0x7f;
            float cy = y;
            G_FontChar *accent = &f->chars[accent_code];
            float accent_width = char_width * accent->width;
            G_FontChar *ch = &f->chars[code & 0xff];
            float ax = cur_x;
            if (ch->width >= .001)
            {
                ch_width = char_width * ch->width;
                ax = cur_x + (ch_width - accent_width) * .5f;
                if (code & 0x8000)
                    cy = y - size * .12f;
            }
            verts[num].x = ax;
            verts[num].y = cy;
            verts[num].z = 0.0f;
            verts[num].rhw = 1.0f;
            verts[num].color = color;
            verts[num].u = accent->u;
            verts[num].v = accent->v;
            verts[num + 1].x = ax + accent_width;
            verts[num + 1].y = cy;
            verts[num + 1].z = 0.0f;
            verts[num + 1].rhw = 1.0f;
            verts[num + 1].color = color;
            verts[num + 1].u = accent->u1;
            verts[num + 1].v = accent->v;
            verts[num + 2].x = ax;
            verts[num + 2].y = cy + size;
            verts[num + 2].z = 0.0f;
            verts[num + 2].rhw = 1.0f;
            verts[num + 2].color = color;
            verts[num + 2].u = accent->u;
            verts[num + 2].v = tex_height + accent->v;
            verts[num + 3].x = ax + accent_width;
            verts[num + 3].y = cy + size;
            verts[num + 3].z = 0.0f;
            verts[num + 3].rhw = 1.0f;
            verts[num + 3].color = color;
            verts[num + 3].u = accent->u1;
            verts[num + 3].v = tex_height + accent->v;
            num += 4;
            if (num >= OUTTEXT_MAX_CHARS * 4)
                break;
            if (ch->width < .001)
            {
                if (code & 0xff)
                    cur_x += size * .3f;
                else
                    cur_x += accent_width - spacing;
                continue;
            }
            verts[num].x = cur_x;
            verts[num].y = y;
            verts[num].z = 0.0f;
            verts[num].rhw = 1.0f;
            verts[num].color = color;
            verts[num].u = ch->u;
            verts[num].v = ch->v;
            verts[num + 1].x = cur_x + ch_width;
            verts[num + 1].y = y;
            verts[num + 1].z = 0.0f;
            verts[num + 1].rhw = 1.0f;
            verts[num + 1].color = color;
            verts[num + 1].u = ch->u1;
            verts[num + 1].v = ch->v;
            verts[num + 2].x = cur_x;
            verts[num + 2].y = y + size;
            verts[num + 2].z = 0.0f;
            verts[num + 2].rhw = 1.0f;
            verts[num + 2].color = color;
            verts[num + 2].u = ch->u;
            verts[num + 2].v = tex_height + ch->v;
            verts[num + 3].x = cur_x + ch_width;
            verts[num + 3].y = y + size;
            verts[num + 3].z = 0.0f;
            verts[num + 3].rhw = 1.0f;
            verts[num + 3].color = color;
            verts[num + 3].u = ch->u1;
            verts[num + 3].v = tex_height + ch->v;
            num += 4;
            if (num >= OUTTEXT_MAX_CHARS * 4)
                break;
            cur_x += ch_width - spacing;
        }
        else
        {
            G_FontChar *ch = &f->chars[code & 0xff];
            if (ch->width < .001)
                cur_x += size * .3f;
            else
            {
                ch_width = char_width * ch->width;
                verts[num].x = cur_x;
                verts[num].y = y;
                verts[num].z = 0.0f;
                verts[num].rhw = 1.0f;
                verts[num].color = color;
                verts[num].u = ch->u;
                verts[num].v = ch->v;
                verts[num + 1].x = cur_x + ch_width;
                verts[num + 1].y = y;
                verts[num + 1].z = 0.0f;
                verts[num + 1].rhw = 1.0f;
                verts[num + 1].color = color;
                verts[num + 1].u = ch->u1;
                verts[num + 1].v = ch->v;
                verts[num + 2].x = cur_x;
                verts[num + 2].y = y + size;
                verts[num + 2].z = 0.0f;
                verts[num + 2].rhw = 1.0f;
                verts[num + 2].color = color;
                verts[num + 2].u = ch->u;
                verts[num + 2].v = tex_height + ch->v;
                verts[num + 3].x = cur_x + ch_width;
                verts[num + 3].y = y + size;
                verts[num + 3].z = 0.0f;
                verts[num + 3].rhw = 1.0f;
                verts[num + 3].color = color;
                verts[num + 3].u = ch->u1;
                verts[num + 3].v = tex_height + ch->v;
                cur_x += ch_width - spacing;
                num += 4;
                if (num >= OUTTEXT_MAX_CHARS * 4)
                    break;
            }
        }
    }
    igraph->SetTexture(font_textures[font]);
    igraph->DrawIndexedPrimitiveList(PT_TRIANGLELIST, num, __idx, num / 2, verts, LS3D_STREAM_TLVERTEX);
    if (flags & OUTTEXT_UNDERLINE)
    {
        y += !font ? size * .9f : size * .8f;
        verts[0].x = x;
        verts[0].y = y;
        verts[0].z = 0.0f;
        verts[0].rhw = 1.0f;
        verts[0].color = 0x7fff0000;
        verts[0].u = 0.0f;
        verts[0].v = 0.0f;
        float end = cur_x + spacing;
        verts[1].x = end;
        verts[1].y = y;
        verts[1].z = 0.0f;
        verts[1].rhw = 1.0f;
        verts[1].color = 0x7fff0000;
        verts[1].u = 0.0f;
        verts[1].v = 0.0f;
        igraph->SetTexture(NULL);
        igraph->DrawPrimitiveList(PT_LINELIST, 1, verts, LS3D_STREAM_TLVERTEX);
        verts[0].x = x;
        verts[0].y = y - 1.0f;
        verts[0].z = 0.0f;
        verts[0].rhw = 1.0f;
        verts[0].color = 0x7fff0000;
        verts[0].u = 0.0f;
        verts[0].v = 0.0f;
        verts[1].x = end;
        verts[1].y = y - 1.0f;
        verts[1].z = 0.0f;
        verts[1].rhw = 1.0f;
        verts[1].color = 0x7fff0000;
        verts[1].u = 0.0f;
        verts[1].v = 0.0f;
        igraph->DrawPrimitiveList(PT_LINELIST, 1, verts, LS3D_STREAM_TLVERTEX);
    }
    return cur_x;
}

// The lines end at '~', each sy below the one before
// FUNCTION: GAME 0x604700
void G_IndicatorsClass::OutTextMultiLine(const char *text, float x, float y, float sx, float sy, uint color, uint flags,
    uint font)
{
    if (!text)
        return;
    const char *end;
    do
    {
        end = strchr(text, '~');
        OutText(text, x, y, sx, sy, color, flags, font, end);
        y += sy;
        text = end + 1;
    } while (end);
}

// FUNCTION: GAME 0x604770
void G_IndicatorsClass::DrawCursor(float x, float y)
{
    TLVERTEX verts[4];
    verts[0].x = x;
    verts[0].y = y;
    verts[0].z = 0.0f;
    verts[0].rhw = 1.0f;
    verts[0].color = 0xffffffff;
    verts[0].u = 20.0f / 256;
    verts[0].v = 233.0f / 256;
    verts[1].x = x + 15.0f;
    verts[1].y = y;
    verts[1].z = 0.0f;
    verts[1].rhw = 1.0f;
    verts[1].color = 0xffffffff;
    verts[1].u = 88.0f / 256;
    verts[1].v = 233.0f / 256;
    verts[2].x = x;
    verts[2].y = y + 15.0f;
    verts[2].z = 0.0f;
    verts[2].rhw = 1.0f;
    verts[2].color = 0xffffffff;
    verts[2].u = 20.0f / 256;
    verts[2].v = 250.0f / 256;
    verts[3].x = x + 15.0f;
    verts[3].y = y + 15.0f;
    verts[3].z = 0.0f;
    verts[3].rhw = 1.0f;
    verts[3].color = 0xffffffff;
    verts[3].u = 88.0f / 256;
    verts[3].v = 250.0f / 256;
    igraph->SetTexture(texture_int);
    igraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, verts, LS3D_STREAM_TLVERTEX);
}

// The aspect is the screen's width over its height; the middle screen's is a half or a third of that
// FUNCTION: GAME 0x604890
void G_IndicatorsClass::SetCameraFOV(I3D_camera *cam, float fov)
{
    if (!cam)
        return;
    switch (parheliaIndex)
    {
    case PARHELIA_DUAL:
        if (parhelia_center)
        {
            cam->SetFOV(fov);
            cam->SetAspectRatio((float)igraph->Scrn_sx() / igraph->Scrn_sy() * .5f);
        }
        else
        {
            fov *= 1.6f;
            if (fov > PI * .98f)
                fov = PI * .98f;
            cam->SetFOV(fov);
            cam->SetAspectRatio((float)igraph->Scrn_sx() / igraph->Scrn_sy());
        }
        break;
    case PARHELIA_TRIPLE:
        if (parhelia_center)
        {
            cam->SetFOV(fov);
            cam->SetAspectRatio((float)igraph->Scrn_sx() / igraph->Scrn_sy() * (1.0f / 3));
        }
        else
        {
            fov *= 2;
            if (fov > PI * .98f)
                fov = PI * .98f;
            cam->SetFOV(fov);
            cam->SetAspectRatio((float)igraph->Scrn_sx() / igraph->Scrn_sy());
        }
        break;
    default:
        cam->SetFOV(fov);
        cam->SetAspectRatio((float)igraph->Scrn_sx() / igraph->Scrn_sy());
    }
}

// The viewport's edges, not its size: the left half of two screens, the middle third of three. The scene is read again
// after the screens are cleared, the camera kept from before.
// FUNCTION: GAME 0x604a80
void G_IndicatorsClass::ParheliaSetCenter(bool center)
{
    parhelia_center = center;
    I3D_camera *cam = act_mission->GetScene()->current_camera;
    if (center)
    {
        switch (parheliaIndex)
        {
        case PARHELIA_DUAL:
            igraph->ResetRenderProps();
            igraph->Clear(0, 1.0f, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
            igraph->Present();
            igraph->Clear(0, 1.0f, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
            igraph->Present();
            igraph->Clear(0, 1.0f, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
            igraph->Present();
            act_mission->GetScene()->SetViewport(0, 0, igraph->Scrn_sx() / 2, igraph->Scrn_sy());
            if (cam)
                cam->SetAspectRatio((float)igraph->Scrn_sx() / igraph->Scrn_sy() * .5f);
            break;
        case PARHELIA_TRIPLE:
            igraph->ResetRenderProps();
            igraph->Clear(0, 1.0f, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
            igraph->Present();
            igraph->Clear(0, 1.0f, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
            igraph->Present();
            igraph->Clear(0, 1.0f, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
            igraph->Present();
            act_mission->GetScene()->SetViewport(igraph->Scrn_sx() / 3, 0, igraph->Scrn_sx() / 3 * 2, igraph->Scrn_sy());
            if (cam)
                cam->SetAspectRatio((float)igraph->Scrn_sx() / igraph->Scrn_sy() * (1.0f / 3));
            break;
        default:
            act_mission->GetScene()->SetViewport(0, 0, igraph->Scrn_sx(), igraph->Scrn_sy());
            if (cam)
                cam->SetAspectRatio((float)igraph->Scrn_sx() / igraph->Scrn_sy());
        }
    }
    else
    {
        act_mission->GetScene()->SetViewport(0, 0, igraph->Scrn_sx(), igraph->Scrn_sy());
        if (cam)
            cam->SetAspectRatio((float)igraph->Scrn_sx() / igraph->Scrn_sy());
    }
}
