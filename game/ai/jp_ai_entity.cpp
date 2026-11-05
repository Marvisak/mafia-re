// The AI's humans (PS2 jp_ai_entity.cpp, C_entity's; the PC's file is the one at 0x505400-0x53038c,
// docs/game-files.md), whose constructor (0x5071a0) comes after functions of its own. Not written yet: the
// constructor, which C_mission::CreateActor calls, the cutscene's block and wake, which C_game::EntityBlock calls,
// what the script's commands call (the senses', the logs' and the jobs' functions, ForceScript, WeaponChoose,
// AddUsingObject), what the AI's following and fights and the police call (Event, hasAction, LockSubstate,
// IsInDanger) and ResetState, which C_mission::FrmActBuff_GetActor calls, are stubs.

#include "ai/jp_ai_entity.h"

#include "stub.h"

// STUB: GAME 0x505d20
void ai_sensors::Init(void *owner)
{
    StubLog("ai_sensors::Init");
}

// STUB: GAME 0x506340
void ai_vis_logs::Init(void *owner)
{
    StubLog("ai_vis_logs::Init");
}

// STUB: GAME 0x5071a0
C_entity::C_entity()
{
    StubLog("C_entity::C_entity");
}

// STUB: GAME 0x523f80
void C_entity::ForceScript(bool unknown)
{
    StubLog("C_entity::ForceScript");
}

// STUB: GAME 0x5240a0
void C_entity::Event(C_actor *act, uint id, void *data)
{
    StubLog("C_entity::Event");
}

// STUB: GAME 0x524650
void ai_action_manager::Clear(bool unknown)
{
    StubLog("ai_action_manager::Clear");
}

// STUB: GAME 0x527600
ai_action *ai_action_manager::GetBaseJob()
{
    StubLog("ai_action_manager::GetBaseJob");
    return NULL;
}

// STUB: GAME 0x527a50
bool ai_action_manager::hasAction(uint type, ai_action **action, bool unknown)
{
    StubLog("ai_action_manager::hasAction");
    return false;
}

// STUB: GAME 0x528370
void C_entity::WeaponChoose(uint id)
{
    StubLog("C_entity::WeaponChoose");
}

// STUB: GAME 0x529100
void C_entity::AddUsingObject()
{
    StubLog("C_entity::AddUsingObject");
}

// STUB: GAME 0x5299d0
void C_entity::LockSubstate(uint unknown, uint unknown2, float unknown3, uint unknown4, bool unknown5)
{
    StubLog("C_entity::LockSubstate");
}

// STUB: GAME 0x52f850
bool C_entity::IsInDanger()
{
    StubLog("C_entity::IsInDanger");
    return false;
}

// STUB: GAME 0x52ff50
void C_entity::SedniNaPrdelASledujKino(bool unknown)
{
    StubLog("C_entity::SedniNaPrdelASledujKino");
}

// STUB: GAME 0x52ffe0
void C_entity::VzbudSeUzSouTitulky()
{
    StubLog("C_entity::VzbudSeUzSouTitulky");
}

// STUB: GAME 0x507310
C_entity::~C_entity()
{
    StubLog("C_entity::~C_entity");
}

// STUB: GAME 0x507470
bool C_entity::Init(I3D_frame *frm)
{
    StubLog("C_entity::Init");
    return C_human::Init(frm);
}

// STUB: GAME 0x507490
void C_entity::DuplicateFrom(C_actor *act)
{
    StubLog("C_entity::DuplicateFrom");
    C_human::DuplicateFrom(act);
}

// STUB: GAME 0x5074d0
void C_entity::GameInit()
{
    StubLog("C_entity::GameInit");
    C_human::GameInit();
}

// STUB: GAME 0x507c80
void C_entity::ResetState()
{
    StubLog("C_entity::ResetState");
}

// STUB: GAME 0x507d30
void C_entity::GameDone()
{
    StubLog("C_entity::GameDone");
    C_human::GameDone();
}

// STUB: GAME 0x507dd0
bool C_entity::SaveData(C_chunk *chunk)
{
    StubLog("C_entity::SaveData");
    return C_human::SaveData(chunk);
}

// STUB: GAME 0x507e20
bool C_entity::LoadData(C_chunk *chunk)
{
    StubLog("C_entity::LoadData");
    return C_human::LoadData(chunk);
}

// STUB: GAME 0x507ec0
bool C_entity::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    I3D_frame *frm)
{
    StubLog("C_entity::Collision");
    return C_human::Collision(act, v1, v2, v3, f, frm);
}

// STUB: GAME 0x507ed0
bool C_entity::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
    uint u, I3D_frame *frm)
{
    StubLog("C_entity::Hit");
    return C_human::Hit(type, v1, v2, v3, f, act, u, frm);
}

// STUB: GAME 0x508150
void C_entity::AI(uint time)
{
    StubLog("C_entity::AI");
    C_human::AI(time);
}

// STUB: GAME 0x51b020
std::vector<C_program::S_instruction> *C_entity::GetProgram()
{
    StubLog("C_entity::GetProgram");
    return C_human::GetProgram();
}

// STUB: GAME 0x51b2f0
void C_entity::Update(uint time)
{
    StubLog("C_entity::Update");
    C_human::Update(time);
}

// STUB: GAME 0x51b3f0
void C_entity::Death()
{
    StubLog("C_entity::Death");
    C_human::Death();
}

// STUB: GAME 0x51b470
bool C_entity::DecodeParams(char *params, S_instruction &ins)
{
    StubLog("C_entity::DecodeParams");
    return C_program::DecodeParams(params, ins);
}

// STUB: GAME 0x51cdd0
int C_entity::DecodeInstruction(char *line, int &len)
{
    StubLog("C_entity::DecodeInstruction");
    return C_program::DecodeInstruction(line, len);
}

// STUB: GAME 0x51f3e0
void C_entity::DecompileCommand(int i, char *s)
{
    StubLog("C_entity::DecompileCommand");
    C_program::DecompileCommand(i, s);
}

// STUB: GAME 0x51fda0
void C_entity::Process(uint time)
{
    StubLog("C_entity::Process");
    C_program::Process(time);
}

// STUB: GAME 0x524070
void C_entity::CallBack(E_cb_action action)
{
    StubLog("C_entity::CallBack");
    C_program::CallBack(action);
}

// STUB: GAME 0x52db20
int C_entity::SaveGameGetSize()
{
    StubLog("C_entity::SaveGameGetSize");
    return C_human::SaveGameGetSize();
}

// STUB: GAME 0x52db80
bool C_entity::SaveGameSave(void **buf)
{
    StubLog("C_entity::SaveGameSave");
    return C_human::SaveGameSave(buf);
}

// STUB: GAME 0x52dc40
bool C_entity::SaveGameLoad(void **buf)
{
    StubLog("C_entity::SaveGameLoad");
    return C_human::SaveGameLoad(buf);
}

// STUB: GAME 0x5080e0
void C_entity::cbHitInfo(int type, float f, C_actor *act)
{
    StubLog("C_entity::cbHitInfo");
}

// STUB: GAME 0x5289b0
void C_entity::InvalidateSelf()
{
    StubLog("C_entity::InvalidateSelf");
}

// STUB: GAME 0x528610
void C_entity::WeaponGetInfo()
{
    StubLog("C_entity::WeaponGetInfo");
}
