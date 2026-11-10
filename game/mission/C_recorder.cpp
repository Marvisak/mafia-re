// The in-engine cutscenes (PC only, the file's name is ours, after its class's; the PC's file is the one at
// 0x5983e0-0x5a075c, docs/game-files.md): the keys' interpolation (0x598410, 0x5984e0), the object's initializer
// (0x59cc20) and constructor (0x59cc40) mid-file, Close (0x59c6a0), Tick (0x599610), and the seagulls after them (their
// constructor 0x59e7e0).
// Written so far: SetState, which C_game::Done calls, the vtable's getters, RemoveActor, which C_actor's destructor
// calls, with the recording's, Close, which C_mission::Close calls, with what it calls of the cutscene (RemoveActors,
// ReleaseAnims), of its animations' names (Clear) and of the recording (Clear, ReleaseUnknown0), and IsPlaying; the
// vtable's getters, setters and the functions that pass on to the animations' names and the recording (the
// recordings', which the scripts call, among them), the names' constructor, and the keys' ease, which C_actor's
// cutscene functions call. Stubs: the vtable's other functions, the two objects' functions those pass on to, and the
// seagulls' constructor, which C_mission::CreateActor calls.

#include "mission/C_recorder.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "ui/g_dabing.h"
#include "ui/g_indicators.h"

#include <string.h>

#include "stub.h"

// GLOBAL: GAME 0x647900
C_recorder cutscene;

// GLOBAL: GAME 0x647a58
int cutscene_play_ticks;

// GLOBAL: GAME 0x647a4c
std::vector<S_cutscene_placement> *saved_placements;

// The cutscene's time at which its show starts (the name is ours): SetShowStart sets it, OpenShow clears it
// GLOBAL: GAME 0x647a54
int cutscene_show_start;

// The time between two keys eased: accelerated over a, at constant speed, then slowed over b (a and b scaled down to
// a sum of 1); the time itself where they add up to nothing
// FUNCTION: GAME 0x598410
float Ease(float u, float a, float b)
{
    float s = a + b;
    if (s < .001)
        return u;
    if (s > 1.0f)
    {
        a = a / s;
        b = b / s;
    }
    float k = 1.0f / (2.0f - a - b);
    if (u < a)
        return k / a * u * u;
    if (u < 1.0f - b)
        return k * (2.0f * u - a);
    u = 1.0f - u;
    float r = 1.0f - k / b * u * u;
    return r;
}

// STUB: GAME 0x5987f0
void *C_recorder::vfunc_5(int a, int b, int c, int type)
{
    StubLog("C_recorder::vfunc_5");
    return NULL;
}

// STUB: GAME 0x598970
void C_recorder::vfunc_6(void *p)
{
    StubLog("C_recorder::vfunc_6");
}

// Each case walks the second list, then the first one
// FUNCTION: GAME 0x5989a0
void C_recorder::SetState(int new_state)
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
            actors_2[i]->act_flags &= ~ACTF_CUTSCENE;
            actors_2[i]->SetCutsceneRecord(false);
            actors_2[i]->SetCutscenePlay(false);
        }
        for (i = 0; i < actors.size(); i++)
        {
            actors[i]->act_flags &= ~ACTF_CUTSCENE;
            actors[i]->SetCutscenePlay(false);
        }
        unknown_128 = false;
        break;
    case CUTSCENE_STATE_1:
        for (i = 0; i < actors_2.size(); i++)
        {
            actors_2[i]->act_flags |= ACTF_CUTSCENE;
            actors_2[i]->SetCutsceneRecord(false);
            actors_2[i]->SetCutscenePlay(true);
        }
        for (i = 0; i < actors.size(); i++)
        {
            actors[i]->act_flags |= ACTF_CUTSCENE;
            actors[i]->SetCutscenePlay(true);
        }
        break;
    case CUTSCENE_STATE_2:
        for (i = 0; i < actors_2.size(); i++)
        {
            actors_2[i]->act_flags &= ~ACTF_CUTSCENE;
            actors_2[i]->SetCutscenePlay(false);
            actors_2[i]->SetCutsceneRecord(true);
        }
        for (i = 0; i < actors.size(); i++)
        {
            actors[i]->act_flags |= ACTF_CUTSCENE;
            actors[i]->SetCutscenePlay(true);
        }
        break;
    }
}

