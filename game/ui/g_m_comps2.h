#pragma once

// The classes of g_m_comps2.cpp's components that are written: the PC's own ('vidc') with the two menus it opens. In a
// header of their own, which only their file includes: the profile's controls they read stay out of the other
// components' files.

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
    GM_WaitForVid(int control) : control(control), unknown_34(false) {}

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
        if (unknown_34)
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
    bool unknown_34;                    // 0x34, set: only axes (nothing sets it)
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
        flags |= COMPF_UNKNOWN_8000;
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

// The factory's functions
// TEMPLATE: GAME 0x5dd8e0
// ?Make@?$G_Factory@VGM_Vidc@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5de3a0
// ?MakeDup@?$G_Factory@VGM_Vidc@@@@SIPAVGM_Component@@PAV2@@Z
