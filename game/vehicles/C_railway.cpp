// The rail vehicles (PS2 C_railway.cpp, docs/game-files.md: the PC's file at 0x4826c0-0x49908c), in PS2's order: the
// seats (S_RailSeats), the passengers (S_rail_human) and their update, the vehicle's statics, the vehicle (C_railway),
// its entries (rail_entries), doors (rail_doors) and windows (rail_glass, rail_window).

#include "vehicles/C_railway.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "actors/C_animations.h"
#include "actors/C_dyn_object.h"
#include "actors/C_human.h"
#include "ai/C_person_traffic.h"
#include "ai/pathweb.h"
#include "ai/road_web.h"
#include "C_chunk.h"
#include "collision/g_c_glob.h"
#include "collision/g_colls.h"
#include "g_material.h"
#include "IGraph/IGraph.h"
#include "I3D_cache.h"
#include "mission/game.h"
#include "mission/Mission.h"
#include "ui/g_indicators.h"
#include "vehicles/C_car.h"
#include "vehicles/C_trf_c.h"
#include "vehicles/VE_BreakageGlass.h"
#include "stub.h"

// The enumerations' callbacks (PS2 names), after Enumerate
I3DENUMRET __stdcall cbEnumDummies(I3D_frame *frm, uint user);
I3DENUMRET __stdcall cbEnumDummiesVagons(I3D_frame *frm, uint user);
I3DENUMRET __stdcall cbEnumLights(I3D_frame *frm, uint user);
I3DENUMRET __stdcall cbEnumDestroy(I3D_frame *frm, uint user);

// The first free seat of a wagon, looking from where a running counter says (so the passengers spread over the
// vehicle); with a position, one on its side of the plane z = dist. The seat's number, 0xffff for none
// FUNCTION: GAME 0x4826c0
uint S_RailSeats::FindFreeSeat(uint wagon, S_vector *pos, float dist)
{
    static uint counter;
    for (uint i = 0; i < num_rows; i++)
    {
        uint r = (counter + i) % num_rows;
        for (uint j = 0; j < rows[r].num_seats; j++)
        {
            uint s = (counter + j) % rows[r].num_seats;
            if (rows[r].state[s] == RSEAT_FREE && rows[r].wagon == wagon)
            {
                if (!pos || (pos->z < dist && GetSeatPos(rows[r].first_seat + s).z < dist) ||
                    (pos->z > dist && GetSeatPos(rows[r].first_seat + s).z > dist))
                    {
                    uint seat = rows[r].first_seat + s;
                    counter++;
                    return seat;
                }
            }
        }
    }
    return 0xffff;
}

// The seats free in a wagon (-1: in all)
// FUNCTION: GAME 0x482810
int S_RailSeats::NumFreeSeats(int wagon)
{
    int n = 0;
    for (uint i = 0; i < num_rows; i++)
    {
        for (uint j = 0; j < rows[i].num_seats; j++)
        {
            if (rows[i].state[j] == RSEAT_FREE && (wagon == rows[i].wagon || wagon == -1))
                n++;
        }
    }
    return n;
}

// The place of a seat: along its row from the row's start, a unit a seat; null for no such seat
// FUNCTION: GAME 0x482850
S_vector S_RailSeats::GetSeatPos(uint seat)
{
    for (uint i = 0; i < num_rows; i++)
    {
        if (seat >= rows[i].first_seat && seat < rows[i].first_seat + rows[i].num_seats)
        {
            uint s = seat - rows[i].first_seat;
            S_vector d = rows[i].end - rows[i].start;
            S_vector n = d;
            n.Normalize();
            float k = (float)s;
            S_vector r = rows[i].start + n * k;
            return r;
        }
    }
    return S_vector(0.0f, 0.0f, 0.0f);
}

// The direction a seat faces; null for no such seat
// FUNCTION: GAME 0x482a70
S_vector S_RailSeats::GetSeatDir(uint seat)
{
    for (uint i = 0; i < num_rows; i++)
    {
        if (seat >= rows[i].first_seat && seat < rows[i].first_seat + rows[i].num_seats)
            return rows[i].dir;
    }
    return S_vector(0.0f, 0.0f, 0.0f);
}

// The passenger leaves the seat he has reserved or taken
// FUNCTION: GAME 0x482ae0
bool S_RailSeats::EmptySeat(uint seat, void *who)
{
    for (uint i = 0; i < num_rows; i++)
    {
        if (seat >= rows[i].first_seat && seat < rows[i].first_seat + rows[i].num_seats)
        {
            uint s = seat - rows[i].first_seat;
            if (rows[i].who[s] == who)
            {
                rows[i].state[s] = RSEAT_FREE;
                rows[i].who[s] = NULL;
                return true;
            }
        }
    }
    return false;
}

// The passenger reserves a free seat
// FUNCTION: GAME 0x482b70
bool S_RailSeats::ReserveSeat(uint seat, void *who)
{
    for (uint i = 0; i < num_rows; i++)
    {
        if (seat >= rows[i].first_seat && seat < rows[i].first_seat + rows[i].num_seats)
        {
            uint s = seat - rows[i].first_seat;
            if (rows[i].state[s] == RSEAT_FREE)
            {
                rows[i].state[s] = RSEAT_RESERVED;
                rows[i].who[s] = who;
                return true;
            }
        }
    }
    return false;
}

// The passenger sits on the seat (a free one is his then, a reserved one must be his); true when he has it
// FUNCTION: GAME 0x482bf0
bool S_RailSeats::FullSeat(uint seat, void *who)
{
    for (uint i = 0; i < num_rows; i++)
    {
        if (seat >= rows[i].first_seat && seat < rows[i].first_seat + rows[i].num_seats)
        {
            uint s = seat - rows[i].first_seat;
            if (rows[i].who[s] == who)
            {
                switch (rows[i].state[s])
                {
                case RSEAT_FREE:
                    rows[i].who[s] = who;
                    rows[i].state[s] = RSEAT_TAKEN;
                    return true;
                case RSEAT_RESERVED:
                    if (rows[i].who[s] != who)
                        return false;
                    rows[i].state[s] = RSEAT_TAKEN;
                    return true;
                case RSEAT_TAKEN:
                    return rows[i].who[s] == who ? true : false;
                }
            }
        }
    }
    return false;
}

// Appends a row
// FUNCTION: GAME 0x482ce0
void S_RailSeats::AddRow(rail_seat_row *row)
{
    rail_seat_row *new_rows = new rail_seat_row[num_rows + 1];
    if (num_rows)
        memcpy(new_rows, rows, num_rows * sizeof(rail_seat_row));
    rail_seat_row *dst = &new_rows[num_rows++];
    *dst = *row;
    delete[] rows;
    rows = new_rows;
}

// Repeats the rows for each of the wagons (the rows of wagon 0 first, then those of wagon 1, ...)
// FUNCTION: GAME 0x482d60
void S_RailSeats::Duplicate(uint wagons)
{
    rail_seat_row *new_rows = new rail_seat_row[num_rows * wagons];
    uint k = 0;
    if (rows)
    {
        for (uint w = 0; w < wagons; w++)
        {
            for (uint i = 0; i < num_rows; i++)
                {
                rail_seat_row *dst = &new_rows[k++];
                *dst = rows[i];
            }
        }
        delete[] rows;
    }
    rows = new_rows;
    num_rows *= wagons;
}

// Lays the rows out for the vehicle's wagons: each row's seats (its length in units) get their number, wagon and
// the direction across the row, towards the origin's side of the row's plane, all free
// FUNCTION: GAME 0x482e20
void S_RailSeats::Init(C_railway *railway)
{
    uint rows_per_wagon = num_rows;
    Duplicate(railway->num_wagons + 1);
    uint seat = 0;
    for (uint i = 0; i < num_rows; i++)
    {
        S_vector d = rows[i].end - rows[i].start;
        S_vector n = d;
        float len = d.Magnitude();
        n.Normalize();
        uint wagon = i / rows_per_wagon;
        S_vector up(0.0f, 1.0f, 0.0f);
        S_vector across = n.Cross(up);
        across.Normalize();
        S_vector origin(0.0f, 0.0f, 0.0f);
        float side = -across.Dot(rows[i].start);
        if (side + across.Dot(origin) < 0.0f)
            across *= -1.0f;
        rows[i].wagon = wagon;
        rows[i].dir = across;
        rows[i].first_seat = seat;
        rows[i].num_seats = (uint)len;
        if (!rows[i].num_seats)
            rows[i].num_seats = 1;
        rows[i].state = new uint[rows[i].num_seats];
        rows[i].who = (void **)new uint[rows[i].num_seats];
        seat += rows[i].num_seats;
        for (uint j = 0; j < rows[i].num_seats; j++)
        {
            rows[i].state[j] = RSEAT_FREE;
            rows[i].who[j] = NULL;
        }
    }
}

// Frees the rows and their seats
// FUNCTION: GAME 0x483290
void S_RailSeats::Destroy()
{
    for (uint i = 0; i < num_rows; i++)
    {
        delete[] rows[i].state;
        delete[] rows[i].who;
    }
    delete[] rows;
}

// FUNCTION: GAME 0x4832e0
S_rail_human::S_rail_human()
{
    fear.Reset(0);
    old_state = RH_DEAD;
    collision_data = NULL;
}

// Turns the passenger toward the direction at 2 radians a second, the turn animation on the way: false while turning,
// true once it faces it
// FUNCTION: GAME 0x483310
bool S_rail_human::RotateTo(S_vector pos, uint time)
{
    S_vector dir;
    dir = frame_model->GetDir();
    float angle = AngleTo2(S_vector2(pos.x, pos.z), S_vector2(dir.x, dir.z));
    new_animation_number = angle > 0.0f ? RANIM_RIGHT : RANIM_LEFT;
    float step = (angle > 0.0f ? -2.0f : 2.0f) * (time * .001f);
    if (fabs(step) > fabs(angle))
    {
        new_animation_number = RANIM_BREATH;
        frame_model->SetDir(pos, 0.0f);
        return false;
    }
    RotateDir(dir, step);
    frame_model->SetDir(dir, 0.0f);
    return true;
}

// Walks the passenger toward the place: turns toward it first (the turn animations, 2 radians a second), walks
// when it nearly faces it (1.7 times the time in meters); false once there
// FUNCTION: GAME 0x483580
bool S_rail_human::MoveTo(S_vector pos, uint time)
{
    S_vector flat;
    const S_vector &p = frame_model->GetPos();
    S_vector dir;
    dir.x = pos.x - p.x;
    dir.y = pos.y - p.y;
    dir.z = pos.z - p.z;
    S_vector move_dir = dir;
    float dist = dir.Magnitude();
    if (dist < 0.01)
    {
        new_animation_number = RANIM_BREATH;
        return false;
    }
    move_dir = move_dir * (1.0f / dist);
    dir = frame_model->GetDir();
    float angle = AngleTo2(S_vector2(move_dir.x, move_dir.z), S_vector2(dir.x, dir.z));
    float t;
    if (angle > 0.0f)
    {
        if (angle > .034906585f)
        {
            new_animation_number = RANIM_RIGHT;
            float step = -2.0f * (time * .001f);
            if (fabs(step) > fabs(angle))
            {
                new_animation_number = RANIM_WALK;
                flat = move_dir;
                flat.y = 0.0f;
                flat.Normalize();
                frame_model->SetDir(flat, 0.0f);
                return true;
            }
            RotateDir(dir, step);
            flat = dir;
            flat.y = 0.0f;
            flat.Normalize();
            frame_model->SetDir(flat, 0.0f);
            return true;
        }
        new_animation_number = RANIM_WALK;
        t = time * .001f;
        float step = t * -2.0f;
        if (fabs(step) > fabs(angle))
            dir = move_dir;
        else
            RotateDir(dir, step);
        flat = dir;
        flat.y = 0.0f;
        flat.Normalize();
        frame_model->SetDir(flat, 0.0f);
    }
    else
    {
        if (angle < -.034906585f)
        {
            new_animation_number = RANIM_LEFT;
            float step = 2.0f * (time * .001f);
            if (fabs(step) > fabs(angle))
            {
                new_animation_number = RANIM_WALK;
                flat = move_dir;
                flat.y = 0.0f;
                flat.Normalize();
                frame_model->SetDir(flat, 0.0f);
                return true;
            }
            RotateDir(dir, step);
            flat = dir;
            flat.y = 0.0f;
            flat.Normalize();
            frame_model->SetDir(flat, 0.0f);
            return true;
        }
        new_animation_number = RANIM_WALK;
        t = time * .001f;
        float step = t * -2.0f;
        if (fabs(step) > fabs(angle))
            dir = move_dir;
        else
            RotateDir(dir, step);
        flat = dir;
        flat.y = 0.0f;
        flat.Normalize();
        frame_model->SetDir(flat, 0.0f);
    }
    if (t * 1.7f < dist)
    {
        dir *= t * 1.7f;
        dir += frame_model->GetPos();
        frame_model->SetPos(dir);
        new_animation_number = RANIM_WALK;
        return true;
    }
    frame_model->SetPos(pos);
    new_animation_number = RANIM_BREATH;
    return false;
}

// The passengers' animations (PS2 ranims), RANIM_*'s index: the animation, use_track and load_track (flag1) and once
// (flag2) of C_animations_machine_de_luxe::SetAnim
// GLOBAL: GAME 0x6345c0
S_rail_anim ranims[27] = {
    // STRING: GAME 0x634870
    {RANIM_BREATH, "breath1.i3d", false, false},
    {RANIM_WALK, "walk1.i3d", false, false},
    {RANIM_SALINA_NASTUP, "salina nastup.i3d", true, true},
    {RANIM_SALINA_VYSTUP, "salina vystup.i3d", true, true},
    // STRING: GAME 0x634864
    {RANIM_SED, "x sed1.I3D", false, false},
    {RANIM_RIGHT, "right1.i3d", false, false},
    {RANIM_LEFT, "left1.i3d", false, false},
    {RANIM_SMRT_STOJ, "smrt stoj zada A.I3D", false, true},
    {RANIM_SMRT_STOJ + 1, "smrt stoj bricho A.i3d", false, true},
    {RANIM_SMRT_STOJ + 2, "smrt stoj hlava otocka.i3d", false, true},
    {RANIM_SMRT_STOJ + 3, "smrt stoj bricho C.I3D", false, true},
    {RANIM_SMRT_STOJ + 4, "smrt stoj zada B.I3D", false, true},
    // STRING: GAME 0x634850
    {RANIM_SMRT_ZIDLE, "x smrt zidle01.I3D", false, true},
    // STRING: GAME 0x63483c
    {RANIM_SMRT_ZIDLE + 1, "x smrt zidle02.I3D", false, true},
    // STRING: GAME 0x63482c
    {RANIM_SALINDRIVER, "salindriver.I3D", false, false},
    // STRING: GAME 0x634818
    {RANIM_SED_PANIKA, "x sed panika01.I3D", false, false},
    // STRING: GAME 0x634804
    {RANIM_SED_PANIKA + 1, "x sed panika02.I3D", false, false},
    // STRING: GAME 0x6347e8
    {RANIM_PANIKA_STOJ, "panika stoj stativ01a.I3D", false, true},
    // STRING: GAME 0x6347cc
    {RANIM_PANIKA_STOJ + 1, "panika stoj stativ01b.I3D", false, true},
    // STRING: GAME 0x6347b0
    {RANIM_PANIKA_STOJ + 2, "panika stoj stativ01c.I3D", false, true},
    // STRING: GAME 0x634794
    {RANIM_PANIKA_STOJ + 3, "panika stoj stativ02a.I3D", false, true},
    // STRING: GAME 0x634778
    {RANIM_PANIKA_STOJ + 4, "panika stoj stativ02b.I3D", false, true},
    // STRING: GAME 0x63475c
    {RANIM_PANIKA_STOJ + 5, "panika stoj stativ02c.I3D", false, true},
    // STRING: GAME 0x634748
    {RANIM_STROJ_TRAM_SMRT, "StrojTramSmrt.I3D", false, true},
    // STRING: GAME 0x634734
    {RANIM_STROJ_TRAM_SMRT + 1, "StrojTramSmrt02.I3D", false, true},
    // STRING: GAME 0x634724
    {RANIM_ZEN_SALSEZ, "ZenSalsez02.I3D", false, true},
    // STRING: GAME 0x634714
    {RANIM_ZEN_SALSEZ + 1, "ZenSalsez01.I3D", false, true},
};

// Starts new_animation_number if it isn't the one playing, runs the animations for the time and ends the boarding
// or alighting one's wait
// FUNCTION: GAME 0x4841a0
void S_rail_human::SetAnim(uint time)
{
    if (new_animation_number != animation_number && anim_container)
        anim_container->SetAnim(ranims[new_animation_number].file, false, false, ranims[new_animation_number].flag2,
            4.0f, 0.0f, ranims[new_animation_number].flag1, ranims[new_animation_number].flag1, NULL, true, (uint)-1);
    if (anim_container && anim_container->IsActive() && (anim_container->Tick(time) & ANIMTICK_END))
        anim_running = false;
    if (frame_model)
        frame_model->Tick(time);
    frame_model->Update();
    animation_number = new_animation_number;
}

// FUNCTION: GAME 0x484230
void S_rail_human::Kill(C_railway *tram)
{
    // Panicking: back to the state it saved, so the death fits what it was doing
    if (state == RH_PANIC)
        state = old_state;

    switch (state)
    {
    case RH_CHOOSE_ENTRY:
    case RH_TO_ENTRY:
    case RH_TURN_TO_DOOR:
    case RH_BOARD_ANIM:
    case RH_WALK_IN:
    case RH_LINK:
    case RH_STAND_UP:
    case RH_TO_EXIT:
    case RH_TURN_TO_EXIT:
    case RH_ALIGHT_ANIM:
    case RH_WALK_OUT:
    case RH_UNLINK:
    case RH_WALK_AWAY:
        // On the way in or out: the lock it holds, which the update would give back at the end of RH_LINK or
        // RH_WALK_AWAY (in RH_CHOOSE_ENTRY, and in RH_TO_EXIT straight from RH_FIND_SEAT, it has none yet)
        tram->Unlock();
        // fall through
    case RH_FIND_SEAT:
    case RH_TO_SEAT:
    case RH_SIT_DOWN:
        new_animation_number = rnd(5) + RANIM_SMRT_STOJ;
        break;
    case RH_SITTING:
    case RH_SCARED_SITTING:
        new_animation_number = rnd(2) + RANIM_SMRT_ZIDLE;
        break;
    case RH_DRIVER:
        new_animation_number = rnd(2) + RANIM_STROJ_TRAM_SMRT;
        break;
    default: // already dying or dead: the animation stays
        break;
    }

    state = RH_DYING;
    // Starts the death animation
    SetAnim(1);
}

// Makes the passenger's collision, a box of the model's bounding volume at its place, and puts it into the grid
// FUNCTION: GAME 0x4842d0
void S_rail_human::CreateColl(C_railway *tram)
{
    I3D_bvolume bvol = frame_model->GetHRBoundVolume();
    collision_data = new tDynamicCollObject;
    collision_data->type = DYNCOLL_DYNAMIC;
    collision_data->frame = frame_model;
    collision_data->owner = (C_actor *)tram;
    ((I3D_bsphere &)collision_data->bsphere).radius = bvol.bsphere.radius;
    (I3D_bbox &)collision_data->bbox = bvol.bbox;
    ((I3D_bsphere &)collision_data->bsphere).pos = bvol.bsphere.pos * frame_model->GetMatrix();
    collision_data->flags = DYNCOLLF_LOWER;
    collision_data->matrix = (S_matrix *)&frame_model->GetMatrix();
    inverse_global_matrix = frame_model->GetMatrix();
    inverse_global_matrix.Invert();
    collision_data->inv_matrix = &inverse_global_matrix;
    gifCollision.DynInsert(collision_data);
}

// Moves the collision with the model
// FUNCTION: GAME 0x484430
void S_rail_human::UpdateColl()
{
    if (collision_data)
    {
        I3D_bvolume bvol = frame_model->GetHRBoundVolume();
        ((I3D_bsphere &)collision_data->bsphere).pos = bvol.bsphere.pos * frame_model->GetMatrix();
        inverse_global_matrix = frame_model->GetMatrix();
        inverse_global_matrix.Invert();
        gifCollision.DynUpdate(collision_data);
    }
}

// Deletes the animations and the collision
// FUNCTION: GAME 0x484500
void S_rail_human::Done()
{
    if (anim_container)
    {
        delete anim_container;
        anim_container = NULL;
    }
    if (collision_data)
    {
        gifCollision.DynErase(collision_data);
        collision_data = NULL;
    }
}

// A sitting passenger is scared where it sits, one in another state panics (saving it) until the time passes
// FUNCTION: GAME 0x484550
void S_rail_human::DoPanic(uint time)
{
    switch (state)
    {
    case RH_FIND_SEAT:
    case RH_TO_SEAT:
    case RH_SIT_DOWN:
    case RH_STAND_UP:
    case RH_TO_EXIT:
    case RH_TURN_TO_EXIT:
    case RH_ALIGHT_ANIM:
    case RH_WALK_OUT:
    case RH_UNLINK:
        old_state = state;
        state = RH_PANIC;
        new_animation_number = rnd(6) + RANIM_PANIKA_STOJ;
        old_animation_number = animation_number;
        fear.Reset(time);
        SetAnim(1);
        break;
    case RH_SITTING:
        state = RH_SCARED_SITTING;
        new_animation_number = rnd(2) + RANIM_SED_PANIKA;
        fear.Reset(time);
        SetAnim(1);
        break;
    }
}