// FUNCTION: GAME 0x598b70
int C_recorder::GetState()
{
    return state;
}

// STUB: GAME 0x598b80
void C_recorder::SetTime(int unknown)
{
    StubLog("C_recorder::SetTime");
}

// FUNCTION: GAME 0x598d90
int C_recorder::GetCurTime()
{
    return cur_time;
}

// FUNCTION: GAME 0x598da0
int C_recorder::GetScaledTime()
{
    return scaled_time;
}

// STUB: GAME 0x598db0
bool C_recorder::AddActor(C_actor *actor, int a, int b, int c)
{
    StubLog("C_recorder::AddActor");
    return false;
}

// STUB: GAME 0x598fb0
void C_recorder::vfunc_7(int unknown)
{
    StubLog("C_recorder::vfunc_7");
}

// FUNCTION: GAME 0x599010
bool C_recorder::RemoveActor(C_actor *actor, bool unknown)
{
    recording.RemoveActor(actor, unknown);
    uint i;
    for (i = 0; i < actors.size(); i++)
    {
        if (actor == actors[i])
        {
            actor->act_flags &= ~ACTF_CUTSCENE;
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
void C_recorder::RemoveActors()
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
            delete actors[i]->records;
            actors[i]->act_flags &= ~ACTF_CUTSCENE;
            actors[i]->records = NULL;
        }
        for (i = 0; i < actors_2.size(); i++)
        {
            delete actors_2[i]->records;
            actors_2[i]->act_flags &= ~ACTF_CUTSCENE;
            actors_2[i]->records = NULL;
        }
        recording.Clear();
    }
}

// STUB: GAME 0x5991e0
bool C_recorder::HasActor(C_actor *actor)
{
    StubLog("C_recorder::HasActor");
    return false;
}

// STUB: GAME 0x599240
void C_recorder::vfunc_13(C_actor *actor, int unknown)
{
    StubLog("C_recorder::vfunc_13");
}

// STUB: GAME 0x599370
bool C_recorder::HasActor2Only(C_actor *actor)
{
    StubLog("C_recorder::HasActor2Only");
    return false;
}

// STUB: GAME 0x5993d0
void C_recorder::vfunc_11()
{
    StubLog("C_recorder::vfunc_11");
}

// STUB: GAME 0x599610
void C_recorder::Tick(uint time)
{
    StubLog("C_recorder::Tick %u", time);
}

// STUB: GAME 0x59a240
void C_recorder::vfunc_24(C_actor *actor)
{
    StubLog("C_recorder::vfunc_24");
}

// STUB: GAME 0x59a3c0
void C_recorder::RecLoadFull(const char *name)
{
    StubLog("C_recorder::RecLoadFull %s", name);
}

// STUB: GAME 0x59ac30
void C_recorder::vfunc_26(const char *name)
{
    StubLog("C_recorder::vfunc_26 %s", name);
}

// STUB: GAME 0x59b130
void C_recorder::ScaleTime(uint &time)
{
    StubLog("C_recorder::ScaleTime %u", time);
}

// STUB: GAME 0x59b210
void C_recorder::vfunc_40()
{
    StubLog("C_recorder::vfunc_40");
}

// STUB: GAME 0x59b2a0
void C_recorder::vfunc_41(int unknown)
{
    StubLog("C_recorder::vfunc_41");
}

// STUB: GAME 0x59b430
void C_recorder::vfunc_42(int unknown)
{
    StubLog("C_recorder::vfunc_42");
}

// STUB: GAME 0x59b570
void C_recorder::vfunc_43(const char *name)
{
    StubLog("C_recorder::vfunc_43 %s", name);
}

