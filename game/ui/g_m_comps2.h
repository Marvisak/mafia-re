#pragma once

// The classes of g_m_comps2.cpp's components: the PC's own ('vidc') with the two menus it opens, then PS2's in its order
// (the combo box with its list's menu and window, the list box, the list of cars, the view scroll, the text input and
// the big text). In a header of their own, which only their file includes: the profile's controls they read stay out
// of the other components' files.

#include "ui/g_m_comps.h"
#include "ui/g_loadsave.h"
#include "input/controls.h"

#include <stdio.h>
#include <string.h>

// The menus' items, tables\menu\waitforvid.mnu and setcontroltovid.mnu (the names are ours, after their texts)
#define WAITFORVID_CONTROL 0xc          // the control's label
#define SETCONTROLTOVID_CONTROL 0xc     // under "Already used by:", the other control's label
#define SETCONTROLTOVID_YES 0x64
#define SETCONTROLTOVID_NO 0x65

// The question whether to take a binding from the control that has it, "setcontroltovid" (PC only, the name is ours,
// after its menu's), 0x44 bytes, which the waiting for a new binding opens: Yes binds it to the control, unbinding the
// others that have it, No (or Escape) leaves the bindings as they were. Its constructor is inline: the binding is
// copied in its body, after the vtable.
// VTABLE: GAME 0x626eb0
class GM_SetControlToVid : public GM_Menu
{
public:
    GM_SetControlToVid(int control, int other, S_control binding) : control(control), other(other)
    {
        this->binding = binding;
    }

    virtual const char *OnBeforeCreate()
    {
        // STRING: GAME 0x6381b4
        return "setcontroltovid";
    }
    virtual int OnCreate()
    {
        if (other >= 0)
            SetTextID(SETCONTROLTOVID_CONTROL, mafiaLoadSave.GetControlTextID(other));
        return 0;
    }
    // Escape clicks No
    virtual int OnKey(uchar key, uint)
    {
        return key == DIK_ESCAPE ? SETCONTROLTOVID_NO : 0;
    }
    // Yes binds as GM_WaitForVid::OnTick does where no other control has the binding
    virtual int OnClick(uint id)
    {
        switch (id)
        {
        case SETCONTROLTOVID_YES:
            switch (control)
            {
            case CTRL_AIM:
            case CTRL_AIM + CTRL_SECONDARY:
                binding.half = CONTROL_AXIS_FULL;
                break;
            case CTRL_CLUTCH + CTRL_SECONDARY:
                mafiaLoadSave.RemoveBinding(control, &binding, true);
                binding.half = CONTROL_AXIS_PEDAL;
                break;
            case CTRL_LEFT + CTRL_SECONDARY:
                mafiaLoadSave.SetOppositeBinding(CTRL_RIGHT + CTRL_SECONDARY, &binding);
                break;
            case CTRL_RIGHT + CTRL_SECONDARY:
                mafiaLoadSave.SetOppositeBinding(CTRL_LEFT + CTRL_SECONDARY, &binding);
                break;
            case CTRL_STEER_LEFT + CTRL_SECONDARY:
                mafiaLoadSave.SetOppositeBinding(CTRL_STEER_RIGHT + CTRL_SECONDARY, &binding);
                break;
            case CTRL_STEER_RIGHT + CTRL_SECONDARY:
                mafiaLoadSave.SetOppositeBinding(CTRL_STEER_LEFT + CTRL_SECONDARY, &binding);
                break;
            }
            mafiaLoadSave.RemoveBinding(control, &binding, false);
            mafiaLoadSave.bindings[control] = binding;
            MenuClose((uint)-1);
            break;
        case SETCONTROLTOVID_NO:
            MenuClose((uint)-1);
            break;
        }
        return 0;
    }

    int control;                        // 0x30, the index of the binding it sets (the names are ours)
    int other;                          // 0x34, the other control that has the binding
    S_control binding;                  // 0x38
};

// The menu that waits for a control's new key, button or axis, "waitforvid" (PC only, the name is ours, after its
// menu's), 0x38 bytes, which a control's binding (GM_Vidc) opens. Its OnTick reads the input and sets the binding,
// asking in the menu "setcontroltovid" whether to take it from another control that has it. Its constructor is inline.
// VTABLE: GAME 0x626e80
class GM_WaitForVid : public GM_Menu
{
public:
    GM_WaitForVid(int control) : control(control), axes_only(false) {}

