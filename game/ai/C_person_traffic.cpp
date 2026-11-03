// The pedestrians' generator (PS2 C_person_traffic.cpp, C_traffic_generator's; the PC's file is the one at
// 0x44c520-0x459e8c, docs/game-files.md), whose constructor (0x44d410) comes after functions of its own. Written so far:
// C_don_ocas's constructor, the static functions C_game's Init and Done call (SetVisible, PlayerInTaxi,
// EnableTaxiDriver, ClearElements and FreeDynCollElements), ZrusVsechnyMavajici, and TickElements, which C_game::Tick
// calls, and of what TickElements calls TryGoToAssHole and IsCarHasOcas, which it calls; the constructor, which
// C_mission::CreateActor calls, GeneratePassengerForTaxi, which TickElements calls, AddFreePerson, which a script's
// command calls, PlayerHoMaVytaseneho, which C_shvestky::Tick calls, and C_traffic_element's SetNewNode,
// EndMavaniNaTaxi, Tick and Deactivate are stubs.
// The vectors' initializers start the file (0x44c520, 0x44c5a0), then an empty one (0x44c620). Open: their destructors
// (0x44c560, 0x44c5e0) test and set a guard byte they share (0x63d760), a bit each, as C_actor.cpp's do.

#include "ai/C_person_traffic.h"

#include <float.h>
#include <string.h>

#include "actors/C_animations.h"
#include "actors/C_human.h"
#include "collision/g_colls.h"
#include "mission/Mission.h"
#include "ui/g_loadsave.h"
#include "vehicles/C_car.h"
#include "vehicles/C_trf_c.h"
#include "stub.h"

// GLOBAL: GAME 0x63d764
std::vector<C_traffic_element *> C_traffic_generator::array;
// GLOBAL: GAME 0x63d750
std::vector<C_animations_machine_de_luxe *> C_traffic_generator::anims;

// GLOBAL: GAME 0x632638
float C_traffic_generator::ratio_all = 1.0f;
// GLOBAL: GAME 0x63263c
float C_traffic_generator::ratio_police = 1.0f;
// GLOBAL: GAME 0x63d728
bool C_traffic_generator::global_valid_scream;
// GLOBAL: GAME 0x63d729
bool C_traffic_generator::player_in_taxi;
// GLOBAL: GAME 0x63d72a
bool C_traffic_generator::enable_taxi_driver;
// GLOBAL: GAME 0x63d730
bool C_traffic_generator::all_visible;
// GLOBAL: GAME 0x63d738
int C_traffic_generator::t_horn;
// GLOBAL: GAME 0x63d734
int C_traffic_generator::try_do_rici;
// GLOBAL: GAME 0x63d72c
int C_traffic_generator::taxi_timer;