// STUB: GAME 0x59b5b0
void C_recorder::vfunc_44(const char *name)
{
    StubLog("C_recorder::vfunc_44 %s", name);
}

// FUNCTION: GAME 0x59b5f0
C_cutscene_anim_names::C_cutscene_anim_names()
{
    unknown_c = 0x100;
    num_items = 0;
    items = NULL;
}

// STUB: GAME 0x59b7c0
const char *C_cutscene_anim_names::GetName(int id)
{
    StubLog("C_cutscene_anim_names::GetName %d", id);
    return NULL;
}

// STUB: GAME 0x59b800
int C_cutscene_anim_names::GetValue(int id)
{
    StubLog("C_cutscene_anim_names::GetValue %d", id);
    return 0;
}

// STUB: GAME 0x59b840
void C_cutscene_anim_names::SetValue(int id, int value)
{
    StubLog("C_cutscene_anim_names::SetValue %d", id);
}

// STUB: GAME 0x59b8c0
void C_cutscene_anim_names::LoadBuffer(void *buf)
{
    StubLog("C_cutscene_anim_names::LoadBuffer");
}

// STUB: GAME 0x59b940
void C_cutscene_anim_names::SaveBuffer(void *buf)
{
    StubLog("C_cutscene_anim_names::SaveBuffer");
}

// STUB: GAME 0x59b9a0
void C_cutscene_anim_names::LoadFile(FILE *file)
{
    StubLog("C_cutscene_anim_names::LoadFile");
}

// STUB: GAME 0x59ba40
void C_cutscene_anim_names::SaveFile(FILE *file)
{
    StubLog("C_cutscene_anim_names::SaveFile");
}

// STUB: GAME 0x59bab0
void C_cutscene_anim_names::LoadDta(int handle)
{
    StubLog("C_cutscene_anim_names::LoadDta");
}

// FUNCTION: GAME 0x59bb50
void C_cutscene_anim_names::Clear()
{
    if (items)
        delete[] items;
    items = NULL;
    num_items = 0;
}

// STUB: GAME 0x59bb70
int C_cutscene_anim_names::GetSaveSize()
{
    StubLog("C_cutscene_anim_names::GetSaveSize");
    return 0;
}

// STUB: GAME 0x59bb90
void C_recorder::AddAnim(const char *name, int unknown)
{
    StubLog("C_recorder::AddAnim %s", name);
}

// STUB: GAME 0x59bc20
S_cutscene_anim *C_recorder::CreateAnim(const char *name, bool tck)
{
    StubLog("C_recorder::CreateAnim %s", name);
    return NULL;
}

// STUB: GAME 0x59bd80
S_cutscene_anim *C_recorder::GetAnimByIndex(int index)
{
    StubLog("C_recorder::GetAnimByIndex %d", index);
    return NULL;
}

// STUB: GAME 0x59bdb0
S_cutscene_anim *C_recorder::GetAnim(const char *name)
{
    StubLog("C_recorder::GetAnim %s", name);
    return NULL;
}

// STUB: GAME 0x59bdf0
void C_recorder::LoadTCK(const char *name, int unknown)
{
    StubLog("C_recorder::LoadTCK %s", name);
}

// STUB: GAME 0x59c170
void C_recorder::vfunc_21(int unknown)
{
    StubLog("C_recorder::vfunc_21");
}

// STUB: GAME 0x59c2f0
void C_recorder::vfunc_22()
{
    StubLog("C_recorder::vfunc_22");
}

// FUNCTION: GAME 0x59c4e0
void C_recorder::ReleaseAnims()
{
    for (S_cutscene_anim **p = anims.begin(); p < anims.end(); p++)
    {
        if (*p)
        {
            if ((*p)->anim_set)
                (*p)->anim_set->Release();
            delete *p;
        }
    }
    anims.clear();
}

// STUB: GAME 0x59c580
int C_recorder::AddAnimName(const char *name)
{
    StubLog("C_recorder::AddAnimName %s", name);
    return -1;
}

