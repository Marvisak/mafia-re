#pragma once

#include "Main.h"

#include <vector>

// The key codes GM_MenuItem::Key tests, through IJoy's header, which sets DirectInput's version first
#include "ijoy.h"

#include "ui/g_indicators.h"
#include "ui/g_menu.h"
#include "ui/g_textdb.h"

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
#define COMPF_TEXT_RED 0x400            // a menu item's text red; a player row's ready mark (GM_NetPlayer)
// A player row's (GM_NetPlayer; the names are guesses): the X box at its right end, the AI mark where it has no
// ready mark, and the X box sunk while the row is active
#define COMPF_X_BOX 0x800
#define COMPF_AI 0x1000
#define COMPF_MOUSE_CAPTURE 0x4000      // the active component gets the mouse's clicks wherever the mouse is
#define COMPF_ACTIVATABLE 0x8000        // with COMPF_VISIBLE and COMPF_ENABLED, the component can be activated
#define COMPF_PRESSED 0x10000000        // (the name is a guess, as above)
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

// A stand-in (PS2's GM_Nahrada, Czech "nahrada", a substitute), 'xxxx': a green outline with its text in it, a parent
// whose children are placed from its parent's client origin
// VTABLE: GAME 0x625f78
class GM_Nahrada : public GM_Component
{
public:
    GM_Nahrada(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent) {}
    GM_Nahrada(GM_Component *component) : GM_Component(component) {}

    virtual uint GetType() { return 'xxxx'; }
    virtual void GetClientOrigin(float &x, float &y)
    {
        x = this->x;
        y = this->y;
        if (parent)
            parent->GetClientOrigin(x, y);
    }
    virtual bool CanBeParent() { return true; }
    // The original kept the copy of 'vidc' (g_m_comps2.cpp), the same code
    virtual void Draw(uint draw_flags)
    {
        Rectangle(x, y, sx, sy, 0x7f00ff7f);
        mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xff00ff7f, 0, 0, NULL);
    }
};

// The menu's whole screen (PS2's GM_UpUp), 'upup': its Draw sets the viewport to the screen, as a window's does first.
// CreateCompsByDef handles the type itself and never asks the factory.
// VTABLE: GAME 0x626078
class GM_UpUp : public GM_Component
{
public:
    GM_UpUp(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent) {}
    GM_UpUp(GM_Component *component) : GM_Component(component) {}

    virtual uint GetType() { return 'upup'; }
    virtual void Draw(uint draw_flags)
    {
        SetViewport(0, 0, igraph->Scrn_sx(), igraph->Scrn_sy());
    }
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
            if (draw_flags & COMPF_ENABLED && draw_flags & MENUDRAW_ACTIVE_MENU)
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
            if (draw_flags & COMPF_ENABLED && draw_flags & MENUDRAW_ACTIVE_MENU)
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

// A window without anything drawn (PS2's), 'winc': only a parent, whose children are placed from its own position
// VTABLE: GAME 0x626180
class GM_WindowClean : public GM_Component
{
public:
    GM_WindowClean(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent) {}
    GM_WindowClean(GM_Component *component) : GM_Component(component) {}

    virtual uint GetType() { return 'winc'; }
    virtual bool CanBeParent() { return true; }
    // PS2's is its own empty function, the PC's folded with the base's
    virtual void Draw(uint draw_flags) {}
};

// A scrolled window's bar on the 800x600 screen, along its right edge (the name is ours)
#define SCROLL_BAR_WIDTH 20.0f

// A scrolled window (PS2's), 'wins': only a parent, whose children are placed from its position less the scroll, with
// a scroll bar along its right edge: black outlines, a grey thumb and two arrows, red while it is active. Its value is
// the scroll, 0 to its range's maximum (100 at first; the minimum is always 0). The keys' Up and Down move it by the
// bar's width, the mouse's left button on the arrows by a pixel a millisecond and between them to where the mouse is;
// the scroll drawn follows the value by a pixel a millisecond (Tick), which also scrolls to show the active child. Its
// Draw sets the viewport inside the window left of the bar, for the children. It can be activated and captures the
// mouse. PS2's can't be activated and has no mouse, scrolls to 2 above an active child, sets the scroll drawn to 0 at
// first and draws the bar only for a range above 5.
// VTABLE: GAME 0x626200
class GM_WindowScroll : public GM_Component
{
public:
    GM_WindowScroll(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        flags |= COMPF_ACTIVATABLE | COMPF_MOUSE_CAPTURE;
        scroll = 0.0f;
        scroll_max = 100.0f;
        bar_width = GM_Menu::menuScreenScale.x * SCROLL_BAR_WIDTH;
    }
    GM_WindowScroll(GM_Component *component) : GM_Component(component)
    {
        scroll = 0.0f;
        scroll_max = 100.0f;
        bar_width = GM_Menu::menuScreenScale.x * SCROLL_BAR_WIDTH;
    }

    virtual uint GetType() { return 'wins'; }
    // The original kept GM_WindowClean's copy
    virtual bool CanBeParent() { return true; }
    virtual void GetClientOrigin(float &x, float &y)
    {
        x = this->x;
        y = this->y - scroll_shown;
    }
    // Over the bar
    virtual bool IsOverActive(float x, float y)
    {
        x -= this->x + sx - bar_width;
        y -= this->y;
        if (x < 0.0f || x > bar_width || y < 0.0f || y > sy)
            return false;
        return true;
    }
    // The scroll, in the range; the scroll drawn follows it
    virtual void SetFloat(float value)
    {
        scroll = value;
        if (scroll < 0.0f)
            scroll = 0.0f;
        if (scroll > scroll_max)
            scroll = scroll_max;
    }
    // The range's maximum, not below 0, and the scroll in it, drawn there at once; the minimum is always 0
    virtual void SetRangeFloat(float min, float max)
    {
        scroll_max = max;
        if (scroll_max < 0.0f)
            scroll_max = 0.0f;
        if (scroll < 0.0f)
            scroll = 0.0f;
        if (scroll > scroll_max)
            scroll = scroll_max;
        scroll_shown = scroll;
    }
    virtual float GetFloat() { return scroll; }
    virtual void GetRangeFloat(float &min, float &max)
    {
        min = 0.0f;
        max = scroll_max;
    }
    // The bar's outline, the arrows' squares at its ends and the thumb below the upper one, at the scroll drawn's share
    // of the range (at the top for a range up to 5)
    virtual void Draw(uint draw_flags)
    {
        float thumb_y;
        if (scroll_max > 5.0f)
            thumb_y = scroll_shown / scroll_max * (sy - bar_width * 3.0f);
        else
            thumb_y = 0.0f;
        uint color = flags & COMPF_ACTIVE ? 0xffff0000 : 0xff7f7f7f;
        RectangleFilled(x + sx - bar_width, y + thumb_y + bar_width, bar_width, bar_width, 0xff7f7f7f, NULL);
        Rectangle(x + sx - bar_width, y, bar_width, sy, 0xff000000);
        Rectangle(x + sx - bar_width, y + bar_width, bar_width, sy - 2.0f * bar_width, 0xff000000);
        Triangle(x + sx - bar_width * .75f, y + bar_width * .25f, bar_width * .5f, bar_width * .5f, TRIANGLE_UP, color);
        Triangle(x + sx - bar_width * .75f, y + sy - bar_width * .75f, bar_width * .5f, bar_width * .5f, TRIANGLE_DOWN,
            color);
        SetViewport((int)(x + 1.0f), (int)(y + 1.0f), (int)(sx - bar_width - 2.0f), (int)(sy - 2.0f));
    }
    // The scroll drawn moves to the value by a pixel a millisecond. Then the value shows the first child that is active
    // (activatable, visible and enabled): its top where it is above the window, its bottom where it is below.
    virtual void Tick(uint time)
    {
        float step = (float)time;
        if (scroll > scroll_shown)
        {
            scroll_shown += step;
            if (scroll_shown > scroll)
                scroll_shown = scroll;
        }
        else
        {
            scroll_shown -= step;
            if (scroll_shown < scroll)
                scroll_shown = scroll;
        }
        GM_Component *active = NULL;
        for (std::vector<GM_Component *>::iterator it = children.begin(); it < children.end(); it++)
            if ((*it)->flags & COMPF_ACTIVE && (*it)->flags & COMPF_ACTIVATABLE && (*it)->flags & COMPF_VISIBLE &&
                (*it)->flags & COMPF_ENABLED)
            {
                active = *it;
                break;
            }
        if (active)
        {
            if (active->rel_y < scroll)
                SetFloat(active->rel_y);
            if (active->rel_y + active->sy > sy + scroll)
                SetFloat(active->rel_y + active->sy - sy);
        }
    }
    // While the left button is down (wherever the mouse is, captured): on the upper arrow the scroll goes up by a pixel a
    // millisecond, on the lower one down, and between them the thumb's middle goes to the mouse; drawn there at once.
    // Never clicks.
    virtual uint Mouse(int x, int y, int z, uint buttons, uint time)
    {
        if (buttons & MENUMOUSE_LEFT)
        {
            float mouse_y = y - this->y;
            if (mouse_y < bar_width)
            {
                scroll -= time;
                if (scroll < 0.0f)
                    scroll = 0.0f;
                scroll_shown = scroll;
            }
            else if (mouse_y > sy - bar_width)
            {
                scroll += time;
                if (scroll > scroll_max)
                    scroll = scroll_max;
                scroll_shown = scroll;
            }
            else
            {
                scroll = (mouse_y - bar_width * 1.5f) / (sy - bar_width * 3.0f) * scroll_max;
                if (scroll < 0.0f)
                    scroll = 0.0f;
                if (scroll > scroll_max)
                    scroll = scroll_max;
                scroll_shown = scroll;
            }
        }
        return 0;
    }
    // Up and Down move the scroll by the bar's width, drawn there at once. Never clicks.
    virtual uint Key(uint shift_keys, uint key)
    {
        if (key == DIK_UP)
            scroll -= bar_width;
        if (key == DIK_DOWN)
            scroll += bar_width;
        if (scroll < 0.0f)
            scroll = 0.0f;
        if (scroll > scroll_max)
            scroll = scroll_max;
        scroll_shown = scroll;
        return 0;
    }

