// The engine's globals and its plain C exports, at 0x1006b430-0x1006bd3e in the original. The PS2 build names the
// file (__sinit_Globals.cpp): its functions TEV to ErrorHandler come right before IGraph.cpp's there, as they do here.
// Both builds list it in one group with IGraph.cpp, IShow.cpp and ITexture.cpp, hence the IGraph folder. The PC adds
// GetEngineVersion, dbgPrintf1 and the log file. The file ends with the header's zero vector's $E (see
// tools/autopch.py).
// The original's .bss (0x101c5048-0x101c529f): log_name, str, UnitVector, ZeroVector, timer_frequency, UnitBBox, the
// header's zero vector, 0x60 bytes nothing references, v, v's guard, 1 byte nothing references, StaticInstance, then
// the zero-initialized globals. The unreferenced bytes look like another function's static S_vector[8] and its guard,
// a function /OPT:REF dropped. VC6's hash (tools/bsshash.py) orders by name: log_name and timer_frequency (PC only)
// and that function are still to be named, and v's decorated name depends on BBoxVertices' return type. The other
// names are PS2's, the two function statics' included (str$674, v$795 next to UnitBBox in PS2's .bss).
#include "ls3df.h"
#include "I3D/I3D.h"
#include "IGraph/IGraph.h"
#include "ISound/ISound.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// The log file, log.txt in the executable's folder, set by the first function that writes to it
// GLOBAL: LS3DF 0x101c5048
static char log_name[256];

// QueryPerformanceFrequency's ticks per 1/16 of a microsecond, for the engine's timer
// GLOBAL: LS3DF 0x101c51a0
ulong timer_frequency;

// VC6 lays out zero-initialized globals in declaration order, after the ones its .bss hash places (StaticInstance).
// Hashed, pGlobalIGraph would come first.

// GLOBAL: LS3DF 0x101c5293
bool disableLog = false;

// GLOBAL: LS3DF 0x101c5294
IGraph *pGlobalIGraph = NULL;

// GLOBAL: LS3DF 0x101c5298
I3D_driver *pI3DDriver = NULL;

// GLOBAL: LS3DF 0x101c529c
ISND_driver *pISndDriver = NULL;

void dbgClear();

// The engine's start and end: the constructor clears the log and creates the three drivers, the destructor deletes
// them (PS2 names). The constructor is defined in the class, so its out-of-line copy comes right after the $E that
// constructs StaticInstance, as the original's does.
class BreBre {
public:
    // FUNCTION: LS3DF 0x1006b450
    BreBre()
    {
        dbgClear();
        dbgPrintf("LS3D Engine V3.95.1\t\t\t%s    GetVer:%d\n", "Jun 24 2003", ENGINE_VERSION);
        QueryPerformanceFrequency((LARGE_INTEGER *)&timer_frequency);
        timer_frequency = (timer_frequency << 4) / 1000000;
        if (!timer_frequency)
            timer_frequency = 1;
        pGlobalIGraph = new IGraph;
        pI3DDriver = new I3D_driver;
        pISndDriver = new ISND_driver;
    }

    ~BreBre()
    {
        delete pI3DDriver;
        pI3DDriver = NULL;
        delete pGlobalIGraph;
        pGlobalIGraph = NULL;
        delete pISndDriver;
        pISndDriver = NULL;
    }
};

// GLOBAL: LS3DF 0x101c5292
static BreBre StaticInstance;

// FUNCTION: LS3DF 0x1006b6d0
int __cdecl GetEngineVersion()
{
    return ENGINE_VERSION;
}

// FUNCTION: LS3DF 0x1006b6e0
bool __cdecl TEV(const char *title, int engine_version)
{
    if (engine_version == ENGINE_VERSION)
        return true;
    char text[128];
    sprintf(text, "Bad version - engine:%d  client:%d", ENGINE_VERSION, engine_version);
    MessageBoxA(NULL, text, title, MB_SYSTEMMODAL | MB_ICONSTOP);
    return false;
}

// FUNCTION: LS3DF 0x1006b740
IGraph *__cdecl GetIGraph()
{
    return pGlobalIGraph;
}

// FUNCTION: LS3DF 0x1006b750
ISND_driver *__cdecl ISndGetDriver()
{
    return pISndDriver;
}

// FUNCTION: LS3DF 0x1006b760
I3D_driver *__cdecl I3DGetDriver()
{
    return pI3DDriver;
}