// Updates the passengers: each one's state machine (RH_*), then its animation and collision
// FUNCTION: GAME 0x4845f0
void C_railway::trafficAI(uint time)
{
    uint i;
    for (i = 0; i < persons.size(); i++)
    {
        S_rail_human *p = persons[i];
        switch (p->state)
        {
        case RH_CHOOSE_ENTRY:
            if (p->animation_number != -1)
            {
                if (!GetOuterEntry(p->animation_number, p->wagon_index, &p->target_position, &p->look_target))
                    GetOuterEntry(p->frame_model->GetWorldPos(), &p->target_position, &p->look_target,
                        &p->wagon_index);
                p->animation_number = -1;
                p->state = RH_TO_ENTRY;
                Lock();
            }
            else
            {
                GetOuterEntry(p->frame_model->GetWorldPos(), &p->target_position, &p->look_target,
                    &p->wagon_index);
                p->state = RH_TO_ENTRY;
                Lock();
            }
            break;
        case RH_TO_ENTRY:
            if (!p->MoveTo(p->target_position, time))
                p->state = RH_TURN_TO_DOOR;
            break;
        case RH_TURN_TO_DOOR:
            if (p->RotateTo(p->look_target, time))
                break;
            if (metro)
            {
                p->state = RH_WALK_IN;
                p->new_animation_number = 1;
                p->target_position = p->frame_model->GetPos() + p->frame_model->GetDir();
            }
            else
            {
                p->state = RH_BOARD_ANIM;
                p->new_animation_number = 2;
                p->anim_running = true;
            }
            break;
        case RH_BOARD_ANIM:
            if (!p->anim_running)
                p->state = RH_LINK;
            break;
        case RH_WALK_IN:
            if (!p->MoveTo(p->target_position, time))
                p->state = RH_LINK;
            break;
        case RH_LINK:
        {
            I3D_frame *wagon_frame = p->wagon_index ? wagon_frames[p->wagon_index - 1] : frame;
            const S_matrix &wm = wagon_frame->GetMatrix();
            S_matrix m = wm;
            m.Inverse(S_matrix(wm));
            S_matrix anim_m = m;
            S_vector pos = p->frame_model->GetWorldPos();
            S_vector dir = p->frame_model->GetWorldDir();
            dir.y = 0.0f;
            dir.Normalize();
            pos *= m;
            m(3).x = m(3).y = m(3).z = 0.0f;
            dir *= m;
            p->frame_model->LinkTo(wagon_frame, 0);
            p->frame_model->SetPos(pos);
            p->frame_model->SetDir(dir, 0.0f);
            p->frame_model->Update();
            p->anim_container->RecompileMatrix(anim_m);
            p->new_animation_number = 1;
            p->state = RH_FIND_SEAT;
            S_vector down(0.0f, -0.6f, 0.0f);
            const S_vector &fp = p->frame_model->GetPos();
            S_vector from(fp.x, fp.y + 0.3f, fp.z);
            S_vector hit;
            if (gifCollision.TestLineInVehicle(&dyn_colls[p->wagon_index], from, down, &hit, NULL, 2))
                p->frame_model->SetPos(hit);
            p->target_position.x = 0.0f;
            p->target_position.y = hit.y;
            p->target_position.z = p->frame_model->GetPos().z;
            p->look_target = p->target_position - p->frame_model->GetPos();
            p->look_target.Normalize();
            for (i = 0; i < person_colls.size(); i++)
            {
                if (person_colls[i]->frame == p->frame_model)
                {
                    gifCollision.DynErase(person_colls[i]);
                    person_colls.erase(person_colls.begin() + i);
                    break;
                }
            }
            if (p->collision_data)
            {
                gifCollision.DynErase(p->collision_data);
                p->collision_data = NULL;
            }
            Unlock();
            break;
        }
        case RH_FIND_SEAT:
        {
            if (p->MoveTo(p->target_position, time))
                break;
            S_vector pos = p->frame_model->GetPos();
            int seat = seats.FindFreeSeat(p->wagon_index, &pos, center_z);
            if (seat != 0xffff)
            {
                I3D_frame *frm = p->frame_model;
                p->target_position.x = 0.0f;
                p->target_position.y = frm->GetPos().y;
                p->target_position.z = seats.GetSeatPos(seat).z;
                seats.ReserveSeat(seat, frm);
                p->seat_index = seat;
                p->state = RH_TO_SEAT;
                S_vector down(0.0f, -0.6f, 0.0f);
                const S_vector &fp = frm->GetPos();
                S_vector from(fp.x, fp.y + 0.3f, fp.z);
                S_vector hit;
                if (gifCollision.TestLineInVehicle(&dyn_colls[p->wagon_index], from, down, &hit, NULL, 2))
                    frm->SetPos(hit);
            }
            else
            {
                GetInnerEntry(p->frame_model->GetPos(), &p->target_position, &p->look_target, NULL);
                p->state = RH_TO_EXIT;
            }
            break;
        }
        case RH_TO_SEAT:
            if (p->MoveTo(p->target_position, time))
                break;
            p->target_position = seats.GetSeatPos(p->seat_index);
            p->target_position.y = p->frame_model->GetPos().y;
            p->look_target = seats.GetSeatDir(p->seat_index);
            if (p->target_position.x < 0.0f)
                p->target_position.x = -0.5f;
            else
                p->target_position.x = 0.5f;
            p->state = RH_SIT_DOWN;
            break;
        case RH_SIT_DOWN:
            if (p->MoveTo(p->target_position, time))
                break;
            if (p->RotateTo(p->look_target, time))
                break;
            p->state = RH_SITTING;
            if (p->IsWoman())
                p->new_animation_number = rnd(2) + RANIM_ZEN_SALSEZ;
            else
                p->new_animation_number = RANIM_SED;
            seats.FullSeat(p->seat_index, p->frame_model);
            break;
        case RH_SITTING:
            if (p->should_keep_sitting)
                break;
            seats.EmptySeat(p->seat_index, p->frame_model);
            p->target_position = p->frame_model->GetPos() + seats.GetSeatDir(p->seat_index) * 0.5f;
            p->target_position.y = p->frame_model->GetPos().y;
            p->state = RH_STAND_UP;
            Lock();
            break;
        case RH_STAND_UP:
            if (p->MoveTo(p->target_position, time))
                break;
            GetInnerEntry(p->frame_model->GetPos(), &p->target_position, &p->look_target, NULL);
            p->target_position -= p->look_target * 0.5f;
            p->target_position.y = p->frame_model->GetPos().y;
            p->state = RH_TO_EXIT;
            break;
        case RH_TO_EXIT:
            if (p->MoveTo(p->target_position, time))
                break;
            p->target_position += p->look_target * 0.5f;
            p->target_position.y = p->frame_model->GetPos().y;
            p->state = RH_TURN_TO_EXIT;
            break;
        case RH_TURN_TO_EXIT:
            if (p->RotateTo(p->look_target, time))
                break;
            if (metro)
            {
                p->state = RH_WALK_OUT;
                p->new_animation_number = 1;
                p->target_position = p->frame_model->GetPos() + p->frame_model->GetDir();
            }
            else
            {
                p->state = RH_ALIGHT_ANIM;
                p->new_animation_number = 3;
            }
            break;
        case RH_ALIGHT_ANIM:
            if (!p->anim_running)
                p->state = RH_UNLINK;
            break;
        case RH_WALK_OUT:
            if (!p->MoveTo(p->target_position, time))
                p->state = RH_UNLINK;
            break;
        case RH_UNLINK:
        {
            I3D_frame *wagon_frame = p->wagon_index ? wagon_frames[p->wagon_index - 1] : frame;
            S_matrix m = wagon_frame->GetMatrix();
            S_matrix anim_m = m;
            S_vector pos = p->frame_model->GetPos();
            S_vector dir = p->frame_model->GetDir();
            pos *= m;
            m(3).x = m(3).y = m(3).z = 0.0f;
            dir *= m;
            p->frame_model->LinkTo(act_mission->scene->primary_sector, 0);
            p->frame_model->SetPos(pos);
            p->frame_model->SetDir(dir, 0.0f);
            p->frame_model->Update();
            p->anim_container->RecompileMatrix(anim_m);
            act_mission->scene->SetFrameSectorPos(wagon_frame, wagon_frame->GetWorldPos());
            p->state = RH_WALK_AWAY;
            p->new_animation_number = 1;
            p->target_position = pos + dir;
            break;
        }
        case RH_WALK_AWAY:
            if (p->MoveTo(p->target_position, time))
                break;
            RemovePerson(p);
            i--;
            Unlock();
            continue;
        case RH_DYING:
        {
            I3D_frame *neck = p->frame_model->FindChildFrame("neck", ENUMF_ALL);
            I3D_frame *foot = p->frame_model->FindChildFrame("r_foot", ENUMF_ALL);
            S_vector np = neck->GetPos();
            I3D_frame *f;
            for (f = neck->GetParent(); f != p->frame_model; f = f->GetParent())
                np *= f->GetLocalMatrix();
            np *= f->GetLocalMatrix();
            S_vector fp = foot->GetPos();
            for (f = foot->GetParent(); f != p->frame_model; f = f->GetParent())
                fp *= f->GetLocalMatrix();
            fp *= f->GetLocalMatrix();
            S_vector d = np - fp;
            d.Normalize();
            np.x = d.x * 0.2f + np.x;
            np.y = d.y * 0.2f + np.y;
            np.z = d.z * 0.2f + np.z;
            float dx = 0.0f;
            float dz = 0.0f;
            if (np.x > bound.bbox.max.x)
                dx = bound.bbox.max.x - np.x;
            else if (np.x < bound.bbox.min.x)
                dx = bound.bbox.min.x - np.x;
            if (np.z > bound.bbox.max.z)
                dz = bound.bbox.max.z - np.z;
            else if (np.z < bound.bbox.min.z)
                dz = bound.bbox.min.z - np.z;
            p->frame_model->SetPos(dx + p->frame_model->GetPos().x, p->frame_model->GetPos().y,
                dz + p->frame_model->GetPos().z);
            p->frame_model->Update();
            uint t, length;
            p->anim_container->GetActiveAnimTime(t, length);
            if (t + 250 > length)
            {
                p->state = RH_DEAD;
                if (&driver == p)
                    p->state = RH_DRIVER_DEAD;
            }
            break;
        }
        case RH_SCARED_SITTING:
            if (p->fear.Tick(time))
            {
                p->state = RH_SITTING;
                if (p->IsWoman())
                    p->new_animation_number = rnd(2) + RANIM_ZEN_SALSEZ;
                else
                    p->new_animation_number = RANIM_SED;
            }
            break;
        case RH_PANIC:
        {
            uint t, length;
            p->anim_container->GetActiveAnimTime(t, length);
            if (t + 250 > length && p->new_animation_number == p->animation_number)
            {
                do
                {
                    p->new_animation_number = rnd(6) + RANIM_PANIKA_STOJ;
                } while (p->new_animation_number == p->animation_number);
            }
            if (p->fear.Tick(time))
            {
                p->state = p->old_state;
                p->new_animation_number = p->old_animation_number;
            }
            break;
        }
        }
        p->SetAnim(time);
        p->UpdateColl();
    }
}

// Tells a woman by the first letter of the property of the model's "back1" frame ("B...")
// FUNCTION: GAME 0x4862b0
bool S_rail_human::IsWoman()
{
    I3D_frame *back = frame_model->FindChildFrame("back1", ENUMF_ALL);
    if (back)
    {
        char prop[64];
        strcpy(prop, back->GetProperty());
        if (strlen(prop) > 3)
        {
            strupr(prop);
            char c = prop[0] - 'A';
            if (c == 1)
                return true;
        }
    }
    return false;
}

// Whether a passenger stands ready to leave
// FUNCTION: GAME 0x486320
bool C_railway::WantLeave()
{
    bool ok;
    if (visibility == 0 || visibility == 2)
        ok = true;
    if (!ok)
        return false;
    for (uint i = 0; i < persons.size(); i++)
    {
        S_rail_human *p = persons[i];
        if (p->state == RH_DYING || p->state == RH_DEAD || p->should_keep_sitting)
            continue;
        return true;
    }
    return false;
}

// Whether a passenger stands ready to board
// FUNCTION: GAME 0x486380
bool C_railway::WantEnter()
{
    bool ok;
    if (visibility == 0 || visibility == 2)
        ok = true;
    if (!ok)
        return false;
    for (uint i = 0; i < persons.size(); i++)
    {
        S_rail_human *p = persons[i];
        if ((uint)p->state < RH_FIND_SEAT)
            return true;
    }
    return false;
}

// What PreduceMovement looks out for besides the cars (the names are placeholders), moving things with a position and a
// velocity, the latter 6 seconds' worth
struct S_unknown_63d7d8
{
    uchar unknown_0[0x10];              // 0x0
    S_vector pos;                       // 0x10
    S_vector vel;                       // 0x1c
};

// Their list (the name is a placeholder; not a class member: its destructor has no guard)
// GLOBAL: GAME 0x63d7dc
static std::vector<S_unknown_63d7d8 *> unknown_63d7d8;

// GLOBAL: GAME 0x63d7c8
std::vector<S_shedule_item *> C_railway::shedule_table;

// GLOBAL: GAME 0x63d7b4
std::vector<S_rail_semaphore *> C_railway::semaphores;

// GLOBAL: GAME 0x63d79c
std::vector<C_railway *> C_railway::rlws;

// GLOBAL: GAME 0x63d7b0
I3D_dummy *C_railway::rain_dummy;

// GLOBAL: GAME 0x63d7ac
bool C_railway::salina_v_kine;

// FUNCTION: GAME 0x4865b0
C_railway::C_railway()
{
    collided = false;
    type = ACTOR_RAILWAY;
    rlw_state = 0;
    num_wagons = 1;
    wagon_length = 17.0f;
    vehicle_length = 8.0f;
    max_speed = 12.5f;
    acceleration = 3.0f;
    vehicle_mass = 10000.0f;
    inv_mass = 1.0f / vehicle_mass;
    state_time = 10000;
    driver.anim_container = NULL;
    driver.frame_model = NULL;
    salina_v_kine = false;
    sounds[0] = NULL;
    sounds[1] = 0;
    sounds[2] = NULL;
    semaphore_phase = 0;
    ahead = 0;
    coll_frame = 0;
}

// Stops the model's animation and clears the timer; the vectors free themselves
// FUNCTION: GAME 0x486760
C_railway::~C_railway()
{
    if (frame && frame->GetType() == FRAME_MODEL)
        ((I3D_model *)frame)->StopAnimation(0);
    horn_timer.Reset(0);
}

// Takes a model frame only
// FUNCTION: GAME 0x4868c0
bool C_railway::Init(I3D_frame *frm)
{
    if (frm && frm->GetType() != FRAME_MODEL)
        return false;
    return C_actor::Init(frm) ? true : false;
}

// FUNCTION: GAME 0x4868f0
void C_railway::DuplicateFrom(C_actor *act)
{
    C_actor::DuplicateFrom(act);
    num_wagons = ((C_railway *)act)->num_wagons;
    max_speed = ((C_railway *)act)->max_speed;
}

// Saves the wagons' count and the five floats after it (the file's version byte first)
// FUNCTION: GAME 0x486920
bool C_railway::SaveData(C_chunk *chunk)
{
    uchar version = 0;
    chunk->Write(&version, sizeof(version));
    chunk->Write(&num_wagons, sizeof(num_wagons));
    chunk->Write(&wagon_length, sizeof(wagon_length));
    chunk->Write(&vehicle_length, sizeof(vehicle_length));
    chunk->Write(&max_speed, sizeof(max_speed));
    chunk->Write(&acceleration, sizeof(acceleration));
    chunk->Write(&vehicle_mass, sizeof(vehicle_mass));
    return true;
}

// Loads what SaveData saves
// FUNCTION: GAME 0x486a20
bool C_railway::LoadData(C_chunk *chunk)
{
    uchar version;
    chunk->Read(&version, sizeof(version));
    chunk->Read(&num_wagons, sizeof(num_wagons));
    chunk->Read(&wagon_length, sizeof(wagon_length));
    chunk->Read(&vehicle_length, sizeof(vehicle_length));
    chunk->Read(&max_speed, sizeof(max_speed));
    chunk->Read(&acceleration, sizeof(acceleration));
    chunk->Read(&vehicle_mass, sizeof(vehicle_mass));
    return true;
}

inline C_web_node *C_railway::GetCheckpoints()
{
    return act_mission->web_path->nodes;
}

// Extends the back vector with the nodes behind its last one, along the connections of the closed way, until it
// has 100 of them or three lie farther from pos than back_dist
// FUNCTION: GAME 0x486c30
void C_railway::CreateBackVector(S_vector pos)
{
    uint count = 0;
    while (back_nodes.size() < 100)
    {
        C_web_node *node = &GetCheckpoints()[back_nodes.back()];
        uint i = node->num_conns;
        while (i)
        {
            i--;
            if (node->GetConn(i)->flags & WEBF_ONEWAY)
            {
                uint next = node->GetConn(i)->node;
                back_nodes.push_back(next);
                break;
            }
        }
        float dist = (GetCheckpoints()[back_nodes.back()].GetPos() - pos).Magnitude();
        float limit = back_dist;
        if (dist > limit)
            count++;
        if (count > 2)
            break;
    }
}

// Extends the forward vector from node index, along the connections not closed, until its length passes dist or
// it has 100 nodes
// FUNCTION: GAME 0x486db0
void C_railway::CreateForwardVector(uint index, float dist)
{
    float total = 0.0f;
    uint n = forward_nodes.size();
    if (n)
    {
        S_vector prev = GetCheckpoints()[forward_nodes[0]].GetPos();
        for (uint i = 1; i < n; i++)
        {
            S_vector cur = GetCheckpoints()[forward_nodes[i]].GetPos();
            prev -= cur;
            total += prev.Magnitude();
            prev = GetCheckpoints()[forward_nodes[i]].GetPos();
        }
    }
    if (total <= dist)
    {
        C_web_node *node;
        if (n)
            node = &GetCheckpoints()[forward_nodes[n - 1]];
        else
            node = &GetCheckpoints()[index];
        while (forward_nodes.size() < 100 && total <= dist)
        {
            S_vector prev = node->GetPos();
            uint i = node->num_conns;
            while (i)
            {
                i--;
                if (!(node->GetConn(i)->flags & WEBF_ONEWAY))
                {
                    forward_nodes.push_back(node->GetConn(i)->node);
                    node = &GetCheckpoints()[node->GetConn(i)->node];
                    break;
                }
            }
            total += (prev - node->pos).Magnitude();
        }
    }
}

// The semaphores' lights in each of their ten phases (the name is ours), C_shvestky.cpp's semaphore_colors' copy
// GLOBAL: GAME 0x624a3c
static const int rail_semaphore_colors[10] = {
    SEMAPHORE_RED, SEMAPHORE_RED, SEMAPHORE_RED, SEMAPHORE_RED, SEMAPHORE_RED,
    SEMAPHORE_GREEN, SEMAPHORE_GREEN, SEMAPHORE_GREEN, SEMAPHORE_GREEN,
    SEMAPHORE_ORANGE,
};

// Whether the path node is a stop: the linker folded it with C_wagon's identical copy
// FUNCTION: GAME 0x4f28c0 FOLDED
bool C_railway::IsStopPoint(uint index)
{
    return (act_mission->web_path->nodes[index].flags & WEBF_UNKNOWN_8) ? true : false;
}

// Whether the path node is a crossing's semaphore the vehicle faces at a red light: it takes the phase of the
// direction it moves along
// FUNCTION: GAME 0x487120
bool C_railway::IsSemaPoint(uint index)
{
    C_web_node *node = &act_mission->web_path->nodes[index];
    if (node->flags & 0x800)
    {
        for (uint i = 0; i < semaphores.size(); i++)
        {
            if (semaphores[i]->node == node)
            {
                S_vector a = frame->GetDir();
                S_vector b = frame->GetDir();
                if ((float)fabs(b.x) > (float)fabs(a.z))
                    semaphore_phase = &act_mission->game->semaphore_phase_x;
                else
                    semaphore_phase = &act_mission->game->semaphore_phase_z;
                if (rail_semaphore_colors[*semaphore_phase] & SEMAPHORE_RED)
                    return true;
            }
        }
    }
    return false;
}