    virtual const char *OnBeforeCreate()
    {
        // STRING: GAME 0x6381a8
        return "waitforvid";
    }
    // The question's answer closes this menu too, with its code
    virtual int OnCloseChild(GM_Menu *child, uint, uint code)
    {
        MenuClose(code);
        return 0;
    }
    // The control's label into the window, and the joysticks' state the axes are measured from
    virtual int OnCreate()
    {
        SetTextID(WAITFORVID_CONTROL, mafiaLoadSave.GetControlTextID(control));
        controls.SaveJoyStates();
        return 0;
    }
    // The bindings take their controls' names again
    virtual int OnClose()
    {
        if (parent)
            parent->ForceUpdate();
        return 0;
    }
    // The input of a frame, if there is one: the aim takes the mouse's axes too, the others not. Escape closes the
    // menu. A binding another control of the group has (the player's or the car's; some of the player's in both) is
    // asked about; otherwise the control takes it: the aim the whole axis, the secondary clutch a pedal (with every
    // other control of that axis unbound), the secondary left and right and steering an axis's half, the opposite control
    // the other half. Then IGraph reads its messages and keys again, so the menu doesn't take the key.
    virtual int OnTick(uint time)
    {
        S_control *input;
        if (axes_only)
        {
            if (control == CTRL_AIM || control == CTRL_AIM + CTRL_SECONDARY)
                input = controls.GetLastInput(LASTINPUT_NO_KEYS | LASTINPUT_NO_POVS | LASTINPUT_NO_JOY_BUTTONS |
                    LASTINPUT_NO_MOUSE_BUTTONS, false);
            else
                input = controls.GetLastInput(LASTINPUT_NO_KEYS | LASTINPUT_NO_POVS | LASTINPUT_NO_JOY_BUTTONS |
                    LASTINPUT_NO_MOUSE_BUTTONS | LASTINPUT_NO_MOUSE_AXES, false);
        }
        else
        {
            if (control == CTRL_AIM || control == CTRL_AIM + CTRL_SECONDARY)
                input = controls.GetLastInput(0, false);
            else
                input = controls.GetLastInput(LASTINPUT_NO_MOUSE_AXES, false);
        }
        if (!input)
            return 0;
        if (input->device == CONTROL_KEY && !input->code)
            return 0;
        int other;
        switch (control)
        {
        case CTRL_AIM:
        case CTRL_AIM + CTRL_SECONDARY:
        case CTRL_CLUTCH + CTRL_SECONDARY:
            other = mafiaLoadSave.FindBinding(control, input, true, -1);
            break;
        case CTRL_LEFT + CTRL_SECONDARY:
            other = mafiaLoadSave.FindBinding(CTRL_LEFT + CTRL_SECONDARY, input, true, CTRL_RIGHT + CTRL_SECONDARY);
            break;
        case CTRL_RIGHT + CTRL_SECONDARY:
            other = mafiaLoadSave.FindBinding(CTRL_RIGHT + CTRL_SECONDARY, input, true, CTRL_LEFT + CTRL_SECONDARY);
            break;
        case CTRL_STEER_LEFT + CTRL_SECONDARY:
            other = mafiaLoadSave.FindBinding(CTRL_STEER_LEFT + CTRL_SECONDARY, input, true, CTRL_STEER_RIGHT + CTRL_SECONDARY);
            break;
        case CTRL_STEER_RIGHT + CTRL_SECONDARY:
            other = mafiaLoadSave.FindBinding(CTRL_STEER_RIGHT + CTRL_SECONDARY, input, true, CTRL_STEER_LEFT + CTRL_SECONDARY);
            break;
        case CTRL_INVENTORY:
        case CTRL_HIDE_WEAPON:
        case CTRL_DROP_WEAPON:
        case CTRL_RELOAD:
        case CTRL_OBJECTIVES:
        case CTRL_MAP:
            other = mafiaLoadSave.FindBinding(control, input, false, NUM_CONTROLS);
            break;
        default:
            other = mafiaLoadSave.FindBinding(control, input, false, -1);
            break;
        }
        if (other >= 0)
        {
            if (input->device == CONTROL_KEY && input->code == DIK_ESCAPE)
            {
                MenuClose((uint)-1);
                return 0;
            }
            MenuOpen(new GM_SetControlToVid(control, other, *input), open_id);
        }
        else
        {
            switch (control)
            {
            case CTRL_AIM:
            case CTRL_AIM + CTRL_SECONDARY:
                input->half = CONTROL_AXIS_FULL;
                break;
            case CTRL_CLUTCH + CTRL_SECONDARY:
                mafiaLoadSave.RemoveBinding(control, input, true);
                input->half = CONTROL_AXIS_PEDAL;
                break;
            case CTRL_LEFT + CTRL_SECONDARY:
                mafiaLoadSave.SetOppositeBinding(CTRL_RIGHT + CTRL_SECONDARY, input);
                break;
            case CTRL_RIGHT + CTRL_SECONDARY:
                mafiaLoadSave.SetOppositeBinding(CTRL_LEFT + CTRL_SECONDARY, input);
                break;
            case CTRL_STEER_LEFT + CTRL_SECONDARY:
                mafiaLoadSave.SetOppositeBinding(CTRL_STEER_RIGHT + CTRL_SECONDARY, input);
                break;
            case CTRL_STEER_RIGHT + CTRL_SECONDARY:
                mafiaLoadSave.SetOppositeBinding(CTRL_STEER_LEFT + CTRL_SECONDARY, input);
                break;
            }
            if (input->device == CONTROL_KEY && input->code == DIK_ESCAPE)
            {
                MenuClose((uint)-1);
                return 0;
            }
            mafiaLoadSave.RemoveBinding(control, input, false);
            mafiaLoadSave.bindings[control] = *input;
            MenuClose((uint)-1);
        }
        igraph->ProcessWinMessages();
        igraph->UpdateKeyboardData();
        return 0;
    }

    int control;                        // 0x30, the index of the binding it sets (the names are ours)
    bool axes_only;                     // 0x34, OnTick takes no keys or buttons then (nothing sets it)
};

// A control's binding (PC only, the name is ours, after its type), 'vidc': its text the name of the key, button or
// axis bound to the control of its text's id, the controls menu's pages' (optionsc1 and optionsc2), and of a joystick's
// the joystick's number and its axis's half. A black outline, the text indented; while active it is underlined, with a
// red arrow. The mouse's left button, Space, Enter and the keypad's Enter open the menu that waits for a new
// binding, Backspace unbinds it. It can be activated.
// VTABLE: GAME 0x626e00
class GM_Vidc : public GM_Component
{
public:
    GM_Vidc(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        flags |= COMPF_ACTIVATABLE;
        ForceUpdate();
    }
    GM_Vidc(GM_Component *component) : GM_Component(component)
    {
        ForceUpdate();
    }