    // The names are ours
    float scroll_max;                   // 0x40, the range's maximum (not max, windows.h's macro)
    float scroll;                       // 0x44, the value
    // 0x48, the scroll the children are placed with, which follows the value (Tick); the PC's constructors leave it
    // unset until SetRangeFloat, Key or Mouse sets it
    float scroll_shown;
    float bar_width;                    // 0x4c, SCROLL_BAR_WIDTH on the screen
};

// A text (PS2's), 'stxt', over as many lines as it takes: in the alternate style black, or yellow in the normal color;
// otherwise white, or grey in the normal color (PS2's darker)
// VTABLE: GAME 0x626280
class GM_StaticText : public GM_Component
{
public:
    GM_StaticText(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent) {}
    GM_StaticText(GM_Component *component) : GM_Component(component) {}

    virtual uint GetType() { return 'stxt'; }
    virtual void Draw(uint draw_flags)
    {
        uint color;
        if (flags & COMPF_ALTERNATE_STYLE)
            color = flags & COMPF_ALTERNATE_COLOR ? 0xff000000 : 0xffffff00;
        else
            color = flags & COMPF_ALTERNATE_COLOR ? 0xffffffff : 0xff7f7f7f;
        if (flags & COMPF_TEXT_CENTER)
            mafiaIndicators.OutTextMultiLine((char *)text, x, y, sx, sy, color, OUTTEXT_CENTER, flags & COMPF_FONT);
        else if (flags & COMPF_TEXT_RIGHT)
            mafiaIndicators.OutTextMultiLine((char *)text, x, y, sx, sy, color, OUTTEXT_RIGHT, flags & COMPF_FONT);
        else
            mafiaIndicators.OutTextMultiLine((char *)text, x, y, sx, sy, color, 0, flags & COMPF_FONT);
    }
};

// A static text's own copy of its text (the name is ours), the longest text one less
#define STATICTEXTMEM_SIZE 0x100

// A static text with its own copy of the text (PS2's), 'mtxt': on one line, colored as a static text
// VTABLE: GAME 0x626300
class GM_StaticTextMem : public GM_Component
{
public:
    GM_StaticTextMem(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent) {}
    GM_StaticTextMem(GM_Component *component) : GM_Component(component) {}

    virtual uint GetType() { return 'mtxt'; }
    // The copy's buffer becomes the text
    virtual void SetText(uchar *text)
    {
        strncpy(buffer, (char *)text, STATICTEXTMEM_SIZE);
        this->text = (uchar *)buffer;
    }
    virtual void Draw(uint draw_flags)
    {
        uint color;
        if (flags & COMPF_ALTERNATE_STYLE)
            color = flags & COMPF_ALTERNATE_COLOR ? 0xff000000 : 0xffffff00;
        else
            color = flags & COMPF_ALTERNATE_COLOR ? 0xffffffff : 0xff7f7f7f;
        if (flags & COMPF_TEXT_CENTER)
            mafiaIndicators.OutText((char *)text, x, y, sx, sy, color, OUTTEXT_CENTER, flags & COMPF_FONT, NULL);
        else if (flags & COMPF_TEXT_RIGHT)
            mafiaIndicators.OutText((char *)text, x, y, sx, sy, color, OUTTEXT_RIGHT, flags & COMPF_FONT, NULL);
        else
            mafiaIndicators.OutText((char *)text, x, y, sx, sy, color, 0, flags & COMPF_FONT, NULL);
    }

    char buffer[STATICTEXTMEM_SIZE];    // 0x40 (the name is ours)
};

// A number's or a time's text (the name is ours)
#define STATICTEXTNUM_SIZE 0x20

// A number (PS2's), 'ntxt': its dword printed with the item's text as the format, "%d." without one, on one line,
// colored as a static text
// VTABLE: GAME 0x626380
class GM_StaticTextNum : public GM_Component
{
public:
    GM_StaticTextNum(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        buffer[0] = 0;
    }
    GM_StaticTextNum(GM_Component *component) : GM_Component(component)
    {
        buffer[0] = 0;
    }

    virtual uint GetType() { return 'ntxt'; }
    virtual void SetDword(uint value)
    {
        if (text)
            _snprintf(buffer, STATICTEXTNUM_SIZE, (char *)text, value);
        else
            // STRING: GAME 0x63812c
            _snprintf(buffer, STATICTEXTNUM_SIZE, "%d.", value);
    }
    // The time's copy, the same code
    virtual void Draw(uint draw_flags)
    {
        uint color;
        if (flags & COMPF_ALTERNATE_STYLE)
            color = flags & COMPF_ALTERNATE_COLOR ? 0xff000000 : 0xffffff00;
        else
            color = flags & COMPF_ALTERNATE_COLOR ? 0xffffffff : 0xff7f7f7f;
        if (flags & COMPF_TEXT_CENTER)
            mafiaIndicators.OutText(buffer, x, y, sx, sy, color, OUTTEXT_CENTER, flags & COMPF_FONT, NULL);
        else if (flags & COMPF_TEXT_RIGHT)
            mafiaIndicators.OutText(buffer, x, y, sx, sy, color, OUTTEXT_RIGHT, flags & COMPF_FONT, NULL);
        else
            mafiaIndicators.OutText(buffer, x, y, sx, sy, color, 0, flags & COMPF_FONT, NULL);
    }

    char buffer[STATICTEXTNUM_SIZE];    // 0x40 (the name is ours)
};

// A time (PS2's), 'htxt': its dword in milliseconds printed as minutes, seconds and milliseconds, with the item's text
// as the format or "%d:%02d:%03d", drawn as a number
// VTABLE: GAME 0x626400
class GM_StaticTextTime : public GM_Component
{
public:
    GM_StaticTextTime(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        buffer[0] = 0;
    }
    GM_StaticTextTime(GM_Component *component) : GM_Component(component)
    {
        buffer[0] = 0;
    }

    virtual uint GetType() { return 'htxt'; }
    virtual void SetDword(uint value)
    {
        if (!text)
            // STRING: GAME 0x638130
            _snprintf(buffer, STATICTEXTNUM_SIZE, "%d:%02d:%03d", value / 60000, value / 1000 % 60, value % 1000);
        else
            _snprintf(buffer, STATICTEXTNUM_SIZE, (char *)text, value / 60000, value / 1000 % 60, value % 1000);
    }
    // The number's: a copy the linker folded
    virtual void Draw(uint draw_flags)
    {
        uint color;
        if (flags & COMPF_ALTERNATE_STYLE)
            color = flags & COMPF_ALTERNATE_COLOR ? 0xff000000 : 0xffffff00;
        else
            color = flags & COMPF_ALTERNATE_COLOR ? 0xffffffff : 0xff7f7f7f;
        if (flags & COMPF_TEXT_CENTER)
            mafiaIndicators.OutText(buffer, x, y, sx, sy, color, OUTTEXT_CENTER, flags & COMPF_FONT, NULL);
        else if (flags & COMPF_TEXT_RIGHT)
            mafiaIndicators.OutText(buffer, x, y, sx, sy, color, OUTTEXT_RIGHT, flags & COMPF_FONT, NULL);
        else
            mafiaIndicators.OutText(buffer, x, y, sx, sy, color, 0, flags & COMPF_FONT, NULL);
    }

    char buffer[STATICTEXTNUM_SIZE];    // 0x40 (the name is ours)
};

// A line (PS2's), 'line': from the top left corner to the bottom right one, red in the alternate color, otherwise black
// (the Audio & Video menu's two are flat)
// VTABLE: GAME 0x626480
class GM_Line : public GM_Component
{
public:
    GM_Line(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent) {}
    GM_Line(GM_Component *component) : GM_Component(component) {}