// The distance to the next stop or semaphore along the path ahead, -1 for none; flags tells which (8 a stop here,
// 0x800 a semaphore here or a stop further on)
// FUNCTION: GAME 0x487510
float C_railway::MustStop(uint *flags)
{
    *flags = 0;
    if (back_nodes.size() != 0)
    {
        C_web_node *node = &act_mission->web_path->nodes[back_nodes[0]];
        S_vector prev;
        float dist = 0.0f;
        dist += (front_pos - node->GetPos()).Magnitude();
        prev = node->GetPos();
        if (IsStopPoint(back_nodes[0]))
        {
            *flags = 8;
            return dist;
        }
        if (IsSemaPoint(back_nodes[0]))
        {
            *flags = 0x800;
            return dist;
        }
        int count = forward_nodes.size();
        for (int i = 0; i < count; i++)
        {
            node = &act_mission->web_path->nodes[forward_nodes[i]];
            S_vector cur = node->GetPos();
            dist += (prev - cur).Magnitude();
            if (IsStopPoint(forward_nodes[i]))
            {
                *flags = 0x800;
                return dist;
            }
            prev = node->GetPos();
        }
    }
    return -1.0f;
}

// The position dist along the path behind pos, from the node index on, which it moves on to
// FUNCTION: GAME 0x4877a0
S_vector C_railway::FindBackPos(S_vector pos, float dist, int *index)
{
    uint size = back_nodes.size();
    uint i = *index;
    S_vector d;
    for (; i <= size; i++)
    {
        d = act_mission->web_path->nodes[back_nodes[i]].GetPos() - pos;
        if (d.Magnitude() > dist + 0.001f)
            break;
    }
    if (i == size)
        i--;
    S_vector result;
    if (i == *index)
    {
        d.Normalize();
        result = pos + d * dist;
    }
    else
    {
        uint *p = &back_nodes[i];
        C_mission *mission = act_mission;
        S_vector a = mission->web_path->nodes[p[-1]].GetPos() - pos;
        S_vector b = mission->web_path->nodes[*p].GetPos() - pos;
        S_vector c = mission->web_path->nodes[*p].GetPos() - mission->web_path->nodes[p[-1]].GetPos();
        float len_a = a.Magnitude();
        float len_b = b.Magnitude();
        float delta = len_b - len_a;
        if (delta < MRG_ZERO)
        {
            result = mission->web_path->nodes[*p].GetPos();
        }
        else
        {
            float t = (dist - len_a) / delta;
            S_vector v = c * t;
            result = mission->web_path->nodes[p[-1]].GetPos() + v;
            *index = i;
        }
    }
    return result;
}

// Turns the wagon's spring angle on and swings the two ends p1 and p2 by it: out is their middle, dir their
// direction, normalized
// FUNCTION: GAME 0x487d30
void C_railway::SpringProcess(uint time, int index, S_vector *out, S_vector *dir, S_vector p1, S_vector p2)
{
    if (drive_speed < MRG_ZERO)
        return;
    I3D_frame *frm = index == 0 ? frame : wagon_frames[index - 1];
    S_matrix m = frm->GetMatrix();
    m(3).x = 0.0f;
    m(3).y = 0.0f;
    m(3).z = 0.0f;
    float k = drive_speed / max_speed;
    wagon_angles[index] += (float)time / (1000.0f / (12.0f * k));
    if (wagon_angles[index] > PI * 2.0f)
        wagon_angles[index] -= PI * 2.0f;
    S_vector v(1.0f, 0.0f, 0.0f);
    v *= m;
    float s = (float)sin(wagon_angles[index] + PI / 2);
    p1 += v * s * 0.03f * k;
    p2 -= v * s * 0.03f * k;
    *dir = p1 - p2;
    *out = p2 + *dir * 0.5f;
    dir->Normalize();
}

// Sets the near or far state of the vehicle (visibility) from the distance of the position, with the hysteresis the
// two switches give (PS2 name; inlined)
inline void C_railway::CheckDistance(S_vector pos)
{
    if ((pos - front_pos).Square() < 250000.0f)
    {
        switch (visibility)
        {
        case 1:
        case 3:
            visibility = 2;
            break;
        case 2:
            visibility = 0;
            break;
        }
    }
    else
    {
        switch (visibility)
        {
        case 0:
        case 2:
            visibility = 1;
            break;
        case 1:
            visibility = 3;
            break;
        }
    }
}

// The tram's or train's brain: the state machine of its stops (rlw_state). It drives to the speed it wants (what
// GetSafeSpeed allows), slows down for a stop or a signal, waits for its passengers, lets the doors work, starts the
// sounds and keeps the entries the player can board by
// FUNCTION: GAME 0x488140
void C_railway::AI(uint time)
{
    if (salina_v_kine)
        return;
    unknown_e4 = 0;
    CheckDistance(act_mission->scene->GetActiveCamera()->GetWorldPos());
    bool traffic;
    if (visibility == 0 || visibility == 2)
    {
        traffic = true;
        trafficAI(time);
    }
    else
        traffic = false;
    doors.Tick(time);
    switch (rlw_state)
    {
    case RLW_STOPPED:
    {
        if (sounds[0])
            sounds[0]->SetOn(false);
        if (sounds[2])
            sounds[2]->SetOn(false);
        if (act_mission->game->player)
        {
            if (act_mission->game->player->car == (C_car *)this)
                AddInnerEntries();
            else
                AddOuterEntries();
        }
        for (uint pi = 0; pi < persons.size(); pi++)
            persons[pi]->should_keep_sitting--;
        if (metro)
        {
            int index = -1;
            if (act_mission->game->player)
            {
                S_vector pp = act_mission->game->player->pos;
                float dist = (frame->GetPos() - pp).Magnitude();
                for (uint wi = 0; wi < num_wagons; wi++)
                {
                    S_vector pp2 = act_mission->game->player->pos;
                    if ((wagon_frames[wi]->GetPos() - pp2).Magnitude() < dist)
                        index = wi;
                }
            }
            if (sounds[1])
            {
                if (!sounds[1]->IsPlaying(false))
                {
                    if (sound_table.ConstructSound(sounds[1], sound_ids[1]))
                        sounds[1]->LinkTo(index == -1 ? frame : wagon_frames[index], 0);
                    sounds[1]->SetLoop(false);
                    sounds[1]->SetOn(true);
                }
            }
        }
        SetRLWState(RLW_LEAVING);
        break;
    }
    case RLW_LEAVING:
        doors.Open();
        if (!WantLeave())
        {
            SetRLWState(RLW_ENTERING);
            ExportEntries();
        }
        break;
    case RLW_ENTERING:
        doors.Open();
        if (!WantEnter())
            SetRLWState(RLW_DOORS_OPEN);
        break;
    case RLW_DOORS_OPEN:
        doors.Open();
        if (state_time > 10000)
        {
            state_time = 0;
            SetRLWState(RLW_SCHEDULE);
        }
        state_time += time;
        break;
    case RLW_SCHEDULE:
    {
        C_web_node *node = &act_mission->web_path->nodes[back_nodes[0]];
        if (lock_count == 0 && IsOnSchedule(node))
            SetRLWState(RLW_DEPART);
        break;
    }
    case RLW_DEPART:
    {
        if (metro)
        {
            int index = -1;
            if (act_mission->game->player)
            {
                S_vector pp = act_mission->game->player->pos;
                float dist = (frame->GetPos() - pp).Magnitude();
                for (uint wi = 0; wi < num_wagons; wi++)
                {
                    S_vector pp2 = act_mission->game->player->pos;
                    if ((wagon_frames[wi]->GetPos() - pp2).Magnitude() < dist)
                        index = wi;
                }
            }
            if (sounds[1])
            {
                if (!sounds[1]->IsPlaying(false))
                {
                    if (sound_table.ConstructSound(sounds[1], sound_ids[1]))
                        sounds[1]->LinkTo(index == -1 ? frame : wagon_frames[index], 0);
                    sounds[1]->SetOn(true);
                }
            }
        }
        else
        {
            if (sounds[1])
            {
                if (!sounds[1]->IsPlaying(false))
                {
                    if (sound_table.ConstructSound(sounds[1], sound_ids[1]))
                        sounds[1]->SetOn(true);
                }
            }
        }
        doors.Close();
        safe_speed = max_speed;
        drive_speed = 0;
        for (uint zi = 0; zi <= num_wagons; zi++)
        {
            unknown_c8[zi].x = 0;
            unknown_c8[zi].y = 0;
            unknown_c8[zi].z = 0;
            unknown_cc[zi].x = 0;
            unknown_cc[zi].y = 0;
            unknown_cc[zi].z = 0;
        }
        RemoveInnerEntries();
        RemoveOuterEntries();
        state_time = 0;
        SetRLWState(RLW_DRIVE);
        break;
    }
    case RLW_DRIVE:
    {
        state_time += time;
        doors.Close();
        if (sounds[0])
        {
            sounds[0]->SetVolume(drive_speed / max_speed);
            frame->GetDir();
        }
        Place(time);
        if (traffic)
            Collide();
        float speed = 0;
        safe_speed = GetSafeSpeed(speed);
        DoBrousicka();
        if ((unknown_e4 & RLWF_BLOCKED) && traffic)
        {
            drive_speed = 0;
            safe_speed = 0;
            rlw_state = 1;
            state_time = 0;
            return;
        }
        if (traffic)
        {
            uint stop_type;
            float dist = MustStop(&stop_type);
            switch (stop_type)
            {
            case 8:
                if (state_time > 2000 && dist > 0 && !(brake_dist < dist))
                {
                    safe_speed = 0;
                    SetRLWState(RLW_STOP_SLOW);
                }
                break;
            case 0x800:
                if (dist > 0)
                    SetRLWState(RLW_SIGNAL_SLOW);
                break;
            }
        }
        if (PreduceMovement() && traffic)
        {
            safe_speed = 0;
            SetRLWState(RLW_OBSTACLE);
            state_time = 0;
            return;
        }
        if (safe_speed > drive_speed)
        {
            drive_speed += time * acceleration * 0.001f;
            if (drive_speed > safe_speed)
                drive_speed = safe_speed;
        }
        if (safe_speed < drive_speed)
        {
            drive_speed -= time * acceleration * 0.001f;
            if (drive_speed < safe_speed)
                drive_speed = safe_speed;
        }
        break;
    }
    case RLW_OBSTACLE:
        if (!metro && drive_speed == 0 && act_mission->game->player)
        {
            if (act_mission->game->player->car == (C_car *)this)
                AddInnerEntries();
            else
                AddOuterEntries();
        }
        doors.Close();
        if (drive_speed > 0)
        {
            if (sounds[0])
            {
                frame->GetDir();
                sounds[0]->SetVolume(drive_speed / max_speed);
            }
            drive_speed -= time * acceleration * 0.001f;
            if (drive_speed <= 0)
            {
                drive_speed = 0;
                state_time = 0;
            }
            else if (!PreduceMovement() && lock_count == 0)
            {
                RemoveInnerEntries();
                RemoveOuterEntries();
                safe_speed = max_speed;
                state_time = 0;
                SetRLWState(RLW_DRIVE);
            }
        }
        else
        {
            if (sounds[0])
                sounds[0]->SetOn(false);
            state_time += time;
            if (state_time > 5000 && !PreduceMovement())
            {
                RemoveInnerEntries();
                RemoveOuterEntries();
                safe_speed = max_speed;
                state_time = 0;
                SetRLWState(RLW_DRIVE);
            }
        }
        if (!metro && drive_speed == 0 && horn_timer.Tick(time))
        {
            if (sounds[1] && !sounds[1]->IsPlaying(false) && sound_table.ConstructSound(sounds[1], sound_ids[1]))
            {
                sounds[1]->SetOn(true);
                horn_timer.Reset(7000);
            }
        }
        Place(time);
        if (traffic)
            Collide();
        if ((unknown_e4 & RLWF_BLOCKED) && traffic)
        {
            drive_speed = 0;
            safe_speed = 0;
            SetRLWState(RLW_STOPPED);
            state_time = 0;
            return;
        }
        break;
    case RLW_FOLLOW:
    {
        if (!ahead || ahead == this)
            break;
        if (!metro && drive_speed == 0 && act_mission->game->player)
        {
            if (act_mission->game->player->car == (C_car *)this)
                AddInnerEntries();
            else
                AddOuterEntries();
        }
        if (drive_speed > 0)
        {
            if (sounds[0])
            {
                frame->GetDir();
                sounds[0]->SetVolume(drive_speed / max_speed);
            }
            drive_speed -= time * acceleration * 0.001f;
            if (drive_speed <= 0)
            {
                drive_speed = 0;
                state_time = 0;
            }
            I3D_frame *last = ahead->frame;
            if ((int)ahead->num_wagons > 0)
                last = ahead->wagon_frames[ahead->num_wagons - 1];
            if ((frame->GetPos() - last->GetPos()).Square() > 100.0f)
                drive_speed = 0;
        }
        else
        {
            if (sounds[0])
                sounds[0]->SetOn(false);
            state_time += time;
        }
        Place(time);
        if (traffic)
        {
            Collide();
            if (unknown_e4 & RLWF_BLOCKED)
            {
                drive_speed = 0;
                safe_speed = 0;
                SetRLWState(RLW_STOPPED);
                state_time = 0;
                return;
            }
        }
        {
            I3D_frame *last = ahead->frame;
            if ((int)ahead->num_wagons > 0)
                last = ahead->wagon_frames[ahead->num_wagons - 1];
            if ((frame->GetPos() - last->GetPos()).Square() > 2500.0f && lock_count == 0)
            {
                RemoveInnerEntries();
                RemoveOuterEntries();
                safe_speed = max_speed;
                state_time = 0;
                SetRLWState(saved_rlw_state);
            }
        }
        break;
    }
    case RLW_STOP_SLOW:
    {
        if (sounds[0])
        {
            frame->GetDir();
            sounds[0]->SetVolume(drive_speed / max_speed);
        }
        uint stop_type;
        float dist = MustStop(&stop_type);
        if (dist > 0 && stop_type == 8)
        {
            drive_speed = drive_speed - drive_speed * drive_speed / (dist * 2) * time * 0.001f;
            if (drive_speed <= 0)
            {
                state_time = 0;
                safe_speed = 0;
                drive_speed = 0;
                SetRLWState(RLW_STOPPED);
            }
        }
        else
        {
            state_time = 0;
            safe_speed = 0;
            drive_speed = 0;
            SetRLWState(RLW_STOPPED);
        }
        Place(time);
        if (traffic)
            Collide();
        if ((unknown_e4 & RLWF_BLOCKED) && traffic)
        {
            drive_speed = 0;
            safe_speed = 0;
            state_time = 0;
            SetRLWState(RLW_STOPPED);
            return;
        }
        break;
    }
    case RLW_SIGNAL_SLOW:
        if (!metro && drive_speed == 0 && act_mission->game->player)
        {
            if (act_mission->game->player->car == (C_car *)this)
                AddInnerEntries();
            else
                AddOuterEntries();
        }
        if (drive_speed > 0)
        {
            if (sounds[0])
            {
                frame->GetDir();
                sounds[0]->SetVolume(drive_speed / max_speed);
            }
            drive_speed -= time * acceleration * 0.001f;
            if (drive_speed <= 0)
                drive_speed = 0;
        }
        else
        {
            if (sounds[0])
                sounds[0]->SetOn(false);
        }
        Place(time);
        if (traffic)
            Collide();
        if (unknown_e4 & RLWF_STOP)
        {
            drive_speed = 0;
            safe_speed = 0;
        }
        if ((rail_semaphore_colors[*semaphore_phase] & SEMAPHORE_GREEN) && lock_count == 0)
        {
            RemoveInnerEntries();
            RemoveOuterEntries();
            SetRLWState(RLW_DRIVE);
        }
        break;
    case RLW_NO_DRIVER:
        if (sounds[0])
            sounds[0]->SetOn(false);
        if (drive_speed == 0 && act_mission->game->player)
        {
            if (act_mission->game->player->car == (C_car *)this)
                AddInnerEntries();
            else
                AddOuterEntries();
        }
        safe_speed = 0;
        if (drive_speed > 0)
        {
            if (sounds[0])
            {
                frame->GetDir();
                sounds[0]->SetVolume(drive_speed / max_speed);
            }
            drive_speed -= time * acceleration * 0.001f;
            if (drive_speed <= 0)
                drive_speed = 0;
        }
        else
        {
            if (sounds[0])
                sounds[0]->SetOn(false);
        }
        break;
    }
}

// The vehicle's per-frame update: the visibility change visibility asks for (2 shows the vehicle, its wagons, lights,
// radar marks and collisions, 1 hides them and lets the passengers go), then the frames placed where the AI put them
// and the collisions and the doors followed
// FUNCTION: GAME 0x4894d0
void C_railway::Update(uint time)
{
    S_matrix wm;
    I3D_bvolume bvol;
    uint i;
    int w;
    int s;
    bool visible = false;
    if (visibility == 0 || visibility == 2)
    {
        visible = true;
        if (visibility == 2)
        {
            if (rlw_state == 0 && sounds[0])
                sounds[0]->SetOn(true);
            frame->SetOn(true);
            if (!metro)
                mafiaIndicators.RadarAddObject(colls, 0xffffff00);
            gifCollision.DynInsert(colls);
            for (w = 0; w < (int)num_wagons; w++)
            {
                wagon_frames[w]->SetOn(true);
                if (!metro)
                    mafiaIndicators.RadarAddObject(colls + w + 1, 0xffffff00);
                gifCollision.DynInsert(colls + w + 1);
            }
            for (i = 0; i < lights.size(); i++)
                lights[i].frame->SetOn(true);
        }
    }
    if (visibility == 1)
    {
        if (sounds[0])
            sounds[0]->SetOn(false);
        if (sounds[2])
            sounds[2]->SetOn(false);
        frame->SetOn(false);
        if (!metro)
            mafiaIndicators.RadarRemoveObject(colls);
        gifCollision.DynErase(colls);
        for (w = 0; w < (int)num_wagons; w++)
        {
            wagon_frames[w]->SetOn(false);
            if (!metro)
                mafiaIndicators.RadarRemoveObject(colls + w + 1);
            gifCollision.DynErase(colls + w + 1);
        }
        for (i = 0; i < lights.size(); i++)
            lights[i].frame->SetOn(false);
        for (i = 0; i < person_colls.size(); i++)
            gifCollision.DynErase(person_colls[i]);
        person_colls.clear();
        // The passengers on foot or standing leave with the hidden vehicle; the seated and the driver stay, the dead
        // driver goes back to his place
        for (i = 0; i < persons.size(); i++)
        {
            S_rail_human *person = persons[i];
            if (person->state == RH_SITTING || person->state == RH_DRIVER || person->state == RH_DRIVER_DEAD)
            {
                if (person->state == RH_DRIVER_DEAD)
                {
                    // STRING: GAME 0x634884
                    I3D_frame *seat = frame->FindChildFrame("ridic", ENUMF_DUMMY);
                    if (seat)
                    {
                        persons[i]->frame_model->SetPos(seat->GetPos());
                        persons[i]->frame_model->SetRot(seat->GetRot());
                        persons[i]->frame_model->Update();
                    }
                    persons[i]->state = RH_DRIVER;
                    persons[i]->new_animation_number = RANIM_SALINDRIVER;
                }
            }
            else
            {
                seats.EmptySeat(person->seat_index, person);
                for (uint j = 0; j < person_colls.size(); j++)
                {
                    if (person_colls[j]->frame == persons[i]->frame_model)
                    {
                        gifCollision.DynErase(person_colls[j]);
                        person_colls.erase(person_colls.begin() + j);
                        break;
                    }
                }
                persons.erase(persons.begin() + i);
                if (person->frame_model)
                    person->frame_model->Release();
                person->Done();
                delete person;
                i--;
            }
        }
        if (rlw_state == RLW_NO_DRIVER)
        {
            rlw_state = saved_rlw_state;
            RemoveInnerEntries();
            RemoveOuterEntries();
        }
        lock_count = 0;
    }
    if (visible)
    {
        I3D_sound **sound = &sounds[0];
        for (s = 0; s < 3; s++, sound++)
        {
            if (*sound && (*sound)->IsPlaying(false))
                (*sound)->Update();
        }
        if (unknown_298)
            unknown_298->Update();
        frame->SetPos(pos);
        frame->SetDir(dir, 0.0f);
        {
            I3D_frame *frm = frame;
            I3D_scene *scene = act_mission->scene;
            scene->SetFrameSectorPos(frm, frm->GetWorldPos());
        }
        frame->Update();
        if (!coll_frame)
        {
            wm = frame->GetMatrix();
            *inv_matrices = wm;
            inv_matrices->Invert();
            bvol = frame->GetHRBoundVolume();
            ((I3D_bsphere &)colls->bsphere).pos = bvol.bsphere.pos * frame->GetMatrix();
            gifCollision.DynUpdate(colls);
        }
        else
        {
            ((I3D_bsphere &)colls->bsphere).pos = coll_frame->GetWorldPos();
            coll_matrix = coll_frame->GetMatrix();
            coll_matrix(0) *= 1.0f / coll_frame->GetWorldScale().x;
            coll_matrix(1) *= 1.0f / coll_frame->GetWorldScale().y;
            coll_matrix(2) *= 1.0f / coll_frame->GetWorldScale().z;
            *inv_matrices = coll_matrix;
            inv_matrices->Invert();
            gifCollision.DynUpdate(colls);
        }
        for (i = 0; i < persons.size(); i++)
        {
            for (uint j = 0; j < person_colls.size(); j++)
            {
                if (persons[i]->frame_model == person_colls[j]->frame)
                {
                    I3D_bvolume bvol = person_colls[j]->frame->GetHRBoundVolume();
                    ((I3D_bsphere &)person_colls[j]->bsphere).pos =
                        bvol.bsphere.pos * person_colls[j]->frame->GetMatrix();
                    gifCollision.DynUpdate(person_colls[j]);
                    break;
                }
            }
        }
        if (num_wagons)
        {
            for (w = 0; w < (int)num_wagons; w++)
            {
                if (wagon_frames[w])
                {
                    wagon_frames[w]->SetPos(wagon_pos[w]);
                    wagon_frames[w]->SetDir(wagon_dirs[w], 0.0f);
                    {
                        I3D_frame *frm = wagon_frames[w];
                        I3D_scene *scene = act_mission->scene;
                        scene->SetFrameSectorPos(frm, frm->GetWorldPos());
                    }
                    wagon_frames[w]->Update();
                    wm = wagon_frames[w]->GetMatrix();
                    inv_matrices[w + 1] = wm;
                    inv_matrices[w + 1].Invert();
                    bvol = wagon_frames[w]->GetHRBoundVolume();
                    ((I3D_bsphere &)colls[w + 1].bsphere).pos = bvol.bsphere.pos * wagon_frames[w]->GetMatrix();
                    gifCollision.DynUpdate(&colls[w + 1]);
                }
            }
        }
        doors.Place();
        // The lights follow their frames
        for (i = 0; i < lights.size(); i++)
        {
            S_vector lpos;
            S_quat lrot;
            lpos = lights[i].pos * lights[i].parent->GetMatrix();
            lrot = lights[i].rot.RotateByMatrix(lights[i].parent->GetMatrix());
            lights[i].frame->SetPos(lpos);
            lights[i].frame->SetRot(lrot);
            lights[i].frame->Update();
        }
    }
    else
    {
        frame->SetPos(pos);
        frame->SetDir(dir, 0.0f);
        {
            I3D_frame *frm = frame;
            I3D_scene *scene = act_mission->scene;
            scene->SetFrameSectorPos(frm, frm->GetWorldPos());
        }
        frame->Update();
        for (w = 0; w < (int)num_wagons; w++)
        {
            if (wagon_frames[w])
            {
                wagon_frames[w]->SetPos(wagon_pos[w]);
                wagon_frames[w]->SetDir(wagon_dirs[w], 0.0f);
                {
                    I3D_frame *frm = wagon_frames[w];
                    I3D_scene *scene = act_mission->scene;
                    scene->SetFrameSectorPos(frm, frm->GetWorldPos());
                }
                wagon_frames[w]->Update();
            }
        }
    }
    if (!ahead)
        FindVehicleAhead();
    StopBehindVehicleAhead();
}

