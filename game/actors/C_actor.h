#pragma once

#include "Main.h"

// An animation's file and its id (the struct's name is ours)
struct S_anim_file
{
    const char *name;
    uint id;
};

// The actors' animations by id (PS2 name), not in the ids' order, ended by an id of 0xffff
extern S_anim_file _anim_info_[];

// An id and its animation's place in _anim_info_ (the struct's name is ours)
struct S_anim_index
{
    uint id;
    int index;
};

// _anim_info_'s ids in ascending order (PS2 names), which InitAnimIndex builds
extern S_anim_index *_anim_index_;
extern int _anim_index_count_;

// Builds the animations' index and sorts it (PS2 C_actor.cpp), once, for InitSystem
void InitAnimIndex();
// Frees the index, at CloseSystem
void DoneAnimIndex();