    virtual uint GetType() { return 'line'; }
    virtual void Draw(uint draw_flags)
    {
        if (flags & COMPF_ALTERNATE_COLOR)
            Line(x, y, sx, sy, 0xffff0000);
        else
            Line(x, y, sx, sy, 0xff000000);
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

// A clipper (PS2's GM_Orezavac, Czech "orezavac", a sharpener: what cuts), 'orez': its Draw sets the viewport to the
// component, which the components drawn after it are clipped to
// VTABLE: GAME 0x626580
class GM_Orezavac : public GM_Component
{
public:
    GM_Orezavac(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent) {}
    GM_Orezavac(GM_Component *component) : GM_Component(component) {}

    virtual uint GetType() { return 'orez'; }
    virtual void Draw(uint draw_flags)
    {
        SetViewport((int)x, (int)y, (int)sx, (int)sy);
    }
};

// A picture (PS2's), 'rctt': the texture its string names, over the rectangle, none until it has one. PS2's also takes
// the texture as its pointer (SetPointer, GetPointer).
// VTABLE: GAME 0x626600
class GM_RectangleTexture : public GM_Component
{
public:
    GM_RectangleTexture(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        texture = NULL;
    }
    GM_RectangleTexture(GM_Component *component) : GM_Component(component)
    {
        texture = NULL;
    }
    virtual ~GM_RectangleTexture()
    {
        if (texture)
            texture->Release();
    }

    virtual uint GetType() { return 'rctt'; }
    virtual void Draw(uint draw_flags)
    {
        if (texture)
            RectangleFilled(x, y, sx, sy, 0xffffffff, texture);
    }
    // The texture of that file, the previous one released
    virtual void SetString(uchar *text)
    {
        if (texture)
        {
            texture->Release();
            texture = NULL;
        }
        igraph->CreateITexture((const char *)text, NULL, ITC_ALPHA, &texture);
    }

    ITexture *texture;                  // 0x40 (the name is ours)
};

// A progress bar (PS2's), 'pbar': the menus' beads (GM_Menu::textureProgress), grey over the whole width and red over
// the part its value fills, 0 to 1
// VTABLE: GAME 0x626680
class GM_ProgressBar : public GM_Component
{
public:
    GM_ProgressBar(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        value = 0.0f;
    }
    GM_ProgressBar(GM_Component *component) : GM_Component(component)
    {
        value = 0.0f;
    }

    virtual uint GetType() { return 'pbar'; }
    virtual void Draw(uint draw_flags)
    {
        RectangleProgress(x, y, sx, sy, 0x5f7f7f7f, 1.0f, GM_Menu::textureProgress);
        RectangleProgress(x, y, sx, sy, 0xffff0000, value, GM_Menu::textureProgress);
    }
    virtual void SetFloat(float value) { this->value = value; }

    float value;                        // 0x40 (the name is ours)
};

// A menu's item (PS2's), 'item': its text, which the mouse's left button or Space, Enter and the keypad's Enter click.
// It can be activated.
// VTABLE: GAME 0x626700
class GM_MenuItem : public GM_Component
{
public:
    GM_MenuItem(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        flags |= COMPF_ACTIVATABLE;
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
            if (flags & COMPF_TEXT_RED)
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

// A menu item with its own copy of the text (PS2's), 'itmm': drawn and clicked as a menu item, the copy as a static
// text's. It can be activated.
// VTABLE: GAME 0x626780
class GM_MenuItemMem : public GM_Component
{
public:
    GM_MenuItemMem(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        flags |= COMPF_ACTIVATABLE;
    }
    GM_MenuItemMem(GM_Component *component) : GM_Component(component) {}

    virtual uint GetType() { return 'itmm'; }
    // The menu item's
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
            if (flags & COMPF_TEXT_RED)
                mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xffff0000, text_flags, flags & COMPF_FONT, NULL);
            else
                mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xff000000, text_flags, flags & COMPF_FONT, NULL);
        }
        else
            mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xff7f7f7f, text_flags, flags & COMPF_FONT, NULL);
    }
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
    // The static text's
    virtual void SetText(uchar *text)
    {
        strncpy(buffer, (char *)text, STATICTEXTMEM_SIZE);
        this->text = (uchar *)buffer;
    }

    char buffer[STATICTEXTMEM_SIZE];    // 0x40 (the name is ours)
};

// The text of a number's menu item and of a number edit (the name is ours)
#define MENUITEMNUM_SIZE 0x40

// A number's menu item (PS2's), 'iten': its value, 0 to 99 at first, printed with the item's text as the format, a "-"
// at the left end and a "+" at the right one, drawn as a menu item. Space, the Enters, '=' and the keypad's '+' raise it,
// '-' and the keypad's '-' lower it, within the range; the mouse's wheel moves it by its steps, its left button lowers
// it on the left half and raises it on the right one. It can be activated. PS2's arrows move it too, and around the
// range's ends.
// VTABLE: GAME 0x626800
class GM_MenuItemNum : public GM_Component
{
public:
    GM_MenuItemNum(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        flags |= COMPF_ACTIVATABLE;
        min_value = 0;
        max_value = 99;
        value = 0;
        _snprintf(buffer, MENUITEMNUM_SIZE, (char *)text, value);
    }
    GM_MenuItemNum(GM_Component *component) : GM_Component(component)
    {
        min_value = 0;
        max_value = 99;
        value = 0;
        _snprintf(buffer, MENUITEMNUM_SIZE, (char *)text, value);
    }

    virtual uint GetType() { return 'iten'; }
    // The range, and the value moved into it
    virtual void SetRangeDword(uint min, uint max)
    {
        min_value = min;
        max_value = max;
        if (value < min_value)
            value = min_value;
        if (value > max_value)
            value = max_value;
        _snprintf(buffer, MENUITEMNUM_SIZE, (char *)text, value);
    }
    // Within the range
    virtual void SetDword(uint value)
    {
        this->value = value;
        if (this->value < min_value)
            this->value = min_value;
        if (this->value > max_value)
            this->value = max_value;
        _snprintf(buffer, MENUITEMNUM_SIZE, (char *)text, this->value);
    }
    // The original kept another class's copy, the same code
    virtual uint GetDword() { return value; }
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
            if (flags & COMPF_TEXT_RED)
            {
                mafiaIndicators.OutText(buffer, x, y, sx, sy, 0xffff0000, text_flags, flags & COMPF_FONT, NULL);
                mafiaIndicators.OutText("-", x, y, sx, sy, 0xffff0000, 0, flags & COMPF_FONT, NULL);
                // STRING: GAME 0x638140
                mafiaIndicators.OutText("+", x, y, sx, sy, 0xffff0000, OUTTEXT_RIGHT, flags & COMPF_FONT, NULL);
            }
            else
            {
                mafiaIndicators.OutText(buffer, x, y, sx, sy, 0xff000000, text_flags, flags & COMPF_FONT, NULL);
                mafiaIndicators.OutText("-", x, y, sx, sy, 0xff000000, 0, flags & COMPF_FONT, NULL);
                mafiaIndicators.OutText("+", x, y, sx, sy, 0xff000000, OUTTEXT_RIGHT, flags & COMPF_FONT, NULL);
            }
        }
        else
        {
            mafiaIndicators.OutText(buffer, x, y, sx, sy, 0xff7f7f7f, text_flags, flags & COMPF_FONT, NULL);
            mafiaIndicators.OutText("-", x, y, sx, sy, 0xff7f7f7f, 0, flags & COMPF_FONT, NULL);
            mafiaIndicators.OutText("+", x, y, sx, sy, 0xff7f7f7f, OUTTEXT_RIGHT, flags & COMPF_FONT, NULL);
        }
    }
    virtual uint Mouse(int x, int y, int z, uint buttons, uint time)
    {
        if (z)
        {
            int v = value + z / 120;
            if (v < (int)min_value)
                v = min_value;
            if (v > max_value)
                v = max_value;
            value = v;
            _snprintf(buffer, MENUITEMNUM_SIZE, (char *)text, value);
            return id;
        }
        if (buttons & MENUMOUSE_LEFT_RELEASED)
        {
            if ((float)x > this->x + sx * .5f)
            {
                if (value < max_value)
                    value++;
            }
            else
            {
                if (value > min_value)
                    value--;
            }
            _snprintf(buffer, MENUITEMNUM_SIZE, (char *)text, value);
            return id;
        }
        return 0;
    }
    virtual uint Key(uint shift_keys, uint key)
    {
        if (key == DIK_SPACE || key == DIK_RETURN || key == DIK_NUMPADENTER)
        {
            if (value < max_value)
                value++;
        }
        else if (key == DIK_MINUS || key == DIK_SUBTRACT)
        {
            if (value > min_value)
                value--;
        }
        else if (key == DIK_EQUALS || key == DIK_ADD)
        {
            if (value < max_value)
                value++;
        }
        else
            return 0;
        _snprintf(buffer, MENUITEMNUM_SIZE, (char *)text, value);
        return id;
    }

    // The names are ours (not min and max, windows.h's macros)
    uint min_value;                     // 0x40
    uint max_value;                     // 0x44
    uint value;                         // 0x48
    char buffer[MENUITEMNUM_SIZE];      // 0x4c
};

// The race rounds' texts (the names are ours, after the texts of tables\textdb_en.def)
#define TEXT_ROUND_LOCKED 0x1a7         // "Locked"
#define TEXT_ROUND_UNLOCKED 0x1a8       // "Unlocked"
#define TEXT_ROUND_COMPLETED 0x1a9      // "Completed"

// A race round's menu item (PS2's), 'rari': its text, and on the right "Completed" where its bool is set, "Unlocked"
// otherwise, and "Locked" while disabled; red in the alternate color, otherwise black, grey while disabled. Clicked as a
// menu item. It can be activated.
// VTABLE: GAME 0x626880
class GM_RaceRoundItem : public GM_Component
{
public:
    GM_RaceRoundItem(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        flags |= COMPF_ACTIVATABLE;
        state_text = (uchar *)textDatabase.GetText(TEXT_ROUND_UNLOCKED);
        locked_text = (uchar *)textDatabase.GetText(TEXT_ROUND_LOCKED);
    }
    GM_RaceRoundItem(GM_Component *component) : GM_Component(component)
    {
        state_text = (uchar *)textDatabase.GetText(TEXT_ROUND_UNLOCKED);
        locked_text = (uchar *)textDatabase.GetText(TEXT_ROUND_LOCKED);
    }

    virtual uint GetType() { return 'rari'; }
    // Completed or not
    virtual void SetBool(bool value)
    {
        if (value)
            state_text = (uchar *)textDatabase.GetText(TEXT_ROUND_COMPLETED);
        else
            state_text = (uchar *)textDatabase.GetText(TEXT_ROUND_UNLOCKED);
    }
    virtual void Draw(uint draw_flags)
    {
        uint text_flags = 0;
        if (flags & COMPF_ACTIVE)
            text_flags |= OUTTEXT_UNDERLINE;
        if (draw_flags & COMPF_ENABLED)
        {
            if (flags & COMPF_ALTERNATE_COLOR)
            {
                mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xffff0000, text_flags, flags & COMPF_FONT, NULL);
                text_flags |= OUTTEXT_RIGHT;
                mafiaIndicators.OutText((char *)state_text, x, y, sx, sy, 0xffff0000, text_flags, flags & COMPF_FONT,
                    NULL);
            }
            else
            {
                mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xff000000, text_flags, flags & COMPF_FONT, NULL);
                text_flags |= OUTTEXT_RIGHT;
                mafiaIndicators.OutText((char *)state_text, x, y, sx, sy, 0xff000000, text_flags, flags & COMPF_FONT,
                    NULL);
            }
        }
        else
        {
            mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xff7f7f7f, text_flags, flags & COMPF_FONT, NULL);
            text_flags |= OUTTEXT_RIGHT;
            mafiaIndicators.OutText((char *)locked_text, x, y, sx, sy, 0xff7f7f7f, text_flags, flags & COMPF_FONT,
                NULL);
        }
    }
    // The menu item's
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

    uchar *state_text;                  // 0x40 (the names are ours)
    uchar *locked_text;               // 0x44
};

