#pragma once

#include "Main.h"

#include <vector>

// The key codes GM_MenuItem::Key tests, through IJoy's header, which sets DirectInput's version first
#include "ijoy.h"

#include "ui/g_indicators.h"
#include "ui/g_menu.h"

#include "IGraph/IGraph.h"

// GM_Component::flags (the names are ours: by value where the meaning isn't known yet). GM_Menu::SetHidden and
// SetDisabled set their bits with true, as PS2's.
#define COMPF_FONT 0xf                  // the font of the component's text (OutText's)
#define COMPF_VISIBLE 0x10              // drawn, with its children (BaseDraw); GM_Menu::SetHidden's
#define COMPF_ENABLED 0x20              // GM_Menu::SetDisabled's
#define COMPF_TEXT_RIGHT 0x40           // the text aligned to the right end (OUTTEXT_RIGHT)
#define COMPF_TEXT_CENTER 0x80          // the text centered (OUTTEXT_CENTER)
#define COMPF_ALTERNATE_COLOR 0x100     // PS2's GM_Menu::SetAlternateColor's
#define COMPF_ALTERNATE_STYLE 0x200     // PS2's GM_Menu::SetAlternateStyle's
#define COMPF_UNKNOWN_400 0x400         // a menu item's text red
#define COMPF_UNKNOWN_4000 0x4000       // the active component gets the mouse's clicks wherever the mouse is
#define COMPF_UNKNOWN_8000 0x8000       // with the two above, the component can be activated
#define COMPF_ACTIVE 0x80000000         // the menu's active component

// A window's title bar on the 800x600 screen, which its client area starts below (the name is ours)
#define WINDOW_TITLE_HEIGHT 40.0f

// A menu's component (PS2 GM_Component, g_m_comps.cpp): its items (a button, a text, a slider, a progress bar, ...)
// with their children, created from the menu's definition by their classes' factories (G_Factory, g_m_comps.cpp). The
// PC's virtual functions are PS2's in its order with one of its own after Key, the mouse's (GM_Menu::Tick calls the
// three around it, GM_Menu's wrappers by id those from SetTextID on). The PC defines them in the class, as the other
// components' classes: their copies come after the first factory function that needs the class's vtable (the
// defaults the classes share mostly folded with other files' empty functions). The constructors, SetTextID, BaseDraw,
// BaseAfterMove and the children's insertion are in g_menu.cpp, as PS2's, and FindComponentAt after them. PS2's has the
// members in the same order, 4 bytes lower (its vtable is at the end).
// VTABLE: GAME 0x625ff8
class GM_Component
{
public:
    // From the definition's item: the position and size scaled to the screen (GM_Menu::menuScreenScale), the text of the
    // item's id, and the parent's child if it has one
    GM_Component(GM_DesignStruct *design, GM_Component *parent);
    // A copy of the component with the next id, which is its parent's child after it
    GM_Component(GM_Component *component);
    virtual ~GM_Component() {}
    virtual uint GetFlags() { return 0; }
    virtual uint GetType() { return 'comp'; }
    // Where the children are placed from
    virtual void GetClientOrigin(float &x, float &y)
    {
        x = this->x;
        y = this->y;
    }
    virtual bool CanBeParent() { return false; }
    virtual void Draw(uint flags) {}
    // The screen position from the parent's client origin, or without a parent the position itself
    virtual void AfterMove(uint flags)
    {
        if (parent)
        {
            parent->GetClientOrigin(x, y);
            x += rel_x;
            y += rel_y;
        }
        else
        {
            x = rel_x;
            y = rel_y;
        }
    }
    virtual void Tick(uint time) {}
    // Whether the point is inside the component
    virtual bool IsOverClientArea(float x, float y)
    {
        x -= this->x;
        y -= this->y;
        if (x < 0.0f || x > sx || y < 0.0f || y > sy)
            return false;
        return true;
    }
    virtual bool IsOverActive(float x, float y)
    {
        x -= this->x;
        y -= this->y;
        if (x < 0.0f || x > sx || y < 0.0f || y > sy)
            return false;
        return true;
    }
    // The id of the item the key clicked, 0 for none
    virtual uint Key(uint shift_keys, uint key) { return 0; }
    // The mouse (PC only, the name is ours): the id of the item it clicked, 0 for none
    virtual uint Mouse(int x, int y, int z, uint buttons, uint time) { return 0; }
    virtual void ScrollToShow(GM_Component *component) {}
    virtual void ForceUpdate() {}
    // The text of the text database's id, none for 0
    virtual void SetTextID(uint id);
    virtual uint GetTextID() { return text_id; }
    virtual void SetText(uchar *text) { this->text = text; }
    virtual void SetString(uchar *text) {}
    virtual uchar *GetText() { return text; }
    virtual void SetFloat(float value) {}
    virtual float GetFloat() { return 0.0f; }
    virtual void SetDword(uint value) {}
    virtual uint GetDword() { return 0; }
    virtual void SetBool(bool value) {}
    virtual bool GetBool() { return false; }
    virtual void SetPointer(void *value) {}
    virtual void *GetPointer() { return NULL; }
    virtual void SetRangeFloat(float min, float max) {}
    virtual void GetRangeFloat(float &min, float &max) {}
    virtual void SetRangeDword(uint min, uint max) {}
    virtual void GetRangeDword(uint &min, uint &max) {}

