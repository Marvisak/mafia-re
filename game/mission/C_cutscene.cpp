// The in-engine cutscenes (PC only, the file's name is ours; the PC's file is the one at 0x5983e0-0x5a075c,
// docs/game-files.md): the keys' interpolation (0x598410, 0x5984e0), the object's initializer (0x59cc20) and constructor
// (0x59cc40) mid-file, Close (0x59c6a0), Tick (0x599610), and the seagulls after them (their constructor 0x59e7e0).
// Written so far: SetState, which C_game::Done calls, the vtable's getters, RemoveActor, which C_actor's destructor
// calls, with the object at 0x90's, Close, which C_mission::Close calls, with what it calls of the cutscene
// (RemoveActors, ReleaseUnknown34), of the object at 0x80 (Clear) and of the object at 0x90 (Clear, ReleaseUnknown0),
// and IsPlaying; the vtable's getters, setters and the functions that pass on to the objects at 0x80 and 0x90 (the
// recordings', which the scripts call, among them), and the object at 0x80's constructor. Stubs: the vtable's other
// functions, the two objects' functions those pass on to, the seagulls' constructor, which C_mission::CreateActor
// calls, and the actors' records' Reset, which lies in another file's range.

#include "mission/C_cutscene.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "ui/g_dabing.h"
#include "ui/g_indicators.h"

#include <string.h>

#include "stub.h"

// GLOBAL: GAME 0x647900
C_cutscene cutscene;

// GLOBAL: GAME 0x647a58
int unknown_647a58;

// GLOBAL: GAME 0x647a4c
std::vector<S_cutscene_placement> *unknown_647a4c;

// The name is ours: the vtable's vfunc_51 sets it
// GLOBAL: GAME 0x647a54
int unknown_647a54;

#pragma auto_inline(off)

// STUB: GAME 0x5987f0
void *C_cutscene::vfunc_5(int a, int b, int c, int type)
{
    StubLog("C_cutscene::vfunc_5");
    return NULL;
}

// STUB: GAME 0x598970
void C_cutscene::vfunc_6(void *p)
{
    StubLog("C_cutscene::vfunc_6");
}

#pragma auto_inline(on)

// Each case walks the second list, then the first one
// FUNCTION: GAME 0x5989a0
void C_cutscene::SetState(int new_state)
{
    uint i;
    prev_state = state;
    state = new_state;
    if (prev_state == state)
        return;
    switch (state)
    {
    case CUTSCENE_STATE_0:
        for (i = 0; i < actors_2.size(); i++)
        {
            actors_2[i]->unknown_4 &= ~ACTF_UNKNOWN_10;
            actors_2[i]->vfunc_6(false);
            actors_2[i]->vfunc_7(false);
        }
        for (i = 0; i < actors.size(); i++)
        {
            actors[i]->unknown_4 &= ~ACTF_UNKNOWN_10;
            actors[i]->vfunc_7(false);
        }
        unknown_128 = false;
        break;
    case CUTSCENE_STATE_1:
        for (i = 0; i < actors_2.size(); i++)
        {
            actors_2[i]->unknown_4 |= ACTF_UNKNOWN_10;
            actors_2[i]->vfunc_6(false);
            actors_2[i]->vfunc_7(true);
        }
        for (i = 0; i < actors.size(); i++)
        {
            actors[i]->unknown_4 |= ACTF_UNKNOWN_10;
            actors[i]->vfunc_7(true);
        }
        break;
    case CUTSCENE_STATE_2:
        for (i = 0; i < actors_2.size(); i++)
        {
            actors_2[i]->unknown_4 &= ~ACTF_UNKNOWN_10;
            actors_2[i]->vfunc_7(false);
            actors_2[i]->vfunc_6(true);
        }
        for (i = 0; i < actors.size(); i++)
        {
            actors[i]->unknown_4 |= ACTF_UNKNOWN_10;
            actors[i]->vfunc_7(true);
        }
        break;
    }
}

// FUNCTION: GAME 0x598b70
int C_cutscene::GetState()
{
    return state;
}