// FUNCTION: GAME 0x44c640
C_don_ocas::C_don_ocas()
{
    type = ACTOR_DON_OCAS;
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x44c680
C_don_ocas::~C_don_ocas()
{
}

#pragma auto_inline(off)

// STUB: GAME 0x44d410
C_traffic_generator::C_traffic_generator()
{
    StubLog("C_traffic_generator::C_traffic_generator");
}

// STUB: GAME 0x44e890
void C_traffic_generator::PlayerHoMaVytaseneho()
{
    StubLog("C_traffic_generator::PlayerHoMaVytaseneho");
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

// Nothing while the pedestrians are hidden. The active ones from the last, each with the camera's distance in the
// horizontal plane first (PS2's the same, the height zeroed), then the timers.
// FUNCTION: GAME 0x44ef70
void C_traffic_generator::TickElements(uint time)
{
    if (!all_visible)
        return;
    if (t_horn > 0)
    {
        if (time > t_horn)
            t_horn = 0;
        else
            t_horn -= time;
    }
    S_vector cam = act_mission->GetScene()->GetActiveCamera()->GetWorldPos();
    for (int i = array.size(); i--;)
    {
        if (array[i]->active)
        {
            S_vector d = cam - array[i]->pos;
            d.y = 0.0f;
            array[i]->dist = d.Square();
            array[i]->Tick(time);
        }
    }
    try_do_rici -= time;
    if (try_do_rici < 0)
    {
        try_do_rici = 2123;
        TryGoToAssHole();
    }
    if (enable_taxi_driver && player_in_taxi)
    {
        if (taxi_timer < 0)
            GeneratePassengerForTaxi();
        else
            taxi_timer -= time;
    }
}

// FUNCTION: GAME 0x44f090
bool C_traffic_generator::IsCarHasOcas(C_car *car)
{
    for (int i = array.size(); i--;)
    {
        if (array[i]->active && !array[i]->dead && !array[i]->fleeing && array[i]->car == car)
            return true;
    }
    return false;
}

// The distance of point from the line through orig along dir, -1e16 for a null dir (PS2's; the name is ours). PS2 has
// it written in TryGoToAssHole, inlined and in another order (dir * t from orig - point). On the PC its vector calls
// stay out of line where those around them are inlined, so they are nested calls, and VC6 folds the null dir's
// -1e16 < 5 into the caller's jump.
static inline float DistToLine(const S_vector &orig, const S_vector &dir, const S_vector &point)
{
    float sq = dir.Dot(dir);
    if (sq < MRG_ZERO)
        return -1e16f;
    return ((orig + dir * (dir.Dot(point - orig) / sq)) - point).Magnitude();
}

// Sends a free pedestrian, from the last one, to the nearest parked car the two masks allow (a car parked longer than 15
// s, nobody in it or stealing it, no other pedestrian walking to it) within 20 m and ahead of it, if the car lies within
// 5 m of the line it walks along: the way to the car's placement and two of its navigation points, each put on the
// ground. One pedestrian at most, as PS2's.
// The car's mask is a local: VC6 calls it first and keeps it in ebx across the name's strrchr. p2 is the car's position
// first, one variable with the navigation point and the ground's hit (PS2 has the last two in one too): the original's
// stack area for the position is the point's, and a variable of its own, in the block around the points, gets one to
// itself. Open: from the placement call on, the code generator's temporaries take the register after the original's
// (ecx for eax) with the instructions the same; the first navigation point's square comes x, z, y where the original's
// comes z, y, x (a sum written out gives that, but frees inline budget the helper's first Dot then takes); and the
// position's third word is copied after the element's frame is loaded.
// FUNCTION: GAME 0x44f0e0
void C_traffic_generator::TryGoToAssHole()
{
    if (!all_visible)
        return;
    std::vector<C_traffic_car::S_ELEMENT *> &parking = C_traffic_car::GetParkingCars();
    for (int i = array.size(); i--;)
    {
        if (array[i]->active && !array[i]->dead && !array[i]->fleeing && array[i]->generator &&
            !array[i]->car)
        {
            float best = FLT_MAX;
            int best_i = -1;
            for (int j = parking.size(); j--;)
            {
                uint mask = mafiaCarDatabase.GetCivilMaskByModel(parking[j]->car->GetModelName());
                const char *name = strrchr(array[i]->frame->GetFileName(), '\\');
                if (mask &
                        mafiaCarDatabase.GetPlayerCivilMaskByModel(name ? name + 1 : array[i]->frame->GetFileName()) &&
                    parking[j]->car->GetTimeParking() > 15000 && !C_human::IsThisCarStealing(parking[j]->car) &&
                    !IsCarHasOcas(parking[j]->car))
                {
                    S_vector d = parking[j]->frame->GetWorldPos() - array[i]->frame->GetWorldPos();
                    float dist = d.Square();
                    if (dist < best && dist < 400.0f && d.AngleTo(array[i]->dir) < PI / 2)
                    {
                        best_i = j;
                        best = dist;
                    }
                }
            }
            if (best_i > -1)
            {
                S_vector p2 = parking[best_i]->frame->GetWorldPos();
                S_vector dir = array[i]->target - array[i]->frame->GetWorldPos();
                if (DistToLine(array[i]->target, dir, p2) < 5.0f &&
                    parking[best_i]->car->GetPlacementFromTCK1(0, array[i]->way[0], array[i]->way_dir, true))
                {
                    array[i]->car = parking[best_i]->car;
                    S_vector p1, p3, p4;
                    parking[best_i]->car->GetNavigationPoints(0.5f, p1, p2, p3, p4);
                    p1 *= parking[best_i]->car->frame->GetMatrix();
                    p2 *= parking[best_i]->car->frame->GetMatrix();
                    p3 *= parking[best_i]->car->frame->GetMatrix();
                    p4 *= parking[best_i]->car->frame->GetMatrix();
                    if ((p1 - array[i]->GetPos()).Square() < (p3 - array[i]->GetPos()).Square())
                    {
                        array[i]->way[2] = p2;
                        array[i]->way[1] = p1;
                    }
                    else
                    {
                        array[i]->way[2] = p4;
                        array[i]->way[1] = p3;
                    }
                    array[i]->num_way = 3;
                    array[i]->way_index = 0;
                    for (int k = 0; k < 3; k++)
                    {
                        p1 = array[i]->way[k];
                        p1.y += 1.0f;
                        if (gifCollision.TestLineV(p1, S_vector(0.0f, -3.0f, 0.0f), &p2, NULL, 0, 0))
                        {
                            array[i]->way[k] = p2;
                            array[i]->way[k].y += 0.01f;
                        }
                    }
                    array[i]->SetNewNode();
                    return;
                }
            }
        }
    }
}

#pragma auto_inline(off)

// STUB: GAME 0x44f7d0
void C_traffic_generator::GeneratePassengerForTaxi()
{
    StubLog("C_traffic_generator::GeneratePassengerForTaxi");
}

#pragma auto_inline(on)

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

// STUB: GAME 0x451860
void C_traffic_element::SetNewNode()
{
    StubLog("C_traffic_element::SetNewNode");
}

// STUB: GAME 0x452ca0
void C_traffic_element::EndMavaniNaTaxi()
{
    StubLog("C_traffic_element::EndMavaniNaTaxi");
}

// STUB: GAME 0x452cd0
void C_traffic_element::Tick(uint time)
{
    StubLog("C_traffic_element::Tick");
}

// STUB: GAME 0x4587c0
void C_traffic_element::Deactivate()
{
    StubLog("C_traffic_element::Deactivate");
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x44d4a0
C_traffic_generator::~C_traffic_generator()
{
    StubLog("C_traffic_generator::~C_traffic_generator");
}

// The original's is C_door's, folded: it logs that name, so ours folds too
// STUB: GAME 0x4393a0 FOLDED
bool C_traffic_generator::Init(I3D_frame *frm)
{
    StubLog("C_door::Init");
    return C_actor::Init(frm);
}

// STUB: GAME 0x44d510
void C_traffic_generator::DuplicateFrom(C_actor *act)
{
    StubLog("C_traffic_generator::DuplicateFrom");
    C_actor::DuplicateFrom(act);
}

// STUB: GAME 0x44d670
void C_traffic_generator::SaveData(C_chunk *chunk)
{
    StubLog("C_traffic_generator::SaveData");
    C_actor::SaveData(chunk);
}

// STUB: GAME 0x44d820
void C_traffic_generator::LoadData(C_chunk *chunk)
{
    StubLog("C_traffic_generator::LoadData");
    C_actor::LoadData(chunk);
}

// STUB: GAME 0x44de00
int C_traffic_generator::SaveGameGetSize()
{
    StubLog("C_traffic_generator::SaveGameGetSize");
    return C_actor::SaveGameGetSize();
}

// STUB: GAME 0x44de20
bool C_traffic_generator::SaveGameSave(void **buf)
{
    StubLog("C_traffic_generator::SaveGameSave");
    return C_actor::SaveGameSave(buf);
}

// STUB: GAME 0x44e120
bool C_traffic_generator::SaveGameLoad(void **buf)
{
    StubLog("C_traffic_generator::SaveGameLoad");
    return C_actor::SaveGameLoad(buf);
}

// STUB: GAME 0x44e500
void C_traffic_generator::GameInit()
{
    StubLog("C_traffic_generator::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x44e7f0
void C_traffic_generator::GameDone()
{
    StubLog("C_traffic_generator::GameDone");
    C_actor::GameDone();
}

// STUB: GAME 0x44edb0
void C_traffic_generator::AI(uint time)
{
    StubLog("C_traffic_generator::AI");
    C_actor::AI(time);
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_traffic_generator::Update(uint time)
{
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_traffic_generator::NetDirect(uint time)
{
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x44c690
void C_don_ocas::GameInit()
{
    StubLog("C_don_ocas::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x44c6a0
bool C_don_ocas::IsPossibleColl(C_actor *act, S_vector v1, S_vector v2, float f, float &dist, C_actor **coll)
{
    StubLog("C_don_ocas::IsPossibleColl");
    return C_actor::IsPossibleColl(act, v1, v2, f, dist, coll);
}

// STUB: GAME 0x44ca50
bool C_don_ocas::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    I3D_frame *frm)
{
    StubLog("C_don_ocas::Collision");
    return C_actor::Collision(act, v1, v2, v3, f, frm);
}

// STUB: GAME 0x44ccf0
bool C_don_ocas::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
    uint u, I3D_frame *frm)
{
    StubLog("C_don_ocas::Hit");
    return C_actor::Hit(type, v1, v2, v3, f, act, u, frm);
}

// STUB: GAME 0x44d1c0
bool C_don_ocas::GetBoundPoints2D(std::vector<S_vector2> &points, I3D_frame *frm)
{
    StubLog("C_don_ocas::GetBoundPoints2D");
    return C_actor::GetBoundPoints2D(points, frm);
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x456fa0
void C_traffic_element::ActivateFree(bool b1, bool b2)
{
    StubLog("C_traffic_element::ActivateFree");
}

#pragma auto_inline(on)