    // Draws the component and its children, if it is visible; of the flags only the high byte and the component's own
    // bits reach them
    void BaseDraw(uint draw_flags);
    // AfterMove of the component and its children
    void BaseAfterMove(uint draw_flags);
    // The component under the point that can be activated, of the visible and enabled ones: the last child's first,
    // the deepest first, and NULL for none (PC only, the name is ours: the mouse's)
    GM_Component *FindComponentAt(float x, float y);
    // The component as the last child (PS2's name)
    void InsertComponent(GM_Component *component);
    // The component as the child after the other one, or the last if that isn't a child (PS2's name)
    void InsertComponentAfter(GM_Component *component, GM_Component *after);

    // The names are ours
    uint id;                            // 0x4, the item's id in the menu's definition
    uint flags;                         // 0x8, COMPF_*
    uint text_id;                       // 0xc
    uchar *text;                        // 0x10, the text database's text of text_id
    // 0x14, where the component is drawn, from its parent's client origin (AfterMove)
    float x, y;
    float sx, sy;                       // 0x1c
    // 0x24, the position in the parent (the root's on the screen): CreateCompsByDef moves the root's by the left edge
    // (levyOkraj), PS2's UtilCenterH and UtilCenterV center it in its parent
    float rel_x, rel_y;
    GM_Component *parent;               // 0x2c, NULL for the root
    std::vector<GM_Component *> children; // 0x30
};

// A window (PS2's), 'wind': the paper textures with a title bar, its text the title. Its children are placed below the
// bar.
// VTABLE: GAME 0x6260f8
class GM_Window : public GM_Component
{
public:
    GM_Window(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent) {}
    GM_Window(GM_Component *component) : GM_Component(component) {}

