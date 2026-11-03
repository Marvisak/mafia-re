// The mission's rmorf.bin (PS2 C_rmorf.cpp; the PC's file is the one at 0x499090-0x499b3c, docs/game-files.md): the
// initializer of its global C_rmorf (0x63d7f0), the sets' loader (0x4990d0, PS2 RMRF_S_SETREF::LoadRAWSet), the
// constructor, the destructor, the sets' scalar deleting destructor, then the virtual functions, PS2's order: written
// whole, Mission_Start with PS2's fstring, which it formats the file's name with. Open: three pointer steps in the
// loader.

#include "mission/C_rmorf.h"
#include "mission/Mission.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "I3D/I3D.h"
#include "rw_data.h"

// A std::string formatted as by sprintf, at most 999 characters (PS2 fstring, the copy of its constructor PS2 has here)
class fstring : public std::string
{
public:
    fstring(const char *format, ...);
};

// rmorf.bin's header (the names are ours)
struct S_rmorf_header
{
    int size;                           // the file's, with the header
    float version;                      // RMORF_VERSION
    int num_sets;
};

// The one version the game reads (the name is ours)
#define RMORF_VERSION .1f

// The most regions and levels of a set's weights (the names are ours, by Tick's arrays)
#define RMORF_MAX_REGIONS 16
#define RMORF_MAX_LEVELS 8

// A set's mode (RMRF_S_SETREF::mode; the names are ours, by what Tick does)
#define RMORF_MODE_SMOOTH 1             // the weights move along a half cosine, not linearly
#define RMORF_MODE_CYCLE 0x80           // each level takes the previous one's weight, the first the last one's

// A set read from the mission's rmorf.bin (PS2 RMRF_S_SETREF, C_rmorf.cpp), a list, 0x430 bytes: visuals whose morph
// weights Tick moves, the same in each, from the weights to the weights plus the deltas over a period. The names of the
// members are ours. Defined here, not in C_rmorf.h, for where its scalar deleting destructor goes (C_rmorf.h).
struct RMRF_S_SETREF
{
    int count;                          // 0x0, of the visuals (signed: LoadRAWSet's tests, PS2's too)
    I3D_visual **data;                  // 0x4, the visuals, an array
    RMRF_S_SETREF *next;                // 0x8
    uint num_levels;                    // 0xc
    uint num_regions;                   // 0x10
    uint time;                          // 0x14, in the period
    uint period;                        // 0x18, period_base and a random part of period_rand, each period
    float weights[RMORF_MAX_REGIONS][RMORF_MAX_LEVELS];  // 0x1c, at the period's start
    float deltas[RMORF_MAX_REGIONS][RMORF_MAX_LEVELS];  // 0x21c, by the period's end
    uint mode;                          // 0x41c, RMORF_MODE_*
    uint period_base;                   // 0x420
    uint period_rand;                   // 0x424
    // How many levels the random mode turns on each period: count_base and a random part of count_rand
    uint count_base;                    // 0x428
    uint count_rand;                    // 0x42c

    // A set at the head of a list, the first level of every region at full weight: inlined in LoadRAWSet, PS2's too
    RMRF_S_SETREF(RMRF_S_SETREF *n)
    {
        count = 0;
        data = NULL;
        next = n;
        num_levels = 0;
        num_regions = 0;
        time = 0;
        period = 0;
        mode = 0;
        period_base = 1000;
        period_rand = 1;
        count_base = 1;
        count_rand = 1;
        for (int r = 0; r < RMORF_MAX_REGIONS; r++)
        {
            weights[r][0] = 1.0f;
            deltas[r][0] = 0.0f;
            for (int l = 1; l < RMORF_MAX_LEVELS; l++)
            {
                deltas[r][l] = 0.0f;
                weights[r][l] = 0.0f;
            }
        }
    }

    // Reads a set from data into a new set at the list's head, returning what follows it (PS2 name)
    static char *LoadRAWSet(RMRF_S_SETREF **list, char *data);