// A button (PS2's), 'butt': its text, which clicks as a menu item's does, underlined while active. Red in the alternate
// color, otherwise white in the alternate style and black in the normal one; while disabled dark red or grey. It can be
// activated.
// VTABLE: GAME 0x626900
class GM_Button : public GM_Component
{
public:
    GM_Button(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        flags |= COMPF_ACTIVATABLE;
    }
    GM_Button(GM_Component *component) : GM_Component(component) {}

    virtual uint GetType() { return 'butt'; }
    virtual void Draw(uint draw_flags)
    {
        uint text_flags = 0;
        if (flags & COMPF_ACTIVE)
            text_flags |= OUTTEXT_UNDERLINE;
        if (flags & COMPF_TEXT_CENTER)
            text_flags |= OUTTEXT_CENTER;
        if (flags & COMPF_TEXT_RIGHT)
            text_flags |= OUTTEXT_RIGHT;
        if (flags & COMPF_ALTERNATE_STYLE)
        {
            if (flags & COMPF_ALTERNATE_COLOR)
            {
                if (draw_flags & COMPF_ENABLED)
                    mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xffff0000, text_flags,
                        flags & COMPF_FONT, NULL);
                else
                    mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xff7f3f3f, text_flags,
                        flags & COMPF_FONT, NULL);
            }
            else
            {
                if (draw_flags & COMPF_ENABLED)
                    mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xffffffff, text_flags,
                        flags & COMPF_FONT, NULL);
                else
                    mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xff7f7f7f, text_flags,
                        flags & COMPF_FONT, NULL);
            }
        }
        else
        {
            if (flags & COMPF_ALTERNATE_COLOR)
            {
                if (draw_flags & COMPF_ENABLED)
                    mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xffff0000, text_flags,
                        flags & COMPF_FONT, NULL);
                else
                    mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xff7f3f3f, text_flags,
                        flags & COMPF_FONT, NULL);
            }
            else
            {
                if (draw_flags & COMPF_ENABLED)
                    mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xff000000, text_flags,
                        flags & COMPF_FONT, NULL);
                else
                    mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xff7f7f7f, text_flags,
                        flags & COMPF_FONT, NULL);
            }
        }
    }
    // The menu item's: PS2's has Enter and the keypad's Enter
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

// A clean button (PS2's), 'butc': nothing drawn, a parent, clicked by the mouse's left button released over it. It
// can be activated.
// VTABLE: GAME 0x626980
class GM_ButtonClean : public GM_Component
{
public:
    GM_ButtonClean(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        flags |= COMPF_ACTIVATABLE;
    }
    GM_ButtonClean(GM_Component *component) : GM_Component(component) {}

    virtual uint GetType() { return 'butc'; }
    virtual bool CanBeParent() { return true; }
    // PS2's is its own empty function, the PC's folded with the base's
    virtual void Draw(uint draw_flags) {}
    // The player row's: the original kept its copy, the same code
    virtual uint Mouse(int x, int y, int z, uint buttons, uint time)
    {
        if (buttons & MENUMOUSE_LEFT_RELEASED)
        {
            if (IsOverActive((float)x, (float)y))
                return id;
        }
        return 0;
    }
};

// A slider (PS2's), 'slid': its value, 0 to 1, a red bar from the left between two arrows, each a square as high as
// the slider; black outlines and arrows, the arrows red while it is active, all grey while it is disabled. The keys'
// arrows and the mouse's left button on the arrows move it, the button between them sets it. It can be activated. PS2's
// has keypad 4 and 6 besides the arrows, and draws the arrows first.
// VTABLE: GAME 0x626a00
class GM_Slider : public GM_Component
{
public:
    GM_Slider(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        flags |= COMPF_ACTIVATABLE;
        value = .5f;
    }
    GM_Slider(GM_Component *component) : GM_Component(component)
    {
        value = ((GM_Slider *)component)->value;
    }

    virtual uint GetType() { return 'slid'; }
    virtual void SetFloat(float value)
    {
        this->value = value;
        ForceUpdate();
    }
    virtual float GetFloat() { return value; }
    virtual void Draw(uint draw_flags)
    {
        float bar = value * (sx - 2.0f * sy);
        float arrow_offset = sy / 4;
        float arrow_size = sy / 2;
        if (flags & COMPF_ENABLED)
        {
            RectangleFilled(x + sy, y, bar, sy, 0xffbe002c, NULL);
            Rectangle(x, y, sx, sy, 0xff000000);
            Rectangle(x + sy, y, sx - 2.0f * sy, sy, 0xff000000);
            if (flags & COMPF_ACTIVE)
            {
                Triangle(x + arrow_offset, y + arrow_offset, arrow_size, arrow_size, TRIANGLE_LEFT, 0xffbe002c);
                Triangle(x + sx - sy + arrow_offset, y + arrow_offset, arrow_size, arrow_size, TRIANGLE_RIGHT,
                    0xffbe002c);
            }
            else
            {
                Triangle(x + arrow_offset, y + arrow_offset, arrow_size, arrow_size, TRIANGLE_LEFT, 0xff000000);
                Triangle(x + sx - sy + arrow_offset, y + arrow_offset, arrow_size, arrow_size, TRIANGLE_RIGHT,
                    0xff000000);
            }
        }
        else
        {
            Triangle(x + arrow_offset, y + arrow_offset, arrow_size, arrow_size, TRIANGLE_LEFT, 0xff707070);
            Triangle(x + sx - sy + arrow_offset, y + arrow_offset, arrow_size, arrow_size, TRIANGLE_RIGHT, 0xff707070);
            RectangleFilled(x + sy, y, bar, sy, 0x7f707070, NULL);
            Rectangle(x, y, sx, sy, 0xff707070);
            Rectangle(x + sy, y, sx - 2.0f * sy, sy, 0xff707070);
        }
    }
    // While the left button is down over it: left of the bar the value goes down, right of it up, by .002 a
    // millisecond, and over it the value is where the mouse is
    virtual uint Mouse(int x, int y, int z, uint buttons, uint time)
    {
        float mouse_x = (float)x;
        if (IsOverActive(mouse_x, (float)y))
        {
            float width = sx - 2.0f * sy;
            if (buttons & MENUMOUSE_LEFT)
            {
                int pos = (int)(mouse_x - (this->x + sy));
                if (pos < 0)
                {
                    value -= time * .002f;
                    if (value < 0.0f)
                        value = 0.0f;
                    ForceUpdate();
                    return id;
                }
                else if (pos > width)
                {
                    value += time * .002f;
                    if (value > 1.0f)
                        value = 1.0f;
                    ForceUpdate();
                    return id;
                }
                else
                {
                    value = pos / width;
                    ForceUpdate();
                    return id;
                }
            }
        }
        return 0;
    }
    // The arrows move it by a tenth
    virtual uint Key(uint shift_keys, uint key)
    {
        switch (key)
        {
        case DIK_RIGHT:
            value += .1f;
            if (value > 1.0f)
                value = 1.0f;
            ForceUpdate();
            return id;
        case DIK_LEFT:
            value -= .1f;
            if (value < 0.0f)
                value = 0.0f;
            ForceUpdate();
            return id;
        }
        return 0;
    }

    float value;                        // 0x40 (the name is ours)
};

// A slider of whole numbers (PS2's), 'sldi': its value, 0 to 10 at first, drawn and moved as a slider's, the bar over the
// value's share of the range; the mouse's arrows move it by one while the left button is pressed (MENUMOUSE_LEFT_PRESSED),
// the keys' arrows around the range's ends. It can be activated. Its copy takes the source's value, minimum and maximum
// all into its value, so the maximum's is what it keeps.
// VTABLE: GAME 0x626a80
class GM_SliderInteger : public GM_Component
{
public:
    GM_SliderInteger(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        flags |= COMPF_ACTIVATABLE;
        value = 0;
        min_value = 0;
        max_value = 10;
    }
    GM_SliderInteger(GM_Component *component) : GM_Component(component)
    {
        value = ((GM_SliderInteger *)component)->value;
        value = ((GM_SliderInteger *)component)->min_value;
        value = ((GM_SliderInteger *)component)->max_value;
    }

