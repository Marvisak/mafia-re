#pragma once

#include <string.h>
#include <types.h>

// The reference-counted string of the engine and the game (PS2 C_str), which ls3df.h includes. An empty string has no
// buffer and converts to the address of its null pointer, which reads as "". Insanity3D's C_str (a later, wider
// version, in a header of its own, C_str.hpp) has the same design; the names of the buffer and its members are its.
// Each binary has its own copies of the inline functions, so both have their markers here. A header of its own counts
// for LS3DF's /YX otherwise than the same class in ls3df.h (IGraph.cpp's and I3d_scene.cpp's comments).
class C_str {
    // LS3DF's out-of-line copies the linker kept are in IGraph.cpp
    struct C_rep {
        int ref;
        int size;
        char data[1];

        // FUNCTION: LS3DF 0x1006fe20
        // FUNCTION: LS3DF11 0x1006ceb0
        // FUNCTION: GAME 0x408c50
        C_rep(const char *s, int len) : ref(1), size(len)
        {
            memcpy(data, s, len);
            data[len] = 0;
        }

        // FUNCTION: LS3DF 0x1006fe60
        // FUNCTION: LS3DF11 0x1006cef0
        // FUNCTION: GAME 0x566780
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

        // The buffer was allocated as a char array (PS2 frees it with delete[]; delete this gives the same code)
        // FUNCTION: LS3DF 0x1006fec0
        // FUNCTION: LS3DF11 0x1006cf50
        // FUNCTION: GAME 0x403310
        void Release()
        {
            if (!--ref)
                delete[] (char *)this;
        }

        // The buffer holds the string after the counts. The unwinding after a constructor that throws calls the
        // delete out of line, with the length in edx (/Gr). The buffer is constructed by a new-expression: the
        // original's null path has a new-expression's own xor eax, eax.
        static void *operator new(size_t, int len)
        {
            return new char[len + 9];      // the two counts, the string and its end
        }

        // FUNCTION: LS3DF 0x1006fed0
        // FUNCTION: LS3DF11 0x1006cf60
        // FUNCTION: GAME 0x408c90
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
    // FUNCTION: LS3DF11 0x1006cf70
    ~C_str()
    {
        if (rep)
            rep->Release();
    }

    // FUNCTION: GAME 0x5c08b0
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
    // FUNCTION: LS3DF11 0x1006cf90
    // FUNCTION: GAME 0x4033d0
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
    // FUNCTION: LS3DF11 0x1006d010
    // FUNCTION: GAME 0x403450
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
    // FUNCTION: LS3DF11 0x1006d190
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

    // The game's: the caches' DelDir and Open use them, LS3DF doesn't. Open: their bodies count for LS3DF's /YX
    // (tools/autopch.py), which puts 15 files' deferred items later (the interpolator's, occluder's and math's $E at
    // the original's places, the driver's, sound's and particles' one function past them), and they reorder 1.1's
    // I3D_lit_object::m_AddFGroups (equivalent, not identical).

    // The same buffer, or the same size and bytes
    bool operator==(const C_str &s) const
    {
        if (rep == s.rep)
            return true;
        if (Size() != s.Size())
            return false;
        return !memcmp(*this, s, Size());
    }

    // A letter's upper case. A member, though it doesn't use the string: its out-of-line copy (0x408ca0) is
    // __thiscall. The name is ours.
    int ToUpper(int c) const
    {
        if (c >= 'a' && c <= 'z')
            c += 'A' - 'a';
        return c;
    }

    // The string against a mask: * matches the rest, ? any one character, the others ignore case. An empty string
    // matches only an empty mask. The name is ours. The switch gives the original's layout (the ? case after the
    // others' test).
    // Open: the original's upper case adds -0x20 (add ecx, -0x20), ours subtracts 0x20 in every spelling and type
    // tried.
    // FUNCTION: GAME 0x4035d0
    bool Match(const C_str &mask) const
    {
        uint len = Size(), len1 = mask.Size();
        if (!len || !len1)
            return len == len1;
        const char *cp = *this, *cp1 = mask;
        while (*cp1) {
            switch (*cp1) {
            case '*':
                return true;
            case '?':
                if (!*cp)
                    return false;
                break;
            default:
                if (ToUpper(*cp) != ToUpper(*cp1))
                    return false;
            }
            cp++;
            cp1++;
        }
        return !*cp;
    }

    operator const char *() const
    {
        return rep ? rep->data : (const char *)&rep;
    }
};