// The two have the same code: the original's slots point to one copy
// FUNCTION: GAME 0x59c5c0 FOLDED
const char *C_recorder::vfunc_19(int id)
{
    return anim_names.GetName(id);
}

// FUNCTION: GAME 0x59c5c0 FOLDED
const char *C_recorder::GetAnimName(int id)
{
    return anim_names.GetName(id);
}

// FUNCTION: GAME 0x59c5d0
int C_recorder::GetAnimValue(int id)
{
    return anim_names.GetValue(id);
}

// FUNCTION: GAME 0x59c5e0
void C_recorder::SetAnimValue(int id, int value)
{
    anim_names.SetValue(id, value);
}

// FUNCTION: GAME 0x59c5f0
void C_recorder::ResetSpeedKeys()
{
    speed_keys[2] = 0;
}

// FUNCTION: GAME 0x59c600
void C_recorder::SetSpeedKeysOn(bool on)
{
    speed_keys_on = on;
}

// FUNCTION: GAME 0x59c610
bool C_recorder::GetSpeedKeysOn()
{
    return speed_keys_on;
}

// FUNCTION: GAME 0x59c620
void C_recorder::RecClear()
{
    recording.RecClear();
}

// FUNCTION: GAME 0x59c630
void C_recorder::RecAddActor(const char *name, C_actor *actor)
{
    recording.RecAddActor(name, actor);
}

// FUNCTION: GAME 0x59c640
void C_recorder::RecLoad(const char *name)
{
    recording.RecLoad(name);
}

// FUNCTION: GAME 0x59c650
void C_recorder::DetachActor(int unknown)
{
    recording.DetachActor(unknown);
}

// FUNCTION: GAME 0x59c660
void C_recorder::AttachActor(C_actor *actor, int unknown)
{
    recording.AttachActor(actor, unknown);
    vfunc_24(actor);
}

// FUNCTION: GAME 0x59c690
void C_recorder::vfunc_38()
{
    recording.Unknown59d3c0();
}

// The actors' placements back where they still exist: a human's or an entity's model gets its pose back, a door its
// position and rotation (SetRot with a normalization, written out as C_actor::GameDone's). The accessors after the
// two clears (GetType, GetGame, SetFlags) are the inline calls still to come that leave both clears' _Destroy out
// of line, as the original calls it.
// FUNCTION: GAME 0x59c6a0
void C_recorder::Close()
{
    if (act_mission->skip_frames < 2)
        act_mission->skip_frames = 2;
    ReleaseAnims();
    anim_names.Clear();
    SetState(CUTSCENE_STATE_0);
    SetState(CUTSCENE_STOPPED);
    recording.Clear();
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
    if (dabing.items)
        delete[] dabing.items;
    if (subtitles.items)
        delete[] subtitles.items;
    if (morphs.items)
        delete[] morphs.items;
    memset(unknown_44, 0, sizeof(unknown_44));
    memset(unknown_5c, 0, sizeof(unknown_5c));
    memset(speed_keys, 0, sizeof(speed_keys));
    memset(&dabing, 0, sizeof(dabing));
    memset(&subtitles, 0, sizeof(subtitles));
    memset(&programs, 0, sizeof(programs));
    memset(unknown_cc, 0, sizeof(unknown_cc));
    memset(&morphs, 0, sizeof(morphs));
    cutscene_play_ticks = 0;
    mafiaIndicators.SetOverlayTexture(NULL, false, false);
    mafiaIndicators.SetOverlayFade(0.0f, 0);
    actors.clear();
    actors_2.clear();
    if (saved_placements)
    {
        for (S_cutscene_placement *p = saved_placements->begin(); p < saved_placements->end(); p++)
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
        delete saved_placements;
        saved_placements = NULL;
    }
    DabingReset();
    act_mission->GetGame()->EntityBlock(false, true);
    SetWBuffer(true);
    mafiaIndicators.ParheliaSetCenter(false);
    mafiaIndicators.SubtitlesClear();
    mafiaIndicators.SetFlags(mafiaIndicators.show_flags & ~INDF_CUTSCENE);
}

