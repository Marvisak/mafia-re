// The 32-bit helper of tools/dtadump.py, which builds it with the repository's VC6 and runs it in the game's folder:
// opens the archives through rw_data.dll with the keys Game.exe's InitSystem passes and copies files out of them.
//
//     dtadump <rw_data.dll> <out dir> <file in the archives>...
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <direct.h>

// rw_data's archive object (rw_data/rw_data.h): what dtaCreate returns
class C_rw_data
{
public:
    virtual ~C_rw_data() {}
    virtual int AddRef() = 0;
    virtual int Release() = 0;
    virtual bool UnlockPack(__int64 key) = 0;
};

typedef C_rw_data *(__stdcall *T_dtaCreate)(const char *file_name);
typedef int(__stdcall *T_dtaOpen)(const char *file_name, bool force_dta);
typedef unsigned(__stdcall *T_dtaRead)(int handle, unsigned char *buffer, unsigned len);
typedef bool(__stdcall *T_dtaClose)(int handle);

// The archives in the order of the table at 0x637890, with the keys InitSystem passes to UnlockPack (0x5bf620-0x5bf77b)
static const struct
{
    const char *name;
    unsigned key_lo, key_hi;
} archives[] = {
    {"a8.dta", 0x6a63fa71, 0xec45d8ce}, {"b8.dta", 0x79e21cdb, 0xb60f823f}, {"a2.dta", 0x1417d340, 0xb6399e19},
    {"a6.dta", 0x728e2db9, 0x5055da68}, {"a0.dta", 0x7f3d9b74, 0xec48fe17}, {"a1.dta", 0xe7375f59, 0x0900210e},
    {"a3.dta", 0xa94b8d3c, 0x771f3888}, {"ac.dta", 0xa94b8d3c, 0x771f3888}, {"a4.dta", 0xa94b8d3c, 0x771f3888},
    {"aa.dta", 0xd4ad90c6, 0x67da216e}, {"a5.dta", 0x4f4bb0c6, 0xea340420}, {"a7.dta", 0xf4f03a72, 0xe266fe62},
    {"a9.dta", 0x959d1117, 0x5b763446}, {"ab.dta", 0x7f3d9b74, 0xec48fe17},
};

int main(int argc, char **argv)
{
    int i;
    if (argc < 4)
    {
        printf("dtadump <rw_data.dll> <out dir> <file>...\n");
        return 1;
    }
    HMODULE dll = LoadLibraryA(argv[1]);
    if (!dll)
    {
        printf("can't load %s\n", argv[1]);
        return 1;
    }
    T_dtaCreate dtaCreate = (T_dtaCreate)GetProcAddress(dll, "_dtaCreate@4");
    T_dtaOpen dtaOpen = (T_dtaOpen)GetProcAddress(dll, "_dtaOpen@8");
    T_dtaRead dtaRead = (T_dtaRead)GetProcAddress(dll, "_dtaRead@12");
    T_dtaClose dtaClose = (T_dtaClose)GetProcAddress(dll, "_dtaClose@4");
    // The game doesn't need b8.dta, nor does Steam's install have it
    for (i = 0; i < sizeof(archives) / sizeof(archives[0]); i++)
    {
        C_rw_data *dta = dtaCreate(archives[i].name);
        if (dta)
            dta->UnlockPack(((__int64)archives[i].key_hi << 32) | archives[i].key_lo);
    }
    _mkdir(argv[2]);
    int failed = 0;
    for (i = 3; i < argc; i++)
    {
        int handle = dtaOpen(argv[i], false);
        if (handle < 0)
        {
            printf("%s: not found\n", argv[i]);
            failed = 1;
            continue;
        }
        const char *base = strrchr(argv[i], '\\');
        char out_name[MAX_PATH];
        sprintf(out_name, "%s\\%s", argv[2], base ? base + 1 : argv[i]);
        FILE *out = fopen(out_name, "wb");
        if (!out)
        {
            printf("%s: can't write %s\n", argv[i], out_name);
            dtaClose(handle);
            failed = 1;
            continue;
        }
        static unsigned char buffer[0x8000];
        unsigned len, total = 0;
        while ((len = dtaRead(handle, buffer, sizeof(buffer))) != 0 && len != (unsigned)-1)
        {
            fwrite(buffer, 1, len, out);
            total += len;
        }
        fclose(out);
        dtaClose(handle);
        printf("%s: %u bytes -> %s\n", argv[i], total, out_name);
    }
    return failed;
}
