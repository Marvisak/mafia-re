#pragma once

#include "g_menu.h"

#include <vector>

// The car encyclopedia (PS2 GM_Carcyclopedia, g_m_carc.cpp), 0x84 bytes, the menu of the autosalon mission. Its
// constructor is the implicit one: the base's, then the vectors' (each copies a byte of an uninitialized allocator
// temporary, VC6's vector(const _A& = _A())). PS2's vectors hold car ids (G_CarDatabase::GetCarsByCarcyclopediaMask).
// VTABLE: GAME 0x625e88
class GM_Carcyclopedia : public GM_Menu
{
public:
    uchar unknown_30[0x30];
    std::vector<uint> unknown_60;       // 0x60
    std::vector<uint> unknown_70;       // 0x70
    uint unknown_80;                    // 0x80

    // FUNCTION: GAME 0x5bf510
    virtual const char *OnBeforeCreate()
    {
        // STRING: GAME 0x637b68
        return "carcyclopedia";
    }
    virtual int OnCreate();
    virtual int OnClose();
    virtual int OnKey(uchar key, uint);
    virtual int OnMouse(int x, int y, int z, uint, uint);
    virtual int OnClick(uint id);
    virtual int OnTick(uint time);
};