// The first connection of a web node that isn't the closed side of a one-way: the next node along the rail, NULL for
// none (the name is ours; GameInit's walks inline it)
static inline C_web_node *NextRailNode(C_web_node *node)
{
    for (int j = 0; j < node->num_conns; j++)
    {
        C_web_connection *conn = node->GetConn(j);
        if (!(conn->flags & WEBF_ONEWAY))
            return &act_mission->web_path->nodes[conn->node];
    }
    return NULL;
}

// Sets the vehicle up for the mission: the model's wagons, the collisions, the seats, entries and windows, the lights,
// the driver and the passengers, the semaphores along the rails and the rain dummy
// FUNCTION: GAME 0x48a390
void C_railway::GameInit()
{
    int i, k;
    collided = false;
    salina_v_kine = false;
    C_actor::GameInit();
    metro = false;
    const char *file = ((I3D_model *)frame)->GetFileName();
    const char *name = strrchr(file, '\\');
    if (!name)
        name = file;
    else
        name++;
    // STRING: GAME 0x6348b4
    if (strcmp(name, "metro.i3d") == 0)
        metro = true;
    unknown_70.Zero();
    wagon_angles = new float[num_wagons + 1];
    wagon_angles[0] = rnd() * PI;
    unknown_c8 = new S_vector[num_wagons + 1];
    unknown_cc = new S_vector[num_wagons + 1];
    for (i = 0; i <= num_wagons; i++)
    {
        unknown_c8[i].Zero();
        unknown_cc[i].Zero();
    }
    if (num_wagons)
    {
        wagon_frames = new I3D_frame *[num_wagons];
        wagon_pos = new S_vector[num_wagons];
        wagon_dirs = new S_vector[num_wagons];
        I3D_frame *vehicle = frame;
        file = ((I3D_model *)vehicle)->GetFileName();
        name = strrchr(file, '\\');
        name = name ? name + 1 : file;
        int offset = 5;
        for (i = 0; i < num_wagons; i++)
        {
            wagon_angles[i + 1] = rnd() * PI;
            wagon_frames[i] = act_mission->FrmActBuff_GetModel(name);
            if (!wagon_frames[i])
            {
                wagon_frames[i] = (I3D_frame *)::driver->CreateFrame(FRAME_MODEL);
                if (model_cache.Open((I3D_model *)wagon_frames[i], name, 0, NULL, NULL, NULL) != I3D_OK)
                {
                    wagon_frames[i]->Release();
                    wagon_frames[i] = NULL;
                }
            }
            if (wagon_frames[i])
            {
                wagon_pos[i] = S_vector(pos.x, (float)offset + pos.y, pos.z);
                wagon_dirs[i] = dir;
                wagon_frames[i]->SetPos(pos);
                wagon_frames[i]->SetDir(dir, 0.0f);
                wagon_frames[i]->Update();
                char buf[260];
                // STRING: GAME 0x6348a8
                sprintf(buf, "%s_vagon%i", ((I3D_frame *)vehicle)->GetName(), i);
                wagon_frames[i]->SetName(buf);
                I3D_frame *wf = wagon_frames[i];
                I3D_scene *scene = act_mission->scene;
                scene->SetFrameSector(wf);
                C_actor **p = new C_actor *;
                *p = this;
                SetFrameActor(wagon_frames[i], p);
            }
            offset += 5;
        }
    }
    else
        wagon_frames = NULL;
    drive_speed = 0.0f;
    safe_speed = max_speed;
    state_time = 10000;
    back_dist = (float)(num_wagons + 1) * wagon_length + 30.0f;
    dir.Normalize();
    float half = vehicle_length * .5f;
    front_pos = pos + dir * half;
    back_pos = pos - dir * half;
    back_nodes.clear();
    forward_nodes.clear();
    sound_ids[2] = 0xa1;
    sound_ids[0] = metro ? 0x9d : 0x9e;
    sound_ids[1] = metro ? 0x123 : 0xa0;
    // The engine, the brakes' and the bell's sounds
    for (i = 0; i < 3; i++)
    {
        sounds[i] = act_mission->FrmActBuff_GetActorSound(this, i);
        if (!sounds[i])
        {
            sounds[i] = (I3D_sound *)::driver->CreateFrame(FRAME_SOUND);
            if (sounds[i])
            {
                if (!sound_table.ConstructSound(sounds[i], sound_ids[i]))
                {
                    sounds[i]->Release();
                    sounds[i] = NULL;
                }
            }
        }
        if (sounds[i])
        {
            sounds[i]->SetLoop(false);
            sounds[i]->LinkTo(frame, 0);
            sounds[i]->SetOn(true);
            sounds[i]->Update();
        }
    }
    if (sounds[0])
    {
        sounds[0]->SetLoop(true);
        sounds[0]->Update();
    }
    SetRLWState(0);
    unknown_298 = 0;
    // The vehicle's collision and one for each wagon
    colls = new tDynamicCollObject[num_wagons + 1];
    inv_matrices = new S_matrix[num_wagons + 1];
    colls[0].type = DYNCOLL_UNKNOWN_40;
    colls[0].frame = frame;
    colls[0].owner = this;
    coll_frame = frame->FindChildFrame("DUMMY_COL", ENUMF_DUMMY);
    if (!coll_frame)
    {
        I3D_bvolume bvol;
        frame->GetHRBoundVolume(bvol);
        (I3D_bbox &)colls[0].bbox = bvol.bbox;
        S_vector ext = frame->GetWorldScale();
        float mx = Max(ext.x, Max(ext.y, ext.z));
        ((I3D_bsphere &)colls[0].bsphere).radius = mx * bvol.bsphere.radius;
        ((I3D_bsphere &)colls[0].bsphere).pos = bvol.bsphere.pos * frame->GetMatrix();
        colls[0].matrix = (S_matrix *)&frame->GetMatrix();
        inv_matrices[0] = *colls[0].matrix;
        inv_matrices[0].Invert();
    }
    else
    {
        coll_matrix = coll_frame->GetMatrix();
        float f = 1.0f / coll_frame->GetWorldScale().x;
        coll_matrix(0) *= f;
        f = 1.0f / coll_frame->GetWorldScale().y;
        coll_matrix(1) *= f;
        f = 1.0f / coll_frame->GetWorldScale().z;
        coll_matrix(2) *= f;
        S_vector sc;
        inv_matrices[0] = coll_matrix;
        inv_matrices[0].Invert();
        I3D_bbox box = ((I3D_dummy *)coll_frame)->bbox;
        sc = coll_frame->GetWorldScale();
        S_vector ext(0.0f, 0.0f, 0.0f);
        ext.z = (box.max.z - box.min.z) * sc.z;
        ext.y = (box.max.y - box.min.y) * sc.y;
        ext.x = (box.max.x - box.min.x) * sc.x;
        float radius = ext.Magnitude() * .5f;
        sc = coll_frame->GetWorldScale();
        box.min.x *= sc.x;
        box.min.y *= sc.y;
        box.min.z *= sc.z;
        sc = coll_frame->GetWorldScale();
        box.max.x *= sc.x;
        box.max.y *= sc.y;
        box.max.z *= sc.z;
        ((I3D_bsphere &)colls[0].bsphere).radius = radius;
        (I3D_bbox &)colls[0].bbox = box;
        ((I3D_bsphere &)colls[0].bsphere).pos = coll_frame->GetMatrix()(3);
        colls[0].matrix = &coll_matrix;
    }
    colls[0].inv_matrix = &inv_matrices[0];
    colls[0].flags = DYNCOLLF_UNKNOWN_2000;
    if (!metro)
        mafiaIndicators.RadarAddObject(&colls[0], 0xffffff00);
    gifCollision.DynInsert(&colls[0]);
    for (i = 1; i <= num_wagons; i++)
    {
        colls[i].type = DYNCOLL_UNKNOWN_40;
        colls[i].frame = wagon_frames[i - 1];
        colls[i].owner = this;
        I3D_bvolume bvol;
        wagon_frames[i - 1]->GetHRBoundVolume(bvol);
        (I3D_bbox &)colls[i].bbox = bvol.bbox;
        S_vector ext = wagon_frames[i - 1]->GetWorldScale();
        float mx = Max(ext.x, Max(ext.y, ext.z));
        ((I3D_bsphere &)colls[i].bsphere).radius = mx * bvol.bsphere.radius;
        ((I3D_bsphere &)colls[i].bsphere).pos = bvol.bsphere.pos * frame->GetMatrix();
        colls[i].flags = DYNCOLLF_UNKNOWN_2000;
        colls[i].matrix = (S_matrix *)&wagon_frames[i - 1]->GetMatrix();
        inv_matrices[i] = *colls[i].matrix;
        inv_matrices[i].Invert();
        colls[i].inv_matrix = &inv_matrices[i];
        if (!metro)
            mafiaIndicators.RadarAddObject(&colls[i], 0xffffff00);
        gifCollision.DynInsert(&colls[i]);
    }
    dyn_colls = new std::vector<tDynamicCollObject *>[num_wagons + 1];
    int node = act_mission->web_path->GetNearNode(pos, -1.0f, 0xc);
    if (node != -1)
    {
        back_nodes.push_back(node);
        C_web_node &n = act_mission->web_path->nodes[back_nodes[0]];
        S_vector d = n.GetPos() - front_pos;
        front_pos += d;
        back_pos += d;
    }
    CreateBackVector(front_pos);
    Place(1);
    I3D_bvolume bvol;
    frame->GetHRBoundVolume(bvol);
    center_z = (bvol.bbox.max.z - bvol.bbox.min.z) * .5 + bvol.bbox.min.z;
    float avg = ((bvol.bbox.max.x - bvol.bbox.min.x) + (bvol.bbox.max.y - bvol.bbox.min.y) +
        (bvol.bbox.max.z - bvol.bbox.min.z)) * (1.0f / 3.0f);
    inv_inertia = 1.0f / (avg * avg * vehicle_mass * .25f);
    Enumerate();
    seats.Init(this);
    GenEntries();
    glass.GameInit();
    lock_count = 0;
    visibility = 0;
    C_actor::DebustizeFrame(frame);
    for (i = 0; i < num_wagons; i++)
        C_actor::DebustizeFrame(wagon_frames[i]);
    lights.clear();
    S_enum_lights ctx;
    ctx.tram = this;
    if (act_mission->game->night_mission)
    {
        if (act_mission->options.game_effects > 0)
        {
            S_vector lp;
            lp.x = (bvol.bbox.min.x + bvol.bbox.max.x) * .5f;
            lp.y = (bvol.bbox.min.y + bvol.bbox.max.y) * .5f;
            lp.z = (bvol.bbox.min.z + bvol.bbox.max.z) * .5f;
            if (!metro)
                lp.y -= .5f;
            I3D_light *light = (I3D_light *)::driver->CreateFrame(FRAME_LIGHT);
            light->SetPos(lp);
            light->SetLightType(I3DLIGHT_POINT);
            light->SetDiffuse(.839f, .757f, .588f);
            light->SetDiffusePower(3.0f);
            light->SetRange(1.0f, 10.0f);
            light->SetMode(1);
            light->SetOn(true);
            light->LinkTo(frame, 0);
            light->Update();
            act_mission->scene->primary_sector->AddLight(light);
            ctx.frame = frame;
            frame->EnumFrames(cbEnumLights, (uint)&ctx, ENUMF_ALL, NULL);
            for (i = 0; i < num_wagons; i++)
            {
                light = (I3D_light *)::driver->CreateFrame(FRAME_LIGHT);
                light->SetPos(lp);
                light->SetLightType(I3DLIGHT_POINT);
                light->SetDiffuse(.839f, .757f, .588f);
                light->SetDiffusePower(3.0f);
                light->SetRange(1.0f, 10.0f);
                light->SetMode(1);
                light->SetOn(true);
                light->LinkTo(wagon_frames[i], 0);
                light->Update();
                act_mission->scene->primary_sector->AddLight(light);
                ctx.frame = wagon_frames[i];
                wagon_frames[i]->EnumFrames(cbEnumLights, (uint)&ctx, ENUMF_ALL, NULL);
            }
        }
    }
    else
    {
        ctx.frame = frame;
        frame->EnumFrames(cbEnumLights, (uint)&ctx, ENUMF_ALL, NULL);
        for (i = 0; i < num_wagons; i++)
        {
            ctx.frame = wagon_frames[i];
            wagon_frames[i]->EnumFrames(cbEnumLights, (uint)&ctx, ENUMF_ALL, NULL);
        }
    }
    // The driver, at the frame "ridic"
    I3D_frame *seat = frame->FindChildFrame("ridic", ENUMF_DUMMY);
    if (seat)
    {
        I3D_model *model = act_mission->FrmActBuff_GetModel("pumpar01.I3D");
        if (!model)
        {
            model = (I3D_model *)::driver->CreateFrame(FRAME_MODEL);
            if (model_cache.Open(model, "pumpar01.I3D", 0, NULL, NULL, NULL) != I3D_OK)
            {
                model->Release();
                model = NULL;
            }
        }
        if (model)
        {
            driver.frame_model = model;
            driver.should_keep_sitting = -1;
            driver.wagon_index = 0;
            driver.state = RH_DRIVER;
            model->LinkTo(frame, 0);
            driver.frame_model->SetPos(seat->GetPos());
            driver.frame_model->SetRot(seat->GetRot());
            driver.frame_model->Update();
            I3D_frame *target_e = driver.frame_model->FindChildFrame("targetE", ENUMF_TARGET);
            I3D_frame *target_n = driver.frame_model->FindChildFrame("targetN", ENUMF_TARGET);
            if (target_e)
                target_e->SetOn(false);
            if (target_n)
                target_n->SetOn(false);
            driver.anim_container = new C_animations_machine_de_luxe;
            driver.animation_number = -1;
            driver.new_animation_number = RANIM_SALINDRIVER;
            driver.anim_container->Init((I3D_model *)driver.frame_model);
            driver.SetAnim(1);
            persons.push_back(&driver);
        }
    }
    // The generator of pedestrians in range: the passengers, from its models
    C_actor *gen = NULL;
    for (uint a = 0; a < act_mission->actors.size(); a++)
    {
        C_actor *act = act_mission->actors[a];
        if (act->type == ACTOR_TRAFFIC_GENERATOR)
        {
            S_vector d = act->pos - pos;
            if (d.Magnitude() < 1000000.0f)
                gen = act;
        }
    }
    if (gen)
    {
        S_vector hit(0.0f, 0.0f, 0.0f);
        S_vector from(0.0f, .8f, 0.0f);
        S_vector to(0.0f, -1.6f, 0.0f);
        gifCollision.TestLineInVehicle(dyn_colls, from, to, &hit, NULL, 2);
        C_traffic_generator *tg = (C_traffic_generator *)gen;
        int count = (rnd(num_wagons) + 1) * rnd(5) + 3;
        for (; count; count--)
        {
            const char *model_name = tg->models[rnd(tg->num_models)];
            I3D_model *mdl = act_mission->FrmActBuff_GetModel(model_name);
            if (!mdl)
            {
                mdl = (I3D_model *)::driver->CreateFrame(FRAME_MODEL);
                if (model_cache.Open(mdl, model_name, 0, NULL, NULL, NULL) != I3D_OK)
                {
                    mdl->Release();
                    continue;
                }
            }
            I3D_frame *back = mdl->FindChildFrame("back1", ENUMF_ALL);
            if (back)
            {
                char prop[32];
                strcpy(prop, back->GetProperty());
                if (strlen(prop) > 3)
                {
                    strupr(prop);
                    if (prop[2] == 'B' || prop[2] == 'D' || prop[2] == 'E')
                    {
                        mdl->Release();
                        continue;
                    }
                }
            }
            S_rail_human *person = new S_rail_human;
            person->frame_model = mdl;
            person->should_keep_sitting = rnd(200) + 5;
            person->wagon_index = rnd(num_wagons + 1);
            person->seat_index = seats.FindFreeSeat(person->wagon_index, NULL, 0.0f);
            if (person->seat_index == 0xffff)
            {
                delete person;
                mdl->Release();
                continue;
            }
            seats.ReserveSeat(person->seat_index, person);
            person->should_keep_sitting = rnd(5) + 3;
            person->state = RH_SITTING;
            person->frame_model->LinkTo(person->wagon_index ? wagon_frames[person->wagon_index - 1] : frame, 0);
            S_vector sp = seats.GetSeatPos(person->seat_index);
            S_vector sd = seats.GetSeatDir(person->seat_index);
            sp.x = sp.x < 0.0f ? -.5f : .5f;
            sp.y = hit.y;
            person->frame_model->SetPos(sp);
            person->frame_model->SetDir(sd, 0.0f);
            person->frame_model->Update();
            I3D_frame *target_e = person->frame_model->FindChildFrame("targetE", ENUMF_TARGET);
            I3D_frame *target_n = person->frame_model->FindChildFrame("targetN", ENUMF_TARGET);
            if (target_e)
                target_e->SetOn(false);
            if (target_n)
                target_n->SetOn(false);
            person->anim_container = new C_animations_machine_de_luxe;
            person->animation_number = -1;
            if (person->IsWoman())
                person->new_animation_number = RANIM_ZEN_SALSEZ + rnd(2);
            else
                person->new_animation_number = RANIM_SED;
            person->anim_container->Init((I3D_model *)person->frame_model);
            person->SetAnim(500);
            persons.push_back(person);
        }
    }
    // The semaphores: the first vehicle marks the rail's nodes and finds the road crossings with a light near it
    C_web_node *start = &act_mission->web_path->nodes[back_nodes[0]];
    if (!(start->flags & WEBF_UNKNOWN_1000))
    {
        for (i = 0; i < (int)act_mission->roads->crossings.size(); i++)
        {
            S_road_crossing *crossing = &act_mission->roads->crossings[(ushort)i];
            if (!crossing->semaphore)
                continue;
            for (k = 0; k < 4; k++)
            {
                ushort cn = crossing->conns[k];
                if (cn == 0xffff)
                    continue;
                if (!back_nodes.size())
                    continue;
                S_road_node *rn = &act_mission->roads->nodes[cn & NEARNODE_INDEX];
                start->flags |= WEBF_UNKNOWN_1000;
                C_web_node *best = start;
                S_vector dv = rn->pos - start->GetPos();
                float best_len = dv.Magnitude();
                for (C_web_node *cur = NextRailNode(start); cur && cur != start; cur = NextRailNode(cur))
                {
                    cur->flags |= WEBF_UNKNOWN_1000;
                    S_vector dc = rn->pos - cur->GetPos();
                    float len = dc.Magnitude();
                    if (len < best_len)
                    {
                        best_len = len;
                        best = cur;
                    }
                }
                C_web_node *next = NextRailNode(best);
                S_vector along = next->GetPos() - best->GetPos();
                S_vector across = crossing->pos - rn->pos;
                float angle = across.AngleTo(along);
                if (best_len < 6.0f && angle > -.5f && angle < .5f)
                {
                    best->flags |= WEBF_UNKNOWN_800;
                    S_rail_semaphore *sema = new S_rail_semaphore;
                    sema->crossing = crossing;
                    sema->node = best;
                    semaphores.push_back(sema);
                }
            }
        }
    }
    frame->GetHRBoundVolume(bound);
    rlws.push_back(this);
    rain_dummy = (I3D_dummy *)::driver->CreateFrame(FRAME_DUMMY);
}