    virtual uint GetType() { return 'sldi'; }
    virtual void SetDword(uint value)
    {
        this->value = value;
        ForceUpdate();
    }
    // The original kept another class's copy, the same code
    virtual uint GetDword() { return value; }
    // The selection's: a copy the linker folded
    virtual void SetRangeDword(uint min, uint max)
    {
        min_value = min;
        max_value = max;
    }
    virtual void Draw(uint draw_flags)
    {
        float offset = value - min_value;
        float bar = offset * (sx - 2.0f * sy) / (max_value - min_value);
        float arrow_offset = sy / 4;
        float arrow_size = sy / 2;
        if (flags & COMPF_ENABLED)
        {
            RectangleFilled(x + sy, y, bar, sy, 0xffbe002c, NULL);
            Rectangle(x, y, sx, sy, 0xff000000);
            Rectangle(x + sy, y, sx - 2.0f * sy, sy, 0xff000000);
            if (flags & COMPF_ACTIVE)
            {
                Triangle(x + arrow_offset, y + arrow_offset, arrow_size, arrow_size, TRIANGLE_LEFT, 0xffbe002c);
                Triangle(x + sx - sy + arrow_offset, y + arrow_offset, arrow_size, arrow_size, TRIANGLE_RIGHT,
                    0xffbe002c);
            }
            else
            {
                Triangle(x + arrow_offset, y + arrow_offset, arrow_size, arrow_size, TRIANGLE_LEFT, 0xff000000);
                Triangle(x + sx - sy + arrow_offset, y + arrow_offset, arrow_size, arrow_size, TRIANGLE_RIGHT,
                    0xff000000);
            }
        }
        else
        {
            Triangle(x + arrow_offset, y + arrow_offset, arrow_size, arrow_size, TRIANGLE_LEFT, 0xff707070);
            Triangle(x + sx - sy + arrow_offset, y + arrow_offset, arrow_size, arrow_size, TRIANGLE_RIGHT, 0xff707070);
            RectangleFilled(x + sy, y, bar, sy, 0x7f707070, NULL);
            Rectangle(x, y, sx, sy, 0xff707070);
            Rectangle(x + sy, y, sx - 2.0f * sy, sy, 0xff707070);
        }
    }
    // While the left button is down over it: left of the bar the value goes down by one, right of it up, where the
    // button was just pressed; over the bar it takes the nearest value to the mouse's place
    // Open: the clamp of the decrement, which the original gives its own copy of the ending
    virtual uint Mouse(int x, int y, int z, uint buttons, uint time)
    {
        float mouse_x = (float)x;
        if (IsOverActive(mouse_x, (float)y))
        {
            float width = sx - 2.0f * sy;
            if (buttons & MENUMOUSE_LEFT)
            {
                int pos = (int)(mouse_x - (this->x + sy));
                if (pos < 0)
                {
                    if (buttons & MENUMOUSE_LEFT_PRESSED)
                    {
                        if (--value < min_value)
                            value = min_value;
                        ForceUpdate();
                        return id;
                    }
                }
                else if (pos > width)
                {
                    if (buttons & MENUMOUSE_LEFT_PRESSED)
                    {
                        if (++value > max_value)
                            value = max_value;
                        ForceUpdate();
                        return id;
                    }
                }
                else
                {
                    float f = pos / width * (max_value - min_value) + min_value;
                    value = (int)(f >= 0.0f ? f + .5f : f - .5f);
                    ForceUpdate();
                    return id;
                }
            }
        }
        return 0;
    }
    // The arrows move it by one, around the range's ends
    virtual uint Key(uint shift_keys, uint key)
    {
        switch (key)
        {
        case DIK_RIGHT:
            if (++value > max_value)
                value = min_value;
            ForceUpdate();
            return id;
        case DIK_LEFT:
            if (--value < min_value)
                value = max_value;
            ForceUpdate();
            return id;
        }
        return 0;
    }

    // The names are ours (not min and max, windows.h's macros)
    int min_value;                      // 0x40
    int max_value;                      // 0x44
    int value;                          // 0x48
};

// A player's row's texts (the names are ours, after the texts of tables\textdb_en.def)
#define TEXT_PLAYER_NAME 0x2c0         // "Player Name"
#define TEXT_CAR_NAME 0x2c1            // "Car Name"
#define TEXT_READY_MARK 0x2c2          // "R"
#define TEXT_AI_MARK 0x2c3             // "AI"

// A player's row (PS2's), 'netp', of a race's lobby, which PC 1.2's menus don't have (its texts "Player Name",
// "Car Name", "R" and "AI" are 0x2c0-0x2c3, next to "Active players:", "Ready", "Add AI Opponent", ...). In the
// alternate style the list's header: the two titles on grey. Otherwise a row in an outline, an odd id's on grey: a
// square at the left with the ready mark (a green "R") or else the AI's (a blue "AI"), the two texts smaller, and the
// X box at the right end, raised while the row is active and sunk with COMPF_PRESSED. Its pointer is the player's. A
// copy takes none of its members (PS2's neither). PS2's has an empty Key and no Mouse; the PC's left button released
// over it clicks it, as the clean button's.
// VTABLE: GAME 0x626b00
class GM_NetPlayer : public GM_Component
{
public:
    GM_NetPlayer(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        name[0] = 0;
        car_name[0] = 0;
        player = NULL;
        name_text = textDatabase.GetText(TEXT_PLAYER_NAME);
        car_text = textDatabase.GetText(TEXT_CAR_NAME);
        ready_text = textDatabase.GetText(TEXT_READY_MARK);
        ai_text = textDatabase.GetText(TEXT_AI_MARK);
    }
    GM_NetPlayer(GM_Component *component) : GM_Component(component) {}

    virtual uint GetType() { return 'netp'; }
    virtual void SetPointer(void *value) { player = value; }
    virtual void *GetPointer() { return player; }
    virtual void Draw(uint draw_flags)
    {
        if (flags & COMPF_ALTERNATE_STYLE)
        {
            RectangleFilled(x, y, sx, sy, 0xff7f7f7f, NULL);
            mafiaIndicators.OutText(name_text, x + sy, y, sx, sy, 0xff000000, 0, 0, NULL);
            mafiaIndicators.OutText(car_text, x + sx / 2, y, sx, sy, 0xff000000, 0, 0, NULL);
        }
        else
        {
            if (id & 1)
                RectangleFilled(x, y, sx, sy, 0x7f7f7f7f, NULL);
            Rectangle(x, y, sx, sy, 0xff707070);
            Rectangle(x + sy, y, sx / 2 - sy, sy, 0xff707070);
            if (flags & COMPF_TEXT_RED)
                mafiaIndicators.OutText(ready_text, x, y, sy, sy, 0xff00ff00, OUTTEXT_CENTER, 0, NULL);
            else if (flags & COMPF_AI)
                mafiaIndicators.OutText(ai_text, x, y, sy, sy, 0xff1f1fff, OUTTEXT_CENTER, 0, NULL);
            mafiaIndicators.OutText(name_text, x + sy, y, sx, sy * .8f, 0xff000000, 0, 0, NULL);
            mafiaIndicators.OutText(car_text, x + sx / 2, y, sx, sy * .8f, 0xff000000, 0, 0, NULL);
            if (flags & COMPF_X_BOX)
            {
                // STRING: GAME 0x638148
                mafiaIndicators.OutText("X", x + sx - sy, y, sy, sy, 0xff000000, OUTTEXT_CENTER, 1, NULL);
                if (flags & COMPF_ACTIVE)
                {
                    if (flags & COMPF_PRESSED)
                        RectangleBB(x + sx - sy, y, sy, sy, BEVEL_OUTER_SUNK | BEVEL_INNER_SUNK);
                    else
                        RectangleBB(x + sx - sy, y, sy, sy, BEVEL_RAISED);
                }
                else
                    RectangleBB(x + sx - sy, y, sy, sy, BEVEL_OUTER_SUNK);
            }
        }
    }
    // The clean button's: the original kept this class's copy
    virtual uint Mouse(int x, int y, int z, uint buttons, uint time)
    {
        if (buttons & MENUMOUSE_LEFT_RELEASED)
        {
            if (IsOverActive((float)x, (float)y))
                return id;
        }
        return 0;
    }
    // PS2's is its own empty function, the PC's folded with the base's
    virtual uint Key(uint shift_keys, uint key) { return 0; }

    // The names are ours
    char name[0x50];                    // 0x40, most likely the player's name (no function of the class reads it)
    char car_name[0x50];                // 0x90, most likely the car's
    void *player;                       // 0xe0, SetPointer's
    char *name_text;                    // 0xe4, text 0x2c0, "Player Name"
    char *car_text;                     // 0xe8, text 0x2c1, "Car Name"
    char *ready_text;                   // 0xec, text 0x2c2, "R"
    char *ai_text;                      // 0xf0, text 0x2c3, "AI"
};

// A text edit's buffer (the name is ours): the longest text is one less
#define TEXTEDIT_SIZE 0x40

// A text edit (PS2's), 'edit': its own copy of the text, which the keys extend and Backspace shortens, black and
// centered with a blinking cursor while active. Its text starts as the item's and is at most the length its dword gives
// (the whole buffer's to start). It can be activated.
// VTABLE: GAME 0x626b80
class GM_TextEdit : public GM_Component
{
public:
    GM_TextEdit(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        flags |= COMPF_ACTIVATABLE;
        buffer[0] = 0;
        if (text)
            strncpy(buffer, (char *)text, TEXTEDIT_SIZE);
        max_length = TEXTEDIT_SIZE - 1;
    }
    GM_TextEdit(GM_Component *component) : GM_Component(component)
    {
        buffer[0] = 0;
        if (text)
            strncpy(buffer, (char *)text, TEXTEDIT_SIZE);
        max_length = TEXTEDIT_SIZE - 1;
    }

    virtual uint GetType() { return 'edit'; }
    // The cursor where the text ends, while the component is active and the cursor's blink shows it
    virtual void Draw(uint draw_flags)
    {
        float end = mafiaIndicators.OutText(buffer, x, y, sx, sy, 0xff000000, OUTTEXT_CENTER | OUTTEXT_FIT, 0, NULL);
        if (flags & COMPF_ACTIVE && draw_flags & MENUDRAW_CURSOR)
            // STRING: GAME 0x637938
            mafiaIndicators.OutText("_", end, y, sx, sy, 0xff000000, 0, 0, NULL);
    }
    // Backspace removes the last character; another key adds the character IGraph read for it, if there is room and it
    // isn't a control character. Never clicks.
    virtual uint Key(uint shift_keys, uint key)
    {
        int len = strlen(buffer);
        if (key != DIK_BACK)
        {
            if (len < max_length)
            {
                uchar c = igraph->ReadChar();
                if (c >= ' ')
                {
                    buffer[len] = c;
                    buffer[len + 1] = 0;
                }
            }
        }
        else
        {
            if (len > 0)
                len--;
            buffer[len] = 0;
        }
        return 0;
    }
    // A copy of the text, cut to the length
    virtual void SetText(uchar *text)
    {
        buffer[0] = 0;
        if (text)
            strncpy(buffer, (char *)text, TEXTEDIT_SIZE);
        buffer[max_length] = 0;
    }
    virtual uchar *GetText() { return (uchar *)buffer; }
    // The length, at most the buffer's; the text is cut to it
    virtual void SetDword(uint value)
    {
        max_length = value;
        if (max_length > TEXTEDIT_SIZE - 1)
            max_length = TEXTEDIT_SIZE - 1;
        buffer[max_length] = 0;
    }

