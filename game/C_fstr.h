#pragma once

#include <stdarg.h>
#include <stdio.h>

#include <C_str.h>

// A string formatted as by sprintf, at most 1023 characters (Insanity3D's C_fstr; PS2's C_fstring). The game's: the
// engine has no use for it, and its header can't bring stdio.h to the engine (that moves LS3DF's code). The constructor
// is __cdecl, with the object's address pushed first.
class C_fstr : public C_str {
public:
    // FUNCTION: GAME 0x408cc0
    C_fstr(const char *format, ...)
    {
        char buf[1024];
        va_list args;
        va_start(args, format);
        _vsnprintf(buf, sizeof(buf), format, args);
        va_end(args);
        // C_fstr's own operator= would construct a C_fstr of buf
        C_str::operator=(buf);
    }
};

// C_str's constructor of a char *, the game's first copy. Annotated here by name: a comment line above it in
// common/C_str.h counts for LS3DF's /YX and reorders IGraph.cpp's functions.
// TEMPLATE: GAME 0x403320
// ??0C_str@@QAE@PBD@Z