    virtual uint GetType() { return 'vidc'; }
    // The binding's name into the text. The device is tested through the accessor: written out, VC6 shares the binding's
    // offset with the next read, where the original computes it again. The text is set in each branch, which keeps
    // the joystick's branch a region of its own for the register it saves.
    virtual void ForceUpdate()
    {
        buffer[0] = 0;
        if (mafiaLoadSave.GetBinding(text_id).device == CONTROL_JOY)
        {
            // STRING: GAME 0x63819c
            sprintf(buffer, "Joy%d / ", mafiaLoadSave.bindings[text_id].joy);
            const char *name = controls.GetControlName(&mafiaLoadSave.bindings[text_id]);
            if (name)
                strcat(buffer, name);
            else
                // STRING: GAME 0x638194
                strcat(buffer, "Unknown");
            switch (mafiaLoadSave.bindings[text_id].half)
            {
            case CONTROL_AXIS_POSITIVE:
                // STRING: GAME 0x638184
                strcat(buffer, " / +");
                break;
            case CONTROL_AXIS_NEGATIVE:
                // STRING: GAME 0x63818c
                strcat(buffer, " / -");
                break;
            }
            text = (uchar *)buffer;
        }
        else
        {
            const char *name = controls.GetControlName(&mafiaLoadSave.bindings[text_id]);
            if (name)
                strncpy(buffer, name, sizeof(buffer));
            else
                strcpy(buffer, "");
            text = (uchar *)buffer;
        }
    }
    virtual void Draw(uint draw_flags)
    {
        if (flags & COMPF_ACTIVE)
        {
            Rectangle(x, y, sx, sy, 0xff000000);
            mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xff000000,
                OUTTEXT_UNDERLINE | OUTTEXT_INDENT | OUTTEXT_FIT, 0, NULL);
            Triangle(x + sy / 8, y + sy * (1.0f / 6), sy * (1.0f / 3), sy * (2.0f / 3), TRIANGLE_RIGHT, 0xffbe002c);
        }
        else
        {
            Rectangle(x, y, sx, sy, 0xff000000);
            mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xff000000, OUTTEXT_INDENT | OUTTEXT_FIT, 0, NULL);
        }
    }
    // Never clicks: the menu it opens sets the binding
    virtual uint Mouse(int x, int y, int z, uint buttons, uint time)
    {
        if (buttons & MENUMOUSE_LEFT_RELEASED)
            GM_Menu::activeMenuWindow->MenuOpen(new GM_WaitForVid(text_id), id);
        return 0;
    }
    // The binding's fields are assigned in the order that gives the original's stores
    virtual uint Key(uint shift_keys, uint key)
    {
        switch (key)
        {
        case DIK_BACK:
            {
                S_control none;
                none.device = CONTROL_NONE;
                none.code = 0;
                none.joy = 0;
                none.half = CONTROL_AXIS_FULL;
                mafiaLoadSave.bindings[text_id] = none;
            }
            ForceUpdate();
            break;
        case DIK_SPACE:
        case DIK_RETURN:
        case DIK_NUMPADENTER:
            GM_Menu::activeMenuWindow->MenuOpen(new GM_WaitForVid(text_id), id);
            break;
        }
        return 0;
    }

    char buffer[0x80];                  // 0x40, the text (the name is ours)
};

// A combo box's item (PS2's name; the members' names are ours)
struct GM_ComboBoxStruct
{
    uchar *text;
    uint value;                         // 0x4, the combo box's dword while it is selected
};

// The menu of a combo box's list, tables\menu\combomenu.mnu (the names are ours)
#define COMBOMENU_WINDOW 1              // the list's window ('cmbw')
#define COMBOMENU_BACKGROUND 2          // a clean button over the whole screen
#define COMBOMENU_ITEM 1000             // the first item ('item'), which the others are copies of
#define COMBOMENU_ITEM_END 2000         // the items' ids are below it

// A combo box's list (PS2's GM_ComboMenu), "combomenu", 0x38 bytes: a window under the combo box and as wide, with a
// menu item for each of the box's items. A click on one selects it in the box (SetDword), a click beside the window
// closes the menu; either way the box gets clicked (OnClose). Its constructor is inline: both members are assigned in
// its body, after the vtable.
// VTABLE: GAME 0x626f5c
class GM_ComboMenu : public GM_Menu
{
public:
    GM_ComboMenu(GM_Component *combo)
    {
        this->combo = combo;
        items = (std::vector<GM_ComboBoxStruct> *)combo->GetPointer();
    }

