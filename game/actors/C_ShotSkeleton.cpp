// The bones the shots test against (PS2 C_ShotSkeleton, whose file sorts between g_menu.cpp and g_textdb.cpp on PS2;
// the file's name is ours: the PC's file is the one at 0x5d6f80-0x5d767c, docs/game-files.md). The class is in
// C_human.h, whose member it is. Only a stub so far: TestCollision, which C_game::TickShoot calls.

#include "actors/C_human.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x5d6fe0
int C_ShotSkeleton::TestCollision(const S_vector &from, const S_vector &dir, float dist2)
{
    StubLog("C_ShotSkeleton::TestCollision");
    return 0;
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x5d6f80
void C_ShotSkeleton::Init(I3D_frame *frm)
{
    StubLog("C_ShotSkeleton::Init");
}

#pragma auto_inline(on)