    char buffer[TEXTEDIT_SIZE];         // 0x40 (the names are ours)
    uint max_length;                    // 0x80
};

// A check box (PS2's), 'chck': its bool, a box with an "X" in it while set, and its text right of it; black, grey while
// disabled. Space, the Enters and the mouse's left button released over it toggle it. It can be activated.
// Open: Draw, whose active and inactive branches the original keeps as two copies of the same code (PS2's active one
// underlines the text), where ours merges them.
// VTABLE: GAME 0x626c00
class GM_CheckBox : public GM_Component
{
public:
    GM_CheckBox(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        flags |= COMPF_ACTIVATABLE;
        checked = false;
    }
    GM_CheckBox(GM_Component *component) : GM_Component(component)
    {
        checked = ((GM_CheckBox *)component)->checked;
    }

    virtual uint GetType() { return 'chck'; }
    virtual void Draw(uint draw_flags)
    {
        float box = sy * .8f;
        float box_x = x + sy * .1f;
        if (flags & COMPF_ENABLED)
        {
            if (flags & COMPF_ACTIVE)
            {
                Rectangle(box_x, y, box, box, 0xff000000);
                if (checked)
                    mafiaIndicators.OutText("X", x, y, sy, sy * .8f, 0xff000000, OUTTEXT_CENTER, 0, NULL);
                mafiaIndicators.OutText((char *)text, x + sy, y, sx, sy, 0xff000000, 0, 1, NULL);
            }
            else
            {
                Rectangle(box_x, y, box, box, 0xff000000);
                if (checked)
                    mafiaIndicators.OutText("X", x, y, sy, sy * .8f, 0xff000000, OUTTEXT_CENTER, 0, NULL);
                mafiaIndicators.OutText((char *)text, x + sy, y, sx, sy, 0xff000000, 0, 1, NULL);
            }
        }
        else
        {
            Rectangle(box_x, y, box, box, 0xff707070);
            if (checked)
                mafiaIndicators.OutText("X", x, y, sy, sy * .8f, 0xff707070, OUTTEXT_CENTER, 0, NULL);
            mafiaIndicators.OutText((char *)text, x + sy, y, sx, sy, 0xff707070, 0, 1, NULL);
        }
    }
    virtual void SetBool(bool value) { checked = value; }
    virtual bool GetBool() { return checked; }
    virtual uint Key(uint shift_keys, uint key)
    {
        if (key == DIK_SPACE || key == DIK_RETURN || key == DIK_NUMPADENTER)
        {
            checked = !checked;
            return id;
        }
        return 0;
    }
    virtual uint Mouse(int x, int y, int z, uint buttons, uint time)
    {
        if (buttons & MENUMOUSE_LEFT_RELEASED)
        {
            if (IsOverActive((float)x, (float)y))
            {
                checked = !checked;
                return id;
            }
        }
        return 0;
    }

    bool checked;                       // 0x40 (the name is ours)
    uchar unknown_41[3];                // 0x41
};

// A selection (PS2's), 'bsel': the texts of a range of the text database's ids, its text the selected one, which a
// click moves to the next one, from the last one around to the first. Drawn as a menu item, without its red. It can be
// activated. PS2's arrows move it either way; Space and the Enters click it, as a menu item, and so does the mouse's
// left button.
// VTABLE: GAME 0x626c80
class GM_ButtonSelect : public GM_Component
{
public:
    GM_ButtonSelect(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        flags |= COMPF_ACTIVATABLE;
    }
    GM_ButtonSelect(GM_Component *component) : GM_Component(component) {}

    virtual uint GetType() { return 'bsel'; }
    // The range's first and last ids
    virtual void SetRangeDword(uint min, uint max)
    {
        first_text = min;
        last_text = max;
    }
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
            mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xff000000, text_flags, flags & COMPF_FONT, NULL);
        else
            mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xff7f7f7f, text_flags, flags & COMPF_FONT, NULL);
    }
    virtual uint Mouse(int x, int y, int z, uint buttons, uint time)
    {
        if (buttons & MENUMOUSE_LEFT_RELEASED)
        {
            if (++text_id > last_text)
                text_id = first_text;
            text = (uchar *)textDatabase.GetText(text_id);
            return id;
        }
        return 0;
    }
    virtual uint Key(uint shift_keys, uint key)
    {
        if (key == DIK_SPACE || key == DIK_RETURN || key == DIK_NUMPADENTER)
        {
            if (++text_id > last_text)
                text_id = first_text;
            text = (uchar *)textDatabase.GetText(text_id);
            return id;
        }
        return 0;
    }

    // 0x40, the range of the text database's ids (the names are ours: not min and max, windows.h's macros)
    uint first_text, last_text;
};

// A yes or no selection (PS2's), 'bslb': its bool, its text the text database's of one of two ids, which its range
// gives (the minimum false's, the maximum true's); a click toggles it. Drawn as a selection. It can be activated.
// VTABLE: GAME 0x626d00
class GM_ButtonSelectBool : public GM_Component
{
public:
    GM_ButtonSelectBool(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        flags |= COMPF_ACTIVATABLE;
        value = false;
    }
    GM_ButtonSelectBool(GM_Component *component) : GM_Component(component) {}

    virtual uint GetType() { return 'bslb'; }
    // The two ids
    virtual void SetRangeDword(uint min, uint max)
    {
        false_text = min;
        true_text = max;
        if (value)
            SetTextID(true_text);
        else
            SetTextID(false_text);
    }
    virtual void SetBool(bool value)
    {
        this->value = value;
        if (value)
            SetTextID(true_text);
        else
            SetTextID(false_text);
    }
    virtual bool GetBool() { return value; }
    // The selection's: the original kept this class's copy, the same code
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
            mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xff000000, text_flags, flags & COMPF_FONT, NULL);
        else
            mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xff7f7f7f, text_flags, flags & COMPF_FONT, NULL);
    }
    virtual uint Mouse(int x, int y, int z, uint buttons, uint time)
    {
        if (buttons & MENUMOUSE_LEFT_RELEASED)
        {
            value = !value;
            if (value)
                SetTextID(true_text);
            else
                SetTextID(false_text);
            return id;
        }
        return 0;
    }
    virtual uint Key(uint shift_keys, uint key)
    {
        if (key == DIK_SPACE || key == DIK_RETURN || key == DIK_NUMPADENTER)
        {
            value = !value;
            if (value)
                SetTextID(true_text);
            else
                SetTextID(false_text);
            return id;
        }
        return 0;
    }

    // 0x40, the text database's ids of false and true (the names are ours)
    uint false_text, true_text;
    bool value;                         // 0x48 (the name is ours)
    uchar unknown_49[3];                // 0x49
};

// A number edit (PS2's), 'edin': its value, 0 to 99 at first, printed as "%d" in an outlined box with a black block at
// its right end, drawn as a number's menu item and moved as one. It can be activated.
// VTABLE: GAME 0x626d80
class GM_EditNum : public GM_Component
{
public:
    GM_EditNum(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        flags |= COMPF_ACTIVATABLE;
        min_value = 0;
        max_value = 99;
        value = 0;
        _snprintf(buffer, MENUITEMNUM_SIZE, "%d", value);
    }
    GM_EditNum(GM_Component *component) : GM_Component(component)
    {
        min_value = 0;
        max_value = 99;
        value = 0;
        _snprintf(buffer, MENUITEMNUM_SIZE, "%d", value);
    }

    virtual uint GetType() { return 'edin'; }
    virtual void SetRangeDword(uint min, uint max)
    {
        min_value = min;
        max_value = max;
        if (value < min_value)
            value = min_value;
        if (value > max_value)
            value = max_value;
        _snprintf(buffer, MENUITEMNUM_SIZE, "%d", value);
    }
    virtual void SetDword(uint value)
    {
        this->value = value;
        if (this->value < min_value)
            this->value = min_value;
        if (this->value > max_value)
            this->value = max_value;
        _snprintf(buffer, MENUITEMNUM_SIZE, "%d", this->value);
    }
    // The original kept another class's copy, the same code
    virtual uint GetDword() { return value; }
    virtual void Draw(uint draw_flags)
    {
        Rectangle(x, y, sx, sy, 0xff000000);
        RectangleFilled(x + sx - 10.0f, y, 10.0f, sy, 0xff000000, NULL);
        uint text_flags = 0;
        if (flags & COMPF_ACTIVE)
            text_flags |= OUTTEXT_UNDERLINE;
        if (flags & COMPF_TEXT_CENTER)
            text_flags |= OUTTEXT_CENTER;
        if (flags & COMPF_TEXT_RIGHT)
            text_flags |= OUTTEXT_RIGHT;
        if (draw_flags & COMPF_ENABLED)
        {
            if (flags & COMPF_TEXT_RED)
                mafiaIndicators.OutText(buffer, x, y, sx, sy, 0xffff0000, text_flags, flags & COMPF_FONT, NULL);
            else
                mafiaIndicators.OutText(buffer, x, y, sx, sy, 0xff000000, text_flags, flags & COMPF_FONT, NULL);
        }
        else
            mafiaIndicators.OutText(buffer, x, y, sx, sy, 0xff7f7f7f, text_flags, flags & COMPF_FONT, NULL);
    }
    virtual uint Mouse(int x, int y, int z, uint buttons, uint time)
    {
        if (z)
        {
            int v = value + z / 120;
            if (v < (int)min_value)
                v = min_value;
            if (v > max_value)
                v = max_value;
            value = v;
            _snprintf(buffer, MENUITEMNUM_SIZE, "%d", value);
            return id;
        }
        if (buttons & MENUMOUSE_LEFT_RELEASED)
        {
            if ((float)x > this->x + sx * .5f)
            {
                if (value < max_value)
                    value++;
            }
            else
            {
                if (value > min_value)
                    value--;
            }
            _snprintf(buffer, MENUITEMNUM_SIZE, "%d", value);
            return id;
        }
        return 0;
    }
    virtual uint Key(uint shift_keys, uint key)
    {
        if (key == DIK_SPACE || key == DIK_RETURN || key == DIK_NUMPADENTER)
        {
            if (value < max_value)
                value++;
            _snprintf(buffer, MENUITEMNUM_SIZE, "%d", value);
            return id;
        }
        else if (key == DIK_MINUS || key == DIK_SUBTRACT)
        {
            if (value > min_value)
                value--;
            _snprintf(buffer, MENUITEMNUM_SIZE, "%d", value);
            return id;
        }
        else if (key == DIK_EQUALS || key == DIK_ADD)
        {
            if (value < max_value)
                value++;
            _snprintf(buffer, MENUITEMNUM_SIZE, "%d", value);
            return id;
        }
        return 0;
    }