// FUNCTION: GAME 0x59d1f0
void C_cutscene_recording::RemoveActor(C_actor *actor, bool unknown)
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
                actors[i].actor->records = NULL;
            }
            actors[i].actor = NULL;
            actors.erase(actors.begin() + i);
            return;
        }
    }
}

// FUNCTION: GAME 0x59d2c0
void C_cutscene_recording::Clear()
{
    for (uint i = 0; i < actors.size(); i++)
    {
        if (!actors[i].unknown_0)
            actors[i].records->Reset();
        if (actors[i].actor)
            actors[i].actor->records->first_block = NULL;
    }
    actors.clear();
    ReleaseUnknown0();
}

// FUNCTION: GAME 0x59d350
void C_cutscene_recording::ReleaseUnknown0()
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

// STUB: GAME 0x59d3c0
void C_cutscene_recording::Unknown59d3c0()
{
    StubLog("C_cutscene_recording::Unknown59d3c0");
}

// STUB: GAME 0x59d4d0
void C_cutscene_recording::RecClear()
{
    StubLog("C_cutscene_recording::RecClear");
}

// STUB: GAME 0x59d500
void C_cutscene_recording::RecAddActor(const char *name, C_actor *actor)
{
    StubLog("C_cutscene_recording::RecAddActor %s", name);
}

// STUB: GAME 0x59dcd0
void C_cutscene_recording::RecLoad(const char *name)
{
    StubLog("C_cutscene_recording::RecLoad %s", name);
}

// STUB: GAME 0x59df40
void C_cutscene_recording::DetachActor(int unknown)
{
    StubLog("C_cutscene_recording::DetachActor");
}

// STUB: GAME 0x59dfb0
void C_cutscene_recording::AttachActor(C_actor *actor, int unknown)
{
    StubLog("C_cutscene_recording::AttachActor");
}

// STUB: GAME 0x59e020
void C_recorder::vfunc_45(int unknown)
{
    StubLog("C_recorder::vfunc_45");
}

// FUNCTION: GAME 0x59e100
int C_recorder::vfunc_46()
{
    return 5;
}

// STUB: GAME 0x59e110
void C_recorder::vfunc_47(int a, int b, int c, int d)
{
    StubLog("C_recorder::vfunc_47");
}

// STUB: GAME 0x59e140
void C_recorder::vfunc_48(int a, int b, int c)
{
    StubLog("C_recorder::vfunc_48");
}

// FUNCTION: GAME 0x59e170
void C_recorder::vfunc_49(uint time)
{
    DabingTick(time);
}

// STUB: GAME 0x59e180
void C_recorder::OpenShow(int unknown)
{
    StubLog("C_recorder::OpenShow");
}

// FUNCTION: GAME 0x59e1e0
void C_recorder::SetShowStart(int unknown)
{
    cutscene_show_start = unknown;
}

// STUB: GAME 0x59e1f0
void C_recorder::TickShow(int unknown)
{
    StubLog("C_recorder::TickShow");
}

// FUNCTION: GAME 0x59e2a0
void C_recorder::Stop()
{
    cutscene_play_ticks = 0;
    SetState(CUTSCENE_STATE_0);
    if (act_mission->skip_frames < 4)
        act_mission->skip_frames = 4;
}

// FUNCTION: GAME 0x59e2d0
bool C_recorder::IsPlaying()
{
    if (cutscene_play_ticks)
        return true;
    if (state == CUTSCENE_STATE_0)
        return false;
    return state != CUTSCENE_STOPPED ? true : false;
}

// The vtable's last function: the linker's folded return of 0 of the out-of-line kind (0x4aa430, by name in
// actors/C_actor.h)
int C_recorder::vfunc_53()
{
    return 0;
}

// STUB: GAME 0x59e7e0
C_seagull::C_seagull()
{
    StubLog("C_seagull::C_seagull");
}

// The records' file is the one at 0x5c25b0-0x5c283c, which has no source yet
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
