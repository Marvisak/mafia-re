// The mission's rmorf.bin (PS2 C_rmorf.cpp; the PC's file is the one at 0x499090-0x499b3c, docs/game-files.md): the
// initializer of its global C_rmorf (0x63d7f0), the sets' loader (0x4990d0, PS2 RMRF_S_SETREF::LoadRAWSet), the
// constructor, the destructor, the sets' scalar deleting destructor, then the virtual functions, PS2's order. Written
// so far: the constructor, the destructor, Mission_Start with PS2's fstring, which it formats the file's name with, and
// Mission_Done; the sets' loader and the Tick are stubs.

#include "mission/C_rmorf.h"
#include "mission/Mission.h"

#include <stdarg.h>
#include <stdio.h>

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

#pragma auto_inline(off)

// STUB: GAME 0x4995b0
void C_rmorf::Tick(int time)
{
    StubLog("C_rmorf::Tick %i", time);
}

#pragma auto_inline(on)