// Undoes GameInit: the sounds and the frames given back, the wagons' and the lights' frames released, the arrays,
// collisions, passengers, seats, entries and doors destroyed, and the statics emptied
// FUNCTION: GAME 0x48cfa0
void C_railway::GameDone()
{
    int i;
    uint u;
    back_nodes.clear();
    forward_nodes.clear();
    for (i = 0; i < 3; i++)
    {
        if (sounds[i])
        {
            sounds[i]->LinkTo(NULL, 0);
            sounds[i]->SetOn(false);
            act_mission->FrmActBuff_StoreActorSound(this, i, sounds[i]);
            sounds[i] = NULL;
        }
    }
    if (unknown_298)
    {
        unknown_298->LinkTo(NULL, 0);
        unknown_298->SetOn(false);
        unknown_298->Release();
        unknown_298 = NULL;
    }
    glass.GameDone();
    for (u = 0; u < lights.size(); u++)
        lights[u].frame->Release();
    lights.clear();
    frame->EnumFrames(cbEnumDestroy, 0, ENUMF_LIGHT, NULL);
    for (i = 0; i < num_wagons; i++)
        wagon_frames[i]->EnumFrames(cbEnumDestroy, 0, ENUMF_LIGHT, NULL);
    delete unknown_c8;
    delete unknown_cc;
    delete wagon_angles;
    if (wagon_frames)
    {
        for (i = 0; i < num_wagons; i++)
        {
            if (wagon_frames[i])
            {
                C_actor **p = GetFrameActor(wagon_frames[i]);
                if (p)
                    *p = NULL;
                SetFrameActor(wagon_frames[i], p);
                wagon_frames[i]->LinkTo(NULL, 0);
                act_mission->FrmActBuff_StoreModel((I3D_model *)wagon_frames[i]);
            }
        }
        delete wagon_pos;
        delete wagon_dirs;
        delete wagon_frames;
        wagon_frames = NULL;
    }
    C_actor::GameDone();
    for (i = 0; i <= num_wagons; i++)
    {
        gifCollision.DynErase(&colls[i]);
        if (!metro)
            mafiaIndicators.RadarRemoveObject(&colls[i]);
    }
    delete colls;
    delete inv_matrices;
    for (u = 0; u < dummy_colls.size(); u++)
    {
        delete dummy_colls[u]->matrix;
        delete dummy_colls[u]->inv_matrix;
        delete dummy_colls[u];
    }
    dummy_colls.clear();
    delete[] dyn_colls;
    for (u = 0; u < persons.size(); u++)
    {
        if (persons[u]->frame_model)
        {
            persons[u]->frame_model->LinkTo(NULL, 0);
            act_mission->FrmActBuff_StoreModel((I3D_model *)persons[u]->frame_model);
        }
        persons[u]->Done();
        if (persons[u] != &driver)
            delete persons[u];
    }
    persons.clear();
    for (u = 0; u < person_colls.size(); u++)
        gifCollision.DynErase(person_colls[u]);
    person_colls.clear();
    for (u = 0; u < shedule_table.size(); u++)
        delete shedule_table[u];
    shedule_table.clear();
    rlws.clear();
    ahead = 0;
    seats.Destroy();
    entries.Destroy();
    doors.Destroy();
    DestroyEntries();
    for (u = 0; u < semaphores.size(); u++)
        delete semaphores[u];
    semaphores.clear();
    if (rain_dummy)
    {
        rain_dummy->Release();
        act_mission->GetScene()->DeleteFrame(rain_dummy);
        rain_dummy = NULL;
    }
}

// A person the vehicle hits is taken out of its passengers and given to the traffic as a free pedestrian, which the
// don's tail then lets react; a car that hits it breaks the windows near the place (the windows of the wagon it is
// in), and the vehicle has the frame's collision
// FUNCTION: GAME 0x48d530
bool C_railway::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    I3D_frame *frm)
{
    int i;
    uint u;
    uint k;
    int index = -1;
    for (i = 0; i <= num_wagons; i++)
    {
        if (colls[i].frame == frm)
        {
            index = i;
            break;
        }
    }
    if (index == -1)
    {
        for (u = 0; u < persons.size(); u++)
        {
            if (persons[u]->frame_model == frm)
            {
                for (k = 0; k < person_colls.size(); k++)
                {
                    if (person_colls[k]->frame == frame)
                    {
                        gifCollision.DynErase(person_colls[k]);
                        person_colls.erase(person_colls.begin() + k);
                    }
                }
                S_rail_human *person = persons[u];
                seats.EmptySeat(person->seat_index, person);
                I3D_model *model = (I3D_model *)person->frame_model;
                persons.erase(persons.begin() + u);
                person->Done();
                delete person;
                C_traffic_generator::AddFreePerson(model, this, false);
                act_mission->GetGame()->don_ocas->Collision(act, v1, v2, v3, f, model);
                Unlock();
            }
        }
    }
    else
    {
        collided = true;
        if (act->type == ACTOR_CAR)
        {
            const S_matrix &wm = colls[index].frame->GetMatrix();
            S_matrix inv = wm;
            inv.Inverse(S_matrix(wm));
            S_vector local = v1 * inv;
            if (v2.Square() > 16.0f)
            {
                for (u = 0; u < glass.num_windows; u++)
                {
                    float dist = (glass.windows[u].frame->GetPos() - local).Magnitude();
                    if (glass.windows[u].wagon_num == index && glass.windows[u].state && dist < 3.0f)
                        glass.windows[u].Hit(v2, glass.glass_material);
                }
            }
        }
    }
    return false;
}

// The part of the move of the actor at pos by dir after which a sphere of radius + add around center, moving by
// move, is first met, 0 where it is met already, negative where it isn't (the name is ours)
static inline float RailSphere(float add, const S_vector &center, const S_vector &pos, const S_vector &move,
    const S_vector &dir, float radius)
{
    S_vector d = center - pos;
    S_vector b = move - dir;
    float r = radius + add;
    float rr = r * r;
    float t;
    if (d.Dot(d) <= rr)
        t = 0.0f;
    else
    {
        float a = b.Dot(b);
        if (a <= 0.0f)
            t = -1.0f;
        else
        {
            float bb = b.Dot(d) * 2.0f;
            float disc = bb * bb - (d.Dot(d) - rr) * a * 4.0f;
            if (disc < 0.0f)
                t = disc;
            else
            {
                float s = (float)sqrt(disc);
                float inv = 1.0f / (a * 2.0f);
                float t1 = (s - bb) * inv;
                t = (-bb - s) * inv;
                if (t1 < t)
                    t = t1;
            }
        }
    }
    return t;
}

// Whether the actor at v1 moving by v2 (a sphere of radius f) runs into the vehicle: first a sphere around the
// vehicle moving by its direction, then, where the vehicle isn't on its rails (rlw_state) or is in a game of its own
// (metro), the spheres along its length, then the spheres of each wagon; the part of the move where they first
// meet into dist, the vehicle into coll (the parameters' names are ours)
// FUNCTION: GAME 0x48d890
bool C_railway::IsPossibleColl(C_actor *act, S_vector v1, S_vector v2, float f, float &dist, C_actor **coll)
{
    uint u;
    uint k;
    S_vector move = this->dir * drive_speed * 6.0f;
    float t = RailSphere(wagon_length * 0.5f, pos, v1, move, v2, f);
    if (t < 1.0f && t >= 0.0f)
    {
        if (rlw_state >= 1 && rlw_state <= 5 && !metro)
        {
            dist = t;
            *coll = this;
            return true;
        }
        uint n = (int)(wagon_length * .3333f) - 1;
        float best = 1.1f;
        S_vector c = this->dir * (wagon_length * 0.5f - 1.5f) + pos;
        S_vector c2 = c;
        S_vector center = c2;
        for (u = 0; u < n; u++)
        {
            S_vector p = center - this->dir * (((float)(u * 2) + 1.0f) * 1.5f);
            float t2 = RailSphere(1.5f, p, v1, move, v2, f);
            if (t2 < 1.0f && t2 >= 0.0f)
            {
                if (t2 < best)
                    best = t2;
                *coll = this;
            }
        }
        if (best != 1.1f)
        {
            dist = best;
            return true;
        }
    }
    for (u = 0; u < num_wagons; u++)
    {
        S_vector m = wagon_dirs[u] * drive_speed;
        float t4 = gifCollision.TestSphereSphere(v1, v2, f, wagon_pos[u], m, wagon_length * 0.5f);
        if (t4 < 1.0f && t4 >= 0.0f)
        {
            int n2 = (int)(wagon_length * .3333f);
            S_vector w = wagon_dirs[u] * (wagon_length * 0.5f - 1.5f) + wagon_pos[u];
            S_vector wp = w;
            float best2 = 1.1f;
            for (k = 0; k < n2 - 1; k++)
            {
                S_vector p = wp - wagon_dirs[u] * (((float)(k * 2) + 1.0f) * 1.5f);
                float t5 = gifCollision.TestSphereSphere(v1, v2, f, p, m, 1.5f);
                if (t5 < 1.0f && t5 >= 0.0f)
                {
                    if (t5 < best2)
                        best2 = t5;
                    *coll = this;
                }
            }
            if (best2 != 1.1f)
            {
                dist = best2;
                return true;
            }
        }
    }
    return false;
}

// Fills the curve of spheres the vehicle sweeps in the next stretch of its way: its front at time 0, then each
// node of the forward vector it reaches within the distance it covers (a speed's six seconds), at the share of that
// distance it takes to get there, and the point the distance ends at, at time 1
// FUNCTION: GAME 0x48dfd0
void C_railway::FillCurve(std::vector<g_collision::gcol_CurveSphereStruct> *curve)
{
    curve->clear();
    float reach = drive_speed * 6.0f + 0.2f;
    CreateForwardVector(back_nodes[0], reach);
    g_collision::gcol_CurveSphereStruct pt;
    pt.pos = GetDir() * 0.2f + front_pos;
    pt.t = 0.0f;
    curve->push_back(pt);
    S_vector cur = act_mission->web_path->nodes[back_nodes[0]].GetPos();
    S_vector diff = cur - pt.pos;
    float len = diff.Magnitude();
    float rem;
    if (len > reach)
    {
        diff.Normalize();
        pt.pos += diff * (float)fabs(reach + 0.01f);
        pt.t = 1.0f;
        curve->push_back(pt);
        return;
    }
    rem = reach - len;
    pt.pos = cur;
    pt.t = 1.0f / (reach / (reach - rem));
    curve->push_back(pt);
    S_vector next = act_mission->web_path->nodes[forward_nodes[0]].GetPos();
    diff = next - cur;
    len = diff.Magnitude();
    uint i = 0;
    while (len < rem)
    {
        rem -= len;
        pt.pos = cur;
        pt.t = 1.0f / (reach / (reach - rem));
        curve->push_back(pt);
        i++;
        cur = next;
        next = act_mission->web_path->nodes[forward_nodes[i]].GetPos();
        diff = next - cur;
        len = diff.Magnitude();
    }
    diff.Normalize();
    pt.pos = cur + diff * (float)fabs(rem + 0.01f);
    pt.t = 1.0f;
    curve->push_back(pt);
}

// When the sphere (f's radius) moving as pos + vel * t, t in the curve's time, first meets the curve's spheres (radius
// 1): each segment by TestSphereSphere in its own time, then scaled back; FLT_MAX for a curve of fewer than two points
inline float TestMovingCurve(const S_vector &pos, const S_vector &vel, float f,
    std::vector<g_collision::gcol_CurveSphereStruct> &curve)
{
    float m = 1000.0f;
    if (curve.size() < 2)
        return FLT_MAX;
    const g_collision::gcol_CurveSphereStruct *a1 = curve.begin(), *a = a1++;
    while (a1 < curve.end())
    {
        float dt = a1->t - a->t;
        float t = gifCollision.TestSphereSphere(pos + vel * a->t, vel * dt, f, a->pos, a1->pos - a->pos, 1.0f);
        t = t * dt + a->t;
        if (t >= 0.0f)
        {
            if (t < m)
                m = t;
            else
                break;
        }
        a = a1++;
    }
    return m;
}

// Whether something is in the way of the vehicle: the earliest time (0 to 1) of its curve that a car or a pedestrian
// of the game meets it, which sets state_time to 0 (the vehicle brakes); collided once makes it true without a test
// FUNCTION: GAME 0x48e780
bool C_railway::PreduceMovement()
{
    if (metro)
        return false;
    if (collided)
    {
        collided = false;
        return true;
    }
    float reach = brake_dist + 4.3f;
    if (reach < 4.0f)
        reach = 4.0f;
    std::vector<g_collision::gcol_CurveSphereStruct> curve;
    FillCurve(&curve);
    float margin = drive_speed * 6.0f + 0.2f;
    float best = FLT_MAX;
    C_game *game = act_mission->game;
    int i;
    for (i = game->active_actors.size(); i--;)
    {
        C_actor *act = game->active_actors[i];
        if (act->type != ACTOR_CAR)
            continue;
        C_car *car = (C_car *)act;
        S_vector v6 = car->velocity * 6.0f;
        S_vector dir = car->velocity;
        float f = 0.9f;
        if (car->velocity.Magnitude() < 0.1f)
        {
            dir = car->GetDir();
            v6 = dir * 0.1f;
            f = 1.0f;
        }
        S_vector P = car->GetAIPos(dir.Dot(GetDir()) < 0.0f);
        if (car->velocity.Magnitude() < 0.1f)
            P = car->GetPos();
        float cr = car->speed * 6.0f + 0.1f;
        float dist = (GetDir() * 0.2f + front_pos - P).Magnitude();
        if ((dist < cr + margin || dist < 4.0f) && dist < reach)
        {
            float m = TestMovingCurve(P, v6, f, curve);
            if (m != FLT_MAX && m >= 0.0f && m < 1.0f && m < best)
                best = m;
        }
    }
    std::vector<C_traffic_car::S_ELEMENT *> elems = C_traffic_car::GetElems();
    for (i = elems.size(); i--;)
    {
        C_car *car = elems[i]->unknown_10;
        if (!car)
            car = elems[i]->car;
        if (!car)
            continue;
        S_vector v6 = car->velocity * 6.0f;
        S_vector dir = car->velocity;
        float f = 0.9f;
        if (car->velocity.Magnitude() < 0.1f)
        {
            dir = car->GetDir();
            v6 = dir * 0.1f;
            f = 1.0f;
        }
        S_vector P = car->GetAIPos(dir.Dot(GetDir()) < 0.0f);
        if (car->velocity.Magnitude() < 0.1f)
            P = car->GetPos();
        float cr = car->speed * 6.0f + 0.1f;
        float dist = (GetDir() * 0.2f + front_pos - P).Magnitude();
        if ((dist < cr + margin || dist < 4.0f) && dist < reach)
        {
            float m = TestMovingCurve(P, v6, f, curve);
            if (m != FLT_MAX && m >= 0.0f && m < 1.0f && m < best)
                best = m;
        }
    }
    float margin2 = margin + 1.0f;
    for (i = unknown_63d7d8.size(); i--;)
    {
        S_unknown_63d7d8 *e = unknown_63d7d8[i];
        float dist = (GetDir() * 0.2f + front_pos - e->pos).Magnitude();
        if ((dist < margin2 || dist < 4.0f) && dist < reach)
        {
            float m = TestMovingCurve(e->pos, e->vel, 0.4f, curve);
            if (m != FLT_MAX && m >= 0.0f && m < 1.0f && m < best)
                best = m;
        }
    }
    for (i = game->inactive_actors.size(); i--;)
    {
        C_actor *act = game->inactive_actors[i];
        if (act->type != ACTOR_CAR)
            continue;
        C_car *car = (C_car *)act;
        float cr = car->speed * 6.0f + 0.1f;
        float dist = (GetDir() * 0.2f + front_pos - car->GetPos()).Magnitude();
        if ((dist < cr + margin || dist < 4.0f) && dist < reach)
        {
            float m = gifCollision.TestSphereCurve(car->GetPos(), car->GetDir() * 0.1f, 1.5f, &curve, 1.0f);
            if (m != FLT_MAX && m >= 0.0f && m < 1.0f && m < best)
                best = m;
        }
    }
    if (best != FLT_MAX)
    {
        state_time = 0;
        return true;
    }
    return false;
}

// Moves the vehicle along its rails by time: when it has reached its next node, takes the next one off the path;
// then puts the vehicle and its wagons on the rails
// FUNCTION: GAME 0x48f860
void C_railway::Place(int time)
{
    unknown_e4 = 0;
    brake_dist = drive_speed * drive_speed / (2.0f * acceleration);
    if (back_nodes.empty())
        return;
    S_vector d = act_mission->web_path->nodes[back_nodes[0]].GetPos() - front_pos;
    if (unknown_70.IsNull())
        unknown_70 = d;
    if (d.AngleTo(unknown_70) >= PI / 2 || d.Magnitude() < 0.1f)
    {
        int f = unknown_e4 | 1;
        unknown_e4 = f;
        if (IsStopPoint(back_nodes[0]))
        {
            f |= 2;
            unknown_e4 = f;
        }
        if (IsSemaPoint(back_nodes[0]))
            unknown_e4 |= 4;
        C_web_node *node = &act_mission->web_path->nodes[back_nodes[0]];
        if (!forward_nodes.empty())
        {
            back_nodes.insert(back_nodes.begin(), forward_nodes.front());
            forward_nodes.erase(forward_nodes.begin());
        }
        else
        {
            for (uint i = node->num_conns; i--; )
            {
                C_web_connection *c = node->GetConn(i);
                if (!(c->flags & WEBF_ONEWAY))
                {
                    back_nodes.insert(back_nodes.begin(), node->GetConn(i)->node);
                    break;
                }
            }
        }
        while (back_nodes.size() > 2)
        {
            S_vector back = front_pos - act_mission->web_path->nodes[back_nodes.back()].GetPos();
            if (!(back.Magnitude() > back_dist))
                break;
            back_nodes.pop_back();
        }
        d = act_mission->web_path->nodes[back_nodes[0]].GetPos() - front_pos;
        unknown_70 = d;
    }
    uint first = back_nodes[0];
    CreateForwardVector(first, brake_dist + 5.0f);
    back_nodes.clear();
    back_nodes.push_back(first);
    CreateBackVector(front_pos);
    d.Normalize();
    float step = time * drive_speed * 0.001f;
    d *= step;
    front_pos += d;
    int index = 1;
    back_pos = FindBackPos(front_pos, vehicle_length, &index);
    *unknown_c8 = pos;
    *unknown_cc = dir;
    SpringProcess(time, 0, &pos, &dir, front_pos, back_pos);
    *unknown_c8 = pos - *unknown_c8;
    *unknown_cc = dir - *unknown_cc;
    S_vector back = back_pos;
    if (num_wagons)
    {
        for (int i = 0; i < (int)num_wagons; i++)
        {
            S_vector a = FindBackPos(back, (wagon_length - vehicle_length) * 0.5f, &index);
            S_vector b = FindBackPos(a, vehicle_length, &index);
            unknown_c8[i + 1] = wagon_pos[i];
            unknown_cc[i + 1] = wagon_dirs[i];
            SpringProcess(time, i + 1, &wagon_pos[i], &wagon_dirs[i], a, b);
            wagon_pos[i] = wagon_pos[i] - unknown_c8[i + 1];
            wagon_dirs[i] = wagon_dirs[i] - unknown_cc[i + 1];
            back = b;
        }
    }
}