    virtual uint GetType() { return 'wind'; }
    virtual void GetClientOrigin(float &x, float &y)
    {
        x = this->x;
        y = GM_Menu::menuScreenScale.y * WINDOW_TITLE_HEIGHT + this->y;
    }
    virtual bool CanBeParent() { return true; }
    // In the alternate color the window is darker; the title bar is lit while the window is enabled in the active menu.
    // PS2's text is 0.7 of the bar high, 0.2 down, and narrowed to fit where it isn't centered.
    virtual void Draw(uint draw_flags)
    {
        SetViewport(0, 0, igraph->Scrn_sx(), igraph->Scrn_sy());
        float title_sy = GM_Menu::menuScreenScale.y * WINDOW_TITLE_HEIGHT;
        float text_sy = title_sy * .8;
        float text_dy = title_sy * .1;
        float x = this->x;
        float y = this->y;
        if (flags & COMPF_ALTERNATE_COLOR)
        {
            if (draw_flags & COMPF_ENABLED && draw_flags & MENUDRAW_UNKNOWN_40000000)
            {
                RectangleFilled(x, y + title_sy - 1.0f, sx, sy - title_sy, 0x7f000000, GM_Menu::textureMenuBottom);
                RectangleFilled(x, y, sx, title_sy, 0x7fffffff, GM_Menu::textureMenuTop);
                mafiaIndicators.OutText((char *)text, x, y + text_dy, sx, text_sy, 0xffffffff,
                    flags & COMPF_TEXT_CENTER ? OUTTEXT_CENTER : OUTTEXT_INDENT, 2, NULL);
            }
            else
            {
                RectangleFilled(x, y + title_sy - 1.0f, sx, sy - title_sy, 0x7f7f7f7f, GM_Menu::textureMenuBottom);
                RectangleFilled(x, y, sx, title_sy, 0x7f7f7f7f, GM_Menu::textureMenuTop);
                mafiaIndicators.OutText((char *)text, x, y + text_dy, sx, text_sy, 0xff7f7f7f,
                    flags & COMPF_TEXT_CENTER ? OUTTEXT_CENTER : OUTTEXT_INDENT, 2, NULL);
            }
        }
        else
        {
            if (draw_flags & COMPF_ENABLED && draw_flags & MENUDRAW_UNKNOWN_40000000)
            {
                RectangleFilled(x, y + title_sy - 1.0f, sx, sy - title_sy, 0xffffffff, GM_Menu::textureMenuBottom);
                RectangleFilled(x, y, sx, title_sy, 0xffffffff, GM_Menu::textureMenuTop);
                mafiaIndicators.OutText((char *)text, x, y + text_dy, sx, text_sy, 0xffffffff,
                    flags & COMPF_TEXT_CENTER ? OUTTEXT_CENTER : OUTTEXT_INDENT, 2, NULL);
            }
            else
            {
                RectangleFilled(x, y + title_sy - 1.0f, sx, sy - title_sy, 0xff7f7f7f, GM_Menu::textureMenuBottom);
                RectangleFilled(x, y, sx, title_sy, 0xff7f7f7f, GM_Menu::textureMenuTop);
                mafiaIndicators.OutText((char *)text, x, y + text_dy, sx, text_sy, 0xff7f7f7f,
                    flags & COMPF_TEXT_CENTER ? OUTTEXT_CENTER : OUTTEXT_INDENT, 2, NULL);
            }
        }
    }
};

// A rectangle (PS2's), 'rect': in the alternate style filled, white or black, otherwise a black outline, or filled half
// transparent in the alternate color
// VTABLE: GAME 0x626500
class GM_Rectangle : public GM_Component
{
public:
    GM_Rectangle(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent) {}
    GM_Rectangle(GM_Component *component) : GM_Component(component) {}

    virtual uint GetType() { return 'rect'; }
    virtual void Draw(uint draw_flags)
    {
        if (flags & COMPF_ALTERNATE_STYLE)
        {
            if (flags & COMPF_ALTERNATE_COLOR)
                RectangleFilled(x, y, sx, sy, 0xffffffff, NULL);
            else
                RectangleFilled(x, y, sx, sy, 0xff000000, NULL);
        }
        else
        {
            if (flags & COMPF_ALTERNATE_COLOR)
                RectangleFilled(x, y, sx, sy, 0x7f000000, NULL);
            else
                Rectangle(x, y, sx, sy, 0xff000000);
        }
    }
};

// A menu's item (PS2's), 'item': its text, which the mouse's left button or Space, Enter and the keypad's Enter click.
// It can be activated.
// VTABLE: GAME 0x626700
class GM_MenuItem : public GM_Component
{
public:
    GM_MenuItem(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        flags |= COMPF_UNKNOWN_8000;
    }
    GM_MenuItem(GM_Component *component) : GM_Component(component) {}

