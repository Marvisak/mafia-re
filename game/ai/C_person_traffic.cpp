// The pedestrians' generator (PS2 C_person_traffic.cpp, C_traffic_generator's; the PC's file is the one at
// 0x44c520-0x459e8c, docs/game-files.md), whose constructor (0x44d410) comes after functions of its own. Written so far:
// C_don_ocas's constructor, the static functions C_game's Init and Done call (SetVisible, PlayerInTaxi,
// EnableTaxiDriver, ClearElements and FreeDynCollElements) and ZrusVsechnyMavajici; the constructor, which
// C_mission::CreateActor calls, AddFreePerson, which a script's command calls, and C_traffic_element's EndMavaniNaTaxi
// and Deactivate are stubs.
// The vectors' initializers start the file (0x44c520, 0x44c5a0), then an empty one (0x44c620). Open: their destructors
// (0x44c560, 0x44c5e0) test and set a guard byte they share (0x63d760), a bit each, as C_actor.cpp's do.

#include "ai/C_person_traffic.h"

#include <string.h>

#include "actors/C_animations.h"
#include "vehicles/C_car.h"
#include "stub.h"

// GLOBAL: GAME 0x63d764
std::vector<C_traffic_element *> C_traffic_generator::array;
// GLOBAL: GAME 0x63d750
std::vector<C_animations_machine_de_luxe *> C_traffic_generator::anims;

// GLOBAL: GAME 0x632638
float C_traffic_generator::unknown_632638 = 1.0f;
// GLOBAL: GAME 0x63263c
float C_traffic_generator::unknown_63263c = 1.0f;
// GLOBAL: GAME 0x63d728
bool C_traffic_generator::unknown_63d728;
// GLOBAL: GAME 0x63d729
bool C_traffic_generator::player_in_taxi;
// GLOBAL: GAME 0x63d72a
bool C_traffic_generator::enable_taxi_driver;
// GLOBAL: GAME 0x63d730
bool C_traffic_generator::all_visible;

// FUNCTION: GAME 0x44c640
C_don_ocas::C_don_ocas()
{
    type = ACTOR_DON_OCAS;
}

#pragma auto_inline(off)

// STUB: GAME 0x44d410
C_traffic_generator::C_traffic_generator()
{
    StubLog("C_traffic_generator::C_traffic_generator");
}

#pragma auto_inline(on)

// From the last one, as PS2's, which also gives the shown pedestrians their fake shadows and takes them from the hidden
// ones
// FUNCTION: GAME 0x44e960
void C_traffic_generator::SetVisible(bool visible)
{
    if (all_visible == visible)
        return;
    all_visible = visible;
    for (int i = array.size(); i--;)
    {
        if (array[i]->active && array[i]->frame)
            array[i]->frame->SetOn(visible);
    }
}

// The PC's nine taxis, from the last one (PS2 has eleven, from a table of its own)
// FUNCTION: GAME 0x44f740
void C_traffic_generator::PlayerInTaxi(C_car *car)
{
    player_in_taxi = false;
    if (!car)
        return;
    const char *taxis[] = {
        // STRING: GAME 0x6326a8
        "taxi",
        // STRING: GAME 0x63269c
        "hotrodp4",
        // STRING: GAME 0x632690
        "hotrodp5",
        // STRING: GAME 0x632684
        "tbirdold",
        // STRING: GAME 0x63267c
        "fordhot",
        // STRING: GAME 0x632670
        "phantomtaxi",
        // STRING: GAME 0x632664
        "hotrodp2",
        // STRING: GAME 0x632654
        "fordapicktaxi",
        // STRING: GAME 0x632648
        "blackdragon",
    };
    int i = sizeof(taxis) / sizeof(*taxis);
    while (i--)
    {
        if (!_stricmp(car->model_name, taxis[i]))
        {
            player_in_taxi = true;
            return;
        }
    }
}

// FUNCTION: GAME 0x44fcf0
void C_traffic_generator::EnableTaxiDriver(bool enable)
{
    if (!enable && enable_taxi_driver)
        ZrusVsechnyMavajici();
    enable_taxi_driver = enable;
}

// Ends the waving at a taxi of every pedestrian (PS2's name, Czech: "cancel all the wavers"), from the last one
// FUNCTION: GAME 0x44fd10
void C_traffic_generator::ZrusVsechnyMavajici()
{
    for (int i = array.size(); i--;)
    {
        if (array[i]->waving)
            array[i]->EndMavaniNaTaxi();
    }
}

// PS2's order: the pedestrians forgotten, their animations freed from the last one
// FUNCTION: GAME 0x44fd50
void C_traffic_generator::ClearElements()
{
    array.clear();
    for (int i = anims.size(); i--;)
        delete anims[i];
    anims.clear();
}

// The active pedestrians taken out of the game, from the last one
// FUNCTION: GAME 0x450860
void C_traffic_generator::FreeDynCollElements()
{
    for (int i = array.size(); i--;)
    {
        if (array[i]->active)
            array[i]->Deactivate();
    }
}

#pragma auto_inline(off)

// STUB: GAME 0x450f00
void C_traffic_generator::AddFreePerson(I3D_model *model, C_actor *act, bool unknown)
{
    StubLog("C_traffic_generator::AddFreePerson");
}

// STUB: GAME 0x452ca0
void C_traffic_element::EndMavaniNaTaxi()
{
    StubLog("C_traffic_element::EndMavaniNaTaxi");
}

// STUB: GAME 0x4587c0
void C_traffic_element::Deactivate()
{
    StubLog("C_traffic_element::Deactivate");
}

#pragma auto_inline(on)