// Puts the frame where the vehicle is and moves a sphere along the way it has come: what it meets is hit by the vehicle
// (a person is pushed aside, a car or a dynamic object takes the vehicle's impulse)
// FUNCTION: GAME 0x490140
void C_railway::Collide()
{
    S_vector old_pos = frame->GetPos();
    frame->GetRot();
    frame->SetPos(pos);
    frame->SetDir(dir, 0.0f);
    act_mission->GetScene()->SetFrameSector(frame);
    frame->Update();
    frame->UpdateWMatrix();
    S_vector move = pos - old_pos;
    float len = move.Magnitude();
    if (!(len > 0.0001f))
        return;
    S_vector ndir = move / len;
    gifCollision.DynSetNextCollOwner(colls, NULL);
    S_vector from = front_pos;
    from.y += 0.7f;
    S_vector hitpos;
    S_vector normal;
    g_collision_header *hit = (g_collision_header *)gifCollision.TestSphereB(from, move, 1.5f, &hitpos, &normal,
        COLF_UNKNOWN_2, COLF_ALL);
    gifCollision.DynSetNextCollOwner(NULL, NULL);
    hitpos += normal * -1.5f;
    if (!hit)
        return;
    if (!(hit->flags & COLLHF_UNKNOWN_40))
        return;
    C_actor *owner = hit->owner;
    switch (owner->type)
    {
    case ACTOR_CAR:
    {
        normal.y = 0.0f;
        normal.Normalize();
        move_velocity = ndir * drive_speed;
        ((C_car *)owner)->DoCollisionStatic(hitpos, normal, move_velocity);
        collided = true;
        break;
    }
    case ACTOR_DYN_OBJECT:
    {
        normal.y = 0.0f;
        normal.Normalize();
        S_vector o1;
        S_vector o2;
        float mass;
        S_matrix m;
        ((C_dyn_object *)owner)->unknown_414150(hitpos, o1, o2, mass, m);
        move_velocity = ndir * drive_speed;
        S_vector r = hitpos - pos;
        float rel = (move_velocity - o2).Dot(normal);
        S_vector c = r.Cross(normal) * inv_inertia;
        S_vector a = c.Cross(r);
        S_vector t = o1.Cross(normal);
        S_vector u = (t * m).Cross(o1);
        float inv_m = 1.0f / vehicle_mass;
        float den = (u + a).Dot(normal) + 1.0f / mass + inv_m;
        if (fabs(den) < 1e-5f)
            den = 1e-5f;
        float j = rel / den;
        S_vector v = normal * j;
        S_vector w = v * inv_m;
        S_vector y = w * mass;
        v += y;
        owner->Collision(this, hitpos, v, normal, vehicle_mass, NULL);
        break;
    }
    case ACTOR_PLAYER:
    case ACTOR_HUMAN:
    case ACTOR_ENTITY:
    {
        move_velocity = ndir * drive_speed;
        S_vector v = move_velocity;
        v.x *= 23.2f;
        v.y = 0.0f;
        v.z *= 23.2f;
        owner->Collision(this, hitpos, v, normal, 1.0f, NULL);
        collided = true;
        break;
    }
    case 0x13:
    {
        move_velocity = ndir * drive_speed;
        S_vector v = move_velocity;
        v.y = 0.0f;
        float k = 23.2f;
        v *= k;
        owner->Collision(this, hitpos, v, normal, 1.0f, hit->frame);
        collided = true;
        break;
    }
    }
}

// Explosions break the windows and kill the passengers, shots go along the line to the first thing it meets: a
// window cracks and breaks, a pedestrian's collision, a passenger or the driver dies, or another actor is hit
// FUNCTION: GAME 0x490b40
bool C_railway::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
    uint u, I3D_frame *frm)
{
    bool hit = false;
    uint i, j, k;
    int w;
    switch (type)
    {
    case HIT_EXPLOSION:
    {
        S_vector pos = dir;
        pos.Normalize();
        pos = pos * drive_speed + v1;
        for (i = 0; i < persons.size(); i++)
        {
            if (f > 280.0f)
            {
                for (j = 0; j < person_colls.size(); j++)
                {
                    if (person_colls[j]->frame == persons[i]->frame_model)
                    {
                        gifCollision.DynErase(person_colls[j]);
                        person_colls.erase(person_colls.begin() + j);
                        break;
                    }
                }
                persons[i]->Kill(this);
            }
        }
        for (i = 0; i < glass.num_windows; i++)
        {
            if (f > 150.0f)
                glass.windows[i].Hit(pos, glass.glass_material);
            if (f > 250.0f)
                glass.windows[i].Hit(pos, glass.glass_material);
        }
        for (i = 0; i < glass.num_windows; i++)
        {
            if (glass.windows[i].state == RWINDOW_CRACKED)
                glass.windows[i].frame->SetOn(true);
        }
        break;
    }
    case HIT_SHOT:
    {
        C_human *player = act_mission->GetGame()->player;
        if (player && (C_railway *)player->car == this && act != player)
            player->frame->SetOn(true);
        int wagon = -1;
        for (w = 0; w <= (int)num_wagons; w++)
        {
            if (colls[w].frame == frm)
            {
                wagon = w;
                break;
            }
        }
        if (wagon == -1)
        {
            for (j = 0; j < person_colls.size(); j++)
            {
                if (person_colls[j]->frame == frm)
                {
                    for (k = 0; k < persons.size(); k++)
                    {
                        if (persons[k]->frame_model == frm)
                        {
                            S_rail_human *person = persons[k];
                            seats.EmptySeat(person->seat_index, person);
                            I3D_frame *model = person->frame_model;
                            persons.erase(persons.begin() + k);
                            person->Done();
                            delete person;
                            C_traffic_generator::AddFreePerson((I3D_model *)model, this, false);
                            act_mission->GetGame()->don_ocas->Hit(HIT_SHOT, v1, v2, v3, f, act, 5, model);
                            C_traffic_generator::NewPanic(model);
                            Unlock();
                            material = MAT_PASSENGER;
                            hit = true;
                            break;
                        }
                    }
                    gifCollision.DynErase(person_colls[j]);
                    person_colls.erase(person_colls.begin() + j);
                    break;
                }
            }
        }
        S_vector pos100 = v1 * 100.0f;
        for (;;)
        {
            I3D_COLLISION col;
            I3D_frame *cur = act_mission->scene->TestColHierarchy(v2, pos100, frm, col, 0, NULL, 0, NULL);
            if (!cur)
                break;
            for (i = 0; i < glass.num_windows; i++)
            {
                if (glass.windows[i].wagon_num == wagon && glass.windows[i].state && glass.windows[i].frame == cur)
                    break;
            }
            if (i < glass.num_windows)
            {
                S_vector pos = dir;
                pos.Normalize();
                pos = pos * drive_speed + v1 * .1f;
                glass.windows[i].Hit(pos, glass.glass_material);
                continue;
            }
            while (!hit && cur)
            {
                C_actor **pa = GetFrameActor(cur);
                if (pa && *pa)
                {
                    switch ((*pa)->type)
                    {
                    case ACTOR_PLAYER:
                    case ACTOR_HUMAN:
                    case ACTOR_UNKNOWN_7:
                    case ACTOR_ENTITY:
                        (*pa)->Hit(HIT_SHOT, v1, v2, v3, f, act, 5, NULL);
                        material = (*pa)->GetMaterialID(NULL);
                        hit = true;
                        break;
                    case ACTOR_RAILWAY:
                        material = MAT_METAL_STRUCTURE;
                        hit = true;
                        break;
                    default:
                        cur = cur->m_pOwner;
                        if (!cur)
                            hit = true;
                        break;
                    }
                }
                else
                {
                    for (k = 0; k < persons.size(); k++)
                    {
                        if (persons[k]->frame_model == cur)
                        {
                            persons[k]->Kill(this);
                            material = MAT_PASSENGER;
                            hit = true;
                            break;
                        }
                    }
                    if (driver.frame_model == cur)
                    {
                        driver.Kill(this);
                        material = MAT_PASSENGER;
                        hit = true;
                        if (rlw_state != RLW_NO_DRIVER)
                            saved_rlw_state = rlw_state;
                        SetRLWState(RLW_NO_DRIVER);
                    }
                    else if (!hit)
                    {
                        cur = cur->m_pOwner;
                        if (!cur)
                            hit = true;
                    }
                }
            }
            break;
        }
        for (i = 0; i < glass.num_windows; i++)
        {
            if (glass.windows[i].state == RWINDOW_CRACKED)
                glass.windows[i].frame->SetOn(true);
        }
        player = act_mission->GetGame()->player;
        if (player && (C_railway *)player->car == this && act != player)
            player->frame->SetOn(false);
        DoPanic(10000, wagon);
        return hit;
    }
    }
    return true;
}

// The speed the vehicle may go: the curve ahead along its path (the nodes behind it and in front, from the vehicle's
// position as far as the look-ahead distance) decides it, the cosines of the turns between the path's pieces
// FUNCTION: GAME 0x4914c0
float C_railway::GetSafeSpeed(float &speed)
{
    S_vector pos = front_pos;
    float limit = brake_dist + 4.0f;
    S_vector p = act_mission->web_path->nodes[back_nodes[0]].GetPos();
    S_vector d = p - pos;
    float dist = d.Magnitude();
    float len = dist;
    int n = 0;
    while (dist < limit)
    {
        pos = p;
        p = act_mission->web_path->nodes[forward_nodes[n++]].GetPos();
        p.y += 0.5f;
        d = p - pos;
        len = d.Magnitude();
        dist += len;
    }
    S_vector extra = d * ((limit - dist + len) / len);
    float acc;
    switch (n)
    {
    case 0:
        return max_speed;
    case 1:
    {
        S_vector a = act_mission->web_path->nodes[back_nodes[0]].GetPos() - front_pos;
        a.Normalize();
        S_vector b = extra;
        b.Normalize();
        acc = a.Dot(b);
        break;
    }
    case 2:
    {
        S_vector a = act_mission->web_path->nodes[back_nodes[0]].GetPos() - front_pos;
        a.Normalize();
        S_vector b = act_mission->web_path->nodes[forward_nodes[0]].GetPos() -
            act_mission->web_path->nodes[back_nodes[0]].GetPos();
        b.Normalize();
        float f = a.Dot(b);
        S_vector c = extra;
        c.Normalize();
        speed = c.Dot(b);
        acc = speed * f;
        break;
    }
    default:
    {
        S_vector a = act_mission->web_path->nodes[back_nodes[0]].GetPos() - front_pos;
        a.Normalize();
        S_vector b = act_mission->web_path->nodes[forward_nodes[0]].GetPos() -
            act_mission->web_path->nodes[back_nodes[0]].GetPos();
        b.Normalize();
        float f = a.Dot(b);
        S_vector c = act_mission->web_path->nodes[forward_nodes[1]].GetPos() -
            act_mission->web_path->nodes[forward_nodes[0]].GetPos();
        c.Normalize();
        speed = c.Dot(b);
        acc = speed * f;
        n -= 2;
        for (int i = 0; i < n; i++)
        {
            S_vector d1, d2;
            d1 = act_mission->web_path->nodes[forward_nodes[i + 1]].pos -
                act_mission->web_path->nodes[forward_nodes[i]].pos;
            d1.Normalize();
            d2 = act_mission->web_path->nodes[forward_nodes[i + 2]].pos -
                act_mission->web_path->nodes[forward_nodes[i + 1]].pos;
            d2.Normalize();
            speed = d1.Dot(d2);
            acc = speed * acc;
        }
        break;
    }
    }
    float result;
    if (acc > 0.9865)
        result = max_speed;
    else
        result = acc * (metro ? 15.0f : 10.0f);
    if (result < 5.0f)
        result = 5.0f;
    return result;
}

// Puts the model among the passengers: in the wagon asked for or else the nearest to it, if it has a free seat
// FUNCTION: GAME 0x4928e0
bool C_railway::AddPerson(I3D_model *model, int wagon, int entry)
{
    if (!model)
        return false;
    S_vector pos = model->GetPos();
    int vagon = GetNearestVagon(&pos);
    if (wagon >= 0 && wagon <= (int)num_wagons)
        vagon = wagon;
    if (!seats.NumFreeSeats(vagon))
        return false;
    S_rail_human *rh = new S_rail_human;
    rh->state = RH_CHOOSE_ENTRY;
    rh->frame_model = model;
    rh->should_keep_sitting = rnd(2) + 2;
    rh->wagon_index = vagon;
    rh->animation_number = -1;
    if (wagon == vagon && entry >= 0 && entry < entries.num_entries)
        rh->animation_number = entry;
    rh->new_animation_number = 0;
    rh->anim_container = new C_animations_machine_de_luxe;
    rh->anim_container->Init((I3D_model *)rh->frame_model);
    rh->anim_running = false;
    rh->CreateColl(this);
    I3D_frame *target_e = rh->frame_model->FindChildFrame("targetE", ENUMF_TARGET);
    I3D_frame *target_n = rh->frame_model->FindChildFrame("targetN", ENUMF_TARGET);
    if (target_e)
        target_e->SetOn(false);
    if (target_n)
        target_n->SetOn(false);
    persons.push_back(rh);
    I3D_bvolume bvol;
    rh->frame_model->GetHRBoundVolume(bvol);
    tDynamicCollObject *coll = new tDynamicCollObject;
    coll->type = DYNCOLL_UNKNOWN_42;
    coll->frame = rh->frame_model;
    coll->owner = this;
    ((I3D_bsphere *)coll->bsphere)->radius = bvol.bsphere.radius;
    ((I3D_bsphere *)coll->bsphere)->pos = bvol.bsphere.pos * rh->frame_model->GetMatrix();
    coll->flags = DYNCOLLF_UNKNOWN_2000 | DYNCOLLF_UPPER;
    gifCollision.DynInsert(coll);
    person_colls.push_back(coll);
    return true;
}

// Takes the passenger out, deletes it and gives its model back to the traffic
// FUNCTION: GAME 0x492cc0
void C_railway::RemovePerson(S_rail_human *person)
{
    for (uint i = 0; i < persons.size(); i++)
    {
        if (persons[i] == person)
        {
            I3D_model *model = (I3D_model *)person->frame_model;
            persons.erase(persons.begin() + i);
            person->Done();
            delete person;
            C_traffic_generator::AddFreePerson(model, this, !metro);
        }
    }
}

// The wagon (from 1) whose position is nearest to the point, 0 for the vehicle alone
// FUNCTION: GAME 0x492d60
uint C_railway::GetNearestVagon(S_vector *p)
{
    S_vector d = pos - *p;
    float best = d.Square();
    uint nearest = 0;
    for (int i = 0; i < (int)num_wagons; i++)
    {
        S_vector e = wagon_pos[i] - *p;
        float dist = e.x * e.x + e.y * e.y + e.z * e.z;
        if (dist < best)
        {
            best = dist;
            nearest = i + 1;
        }
    }
    return nearest;
}

// Takes the model's dummies (cbEnumDummies sorts them into raildummy: seats, entries, the dynamic collisions, the
// doors' leaves and the windows, which it pairs up and erases one by one) and builds the vehicle's entries, seats,
// doors, windows and collisions from them; then the windows of each wagon's own dummies
// FUNCTION: GAME 0x492e00
void C_railway::Enumerate()
{
    std::vector<raildummy> dummies;
    rail_window win;
    rail_entry entry;
    rail_seat_row row;
    I3D_frame *model = frame;
    memset(&entries, 0, sizeof(rail_entry));
    memset(&seats, 0, sizeof(seats));
    memset(&doors, 0, sizeof(doors));
    memset(&glass, 0, sizeof(glass));
    doors.Init();
    model->EnumFrames(cbEnumDummies, (uint)&dummies, ENUMF_VISUAL | ENUMF_DUMMY, NULL);
    dummy_colls.clear();
    while (dummies.size())
    {
        uint n = dummies.size();
        uint j;
        switch (dummies[0].type)
        {
        case RDUMMY_COLL:
        {
            tDynamicCollObject *coll = new tDynamicCollObject;
            memset(coll, 0, sizeof(tDynamicCollObject));
            coll->type = DYNCOLL_UNKNOWN_40;
            coll->frame = dummies[0].frame;
            coll->owner = this;
            I3D_frame *frm = dummies[0].frame;
            I3D_bbox bbox = ((I3D_dummy *)frm)->bbox;
            S_vector scale = frm->GetWorldScale();
            ((I3D_bbox &)coll->bbox).min = bbox.min * scale;
            ((I3D_bbox &)coll->bbox).max = bbox.max * scale;
            float radius = Max(scale.x, Max(scale.y, scale.z));
            scale = frm->GetScale();
            ((I3D_bsphere &)coll->bsphere).pos = frm->GetPos();
            ((I3D_bsphere &)coll->bsphere).radius = radius;
            coll->flags = 0;
            coll->frame->SetScale(S_vector(1.0f, 1.0f, 1.0f));
            coll->matrix = new S_matrix;
            S_matrix m = frm->GetLocalMatrix();
            *coll->matrix = m;
            frm->SetScale(scale);
            coll->inv_matrix = new S_matrix;
            *coll->inv_matrix = *coll->matrix;
            coll->inv_matrix->Invert();
            dummy_colls.push_back(coll);
            for (int i = 0; i <= (int)num_wagons; i++)
                gifCollision.DynGetIn(coll, &dyn_colls[i]);
            dummies.erase(dummies.begin());
            break;
        }
        case RDUMMY_ENTRY:
            j = 1;
            while (true)
            {
                if (j >= n)
                    break;
                if (dummies[j].type == RDUMMY_ENTRY && dummies[j].num == dummies[0].num)
                    break;
                j++;
            }
            if (j < n)
            {
                memset(&entry, 0, sizeof(entry));
                if (dummies[0].side == 0)
                {
                    entry.outer_pos = dummies[0].frame->GetPos();
                    entry.inner_pos = dummies[j].frame->GetPos();
                    seats.AddRow(&row);
                }
                if (dummies[0].side == 1)
                {
                    entry.inner_pos = dummies[0].frame->GetPos();
                    entry.outer_pos = dummies[j].frame->GetPos();
                    entries.AddEntry(&entry);
                }
                dummies.erase(dummies.begin() + j);
                dummies.erase(dummies.begin());
            }
            else
                dummies.erase(dummies.begin());
            break;
        case RDUMMY_ROW:
            j = 1;
            while (true)
            {
                if (j >= n)
                    break;
                if (dummies[j].type == RDUMMY_ROW && dummies[j].num == dummies[0].num)
                    break;
                j++;
            }
            if (j < n)
            {
                memset(&row, 0, sizeof(row));
                if (dummies[0].side == 0)
                {
                    row.start = dummies[0].frame->GetPos();
                    row.end = dummies[j].frame->GetPos();
                    seats.AddRow(&row);
                }
                if (dummies[0].side == 1)
                {
                    row.end = dummies[0].frame->GetPos();
                    row.start = dummies[j].frame->GetPos();
                    seats.AddRow(&row);
                }
                dummies.erase(dummies.begin() + j);
                dummies.erase(dummies.begin());
            }
            else
                dummies.erase(dummies.begin());
            break;
        case RDUMMY_DOOR:
            if (dummies[0].side == 0)
            {
                doors.AddFrame(dummies[0].frame, true);
                const char *prop = dummies[0].frame->GetName();
                const char *dot = strchr(prop, '.');
                if (dot)
                    prop = dot + 1;
                for (int i = 0; i < (int)num_wagons; i++)
                {
                    I3D_frame *leaf = wagon_frames[i]->FindChildFrame(prop, ENUMF_ALL);
                    if (leaf)
                        doors.AddFrame(leaf, true);
                }
            }
            if (dummies[0].side == 1)
            {
                doors.AddFrame(dummies[0].frame, false);
                const char *prop = dummies[0].frame->GetName();
                const char *dot = strchr(prop, '.');
                if (dot)
                    prop = dot + 1;
                for (int i = 0; i < (int)num_wagons; i++)
                {
                    I3D_frame *leaf = wagon_frames[i]->FindChildFrame(prop, ENUMF_ALL);
                    if (leaf)
                        doors.AddFrame(leaf, false);
                }
            }
            dummies.erase(dummies.begin());
            break;
        case RDUMMY_GLASS:
            win.frame = dummies[0].frame;
            win.wagon_num = 0;
            win.state = 1;
            glass.AddWindow(&win);
            dummies.erase(dummies.begin());
            break;
        default:
            dummies.erase(dummies.begin());
            break;
        }
    }
    for (int w = 0; w < (int)num_wagons; w++)
    {
        wagon_frames[w]->EnumFrames(cbEnumDummiesVagons, (uint)&dummies, ENUMF_VISUAL | ENUMF_DUMMY, NULL);
        while (dummies.size())
        {
            if (dummies[0].type != RDUMMY_GLASS)
                dummies.erase(dummies.begin());
            else
            {
                win.frame = dummies[0].frame;
                win.wagon_num = w + 1;
                win.state = 1;
                glass.AddWindow(&win);
                dummies.erase(dummies.begin());
            }
        }
    }
}

