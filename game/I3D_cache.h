#pragma once

#include "Main.h"

#include <vector>

#include "ls3df.h"

class I3D_model;
class I3D_sound;
class I3D_animation_set;

// The caches of the models, sounds and animations the missions load (PS2 C_I3D_cache<T>, with C_I3D_model_cache,
// C_I3D_sound_cache and C_I3D_anim_cache derived from it; the header's name is ours), 0x28 bytes. Main.cpp has the
// three. Only what InitSystem and CloseSystem use so far.
// Open: the vtable. Like PS2's, the class and its base C_I3D_cache_base<T> are polymorphic (SetCacheSize, Open, Create,
// CreateNew, Clear, ClearUpper, the destructor, AddDir, ...; the anim cache's vtables are 0x625e24, 0x625df0 and the
// base's 0x625dc0), so Clear is virtual, and CloseSystem calls it on the globals, inlined.
template<class T>
class C_I3D_cache
{
public:
    // A loaded object and the name it was loaded by (PS2 C_I3D_cache<T>::S_itm; the members' names are ours)
    struct S_itm
    {
        T *obj;
        C_str name;
    };

    uchar unknown_0[4];                 // 0x0, the vtable
    std::vector<S_itm> items;           // 0x4
    std::vector<C_str> dirs;            // 0x14, the folders AddDir adds
    int cache_size;                     // 0x24, 0x100 (PS2 SetCacheSize)

    // Releases every object, the last loaded first (PS2's, whose names are char arrays it frees here too)
    void Clear()
    {
        for (int i = items.size(); i--;)
            items[i].obj->Release();
        items.clear();
    }

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

// The items' helpers, which CloseSystem's inlined Clear calls. The linker folded the three caches' copies of each; the
// original kept the first file's (Main.cpp's for the assignment).
// SYNTHETIC: GAME 0x5c2540
// C_I3D_cache<I3D_sound>::S_itm::operator=

// SYNTHETIC: GAME 0x410c30
// C_I3D_cache<I3D_animation_set>::S_itm::~S_itm

// TEMPLATE: GAME 0x409230
// std::vector<C_I3D_cache<I3D_animation_set>::S_itm,std::allocator<C_I3D_cache<I3D_animation_set>::S_itm> >::_Destroy

// TEMPLATE: GAME 0x409260
// ?copy@std@@YIPAUS_itm@?$C_I3D_cache@VI3D_animation_set@@@@PAU23@00@Z
