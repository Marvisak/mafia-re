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
    I3DERR_NOFILE1 = 0x8000000b,   // "No additional file (opacity map)"
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

// The engine's threshold for zero ("marginal zero") and its bits. The names are those of Insanity3D's I3d_math.h
// (Lonely Cat Games' later engine of the same lineage, published with source), which has both helpers below as
// inline assembly too.
#define MRG_ZERO 1e-8f
#define MRG_ZERO_BITMASK 0x322bcc77

// Whether |f| is below MRG_ZERO, compared as integers. The engine's float helpers are inline assembly: every use
// stores the argument for the fld, takes the bits through a pushed temporary and tests the result in al, which no C++
// spelling reproduces. The result is returned in al (warning C4035). The Xbox build has both helpers out of line with
// these instructions (0x20e830, 0x20e840): its compiler doesn't inline functions that contain assembly.
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
        cmp edx, MRG_ZERO_BITMASK
        setl al
    }
}

// Whether f is below MRG_ZERO, the same way without fabs
inline bool IsMrgZeroLess(float f)
{
    __asm {
        xor eax, eax
        cmp f, MRG_ZERO_BITMASK
        setl al
    }
}
#pragma warning(pop)

// Rounds f to an int with the FPU's rounding, without __ftol (Insanity3D's FloatToInt, the same assembly)
inline int FloatToInt(float f)
{
    __asm {
        fld f
        fistp f
    }
    return *(int *)&f;
}

// The engine's array (PS2 C_list<T>): it grows by grow_size elements at a time and removes an element by moving the
// last one into its place. The PS2 build has reserve, push_back and operator= out of line for some element types;
// the member names are guesses. By default it grows by 128 bytes' worth: 32 pointers, but 2 of ITexture.cpp's
// 0x2c-byte texture slots. That default constructor takes no arguments: an array of lists calls it out of line
// (ITexture.cpp's pool array), where a default argument would call a closure that doesn't return this.
template<class T>
class C_list {
    T *array;
    int res_size;
    int count;
    int grow_size;

public:
    C_list() : array(NULL), res_size(0), count(0), grow_size(128 / sizeof(T)) {}

    C_list(int grow) : array(NULL), res_size(0), count(0), grow_size(grow) {}

    void SetGrowSize(int grow)
    {
        grow_size = grow;
    }

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

    // Frees the array
    void clear()
    {
        delete[] array;
        array = NULL;
        count = 0;
        res_size = 0;
    }

    T &operator[](int i)
    {
        return array[i];
    }

    const T &operator[](int i) const
    {
        return array[i];
    }

    // Makes room for n elements (PS2 has it out of line for some element types)
    void reserve(int n)
    {
        if (n < count)
            n = count;
        if (res_size >= n)
            return;
        res_size = n;
        T *a = new T[n];
        for (int i = 0; i < count; i++)
            a[i] = array[i];
        delete[] array;
        array = a;
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

    void push_back(const T &v)
    {
        if (count >= res_size) {
            res_size += grow_size;
            T *a = new T[res_size];
            for (int i = 0; i < count; i++)
                a[i] = array[i];
            delete[] array;
            array = a;
        }
        array[count] = v;
        count++;
    }

    // Removes element i, moving the last one into its place
    void erase(int i)
    {
        if (--count)
            array[i] = array[count];
    }

    // Removes the first element equal to v the same way, and tells whether there was one
    bool remove(const T &v)
    {
        if (count <= 0)
            return false;
        if (count == 1) {
            if (array[0] != v)
                return false;
            count = 0;
            return true;
        }
        T *p = array;
        for (int i = count - 1; i--; p++) {
            if (*p == v) {
                *p = array[--count];
                return true;
            }
        }
        if (*p != v)
            return false;
        --count;
        return true;
    }
};

// The engine's reference-counted string (PS2 C_str). An empty string has no buffer and converts to the address of
// its null pointer, which reads as "". Insanity3D's C_str (a later, wider version) has the same design; the names of
// the buffer and its members are its.
class C_str {
    // The out-of-line copies the linker kept are in IGraph.cpp
    struct C_rep {
        int ref;
        int size;
        char data[1];

