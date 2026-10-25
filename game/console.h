#pragma once

#include "Main.h"

// Loads the console's font (PS2 console.cpp: a tail call of CreateFontTexture), which InitSystem does once
void ConsolesInit();
// Releases the font (PS2 console.cpp), at CloseSystem
void ConsolesDone();
