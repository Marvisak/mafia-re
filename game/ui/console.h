#pragma once

#include "Main.h"

#include <vector>

class ITexture;

// A character of the console's font, 0x14 bytes of fnt_matrix.bin (names ours, from the text drawing at 0x4f6040,
// which takes each quad's texture coordinates from the rectangle)
struct S_font_char {
    float left, top, right, bottom;     // its rectangle in editfont.bmp, in texture coordinates
    float width;                        // its advance, in units of the text's height
};

// Creates the font's texture from editfont.bmp, its alpha from the same file, and reads the characters' rectangles
// from fnt_matrix.bin (PS2 console.cpp)
void CreateFontTexture(ITexture **texture);
// Loads the console's font (PS2 console.cpp), which InitSystem does once
void ConsolesInit();
// Releases the font (PS2 console.cpp), at CloseSystem
void ConsolesDone();

// A line of a console (PS2's name, the same 0x1c bytes; the members' names are ours, after PS2's C_console::OutText,
// which stores its arguments there)
struct S_console_line
{
    char *text;                         // 0x0, a copy, new[]
    int r, g, b;                        // 0x4
    int id;                             // 0x10, OutText's: a line with the last line's id replaces it (-1 for none)
    float unknown_14;                   // 0x14, -1 from PS2's OutText
    float unknown_18;                   // 0x18
};

// A console of lines on the screen (PS2 C_console), 0x3c bytes; C_game has two. The members' names are ours, after what
// C_game::Init sets.
class C_console
{
public:
    std::vector<S_console_line> text_lines; // 0x0
    int r, g, b;                        // 0x10, the text's color
    float x, y;                         // 0x1c, the position, relative to the screen
    float sx, sy;                       // 0x24, the characters' size
    int lines;                          // 0x2c, how many lines it shows
    int time;                           // 0x30, how long a line stays, in ms
    // The time the tick (0x4f5f50) has counted towards removing the first line, faster the more lines there are
    int unknown_34;                     // 0x34
    bool unknown_38;                    // 0x38
    bool unknown_39;                    // 0x39
    bool unknown_3a;                    // 0x3a

    // White lines of the tenth of the screen's size at its corner, 100 of them for a second each (PS2's the same)
    C_console();
    ~C_console();
    // Forgets the lines (PS2 name)
    void Clear();
};

// A console in a slot (PS2's name; the members' names are ours), 0xc bytes: ConsoleReg counts the registrations
struct S_con_register
{
    C_console *console;
    uint slot;                          // 0x4
    int count;                          // 0x8
};

// Registers a console in a slot of _consoles, and takes it out (PS2 names): a console registered twice needs two
// ConsoleUnRegs, the last one clears it
void ConsoleReg(C_console *console, uint slot);
void ConsoleUnReg(C_console *console);