// Collects the model's dummies by their properties into the vector in user
// FUNCTION: GAME 0x493a00
I3DENUMRET __stdcall cbEnumDummies(I3D_frame *frm, uint user)
{
    // STRING: GAME 0x6348f4
    char row[] = "RAIL_ROW";
    // STRING: GAME 0x6348e8
    char door[] = "RAIL_DOOR";
    // STRING: GAME 0x6348dc
    char entry[] = "RAIL_ENTRY";
    std::vector<raildummy> *dummies = (std::vector<raildummy> *)user;
    const char *name = frm->GetProperty();
    int i, j, k;
    char *buf;
    raildummy rd;
    // STRING: GAME 0x6348d0
    if (!strcmp(name, "RAIL_COLL"))
    {
        rd.type = RDUMMY_COLL;
        rd.side = 0;
        rd.num = 0;
        rd.frame = frm;
        dummies->push_back(rd);
        return I3DENUMRET_OK;
    }
    // STRING: GAME 0x6348c0
    if (!strcmp(name, "[DEFORM GLASS]"))
    {
        rd.type = RDUMMY_GLASS;
        rd.side = 0;
        rd.num = 0;
        rd.frame = frm;
        dummies->push_back(rd);
        return I3DENUMRET_OK;
    }
    int len = strlen(name);
    for (i = 0; i < strlen(entry); i++)
        if (entry[i] != name[i])
            goto next_entry;
    {
        rd.type = RDUMMY_ENTRY;
        rd.frame = frm;
        for (j = i + 1; name[j] != '_';)
            if (j++ == len)
                goto next_entry;
        buf = new char[j - i + 1];
        for (k = i; k < j; k++)
            buf[k - i] = name[k];
        buf[j - i] = '\n';
        rd.num = atoi(buf);
        j++;
        if (j < len)
        {
            char c = name[j];
            if (c == 'S')
                rd.side = 0;
            else if (c == 'E')
                rd.side = 1;
            else
                goto skip_entry;
            dummies->push_back(rd);
            delete[] buf;
            return I3DENUMRET_OK;
        }
    skip_entry:
        delete[] buf;
    }
next_entry:
    for (i = 0; i < strlen(row); i++)
        if (row[i] != name[i])
            goto next_row;
    {
        rd.type = RDUMMY_ROW;
        rd.frame = frm;
        for (j = i + 1; name[j] != '_';)
            if (j++ == len)
                goto next_row;
        buf = new char[j - i + 1];
        for (k = i; k < j; k++)
            buf[k - i] = name[k];
        buf[j - i] = '\n';
        rd.num = atoi(buf);
        j++;
        if (j < len)
        {
            char c = name[j];
            if (c == 'S')
            {
                rd.side = 0;
                dummies->push_back(rd);
                delete[] buf;
                return I3DENUMRET_OK;
            }
            if (c == 'E')
            {
                rd.side = 1;
                dummies->push_back(rd);
                delete[] buf;
                return I3DENUMRET_OK;
            }
        }
        delete[] buf;
    }
next_row:
    for (i = 0; i < strlen(door); i++)
        if (door[i] != name[i])
            goto next_door;
    {
        rd.type = RDUMMY_DOOR;
        rd.frame = frm;
        for (j = i + 1; name[j] != '_';)
            if (j++ == len)
                goto next_door;
        buf = new char[j - i + 1];
        for (k = i; k < j; k++)
            buf[k - i] = name[k];
        buf[j - i] = '\n';
        rd.num = atoi(buf);
        j++;
        if (j < len)
        {
            char c = name[j];
            if (c == 'R')
            {
                rd.side = 0;
                dummies->push_back(rd);
                delete[] buf;
                return I3DENUMRET_OK;
            }
            if (c == 'L')
            {
                rd.side = 1;
                dummies->push_back(rd);
                delete[] buf;
                return I3DENUMRET_OK;
            }
        }
        delete[] buf;
    }
next_door:
    return I3DENUMRET_OK;
}

// Adds the wagon's glass dummies to the vector in user
// FUNCTION: GAME 0x494590
I3DENUMRET __stdcall cbEnumDummiesVagons(I3D_frame *frm, uint user)
{
    if (strcmp(frm->GetProperty(), "[DEFORM GLASS]"))
        return I3DENUMRET_OK;
    raildummy rd;
    rd.type = RDUMMY_GLASS;
    rd.side = 0;
    rd.num = 0;
    rd.frame = frm;
    ((std::vector<raildummy> *)user)->push_back(rd);
    return I3DENUMRET_OK;
}

// Switches the vehicle's light dummies by day or night and makes a projector for each RAIL_PROJECTOR at night
// FUNCTION: GAME 0x4947d0
I3DENUMRET __stdcall cbEnumLights(I3D_frame *frm, uint user)
{
    S_enum_lights *ctx = (S_enum_lights *)user;
    const char *name = frm->GetProperty();
    bool night = act_mission->game->night_mission;
    C_railway *tram = ctx->tram;
    // STRING: GAME 0x634934
    if (!strcmpi(name, "RAIL_LIGHT_ON"))
    {
        if (night)
            frm->SetOn(true);
        else
            frm->SetOn(false);
    }
    // STRING: GAME 0x634924
    if (!strcmpi(name, "RAIL_LIGHT_OFF"))
    {
        if (night)
            frm->SetOn(false);
        else
            frm->SetOn(true);
    }
    // STRING: GAME 0x634914
    if (!strcmpi(name, "RAIL_PROJECTOR"))
    {
        if (night && act_mission->options.game_effects > 1)
        {
            I3D_projector *proj = (I3D_projector *)driver->CreateVisual(I3D_VISUAL_PROJECTOR);
            if (!proj)
                return I3DENUMRET_OK;
            S_rail_light elem;
            elem.pos = frm->GetPos();
            elem.rot = frm->GetRot();
            elem.frame = proj;
            elem.parent = ctx->frame;
            S_vector world_pos = elem.pos * ctx->frame->GetMatrix();
            S_quat q = elem.rot;
            q.RotateByMatrix(ctx->frame->GetMatrix());
            proj->SetPos(world_pos);
            proj->SetRot(q);
            proj->SetScale(S_vector(4.0f, 10.0f, 9.0f));
            proj->SetColor(S_vector(1.0f, 1.0f, 1.0f));
            proj->SetMode(PROJECTOR_MODE_ADD_FADE);
            proj->LinkTo(ctx->frame->GetParent(), 0);
            ITexture *tex;
            // STRING: GAME 0x634900
            if (!igraph->CreateITexture("railprojector.bmp", "", 0, &tex))
            {
                proj->SetTexture(tex);
                tex->Release();
            }
            proj->SetOrthogonal(true);
            proj->SetOn(true);
            proj->Update();
            tram->lights.push_back(elem);
        }
        else
            frm->SetOn(false);
    }
    return I3DENUMRET_OK;
}

// Releases the frame
// FUNCTION: GAME 0x494cc0
I3DENUMRET __stdcall cbEnumDestroy(I3D_frame *frm, uint user)
{
    frm->Release();
    return I3DENUMRET_OK;
}

// Appends a copy of the entry to the array, which is made again one longer
// FUNCTION: GAME 0x494cd0
void rail_entries::AddEntry(rail_entry *entry)
{
    rail_entry *new_entries = new rail_entry[num_entries + 1];
    if (entries)
        memcpy(new_entries, entries, num_entries * sizeof(rail_entry));
    rail_entry *p = &new_entries[num_entries];
    num_entries++;
    *p = *entry;
    if (entries)
        delete[] entries;
    entries = new_entries;
}

// Frees the array
// FUNCTION: GAME 0x494d40
void rail_entries::Destroy()
{
    if (entries)
        delete[] entries;
}

// Takes the using objects of the entries out of the game and frees them
// FUNCTION: GAME 0x494d50
void C_railway::DestroyEntries()
{
    if (inner_entries)
        delete[] inner_entries;
    if (outer_entries)
        delete[] outer_entries;
}

// Makes the using objects of the entries, the vehicle's own first and then each wagon's: outside, where the player
// boards, and inside, where he gets out (a train's or a trolley's; the wagons' are always a train's)
// FUNCTION: GAME 0x494d80
void C_railway::GenEntries()
{
    inner_entries = new C_using_object::S_using_object[(num_wagons + 1) * entries.num_entries];
    outer_entries = new C_using_object::S_using_object[(num_wagons + 1) * entries.num_entries];
    uint i;
    for (i = 0; i < entries.num_entries; i++)
    {
        C_using_object::S_using_object *outer = &outer_entries[i];
        C_using_object::S_using_object *inner = &inner_entries[i];
        outer->pos = entries.entries[i].outer_pos;
        outer->radius_2 = .8f;
        outer->pos.y += 1.0f;
        outer->matrix = &frame->GetMatrix();
        outer->flags = 0;
        outer->frame = NULL;
        outer->actor = this;
        outer->item.id = GAMEITEM_SPECIAL_ACTION;
        outer->item.ammo_loaded = metro ? USING_ACTION_BOARD_TRAIN : USING_ACTION_BOARD_TROLLEY;
        outer->item.using_object = outer;
        inner->pos = entries.entries[i].inner_pos;
        inner->radius_2 = .8f;
        inner->pos.y += 1.0f;
        inner->matrix = &frame->GetMatrix();
        inner->flags = 0;
        inner->frame = NULL;
        inner->actor = this;
        inner->item.id = GAMEITEM_SPECIAL_ACTION;
        inner->item.ammo_loaded = metro ? USING_ACTION_EXIT_TRAIN : USING_ACTION_EXIT_TROLLEY;
        inner->item.using_object = inner;
    }
    for (int w = 0; w < num_wagons; w++)
    {
        for (i = 0; i < entries.num_entries; i++)
        {
            C_using_object::S_using_object *outer = &outer_entries[(w + 1) * entries.num_entries + i];
            C_using_object::S_using_object *inner = &inner_entries[(w + 1) * entries.num_entries + i];
            outer->pos = entries.entries[i].outer_pos;
            outer->radius_2 = .8f;
            outer->pos.y += 1.0f;
            outer->matrix = &wagon_frames[w]->GetMatrix();
            outer->actor = this;
            outer->flags = 0;
            outer->frame = NULL;
            outer->item.id = GAMEITEM_SPECIAL_ACTION;
            outer->item.ammo_loaded = USING_ACTION_BOARD_TRAIN;
            outer->item.using_object = outer;
            inner->pos = entries.entries[i].inner_pos;
            inner->radius_2 = .8f;
            inner->pos.y += 1.0f;
            inner->matrix = &wagon_frames[w]->GetMatrix();
            inner->flags = 0;
            inner->frame = NULL;
            inner->actor = this;
            inner->item.id = GAMEITEM_SPECIAL_ACTION;
            inner->item.ammo_loaded = USING_ACTION_EXIT_TRAIN;
            inner->item.using_object = inner;
        }
    }
}

// Puts the using objects outside the vehicle into the game's
// FUNCTION: GAME 0x495100
void C_railway::AddOuterEntries()
{
    for (uint i = 0; i < (num_wagons + 1) * entries.num_entries; i++)
        act_mission->GetGame()->GetUsingObject().AddObject(&outer_entries[i]);
}

// The same for the ones inside
// FUNCTION: GAME 0x495160
void C_railway::AddInnerEntries()
{
    for (uint i = 0; i < (num_wagons + 1) * entries.num_entries; i++)
        act_mission->GetGame()->GetUsingObject().AddObject(&inner_entries[i]);
}

// Takes the using objects outside the vehicle out of the game's
// FUNCTION: GAME 0x4951c0
void C_railway::RemoveOuterEntries()
{
    for (uint i = 0; i < (num_wagons + 1) * entries.num_entries; i++)
        act_mission->GetGame()->GetUsingObject().DelObject(&outer_entries[i]);
}

// The same for the ones inside
// FUNCTION: GAME 0x495220
void C_railway::RemoveInnerEntries()
{
    for (uint i = 0; i < (num_wagons + 1) * entries.num_entries; i++)
        act_mission->GetGame()->GetUsingObject().DelObject(&inner_entries[i]);
}

// The entry nearest the actor, outside: its place, the direction in and the wagon it is in (the vehicle's own for 0)
// FUNCTION: GAME 0x495280
bool C_railway::GetOuterEntry(C_actor *act, S_vector *pos, S_vector *dir, uint *wagon)
{
    S_vector act_pos = act->frame->GetPos();
    *wagon = GetNearestVagon(&act_pos);
    if (*wagon > num_wagons || !entries.num_entries)
        return false;
    int best = -1;
    float best_dist = 1000.0f;
    uint i;
    S_matrix m;
    S_vector p;
    if (*wagon)
    {
        for (i = 0; i < entries.num_entries; i++)
        {
            m = wagon_frames[*wagon - 1]->GetMatrix();
            p = entries.entries[i].outer_pos * m;
            S_vector d = p - act->frame->GetWorldPos();
            float dist = d.y * d.y + d.x * d.x + d.z * d.z;
            if (dist < best_dist)
            {
                best_dist = dist;
                *pos = p;
                best = i;
            }
        }
        S_vector d;
        d = entries.entries[best].inner_pos * m;
        d -= *pos;
        d.Normalize();
        *dir = d;
    }
    else
    {
        for (i = 0; i < entries.num_entries; i++)
        {
            m = frame->GetMatrix();
            p = entries.entries[i].outer_pos * m;
            float dist = (p - act->frame->GetWorldPos()).Square();
            if (dist < best_dist)
            {
                best_dist = dist;
                *pos = p;
                best = i;
            }
        }
        S_vector d;
        d = entries.entries[best].inner_pos * m;
        d -= *pos;
        d.Normalize();
        *dir = d;
    }
    return true;
}

// The entry of the wagon, from 1, 0 for the vehicle itself: where outside it stands, and the horizontal direction in
// FUNCTION: GAME 0x495780
bool C_railway::GetOuterEntry(uint wagon, uint entry, S_vector *pos, S_vector *dir)
{
    if (wagon > num_wagons || entry >= entries.num_entries || !entries.num_entries)
        return false;
    S_matrix m;
    if (wagon)
    {
        m = wagon_frames[wagon - 1]->GetMatrix();
        *pos = entries.entries[entry].outer_pos * m;
        S_vector d;
        d = entries.entries[entry].inner_pos * m;
        d.x -= pos->x;
        d.z -= pos->z;
        d.y = 0.0f;
        d.Normalize();
        *dir = d;
    }
    else
    {
        m = frame->GetMatrix();
        *pos = entries.entries[entry].outer_pos * m;
        S_vector d;
        d = entries.entries[entry].inner_pos * m;
        d.x -= pos->x;
        d.z -= pos->z;
        d.y = 0.0f;
        d.Normalize();
        *dir = d;
    }
    return true;
}

// The entry nearest the actor, inside: its place, the direction out of it and the human's wagon
// FUNCTION: GAME 0x495ab0
bool C_railway::GetInnerEntry(C_actor *act, S_vector *pos, S_vector *dir, uint *wagon)
{
    int best = -1;
    float best_dist = 1000.0f;
    for (uint i = 0; i < entries.num_entries; i++)
    {
        S_vector p = act->GetPos() + act->GetDir();
        S_vector d = entries.entries[i].inner_pos - p;
        float dist = d.Square();
        if (dist < best_dist)
        {
            best_dist = dist;
            *pos = entries.entries[i].inner_pos;
            best = i;
        }
    }
    S_vector d;
    d = entries.entries[best].outer_pos - entries.entries[best].inner_pos;
    d.Normalize();
    *dir = d;
    if (wagon)
        *wagon = ((C_human *)act)->wagon;
    return true;
}

// The entry nearest the actor: inside for a human in a car (here, the vehicle), else outside
// FUNCTION: GAME 0x495d30
bool C_railway::GetEntry(C_actor *act, S_vector *pos, S_vector *dir, uint *wagon)
{
    if (act->type == ACTOR_PLAYER || act->type == ACTOR_HUMAN || act->type == ACTOR_ENTITY)
    {
        if (((C_human *)act)->car)
            return GetInnerEntry(act, pos, dir, wagon);
    }
    return GetOuterEntry(act, pos, dir, wagon);
}

// The entry nearest the position, outside and on the vehicle's right (x above 0): its place, the horizontal direction
// in and the wagon
// FUNCTION: GAME 0x495d70
bool C_railway::GetOuterEntry(S_vector pos, S_vector *out_pos, S_vector *dir, uint *wagon)
{
    *wagon = GetNearestVagon(&pos);
    if (*wagon > num_wagons || !entries.num_entries)
        return false;
    int best = -1;
    float best_dist = 1000.0f;
    uint i;
    S_matrix m;
    S_vector p;
    if (*wagon)
    {
        for (i = 0; i < entries.num_entries; i++)
        {
            m = wagon_frames[*wagon - 1]->GetMatrix();
            p = entries.entries[i].outer_pos * m;
            float dist = (p - pos).Square();
            if (dist < best_dist && entries.entries[i].outer_pos.x > 0.0f)
            {
                best_dist = dist;
                *out_pos = p;
                best = i;
            }
        }
        S_vector d;
        d = entries.entries[best].inner_pos * m;
        d.x -= out_pos->x;
        d.z -= out_pos->z;
        d.y = 0.0f;
        d.Normalize();
        *dir = d;
    }
    else
    {
        for (i = 0; i < entries.num_entries; i++)
        {
            m = frame->GetMatrix();
            p = entries.entries[i].outer_pos * m;
            float dist = (p - pos).Square();
            if (dist < best_dist && entries.entries[i].outer_pos.x > 0.0f)
            {
                best_dist = dist;
                *out_pos = p;
                best = i;
            }
        }
        S_vector d;
        d = entries.entries[best].inner_pos * m;
        d.x -= out_pos->x;
        d.z -= out_pos->z;
        d.y = 0.0f;
        d.Normalize();
        *dir = d;
    }
    return true;
}

// The entry nearest the position, inside and on the vehicle's right: its place and the horizontal direction out
// FUNCTION: GAME 0x496200
bool C_railway::GetInnerEntry(S_vector pos, S_vector *out_pos, S_vector *dir, uint *wagon)
{
    int best = -1;
    float best_dist = 1000.0f;
    for (uint i = 0; i < entries.num_entries; i++)
    {
        float dist = (entries.entries[i].inner_pos - pos).Square();
        if (dist < best_dist && entries.entries[i].inner_pos.x > 0.0f)
        {
            best_dist = dist;
            *out_pos = entries.entries[i].inner_pos;
            best = i;
        }
    }
    S_vector d;
    d = entries.entries[best].outer_pos - entries.entries[best].inner_pos;
    d.y = 0.0f;
    d.Normalize();
    *dir = d;
    return true;
}

// Gives the traffic generator the entries of the vehicle and its wagons that face right (the entries' x above 0), for
// the pedestrians waiting in the station: the wagon, the entry, its place and the direction in, in the world
// FUNCTION: GAME 0x496400
void C_railway::ExportEntries()
{
    std::vector<rentry> *list = new std::vector<rentry>;
    list->clear();
    S_matrix m, rot;
    rentry r;
    uint i;
    {
        m = frame->GetMatrix();
        rot = m;
        rot(3).z = 0.0f;
        rot(3).y = 0.0f;
        rot(3).x = 0.0f;
        for (i = 0; i < entries.num_entries; i++)
        {
            r.wagon = 0;
            r.entry = i;
            r.pos = entries.entries[i].outer_pos * m;
            r.dir = entries.entries[i].inner_pos - entries.entries[i].outer_pos;
            r.dir.y = 0.0f;
            if (r.dir.x < 0.0f)
            {
                r.dir.Normalize();
                r.dir *= rot;
                list->push_back(r);
            }
        }
    }
    for (int w = 0; w < num_wagons; w++)
    {
        m = wagon_frames[w]->GetMatrix();
        rot = m;
        rot(3).z = 0.0f;
        rot(3).y = 0.0f;
        rot(3).x = 0.0f;
        for (i = 0; i < entries.num_entries; i++)
        {
            r.wagon = w + 1;
            r.entry = i;
            r.pos = entries.entries[i].outer_pos * m;
            r.dir = entries.entries[i].inner_pos - entries.entries[i].outer_pos;
            r.dir.y = 0.0f;
            if (r.dir.x < 0.0f)
            {
                r.dir.Normalize();
                r.dir *= rot;
                list->push_back(r);
            }
        }
    }
    C_traffic_generator::RailwayInStation(this, *list);
    delete list;
}

// Opens the doors unless they are open
// FUNCTION: GAME 0x496800
void rail_doors::Open()
{
    if (state != RDOORS_OPEN)
        state = RDOORS_OPENING;
}

// Closes the doors unless they are closed
// FUNCTION: GAME 0x496810
void rail_doors::Close()
{
    if (state != RDOORS_CLOSED)
        state = RDOORS_CLOSING;
}

// Turns the leaves to the current angle: the first set about the axis by -angle, the second by angle
// FUNCTION: GAME 0x496820
void rail_doors::Place()
{
    rot1.Make(axis, -angle);
    rot2.Make(axis, angle);
    uint i;
    for (i = 0; i < num_leaves1; i++)
        leaves1[i]->SetRot(rot1);
    for (i = 0; i < num_leaves2; i++)
        leaves2[i]->SetRot(rot2);
}

// Moves the doors on their way (time in milliseconds, speed in radians a second) and stops them at the end
// FUNCTION: GAME 0x496900
void rail_doors::Tick(int time)
{
    switch (state)
    {
    case RDOORS_OPENING:
        angle += speed * (time * .001f);
        if (angle >= PI / 2)
        {
            angle = PI / 2;
            state = RDOORS_OPEN;
        }
        break;
    case RDOORS_CLOSING:
        angle -= speed * (time * .001f);
        if (angle <= 0.0f)
        {
            angle = 0.0f;
            state = RDOORS_CLOSED;
        }
        break;
    }
}

// Adds a leaf to the first or the second set
// FUNCTION: GAME 0x496970
void rail_doors::AddFrame(I3D_frame *frm, bool first)
{
    if (first)
    {
        I3D_frame **new_leaves = new I3D_frame *[num_leaves1 + 1];
        if (leaves1)
            memcpy(new_leaves, leaves1, num_leaves1 * sizeof(I3D_frame *));
        new_leaves[num_leaves1] = frm;
        num_leaves1++;
        if (leaves1)
            delete leaves1;
        leaves1 = new_leaves;
    }
    else
    {
        I3D_frame **new_leaves = new I3D_frame *[num_leaves2 + 1];
        if (leaves2)
            memcpy(new_leaves, leaves2, num_leaves2 * sizeof(I3D_frame *));
        new_leaves[num_leaves2] = frm;
        num_leaves2++;
        if (leaves2)
            delete leaves2;
        leaves2 = new_leaves;
    }
}

