// The car encyclopedia (PS2 g_m_carc.cpp), the file at 0x5d7680-0x5d87dc (docs/game-files.md): GM_Carcyclopedia's
// functions in PS2's order, OnCreate first, RefreshCar last (PS2's destructor after it is implicit on the PC, its
// OnBeforeCreate's copy is in Main.cpp's range). The PC's OnMouse turns the turntable by the mouse and slows it down
// to its default speed, where PS2's OnTick does the latter, and the PC adds Home, End, H and Space to the keys.
// The mission's and the scene's accessors (GetScene, GetActiveCamera, GetPrimarySector) are the original's pending
// inline calls: written as the members, OnCreate and RefreshCar inline the names' erase and insert(P, x), which the
// original calls. The scene is searched with the frame's FindChildFrame (vtable 0x38), not the scene's FindFrame.
// Open: OnTick's first product loads zoom before the frame's x (equivalent code).

#include "ui/g_m_carc.h"
#include "ui/g_indicators.h"
#include "ui/g_loadsave.h"
#include "ui/g_m_comps.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "vehicles/C_car.h"
#include "I3D_cache.h"

#include <math.h>

#include "IGraph/IGraph.h"
#include "I3D/I3D.h"

// The cars every profile's encyclopedia has besides those the free ride enabled (G_LoadSaveClass::freeride_cars' bits,
// one for each letter): the letter Z's (the name is ours)
#define CARC_ALWAYS (1 << ('Z' - 'A'))

// The turntable's speed it slows down to, in degrees a second (the name is ours)
#define CARC_SPEED (1000.0f / 60.0f)

// FUNCTION: GAME 0x5d7680
int GM_Carcyclopedia::OnCreate()
{
    flags &= ~MENUF_DRAW_PARENT;
    mafiaCarDatabase.LoadDetails();
    mafiaCarDatabase.GetCarsByCarcyclopediaMask(mafiaLoadSave.FreerideGetEnabledCars() | CARC_ALWAYS, &cars);
    names.clear();
    for (uint *p = cars.begin(); p < cars.end(); p++)
        names.push_back((uchar *)mafiaCarDatabase.GetCarName(*p));
    SetPointer(CARC_LIST, &names);
    show_mode = false;
    car = 0;
    color = 0;
    // STRING: GAME 0x63811c
    turntable = act_mission->GetScene()->FindChildFrame("NGon02", ENUMF_ALL);
    model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
    if (turntable)
        model->LinkTo(turntable, 0);
    else
        model->LinkTo(act_mission->GetScene()->GetPrimarySector(), 0);
    model->SetPos(0.0f, 0.0f, 0.0f);
    model->SetDir1(S_vector(1.0f, 0.0f, 0.0f), S_vector(0.0f, 1.0f, 0.0f));
    model->Update();
    mafiaCarDatabase.LoadDetails();
    angle = 0.0f;
    unknown_50 = 60.0f;
    car = 0;
    speed = CARC_SPEED;
    roof_off = false;
    RefreshCar();
    act_mission->GetGame()->camera.PushMode();
    act_mission->GetGame()->camera.SetMode(CAMERA_MENU, true);
    zoom = 1.0f;
    autosalon = act_mission->GetScene()->FindChildFrame("autosalon", ENUMF_ALL);
    // STRING: GAME 0x638110
    autosalon2 = act_mission->GetScene()->FindChildFrame("autosalon2", ENUMF_ALL);
    if (autosalon)
    {
        act_mission->GetScene()->GetActiveCamera()->SetPos(autosalon->GetWorldPos());
        act_mission->GetScene()->GetActiveCamera()->SetDir(autosalon->GetWorldDir(), 0.0f);
        act_mission->GetScene()->GetActiveCamera()->SetRange(1.0f, 500.0f);
        mafiaIndicators.SetCameraFOV(act_mission->GetScene()->GetActiveCamera(), PI / 3);
        act_mission->GetScene()->GetActiveCamera()->Update();
    }
    SetHidden(CARC_WINDOW_LIST, !show_mode);
    SetHidden(CARC_WINDOW_SHOW, show_mode);
    return 0;
}

