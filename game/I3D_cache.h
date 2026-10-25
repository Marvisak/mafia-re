#pragma once

#include "Main.h"
#include "C_fstr.h"
#include "stub.h"

#include <vector>

#include "I3D/I3D.h"

// What the caches report a file they couldn't load or open with: the message, the caller's context (PS2's parameters;
// __fastcall under /Gr). The name is ours.
typedef void I3D_CACHE_CB_PROC(const char *msg, void *context);

// The caches' interface (PS2 C_I3D_cache_base<T>): the vtable of C_I3D_cache<T> but Duplicate, every function pure but
// CreateNew and the destructor. The anim cache's is 0x625dc0, 12 slots.
template<class T>
class C_I3D_cache_base
{
public:
    virtual void SetCacheSize(int size) = 0;
    // Loads a file into the caller's object, which the cache keeps a copy of
    virtual LS3D_RESULT Open(T *obj, const char *name, uint flags, I3D_CACHE_CB_PROC *cb, void *context,
        void *reserved) = 0;
    // The cached object of a name or a new one, loaded from the first folder that has the file
    virtual LS3D_RESULT Create(const char *name, T **obj, I3D_CACHE_CB_PROC *cb, void *context) = 0;
    // A new object for Create (the derived caches ask the engine for one)
    virtual T *CreateNew() const
    {
        return NULL;
    }
    virtual void Clear() = 0;
    virtual ~C_I3D_cache_base() {}
    virtual void AddDir(const char *dir) = 0;
    virtual void DelDir(const char *dir) = 0;
    virtual void ClearDirs() = 0;
    virtual int NumDirs() const = 0;
    virtual const char *GetDir(int index) const = 0;
    // A full path made relative to one of the folders, in place: false if it is in none. PC only (PS2 has ClearUpper
    // in its place), the name is ours.
    virtual bool MakeRelative(char *name) = 0;
};

// The caches of the models, sounds and animations the missions load (PS2 C_I3D_cache<T>, with C_I3D_model_cache,
// C_I3D_sound_cache and C_I3D_anim_cache derived from it; the header's name is ours), 0x28 bytes. Main.cpp has the
// three. Written: Clear and AddDir; the other functions of the vtable are stubs.
// Open: the original's destructor (0x5c1fd0 for the model cache) releases the items, Clear inlined; ours is implicit.
// The constructor sets cache_size to 0x100.
template<class T>
class C_I3D_cache : public C_I3D_cache_base<T>
{
public:
    // A loaded object and the name it was loaded by (PS2 C_I3D_cache<T>::S_itm; the members' names are ours)
    struct S_itm
    {
        T *obj;
        C_str name;
    };

    std::vector<S_itm> items;           // 0x4
    std::vector<C_str> dirs;            // 0x14, the folders AddDir adds, each ending in a backslash
    int cache_size;                     // 0x24, 0x100 (PS2 SetCacheSize)

    virtual void SetCacheSize(int size);
    virtual LS3D_RESULT Open(T *obj, const char *name, uint flags, I3D_CACHE_CB_PROC *cb, void *context,
        void *reserved);
    virtual LS3D_RESULT Create(const char *name, T **obj, I3D_CACHE_CB_PROC *cb, void *context);

    // Releases every object, the last loaded first (PS2's, whose names are char arrays it frees here too)
    virtual void Clear()
    {
        for (int i = items.size(); i--;)
            items[i].obj->Release();
        items.clear();
    }

    virtual void AddDir(const char *dir);
    virtual void DelDir(const char *dir);
    virtual void ClearDirs();
    virtual int NumDirs() const;
    virtual const char *GetDir(int index) const;
    virtual bool MakeRelative(char *name);
    // Makes the first object a copy of the second: Open keeps a copy of the object it loaded
    virtual void Duplicate(T *dst, T *src) = 0;
};

// Adds a folder the cache finds its files in, ending it in a backslash, after removing it if it is there already. The
// name goes through C_fstr, so it is formatted (PS2 copies it with strcpy). The original's three copies are at the end of
// Winstuff.cpp's object, though InitSystem is their only caller (Winstuff.cpp).
// Open: the model cache's copy, the first, adds the inlined operator+='s lengths as [esi + edi + 9], ours as [edi + esi
// + 9]; the other two are the original's. uint len = rep->Size() in operator+= (ls3df.h) gives all three, but moves
// LS3DF (I3d_driver.cpp's deferred $E2, IGraph.cpp) and lowers IGraph::CreateITexture.
template<class T>
void C_I3D_cache<T>::AddDir(const char *dir)
{
    DelDir(dir);
    C_fstr d(dir);
    if (d[(int)d.Size() - 1] != '\\')
        // STRING: GAME 0x636684
        d += "\\";
    dirs.push_back(d);
}

