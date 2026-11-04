#pragma once

#include "ui/g_menu.h"

#include <vector>

class I3D_frame;
class I3D_model;

// The car encyclopedia's items, tables\menu\carcyclopedia.mnu (the names are ours, after their texts and what the code
// does with them)
#define CARC_WINDOW_LIST 1              // the list mode's window, "Select Car"
#define CARC_WINDOW_SHOW 2              // the show mode's window
#define CARC_VIEW 3                     // the view scroll the mouse turns and moves the car with ('vscr')
#define CARC_LIST 20                    // the list of cars ('lstc')
#define CARC_NAME 50                    // the car's name
#define CARC_SECOND_NAME 51             // the variant's name, of the text database ("Mafia")
#define CARC_DESCRIPTION 52             // the car's description, of the text database
#define CARC_COLORS 102                 // enabled for a car with variants: not in the menu's definition

// The car encyclopedia (PS2 GM_Carcyclopedia, g_m_carc.cpp), 0x84 bytes, the menu of the autosalon mission: the car
// on the turntable "NGon02", which the camera shows from between the frames "autosalon" and "autosalon2". In the
// list mode the arrows choose the car and its variant, in the show mode they turn the turntable and move the camera.
// Its constructor is the implicit one: the base's, then the vectors' (each copies a byte of an uninitialized allocator
// temporary, VC6's vector(const _A& = _A())). PS2's has the members in the same order, 4 bytes lower (its vectors 12
// bytes each). The names are ours.
// VTABLE: GAME 0x625e88
class GM_Carcyclopedia : public GM_Menu
{
public:
    int car;                            // 0x30, the index in cars of the car shown, the list's selection
    uint color;                         // 0x34, its variant
    I3D_model *model;                   // 0x38, the car shown, "CarcCar"
    uint unknown_3c;                    // 0x3c
    I3D_frame *turntable;               // 0x40, "NGon02", which the car is linked to
    I3D_frame *autosalon;               // 0x44, the camera's frame at zoom 1
    I3D_frame *autosalon2;              // 0x48, at zoom 0
    float angle;                        // 0x4c, the turntable's, in degrees
    float unknown_50;                   // 0x50, 60 from OnCreate (PS2's 0x4c the same)
    float zoom;                         // 0x54, the camera from autosalon2 (0) to autosalon (1)
    float speed;                        // 0x58, the turntable's, in degrees a second
    bool show_mode;                     // 0x5c, the cursor keys move the view instead of choosing the car
    std::vector<uint> cars;             // 0x60, the cars' descriptors (G_CarDatabase::GetCarsByCarcyclopediaMask)
    std::vector<uchar *> names;         // 0x70, their names, the list's
    bool roof_off;                      // 0x80, C_car::OdjebajTuStrechu's

    // FUNCTION: GAME 0x5bf510
    virtual const char *OnBeforeCreate()
    {
        // STRING: GAME 0x637b68
        return "carcyclopedia";
    }
    virtual int OnCreate();
    virtual int OnClose();
    virtual int OnKey(uchar key, uint);
    virtual int OnMouse(int x, int y, int z, uint buttons, uint time);
    virtual int OnClick(uint id);
    virtual int OnTick(uint time);

    // The car of the selection and its variant: the texts, and the model loaded again (PS2's takes the steps of its
    // model cache, which the PC doesn't have)
    void RefreshCar();
};