    // In the class: C_rmorf's destructor has it inlined, and its scalar deleting destructor calls itself for the list's
    // next set
    ~RMRF_S_SETREF()
    {
        if (data)
            delete[] data;
        data = NULL;
        count = 0;
        if (next)
            delete next;
        next = NULL;
    }
};

// The rmorf module PS2 ticks with its other modules (PS2 name); the PC's game has a C_rmorf of its own, which it ticks,
// and nothing uses this one
// GLOBAL: GAME 0x63d7f0
C_rmorf randomMorfTickableModule;

// A set's header in the file, then its visuals' names, each ending in a 0; the visuals are the scene's frames of those
// names, and the set takes the first one's levels and the most regions of any (PS2's the same, the header read through
// memmove, most likely as a set after another's names needn't start on a word). The header's dwords through a pointer of their own
// give the original's registers (with the data pointer stepped in place, the list and the data swap theirs). Open: the
// original steps that pointer by 4 twice, then once by 0x10 to the names and reads the last three values below it
// (add ebx, 0x10; mov edx, [ebx - 0xc]), where ours steps by 4 six times; memcpy, casts of the data pointer, a char or
// uint pointer, post- or pre-increments, += 1, an index and a header struct don't give it
// FUNCTION: GAME 0x4990d0
char *RMRF_S_SETREF::LoadRAWSet(RMRF_S_SETREF **list, char *data)
{
    I3D_scene *scene = act_mission->scene;
    RMRF_S_SETREF *s = *list = new RMRF_S_SETREF(*list);
    uint *h = (uint *)data;
    s->count = *h++;
    s->mode = *h++;
    s->period_base = *h++;
    s->period_rand = *h++;
    s->count_base = *h++;
    s->count_rand = *h++;
    data = (char *)h;
    if (s->count >= 1)
    {
        s->data = new I3D_visual *[s->count];
        int i;
        for (i = 0; i < s->count; i++)
        {
            s->data[i] = (I3D_visual *)scene->FindFrame(data, ENUMF_ALL);
            data += strlen(data) + 1;
        }
        s->num_levels = s->data[0]->NumLevels();
        s->num_regions = s->data[0]->NumRegions();
        for (i = 1; i < s->count; i++)
        {
            if (s->num_regions < (uint)s->data[i]->NumRegions())
                s->num_regions = s->data[i]->NumRegions();
        }
    }
    return data;
}

// The vtable stored first: an assignment in the body (an initializer stores before it), PS2's order too
// FUNCTION: GAME 0x499280
C_rmorf::C_rmorf()
{
    sets = NULL;
}

// PS2's too: the sets deleted, then the pointer cleared whether there were any
// FUNCTION: GAME 0x499290
C_rmorf::~C_rmorf()
{
    if (sets)
        delete sets;
    sets = NULL;
}

// Reads the mission's sets, each after the last (PS2's the same)
// FUNCTION: GAME 0x499330
void C_rmorf::Mission_Start()
{
    // STRING: GAME 0x63495c
    int h = dtaOpen(fstring("missions\\%s\\rmorf.bin", act_mission->GetMissionName()).c_str(), false);
    if (h < 0)
        return;
    S_rmorf_header header;
    if ((int)dtaRead(h, &header, sizeof(header)) < 0)
    {
        dtaClose(h);
        return;
    }
    if (header.version != RMORF_VERSION)
    {
        dtaClose(h);
        return;
    }
    if (!(header.size - sizeof(header)))
    {
        dtaClose(h);
        return;
    }
    char *data = new char[header.size - sizeof(header)];
    char *p = data;
    if ((int)dtaRead(h, data, header.size - sizeof(header)) >= 0)
    {
        for (int i = 0; i < header.num_sets; i++)
            p = RMRF_S_SETREF::LoadRAWSet(&sets, p);
    }
    delete[] data;
    dtaClose(h);
}

// FUNCTION: GAME 0x499450
fstring::fstring(const char *format, ...)
{
    char buf[1000];
    va_list args;
    va_start(args, format);
    vsprintf(buf, format, args);
    va_end(args);
    assign(buf);
}

