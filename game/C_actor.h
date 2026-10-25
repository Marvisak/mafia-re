#pragma once

#include "Main.h"

// Sorts the animations' index (PS2 C_actor.cpp), which InitSystem builds once
void InitAnimIndex();
// Frees the index, at CloseSystem
void DoneAnimIndex();