// FUNCTION: GAME 0x5d7ae0
int GM_Carcyclopedia::OnClose()
{
    mafiaCarDatabase.UnloadDetails();
    model->Release();
    act_mission->GetGame()->camera.PopMode(false);
    return 0;
}

// In the show mode the arrows move the camera and turn the turntable; the turntable takes its angle, the camera the
// place and direction between its two frames, and the list's selection, which the mouse can change, the car
// FUNCTION: GAME 0x5d7b10
int GM_Carcyclopedia::OnTick(uint time)
{
    float t = time * .001f;
    if (show_mode)
    {
        if (igraph->TestKey(DIK_UP))
            zoom -= t * .2f;
        if (igraph->TestKey(DIK_DOWN))
            zoom += t * .2f;
        if (igraph->TestKey(DIK_RIGHT))
        {
            speed = 100.0f;
            angle += t * 10.0f;
        }
        if (igraph->TestKey(DIK_LEFT))
        {
            speed = -100.0f;
            angle -= t * 10.0f;
        }
    }
    if (angle > 360.0f)
        angle -= 360.0f;
    if (angle < 0.0f)
        angle += 360.0f;
    if (turntable)
    {
        float a = angle * (PI / 180.0f);
        turntable->SetDir(S_vector((float)sin(a), 0.0f, (float)-cos(a)), 0.0f);
        turntable->Update();
    }
    if (autosalon && autosalon2)
    {
        if (zoom < 0.0f)
            zoom = 0.0f;
        if (zoom > 1.0f)
            zoom = 1.0f;
        // dir is assigned, so the sum is copied into it from a temporary (initialized, it is built in place)
        S_vector pos = autosalon->GetWorldPos() * zoom + autosalon2->GetWorldPos() * (1.0f - zoom), dir;
        dir = autosalon->GetWorldDir() * zoom + autosalon2->GetWorldDir() * (1.0f - zoom);
        act_mission->GetScene()->GetActiveCamera()->SetPos(pos);
        act_mission->GetScene()->GetActiveCamera()->SetDir(dir, 0.0f);
        act_mission->GetScene()->GetActiveCamera()->Update();
    }
    uint selected = GetDword(CARC_LIST);
    if (selected != car)
    {
        car = selected;
        RefreshCar();
    }
    return 0;
}

// In the list mode the arrows choose the car and its variant; in both Page Up and Page Down, Home and End choose the
// car, Enter, H and Space change the mode, C takes the roof off and puts it back, and Escape closes the menu
// FUNCTION: GAME 0x5d80c0
int GM_Carcyclopedia::OnKey(uchar key, uint)
{
    if (!show_mode)
    {
        switch (key)
        {
        case DIK_UP:
        case DIK_NUMPAD8:
            if (car)
                car--;
            RefreshCar();
            break;
        case DIK_DOWN:
        case DIK_NUMPAD2:
            if (car < names.size() - 1)
                car++;
            RefreshCar();
            break;
        case DIK_LEFT:
        case DIK_NUMPAD4:
            if (color)
                color--;
            else
                color = mafiaCarDatabase.GetCarMaxColor(cars[car]);
            RefreshCar();
            break;
        case DIK_RIGHT:
        case DIK_NUMPAD6:
            color++;
            RefreshCar();
            break;
        }
    }
    switch (key)
    {
    case DIK_PRIOR:
    case DIK_NUMPAD9:
        if (car > 1)
            car--;
        else
            car = 0;
        RefreshCar();
        return 0;
    case DIK_NEXT:
    case DIK_NUMPAD3:
        if (car < names.size() - 1)
            car++;
        else
            car = names.size() - 1;
        RefreshCar();
        return 0;
    case DIK_HOME:
    case DIK_NUMPAD7:
        car = 0;
        RefreshCar();
        return 0;
    case DIK_END:
    case DIK_NUMPAD1:
        car = names.size() - 1;
        RefreshCar();
        return 0;
    case DIK_C:
        roof_off = !roof_off;
        if (model)
            C_car::OdjebajTuStrechu(model, roof_off);
        return 0;
    case DIK_RETURN:
    case DIK_NUMPADENTER:
    case DIK_H:
    case DIK_SPACE:
        show_mode = !show_mode;
        SetHidden(CARC_WINDOW_LIST, !show_mode);
        SetHidden(CARC_WINDOW_SHOW, show_mode);
        return 0;
    case DIK_ESCAPE:
        MenuClose((uint)-1);
        break;
    }
    return 0;
}

