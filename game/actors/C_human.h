#pragma once

#include "actors/C_actor.h"

class C_car;

// The humans (PS2 C_human, C_human.cpp), the base of the player and of the AI's C_entity, 0xa9c bytes. Only the
// constructor so far, which the derived classes' constructors call, and the members the camera reads.
class C_human : public C_actor
{
public:
    C_human();

    // The humans' force feedback effects, which C_game's Init and Done create and free when the profile has them (PC
    // only, the names are ours)
    static void InitFFEffects();
    static void DoneFFEffects();
    // Hides or shows a driver's legs (PS2 name): empty on the PC
    static void HideLegs(I3D_model *model, bool hide);
    // Gets out of the car the human is in (PS2 name), which G_LoadSaveClass::GarageGameDone asks of the garage cars'
    // owners with the car's frame
    void Intern_FromCar(I3D_frame *frm);

    uchar unknown_70[0x28];             // 0x70
    C_car *car;                         // 0x98, the car the human is in (the name is ours)
    uchar unknown_9c[0x148];            // 0x9c
    bool crouching;                     // 0x1e4 (PS2 0x390), the camera drops while it is set (the name is ours)
    uchar unknown_1e5[0x41f];           // 0x1e5
    float max_health;                   // 0x604 (the name is ours)
    uchar unknown_608[0x3c];            // 0x608
    float health;                       // 0x644, which C_game::Init caps at max_health (the name is ours)
    uchar unknown_648[0x42c];           // 0x648
    C_actor *killer;                    // 0xa74 (PS2 0xba8), whom the death camera looks at (the name is ours)
    uchar unknown_a78[0x24];            // 0xa78
};
