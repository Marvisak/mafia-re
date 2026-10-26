#pragma once

#include "Main.h"

#include <vector>

// GM_Component::flags (the names are ours: by value where the meaning isn't known yet). GM_Menu::SetHidden and
// SetDisabled set their bits with true, as PS2's.
#define COMPF_VISIBLE 0x10              // drawn, with its children (BaseDraw); GM_Menu::SetHidden's
#define COMPF_ENABLED 0x20              // GM_Menu::SetDisabled's
#define COMPF_UNKNOWN_8000 0x8000       // with the two above, the component can be activated
#define COMPF_ACTIVE 0x80000000         // the menu's active component

// A menu's component (PS2 GM_Component, g_m_comps.cpp): its items (a button, a text, a slider, a progress bar, ...)
// with their children, created from the menu's definition. The PC's virtual functions are PS2's in its order with one
// of its own after Key, the mouse's (GM_Menu::Tick calls the three around it, GM_Menu's wrappers by id those from
// SetTextID on). The return types of those nothing calls yet are guesses. PS2's has the members in the same order, 4
// bytes lower (its vtable is at the end). Only what GM_Menu's written functions use so far; BaseDraw and BaseAfterMove
// are in g_menu.cpp, as PS2's.
class GM_Component
{
public:
    virtual ~GM_Component();
    virtual uint GetFlags();
    virtual uint GetType();
    virtual void GetClientOrigin(float &x, float &y);
    virtual bool CanBeParent();
    virtual void Draw(uint flags);
    virtual void AfterMove(uint flags);
    virtual void Tick(uint time);
    virtual bool IsOverClientArea(int x, int y);
    virtual bool IsOverActive(int x, int y);
    // The id of the item the key clicked, 0 for none
    virtual uint Key(uint shift_keys, uint key);
    // The mouse (PC only, the name is ours): the id of the item it clicked, 0 for none
    virtual uint Mouse(int x, int y, int z, uint buttons, uint time);
    virtual void ScrollToShow(GM_Component *component);
    virtual void ForceUpdate();
    virtual void SetTextID(uint id);
    virtual uint GetTextID();
    virtual void SetText(uchar *text);
    virtual void SetString(uchar *text);
    virtual uchar *GetText();
    virtual void SetFloat(float value);
    virtual float GetFloat();
    virtual void SetDword(uint value);
    virtual uint GetDword();
    virtual void SetBool(bool value);
    virtual bool GetBool();
    virtual void SetPointer(void *value);
    virtual void *GetPointer();
    virtual void SetRangeFloat(float min, float max);
    virtual void GetRangeFloat(float &min, float &max);
    virtual void SetRangeDword(uint min, uint max);
    virtual void GetRangeDword(uint &min, uint &max);

    // Draws the component and its children, if it is visible; of the flags only the high byte and the component's own
    // bits reach them
    void BaseDraw(uint draw_flags);
    // AfterMove of the component and its children
    void BaseAfterMove(uint draw_flags);

    uint id;                            // 0x4, the item's id in the menu's definition
    uint flags;                         // 0x8, COMPF_*
    uchar unknown_c[0x24];
    std::vector<GM_Component *> children; // 0x30
};
