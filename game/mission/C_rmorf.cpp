// The mission's rmorf.bin (PS2 C_rmorf.cpp; the PC's file is the one at 0x499090-0x499b3c, docs/game-files.md): the
// initializer of its global C_rmorf (0x63d7f0), the sets' loader (0x4990d0, PS2 RMRF_S_SETREF::LoadRAWSet), the
// constructor, the destructor, the sets' scalar deleting destructor, then the virtual functions, PS2's order. Written
// so far: the constructor, the destructor, Mission_Start with PS2's fstring, which it formats the file's name with,
// Mission_Done and Tick, which C_game::Tick calls; the sets' loader is a stub.

#include "mission/C_rmorf.h"
#include "mission/Mission.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "I3D/I3D.h"
#include "rw_data.h"
#include "stub.h"

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

#pragma auto_inline(off)

// STUB: GAME 0x4990d0
char *RMRF_S_SETREF::LoadRAWSet(RMRF_S_SETREF **list, char *data)
{
    StubLog("RMRF_S_SETREF::LoadRAWSet");
    return data;
}

#pragma auto_inline(on)

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
