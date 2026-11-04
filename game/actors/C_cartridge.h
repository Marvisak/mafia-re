#pragma once

#include "actors/C_bottle.h"

// The cartridge cases the guns throw out (PS2 C_cartridge, C_cartridge.cpp, whose code the PS2 build doesn't have),
// ACTOR_CARTRIDGE, 0x13c bytes: a bottle's physics. C_game::Init creates the game's from 2nabojnice.i3d and
// 2nabojniceb.i3d ("nabojnice" is Czech for a cartridge case; the second ones are the shotguns' shells, which
// C_game::NewCartidge throws out for a close-range weapon).
// VTABLE: GAME 0x623880
class C_cartridge : public C_bottle
{
public:
    C_cartridge();
    virtual ~C_cartridge();
    // The vtable's other overrides (the slots' names are C_actor's), but the linker's folded constant functions and
    // those that only call the base's
    virtual void AI(uint time);
    virtual void Update(uint time);
    // The base's: the original's jumps there, a copy the linker folded
    // FUNCTION: GAME 0x443860 FOLDED
    virtual void NetDirect(uint time)
    {
        C_bottle::NetDirect(time);
    }
    virtual bool Init(I3D_frame *frm);
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf)
    {
        return true;
    }
    virtual bool SaveGameLoad(void **buf)
    {
        return true;
    }
    virtual void GameInit();
    virtual void GameDone();
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm)
    {
        return false;
    }
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm)
    {
        return false;
    }

    // C_game::NewCartidge's, for the case it throws out (the name is ours)
    void SetShotgunShell(bool on);

    // 0x138, a shotgun's shell (cartridges_b's), which sounds as one and bounces less (the name is ours)
    bool shotgun_shell;
    uchar unknown_139[3];               // 0x139

    // The cases' sounds, which C_game::Init creates and DoneIntern releases (PS2 name)
    static I3D_sound *sounds[3];
};

// SYNTHETIC: GAME 0x430800
// C_cartridge::`scalar deleting destructor'
