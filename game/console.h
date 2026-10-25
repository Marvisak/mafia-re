#pragma once

#include "Main.h"

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