// A drag in the view scroll turns the turntable and moves the camera
// FUNCTION: GAME 0x5d8400
int GM_Carcyclopedia::OnClick(uint id)
{
    if (id == CARC_VIEW)
    {
        float dx, dy;
        GetRangeFloat(CARC_VIEW, dx, dy);
        angle += dx;
        zoom += dy;
        speed = dx;
    }
    return 0;
}

// The left button held over the view scroll turns the turntable and moves the camera by the mouse's movement since the
// cursor's last place; otherwise the turntable turns on, its speed going back to CARC_SPEED within half a second. The
// wheel scrolls the list wherever the mouse is (GM_Menu::Tick gives it to the list only when it is over it).
// FUNCTION: GAME 0x5d8450
int GM_Carcyclopedia::OnMouse(int x, int y, int z, uint buttons, uint time)
{
    int dx = x - cursorX;
    int dy = y - cursorY;
    float t = time * .001f;
    GM_Component *view;
    if (buttons & MENUMOUSE_LEFT && (view = FindComponentByID(CARC_VIEW)) != NULL &&
        view->IsOverActive((float)x, (float)y))
    {
        angle += dx;
        zoom -= dy * .005f;
        speed = dx / t;
    }
    else
    {
        angle += t * speed;
        float k = t * 2.0f;
        if (k > 1.0f)
            k = 1.0f;
        speed = speed * (1.0f - k) + k * CARC_SPEED;
    }
    if (z)
    {
        GM_Component *list = FindComponentByID(CARC_LIST);
        if (list && !list->IsOverActive((float)x, (float)y))
            list->Mouse(x, y, z, buttons, time);
    }
    return 0;
}

// FUNCTION: GAME 0x5d8590
void GM_Carcyclopedia::RefreshCar()
{
    uint id;
    if (cars.size())
    {
        if (color > mafiaCarDatabase.GetCarMaxColor(cars[car]))
            color = 0;
        id = mafiaCarDatabase.GetIDByParams(cars[car], 0, color);
    }
    else
        id = 1;
    if (mafiaCarDatabase.GetCarMaxColor(id))
        SetDisabled(CARC_COLORS, true);
    else
        SetDisabled(CARC_COLORS, false);
    SetText(CARC_NAME, (uchar *)mafiaCarDatabase.GetCarName(id));
    SetTextID(CARC_SECOND_NAME, mafiaCarDatabase.GetCarSecondName(id, color));
    if (mafiaCarDatabase.GetCarDetails(id))
        SetTextID(CARC_DESCRIPTION, mafiaCarDatabase.GetCarDetails(id)->text_id);
    char name[0x20];
    mafiaCarDatabase.GetCarModel(id, name, false);
    model->Release();
    model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
    if (turntable)
        model->LinkTo(turntable, 0);
    else
        model->LinkTo(act_mission->GetScene()->GetPrimarySector(), 0);
    model_cache.Open(model, name, 0, NULL, NULL, NULL);
    model->SetPos(0.0f, -.01f, 0.0f);
    model->SetDir1(S_vector(1.0f, 0.0f, 0.0f), S_vector(0.0f, 1.0f, 0.0f));
    model->Update();
    // STRING: GAME 0x638124
    model->SetName("CarcCar");
    C_car::PrepareCarToShow(model);
    C_car::OdjebajTuStrechu(model, roof_off);
    SetDword(CARC_LIST, car);
}
