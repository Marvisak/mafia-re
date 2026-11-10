// The collisions' wrappers, the file at 0x5d5900-0x5d6e4c (docs/game-files.md): PS2's g_colls.cpp, the last of PS2's
// collision files, which has GameDone, the wrappers of the tests (TestLineV, TestLineVInnerDyn, TestLineH, TestLineS,
// TestSphereB, TestDBox) and TestPanak2 and TestPanak2InVehicle, in the PC's order but for TestLineS: the PC has none
// before TestSphereB, and a function of its own after it (0x5d5960, TestSphereSlide, the wrapper of the sliding sphere
// test of the file at 0x5cafb0). Written whole; the file's last function is the std::ctype<unsigned short>::id
// initializer every game file ends with (0x5d6e20, <string> through Main.h).

#include "collision/g_c_glob.h"

#include "actors/C_human.h"

// Where a line meets the plane: VE_BreakageGlass.cpp's (Insanity3D's Intersection with its IntersectionPosition
// written in), which TestPanak2 and TestPanak2InVehicle inline for the plane each sphere hit, the line from where the
// sphere would end up along the plane's normal
inline bool S_plane::Intersection(const S_vector &pos, const S_vector &dir, S_vector &ret) const
{
    float f = normal.Dot(dir);
    if (fabs(f) < MRG_ZERO)
        return false;
    f = -(normal.Dot(pos) + d) / f;
    ret = pos + dir * f;
    return true;
}

// The moving objects' collisions, then the level's; PS2's passes its bool on to DeleteCollsLevel
// FUNCTION: GAME 0x5d5900
void g_collision::GameDone()
{
    DynWipeOut();
    DeleteCollsLevel();
}

// Loads gifCollision whatever it is called on, as TestLineVInnerDyn and TestSphereB
// FUNCTION: GAME 0x5d5910
int g_collision::TestLineV(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal, uint flags,
    uint flags2)
{
    return gifCollision._TestLineV(from, dir, pos, normal, flags, flags2);
}

// FUNCTION: GAME 0x5d5920
int g_collision::TestLineVInnerDyn(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal,
    uint flags, uint flags2)
{
    return gifCollision._TestLineVInnerDyn(from, dir, pos, normal, flags, flags2);
}

// A jump on this (PS2 name), where TestLineV and TestSphereB load gifCollision
// FUNCTION: GAME 0x5d5930
int g_collision::TestLineH(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal, uint flags,
    uint flags2)
{
    return _TestLineH(from, dir, pos, normal, flags, flags2);
}

// PS2's is a jump with this; the PC's loads gifCollision whatever it is called on
// FUNCTION: GAME 0x5d5940
int g_collision::TestSphereB(const S_vector &from, const S_vector &dir, float radius, S_vector *pos, S_vector *normal,
    uint flags, uint flags2)
{
    return gifCollision._TestSphereB(from, dir, radius, pos, normal, flags, flags2);
}

// The sliding sphere test (_TestSphereSlide) on gifCollision, whatever it is called on, without flags: its contacts,
// the planes the sphere slid along, into contacts and their number into num (PC only, the names are ours; only
// C_dyn_object::AI calls it). The end position and normal are dropped.
// FUNCTION: GAME 0x5d5960
int g_collision::TestSphereSlide(const S_vector &from, const S_vector &dir, float radius,
    S_slide_contact **contacts, uint *num)
{
    S_vector pos, normal;
    int ret = gifCollision._TestSphereSlide(from, dir, radius, &pos, &normal, 0, 0);
    *contacts = gifCollision.contacts;
    *num = gifCollision.num_contacts;
    return ret;
}

// A jump on this, as TestLineH (PS2 name), which only a vehicle's code calls (0x4e9088)
// FUNCTION: GAME 0x5d59b0
bool g_collision::TestDBox(const I3D_bbox &bbox, const S_matrix &m0, const S_matrix &m1, const S_vector &dir,
    const S_vector &v1, const S_vector &v2, uint flags, uint flags2)
{
    return _TestDBox(bbox, m0, m1, dir, v1, v2, flags, flags2);
}