#pragma auto_inline(off)

// STUB: GAME 0x598b80
void C_cutscene::vfunc_2(int unknown)
{
    StubLog("C_cutscene::vfunc_2");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x598d90
int C_cutscene::GetUnknown11c()
{
    return unknown_11c;
}

// FUNCTION: GAME 0x598da0
int C_cutscene::GetUnknown120()
{
    return unknown_120;
}

#pragma auto_inline(off)

// STUB: GAME 0x598db0
bool C_cutscene::vfunc_8(C_actor *actor, int a, int b, int c)
{
    StubLog("C_cutscene::vfunc_8");
    return false;
}

// STUB: GAME 0x598fb0
void C_cutscene::vfunc_7(int unknown)
{
    StubLog("C_cutscene::vfunc_7");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x599010
bool C_cutscene::RemoveActor(C_actor *actor, bool unknown)
{
    unknown_90.RemoveActor(actor, unknown);
    uint i;
    for (i = 0; i < actors.size(); i++)
    {
        if (actor == actors[i])
        {
            actor->unknown_4 &= ~ACTF_UNKNOWN_10;
            actors.erase(actors.begin() + i);
            return true;
        }
    }
    for (i = 0; i < actors_2.size(); i++)
    {
        if (actor == actors_2[i])
        {
            actors_2.erase(actors_2.begin() + i);
            return true;
        }
    }
    return false;
}

// FUNCTION: GAME 0x5990e0
void C_cutscene::RemoveActors()
{
    uint i;
    if (unknown_ec == 1)
    {
        for (i = 0; i < actors.size(); i++)
            RemoveActor(actors[i], true);
        for (i = 0; i < actors_2.size(); i++)
            RemoveActor(actors_2[i], true);
    }
    else
    {
        for (i = 0; i < actors.size(); i++)
        {
            delete actors[i]->unknown_8;
            actors[i]->unknown_4 &= ~ACTF_UNKNOWN_10;
            actors[i]->unknown_8 = NULL;
        }
        for (i = 0; i < actors_2.size(); i++)
        {
            delete actors_2[i]->unknown_8;
            actors_2[i]->unknown_4 &= ~ACTF_UNKNOWN_10;
            actors_2[i]->unknown_8 = NULL;
        }
        unknown_90.Clear();
    }
}

#pragma auto_inline(off)

// STUB: GAME 0x5991e0
bool C_cutscene::HasActor(C_actor *actor)
{
    StubLog("C_cutscene::HasActor");
    return false;
}

// STUB: GAME 0x599240
void C_cutscene::vfunc_13(C_actor *actor, int unknown)
{
    StubLog("C_cutscene::vfunc_13");
}

// STUB: GAME 0x599370
bool C_cutscene::HasActor2Only(C_actor *actor)
{
    StubLog("C_cutscene::HasActor2Only");
    return false;
}

// STUB: GAME 0x5993d0
void C_cutscene::vfunc_11()
{
    StubLog("C_cutscene::vfunc_11");
}

// STUB: GAME 0x59a240
void C_cutscene::vfunc_24(C_actor *actor)
{
    StubLog("C_cutscene::vfunc_24");
}

// STUB: GAME 0x59a3c0
void C_cutscene::RecLoadFull(const char *name)
{
    StubLog("C_cutscene::RecLoadFull %s", name);
}

// STUB: GAME 0x59ac30
void C_cutscene::vfunc_26(const char *name)
{
    StubLog("C_cutscene::vfunc_26 %s", name);
}

// STUB: GAME 0x59b210
void C_cutscene::vfunc_40()
{
    StubLog("C_cutscene::vfunc_40");
}

// STUB: GAME 0x59b2a0
void C_cutscene::vfunc_41(int unknown)
{
    StubLog("C_cutscene::vfunc_41");
}

// STUB: GAME 0x59b430
void C_cutscene::vfunc_42(int unknown)
{
    StubLog("C_cutscene::vfunc_42");
}

// STUB: GAME 0x59b570
void C_cutscene::vfunc_43(const char *name)
{
    StubLog("C_cutscene::vfunc_43 %s", name);
}

// STUB: GAME 0x59b5b0
void C_cutscene::vfunc_44(const char *name)
{
    StubLog("C_cutscene::vfunc_44 %s", name);
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x59b5f0
C_cutscene_unknown_80::C_cutscene_unknown_80()
{
    unknown_c = 0x100;
    unknown_4 = 0;
    unknown_8 = NULL;
}

#pragma auto_inline(off)

// STUB: GAME 0x59b7c0
const char *C_cutscene_unknown_80::GetName(int id)
{
    StubLog("C_cutscene_unknown_80::GetName %d", id);
    return NULL;
}

// STUB: GAME 0x59b800
int C_cutscene_unknown_80::GetValue(int id)
{
    StubLog("C_cutscene_unknown_80::GetValue %d", id);
    return 0;
}

// STUB: GAME 0x59b840
void C_cutscene_unknown_80::SetValue(int id, int value)
{
    StubLog("C_cutscene_unknown_80::SetValue %d", id);
}

// STUB: GAME 0x59b8c0
void C_cutscene_unknown_80::LoadBuffer(void *buf)
{
    StubLog("C_cutscene_unknown_80::LoadBuffer");
}

// STUB: GAME 0x59b940
void C_cutscene_unknown_80::SaveBuffer(void *buf)
{
    StubLog("C_cutscene_unknown_80::SaveBuffer");
}

// STUB: GAME 0x59b9a0
void C_cutscene_unknown_80::LoadFile(FILE *file)
{
    StubLog("C_cutscene_unknown_80::LoadFile");
}

// STUB: GAME 0x59ba40
void C_cutscene_unknown_80::SaveFile(FILE *file)
{
    StubLog("C_cutscene_unknown_80::SaveFile");
}

// STUB: GAME 0x59bab0
void C_cutscene_unknown_80::LoadDta(int handle)
{
    StubLog("C_cutscene_unknown_80::LoadDta");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x59bb50
void C_cutscene_unknown_80::Clear()
{
    if (unknown_8)
        delete[] unknown_8;
    unknown_8 = NULL;
    unknown_4 = 0;
}

#pragma auto_inline(off)

// STUB: GAME 0x59bb70
int C_cutscene_unknown_80::GetSaveSize()
{
    StubLog("C_cutscene_unknown_80::GetSaveSize");
    return 0;
}

// STUB: GAME 0x59bb90
void C_cutscene::vfunc_15(const char *name, int unknown)
{
    StubLog("C_cutscene::vfunc_15 %s", name);
}

// STUB: GAME 0x59bc20
S_cutscene_anim *C_cutscene::vfunc_16(const char *name, bool tck)
{
    StubLog("C_cutscene::vfunc_16 %s", name);
    return NULL;
}

// STUB: GAME 0x59bd80
S_cutscene_anim *C_cutscene::GetAnimByIndex(int index)
{
    StubLog("C_cutscene::GetAnimByIndex %d", index);
    return NULL;
}

// STUB: GAME 0x59bdb0
S_cutscene_anim *C_cutscene::GetAnim(const char *name)
{
    StubLog("C_cutscene::GetAnim %s", name);
    return NULL;
}

// STUB: GAME 0x59bdf0
void C_cutscene::vfunc_20(const char *name, int unknown)
{
    StubLog("C_cutscene::vfunc_20 %s", name);
}

// STUB: GAME 0x59c170
void C_cutscene::vfunc_21(int unknown)
{
    StubLog("C_cutscene::vfunc_21");
}

// STUB: GAME 0x59c2f0
void C_cutscene::vfunc_22()
{
    StubLog("C_cutscene::vfunc_22");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x59c4e0
void C_cutscene::ReleaseUnknown34()
{
    for (S_cutscene_anim **p = unknown_34.begin(); p < unknown_34.end(); p++)
    {
        if (*p)
        {
            if ((*p)->anim_set)
                (*p)->anim_set->Release();
            delete *p;
        }
    }
    unknown_34.clear();
}

#pragma auto_inline(off)

// STUB: GAME 0x59c580
int C_cutscene::vfunc_34(const char *name)
{
    StubLog("C_cutscene::vfunc_34 %s", name);
    return -1;
}

#pragma auto_inline(on)

// The two have the same code: the original's slots point to one copy
// FUNCTION: GAME 0x59c5c0 FOLDED
const char *C_cutscene::vfunc_19(int id)
{
    return unknown_80.GetName(id);
}

// FUNCTION: GAME 0x59c5c0 FOLDED
const char *C_cutscene::vfunc_35(int id)
{
    return unknown_80.GetName(id);
}

// FUNCTION: GAME 0x59c5d0
int C_cutscene::vfunc_36(int id)
{
    return unknown_80.GetValue(id);
}

// FUNCTION: GAME 0x59c5e0
void C_cutscene::vfunc_37(int id, int value)
{
    unknown_80.SetValue(id, value);
}

// FUNCTION: GAME 0x59c5f0
void C_cutscene::vfunc_23()
{
    unknown_68[2] = 0;
}

// FUNCTION: GAME 0x59c600
void C_cutscene::SetUnknown129(bool on)
{
    unknown_129 = on;
}

// FUNCTION: GAME 0x59c610
bool C_cutscene::GetUnknown129()
{
    return unknown_129;
}

// FUNCTION: GAME 0x59c620
void C_cutscene::RecClear()
{
    unknown_90.RecClear();
}

// FUNCTION: GAME 0x59c630
void C_cutscene::RecAddActor(const char *name, C_actor *actor)
{
    unknown_90.RecAddActor(name, actor);
}

// FUNCTION: GAME 0x59c640
void C_cutscene::RecLoad(const char *name)
{
    unknown_90.RecLoad(name);
}

// FUNCTION: GAME 0x59c650
void C_cutscene::vfunc_32(int unknown)
{
    unknown_90.Unknown59df40(unknown);
}

// FUNCTION: GAME 0x59c660
void C_cutscene::vfunc_33(C_actor *actor, int unknown)
{
    unknown_90.Unknown59dfb0(actor, unknown);
    vfunc_24(actor);
}

// FUNCTION: GAME 0x59c690
void C_cutscene::vfunc_38()
{
    unknown_90.Unknown59d3c0();
}

// The actors' placements back where they still exist: a human's or an entity's model gets its pose back, a door its
// position and rotation (SetRot with a normalization, written out as C_actor::GameDone's). The accessors after the
// two clears (GetType, GetGame, SetFlags) are the inline calls still to come that leave both clears' _Destroy out
// of line, as the original calls it.
// FUNCTION: GAME 0x59c6a0
void C_cutscene::Close()
{
    if (act_mission->unknown_78 < 2)
        act_mission->unknown_78 = 2;
    ReleaseUnknown34();
    unknown_80.Clear();
    SetState(CUTSCENE_STATE_0);
    SetState(CUTSCENE_STOPPED);
    unknown_90.Clear();
    RemoveActors();
    unknown_128 = true;
    if (unknown_f0)
        delete[] unknown_f0;
    if (unknown_f4)
        delete[] unknown_f4;
    if (unknown_f8)
        delete[] unknown_f8;
    unknown_f0 = NULL;
    unknown_f4 = NULL;
    unknown_f8 = NULL;
    if (unknown_74.items)
        delete[] unknown_74.items;
    if (unknown_b4.items)
        delete[] unknown_b4.items;
    if (unknown_d8.items)
        delete[] unknown_d8.items;
    memset(unknown_44, 0, sizeof(unknown_44));
    memset(unknown_5c, 0, sizeof(unknown_5c));
    memset(unknown_68, 0, sizeof(unknown_68));
    memset(&unknown_74, 0, sizeof(unknown_74));
    memset(&unknown_b4, 0, sizeof(unknown_b4));
    memset(unknown_c0, 0, sizeof(unknown_c0));
    memset(unknown_cc, 0, sizeof(unknown_cc));
    memset(&unknown_d8, 0, sizeof(unknown_d8));
    unknown_647a58 = 0;
    mafiaIndicators.SetUnknownTexture(NULL, false, false);
    mafiaIndicators.SetUnknown4484(0.0f, 0);
    actors.clear();
    actors_2.clear();
    if (unknown_647a4c)
    {
        for (S_cutscene_placement *p = unknown_647a4c->begin(); p < unknown_647a4c->end(); p++)
        {
            if (C_actor::IsGlobalActor(p->actor))
            {
                switch (p->actor->GetType())
                {
                case ACTOR_HUMAN:
                case ACTOR_ENTITY:
                    ((I3D_model *)p->frame)->SetPoseBlend(1.0f);
                    break;
                case ACTOR_DOOR:
                    {
                        p->frame->SetPos(p->pos);
                        I3D_frame *frm = p->frame;
                        frm->m_Rot = p->rot;
                        frm->m_Rot.Normalize();
                        frm->m_FrmFlags = (frm->m_FrmFlags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID |
                            FRMFLAGS_MAT_VALID | FRMFLAGS_ROT_VALID)) | FRMFLAGS_UPDATE | FRMFLAGS_QUAT_VALID;
                        p->frame->Update();
                    }
                    break;
                }
            }
        }
        delete unknown_647a4c;
        unknown_647a4c = NULL;
    }
    DabingReset();
    act_mission->GetGame()->EntityBlock(false, true);
    SetWBuffer(true);
    mafiaIndicators.ParheliaSetCenter(false);
    mafiaIndicators.SubtitlesClear();
    mafiaIndicators.SetFlags(mafiaIndicators.unknown_40a4 & ~INDF_UNKNOWN_800);
}

// FUNCTION: GAME 0x59d1f0
void C_cutscene_unknown_90::RemoveActor(C_actor *actor, bool unknown)
{
    for (uint i = 0; i < actors.size(); i++)
    {
        if (actors[i].actor == actor)
        {
            if (unknown)
            {
                if (!actors[i].unknown_0)
                    actors[i].records->Reset();
                actors[i].records = NULL;
                actors[i].actor->unknown_8 = NULL;
            }
            actors[i].actor = NULL;
            actors.erase(actors.begin() + i);
            return;
        }
    }
}

// FUNCTION: GAME 0x59d2c0
void C_cutscene_unknown_90::Clear()
{
    for (uint i = 0; i < actors.size(); i++)
    {
        if (!actors[i].unknown_0)
            actors[i].records->Reset();
        if (actors[i].actor)
            actors[i].actor->unknown_8->unknown_18 = NULL;
    }
    actors.clear();
    ReleaseUnknown0();
}

// FUNCTION: GAME 0x59d350
void C_cutscene_unknown_90::ReleaseUnknown0()
{
    int count = 0;
    if (unknown_0)
    {
        for (uint i = 0; i < actors.size(); i++)
        {
            if (actors[i].unknown_0 == 1)
                count++;
        }
        if (!count)
        {
            delete unknown_0;
            unknown_0 = NULL;
        }
    }
}

#pragma auto_inline(off)

// STUB: GAME 0x59d3c0
void C_cutscene_unknown_90::Unknown59d3c0()
{
    StubLog("C_cutscene_unknown_90::Unknown59d3c0");
}

// STUB: GAME 0x59d4d0
void C_cutscene_unknown_90::RecClear()
{
    StubLog("C_cutscene_unknown_90::RecClear");
}

// STUB: GAME 0x59d500
void C_cutscene_unknown_90::RecAddActor(const char *name, C_actor *actor)
{
    StubLog("C_cutscene_unknown_90::RecAddActor %s", name);
}

// STUB: GAME 0x59dcd0
void C_cutscene_unknown_90::RecLoad(const char *name)
{
    StubLog("C_cutscene_unknown_90::RecLoad %s", name);
}

// STUB: GAME 0x59df40
void C_cutscene_unknown_90::Unknown59df40(int unknown)
{
    StubLog("C_cutscene_unknown_90::Unknown59df40");
}

// STUB: GAME 0x59dfb0
void C_cutscene_unknown_90::Unknown59dfb0(C_actor *actor, int unknown)
{
    StubLog("C_cutscene_unknown_90::Unknown59dfb0");
}

// STUB: GAME 0x59e020
void C_cutscene::vfunc_45(int unknown)
{
    StubLog("C_cutscene::vfunc_45");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x59e100
int C_cutscene::vfunc_46()
{
    return 5;
}

#pragma auto_inline(off)

// STUB: GAME 0x59e110
void C_cutscene::vfunc_47(int a, int b, int c, int d)
{
    StubLog("C_cutscene::vfunc_47");
}

// STUB: GAME 0x59e140
void C_cutscene::vfunc_48(int a, int b, int c)
{
    StubLog("C_cutscene::vfunc_48");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x59e170
void C_cutscene::vfunc_49(uint time)
{
    DabingTick(time);
}

#pragma auto_inline(off)

// STUB: GAME 0x59e180
void C_cutscene::vfunc_50(int unknown)
{
    StubLog("C_cutscene::vfunc_50");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x59e1e0
void C_cutscene::vfunc_51(int unknown)
{
    unknown_647a54 = unknown;
}

#pragma auto_inline(off)

// STUB: GAME 0x59e1f0
void C_cutscene::vfunc_52(int unknown)
{
    StubLog("C_cutscene::vfunc_52");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x59e2d0
bool C_cutscene::IsPlaying()
{
    if (unknown_647a58)
        return true;
    if (state == CUTSCENE_STATE_0)
        return false;
    return state != CUTSCENE_STOPPED ? true : false;
}

// The vtable's last function: the linker's folded return of 0 of the out-of-line kind (0x4aa430, by name in
// actors/C_actor.h)
int C_cutscene::vfunc_53()
{
    return 0;
}

#pragma auto_inline(off)

// STUB: GAME 0x59e7e0
C_seagull::C_seagull()
{
    StubLog("C_seagull::C_seagull");
}

// The records' file is the one at 0x5c25b0-0x5c283c, which has no source yet
// STUB: GAME 0x5c25e0
void C_cutscene_records::Reset()
{
    StubLog("C_cutscene_records::Reset");
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x59e850
C_seagull::~C_seagull()
{
    StubLog("C_seagull::~C_seagull");
}

// The linker's folded empty function of the out-of-line kind (0x46ca70)
void C_seagull::vfunc_38(int a, int b)
{
}

// The original's is C_pumper's, folded: it logs that name, so ours folds too
// STUB: GAME 0x47c760 FOLDED
bool C_seagull::Init(I3D_frame *frm)
{
    StubLog("C_pumper::Init");
    return C_actor::Init(frm);
}

// The linker's folded copy (0x4aa490)
bool C_seagull::SaveGameSave(void **buf)
{
    return true;
}

// The linker's folded copy (0x4aa490)
bool C_seagull::SaveGameLoad(void **buf)
{
    return true;
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_seagull::Update(uint time)
{
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_seagull::NetDirect(uint time)
{
}

// STUB: GAME 0x59e890
void C_seagull::GameInit()
{
    StubLog("C_seagull::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x59f6a0
void C_seagull::GameDone()
{
    StubLog("C_seagull::GameDone");
    C_actor::GameDone();
}

// STUB: GAME 0x59f710
bool C_seagull::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    I3D_frame *frm)
{
    StubLog("C_seagull::Collision");
    return C_actor::Collision(act, v1, v2, v3, f, frm);
}

// STUB: GAME 0x59f770
bool C_seagull::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
    uint u, I3D_frame *frm)
{
    StubLog("C_seagull::Hit");
    return C_actor::Hit(type, v1, v2, v3, f, act, u, frm);
}

// STUB: GAME 0x59f920
void C_seagull::AI(uint time)
{
    StubLog("C_seagull::AI");
    C_actor::AI(time);
}

#pragma auto_inline(on)
