#pragma once

#include "actors/C_actor.h"
#include "actors/C_animations.h"
#include "actors/g_inventory.h"

class C_car;
class C_car_ex;

// The humans' properties, max_properties' and properties' indices: _human_property_'s (C_program.cpp), whose names
// the script's HUMAN_GETPROPERTY and HUMAN_SETPROPERTY take (the name is ours)
#define HUMAN_PROPERTY_ENERGY 1         // "Energy", the health

// The humans (PS2 C_human, C_human.cpp), the base of the player and of the AI's C_entity, 0xa9c bytes. Only the
// constructor so far, which the derived classes' constructors call, the members the camera and the script read, and the
// functions the script calls, as stubs.
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
    // PS2 names, which the script calls; the parameters' names are ours
    // Gets into the truck, HUMAN_ENTERTOTRUCK's
    void _intern_UseCarTruck(C_car_ex *car, bool unknown);
    // The pose at the position, by value (PS2's takes a reference)
    void PoseSetPoseNormal(S_vector pos);
    void PoseNullRotate();
    void Do_StopAnim();
    void Do_Breath();
    // The animations replaced, the second NULL where the script gives none
    void ChangeAnim(char *anim1, char *anim2);
    // Gets into the car's seat
    void Intern_UseCar(C_car *car, int seat);
    void ChangeWeaponModel();
    // Drops the items out of the inventory
    bool DropOutItems(std::vector<S_GameItem> &items);
    void Do_ThrowGrenade(S_vector &dir);
    void Do_Holster();
    int Do_PlayAnim(const char *anim, bool simple, bool unknown);
    // The model changed for the named one
    void _intern_ChangeModel(char *name, bool unknown);
    void ResetActions();

    int anim_id;                        // 0x70 (PS2 0x200), HUMAN_GETACTANIMID's (the name is ours)
    uchar unknown_74[0x24];             // 0x74
    C_car *car;                         // 0x98, the car the human is in (the name is ours)
    uchar unknown_9c[8];                // 0x9c
    // The cars the script's CAR_LOCK and CAR_LOCK_ALL lock to the human (the names are ours)
    C_car *locked_car;                  // 0xa4
    C_car *locked_vehicle;              // 0xa8
    // 0xac (PS2 0x238), the seat in the car most likely, -1 for none (the name is a guess)
    int seat;
    uchar unknown_b0[0xc];              // 0xb0
    // 0xbc (PS2 0x250), whose Reset the script's ACTOR_SETPLACEMENT calls (the name is ours)
    C_animations_machine_de_luxe anim_machine;
    bool crouching;                     // 0x1e4 (PS2 0x390), the camera drops while it is set (the name is ours)
    uchar unknown_1e5[0xf3];            // 0x1e5
    I3D_frame *look_frame;              // 0x2d8 (PS2 0x454), SetLookFrame's (the name is ours)
    uchar unknown_2dc[0x130];           // 0x2dc
    int unknown_40c;                    // 0x40c (PS2 0x588), a state: 1, 2 and 9 are ready, 9 keeps the car
    uchar unknown_410[8];               // 0x410
    float unknown_418;                  // 0x418 (PS2 0x594), HUMAN_FORCEFALL adds 0.1
    uchar unknown_41c[0x48];            // 0x41c
    int unknown_464;                    // 0x464 (PS2 0x5bc), ACTOR_SETPLACEMENT clears it
    uchar unknown_468[0x10];            // 0x468
    C_car *stealing_car;                // 0x478 (PS2 0x5c4), IFPLAYERSTEALCAR's (the name is ours)
    int shadow;                         // 0x47c (PS2 0x5c8), C_game::AddFrameForShadow's (the name is ours)
    G_Inventory inventory;              // 0x480 (the name is ours)
    uchar unknown_564[8];               // 0x564
    // The frames the hands hold things by, LINKTOHAND's (PS2 0x5f8 and 0x5fc; the names are ours)
    I3D_frame *right_hand;              // 0x56c
    I3D_frame *left_hand;               // 0x570
    uchar unknown_574[0x24];            // 0x574
    bool unknown_598;                   // 0x598, cleared while a record (RECLOADFULL) plays
    uchar unknown_599[0x67];            // 0x599
    // The properties' highest values and the current ones, HUMAN_PROPERTY_* (PS2 0x664 and 0x6a4; the names are ours):
    // the 15 _human_property_ names in 16 floats each
    float max_properties[16];           // 0x600
    float properties[16];               // 0x640, which C_game::Init caps at max_properties
    uchar unknown_680[4];               // 0x680
    bool shut_up;                       // 0x684 (PS2 0x6e8), HUMAN_SHUTUP's (the name is ours)
    uchar unknown_685[0x53];            // 0x685
    // The HUD's wingman indicator, -1 for none, and its name, 0 to 7 (PS2 0x734 and 0x738; the names are ours)
    int wingman;                        // 0x6d8
    uint wingman_name;                  // 0x6dc
    I3D_frame *fire_target;             // 0x6e0, HUMAN_SETFIRETARGET's, NULL for none (the name is ours)
    uchar unknown_6e4[0x14];            // 0x6e4
    // What the human carries (PS2 0x754, 0x758 and 0x75c, Do_FuckingBox's and Do_DeadBody's; the names are ours)
    I3D_frame *box;                     // 0x6f8
    int box_index;                      // 0x6fc
    I3D_frame *dead_body;               // 0x700
    uchar unknown_704[0x1c];            // 0x704
    bool no_anim_hit;                   // 0x720 (PS2 0x770), SetNoAnimHit's (the name is ours)
    uchar unknown_721[0x313];           // 0x721
    bool ab_created;                    // 0xa34 (PS2 0xb80), HUMAN_CREATEAB sets it, HUMAN_ERASEAB clears it (ours)
    uchar unknown_a35[0x3f];            // 0xa35
    C_actor *killer;                    // 0xa74 (PS2 0xba8), whom the death camera looks at (the name is ours)
    uchar unknown_a78[0x24];            // 0xa78

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours)
    // FUNCTION: GAME 0x47b7a0
    void SetLookFrame(I3D_frame *frm)
    {
        look_frame = frm;
    }
    // FUNCTION: GAME 0x47b7b0
    C_car *GetCar() const
    {
        return car;
    }
    // FUNCTION: GAME 0x47b7c0
    void SetWingman(int index, uint name)
    {
        wingman = index;
        wingman_name = name;
    }
    // FUNCTION: GAME 0x47b7e0
    void SetFireTarget(I3D_frame *frm)
    {
        fire_target = frm;
    }
    // FUNCTION: GAME 0x47b7f0
    void SetNoAnimHit(bool on)
    {
        no_anim_hit = on;
    }
    // FUNCTION: GAME 0x47b800
    void SetUnknown598(bool value)
    {
        unknown_598 = value;
    }
    // Whether the human can go on, HUMAN_WAITTOREADY's (PS2 has the same test inline)
    // FUNCTION: GAME 0x47b810
    bool IsReady() const
    {
        return unknown_5d && (unknown_40c == 1 || unknown_40c == 2 || unknown_40c == 9) &&
            !(anim_machine.unknown_f4 && anim_machine.unknown_10c);
    }
    // FUNCTION: GAME 0x47b850
    void SetShutUp(bool b)
    {
        shut_up = b;
    }
};