    virtual const char *OnBeforeCreate()
    {
        // STRING: GAME 0x6381c4
        return "combomenu";
    }
    // The background over the screen, the window under the combo box, and the items, the first one's copies each below
    // the one before, as wide as the box less the first one's indent on both sides, each with its item's text. Without
    // items the first one is hidden.
    virtual int OnCreate()
    {
        GM_Component *c = FindComponentByID(COMBOMENU_BACKGROUND);
        if (c && combo)
        {
            c->rel_x = 0.0f;
            c->rel_y = 0.0f;
            c->sx = (float)igraph->Scrn_sx();
            c->sy = (float)igraph->Scrn_sy();
        }
        c = FindComponentByID(COMBOMENU_WINDOW);
        if (c && combo)
        {
            c->rel_x = combo->x;
            c->rel_y = combo->y + combo->sy;
            c->sx = combo->sx;
        }
        c = FindComponentByID(COMBOMENU_ITEM);
        if (items && items->size() && c)
        {
            c->sx = combo->sx - c->x * 2;
            float y = c->y + c->sy;
            uint i;
            for (i = 1; i < items->size(); i++)
                y = PridejComponent(y, COMBOMENU_ITEM + i - 1, COMBOMENU_ITEM + i, COMBOMENU_WINDOW, 0, 0, 0);
            for (i = 0; i < items->size(); i++)
                SetText(COMBOMENU_ITEM + i, (*items)[i].text);
        }
        else
            SetHidden(COMBOMENU_ITEM, false);
        return 0;
    }
    // The combo box gets clicked
    virtual int OnClose()
    {
        if (parent)
            parent->OnClick(open_id);
        return 0;
    }
    virtual int OnClick(uint id)
    {
        if (id == COMBOMENU_BACKGROUND)
            MenuClose((uint)-1);
        if (items && items->size() && id >= COMBOMENU_ITEM && id < COMBOMENU_ITEM_END)
        {
            combo->SetDword((*items)[id - COMBOMENU_ITEM].value);
            MenuClose((uint)-1);
        }
        return 0;
    }

    GM_Component *combo;                // 0x30, the combo box (the names are ours)
    std::vector<GM_ComboBoxStruct> *items; // 0x34, its items
};

// A combo box (PS2's), 'comb': the selected one of its items, which its pointer gives (a vector of GM_ComboBoxStruct,
// the caller's), its text in a grey box with a button at the right end, whose arrow points down, or up while the box's
// list is open over the menu; the outlines black, the arrow red while the box is active. Its dword is the selected
// item's value, -1 for none, its text the item's text. The mouse's left button released over it, Space and the Enters
// open its list (GM_ComboMenu). It can be activated.
// VTABLE: GAME 0x626ee0
class GM_ComboBox : public GM_Component
{
public:
    GM_ComboBox(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        flags |= COMPF_ACTIVATABLE;
        items = NULL;
        selected = NULL;
    }
    GM_ComboBox(GM_Component *component) : GM_Component(component)
    {
        items = NULL;
        selected = NULL;
    }

    virtual uint GetType() { return 'comb'; }
    // The items, the first one selected
    virtual void SetPointer(void *value)
    {
        items = (std::vector<GM_ComboBoxStruct> *)value;
        if (items && items->size())
            selected = items->begin();
        else
            selected = NULL;
    }
    // The original kept the list box's copy, the same code
    virtual void *GetPointer() { return items; }
    // Selects the item of that value, none where no item has it
    virtual void SetDword(uint value)
    {
        std::vector<GM_ComboBoxStruct>::iterator it;
        for (it = items->begin(); it < items->end(); it++)
            if (it->value == value)
            {
                selected = it;
                break;
            }
        if (it >= items->end())
            selected = NULL;
    }
    virtual uint GetDword() { return selected ? selected->value : (uint)-1; }
    virtual uchar *GetText() { return selected ? selected->text : NULL; }
    virtual void Draw(uint draw_flags)
    {
        uint direction = TRIANGLE_DOWN;
        if (!(draw_flags & MENUDRAW_ACTIVE_MENU) && GM_Menu::activeMenuWindow->open_id == id)
            direction = TRIANGLE_UP;
        RectangleFilled(x, y, sx, sy, 0x7f7f7f7f, NULL);
        if (flags & COMPF_ACTIVE)
        {
            Rectangle(x, y, sx, sy, 0xff000000);
            Rectangle(x + sx - sy, y, sy, sy, 0xff000000);
            float size = sy * .5f + 1.0f;
            Triangle(x + sx - sy + sy * .25f, y + sy * .25f, size, size, direction, 0xffbe002c);
            if (selected)
                mafiaIndicators.OutText((char *)selected->text, x, y, sx, sy, 0xff000000, OUTTEXT_INDENT | OUTTEXT_FIT,
                    0, NULL);
        }
        else
        {
            Rectangle(x, y, sx, sy, 0xff000000);
            Rectangle(x + sx - sy, y, sy, sy, 0xff000000);
            float size = sy * .5f + 1.0f;
            Triangle(x + sx - sy + sy * .25f, y + sy * .25f, size, size, direction, 0xff000000);
            if (selected)
                mafiaIndicators.OutText((char *)selected->text, x, y, sx, sy, 0xff000000, OUTTEXT_INDENT | OUTTEXT_FIT,
                    0, NULL);
        }
    }
    // Never clicks: the list clicks it as it closes
    virtual uint Mouse(int x, int y, int z, uint buttons, uint time)
    {
        if (buttons & MENUMOUSE_LEFT_RELEASED)
        {
            if (IsOverActive((float)x, (float)y))
                GM_Menu::activeMenuWindow->MenuOpen(new GM_ComboMenu(this), id);
        }
        return 0;
    }
    virtual uint Key(uint shift_keys, uint key)
    {
        if (key == DIK_SPACE || key == DIK_RETURN || key == DIK_NUMPADENTER)
            GM_Menu::activeMenuWindow->MenuOpen(new GM_ComboMenu(this), id);
        return 0;
    }

    std::vector<GM_ComboBoxStruct> *items; // 0x40 (the names are ours)
    GM_ComboBoxStruct *selected;        // 0x44, NULL for none
};

// A combo box's list window (PS2's), 'cmbw': the menus' bottom texture tinted blue, a parent whose children are placed
// from its position. Its Draw sets the viewport to it, for the items.
// VTABLE: GAME 0x626f90
class GM_ComboWindow : public GM_Component
{
public:
    GM_ComboWindow(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent) {}
    GM_ComboWindow(GM_Component *component) : GM_Component(component) {}

