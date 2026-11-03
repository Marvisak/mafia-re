// The collisions, the file at 0x5c2840-0x5c503c (docs/game-files.md); the name is a guess, PS2's g_colls.cpp. It starts
// with gifCollision's initializer, whose constructor's copy is the file's last function. Written so far: DynOptimizeTick,
// which C_game::Tick calls, DeleteCollsLevel and DynWipeOut, which GameDone calls, GameDone, TestLineV and TestSphereB
// (in the file at 0x5d5900 on the PC); the rest of what main, the game, the detector and the script's commands call is
// stubs.

#include "collision/g_colls.h"

#include "stub.h"

// GLOBAL: GAME 0x647f48
g_collision gifCollision;

// The level's collisions and the grid freed, the grid's lists emptied (PS2's frees the lists too, and with its bool
// set keeps a level that has its unknown_c)
// FUNCTION: GAME 0x5c28d0
void g_collision::DeleteCollsLevel()
{
    if (level)
    {
        if (level->unknown_2c)
            delete[] level->unknown_2c;
        delete level;
        level = NULL;
    }
    if (dyn_grid)
    {
        delete[] dyn_grid->cells;
        delete dyn_grid;
        dyn_grid = NULL;
        free_cells.clear();
        dyn_queue.clear();
    }
}

#pragma auto_inline(off)

// STUB: GAME 0x5c2b70
bool g_collision::LoadCollision(const char *name, bool)
{
    StubLog("g_collision::LoadCollision %s", name);
    return false;
}

// The cells to check from the last one: one left without objects is taken out of the grid onto the free ones (PS2's the
// same)
// FUNCTION: GAME 0x5c3610
void g_collision::DynOptimizeTick()
{
    while (dyn_queue.size())
    {
        tDynamicPolicko *p = dyn_queue.back();
        if (!p->objects.size())
        {
            dyn_grid->cells[p->x + p->y * dyn_grid->width] = NULL;
            free_cells.push_back(dyn_queue.back());
        }
        dyn_queue.pop_back();
    }
}

// STUB: GAME 0x5c38b0
void g_collision::DynErase(tDynamicCollObject *obj)
{
    StubLog("g_collision::DynErase");
}

// STUB: GAME 0x5c3ac0
void g_collision::DynUpdate(tDynamicCollObject *obj)
{
    StubLog("g_collision::DynUpdate");
}

// STUB: GAME 0x5c3d10
void g_collision::DynSetNextCollOwner(tDynamicCollObject *obj, void *callback)
{
    StubLog("g_collision::DynSetNextCollOwner");
}

// STUB: GAME 0x5c3d60
void g_collision::DynSetNextCollOwnerMult(tDynamicCollObject **objs, uint num, void *callback)
{
    StubLog("g_collision::DynSetNextCollOwnerMult");
}

#pragma auto_inline(on)

// The grid's cells with objects emptied and taken out onto the cells to check, then every cell of the two lists freed
// once (PS2's also clears the objects' unknown_c and a flag of its own list's, and keeps the cells)
// FUNCTION: GAME 0x5c3da0
void g_collision::DynWipeOut()
{
    if (dyn_grid)
    {
        for (uint y = 0; y < dyn_grid->height; y++)
        {
            for (uint x = 0; x < dyn_grid->width; x++)
            {
                if (dyn_grid->cells[x + y * dyn_grid->width] &&
                    dyn_grid->cells[x + y * dyn_grid->width]->objects.size())
                {
                    dyn_grid->cells[x + y * dyn_grid->width]->objects.clear();
                    dyn_queue.push_back(dyn_grid->cells[x + y * dyn_grid->width]);
                    dyn_grid->cells[x + y * dyn_grid->width] = NULL;
                }
            }
        }
        std::vector<tDynamicPolicko *> cells;
        tDynamicPolicko **it, **c;
        for (it = free_cells.begin(); it < free_cells.end(); it++)
        {
            for (c = cells.begin(); c < cells.end(); c++)
            {
                if (*it == *c)
                    break;
            }
            if (c >= cells.end())
                cells.push_back(*it);
        }
        free_cells.clear();
        for (it = dyn_queue.begin(); it < dyn_queue.end(); it++)
        {
            for (c = cells.begin(); c < cells.end(); c++)
            {
                if (*it == *c)
                    break;
            }
            if (c >= cells.end())
                cells.push_back(*it);
        }
        dyn_queue.clear();
        for (it = cells.begin(); it < cells.end(); it++)
        {
            (*it)->objects.clear();
            delete *it;
        }
        cells.clear();
    }
}