// Closed, without leaves, about the y axis, opening by 90 degrees at 1 radian a second
// FUNCTION: GAME 0x496a40
void rail_doors::Init()
{
    num_leaves2 = 0;
    num_leaves1 = 0;
    angle = 0.0f;
    max_angle = PI / 2;
    speed = 1.0f;
    axis = S_vector(0.0f, 1.0f, 0.0f);
    state = RDOORS_CLOSED;
}

// Frees the leaves' arrays
// FUNCTION: GAME 0x496a90
void rail_doors::Destroy()
{
    if (leaves1)
        delete leaves1;
    if (leaves2)
        delete leaves2;
}

// Takes the first window's material and makes a copy of it with the glass' texture
// FUNCTION: GAME 0x496ac0
void rail_glass::GameInit()
{
    if (num_windows > 0)
    {
        orig_material = ((I3D_object *)windows[0].frame)->GetMesh()->GetLOD(0)->GetFGroup(0)->material;
        glass_material = driver->CreateMaterial();
        glass_material->Duplicate(orig_material);
        ITexture *texture;
        if (!igraph->CreateITexture(
                // STRING: GAME 0x634944
                "2ft19r.bmp",
                // STRING: GAME 0x634950
                "2ft19r+.bmp", 4, &texture))
                {
            glass_material->SetTexture(texture);
            texture->Release();
        }
    }
}

// Restores the windows and frees them and the copy of the material
// FUNCTION: GAME 0x496b40
void rail_glass::GameDone()
{
    for (uint i = 0; i < num_windows; i++)
        windows[i].Restore(orig_material);
    if (windows)
        delete windows;
    windows = NULL;
    num_windows = 0;
    if (glass_material)
        glass_material->Release();
}

// Adds a window, copied
// FUNCTION: GAME 0x496ba0
void rail_glass::AddWindow(rail_window *window)
{
    rail_window *new_windows = new rail_window[num_windows + 1];
    if (windows)
        memcpy(new_windows, windows, num_windows * sizeof(rail_window));
    rail_window *dst = &new_windows[num_windows];
    num_windows++;
    *dst = *window;
    if (windows)
        delete windows;
    windows = new_windows;
}

// Puts the frame's own mesh back and frees the copy
// FUNCTION: GAME 0x496c40
void rail_window::Restore(I3D_material *mat)
{
    state = RWINDOW_WHOLE;
    if (cracked_mesh)
    {
        ((I3D_object *)frame)->SetMesh(own_mesh, false);
        cracked_mesh->GetLOD(0)->fgroups->SetMaterial(mat);
        mat->Release();
        cracked_mesh->Release();
        own_mesh = NULL;
        cracked_mesh = NULL;
    }
    frame->SetOn(true);
}

// A whole window gets a copy of its mesh with the cracked glass (the frame switched off until Hit's end), a cracked
// one breaks
// FUNCTION: GAME 0x496cb0
void rail_window::Hit(const S_vector &pos, I3D_material *mat)
{
    switch (state)
    {
    case RWINDOW_CRACKED:
        act_mission->GetGame()->glass.BreakGlass(frame, &pos, "lustr1.wav", true);
        state = RWINDOW_BROKEN;
        frame->SetOn(false);
        break;
    case RWINDOW_WHOLE:
        own_mesh = ((I3D_object *)frame)->mesh;
        cracked_mesh = driver->CreateMesh();
        if (own_mesh && cracked_mesh)
        {
            cracked_mesh->Duplicate(own_mesh);
            cracked_mesh->GetLOD(0)->GetFGroup(0)->material->ref++;
            if (mat)
                cracked_mesh->GetLOD(0)->GetFGroup(0)->SetMaterial(mat);
            state = RWINDOW_CRACKED;
            ((I3D_object *)frame)->SetMesh(cracked_mesh, false);
            frame->SetOn(false);
        }
        break;
    }
}

// Whether the vehicle is in the station (the states 1 to 4)
// FUNCTION: GAME 0x496d90
bool C_railway::isInStation()
{
    if ((uint)rlw_state > 0 && (uint)rlw_state <= 4)
        return true;
    return false;
}

// Whether the vehicle is standing (in some states only when its speed is 0)
// FUNCTION: GAME 0x496db0
bool C_railway::isInStop()
{
    switch (rlw_state)
    {
    case 7:
    case 10:
    case 11:
    case 12:
        if (!metro && drive_speed == 0.0f)
            return true;
        return false;
    case 1:
    case 2:
    case 3:
    case 4:
        return true;
    }
    return false;
}

// The vehicle's frame, or a wagon's
// FUNCTION: GAME 0x496e10
I3D_frame *C_railway::GetVagon(uint wagon)
{
    if (!wagon)
        return frame;
    if (wagon > num_wagons)
        return NULL;
    return wagon_frames[wagon - 1];
}

// The saved game's copies (PS2 names): the linker folded them with ai_follow_manager's, whose copies it kept
// FUNCTION: GAME 0x5334f0 FOLDED
void C_railway::MemWrite(void **buf, void *src, uint count, uint size)
{
    memcpy(*buf, src, count * size);
    *buf = (uchar *)*buf + count * size;
}

// FUNCTION: GAME 0x532150 FOLDED
void C_railway::MemRead(void **buf, void *dst, uint count, uint size)
{
    memcpy(dst, *buf, count * size);
    *buf = (uchar *)*buf + count * size;
}

// FUNCTION: GAME 0x496e40
int C_railway::SaveGameGetSize()
{
    return C_actor::SaveGameGetSize() + 80;
}

// Saves the position, direction and the route's state
// FUNCTION: GAME 0x496e50
bool C_railway::SaveGameSave(void **buf)
{
    int version = 1;
    if (!C_actor::SaveGameSave(buf))
        return false;
    MemWrite(buf, &version, 1, sizeof(version));
    MemWrite(buf, &pos, 1, sizeof(S_vector));
    MemWrite(buf, &dir, 1, sizeof(S_vector));
    MemWrite(buf, &move_velocity, 1, sizeof(S_vector));
    MemWrite(buf, &safe_speed, 1, sizeof(int));
    MemWrite(buf, &drive_speed, 1, sizeof(float));
    MemWrite(buf, &max_speed, 1, sizeof(float));
    MemWrite(buf, &acceleration, 1, sizeof(float));
    MemWrite(buf, &brake_dist, 1, sizeof(float));
    MemWrite(buf, &state_time, 1, sizeof(int));
    MemWrite(buf, &rlw_state, 1, sizeof(int));
    MemWrite(buf, &unknown_e4, 1, sizeof(int));
    MemWrite(buf, &visibility, 1, sizeof(int));
    MemWrite(buf, &back_nodes[0], 1, sizeof(uint));
    return true;
}

// Loads what SaveGameSave saved and puts the vehicle back on its route
// FUNCTION: GAME 0x496f80
bool C_railway::SaveGameLoad(void **buf)
{
    int version;
    int index;
    if (!C_actor::SaveGameLoad(buf))
        return false;
    MemRead(buf, &version, 1, sizeof(version));
    if (version != 1)
        return false;
    MemRead(buf, &pos, 1, sizeof(S_vector));
    MemRead(buf, &dir, 1, sizeof(S_vector));
    MemRead(buf, &move_velocity, 1, sizeof(S_vector));
    MemRead(buf, &safe_speed, 1, sizeof(int));
    MemRead(buf, &drive_speed, 1, sizeof(float));
    MemRead(buf, &max_speed, 1, sizeof(float));
    MemRead(buf, &acceleration, 1, sizeof(float));
    MemRead(buf, &brake_dist, 1, sizeof(float));
    MemRead(buf, &state_time, 1, sizeof(int));
    MemRead(buf, &rlw_state, 1, sizeof(int));
    MemRead(buf, &unknown_e4, 1, sizeof(int));
    MemRead(buf, &version, 1, sizeof(int));
    MemRead(buf, &index, 1, sizeof(uint));
    dir.Normalize();
    unknown_70.Zero();
    front_pos = dir * (vehicle_length * 0.5f) + pos;
    back_pos = pos - dir * (vehicle_length * 0.5f);
    back_nodes.clear();
    forward_nodes.clear();
    if (!(act_mission->web_path->nodes[index].flags & (WEBF_UNKNOWN_4 | WEBF_UNKNOWN_8)))
        index = act_mission->web_path->GetNearNode(pos, -1.0f, WEBF_UNKNOWN_4 | WEBF_UNKNOWN_8);
    back_nodes.push_back(index);
    CreateBackVector(front_pos);
    CreateForwardVector(back_nodes[0], brake_dist + 5.0f);
    Place(1);
    if (GetFrame())
    {
        GetFrame()->SetPos(pos);
        GetFrame()->SetDir(dir, 0.0f);
        GetFrame()->Update();
    }
    if (rlw_state == RLW_SIGNAL_SLOW)
    {
        if (fabs(GetFrame()->GetDir().x) > fabs(GetFrame()->GetDir().z))
            semaphore_phase = &act_mission->GetGame()->semaphore_phase_x;
        else
            semaphore_phase = &act_mission->GetGame()->semaphore_phase_z;
    }
    return true;
}

// The sphere moving as pos + dir * t (radius r0) against the curve's spheres (radius r1), each segment's quadratic in
// its own time, scaled back to the curve's: the earliest time found, 1000 for none (PC only, the name is ours; the
// original has it inline in a header, its copy deferred)
// FUNCTION: GAME 0x497710
float g_collision::TestSphereCurve(const S_vector &pos, const S_vector &dir, float r0,
    const std::vector<gcol_CurveSphereStruct> *curve, float r1)
{
    float best = 1000.0f;
    if (curve->size() >= 2)
    {
    const gcol_CurveSphereStruct *a1 = curve->begin(), *a = a1++;
    float rr = (r0 + r1) * (r0 + r1);
    while (a1 < curve->end())
    {
        float dt = a1->t - a->t;
        S_vector seg = a1->pos - a->pos;
        S_vector dd = dir * dt;
        S_vector p = a->pos - (pos + dir * a->t);
        S_vector d = seg - dd;
        float t;
        if (p.Square() <= rr)
            t = 0.0f;
        else
        {
            float A = d.Square();
            if (A <= 0.0f)
                t = -1.0f;
            else
            {
                float B = 2.0f * d.Dot(p);
                float disc = B * B - 4.0f * A * (p.Dot(p) - rr);
                if (disc < 0.0f)
                    t = disc;
                else
                {
                    float root = (float)sqrt(disc);
                    float inv = 1.0f / (2.0f * A);
                    float t1 = (root - B) * inv;
                    float t2 = (-B - root) * inv;
                    t = t1 < t2 ? t1 : t2;
                }
            }
        }
        t = t * dt + a->t;
        if (t >= 0.0f)
        {
            if (t < best)
                best = t;
            else
                break;
        }
        a = a1++;
    }
    }
    return best;
}

// Takes a lock: a passenger that chose its entry or stands up to leave holds one until it is in or out
// FUNCTION: GAME 0x497980
void C_railway::Lock()
{
    ++lock_count;
}

// Gives a lock back: a passenger linked to the vehicle or walked away from it, or Kill for one dying on the way
// FUNCTION: GAME 0x497990
void C_railway::Unlock()
{
    if (lock_count > 0)
        --lock_count;
}

// Whether the node may be used again: true for none, for one not seen in the last minute (which it notes) and for a
// new one; false for one that was
// FUNCTION: GAME 0x4979b0
bool C_railway::IsOnSchedule(C_web_node *node)
{
    if (!node)
        return true;
    for (uint i = 0; i < shedule_table.size(); i++)
    {
        if (shedule_table[i]->node == node)
        {
            if (timeGetTime() - shedule_table[i]->time > 60000)
            {
                shedule_table[i]->time = timeGetTime();
                return true;
            }
            return false;
        }
    }
    S_shedule_item *item = new S_shedule_item(node, timeGetTime());
    shedule_table.push_back(item);
    return true;
}

// Every passenger of the wagon panics, for a random part of the time
// FUNCTION: GAME 0x497c00
void C_railway::DoPanic(uint time, uint how)
{
    for (uint i = 0; i < persons.size(); i++)
    {
        if (persons[i] != &driver && persons[i]->wagon_index == how)
        {
            float r = rnd() * .2f + .8f;
            persons[i]->DoPanic((uint)(r * time));
        }
    }
}

// Sets the state; the frame comes on for 0
// FUNCTION: GAME 0x497c90
void C_railway::SetRLWState(uint state)
{
    if (state == 0 && sounds[0])
        sounds[0]->SetOn(true);
    rlw_state = state;
}

// The trolley's shoe: while the vehicle is near the first node of its way and the way bends by a small angle, the
// shoe's sound plays and its frame is updated
// FUNCTION: GAME 0x497cc0
void C_railway::DoBrousicka()
{
    if (back_nodes.size() > 1 && forward_nodes.size())
    {
        C_web_node *nodes = act_mission->web_path->nodes;
        const S_vector &a = nodes[back_nodes[0]].pos;
        S_vector d = frame->GetPos() - a;
        if (d.Square() > 25.0f)
            return;
        S_vector v1 = nodes[back_nodes[1]].pos - a;
        S_vector v2 = nodes[forward_nodes[0]].pos - a;
        float angle = v1.AngleTo(v2);
        if (angle > .17f && angle < 1.4007964f && sounds[2])
        {
            sounds[2]->SetVolume(1.0f);
            if (!sounds[2]->IsPlaying(false))
                sounds[2]->SetOn(true);
            S_vector dir = frame->GetDir();
            S_vector pos = dir * drive_speed;
            sounds[2]->Update();
        }
    }
}

// A vehicle that goes out of the game (ACTSTATE_NONE) lets go of its sounds and takes its frames, collisions and
// radar marks away
// FUNCTION: GAME 0x498060
void C_railway::ChangeState()
{
    int i;
    if (act_state == ACTSTATE_NONE)
    {
        for (i = 0; i < 3; i++)
        {
            if (sounds[i])
            {
                sounds[i]->LinkTo(NULL, 0);
                sounds[i]->SetOn(false);
                sounds[i]->Release();
                sounds[i] = NULL;
            }
        }
        if (unknown_298)
        {
            unknown_298->LinkTo(NULL, 0);
            unknown_298->SetOn(false);
            unknown_298->Release();
            unknown_298 = NULL;
        }
        if (!metro)
            mafiaIndicators.RadarRemoveObject(colls);
        gifCollision.DynErase(colls);
        frame->SetOn(false);
        for (i = 0; i < num_wagons; i++)
        {
            wagon_frames[i]->SetOn(false);
            if (!metro)
                mafiaIndicators.RadarRemoveObject(&colls[i + 1]);
            gifCollision.DynErase(&colls[i + 1]);
        }
    }
}

// The vehicle's outline: the frame's and the wagons' boxes, 0.5 out on each side, as one polygon of their bottom
// corners (the frame's first three, each wagon's two on the far side, back along the wagons' other side, and the
// frame's last)
// FUNCTION: GAME 0x498180
bool C_railway::GetBoundPoints2D(std::vector<S_vector2> &points, I3D_frame *frm)
{
    bool ok = false;
    if (frame)
        ok = true;
    int i;
    for (i = 0; i < (int)num_wagons; i++)
        if (wagon_frames[i] == frame)
            ok = true;
    if (ok)
    {
        I3D_bvolume bvol = frame->GetHRBoundVolume();
        S_vector v1;
        v1.x = bvol.bbox.min.x - .5f;
        v1.y = bvol.bbox.min.y;
        v1.z = bvol.bbox.min.z - .5f;
        v1 *= frame->GetMatrix();
        S_vector2 p1(v1.x, v1.z);
        points.push_back(p1);
        {
            S_vector v2;
            v2.x = bvol.bbox.min.x - .5f;
            v2.y = bvol.bbox.min.y;
            v2.z = bvol.bbox.max.z + .5f;
            v2 *= frame->GetMatrix();
            S_vector2 p2(v2.x, v2.z);
            points.push_back(p2);
        }
        {
            S_vector v3;
            v3.x = bvol.bbox.max.x + .5f;
            v3.y = bvol.bbox.min.y;
            v3.z = bvol.bbox.max.z + .5f;
            v3 *= frame->GetMatrix();
            S_vector2 p3(v3.x, v3.z);
            points.push_back(p3);
        }
        for (i = 0; i < (int)num_wagons; i++)
        {
            I3D_frame *wagon = wagon_frames[i];
            bvol = wagon->GetHRBoundVolume();
            S_vector v4;
            v4.x = bvol.bbox.min.x - .5f;
            v4.y = bvol.bbox.min.y;
            v4.z = bvol.bbox.max.z + .5f;
            v4 *= wagon->GetMatrix();
            S_vector2 p4(v4.x, v4.z);
            points.push_back(p4);
            S_vector v5;
            v5.x = bvol.bbox.max.x + .5f;
            v5.y = bvol.bbox.min.y;
            v5.z = bvol.bbox.max.z + .5f;
            v5 *= wagon->GetMatrix();
            S_vector2 p5(v5.x, v5.z);
            points.push_back(p5);
        }
        for (i = (int)num_wagons - 1; i >= 0; i--)
        {
            I3D_frame *wagon = wagon_frames[i];
            bvol = wagon->GetHRBoundVolume();
            S_vector v6;
            v6.x = bvol.bbox.max.x + .5f;
            v6.y = bvol.bbox.min.y;
            v6.z = bvol.bbox.min.z - .5f;
            v6 *= wagon->GetMatrix();
            S_vector2 p6(v6.x, v6.z);
            points.push_back(p6);
            S_vector v7;
            v7.x = bvol.bbox.min.x - .5f;
            v7.y = bvol.bbox.min.y;
            v7.z = bvol.bbox.min.z - .5f;
            v7 *= wagon->GetMatrix();
            S_vector2 p7(v7.x, v7.z);
            points.push_back(p7);
        }
        bvol = frame->GetHRBoundVolume();
        {
            S_vector v8;
            v8.x = bvol.bbox.max.x + .5f;
            v8.y = bvol.bbox.min.y;
            v8.z = bvol.bbox.min.z - .5f;
            v8 *= frame->GetMatrix();
            S_vector2 p8(v8.x, v8.z);
            points.push_back(p8);
        }
    }
    return ok;
}

// The node the first open connection of a node leads to, the node itself if it has none (the name is ours)
static inline C_web_node *NextNode(C_web_node *nodes, C_web_node *node)
{
    for (int i = 0; i < node->num_conns; i++)
    {
        C_web_connection *conn = node->GetConn(i);
        if (conn->flags & WEBF_ONEWAY)
            continue;
        return &nodes[conn->node];
    }
    return node;
}

// Looks for the vehicle ahead (ahead), the vehicle itself if there is none: along the nodes' open connections from
// the first node of its route, the first node another vehicle's route starts at
// FUNCTION: GAME 0x498790
void C_railway::FindVehicleAhead()
{
    if (back_nodes.size())
    {
        C_web_node *nodes = act_mission->web_path->nodes;
        C_web_node *start = &nodes[back_nodes[0]];
        C_web_node *node = NextNode(nodes, start);
        while (start != node)
        {
            for (uint j = 0; j != rlws.size(); j++)
            {
                C_railway *other = rlws[j];
                if (other->back_nodes.size() && &nodes[other->back_nodes[0]] == node)
                {
                    ahead = other;
                    goto found;
                }
            }
            node = NextNode(nodes, node);
        }
    }
found:
    if (!ahead)
        ahead = this;
}

// Stops the vehicle (RLW_FOLLOW) when the one ahead is closer than 40 metres
// FUNCTION: GAME 0x498910
void C_railway::StopBehindVehicleAhead()
{
    if (ahead && ahead != this && rlw_state != RLW_FOLLOW)
    {
        I3D_frame *frm = ahead->frame;
        if ((int)ahead->num_wagons > 0)
            frm = ahead->wagon_frames[ahead->num_wagons - 1];
        if ((frame->GetPos() - frm->GetPos()).Square() < 1600.0f)
        {
            saved_rlw_state = rlw_state;
            SetRLWState(RLW_FOLLOW);
        }
    }
}

// Puts the rain dummy into the vehicle (the wagon's frame) with its bounding box, or takes it out
// FUNCTION: GAME 0x4989a0
void C_railway::SetRainDummy(bool in, I3D_frame *frm)
{
    if (in)
    {
        if (act_mission->scene->GetWeatherSystemParam(WS_PARAM_ON))
        {
            I3D_bvolume bvol;
            frm->GetHRBoundVolume(bvol);
            rain_dummy->SetBBox(bvol.bbox);
            rain_dummy->LinkTo(frm, 0);
            act_mission->scene->WeatherSystemAddDummy(rain_dummy);
        }
    }
    else
    {
        rain_dummy->LinkTo(NULL, 0);
        act_mission->scene->WeatherSystemDelDummy(rain_dummy);
    }
}

// FUNCTION: GAME 0x498a70
int C_railway::GetMaterialID(I3D_frame *frm)
{
    return material;
}

// The speed along the frame's direction
// FUNCTION: GAME 0x498a80
void C_railway::GetActorSpeed(S_vector &speed)
{
    speed = frame->GetDir();
    speed *= drive_speed;
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_railway::NetDirect(uint time)
{
}