    virtual uint GetType() { return 'cmbw'; }
    // The original kept GM_WindowClean's copy
    virtual bool CanBeParent() { return true; }
    virtual void Draw(uint draw_flags)
    {
        RectangleFilled(x, y, sx, sy, 0xffafafff, GM_Menu::textureMenuBottom);
        SetViewport((int)x, (int)y, (int)sx, (int)sy);
    }
};

// A list box's item (PS2's name; the members' names are ours)
struct GM_ListBoxStruct
{
    uchar *text;
    uint unknown_4;                     // nothing here reads it
    bool selected;                      // 0x8, a click toggles it
    uchar unknown_9[3];
};

// A list box's row, on the screen as it is (the name is ours)
#define LISTBOX_ROW_HEIGHT 15

// A list box (PS2's), 'list': its items, which its pointer gives (a vector of GM_ListBoxStruct, the caller's), a
// LISTBOX_ROW_HEIGHT row each, black, the selected ones on blue, with a scroll bar along its right edge as the scrolled
// window's but of SCROLL_BAR_WIDTH unscaled: black outlines and a grey thumb, red while active. Its value is the scroll,
// 0 to the rows' height less its own. The mouse's left button pressed left of the bar toggles the item under it, on the
// bar it captures the mouse, which then moves the thumb's middle to it while the button is down; Up and Down scroll by
// 10. It can be activated.
// VTABLE: GAME 0x627010
class GM_ListBox : public GM_Component
{
public:
    GM_ListBox(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        flags |= COMPF_ACTIVATABLE;
        scroll = 0.0f;
        scroll_max = 100.0f;
        items = NULL;
    }
    GM_ListBox(GM_Component *component) : GM_Component(component)
    {
        scroll = 0.0f;
        scroll_max = 100.0f;
        items = NULL;
    }

    virtual uint GetType() { return 'list'; }
    // The items, and the range for their rows
    virtual void SetPointer(void *value)
    {
        items = (std::vector<GM_ListBoxStruct> *)value;
        if (items)
            scroll_max = (float)(items->size() * LISTBOX_ROW_HEIGHT) - sy;
        else
            scroll_max = 0.0f;
        if (scroll_max < 0.0f)
            scroll_max = 0.0f;
        if (scroll > scroll_max)
            scroll = scroll_max;
    }
    virtual void *GetPointer() { return items; }
    // The bar, then the rows inside the box left of it (the viewport set there), each text 10 wide
    virtual void Draw(uint draw_flags)
    {
        float thumb_y;
        if (scroll_max > 0.0f)
            thumb_y = scroll / scroll_max * (sy - SCROLL_BAR_WIDTH * 3);
        else
            thumb_y = 0.0f;
        if (flags & COMPF_ACTIVE)
            RectangleFilled(x + sx - SCROLL_BAR_WIDTH, y + thumb_y + SCROLL_BAR_WIDTH, SCROLL_BAR_WIDTH, SCROLL_BAR_WIDTH,
                0xffff0000, NULL);
        else
            RectangleFilled(x + sx - SCROLL_BAR_WIDTH, y + thumb_y + SCROLL_BAR_WIDTH, SCROLL_BAR_WIDTH, SCROLL_BAR_WIDTH,
                0xff7f7f7f, NULL);
        Rectangle(x, y, sx, sy, 0xff000000);
        Rectangle(x + sx - SCROLL_BAR_WIDTH, y, SCROLL_BAR_WIDTH, sy, 0xff000000);
        Rectangle(x + sx - SCROLL_BAR_WIDTH, y + SCROLL_BAR_WIDTH, SCROLL_BAR_WIDTH, sy - SCROLL_BAR_WIDTH * 2,
            0xff000000);
        SetViewport((int)(x + 1.0f), (int)(y + 1.0f), (int)(sx - (SCROLL_BAR_WIDTH + 1)), (int)(sy - 1.0f));
        if (items)
        {
            float row_y = y - scroll;
            for (std::vector<GM_ListBoxStruct>::iterator it = items->begin(); it < items->end(); it++)
            {
                if (it->selected)
                    RectangleFilled(x, row_y, sx, LISTBOX_ROW_HEIGHT, 0x7f0000ff, NULL);
                mafiaIndicators.OutText((char *)it->text, x, row_y, 10.0f, LISTBOX_ROW_HEIGHT, 0xff000000, 0, 0, NULL);
                row_y += LISTBOX_ROW_HEIGHT;
            }
        }
        SetViewport(0, 0, igraph->Scrn_sx(), igraph->Scrn_sy());
    }
    // Never clicks
    virtual uint Mouse(int x, int y, int z, uint buttons, uint time)
    {
        if (buttons & MENUMOUSE_LEFT_PRESSED)
        {
            if (x > this->x + sx - SCROLL_BAR_WIDTH)
                flags |= COMPF_MOUSE_CAPTURE;
            else
            {
                y -= this->y;
                y += scroll;
                int row = y / LISTBOX_ROW_HEIGHT;
                if (items && row < items->size())
                    (*items)[row].selected = !(*items)[row].selected;
            }
        }
        if (buttons & MENUMOUSE_LEFT && flags & COMPF_MOUSE_CAPTURE)
        {
            scroll = (y - this->y - SCROLL_BAR_WIDTH * 1.5f) / (sy - SCROLL_BAR_WIDTH * 3) * scroll_max;
            if (scroll < 0.0f)
                scroll = 0.0f;
            if (scroll > scroll_max)
                scroll = scroll_max;
        }
        if (buttons & MENUMOUSE_LEFT_RELEASED)
            flags &= ~COMPF_MOUSE_CAPTURE;
        return 0;
    }
    // Never clicks
    virtual uint Key(uint shift_keys, uint key)
    {
        if (key == DIK_UP)
            scroll -= 10.0f;
        else if (key == DIK_DOWN)
            scroll += 10.0f;
        if (scroll < 0.0f)
            scroll = 0.0f;
        if (scroll > scroll_max)
            scroll = scroll_max;
        return 0;
    }