// FUNCTION: LS3DF 0x1006b770
const char *__cdecl I3DGetResultMessage(LS3D_RESULT result)
{
    switch (result) {
    case I3D_OK:
        return "OK";
    case I3DERR_NOFILE:
        return "File not found";
    case I3DERR_OUTOFMEM:
        return "Out of memory";
    case I3DERR_BADTEXTUREFORMAT:
        return "Bad texture format";
    case I3DERR_BADTEXTURE:
        return "Bad texture";
    case I3DERR_GENERIC:
        return "Generic error";
    case I3DERR_OBJECTNOTFOUND:
        return "Object not found";
    case I3DERR_INVALIDPARAMS:
        return "Invalid parameter";
    case I3DERR_NOTLOADED:
        return "Not loaded";
    case I3DERR_NOTINITIALIZED:
        return "Not initialized";
    case I3DERR_FILECORRUPTED:
        return "File corrupted";
    case I3DERR_NOFILE1:
        return "No additional file (opacity map)";
    case I3DERR_CYCLICLINK:
        return "Cyclic link of hierarchy";
    case I3DERR_CANCELED:
        return "Enumeration canceled";
    case I3DERR_UNSUPPORTED:
        return "Unsupported";
    case I3DERR_NO3DHARDWARE:
        return "No 3D hardware";
    case I3DERR_BUFFERLOST:
        return "Sound buffer lost";
    case I3DERR_USERBREAK:
        return "User break";
    case I3DERR_FILESYSTEM:
        return "File system error";
    case I3DERR_BADVERSION:
        return "Bad version of file";
    case I3DERR_BADTIMESTAMP:
        return "Bad timestamp of file";
    case I3DERR_LIMITEXCEEDED:
        return "Some limit of engine is exceeded";
    }
    // GLOBAL: LS3DF 0x101c5148
    static char str[64];
    sprintf(str, "Unknown result:0x%x", result);
    return str;
}

// The command line's folder, without the quote that starts a quoted one, and log.txt. It is inlined everywhere.
static void InitLogName()
{
    if (log_name[0])
        return;
    char dir[248];
    _splitpath(GetCommandLineA(), log_name, dir, NULL, NULL);
    if (dir[0] == '"')
        strcat(log_name, dir + 1);
    else
        strcat(log_name, dir);
    strcat(log_name, "log.txt");
}

// Starts a new log with the date. BreBre's constructor inlines it though it is defined further down (/Ob2), so
// /OPT:REF drops the out-of-line copy; PS2 has it out of line (dbgClear__Fv).
void dbgClear()
{
    InitLogName();
    FILE *f = fopen(log_name, "wt");
    if (!f)
        return;
    char date[16], time[16];
    _strdate(date);
    _strtime(time);
    fprintf(f, "Date: %s    time:%s\n", date, time);
    fclose(f);
}

// FUNCTION: LS3DF 0x1006b890
int __cdecl dbgPrintf(const char *format, ...)
{
    if (disableLog)
        return 0;
    InitLogName();
    FILE *f = fopen(log_name, "at");
    if (!f)
        return -1;
    int n = vfprintf(f, format, (va_list)(&format + 1));
    fprintf(f, "\n");
    fclose(f);
    return n;
}

// dbgPrintf without the line break
// FUNCTION: LS3DF 0x1006b980
int __cdecl dbgPrintf1(const char *format, ...)
{
    if (disableLog)
        return 0;
    InitLogName();
    FILE *f = fopen(log_name, "at");
    if (!f)
        return -1;
    int n = vfprintf(f, format, (va_list)(&format + 1));
    fclose(f);
    return n;
}

// FUNCTION: LS3DF 0x1006ba70
void dbgPrintGuid(const void *guid)
{
    if (disableLog || !guid)
        return;
    InitLogName();
    FILE *f = fopen(log_name, "at");
    if (!f)
        return;
    const GUID &g = *(const GUID *)guid;
    fprintf(f, "                GUID: %08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X\n", g.Data1, g.Data2, g.Data3,
        g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3], g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);
    fclose(f);
}

// GLOBAL: LS3DF 0x101c51a8
I3D_bbox UnitBBox(S_vector(-1.0f, -1.0f, -1.0f), S_vector(1.0f, 1.0f, 1.0f));

// FUNCTION: LS3DF 0x1006bc00
const S_vector *BBoxVertices(const I3D_bbox &bbox)
{
    // GLOBAL: LS3DF 0x101c5230
    static S_vector v[8];
    S_vector *p = v;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            p->x = i ? bbox.min.x : bbox.max.x;
            p->y = j ? bbox.min.y : bbox.max.y;
            p->z = bbox.min.z;
            p++;
            p->x = i ? bbox.min.x : bbox.max.x;
            p->y = j ? bbox.min.y : bbox.max.y;
            p->z = bbox.max.z;
            p++;
        }
    }
    return v;
}

// GLOBAL: LS3DF 0x101c5188
S_vector UnitVector(1.0f, 1.0f, 1.0f);

// GLOBAL: LS3DF 0x101c5194
S_vector ZeroVector(0.0f, 0.0f, 0.0f);

// Hands an error to the application's handler and exits on a fatal one
// FUNCTION: LS3DF 0x1006bce0
void ErrorHandler(LS3D_RESULT result, E_SEVERITY severity, const char *message)
{
    if (m_InitDesc.error_handler)
        m_InitDesc.error_handler(result, severity, message ? message : "");
    if (severity >= ERR_FATAL)
        exit(-1);
}