    virtual uint GetType() { return 'item'; }
    // Underlined while active, black (or red) while enabled, grey otherwise
    virtual void Draw(uint draw_flags)
    {
        uint text_flags = 0;
        if (flags & COMPF_ACTIVE)
            text_flags |= OUTTEXT_UNDERLINE;
        if (flags & COMPF_TEXT_CENTER)
            text_flags |= OUTTEXT_CENTER;
        if (flags & COMPF_TEXT_RIGHT)
            text_flags |= OUTTEXT_RIGHT;
        if (draw_flags & COMPF_ENABLED)
        {
            if (flags & COMPF_UNKNOWN_400)
                mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xffff0000, text_flags, flags & COMPF_FONT, NULL);
            else
                mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xff000000, text_flags, flags & COMPF_FONT, NULL);
        }
        else
            mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xff7f7f7f, text_flags, flags & COMPF_FONT, NULL);
    }
    // PS2's has Enter and the keypad's Enter
    virtual uint Key(uint shift_keys, uint key)
    {
        if (key == DIK_SPACE)
            return id;
        if (key == DIK_RETURN)
            return id;
        if (key == DIK_NUMPADENTER)
            return id;
        return 0;
    }
    virtual uint Mouse(int x, int y, int z, uint buttons, uint time)
    {
        if (buttons & MENUMOUSE_LEFT_RELEASED)
            return id;
        return 0;
    }
};

// The copies of the classes' functions, by name: the original's come in the order the factories first need them
// (docs/game-files.md), so not in the order of the classes above. Those not listed folded with other files' functions
// of the same code (the empty ones, GetTextID's and GetText's accessors). The destructors and scalar deleting
// destructors of the classes are one each, and GetBool is CanBeParent's code.
// FUNCTION: GAME 0x5d9220
// ?AfterMove@GM_Component@@UAEXI@Z

// FUNCTION: GAME 0x5d9260
// ?IsOverActive@GM_Component@@UAE_NMM@Z

// FUNCTION: GAME 0x5d92c0
// ?SetText@GM_Component@@UAEXPAE@Z

// FUNCTION: GAME 0x5d92e0
// ?GetFloat@GM_Component@@UAEMXZ

// FUNCTION: GAME 0x5d9330
// ?GetType@GM_Component@@UAEIXZ

// FUNCTION: GAME 0x5d9340
// ?GetClientOrigin@GM_Component@@UAEXAAM0@Z

// FUNCTION: GAME 0x5d9360
// ?GetBool@GM_Component@@UAE_NXZ

// FUNCTION: GAME 0x5d9530
// ?GetType@GM_Window@@UAEIXZ

// FUNCTION: GAME 0x5d9540
// ?GetClientOrigin@GM_Window@@UAEXAAM0@Z

// FUNCTION: GAME 0x5d9570
// ?Draw@GM_Window@@UAEXI@Z

// The original kept another class's copy
// FUNCTION: GAME 0x5d9910
// ?CanBeParent@GM_Window@@UAE_NXZ

// FUNCTION: GAME 0x5d9f60
// ??1GM_Rectangle@@UAE@XZ

// FUNCTION: GAME 0x5da7e0
// ?GetType@GM_Rectangle@@UAEIXZ

// FUNCTION: GAME 0x5da7f0
// ?Draw@GM_Rectangle@@UAEXI@Z

// FUNCTION: GAME 0x5dadb0
// ?GetType@GM_MenuItem@@UAEIXZ

// FUNCTION: GAME 0x5dadc0
// ?Draw@GM_MenuItem@@UAEXI@Z

// FUNCTION: GAME 0x5dae60
// ?Mouse@GM_MenuItem@@UAEIHHHII@Z

// The original kept another class's copy
// FUNCTION: GAME 0x5daf60
// ?Key@GM_MenuItem@@UAEIII@Z

// In the last file of components (g_m_compsm.cpp)
// SYNTHETIC: GAME 0x5e0580
// ??_GGM_Window@@UAEPAXI@Z

// The factories' functions
// TEMPLATE: GAME 0x5d90f0
// ?Make@?$G_Factory@VGM_Component@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5d9160
// ?MakeDup@?$G_Factory@VGM_Component@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5d94d0
// ?Make@?$G_Factory@VGM_Window@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5d9840
// ?MakeDup@?$G_Factory@VGM_Window@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5da780
// ?Make@?$G_Factory@VGM_Rectangle@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5da870
// ?MakeDup@?$G_Factory@VGM_Rectangle@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5dad40
// ?Make@?$G_Factory@VGM_MenuItem@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5dae80
// ?MakeDup@?$G_Factory@VGM_MenuItem@@@@SIPAVGM_Component@@PAV2@@Z