// The human's three spheres moved together by p->dir, up to ten passes (PS2 name): each pass moves the three by the
// same step against the level and the moving objects (_TestSphereB, the legs' sphere pushing the scattered objects it
// hits), and of the spheres that hit, the nearest one's normal goes into p->normal and its hit plane takes the rest of
// the move: the step is the way to its hit, the next move the rest projected onto the plane and a hundredth of the
// normal away from it, a fifth of it where the sphere started inside the plane. The passes end with one that hits
// nothing. The collision hit last (0 for none), each pass's into p->hits, the legs' sphere's end into p->pos_out.
// which starts at 4 (PS2's), so VC6 drops the first test. Each case has a plane and a target of its own at function
// scope: the original's frame has an area for each, where locals of the case blocks would share one. Open: the inline
// budget, about one statement's worth short (the final sum's constructor in case 1, case 3's operator-, legs' +=).
// FUNCTION: GAME 0x5d59c0
int g_collision::TestPanak2(tgcPanak2 *p)
{
    int hit = 0;
    int i = 0;
    int which = 4;
    S_vector legs = p->legs;
    S_vector body = p->body;
    S_vector head = p->head;
    S_vector dir = p->dir;
    S_vector move;
    S_vector pos0, n0, pos1, n1, pos2, n2;
    S_plane pl0, pl1, pl2;
    S_vector to0, to1, to2;
    while (which && i < 10)
    {
        push_scatter = true;
        float best = FLT_MAX;
        which = 0;
        int r = _TestSphereB(legs, dir, p->radius[0], &pos0, &n0, p->flags, p->flags2);
        if (r)
        {
            best = lastSphereDistance;
            which = 1;
            hit = r;
            p->normal = n0;
        }
        push_scatter = false;
        r = _TestSphereB(body, dir, p->radius[1], &pos1, &n1, p->flags, p->flags2);
        if (r && best > lastSphereDistance)
        {
            best = lastSphereDistance;
            which = 2;
            hit = r;
            p->normal = n1;
        }
        r = _TestSphereB(head, dir, p->radius[2], &pos2, &n2, p->flags, p->flags2);
        if (r && best > lastSphereDistance)
        {
            best = lastSphereDistance;
            which = 3;
            hit = r;
            p->normal = n2;
        }
        p->hits[i] = hit;
        switch (which)
        {
        case 0:
            move = dir;
            break;
        case 1:
            move = pos0 - legs;
            pl0.normal = n0;
            pl0.d = -n0.Dot(pos0);
            if (!pl0.Intersection(legs + dir, n0, to0))
                to0 = legs + dir;
            dir = to0 - pos0 + n0 * .01f;
            break;
        case 2:
            move = pos1 - body;
            pl1.normal = n1;
            pl1.d = -n1.Dot(pos1);
            if (!pl1.Intersection(body + dir, n1, to1))
                to1 = body + dir;
            dir = to1 - pos1 + n1 * .01f;
            break;
        case 3:
            move = pos2 - head;
            pl2.normal = n2;
            pl2.d = -n2.Dot(pos2);
            if (!pl2.Intersection(head + dir, n2, to2))
                to2 = head + dir;
            dir = to2 - pos2 + n2 * .01f;
            break;
        }
        if (lastSphereDistance < 0.0f)
            dir = dir / 5.0f;
        legs += move;
        body += move;
        head += move;
        i++;
    }
    p->pos_out = legs;
    p->last_pass = i - 1;
    return hit;
}

// The same against the collisions of a vehicle the human is in (PS2 name; TestSphereInVehicle), up to four passes,
// without the scattered objects, the passes' hits and the normal (PS2's counts i from 4 down as well). Open: the inline
// budget at case 3's end (the original inlines n2 * .01f and to2 - pos2, their constructors out of line).
// FUNCTION: GAME 0x5d6430
int g_collision::TestPanak2InVehicle(std::vector<tDynamicCollObject *> *colls, tgcPanak2 *p)
{
    int hit = 0;
    int i = 4;
    int which = 4;
    S_vector legs = p->legs;
    S_vector body = p->body;
    S_vector head = p->head;
    S_vector dir = p->dir;
    S_vector move;
    S_vector pos0, n0, pos1, n1, pos2, n2;
    S_plane pl0, pl1, pl2;
    S_vector to0, to1, to2;
    while (which && i--)
    {
        float best = FLT_MAX;
        which = 0;
        int r = TestSphereInVehicle(colls, legs, dir, p->radius[0], &pos0, &n0, p->flags, p->flags2);
        if (r)
        {
            best = lastSphereDistance;
            which = 1;
            hit = r;
        }
        r = TestSphereInVehicle(colls, body, dir, p->radius[1], &pos1, &n1, p->flags, p->flags2);
        if (r && best > lastSphereDistance)
        {
            best = lastSphereDistance;
            which = 2;
            hit = r;
        }
        r = TestSphereInVehicle(colls, head, dir, p->radius[2], &pos2, &n2, p->flags, p->flags2);
        if (r && best > lastSphereDistance)
        {
            best = lastSphereDistance;
            which = 3;
            hit = r;
        }
        switch (which)
        {
        case 0:
            move = dir;
            break;
        case 1:
            move = pos0 - legs;
            pl0.normal = n0;
            pl0.d = -n0.Dot(pos0);
            if (!pl0.Intersection(legs + dir, n0, to0))
                to0 = legs + dir;
            dir = to0 - pos0 + n0 * .01f;
            break;
        case 2:
            move = pos1 - body;
            pl1.normal = n1;
            pl1.d = -n1.Dot(pos1);
            if (!pl1.Intersection(body + dir, n1, to1))
                to1 = body + dir;
            dir = to1 - pos1 + n1 * .01f;
            break;
        case 3:
            move = pos2 - head;
            pl2.normal = n2;
            pl2.d = -n2.Dot(pos2);
            if (!pl2.Intersection(head + dir, n2, to2))
                to2 = head + dir;
            dir = to2 - pos2 + n2 * .01f;
            break;
        }
        if (lastSphereDistance < 0.0f)
            dir = dir / 5.0f;
        legs += move;
        body += move;
        head += move;
    }
    p->pos_out = legs;
    return hit;
}