// FUNCTION: GAME 0x499560
void C_rmorf::Mission_Done()
{
    if (sets)
        delete sets;
    sets = NULL;
}

// Each set's period: at its end the weights take the deltas, and new deltas are chosen, either each level the previous
// level's weight or a random count of levels with random shares of 1 (the others to 0); then every visual of the set
// gets the weights the time in the period gives
// FUNCTION: GAME 0x4995b0
void C_rmorf::Tick(int time)
{
    if (time <= 0)
        return;
    if (!sets)
        return;
    for (RMRF_S_SETREF *s = sets; s; s = s->next)
    {
        uint num_visuals = s->count;
        uint num_regions = s->num_regions;
        I3D_visual **visuals = s->data;
        uint num_levels = s->num_levels;
        s->time += time;
        if (s->time >= s->period)
        {
            if (!s->period)
                s->time = 0;
            else
                s->time %= s->period;
            s->period = s->period_base;
            if (s->period_rand > 1)
                s->period += rand() % s->period_rand;
            if (s->time >= s->period)
                s->time = 0;
            if (s->mode & RMORF_MODE_CYCLE)
            {
                for (uint r = 0; r < num_regions; r++)
                {
                    uint l;
                    for (l = 0; l < num_levels; l++)
                        s->weights[r][l] += s->deltas[r][l];
                    s->deltas[r][0] = s->weights[r][num_levels - 1] - s->weights[r][0];
                    for (l = 1; l < num_levels; l++)
                        s->deltas[r][l] = s->weights[r][l - 1] - s->weights[r][l];
                }
            }
            else
            {
                for (uint r = 0; r < num_regions; r++)
                {
                    // Which levels are on (1), then which are chosen (2), then their shares
                    uint share[RMORF_MAX_LEVELS];
                    uint num_off = num_levels;
                    uint l;
                    for (l = 0; l < num_levels; l++)
                    {
                        s->weights[r][l] += s->deltas[r][l];
                        if (s->weights[r][l] > 0.0f)
                        {
                            num_off--;
                            share[l] = 1;
                        }
                        else
                            share[l] = 0;
                    }
                    uint total = 0;
                    uint num = s->count_base;
                    if (s->count_rand > 1)
                        num += rand() % s->count_rand;
                    if (!num)
                        num = 1;
                    if (num > num_levels)
                        num = num_levels;
                    // Too few off: all of them, and the rest from those on
                    if (num_off <= num)
                    {
                        for (l = 0; l < num_levels; l++)
                            share[l] = share[l] ? 0 : 2;
                        num -= num_off;
                    }
                    while (num)
                    {
                        uint i;
                        do
                            i = rand() % num_levels;
                        while (share[i]);
                        share[i] = 2;
                        num--;
                    }
                    for (l = 0; l < num_levels; l++)
                    {
                        if (share[l] == 2)
                        {
                            share[l] = rand() % 10 + 1;
                            total += share[l];
                        }
                        else
                            share[l] = 0;
                    }
                    for (l = 0; l < num_levels; l++)
                    {
                        if (share[l])
                            s->deltas[r][l] = (float)share[l] / total - s->weights[r][l];
                        else
                            s->deltas[r][l] = -s->weights[r][l];
                    }
                }
            }
        }
        float t = s->period > 0 ? (float)s->time / s->period : 0.0f;
        switch (s->mode)
        {
        case RMORF_MODE_SMOOTH:
            t = (1.0f - (float)cos(t * PI)) * .5f;
        }
        float weights[RMORF_MAX_REGIONS][RMORF_MAX_LEVELS];
        uint r, l;
        for (r = 0; r < num_regions; r++)
            for (l = 0; l < num_levels; l++)
                weights[r][l] = s->weights[r][l] + s->deltas[r][l] * t;
        for (uint i = 0; i < num_visuals; i++)
        {
            uint n = visuals[i]->NumRegions();
            for (r = 0; r < n; r++)
                for (l = 0; l < num_levels; l++)
                    visuals[i]->SetWeight(r, l, weights[r][l]);
        }
    }
}