    std::vector<GM_ListBoxStruct> *items; // 0x40 (the names are ours)
    float scroll_max;                   // 0x44, the range's maximum (not max, windows.h's macro)
    float scroll;                       // 0x48
};

// A list's row on the 800x600 screen (the name is ours)
#define LISTCARS_ROW_HEIGHT 20.0f

// The list of cars (PS2's GM_ListCars), 'lstc': its texts, which its pointer gives (a vector of the texts, the
// caller's), a row each, scaled to the screen; white, the selected one red while the list is active, with a red arrow
// left of the third row. The list scrolls so that the selected row is the third: the target row moves first, the
// scroll follows it at 10 rows a second (Tick), and the row is selected once it's there. Its dword is the selected
// row; the mouse's left button pressed over a row makes it the target, the wheel moves the target a row for each 100
// of its delta and clicks. It can be activated.
// VTABLE: GAME 0x627090
class GM_ListCars : public GM_Component
{
public:
    GM_ListCars(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        flags |= COMPF_ACTIVATABLE;
        items = NULL;
        target = 0;
        selected = 0;
        scroll = 0.0f;
        row_height = GM_Menu::menuScreenScale.y * LISTCARS_ROW_HEIGHT;
    }
    GM_ListCars(GM_Component *component) : GM_Component(component)
    {
        items = NULL;
        target = 0;
        selected = 0;
        scroll = 0.0f;
        row_height = GM_Menu::menuScreenScale.y * LISTCARS_ROW_HEIGHT;
    }

    virtual uint GetType() { return 'lstc'; }
    // The original kept the progress bar's copy, the same code
    virtual void SetPointer(void *value) { items = (std::vector<uchar *> *)value; }
    // The row, selected at once
    virtual void SetDword(uint value)
    {
        target = value;
        selected = value;
    }
    // The original kept another class's copy, the same code
    virtual uint GetDword() { return selected; }
    virtual void Tick(uint time)
    {
        float target_scroll = target * row_height;
        float seconds = time * .001f;
        if (scroll > target_scroll)
        {
            scroll -= seconds * row_height * 10.0f;
            if (scroll < target_scroll)
            {
                scroll = target_scroll;
                selected = target;
            }
        }
        else
        {
            scroll += seconds * row_height * 10.0f;
            if (scroll > target_scroll)
            {
                scroll = target_scroll;
                selected = target;
            }
        }
    }
    // The rows inside the list (the viewport set to it), from two below its top
    virtual void Draw(uint draw_flags)
    {
        if (!items)
            return;
        Triangle(x - row_height, y + row_height * 2, row_height * .6f, row_height, TRIANGLE_RIGHT, 0xffbe002c);
        SetViewport((int)x, (int)y, (int)sx, (int)sy);
        for (int i = 0; i < items->size(); i++)
        {
            if (i == selected && flags & COMPF_ACTIVE)
                mafiaIndicators.OutText((char *)(*items)[i], x, (i + 2) * row_height + y - scroll, sx, row_height,
                    0xffbe002c, 0, 0, NULL);
            else
                mafiaIndicators.OutText((char *)(*items)[i], x, (i + 2) * row_height + y - scroll, sx, row_height,
                    0xffffffff, 0, 0, NULL);
        }
        SetViewport(0, 0, igraph->Scrn_sx(), igraph->Scrn_sy());
    }
    virtual uint Mouse(int x, int y, int z, uint buttons, uint time)
    {
        if (!items)
            return 0;
        if (buttons & MENUMOUSE_LEFT_PRESSED)
        {
            target = (int)((y - this->y + scroll) / row_height - 2.0f);
            if (target < 0)
                target = 0;
            if (target >= items->size())
                target = items->size() - 1;
        }
        if (z)
        {
            target -= z / 100;
            if (target < 0)
                target = 0;
            if (target >= items->size())
                target = items->size() - 1;
            return id;
        }
        return 0;
    }

    std::vector<uchar *> *items;        // 0x40 (the names are ours)
    int target;                         // 0x44, the row the scroll goes to
    uint selected;                      // 0x48
    float scroll;                       // 0x4c
    float row_height;                   // 0x50, LISTCARS_ROW_HEIGHT on the screen
};

// A view's scroll (PS2's GM_ViewScroll), 'vscr': nothing drawn. While the mouse's left button is down it clicks, its
// range the mouse's movement since the button was pressed, in pixels a millisecond (x as the minimum, y as the
// maximum). PS2's has only the range.
// VTABLE: GAME 0x627110
class GM_ViewScroll : public GM_Component
{
public:
    GM_ViewScroll(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        speed_x = 0.0f;
        speed_y = 0.0f;
    }
    GM_ViewScroll(GM_Component *component) : GM_Component(component)
    {
        speed_x = 0.0f;
        speed_y = 0.0f;
    }

    virtual uint GetType() { return 'vscr'; }
    virtual void GetRangeFloat(float &min, float &max)
    {
        min = speed_x;
        max = speed_y;
    }
    virtual uint Mouse(int x, int y, int z, uint buttons, uint time)
    {
        if (buttons & MENUMOUSE_LEFT_PRESSED)
        {
            press_x = x;
            press_y = y;
        }
        if (buttons & MENUMOUSE_LEFT)
        {
            speed_x = (x - press_x) / (float)time;
            speed_y = (y - press_y) / (float)time;
            return id;
        }
        return 0;
    }