    // The names are ours (not min and max, windows.h's macros)
    uint min_value;                     // 0x40
    uint max_value;                     // 0x44
    uint value;                         // 0x48
    char buffer[MENUITEMNUM_SIZE];      // 0x4c
};

// The factory of a component's class not written yet (ours, not the original's): it registers the class's type as
// G_Factory does, but makes nothing, so a menu skips the item as it would without a factory, and logs the item through
// StubLog. The components' files have one for each such class, under the name of the original's factory. Its functions
// are in g_m_comps.cpp, out of line: declared alone, it leaves the other files that include this header as they were.
class G_StubFactory : public G_FactoryItem
{
public:
    G_StubFactory(uint type, const char *name);

    static GM_Component *Make(GM_DesignStruct *design, GM_Component *parent);
    static GM_Component *MakeDup(GM_Component *component);

    const char *name;                   // 0x10, the class's
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

// FUNCTION: GAME 0x5d92f0
// ?GetType@GM_Nahrada@@UAEIXZ

// FUNCTION: GAME 0x5d9300
// ?GetClientOrigin@GM_Nahrada@@UAEXAAM0@Z

// FUNCTION: GAME 0x5d9330
// ?GetType@GM_Component@@UAEIXZ

// FUNCTION: GAME 0x5d9340
// ?GetClientOrigin@GM_Component@@UAEXAAM0@Z

// FUNCTION: GAME 0x5d9360
// ?GetBool@GM_Component@@UAE_NXZ

// FUNCTION: GAME 0x5d9430
// ?GetType@GM_UpUp@@UAEIXZ

// FUNCTION: GAME 0x5d9440
// ?Draw@GM_UpUp@@UAEXI@Z

// FUNCTION: GAME 0x5d9530
// ?GetType@GM_Window@@UAEIXZ

// FUNCTION: GAME 0x5d9540
// ?GetClientOrigin@GM_Window@@UAEXAAM0@Z

// FUNCTION: GAME 0x5d9570
// ?Draw@GM_Window@@UAEXI@Z

// FUNCTION: GAME 0x5d9900
// ?GetType@GM_WindowClean@@UAEIXZ

// GM_WindowClean's copy, which GM_Window's folded into
// FUNCTION: GAME 0x5d9910
// ?CanBeParent@GM_ButtonClean@@UAE_NXZ

// FUNCTION: GAME 0x5d9a20
// ?GetType@GM_WindowScroll@@UAEIXZ

// FUNCTION: GAME 0x5d9a30
// ?GetClientOrigin@GM_WindowScroll@@UAEXAAM0@Z

// FUNCTION: GAME 0x5d9a50
// ?IsOverActive@GM_WindowScroll@@UAE_NMM@Z

// FUNCTION: GAME 0x5d9ab0
// ?SetFloat@GM_WindowScroll@@UAEXM@Z

// FUNCTION: GAME 0x5d9af0
// ?SetRangeFloat@GM_WindowScroll@@UAEXMM@Z

// FUNCTION: GAME 0x5d9b50
// ?GetFloat@GM_WindowScroll@@UAEMXZ

// FUNCTION: GAME 0x5d9b60
// ?GetRangeFloat@GM_WindowScroll@@UAEXAAM0@Z

// FUNCTION: GAME 0x5d9b80
// ?Draw@GM_WindowScroll@@UAEXI@Z

// FUNCTION: GAME 0x5d9d30
// ?Tick@GM_WindowScroll@@UAEXI@Z

// FUNCTION: GAME 0x5d9e10
// ?Mouse@GM_WindowScroll@@UAEIHHHII@Z

// FUNCTION: GAME 0x5d9f00
// ?Key@GM_WindowScroll@@UAEIII@Z

// FUNCTION: GAME 0x5d9f60
// ??1GM_WindowClean@@UAE@XZ

// FUNCTION: GAME 0x5da080
// ?GetType@GM_StaticText@@UAEIXZ

// FUNCTION: GAME 0x5da090
// ?Draw@GM_StaticText@@UAEXI@Z

// FUNCTION: GAME 0x5da1e0
// ?GetType@GM_StaticTextMem@@UAEIXZ

// FUNCTION: GAME 0x5da1f0
// ?SetText@GM_StaticTextMem@@UAEXPAE@Z

// FUNCTION: GAME 0x5da220
// ?Draw@GM_StaticTextMem@@UAEXI@Z

// FUNCTION: GAME 0x5da3a0
// ?GetType@GM_StaticTextNum@@UAEIXZ

// FUNCTION: GAME 0x5da3b0
// ?SetDword@GM_StaticTextNum@@UAEXI@Z

// FUNCTION: GAME 0x5da3f0
// ?Draw@GM_StaticTextTime@@UAEXI@Z

// FUNCTION: GAME 0x5da560
// ?GetType@GM_StaticTextTime@@UAEIXZ

// FUNCTION: GAME 0x5da570
// ?SetDword@GM_StaticTextTime@@UAEXI@Z

// FUNCTION: GAME 0x5da6c0
// ?GetType@GM_Line@@UAEIXZ

// FUNCTION: GAME 0x5da6d0
// ?Draw@GM_Line@@UAEXI@Z

// FUNCTION: GAME 0x5da7e0
// ?GetType@GM_Rectangle@@UAEIXZ

// FUNCTION: GAME 0x5da7f0
// ?Draw@GM_Rectangle@@UAEXI@Z

// FUNCTION: GAME 0x5da930
// ?GetType@GM_Orezavac@@UAEIXZ

// FUNCTION: GAME 0x5da940
// ?Draw@GM_Orezavac@@UAEXI@Z

// FUNCTION: GAME 0x5daa60
// ?GetType@GM_RectangleTexture@@UAEIXZ

// FUNCTION: GAME 0x5daa70
// ?SetString@GM_RectangleTexture@@UAEXPAE@Z

// FUNCTION: GAME 0x5daaa0
// ?Draw@GM_RectangleTexture@@UAEXI@Z

// SYNTHETIC: GAME 0x5daad0
// GM_RectangleTexture::`scalar deleting destructor'

// FUNCTION: GAME 0x5daaf0
// ??1GM_RectangleTexture@@UAE@XZ

// FUNCTION: GAME 0x5dac60
// ?GetType@GM_ProgressBar@@UAEIXZ

// FUNCTION: GAME 0x5dac70
// ?Draw@GM_ProgressBar@@UAEXI@Z

// FUNCTION: GAME 0x5dadb0
// ?GetType@GM_MenuItem@@UAEIXZ

// FUNCTION: GAME 0x5dadc0
// ?Draw@GM_MenuItem@@UAEXI@Z

// FUNCTION: GAME 0x5dae60
// ?Mouse@GM_MenuItem@@UAEIHHHII@Z

// FUNCTION: GAME 0x5daf50
// ?GetType@GM_MenuItemMem@@UAEIXZ

// The original kept another class's copy
// FUNCTION: GAME 0x5daf60
// ?Key@GM_MenuItemMem@@UAEIII@Z

// FUNCTION: GAME 0x5db0a0
// ?GetType@GM_MenuItemNum@@UAEIXZ

// FUNCTION: GAME 0x5db0b0
// ?SetRangeDword@GM_MenuItemNum@@UAEXII@Z

// FUNCTION: GAME 0x5db0f0
// ?SetDword@GM_MenuItemNum@@UAEXI@Z

// FUNCTION: GAME 0x5db130
// ?Draw@GM_MenuItemNum@@UAEXI@Z

// FUNCTION: GAME 0x5db2d0
// ?Mouse@GM_MenuItemNum@@UAEIHHHII@Z

// FUNCTION: GAME 0x5db370
// ?Key@GM_MenuItemNum@@UAEIII@Z

// FUNCTION: GAME 0x5db510
// ?GetType@GM_RaceRoundItem@@UAEIXZ

// FUNCTION: GAME 0x5db520
// ?SetBool@GM_RaceRoundItem@@UAEX_N@Z

// FUNCTION: GAME 0x5db560
// ?Draw@GM_RaceRoundItem@@UAEXI@Z

// FUNCTION: GAME 0x5db7a0
// ?GetType@GM_Button@@UAEIXZ

// FUNCTION: GAME 0x5db7b0
// ?Draw@GM_Button@@UAEXI@Z

// FUNCTION: GAME 0x5db960
// ?GetType@GM_ButtonClean@@UAEIXZ

// FUNCTION: GAME 0x5dba40
// ?GetType@GM_Slider@@UAEIXZ

// FUNCTION: GAME 0x5dba50
// ?SetFloat@GM_Slider@@UAEXM@Z

// FUNCTION: GAME 0x5dba60
// ?GetFloat@GM_Slider@@UAEMXZ

// FUNCTION: GAME 0x5dba70
// ?Draw@GM_Slider@@UAEXI@Z

// FUNCTION: GAME 0x5dbc90
// ?Mouse@GM_Slider@@UAEIHHHII@Z

// FUNCTION: GAME 0x5dbda0
// ?Key@GM_Slider@@UAEIII@Z

// FUNCTION: GAME 0x5dbf10
// ?GetType@GM_SliderInteger@@UAEIXZ

// FUNCTION: GAME 0x5dbf20
// ?SetDword@GM_SliderInteger@@UAEXI@Z

// FUNCTION: GAME 0x5dbf30
// ?Draw@GM_SliderInteger@@UAEXI@Z

// FUNCTION: GAME 0x5dc170
// ?Mouse@GM_SliderInteger@@UAEIHHHII@Z

// FUNCTION: GAME 0x5dc290
// ?Key@GM_SliderInteger@@UAEIII@Z

// FUNCTION: GAME 0x5dc450
// ?GetType@GM_NetPlayer@@UAEIXZ

// FUNCTION: GAME 0x5dc460
// ?SetPointer@GM_NetPlayer@@UAEXPAX@Z

// FUNCTION: GAME 0x5dc470
// ?GetPointer@GM_NetPlayer@@UAEPAXXZ

// FUNCTION: GAME 0x5dc480
// ?Draw@GM_NetPlayer@@UAEXI@Z

// FUNCTION: GAME 0x5dc700
// ?Mouse@GM_NetPlayer@@UAEIHHHII@Z

// FUNCTION: GAME 0x5dc850
// ?GetType@GM_TextEdit@@UAEIXZ

// FUNCTION: GAME 0x5dc860
// ?SetText@GM_TextEdit@@UAEXPAE@Z

// FUNCTION: GAME 0x5dc8a0
// ?GetText@GM_TextEdit@@UAEPAEXZ

// FUNCTION: GAME 0x5dc8b0
// ?SetDword@GM_TextEdit@@UAEXI@Z

// FUNCTION: GAME 0x5dc8f0
// ?Key@GM_TextEdit@@UAEIII@Z

// FUNCTION: GAME 0x5dc950
// ?Draw@GM_TextEdit@@UAEXI@Z

// FUNCTION: GAME 0x5dcae0
// ?GetType@GM_CheckBox@@UAEIXZ

// FUNCTION: GAME 0x5dcaf0
// ?Draw@GM_CheckBox@@UAEXI@Z

// FUNCTION: GAME 0x5dccc0
// ?SetBool@GM_CheckBox@@UAEX_N@Z

// FUNCTION: GAME 0x5dccd0
// ?GetBool@GM_CheckBox@@UAE_NXZ

// FUNCTION: GAME 0x5dcce0
// ?Key@GM_CheckBox@@UAEIII@Z

// FUNCTION: GAME 0x5dcd10
// ?Mouse@GM_CheckBox@@UAEIHHHII@Z

// FUNCTION: GAME 0x5dce20
// ?GetType@GM_ButtonSelect@@UAEIXZ

// FUNCTION: GAME 0x5dce30
// ?SetRangeDword@GM_SliderInteger@@UAEXII@Z

// FUNCTION: GAME 0x5dce50
// ?Mouse@GM_ButtonSelect@@UAEIHHHII@Z

// FUNCTION: GAME 0x5dce90
// ?Key@GM_ButtonSelect@@UAEIII@Z

// FUNCTION: GAME 0x5dcfc0
// ?GetType@GM_ButtonSelectBool@@UAEIXZ

// FUNCTION: GAME 0x5dcfd0
// ?SetRangeDword@GM_ButtonSelectBool@@UAEXII@Z

// FUNCTION: GAME 0x5dd000
// ?SetBool@GM_ButtonSelectBool@@UAEX_N@Z

// FUNCTION: GAME 0x5dd030
// ?GetBool@GM_ButtonSelectBool@@UAE_NXZ

// The original kept the copy of the yes or no selection ('bslb'), which draws the same, after its factory's Make
// FUNCTION: GAME 0x5dd040
// ?Draw@GM_ButtonSelect@@UAEXI@Z

// FUNCTION: GAME 0x5dd0d0
// ?Mouse@GM_ButtonSelectBool@@UAEIHHHII@Z

// FUNCTION: GAME 0x5dd120
// ?Key@GM_ButtonSelectBool@@UAEIII@Z

// FUNCTION: GAME 0x5dd290
// ?GetType@GM_EditNum@@UAEIXZ

// FUNCTION: GAME 0x5dd2a0
// ?SetRangeDword@GM_EditNum@@UAEXII@Z

// FUNCTION: GAME 0x5dd2e0
// ?SetDword@GM_EditNum@@UAEXI@Z

// FUNCTION: GAME 0x5dd320
// ?Draw@GM_EditNum@@UAEXI@Z

// FUNCTION: GAME 0x5dd420
// ?Mouse@GM_EditNum@@UAEIHHHII@Z

// FUNCTION: GAME 0x5dd4d0
// ?Key@GM_EditNum@@UAEIII@Z

// The original kept another class's copy, the list's ('lstc', g_m_comps2.cpp), which sets its member at 0x40 too
// FUNCTION: GAME 0x5df350
// ?SetFloat@GM_ProgressBar@@UAEXM@Z

// The original kept another class's copy in g_m_comps2.cpp's range, the same code (returns the member at 0x48)
// FUNCTION: GAME 0x5df370
// ?GetDword@GM_MenuItemNum@@UAEIXZ

// The original kept another class's copy in g_m_comps2.cpp's range ('vidc''s neighbour), the same code
// FUNCTION: GAME 0x5df8f0
// ?Draw@GM_Nahrada@@UAEXI@Z

// In the last file of components (g_m_compsm.cpp)
// SYNTHETIC: GAME 0x5e0580
// ??_EGM_EditNum@@UAEPAXI@Z

// The factories' functions
// TEMPLATE: GAME 0x5d90f0
// ?Make@?$G_Factory@VGM_Component@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5d9160
// ?MakeDup@?$G_Factory@VGM_Component@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5d91c0
// ?Make@?$G_Factory@VGM_Nahrada@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5d9370
// ?MakeDup@?$G_Factory@VGM_Nahrada@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5d93d0
// ?Make@?$G_Factory@VGM_UpUp@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5d9470
// ?MakeDup@?$G_Factory@VGM_UpUp@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5d94d0
// ?Make@?$G_Factory@VGM_Window@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5d9840
// ?MakeDup@?$G_Factory@VGM_Window@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5d98a0
// ?Make@?$G_Factory@VGM_WindowClean@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5d9920
// ?MakeDup@?$G_Factory@VGM_WindowClean@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5d9980
// ?Make@?$G_Factory@VGM_WindowScroll@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5d9f90
// ?MakeDup@?$G_Factory@VGM_WindowScroll@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5da020
// ?Make@?$G_Factory@VGM_StaticText@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5da120
// ?MakeDup@?$G_Factory@VGM_StaticText@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5da180
// ?Make@?$G_Factory@VGM_StaticTextMem@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5da2c0
// ?MakeDup@?$G_Factory@VGM_StaticTextMem@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5da320
// ?Make@?$G_Factory@VGM_StaticTextNum@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5da470
// ?MakeDup@?$G_Factory@VGM_StaticTextNum@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5da4e0
// ?Make@?$G_Factory@VGM_StaticTextTime@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5da5f0
// ?MakeDup@?$G_Factory@VGM_StaticTextTime@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5da660
// ?Make@?$G_Factory@VGM_Line@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5da720
// ?MakeDup@?$G_Factory@VGM_Line@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5da780
// ?Make@?$G_Factory@VGM_Rectangle@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5da870
// ?MakeDup@?$G_Factory@VGM_Rectangle@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5da8d0
// ?Make@?$G_Factory@VGM_Orezavac@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5da980
// ?MakeDup@?$G_Factory@VGM_Orezavac@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5da9e0
// ?Make@?$G_Factory@VGM_RectangleTexture@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5dab60
// ?MakeDup@?$G_Factory@VGM_RectangleTexture@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5dabe0
// ?Make@?$G_Factory@VGM_ProgressBar@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5dacc0
// ?MakeDup@?$G_Factory@VGM_ProgressBar@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5dad40
// ?Make@?$G_Factory@VGM_MenuItem@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5dae80
// ?MakeDup@?$G_Factory@VGM_MenuItem@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5daee0
// ?Make@?$G_Factory@VGM_MenuItemMem@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5daf90
// ?MakeDup@?$G_Factory@VGM_MenuItemMem@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5daff0
// ?Make@?$G_Factory@VGM_MenuItemNum@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5db3e0
// ?MakeDup@?$G_Factory@VGM_MenuItemNum@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5db480
// ?Make@?$G_Factory@VGM_RaceRoundItem@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5db6b0
// ?MakeDup@?$G_Factory@VGM_RaceRoundItem@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5db730
// ?Make@?$G_Factory@VGM_Button@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5db890
// ?MakeDup@?$G_Factory@VGM_Button@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5db8f0
// ?Make@?$G_Factory@VGM_ButtonClean@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5db970
// ?MakeDup@?$G_Factory@VGM_ButtonClean@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5db9d0
// ?Make@?$G_Factory@VGM_Slider@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5dbe20
// ?MakeDup@?$G_Factory@VGM_Slider@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5dbe80
// ?Make@?$G_Factory@VGM_SliderInteger@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5dc300
// ?MakeDup@?$G_Factory@VGM_SliderInteger@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5dc370
// ?Make@?$G_Factory@VGM_NetPlayer@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5dc740
// ?MakeDup@?$G_Factory@VGM_NetPlayer@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5dc7a0
// ?Make@?$G_Factory@VGM_TextEdit@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5dc9d0
// ?MakeDup@?$G_Factory@VGM_TextEdit@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5dca60
// ?Make@?$G_Factory@VGM_CheckBox@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5dcd50
// ?MakeDup@?$G_Factory@VGM_CheckBox@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5dcdb0
// ?Make@?$G_Factory@VGM_ButtonSelect@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5dcee0
// ?MakeDup@?$G_Factory@VGM_ButtonSelect@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5dcf40
// ?Make@?$G_Factory@VGM_ButtonSelectBool@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5dd180
// ?MakeDup@?$G_Factory@VGM_ButtonSelectBool@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5dd1e0
// ?Make@?$G_Factory@VGM_EditNum@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5dd590
// ?MakeDup@?$G_Factory@VGM_EditNum@@@@SIPAVGM_Component@@PAV2@@Z
