// The HUD (PS2 g_indicators.cpp; the PC's file is the one at 0x5f6a20-0x604e5c, docs/game-files.md). Written so far:
// the constructor, ResetValues and the setters it calls, LoadFont, LanguageSelect, Create, Destroy, _SpocitejRafiku2
// and ResolutionChanged; the rest of what main and the profiles call is stubs.

#include "ui/g_indicators.h"
#include "ui/g_textdb.h"

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
    unknown_4208 = 0;
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
    unknown_4278 = 0;
    unknown_4458 = .5f;
    unknown_43e4 = 0;
    unknown_43e8 = 0;
    SYSTEMTIME time;
    GetLocalTime(&time);
    TimerSetTime(time.wHour, time.wMinute, time.wSecond);
    TimerSetInterval(130);
    WideScreenSetRatio(.5f);
    memset(_VtxTimerCifernik, 0, sizeof(_VtxTimerCifernik));
    memset(_VtxTimerInterval, 0, sizeof(_VtxTimerInterval));
    unknown_44a8 = 0;
    radar_cars.clear();
    unknown_44bc = 0;
    radar_items.clear();
    unknown_44c0 = .9f;
    unknown_44cc = 0;
    unknown_44d0 = 0;
    race_best_time[0] = 0;
    race_lap_time[0] = 0;
    race_time[0] = 0;
    race_position[0] = 0;
    race_laps[0] = 0;
    unknown_46dc = 0;
    unknown_4484 = 0;
    unknown_4490 = 0;
    unknown_448c = 0;
    unknown_4480 = 0;
    unknown_46e0 = 0;
    unknown_46e4 = -1;
    unknown_46ec = false;
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
        dtaRead(file, fonts[font], sizeof(fonts[font]));
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
    unknown_4084 = NULL;
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
    if (unknown_4084)
        unknown_4084->Release();
    unknown_4084 = NULL;
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
            unknown_4208 = 0;
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

// FUNCTION: GAME 0x5f9940
void G_IndicatorsClass::IntroSubtitlesClear()
{
    intro_subtitles.clear();
    intro_subtitles_waiting.clear();
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

// FUNCTION: GAME 0x5fb010
void G_IndicatorsClass::BonusSetCount(int count)
{
    bonus_count = count;
    if (count != BONUS_NONE)
        sprintf(bonus_text, textDatabase.GetText(0xb3), count);
    else
        bonus_text[0] = 0;
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

#pragma auto_inline(off)

// STUB: GAME 0x604a80
void G_IndicatorsClass::ParheliaSetCenter(bool center)
{
    StubLog("G_IndicatorsClass::ParheliaSetCenter");
}

#pragma auto_inline(on)