    int press_x, press_y;               // 0x40, where the button was pressed (the names are ours)
    float speed_x, speed_y;             // 0x48
};

// A text input (PS2's GM_TextInput), 'tinp': on the PC only drawn, as the stand-in (a green outline with its text)
// VTABLE: GAME 0x627190
class GM_TextInput : public GM_Component
{
public:
    GM_TextInput(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent) {}
    GM_TextInput(GM_Component *component) : GM_Component(component) {}

    virtual uint GetType() { return 'tinp'; }
    // The original kept this copy for the stand-in's, the same code
    virtual void Draw(uint draw_flags)
    {
        Rectangle(x, y, sx, sy, 0x7f00ff7f);
        mafiaIndicators.OutText((char *)text, x, y, sx, sy, 0xff00ff7f, 0, 0, NULL);
    }
};

// A big text's buffer (the names are ours): AddText adds no line that would end past the limit
#define STATICTEXTBIG_SIZE 0x1000
#define STATICTEXTBIG_LIMIT 3000

// A text of many lines (PS2's GM_StaticTextBig), 'stxb': its own copy of the text, broken into lines as wide as the
// component (HledejZlom, font 1), each ending in '~'; white, or in the alternate style red, centered and in font 1. Its
// value is the lines' height. Its SetTextID doesn't keep the id.
// VTABLE: GAME 0x627210
class GM_StaticTextBig : public GM_Component
{
public:
    GM_StaticTextBig(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        SetText(text);
    }
    GM_StaticTextBig(GM_Component *component) : GM_Component(component)
    {
        SetText(text);
    }

    // Empties the text (PS2's)
    void Clear()
    {
        lines = 0;
        buffer[0] = 0;
    }
    // Adds the text's lines (PS2's): a break at a space or a '~' drops that character. Counts one line more than it
    // adds.
    void AddText(uchar *text)
    {
        if (!text)
            return;
        char *end = strchr(buffer, 0);
        lines++;
        const char *brk;
        do
        {
            brk = mafiaIndicators.HledejZlom((char *)text, sx, sy, 1, NULL);
            if (brk)
            {
                int len = brk - (char *)text;
                if (end + len > &buffer[STATICTEXTBIG_LIMIT])
                    return;
                memcpy(end, text, len);
                end += len;
                *end++ = '~';
                *end = 0;
                if (!*brk)
                    return;
                if (*brk == ' ' || *brk == '~')
                    text = (uchar *)brk + 1;
                else
                    text = (uchar *)brk;
            }
            lines++;
        } while (brk);
    }

    virtual uint GetType() { return 'stxb'; }
    virtual void SetText(uchar *text)
    {
        Clear();
        AddText(text);
    }
    virtual void SetTextID(uint id)
    {
        Clear();
        AddText((uchar *)textDatabase.GetTextOrNULL(id));
    }
    virtual float GetFloat() { return lines * sy; }
    virtual void Draw(uint draw_flags)
    {
        if (flags & COMPF_ALTERNATE_STYLE)
            mafiaIndicators.OutTextMultiLine(buffer, x, y, sx, sy, 0xffbe002c, OUTTEXT_CENTER, 1);
        else
            mafiaIndicators.OutTextMultiLine(buffer, x, y, sx, sy, 0xffffffff, 0, 0);
    }

    uint lines;                         // 0x40 (the names are ours)
    char buffer[STATICTEXTBIG_SIZE];    // 0x44
};

// The copies of the classes' functions, by name, as g_m_comps.h's: GM_Vidc's in the order of the class, GM_WaitForVid's
// after Mouse, which needs its vtable first, before Key
// FUNCTION: GAME 0x5dda80
// ?GetType@GM_Vidc@@UAEIXZ

// FUNCTION: GAME 0x5dda90
// ?ForceUpdate@GM_Vidc@@UAEXXZ

// FUNCTION: GAME 0x5ddbd0
// ?Draw@GM_Vidc@@UAEXI@Z

// FUNCTION: GAME 0x5ddcb0
// ?Mouse@GM_Vidc@@UAEIHHHII@Z

// FUNCTION: GAME 0x5ddd40
// ?OnBeforeCreate@GM_WaitForVid@@UAEPBDXZ

// FUNCTION: GAME 0x5ddd50
// ?OnCloseChild@GM_WaitForVid@@UAEHPAVGM_Menu@@II@Z

// FUNCTION: GAME 0x5ddd60
// ?OnCreate@GM_WaitForVid@@UAEHXZ

// FUNCTION: GAME 0x5ddd90
// ?OnClose@GM_WaitForVid@@UAEHXZ

// FUNCTION: GAME 0x5ddda0
// ?OnTick@GM_WaitForVid@@UAEHI@Z

// GM_SetControlToVid's after OnTick, which needs its vtable first
// FUNCTION: GAME 0x5de090
// ?OnBeforeCreate@GM_SetControlToVid@@UAEPBDXZ

// FUNCTION: GAME 0x5de0a0
// ?OnCreate@GM_SetControlToVid@@UAEHXZ

// FUNCTION: GAME 0x5de0d0
// ?OnKey@GM_SetControlToVid@@UAEHEI@Z

// FUNCTION: GAME 0x5de0f0
// ?OnClick@GM_SetControlToVid@@UAEHI@Z

// FUNCTION: GAME 0x5de210
// ?Key@GM_Vidc@@UAEIII@Z

