#pragma once

#include "Main.h"

class I3D_model;
class I3D_sound;
class I3D_animation_set;

// The caches of the models, sounds and animations the missions load (PS2 C_I3D_cache<T>, with C_I3D_model_cache,
// C_I3D_sound_cache and C_I3D_anim_cache derived from it; the header's name is ours). Main.cpp has the three. Only what
// InitSystem uses so far.
template<class T>
class C_I3D_cache
{
public:
    // Adds a folder the cache finds its files in
    void AddDir(const char *dir);
};

class C_I3D_model_cache : public C_I3D_cache<I3D_model>
{
};

class C_I3D_sound_cache : public C_I3D_cache<I3D_sound>
{
};

class C_I3D_anim_cache : public C_I3D_cache<I3D_animation_set>
{
};

extern C_I3D_model_cache model_cache;
extern C_I3D_sound_cache sound_cache;
extern C_I3D_anim_cache anim_cache;