        // FUNCTION: LS3DF 0x1006fe20
        C_rep(const char *s, int len) : ref(1), size(len)
        {
            memcpy(data, s, len);
            data[len] = 0;
        }

        // FUNCTION: LS3DF 0x1006fe60
        C_rep(const char *s1, const char *s2, int len1, int len2) : ref(1), size(len1 + len2)
        {
            memcpy(data, s1, len1);
            memcpy(data + len1, s2, len2);
            data[size] = 0;
        }

        int Size() const
        {
            return size;
        }

        // The buffer was allocated as a char array (PS2 frees it with delete[])
        // FUNCTION: LS3DF 0x1006fec0
        void Release()
        {
            if (!--ref)
                delete[] (char *)this;
        }

        // The buffer holds the string after the counts. The unwinding after a constructor that throws calls the
        // delete out of line, with the length in edx (/Gr).
        static void *operator new(size_t, int len)
        {
            return new char[len + 9];
        }

        // FUNCTION: LS3DF 0x1006fed0
        static void operator delete(void *p, int)
        {
            delete[] (char *)p;
        }
    } *rep;

public:
    C_str() : rep(NULL) {}

    C_str(const char *cp)
    {
        if (!cp)
            rep = NULL;
        else {
            int len = strlen(cp);
            rep = new (len) C_rep(cp, len);
        }
    }

    C_str(const C_str &s) : rep(s.rep)
    {
        if (rep)
            ++rep->ref;
    }

    // FUNCTION: LS3DF 0x1006fee0
    ~C_str()
    {
        if (rep)
            rep->Release();
    }

    C_str &operator=(const C_str &s)
    {
        if (s.rep)
            ++s.rep->ref;
        if (rep)
            rep->Release();
        rep = s.rep;
        return *this;
    }

    // FUNCTION: LS3DF 0x1006ff00
    C_str &operator=(const char *cp)
    {
        // NULL first, as in Insanity3D: PS2's order (the string's case first, NULL in an else) gives other code
        C_rep *new_rep;
        if (!cp)
            new_rep = NULL;
        else {
            int len = strlen(cp);
            new_rep = new (len) C_rep(cp, len);
        }
        if (rep)
            rep->Release();
        rep = new_rep;
        return *this;
    }

    C_str &operator+=(const char *cp)
    {
        if (!rep)
            return operator=(cp);
        int len = rep->size, len1 = strlen(cp);
        C_rep *new_rep = new (len + len1) C_rep(rep->data, cp, len, len1);
        rep->Release();
        rep = new_rep;
        return *this;
    }

    // FUNCTION: LS3DF 0x1006ff80
    C_str operator+(const char *cp) const
    {
        if (!rep)
            return cp;
        int len = rep->size, len1 = strlen(cp);
        C_str ret;
        ret.rep = new (len + len1) C_rep(rep->data, cp, len, len1);
        return ret;
    }

    // A character to change: the buffer is made the string's own first. The length for new is an accessor's value,
    // which the unwinding keeps for the delete (IGraph::CreateITexture); rep->size would be read again there.
    // FUNCTION: LS3DF 0x10070100
    char &operator[](int pos)
    {
        if (rep->ref != 1) {
            C_rep *new_rep = new (rep->Size()) C_rep(rep->data, rep->size);
            rep->Release();
            rep = new_rep;
        }
        return rep->data[pos];
    }

    // Unsigned, as Insanity3D's size_t: IGraph::CreateITexture's Size() > 0 is an unsigned compare
    uint Size() const
    {
        return rep ? rep->size : 0;
    }

    operator const char *() const
    {
        return rep ? rep->data : (const char *)&rep;
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

// The destructors IGraph.cpp's thunks in .CRT$XCU register with atexit for m_AdapterFormatList, m_CachedTextureList,
// m_UncachedTextureList and m_MapsList, by name. VC6 numbers these functions per file, and only these names are in
// no other file; reccmp pairs a name several files have ($E2, $E7, $E12) only with a warning for each, so the other
// destructors stay unmatched, and their thunks below 100% though identical.
// SYNTHETIC: LS3DF 0x1006be60
// $E17
// SYNTHETIC: LS3DF 0x1006beb0
// $E22
// SYNTHETIC: LS3DF 0x1006bf00
// $E27
// SYNTHETIC: LS3DF 0x1006bf50
// $E32
