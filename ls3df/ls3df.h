#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <math.h>
#include <string.h>
#include <new.h>
#include <types.h>

// What GetEngineVersion returns and TEV checks the game's version against
#define ENGINE_VERSION 395

// The result of most engine functions (PS2 name). The values' names are guesses from I3DGetResultMessage's texts.
enum LS3D_RESULT {
    I3D_OK = 0,
    I3DERR_NOFILE = 0x80000001,             // "File not found"
    I3DERR_OUTOFMEM = 0x80000002,           // "Out of memory"
    I3DERR_BADTEXTUREFORMAT = 0x80000003,   // "Bad texture format"
    I3DERR_BADTEXTURE = 0x80000004,         // "Bad texture"
    I3DERR_GENERIC = 0x80000005,            // "Generic error"
    I3DERR_OBJECTNOTFOUND = 0x80000006,     // "Object not found"
    I3DERR_INVALIDPARAMS = 0x80000007,      // "Invalid parameter"
    I3DERR_NOTLOADED = 0x80000008,          // "Not loaded"
    I3DERR_NOTINITIALIZED = 0x80000009,     // "Not initialized"
    I3DERR_FILECORRUPTED = 0x8000000a,      // "File corrupted"
    I3DERR_NOADDITIONALFILE = 0x8000000b,   // "No additional file (opacity map)"
    I3DERR_CYCLICLINK = 0x8000000c,         // "Cyclic link of hierarchy"
    I3DERR_CANCELED = 0x8000000d,           // "Enumeration canceled"
    I3DERR_UNSUPPORTED = 0x8000000e,        // "Unsupported"
    I3DERR_NO3DHARDWARE = 0x8000000f,       // "No 3D hardware"
    I3DERR_BUFFERLOST = 0x80000010,         // "Sound buffer lost"
    I3DERR_USERBREAK = 0x80000011,          // "User break"
    I3DERR_FILESYSTEM = 0x80000012,         // "File system error"
    I3DERR_BADVERSION = 0x80000013,         // "Bad version of file"
    I3DERR_BADTIMESTAMP = 0x80000014,       // "Bad timestamp of file"
    I3DERR_LIMITEXCEEDED = 0x80000015,      // "Some limit of engine is exceeded"
};

// The engine's threshold for zero (the names are guesses)
#define MRG_ZERO 1e-8f

// Whether |f| is below MRG_ZERO, compared as integers. The engine's float helpers are inline assembly: every use
// stores the argument for the fld, takes the bits through a pushed temporary and tests the result in al, which no C++
// spelling reproduces. The result is returned in al (warning C4035).
#pragma warning(push)
#pragma warning(disable: 4035)
inline bool IsAbsMrgZero(float f)
{
    __asm {
        fld f
        push eax
        fabs
        fstp dword ptr [esp]
        pop edx
        xor eax, eax
        cmp edx, 0x322bcc77
        setl al
    }
}

// Whether f is below MRG_ZERO, the same way without fabs
inline bool IsMrgZeroLess(float f)
{
    __asm {
        xor eax, eax
        cmp f, 0x322bcc77
        setl al
    }
}
#pragma warning(pop)

// The engine's array (PS2 C_list<T>): it grows by grow_size elements at a time and removes an element by moving the
// last one into its place. The PS2 build has reserve, push_back and operator= out of line for some element types;
// the member names are guesses.
template<class T>
class C_list {
    T *array;
    int res_size;
    int count;
    int grow_size;

public:
    C_list(int grow = 32) : array(NULL), res_size(0), count(0), grow_size(grow) {}

    ~C_list()
    {
        delete[] array;
        array = NULL;
        count = 0;
        res_size = 0;
    }

    int size() const
    {
        return count;
    }

    T &operator[](int i)
    {
        return array[i];
    }

    const T &operator[](int i) const
    {
        return array[i];
    }

    // Appends an element and returns it, for the caller to fill in
    T &push_back()
    {
        if (count >= res_size) {
            res_size += grow_size;
            T *a = new T[res_size];
            for (int i = 0; i < count; i++)
                a[i] = array[i];
            delete[] array;
            array = a;
        }
        return array[count++];
    }

    // Removes element i, moving the last one into its place
    void erase(int i)
    {
        if (--count)
            array[i] = array[count];
    }

    // Removes the first element equal to v the same way
    void remove(const T &v)
    {
        if (count <= 0)
            return;
        if (count == 1) {
            if (array[0] == v)
                count = 0;
            return;
        }
        T *p = array;
        for (int i = count - 1; i--; p++) {
            if (*p == v) {
                *p = array[--count];
                return;
            }
        }
        if (*p == v)
            --count;
    }
};

// The engine's reference-counted string (PS2 C_str). An empty string has no buffer and converts to the address of
// its null pointer, which reads as "".
class C_str {
    struct S_buffer {
        int ref;
        int len;
        char str[1];

        S_buffer(const char *s, int l) : ref(1), len(l)
        {
            memcpy(str, s, l);
            str[l] = 0;
        }

        void Release()
        {
            if (!--ref)
                delete this;
        }
    } *buf;

public:
    C_str() : buf(NULL) {}

    C_str(const char *s)
    {
        int len = strlen(s);
        buf = new (new char[len + 9]) S_buffer(s, len);
    }

    ~C_str()
    {
        if (buf)
            buf->Release();
    }

    // The out-of-line copy the linker kept is in IGraph code
    // FUNCTION: LS3DF 0x1006ff00
    C_str &operator=(const char *s)
    {
        // b starts as s, NULL when there is no string: the original keeps both in one register and never sets b to
        // NULL, which neither NULL first nor an else branch reproduces
        S_buffer *b = (S_buffer *)s;
        if (s) {
            int len = strlen(s);
            b = new (new char[len + 9]) S_buffer(s, len);
        }
        if (buf)
            buf->Release();
        buf = b;
        return *this;
    }

    operator const char *() const
    {
        return buf ? buf->str : (const char *)&buf;
    }
};

class IGraph;
class I3D_driver;
class ISND_driver;

// The engine's three drivers, which GetIGraph, I3DGetDriver and ISndGetDriver return
extern bool disableLog;
extern IGraph *pGlobalIGraph;
extern I3D_driver *pI3DDriver;
extern ISND_driver *pISndDriver;

// The undecorated exports are __cdecl, where the engine's default is __fastcall (/Gr). __declspec(dllexport) has to
// come before the return type: VC6 ignores it after a '*' (C4518).
#define LS3D_EXPORT extern "C" __declspec(dllexport)

LS3D_EXPORT int __cdecl GetEngineVersion();
LS3D_EXPORT bool __cdecl TEV(const char *title, int engine_version);
LS3D_EXPORT IGraph *__cdecl GetIGraph();
LS3D_EXPORT ISND_driver *__cdecl ISndGetDriver();
LS3D_EXPORT I3D_driver *__cdecl I3DGetDriver();
LS3D_EXPORT const char *__cdecl I3DGetResultMessage(LS3D_RESULT result);
// Append a line to log.txt next to the executable, unless disableLog is set
LS3D_EXPORT int __cdecl dbgPrintf(const char *format, ...);
LS3D_EXPORT int __cdecl dbgPrintf1(const char *format, ...);
void dbgPrintGuid(const void *guid);