#pragma auto_inline(off)

// The PC's is in the file at 0x5c5040 (docs/game-files.md)
// STUB: GAME 0x5c6b10
int g_collision::_TestLineV(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal, uint flags,
    uint flags2)
{
    StubLog("g_collision::_TestLineV");
    return 0;
}

// The PC's is in the file at 0x5c5040 (docs/game-files.md)
// STUB: GAME 0x5c74d0
int g_collision::TestLineHStatic(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal,
    uint flags)
{
    StubLog("g_collision::TestLineHStatic");
    return 0;
}

// The PC's is in the file at 0x5c5040 (docs/game-files.md)
// STUB: GAME 0x5c8440
tDynamicCollObject *g_collision::TestLineHDynamic(const S_vector &from, const S_vector &dir, S_vector *pos,
    S_vector *normal, uint flags)
{
    StubLog("g_collision::TestLineHDynamic");
    return NULL;
}

// The PC's is in the file at 0x5c5040 (docs/game-files.md)
// STUB: GAME 0x5c8770
int g_collision::_TestLineH(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal, uint flags,
    uint flags2)
{
    StubLog("g_collision::_TestLineH");
    return 0;
}

// The PC's is in the file at 0x5c9e70 (docs/game-files.md)
// STUB: GAME 0x5ca5c0
uint g_collision::FindCollsByFrame(I3D_frame *frm, std::vector<g_collision_header *> &colls)
{
    StubLog("g_collision::FindCollsByFrame");
    return 0;
}

// PS2's is another file's than TestSphereB; the PC's is in the file at 0x5cafb0 (docs/game-files.md)
// STUB: GAME 0x5d39a0
int g_collision::_TestSphereB(const S_vector &from, const S_vector &dir, float radius, S_vector *pos,
    S_vector *normal, uint flags, uint flags2)
{
    StubLog("g_collision::_TestSphereB");
    return false;
}

#pragma auto_inline(on)

// The moving objects' collisions, then the level's; PS2's passes its bool on to DeleteCollsLevel
// FUNCTION: GAME 0x5d5900
void g_collision::GameDone()
{
    DynWipeOut();
    DeleteCollsLevel();
}

// Loads gifCollision whatever it is called on, as TestSphereB; PS2 has the wrappers in the PC's order (TestLineV,
// TestLineVInnerDyn, TestLineH, TestSphereB)
// FUNCTION: GAME 0x5d5910
int g_collision::TestLineV(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal, uint flags,
    uint flags2)
{
    return gifCollision._TestLineV(from, dir, pos, normal, flags, flags2);
}

// A jump on this (PS2 name), where TestLineV and TestSphereB load gifCollision
// FUNCTION: GAME 0x5d5930
int g_collision::TestLineH(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal, uint flags,
    uint flags2)
{
    return _TestLineH(from, dir, pos, normal, flags, flags2);
}

// PS2's g_colls.cpp has it after g_camera.cpp's functions (12 bytes, a jump with this); the PC's is in the file at
// 0x5d5900 and loads gifCollision whatever it is called on
// FUNCTION: GAME 0x5d5940
int g_collision::TestSphereB(const S_vector &from, const S_vector &dir, float radius, S_vector *pos, S_vector *normal,
    uint flags, uint flags2)
{
    return gifCollision._TestSphereB(from, dir, radius, pos, normal, flags, flags2);
}

#pragma auto_inline(off)

// STUB: GAME 0x5c36c0
void g_collision::DynInsert(tDynamicCollObject *obj)
{
    StubLog("g_collision::DynInsert");
}

// STUB: GAME 0x5c41d0
void g_collision::DynGetOut(tDynamicCollObject *obj, std::vector<tDynamicCollObject *> *colls)
{
    StubLog("g_collision::DynGetOut");
}

#pragma auto_inline(on)