// Removes a folder, the last one added of that name; the name is formatted and ended as AddDir does.
// Open: the original's inlined erase destroys the string through C_str's scalar deleting destructor (0x5c24f0, with 0),
// ours inlines it: the original had one more inline call after the erase (a probe call after it or after the loop gives
// the original's), not found. The model cache's copy also has AddDir's operator+= register order.
template<class T>
void C_I3D_cache<T>::DelDir(const char *dir)
{
    C_fstr d(dir);
    if (d[(int)d.Size() - 1] != '\\')
        d += "\\";
    for (int i = dirs.size(); i--;) {
        if (dirs[i] == d) {
            dirs.erase(dirs.begin() + i);
            break;
        }
    }
}

// The stubs: the linker folds the three caches' copies of each

template<class T>
void C_I3D_cache<T>::SetCacheSize(int size)
{
    StubLog("C_I3D_cache::SetCacheSize %d", size);
}

template<class T>
LS3D_RESULT C_I3D_cache<T>::Open(T *obj, const char *name, uint flags, I3D_CACHE_CB_PROC *cb, void *context,
    void *reserved)
{
    StubLog("C_I3D_cache::Open %s", name);
    return I3DERR_NOFILE;
}

template<class T>
LS3D_RESULT C_I3D_cache<T>::Create(const char *name, T **obj, I3D_CACHE_CB_PROC *cb, void *context)
{
    StubLog("C_I3D_cache::Create %s", name);
    *obj = NULL;
    return I3DERR_NOFILE;
}

template<class T>
void C_I3D_cache<T>::ClearDirs()
{
    StubLog("C_I3D_cache::ClearDirs");
}

template<class T>
int C_I3D_cache<T>::NumDirs() const
{
    StubLog("C_I3D_cache::NumDirs");
    return 0;
}

template<class T>
const char *C_I3D_cache<T>::GetDir(int index) const
{
    StubLog("C_I3D_cache::GetDir %d", index);
    return NULL;
}

template<class T>
bool C_I3D_cache<T>::MakeRelative(char *name)
{
    StubLog("C_I3D_cache::MakeRelative %s", name);
    return false;
}

// VTABLE: GAME 0x625cf4
class C_I3D_model_cache : public C_I3D_cache<I3D_model>
{
public:
    // STUB: GAME 0x5c23e0
    virtual I3D_model *CreateNew() const
    {
        StubLog("C_I3D_model_cache::CreateNew");
        return NULL;
    }

    // STUB: GAME 0x5c23a0
    virtual void Duplicate(I3D_model *dst, I3D_model *src)
    {
        StubLog("C_I3D_model_cache::Duplicate");
    }
};

// VTABLE: GAME 0x625d8c
class C_I3D_sound_cache : public C_I3D_cache<I3D_sound>
{
public:
    // STUB: GAME 0x5c2430
    virtual I3D_sound *CreateNew() const
    {
        StubLog("C_I3D_sound_cache::CreateNew");
        return NULL;
    }

    // STUB: GAME 0x5c23f0
    virtual void Duplicate(I3D_sound *dst, I3D_sound *src)
    {
        StubLog("C_I3D_sound_cache::Duplicate");
    }
};

// VTABLE: GAME 0x625e24
class C_I3D_anim_cache : public C_I3D_cache<I3D_animation_set>
{
public:
    // STUB: GAME 0x5c2440
    virtual I3D_animation_set *CreateNew() const
    {
        StubLog("C_I3D_anim_cache::CreateNew");
        return NULL;
    }

    // Nothing to copy: the original's is an empty ret 8, folded with the other empty functions of its kind (0x4d2040)
    virtual void Duplicate(I3D_animation_set *dst, I3D_animation_set *src) {}
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

// The items' erase, which C_mission::UnInit's inlined Clear calls out of line (CloseSystem's inlines it); the original
// kept Mission.cpp's copy.
// Open: its inlined copy loop walks the destination and reads the source's name relative to it, ours walks the
// source's name. The out-of-line copy, assignment and destructor match, and an explicit assignment, copy constructor
// or destructor of S_itm, or erase called directly, compile alike.
// TEMPLATE: GAME 0x548330
// std::vector<C_I3D_cache<I3D_animation_set>::S_itm,std::allocator<C_I3D_cache<I3D_animation_set>::S_itm> >::erase

// The folders' insert, which AddDir's inlined push_back calls, and its helper
// TEMPLATE: GAME 0x566c60
// ?insert@?$vector@VC_str@@V?$allocator@VC_str@@@std@@@std@@QAEXPAVC_str@@IABV3@@Z

// TEMPLATE: GAME 0x566f00
// ?_Construct@std@@YIXPAVC_str@@ABV2@@Z

// TEMPLATE: GAME 0x5665a0
// C_I3D_cache<I3D_model>::AddDir

// TEMPLATE: GAME 0x5667e0
// C_I3D_cache<I3D_sound>::AddDir

// TEMPLATE: GAME 0x566990
// C_I3D_cache<I3D_animation_set>::AddDir

// TEMPLATE: GAME 0x5c06a0
// C_I3D_cache<I3D_model>::DelDir

// TEMPLATE: GAME 0x5c10e0
// C_I3D_cache<I3D_sound>::DelDir

// TEMPLATE: GAME 0x5c1aa0
// C_I3D_cache<I3D_animation_set>::DelDir