// The other classes' in the order of each class, GM_ComboMenu's after the combo box's Mouse, which needs its vtable first
// FUNCTION: GAME 0x5de610
// ?GetType@GM_ComboBox@@UAEIXZ

// FUNCTION: GAME 0x5de620
// ?SetPointer@GM_ComboBox@@UAEXPAX@Z

// FUNCTION: GAME 0x5de650
// ?SetDword@GM_ComboBox@@UAEXI@Z

// FUNCTION: GAME 0x5de690
// ?GetDword@GM_ComboBox@@UAEIXZ

// FUNCTION: GAME 0x5de6a0
// ?GetText@GM_ComboBox@@UAEPAEXZ

// FUNCTION: GAME 0x5de6b0
// ?Draw@GM_ComboBox@@UAEXI@Z

// FUNCTION: GAME 0x5de880
// ?Mouse@GM_ComboBox@@UAEIHHHII@Z

// FUNCTION: GAME 0x5de930
// ?OnBeforeCreate@GM_ComboMenu@@UAEPBDXZ

// FUNCTION: GAME 0x5de940
// ?OnCreate@GM_ComboMenu@@UAEHXZ

// FUNCTION: GAME 0x5deaa0
// ?OnClose@GM_ComboMenu@@UAEHXZ

// FUNCTION: GAME 0x5deac0
// ?OnClick@GM_ComboMenu@@UAEHI@Z

// FUNCTION: GAME 0x5deb30
// ?Key@GM_ComboBox@@UAEIII@Z

// FUNCTION: GAME 0x5decb0
// ?GetType@GM_ComboWindow@@UAEIXZ

// FUNCTION: GAME 0x5decc0
// ?Draw@GM_ComboWindow@@UAEXI@Z

// FUNCTION: GAME 0x5dee10
// ?GetType@GM_ListBox@@UAEIXZ

// FUNCTION: GAME 0x5dee20
// ?SetPointer@GM_ListBox@@UAEXPAX@Z

// The combo box's too
// FUNCTION: GAME 0x5deeb0
// ?GetPointer@GM_ListBox@@UAEPAXXZ

// FUNCTION: GAME 0x5deec0
// ?Draw@GM_ListBox@@UAEXI@Z

// FUNCTION: GAME 0x5df0a0
// ?Mouse@GM_ListBox@@UAEIHHHII@Z

// FUNCTION: GAME 0x5df1c0
// ?Key@GM_ListBox@@UAEIII@Z

// FUNCTION: GAME 0x5df340
// ?GetType@GM_ListCars@@UAEIXZ

// FUNCTION: GAME 0x5df360
// ?SetDword@GM_ListCars@@UAEXI@Z

// FUNCTION: GAME 0x5df380
// ?Tick@GM_ListCars@@UAEXI@Z

// FUNCTION: GAME 0x5df410
// ?Draw@GM_ListCars@@UAEXI@Z

// FUNCTION: GAME 0x5df570
// ?Mouse@GM_ListCars@@UAEIHHHII@Z

// FUNCTION: GAME 0x5df760
// ?GetType@GM_ViewScroll@@UAEIXZ

// FUNCTION: GAME 0x5df770
// ?GetRangeFloat@GM_ViewScroll@@UAEXAAM0@Z

// FUNCTION: GAME 0x5df790
// ?Mouse@GM_ViewScroll@@UAEIHHHII@Z

// FUNCTION: GAME 0x5df8e0
// ?GetType@GM_TextInput@@UAEIXZ

// FUNCTION: GAME 0x5dfab0
// ?GetType@GM_StaticTextBig@@UAEIXZ

// FUNCTION: GAME 0x5dfac0
// ?SetText@GM_StaticTextBig@@UAEXPAE@Z

// FUNCTION: GAME 0x5dfb60
// ?SetTextID@GM_StaticTextBig@@UAEXI@Z

// FUNCTION: GAME 0x5dfc10
// ?GetFloat@GM_StaticTextBig@@UAEMXZ

// FUNCTION: GAME 0x5dfc30
// ?Draw@GM_StaticTextBig@@UAEXI@Z

// The factories' functions
// TEMPLATE: GAME 0x5dd8e0
// ?Make@?$G_Factory@VGM_Vidc@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5de3a0
// ?MakeDup@?$G_Factory@VGM_Vidc@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5de580
// ?Make@?$G_Factory@VGM_ComboBox@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5debd0
// ?MakeDup@?$G_Factory@VGM_ComboBox@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5dec50
// ?Make@?$G_Factory@VGM_ComboWindow@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5ded20
// ?MakeDup@?$G_Factory@VGM_ComboWindow@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5ded80
// ?Make@?$G_Factory@VGM_ListBox@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5df220
// ?MakeDup@?$G_Factory@VGM_ListBox@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5df2a0
// ?Make@?$G_Factory@VGM_ListCars@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5df650
// ?MakeDup@?$G_Factory@VGM_ListCars@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5df6e0
// ?Make@?$G_Factory@VGM_ViewScroll@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5df800
// ?MakeDup@?$G_Factory@VGM_ViewScroll@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5df880
// ?Make@?$G_Factory@VGM_TextInput@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5df940
// ?MakeDup@?$G_Factory@VGM_TextInput@@@@SIPAVGM_Component@@PAV2@@Z

// TEMPLATE: GAME 0x5df9a0
// ?Make@?$G_Factory@VGM_StaticTextBig@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5dfc70
// ?MakeDup@?$G_Factory@VGM_StaticTextBig@@@@SIPAVGM_Component@@PAV2@@Z
