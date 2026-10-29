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

// A button (PS2's), 'butt': its text, which clicks as a menu item's does, underlined while active. Red in the alternate
// color, otherwise white in the alternate style and black in the normal one; while disabled dark red or grey. It can be
// activated.
// VTABLE: GAME 0x626900
class GM_Button : public GM_Component
{
public:
    GM_Button(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        flags |= COMPF_UNKNOWN_8000;
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
        flags |= COMPF_UNKNOWN_8000;
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
// ?IsOverClientArea@GM_Component@@UAE_NMM@Z

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

// FUNCTION: GAME 0x5d9900
// ?GetType@GM_WindowClean@@UAEIXZ

// GM_WindowClean's copy, which GM_Window's folded into
// FUNCTION: GAME 0x5d9910
// ?CanBeParent@GM_Window@@UAE_NXZ

// FUNCTION: GAME 0x5d9f60
// ??1GM_ProgressBar@@UAE@XZ

// FUNCTION: GAME 0x5da080
// ?GetType@GM_StaticText@@UAEIXZ

// FUNCTION: GAME 0x5da090
// ?Draw@GM_StaticText@@UAEXI@Z

// FUNCTION: GAME 0x5da7e0
// ?GetType@GM_Rectangle@@UAEIXZ

// FUNCTION: GAME 0x5da7f0
// ?Draw@GM_Rectangle@@UAEXI@Z

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
// ?Mouse@GM_Button@@UAEIHHHII@Z

// The original kept another class's copy
// FUNCTION: GAME 0x5daf60
// ?Key@GM_Button@@UAEIII@Z

// FUNCTION: GAME 0x5db7a0
// ?GetType@GM_Button@@UAEIXZ

// FUNCTION: GAME 0x5db7b0
// ?Draw@GM_Button@@UAEXI@Z

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

// The original kept another class's copy, the list's ('lstc', g_m_comps2.cpp), which sets its member at 0x40 too
// FUNCTION: GAME 0x5df350
// ?SetFloat@GM_ProgressBar@@UAEXM@Z

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

// TEMPLATE: GAME 0x5d98a0
// ?Make@?$G_Factory@VGM_WindowClean@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5d9920
// ?MakeDup@?$G_Factory@VGM_WindowClean@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5da020
// ?Make@?$G_Factory@VGM_StaticText@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5da120
// ?MakeDup@?$G_Factory@VGM_StaticText@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5da780
// ?Make@?$G_Factory@VGM_Rectangle@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5da870
// ?MakeDup@?$G_Factory@VGM_Rectangle@@@@SIPAVGM_Component@@PAV2@@Z

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

// TEMPLATE: GAME 0x5db730
// ?Make@?$G_Factory@VGM_Button@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5db890
// ?MakeDup@?$G_Factory@VGM_Button@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5dc7a0
// ?Make@?$G_Factory@VGM_TextEdit@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5dc9d0
// ?MakeDup@?$G_Factory@VGM_TextEdit@@@@SIPAVGM_Component@@PAV2@@Z
