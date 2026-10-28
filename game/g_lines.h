#pragma once

#include "Main.h"

struct S_vector;

// The lines drawn over each frame (PC only, the names are ours): world-space lines collected during the frame, which the
// scene's callback draws after the scene and then forgets. Only the explosions' debris adds any.

// The most lines a frame takes; AddLine ignores the rest
#define MAX_LINES 10000

// A line from one point to another, with a color at each end; color_to 0 takes color
void AddLine(const S_vector &from, const S_vector &to, uint color, uint color_to);
// Draws the frame's lines untextured, in world space
void DrawLines();
// Forgets the frame's lines
void ClearLines();

// g_lines.cpp's copy of the engine header's identity matrix, which DrawLines passes, one of the original's five. Ours are
// in every game file (the engine's headers give each one, docs/game-files.md), so reccmp pairs the name with our first
// copy, not g_lines.cpp's: the push reads equal by name all the same.
// GLOBAL: GAME 0x625f30
// IdentityMatrixData
