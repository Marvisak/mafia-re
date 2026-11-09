// The collisions' line tests (PS2 g_c_line.cpp, the second of PS2's collision files; the PC's file is the one at
// 0x5c5040-0x5c9e6c, docs/game-files.md), written whole, PS2's functions in PS2's order: the moving objects' tests,
// the volumes' (an axis-aligned box from outside and from inside, the volumes that aren't faces, a cell's volumes),
// the line through the rectangle of cells its bounds cover, the line walked cell by cell against the level and
// against the moving objects, the vehicles'. Every test fills a tCLineParam and keeps the nearest hit in it. PS2's
// LineInAACapsule tests a volume type with a matrix the PC's level doesn't have, so it has no PC counterpart.

#include "collision/g_c_glob.h"

#include "actors/C_scatter.h"

// The octants of a horizontal line, which order TestLineHStatic's and _TestLineH's walks over the grid's cells (the
// names are ours): along z where the line runs more along z than along x (row by row, otherwise column by column),
// and the signs of its x and z
#define LINEH_NEG_X 1
#define LINEH_NEG_Z 2
#define LINEH_ALONG_Z 8

// The moving objects in the grid's cells under the line's bounds and a cell around them, but those with one of the
// line's flags and the next test's owners, whose bounding sphere the line passes within its radius and overlaps in x and
// z: the spheres (0x42, 0x44) by where the line enters them, a box (0x41) by LineInAABox, an oriented box (0x40) the
// same in its own space (the line's ends moved into it by the inverse matrix, the normal turned back by the matrix); a
// track's (0x48) only goes to the stepping human's cbCollTester. True where one was hit nearer than p's hit, which
// then goes into p. The sphere and oriented box cases are LineInVehicle's.
// FUNCTION: GAME 0x5c5040
bool g_collision::LineInDynamics(tCLineParam &p)
{
    bool hit = false;
    int x0 = (int)((p.min_x - dyn_grid->min_x) / dyn_grid->cell_size_x - 1.0f);
    int x1 = (int)((p.max_x - dyn_grid->min_x) / dyn_grid->cell_size_x + 2.0f);
    int z0 = (int)((p.min_z - dyn_grid->min_z) / dyn_grid->cell_size_z - 1.0f);
    int z1 = (int)((p.max_z - dyn_grid->min_z) / dyn_grid->cell_size_z + 2.0f);
    if (x0 < 0)
        x0 = 0;
    if (z0 < 0)
        z0 = 0;
    if ((uint)x1 > dyn_grid->width)
        x1 = dyn_grid->width;
    if ((uint)z1 > dyn_grid->height)
        z1 = dyn_grid->height;
    for (int z = z0; z < z1; z++)
    {
        for (int x = x0; x < x1; x++)
        {
            tDynamicPolicko *cell = dyn_grid->cells[x + z * dyn_grid->width];
            if (!cell)
                continue;
            for (tDynamicCollObject **it = cell->objects.begin(); it < cell->objects.end(); it++)
            {
                if ((*it)->flags & p.flags)
                    continue;
                bool owner = false;
                for (uint i = num_next_coll_owners; i--;)
                {
                    if (*it == next_coll_owners[i])
                    {
                        owner = true;
                        break;
                    }
                }
                if (owner)
                    continue;
                if (((I3D_bsphere &)(*it)->bsphere).pos.DistanceToLine(p.from, p.dir) >
                    ((I3D_bsphere &)(*it)->bsphere).radius)
                    continue;
                if (((I3D_bsphere &)(*it)->bsphere).pos.x + ((I3D_bsphere &)(*it)->bsphere).radius < p.min_x)
                    continue;
                if (((I3D_bsphere &)(*it)->bsphere).pos.x - ((I3D_bsphere &)(*it)->bsphere).radius > p.max_x)
                    continue;
                if (((I3D_bsphere &)(*it)->bsphere).pos.z + ((I3D_bsphere &)(*it)->bsphere).radius < p.min_z)
                    continue;
                if (((I3D_bsphere &)(*it)->bsphere).pos.z - ((I3D_bsphere &)(*it)->bsphere).radius > p.max_z)
                    continue;
                switch ((*it)->type & 0xff)
                {
                case DYNCOLL_UNKNOWN_48:
                    if (step_actor)
                        step_actor->cbCollTester(*it);
                    break;
                case DYNCOLL_UNKNOWN_42:
                case DYNCOLL_UNKNOWN_44:
                {
                    const I3D_bsphere &bs = (I3D_bsphere &)(*it)->bsphere;
                    S_vector d = bs.pos - p.from;
                    float b = d.Dot(p.dir);
                    float r = bs.radius;
                    float disc = b * b - (d.x * d.x + d.y * d.y + d.z * d.z - r * r) * p.dir_sq;
                    if (disc < 0.0f)
                        break;
                    float t = (b - sqrt(disc)) / p.dir_sq;
                    if (t < 0.0f)
                        break;
                    if (t >= p.t)
                        break;
                    p.normal = bs.pos - (p.from + p.dir * t);
                    p.t = t;
                    p.hit = (g_collision_header *)*it;
                    hit = true;
                    break;
                }
                case DYNCOLL_UNKNOWN_41:
                    if (LineInAABox(((I3D_bbox &)(*it)->bbox).min, ((I3D_bbox &)(*it)->bbox).max, p))
                    {
                        p.hit = (g_collision_header *)*it;
                        hit = true;
                    }
                    break;
                case DYNCOLL_UNKNOWN_40:
                {
                    tCLineParam lp = p;
                    lp.from *= *(*it)->inv_matrix;
                    lp.to *= *(*it)->inv_matrix;
                    lp.dir = lp.to - lp.from;
                    if (LineInAABox(((I3D_bbox &)(*it)->bbox).min, ((I3D_bbox &)(*it)->bbox).max, lp))
                    {
                        p.t = lp.t;
                        p.normal = lp.normal.RotateByMatrix(*(*it)->matrix);
                        hit = true;
                        p.hit = (g_collision_header *)*it;
                    }
                    break;
                }
                }
            }
        }
    }
    return hit;
}

// LineInDynamics for _TestLineVInnerDyn: a sphere the line starts in is hit at once (t 0, the normal from the line's
// start to its center), and a scattered object (0x47) goes to its owner's LineHit, where that is a C_scatter, in place
// of the tracks' test
// FUNCTION: GAME 0x5c5580
bool g_collision::LineInDynamicsInnerDyn(tCLineParam &p)
{
    bool hit = false;
    int x0 = (int)((p.min_x - dyn_grid->min_x) / dyn_grid->cell_size_x - 1.0f);
    int x1 = (int)((p.max_x - dyn_grid->min_x) / dyn_grid->cell_size_x + 2.0f);
    int z0 = (int)((p.min_z - dyn_grid->min_z) / dyn_grid->cell_size_z - 1.0f);
    int z1 = (int)((p.max_z - dyn_grid->min_z) / dyn_grid->cell_size_z + 2.0f);
    if (x0 < 0)
        x0 = 0;
    if (z0 < 0)
        z0 = 0;
    if ((uint)x1 > dyn_grid->width)
        x1 = dyn_grid->width;
    if ((uint)z1 > dyn_grid->height)
        z1 = dyn_grid->height;
    for (int z = z0; z < z1; z++)
    {
        for (int x = x0; x < x1; x++)
        {
            tDynamicPolicko *cell = dyn_grid->cells[x + z * dyn_grid->width];
            if (!cell)
                continue;
            for (tDynamicCollObject **it = cell->objects.begin(); it < cell->objects.end(); it++)
            {
                if ((*it)->flags & p.flags)
                    continue;
                bool owner = false;
                for (uint i = num_next_coll_owners; i--;)
                {
                    if (*it == next_coll_owners[i])
                    {
                        owner = true;
                        break;
                    }
                }
                if (owner)
                    continue;
                if (((I3D_bsphere &)(*it)->bsphere).pos.DistanceToLine(p.from, p.dir) >
                    ((I3D_bsphere &)(*it)->bsphere).radius)
                    continue;
                if (((I3D_bsphere &)(*it)->bsphere).pos.x + ((I3D_bsphere &)(*it)->bsphere).radius < p.min_x)
                    continue;
                if (((I3D_bsphere &)(*it)->bsphere).pos.x - ((I3D_bsphere &)(*it)->bsphere).radius > p.max_x)
                    continue;
                if (((I3D_bsphere &)(*it)->bsphere).pos.z + ((I3D_bsphere &)(*it)->bsphere).radius < p.min_z)
                    continue;
                if (((I3D_bsphere &)(*it)->bsphere).pos.z - ((I3D_bsphere &)(*it)->bsphere).radius > p.max_z)
                    continue;
                switch ((*it)->type & 0xff)
                {
                case DYNCOLL_UNKNOWN_47:
                    if ((*it)->owner->type == ACTOR_SCATTER)
                        ((C_scatter *)(*it)->owner)->LineHit(*it, p);
                    break;
                case DYNCOLL_UNKNOWN_42:
                case DYNCOLL_UNKNOWN_44:
                {
                    const I3D_bsphere &bs = (I3D_bsphere &)(*it)->bsphere;
                    S_vector d = bs.pos - p.from;
                    float r2 = bs.radius * bs.radius;
                    float sq = d.Square();
                    if (sq < r2)
                    {
                        p.normal = ((I3D_bsphere &)(*it)->bsphere).pos - p.from;
                        p.t = 0.0f;
                        p.hit = (g_collision_header *)*it;
                        hit = true;
                        break;
                    }
                    float b = d.Dot(p.dir);
                    float disc = b * b - (sq - r2) * p.dir_sq;
                    if (disc < 0.0f)
                        break;
                    float t = (b - sqrt(disc)) / p.dir_sq;
                    if (t < 0.0f)
                        break;
                    if (t >= p.t)
                        break;
                    p.normal = bs.pos - (p.from + p.dir * t);
                    p.t = t;
                    p.hit = (g_collision_header *)*it;
                    hit = true;
                    break;
                }
                case DYNCOLL_UNKNOWN_41:
                    if (LineInAABox(((I3D_bbox &)(*it)->bbox).min, ((I3D_bbox &)(*it)->bbox).max, p))
                    {
                        p.hit = (g_collision_header *)*it;
                        hit = true;
                    }
                    break;
                case DYNCOLL_UNKNOWN_40:
                {
                    // Declared, then assigned (or a memcpy): initialized from p, as LineInDynamics' copy, it leaves
                    // the budget for DistanceToLine's operator* above, which the original calls out of line here
                    tCLineParam lp;
                    lp = p;
                    lp.from *= *(*it)->inv_matrix;
                    lp.to *= *(*it)->inv_matrix;
                    lp.dir = lp.to - lp.from;
                    if (LineInAABox(((I3D_bbox &)(*it)->bbox).min, ((I3D_bbox &)(*it)->bbox).max, lp))
                    {
                        p.t = lp.t;
                        p.normal = lp.normal.RotateByMatrix(*(*it)->matrix);
                        hit = true;
                        p.hit = (g_collision_header *)*it;
                    }
                    break;
                }
                }
            }
        }
    }
    return hit;
}

// The line against an axis-aligned box from outside, an axis at a time (y, x, z): the plane of the face the line
// enters along the axis it moves on (min's moving up the axis, max's down), and where the line meets it nearer than
// p's hit, ahead of from and inside the face's bounds, its place along dir into p.t and the axis as the normal, the
// way the line moves (not the face's outward normal). A face plane past the line's end ends the test, and so does the
// z face's plane at p's hit or beyond.
// FUNCTION: GAME 0x5c5b20
bool g_collision::LineInAABox(const S_vector &min, const S_vector &max, tCLineParam &p)
{
    float dy = p.dir.y;
    if (fabs(dy) > MRG_ZERO) {
        float t;
        if (dy > 0.0f)
            t = (min.y - p.from.y) / dy;
        else
            t = (max.y - p.from.y) / dy;
        if (t > 1.0f)
            return false;
        if (t < p.t && t > 0.0f) {
            S_vector v = p.from + p.dir * t;
            if (v.x >= min.x && v.x <= max.x && v.z >= min.z && v.z <= max.z) {
                if (dy > 0.0f)
                    p.normal = S_vector(0.0f, 1.0f, 0.0f);
                else
                    p.normal = S_vector(0.0f, -1.0f, 0.0f);
                p.t = t;
                return true;
            }
        }
    }
    float dx = p.dir.x;
    if (fabs(dx) > MRG_ZERO) {
        float t;
        if (dx > 0.0f)
            t = (min.x - p.from.x) / dx;
        else
            t = (max.x - p.from.x) / dx;
        if (t > 1.0f)
            return false;
        if (t < p.t && t > 0.0f) {
            S_vector v = p.from + p.dir * t;
            if (v.y >= min.y && v.y <= max.y && v.z >= min.z && v.z <= max.z) {
                if (dx > 0.0f)
                    p.normal = S_vector(1.0f, 0.0f, 0.0f);
                else
                    p.normal = S_vector(-1.0f, 0.0f, 0.0f);
                p.t = t;
                return true;
            }
        }
    }
    float dz = p.dir.z;
    if (fabs(dz) > MRG_ZERO) {
        float t;
        if (dz > 0.0f)
            t = (min.z - p.from.z) / dz;
        else
            t = (max.z - p.from.z) / dz;
        if (t > 1.0f)
            return false;
        if (t >= p.t)
            return false;
        if (t > 0.0f) {
            S_vector v = p.from + p.dir * t;
            if (v.y >= min.y && v.y <= max.y && v.x >= min.x && v.x <= max.x) {
                if (dz > 0.0f)
                    p.normal = S_vector(0.0f, 0.0f, 1.0f);
                else
                    p.normal = S_vector(0.0f, 0.0f, -1.0f);
                p.t = t;
                return true;
            }
        }
    }
    return false;
}

// The line against an axis-aligned box from inside (LineInVehicle's, its only caller), an axis at a time (y, x, z):
// the plane of the face the line leaves along the axis it moves on (max's moving up the axis, min's down), and where
// the line meets it nearer than p's hit and ahead of from, its place along dir into p.t and the axis as the normal,
// the way the line moves. The first such plane is taken, not the nearest, and the face's bounds aren't tested.
// FUNCTION: GAME 0x5c5ec0
bool g_collision::LineInAABoxInverted(const S_vector &min, const S_vector &max, tCLineParam &p)
{
    float dy = p.dir.y;
    if (fabs(dy) > MRG_ZERO) {
        float t;
        if (dy < 0.0f)
            t = (min.y - p.from.y) / dy;
        else
            t = (max.y - p.from.y) / dy;
        if (t < p.t && t > 0.0f) {
            if (dy > 0.0f)
                p.normal = S_vector(0.0f, 1.0f, 0.0f);
            else
                p.normal = S_vector(0.0f, -1.0f, 0.0f);
            p.t = t;
            return true;
        }
    }
    float dx = p.dir.x;
    if (fabs(dx) > MRG_ZERO) {
        float t;
        if (dx < 0.0f)
            t = (min.x - p.from.x) / dx;
        else
            t = (max.x - p.from.x) / dx;
        if (t < p.t && t > 0.0f) {
            if (dx > 0.0f)
                p.normal = S_vector(1.0f, 0.0f, 0.0f);
            else
                p.normal = S_vector(-1.0f, 0.0f, 0.0f);
            p.t = t;
            return true;
        }
    }
    float dz = p.dir.z;
    if (fabs(dz) > MRG_ZERO) {
        float t;
        if (dz < 0.0f)
            t = (min.z - p.from.z) / dz;
        else
            t = (max.z - p.from.z) / dz;
        if (t < p.t && t > 0.0f) {
            if (dz > 0.0f)
                p.normal = S_vector(0.0f, 0.0f, 1.0f);
            else
                p.normal = S_vector(0.0f, 0.0f, -1.0f);
            p.t = t;
            return true;
        }
    }
    return false;
}

// A volume that isn't a face, by its type (the cell's entries' type byte, as BoxInNonFace tells them apart), and where
// the line hits it nearer than p's hit, t, the normal and the volume into p: an upright cylinder (no ends, its axis at
// x, z) by the line's horizontal part, its normal from the hit to the axis; an oriented box, a rectangle at y = 0 in
// its own space, and the box with an oriented box in it, that box by LineInAABox, both with the line moved into their
// own space and the normal (the axis, as LineInAABox's) rotated back; an axis-aligned box by LineInAABox; a sphere,
// its normal from the hit to its center. The normals point into the volume and aren't normalized. PS2's has the
// cylinder written in too (with the same arithmetic: t is the chord's near end, then times the reciprocal); its
// LineInAACapsule is for a type of PS2's own, with a matrix, which the PC doesn't have.
// FUNCTION: GAME 0x5c60e0
bool g_collision::LineInNonface(g_volume_triface *vol, tCLineParam &p)
{
    switch (vol->flags & 0xff)
    {
    case VOLTYPE_CYLINDER:
    {
        g_volume_cylinder *cyl = (g_volume_cylinder *)vol;
        S_vector c(cyl->x, 0.0f, cyl->z);
        S_vector a(c.x - p.from.x, 0.0f, c.z - p.from.z);
        S_vector d = p.dir;
        d.y = 0.0f;
        float len = d.Magnitude();
        if (fabs(len) < MRG_ZERO)
            return false;
        float inv = 1.0f / len;
        d *= inv;
        float dot = a.Dot(d);
        float disc = cyl->radius * cyl->radius - (a.Dot(a) - dot * dot);
        if (disc < 0.0f)
            return false;
        float t = dot - sqrt(disc);
        t *= inv;
        if (t < 0.0f)
            return false;
        if (t >= p.t)
            return false;
        S_vector pos = p.from + p.dir * t;
        p.normal = S_vector(cyl->x, 0.0f, cyl->z) - pos;
        p.normal.y = 0.0f;
        p.t = t;
        p.hit = (g_collision_header *)vol;
        return true;
    }
    case VOLTYPE_OBB:
    {
        g_volume_obb *obb = (g_volume_obb *)vol;
        tCLineParam lp = p;
        lp.from *= obb->inv_matrix;
        lp.to *= obb->inv_matrix;
        lp.dir = lp.to - lp.from;
        if (fabs(lp.dir.y) > MRG_ZERO) {
            float t = -lp.from.y / lp.dir.y;
            if (t >= p.t)
                return false;
            if (t > 0.0f) {
                S_vector v = lp.from + lp.dir * t;
                if (v.x >= obb->extents.min.x && v.x <= obb->extents.max.x && v.z >= obb->extents.min.z &&
                    v.z <= obb->extents.max.z) {
                    p.t = t;
                    if (lp.dir.y > 0.0f)
                        p.normal = S_vector(0.0f, 1.0f, 0.0f).RotateByMatrix(obb->matrix);
                    else
                        p.normal = S_vector(0.0f, -1.0f, 0.0f).RotateByMatrix(obb->matrix);
                    p.hit = (g_collision_header *)vol;
                    return true;
                }
            }
        }
        return false;
    }
    case VOLTYPE_XTOBB:
    {
        g_volume_xtobb *box = (g_volume_xtobb *)vol;
        tCLineParam lp = p;
        lp.from *= box->inv_matrix;
        lp.to *= box->inv_matrix;
        lp.dir = lp.to - lp.from;
        if (LineInAABox(box->extents.min, box->extents.max, lp)) {
            p.t = lp.t;
            p.normal = lp.normal.RotateByMatrix(box->matrix);
            p.hit = (g_collision_header *)vol;
            return true;
        }
        return false;
    }
    case VOLTYPE_AABB:
    {
        g_volume_aabb *box = (g_volume_aabb *)vol;
        if (LineInAABox(box->bbox.min, box->bbox.max, p)) {
            p.hit = (g_collision_header *)vol;
            return true;
        }
        return false;
    }
    case VOLTYPE_SPHERE:
    {
        g_volume_sphere *sph = (g_volume_sphere *)vol;
        if (p.dir_sq == 0.0f)
            return false;
        S_vector a = sph->pos - p.from;
        float dot = a.Dot(p.dir);
        float disc = dot * dot - (a.Dot(a) - sph->radius * sph->radius) * p.dir_sq;
        if (disc < 0.0f)
            return false;
        float t = (dot - sqrt(disc)) / p.dir_sq;
        if (t < 0.0f)
            return false;
        if (t >= p.t)
            return false;
        S_vector pos = p.from + p.dir * t;
        p.normal = sph->pos - pos;
        p.t = t;
        p.hit = (g_collision_header *)vol;
        return true;
    }
    }
    return false;
}

// A cell's volumes against the line (PS2's): a volume that isn't a face through LineInNonface, a face where the line
// crosses its plane from the front within its bounds, nearer than p's hit, and the place lies inside it, tested in the
// two coordinates its coherence code picks (GrantCoherence). The faces are sorted by their first vertex's x, so the
// first beyond the line's bounds ends the search.
// FUNCTION: GAME 0x5c66a0
bool g_collision::LineInTreeLevel(tCollTreeLevel *cell, tCLineParam &p)
{
    bool hit = false;
    int i = cell->num_volumes;
    g_volume_triface **vols = cell->volumes;
    for (; i > 0; i--)
    {
        g_volume_triface *f = *vols++;
        if (p.flags2 & f->flags)
            continue;
        if (f->flags & (VOLF_NONFACE | VOLF_DISABLED))
        {
            hit |= LineInNonface(f, p);
            continue;
        }
        if (f->vertices[2]->x < p.min_x)
            continue;
        if (f->vertices[0]->x > p.max_x)
            break;
        // The face's bounds in z: the lower one, and the higher one in u, the variable that takes the place's first
        // coordinate on the face below (one variable, as the original's frame shows)
        float v, min_z = f->vertices[0]->z, u = min_z;
        float z = f->vertices[2]->z;
        if (z < min_z)
            min_z = z;
        else if (z > u)
            u = z;
        z = f->vertices[1]->z;
        if (z < min_z)
            min_z = z;
        else if (z > u)
            u = z;
        if (!(min_z <= p.max_z))
            continue;
        if (!(u >= p.min_z))
            continue;
        float d = f->normal.Dot(p.dir);
        if (d < MRG_ZERO)
            continue;
        float t = -(f->normal.Dot(p.from) + f->d) / d;
        if (t < 0.0f)
            continue;
        if (p.t <= t)
            continue;
        S_vector pt = p.from + p.dir * t;
        const S_vector &a = *f->vertices[0], &b = *f->vertices[1], &c = *f->vertices[2];
        float x0 = a.x, z0 = a.z;
        float px = pt.x - x0;
        float pz = pt.z - z0;
        float py = pt.y - a.y;
        float e1x = b.x - x0;
        float e2x = c.x - x0;
        float e1z = b.z - z0;
        float e2z = c.z - z0;
        float e1y = b.y - a.y;
        float e2y = c.y - a.y;
        switch ((uchar)f->flags)
        {
        case COHERENCE_X_LEVEL:
            u = py / e2y;
            if (u < 0.0f || u > 1.0f)
                continue;
            v = (pz - u * e2z) / e1z;
            if (v < 0.0f || u + v > 1.0f)
                continue;
            break;
        case COHERENCE_X:
            u = (e1y * pz - e1z * py) / (e1y * e2z - e2y * e1z);
            if (u < 0.0f || u > 1.0f)
                continue;
            v = (py - u * e2y) / e1y;
            if (v < 0.0f || u + v > 1.0f)
                continue;
            break;
        case COHERENCE_Y_ALONG_Z:
            u = px / e2x;
            if (u < 0.0f || u > 1.0f)
                continue;
            v = (pz - u * e2z) / e1z;
            if (v < 0.0f || u + v > 1.0f)
                continue;
            break;
        case COHERENCE_Y_ALONG_X:
            u = pz / e2z;
            if (u < 0.0f || u > 1.0f)
                continue;
            v = (px - u * e2x) / e1x;
            if (v < 0.0f || u + v > 1.0f)
                continue;
            break;
        case COHERENCE_Y:
            u = (e1x * pz - e1z * px) / (e2z * e1x - e1z * e2x);
            if (u < 0.0f || u > 1.0f)
                continue;
            v = (px - u * e2x) / e1x;
            if (v < 0.0f || u + v > 1.0f)
                continue;
            break;
        case COHERENCE_Z_UPRIGHT:
            u = px / e2x;
            if (u < 0.0f || u > 1.0f)
                continue;
            v = (py - u * e2y) / e1y;
            if (v < 0.0f || u + v > 1.0f)
                continue;
            break;
        case COHERENCE_Z:
            u = (e1x * py - e1y * px) / (e2y * e1x - e1y * e2x);
            if (u < 0.0f || u > 1.0f)
                continue;
            v = (px - u * e2x) / e1x;
            if (v < 0.0f || u + v > 1.0f)
                continue;
            break;
        default:
            continue;
        }
        p.t = t;
        p.normal = f->normal;
        hit = true;
        p.hit = (g_collision_header *)f;
    }
    return hit;
}

// A line from from along dir against the level (PS2's), the one for short or upright lines: the moving objects where
// flags isn't COLF_ALL, then every cell of the rectangle the line's bounds cover whose volumes reach the line's lower
// end (TestLineHStatic and _TestLineH walk only the cells the line crosses). The hit's place into pos, its normal
// turned against the line and normalized into normal; the volume or moving object hit returned, 0 for none.
// FUNCTION: GAME 0x5c6b10
int g_collision::_TestLineV(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal, uint flags,
    uint flags2)
{
    if (!level)
        return 0;
    tCLineParam p;
    p.hit = NULL;
    p.from = from;
    p.dir = dir;
    p.to = from + dir;
    p.flags2 = flags2 | VOLF_DISABLED;
    p.t = 1.0f;
    p.flags = flags;
    p.min_x = Min(from.x, p.to.x);
    p.min_z = Min(from.z, p.to.z);
    p.max_x = Max(from.x, p.to.x);
    p.max_z = Max(from.z, p.to.z);
    float min_y = Min(from.y, p.to.y);
    p.dir_sq = dir.Dot(dir);
    if (flags != COLF_ALL && dyn_grid)
        LineInDynamics(p);
    float f = 1.0f / level->cell_size_x;
    int x0 = (int)((p.min_x - level->min_x) * f);
    int x1 = (int)((p.max_x - level->min_x) * f + 1.0f);
    f = 1.0f / level->cell_size_z;
    int z0 = (int)((p.min_z - level->min_z) * f);
    int z1 = (int)((p.max_z - level->min_z) * f + 1.0f);
    if (x0 < 0)
        x0 = 0;
    if (z0 < 0)
        z0 = 0;
    if (x1 > level->width)
        x1 = level->width;
    if (z1 > level->height)
        z1 = level->height;
    p.walked = 0;
    for (int z = z0; z < z1; z++)
    {
        for (int x = x0; x < x1; x++)
        {
            tCollTreeLevel *cell = level->cells[x + z * level->width];
            if (cell->max_y >= min_y)
                LineInTreeLevel(cell, p);
            last_x = x;
            last_cell = cell;
            last_z = z;
            p.walked |= LINEWALK_POS_X;
        }
        p.walked |= LINEWALK_POS_Z;
    }
    if (!p.hit)
        return 0;
    if (pos)
        *pos = from + dir * p.t;
    if (normal)
    {
        *normal = -p.normal;
        normal->Normalize();
    }
    return (int)p.hit;
}

// _TestLineV with the moving objects tested through LineInDynamicsInnerDyn (PS2's), instruction for instruction
// FUNCTION: GAME 0x5c6ff0
int g_collision::_TestLineVInnerDyn(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal,
    uint flags, uint flags2)
{
    if (!level)
        return 0;
    tCLineParam p;
    p.hit = NULL;
    p.from = from;
    p.dir = dir;
    p.to = from + dir;
    p.flags2 = flags2 | VOLF_DISABLED;
    p.t = 1.0f;
    p.flags = flags;
    p.min_x = Min(from.x, p.to.x);
    p.min_z = Min(from.z, p.to.z);
    p.max_x = Max(from.x, p.to.x);
    p.max_z = Max(from.z, p.to.z);
    float min_y = Min(from.y, p.to.y);
    p.dir_sq = dir.Dot(dir);
    if (flags != COLF_ALL && dyn_grid)
        LineInDynamicsInnerDyn(p);
    float f = 1.0f / level->cell_size_x;
    int x0 = (int)((p.min_x - level->min_x) * f);
    int x1 = (int)((p.max_x - level->min_x) * f + 1.0f);
    f = 1.0f / level->cell_size_z;
    int z0 = (int)((p.min_z - level->min_z) * f);
    int z1 = (int)((p.max_z - level->min_z) * f + 1.0f);
    if (x0 < 0)
        x0 = 0;
    if (z0 < 0)
        z0 = 0;
    if (x1 > level->width)
        x1 = level->width;
    if (z1 > level->height)
        z1 = level->height;
    p.walked = 0;
    for (int z = z0; z < z1; z++)
    {
        for (int x = x0; x < x1; x++)
        {
            tCollTreeLevel *cell = level->cells[x + z * level->width];
            if (cell->max_y >= min_y)
                LineInTreeLevel(cell, p);
            last_x = x;
            last_cell = cell;
            last_z = z;
            p.walked |= LINEWALK_POS_X;
        }
        p.walked |= LINEWALK_POS_Z;
    }
    if (!p.hit)
        return 0;
    if (pos)
        *pos = from + dir * p.t;
    if (normal)
    {
        *normal = -p.normal;
        normal->Normalize();
    }
    return (int)p.hit;
}

// A line from from along dir against the level's static collisions (PS2's): a line nearly upright goes to _TestLineV
// (without the moving objects). Otherwise flags2 gives the volumes' flags left out, as _TestLineV's, but those with
// COLF2_UNKNOWN_200000 are left out only half the time, at random. The grid's cells are walked along the line, column
// by column where it runs more along x, row by row where it runs more along z (the line's octant picks the order),
// each cell whose volumes reach the line's lower end tested, until the column or row holds the end of the line at the
// nearest hit so far. The hit's place into pos, its normal turned against the line and normalized into normal; the
// volume hit returned, 0 for none.
// FUNCTION: GAME 0x5c74d0
int g_collision::TestLineHStatic(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal,
    uint flags2)
{
    if (fabs(dir.x) + fabs(dir.z) < MRG_ZERO)
        return _TestLineV(from, dir, pos, normal, COLF_ALL, flags2);
    if (!level)
        return 0;
    tCLineParam p;
    p.hit = NULL;
    p.walked = 0;
    p.flags2 = flags2 | VOLF_DISABLED;
    if (flags2 & COLF2_UNKNOWN_200000)
    {
        if (rand() & 8)
            p.flags2 ^= COLF2_UNKNOWN_200000;
    }
    p.t = 1.0f;
    p.from = from;
    p.dir = dir;
    p.to = from + dir;
    p.min_x = Min(from.x, p.to.x);
    p.min_z = Min(from.z, p.to.z);
    p.max_x = Max(from.x, p.to.x);
    p.max_z = Max(from.z, p.to.z);
    float min_y = Min(from.y, p.to.y);
    p.dir_sq = dir.Dot(dir);
    int x0 = (int)((p.min_x - level->min_x) / level->cell_size_x);
    int x1 = (int)((p.max_x - level->min_x) / level->cell_size_x + 1.0f);
    int z0 = (int)((p.min_z - level->min_z) / level->cell_size_z);
    int z1 = (int)((p.max_z - level->min_z) / level->cell_size_z + 1.0f);
    if (x0 < 0)
        x0 = 0;
    if (z0 < 0)
        z0 = 0;
    if (x1 > level->width)
        x1 = level->width;
    if (z1 > level->height)
        z1 = level->height;
    int octant = 0;
    if (fabs(dir.x) < fabs(dir.z))
        octant = LINEH_ALONG_Z;
    if (dir.x < 0.0f)
        octant |= LINEH_NEG_X;
    if (dir.z < 0.0f)
        octant |= LINEH_NEG_Z;
    // A column's rows from row0 to row1, or a row's columns from col0 to col1: one pair for each kind of walk, as the
    // original's frame has them (with one pair for both, its slots come out otherwise)
    int x, z, row0, row1, col0, col1;
    tCollTreeLevel *cell;
    switch (octant)
    {
    case 0:
        for (x = x0; x < x1; x++)
        {
            row0 = (int)((((level->grid_x[x] - from.x) / dir.x) * dir.z + from.z - level->min_z) /
                level->cell_size_z);
            row1 = (int)((((level->grid_x[x + 1] - from.x) / dir.x) * dir.z + from.z - level->min_z) /
                level->cell_size_z + 1.0f);
            row0 = max(row0, z0);
            row1 = min(row1, z1);
            p.walked &= ~LINEWALK_POS_Z;
            for (z = row0; z < row1; z++)
            {
                cell = level->cells[x + z * level->width];
                if (cell->max_y >= min_y)
                    LineInTreeLevel(cell, p);
                p.walked |= LINEWALK_POS_Z;
            }
            if (level->grid_x[x + 1] >= from.x + dir.x * p.t)
                break;
            p.walked |= LINEWALK_POS_X;
        }
        break;
    case LINEH_NEG_Z:
        for (x = x0; x < x1; x++)
        {
            row0 = (int)((((level->grid_x[x + 1] - from.x) / dir.x) * dir.z + from.z - level->min_z) /
                level->cell_size_z);
            row1 = (int)((((level->grid_x[x] - from.x) / dir.x) * dir.z + from.z - level->min_z) /
                level->cell_size_z + 1.0f);
            row0 = max(row0, z0);
            row1 = min(row1, z1);
            p.walked &= ~LINEWALK_POS_Z;
            for (z = row0; z < row1; z++)
            {
                cell = level->cells[x + z * level->width];
                if (cell->max_y >= min_y)
                    LineInTreeLevel(cell, p);
                p.walked |= LINEWALK_POS_Z;
            }
            if (level->grid_x[x + 1] >= from.x + dir.x * p.t)
                break;
            p.walked |= LINEWALK_POS_X;
        }
        break;
    case LINEH_NEG_X:
        for (x = x1 - 1; x >= x0; x--)
        {
            row0 = (int)((((level->grid_x[x + 1] - from.x) / dir.x) * dir.z + from.z - level->min_z) /
                level->cell_size_z);
            row1 = (int)((((level->grid_x[x] - from.x) / dir.x) * dir.z + from.z - level->min_z) /
                level->cell_size_z + 1.0f);
            row0 = max(row0, z0);
            row1 = min(row1, z1);
            p.walked &= ~LINEWALK_POS_Z;
            for (z = row0; z < row1; z++)
            {
                cell = level->cells[x + z * level->width];
                if (cell->max_y >= min_y)
                    LineInTreeLevel(cell, p);
                p.walked |= LINEWALK_POS_Z;
            }
            if (level->grid_x[x] <= from.x + dir.x * p.t)
                break;
            p.walked |= LINEWALK_NEG_X;
        }
        break;
    case LINEH_NEG_X | LINEH_NEG_Z:
        for (x = x1 - 1; x >= x0; x--)
        {
            row0 = (int)((((level->grid_x[x] - from.x) / dir.x) * dir.z + from.z - level->min_z) /
                level->cell_size_z);
            row1 = (int)((((level->grid_x[x + 1] - from.x) / dir.x) * dir.z + from.z - level->min_z) /
                level->cell_size_z + 1.0f);
            row0 = max(row0, z0);
            row1 = min(row1, z1);
            p.walked &= ~LINEWALK_POS_Z;
            for (z = row0; z < row1; z++)
            {
                cell = level->cells[x + z * level->width];
                if (cell->max_y >= min_y)
                    LineInTreeLevel(cell, p);
                p.walked |= LINEWALK_POS_Z;
            }
            if (level->grid_x[x] <= from.x + dir.x * p.t)
                break;
            p.walked |= LINEWALK_NEG_X;
        }
        break;
    case LINEH_ALONG_Z:
        for (z = z0; z < z1; z++)
        {
            col0 = (int)((((level->grid_z[z] - from.z) / dir.z) * dir.x + from.x - level->min_x) /
                level->cell_size_x);
            col1 = (int)((((level->grid_z[z + 1] - from.z) / dir.z) * dir.x + from.x - level->min_x) /
                level->cell_size_x + 1.0f);
            col0 = max(col0, x0);
            col1 = min(col1, x1);
            p.walked &= ~LINEWALK_POS_X;
            for (x = col0; x < col1; x++)
            {
                cell = level->cells[x + z * level->width];
                if (cell->max_y >= min_y)
                    LineInTreeLevel(cell, p);
                p.walked |= LINEWALK_POS_X;
            }
            if (level->grid_z[z + 1] >= from.z + dir.z * p.t)
                break;
            p.walked |= LINEWALK_POS_Z;
        }
        break;
    case LINEH_ALONG_Z | LINEH_NEG_X:
        for (z = z0; z < z1; z++)
        {
            col0 = (int)((((level->grid_z[z + 1] - from.z) / dir.z) * dir.x + from.x - level->min_x) /
                level->cell_size_x);
            col1 = (int)((((level->grid_z[z] - from.z) / dir.z) * dir.x + from.x - level->min_x) /
                level->cell_size_x + 1.0f);
            col0 = max(col0, x0);
            col1 = min(col1, x1);
            p.walked &= ~LINEWALK_POS_X;
            for (x = col0; x < col1; x++)
            {
                cell = level->cells[x + z * level->width];
                if (cell->max_y >= min_y)
                    LineInTreeLevel(cell, p);
                p.walked |= LINEWALK_POS_X;
            }
            if (level->grid_z[z + 1] >= from.z + dir.z * p.t)
                break;
            p.walked |= LINEWALK_POS_Z;
        }
        break;
    case LINEH_ALONG_Z | LINEH_NEG_Z:
        for (z = z1 - 1; z >= z0; z--)
        {
            col0 = (int)((((level->grid_z[z + 1] - from.z) / dir.z) * dir.x + from.x - level->min_x) /
                level->cell_size_x);
            col1 = (int)((((level->grid_z[z] - from.z) / dir.z) * dir.x + from.x - level->min_x) /
                level->cell_size_x + 1.0f);
            col0 = max(col0, x0);
            col1 = min(col1, x1);
            p.walked &= ~LINEWALK_POS_X;
            for (x = col0; x < col1; x++)
            {
                cell = level->cells[x + z * level->width];
                if (cell->max_y >= min_y)
                    LineInTreeLevel(cell, p);
                p.walked |= LINEWALK_POS_X;
            }
            // The row's upper edge, where the other walks take the far one (PS2's the same): the walk stops a
            // row late, once the line ends above the row just tested
            if (level->grid_z[z + 1] <= from.z + dir.z * p.t)
                break;
            p.walked |= LINEWALK_NEG_Z;
        }
        break;
    case LINEH_ALONG_Z | LINEH_NEG_X | LINEH_NEG_Z:
        for (z = z1 - 1; z >= z0; z--)
        {
            col0 = (int)((((level->grid_z[z] - from.z) / dir.z) * dir.x + from.x - level->min_x) /
                level->cell_size_x);
            col1 = (int)((((level->grid_z[z + 1] - from.z) / dir.z) * dir.x + from.x - level->min_x) /
                level->cell_size_x + 1.0f);
            col0 = max(col0, x0);
            col1 = min(col1, x1);
            p.walked &= ~LINEWALK_POS_X;
            for (x = col0; x < col1; x++)
            {
                cell = level->cells[x + z * level->width];
                if (cell->max_y >= min_y)
                    LineInTreeLevel(cell, p);
                p.walked |= LINEWALK_POS_X;
            }
            if (level->grid_z[z + 1] <= from.z + dir.z * p.t)
                break;
            p.walked |= LINEWALK_NEG_Z;
        }
        break;
    }
    if (!p.hit)
        return 0;
    if (pos)
        *pos = from + dir * p.t;
    if (normal)
    {
        // Through a reference: written to *normal directly, the normalization sums the squares in another order
        S_vector &n = *normal;
        n = -p.normal;
        n.Normalize();
    }
    return (int)p.hit;
}

// A line from from along dir against the moving objects' collisions only, through LineInDynamicsInnerDyn (PS2's):
// the object hit, NULL for none, the hit's place into pos and its normal turned against the line and normalized into
// normal
// FUNCTION: GAME 0x5c8440
tDynamicCollObject *g_collision::TestLineHDynamic(const S_vector &from, const S_vector &dir, S_vector *pos,
    S_vector *normal, uint flags)
{
    tCLineParam p;
    p.hit = NULL;
    p.t = 1.0f;
    p.from = from;
    p.dir = dir;
    p.to = from + dir;
    p.flags = flags;
    p.min_x = Min(from.x, p.to.x);
    p.min_z = Min(from.z, p.to.z);
    p.max_x = Max(from.x, p.to.x);
    p.max_z = Max(from.z, p.to.z);
    p.dir_sq = dir.Dot(dir);
    if (!dyn_grid || !LineInDynamicsInnerDyn(p))
        return NULL;
    if (pos)
        *pos = from + dir * p.t;
    if (normal)
    {
        // Through a reference: written to *normal directly, the normalization sums the squares in another order
        S_vector &n = *normal;
        n = -p.normal;
        n.Normalize();
    }
    return (tDynamicCollObject *)p.hit;
}

// A horizontal line from from along dir (PS2's): a line nearly upright goes to _TestLineV. Otherwise the moving objects
// unless flags is COLF_ALL, the line's bounds cut back to their nearest hit, then the grid's cells are walked along the
// line, column by column where it runs more along x, row by row where it runs more along z (eight cases by its
// direction's signs), each cell whose volumes reach the line's lower end tested, until the nearest hit lies within the
// column or row. The hit's place into pos, its normal turned against the line and normalized into normal; the volume
// or moving object hit returned, 0 for none.
// FUNCTION: GAME 0x5c8770
int g_collision::_TestLineH(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal, uint flags,
    uint flags2)
{
    if (fabs(dir.x) + fabs(dir.z) < MRG_ZERO)
        return _TestLineV(from, dir, pos, normal, flags, flags2);
    if (!level)
        return 0;
    tCLineParam p;
    p.flags2 = flags2 | VOLF_DISABLED;
    p.hit = NULL;
    p.flags = flags;
    p.walked = 0;
    p.from = from;
    p.dir = dir;
    p.to = from + dir;
    p.t = 1.0f;
    p.min_x = Min(from.x, p.to.x);
    p.min_z = Min(from.z, p.to.z);
    p.max_x = Max(from.x, p.to.x);
    p.max_z = Max(from.z, p.to.z);
    float min_y = Min(from.y, p.to.y);
    p.dir_sq = dir.Dot(dir);
    if (flags != COLF_ALL && dyn_grid && LineInDynamics(p))
    {
        p.to = from + dir * p.t;
        p.min_x = Min(from.x, p.to.x);
        p.min_z = Min(from.z, p.to.z);
        p.max_x = Max(from.x, p.to.x);
        p.max_z = Max(from.z, p.to.z);
    }
    int x0 = (int)((p.min_x - level->min_x) / level->cell_size_x);
    int x1 = (int)((p.max_x - level->min_x) / level->cell_size_x + 1.0f);
    int z0 = (int)((p.min_z - level->min_z) / level->cell_size_z);
    int z1 = (int)((p.max_z - level->min_z) / level->cell_size_z + 1.0f);
    if (x0 < 0)
        x0 = 0;
    if (z0 < 0)
        z0 = 0;
    if (x1 > level->width)
        x1 = level->width;
    if (z1 > level->height)
        z1 = level->height;
    int octant = 0;
    if (fabs(dir.x) < fabs(dir.z))
        octant = LINEH_ALONG_Z;
    if (dir.x < 0.0f)
        octant |= LINEH_NEG_X;
    if (dir.z < 0.0f)
        octant |= LINEH_NEG_Z;
    // A column's rows from row0 to row1, or a row's columns from col0 to col1: one pair for each kind of walk, as the
    // original's frame has them (with one pair for both, its slots come out otherwise). The two walks down the rows
    // keep prev, the row before's end or first column, for the cells beyond it, whose neighbour in that row wasn't
    // tested.
    int x, z, row0, row1, col0, col1, prev;
    tCollTreeLevel *cell;
    switch (octant)
    {
    case 0:
        for (x = x0; x < x1; x++)
        {
            row0 = (int)((((level->grid_x[x] - from.x) / dir.x) * dir.z + from.z - level->min_z) /
                level->cell_size_z);
            row1 = (int)((((level->grid_x[x + 1] - from.x) / dir.x) * dir.z + from.z - level->min_z) /
                level->cell_size_z + 1.0f);
            row0 = max(row0, z0);
            row1 = min(row1, z1);
            p.walked &= ~LINEWALK_POS_Z;
            for (z = row0; z < row1; z++)
            {
                cell = level->cells[x + z * level->width];
                if (cell->max_y >= min_y)
                    LineInTreeLevel(cell, p);
                p.walked |= LINEWALK_POS_Z;
            }
            if (level->grid_x[x + 1] >= from.x + dir.x * p.t)
                break;
            p.walked |= LINEWALK_POS_X;
        }
        break;
    case LINEH_NEG_Z:
        for (x = x0; x < x1; x++)
        {
            row0 = (int)((((level->grid_x[x + 1] - from.x) / dir.x) * dir.z + from.z - level->min_z) /
                level->cell_size_z);
            row1 = (int)((((level->grid_x[x] - from.x) / dir.x) * dir.z + from.z - level->min_z) /
                level->cell_size_z + 1.0f);
            row0 = max(row0, z0);
            row1 = min(row1, z1);
            p.walked &= ~LINEWALK_POS_Z;
            for (z = row0; z < row1; z++)
            {
                cell = level->cells[x + z * level->width];
                if (cell->max_y >= min_y)
                    LineInTreeLevel(cell, p);
                p.walked |= LINEWALK_POS_Z;
            }
            if (level->grid_x[x + 1] >= from.x + dir.x * p.t)
                break;
            p.walked |= LINEWALK_POS_X;
        }
        break;
    case LINEH_NEG_X:
        for (x = x1 - 1; x >= x0; x--)
        {
            row0 = (int)((((level->grid_x[x + 1] - from.x) / dir.x) * dir.z + from.z - level->min_z) /
                level->cell_size_z);
            row1 = (int)((((level->grid_x[x] - from.x) / dir.x) * dir.z + from.z - level->min_z) /
                level->cell_size_z + 1.0f);
            row0 = max(row0, z0);
            row1 = min(row1, z1);
            p.walked &= ~LINEWALK_POS_Z;
            for (z = row0; z < row1; z++)
            {
                cell = level->cells[x + z * level->width];
                if (cell->max_y >= min_y)
                    LineInTreeLevel(cell, p);
                p.walked |= LINEWALK_POS_Z;
            }
            if (level->grid_x[x] <= from.x + dir.x * p.t)
                break;
            p.walked |= LINEWALK_NEG_X;
        }
        break;
    case LINEH_NEG_X | LINEH_NEG_Z:
        for (x = x1 - 1; x >= x0; x--)
        {
            row0 = (int)((((level->grid_x[x] - from.x) / dir.x) * dir.z + from.z - level->min_z) /
                level->cell_size_z);
            row1 = (int)((((level->grid_x[x + 1] - from.x) / dir.x) * dir.z + from.z - level->min_z) /
                level->cell_size_z + 1.0f);
            row0 = max(row0, z0);
            row1 = min(row1, z1);
            p.walked &= ~LINEWALK_POS_Z;
            for (z = row0; z < row1; z++)
            {
                cell = level->cells[x + z * level->width];
                if (cell->max_y >= min_y)
                    LineInTreeLevel(cell, p);
                p.walked |= LINEWALK_POS_Z;
            }
            if (level->grid_x[x] <= from.x + dir.x * p.t)
                break;
            p.walked |= LINEWALK_NEG_X;
        }
        break;
    case LINEH_ALONG_Z:
        for (z = z0; z < z1; z++)
        {
            col0 = (int)((((level->grid_z[z] - from.z) / dir.z) * dir.x + from.x - level->min_x) /
                level->cell_size_x);
            col1 = (int)((((level->grid_z[z + 1] - from.z) / dir.z) * dir.x + from.x - level->min_x) /
                level->cell_size_x + 1.0f);
            col0 = max(col0, x0);
            col1 = min(col1, x1);
            p.walked &= ~LINEWALK_POS_X;
            for (x = col0; x < col1; x++)
            {
                cell = level->cells[x + z * level->width];
                if (cell->max_y >= min_y)
                    LineInTreeLevel(cell, p);
                p.walked |= LINEWALK_POS_X;
            }
            if (level->grid_z[z + 1] >= from.z + dir.z * p.t)
                break;
            p.walked |= LINEWALK_POS_Z;
        }
        break;
    case LINEH_ALONG_Z | LINEH_NEG_X:
        for (z = z0; z < z1; z++)
        {
            col0 = (int)((((level->grid_z[z + 1] - from.z) / dir.z) * dir.x + from.x - level->min_x) /
                level->cell_size_x);
            col1 = (int)((((level->grid_z[z] - from.z) / dir.z) * dir.x + from.x - level->min_x) /
                level->cell_size_x + 1.0f);
            col0 = max(col0, x0);
            col1 = min(col1, x1);
            p.walked &= ~LINEWALK_POS_X;
            for (x = col0; x < col1; x++)
            {
                cell = level->cells[x + z * level->width];
                if (cell->max_y >= min_y)
                    LineInTreeLevel(cell, p);
                p.walked |= LINEWALK_POS_X;
            }
            if (level->grid_z[z + 1] >= from.z + dir.z * p.t)
                break;
            p.walked |= LINEWALK_POS_Z;
        }
        break;
    case LINEH_ALONG_Z | LINEH_NEG_Z:
        prev = x1 + 1;
        for (z = z1 - 1; z >= z0; z--)
        {
            col0 = (int)((((level->grid_z[z + 1] - from.z) / dir.z) * dir.x + from.x - level->min_x) /
                level->cell_size_x);
            col1 = (int)((((level->grid_z[z] - from.z) / dir.z) * dir.x + from.x - level->min_x) /
                level->cell_size_x + 1.0f);
            col0 = max(col0, x0);
            col1 = min(col1, x1);
            p.walked &= ~LINEWALK_POS_X;
            for (x = col0; x < col1; x++)
            {
                cell = level->cells[x + z * level->width];
                if (x >= prev)
                    p.walked &= ~LINEWALK_NEG_Z;
                if (cell->max_y >= min_y)
                    LineInTreeLevel(cell, p);
                p.walked |= LINEWALK_POS_X;
            }
            // The row's upper edge, where the other walks take the far one (PS2's the same): the walk stops a
            // row late, once the line ends above the row just tested
            if (level->grid_z[z + 1] <= from.z + dir.z * p.t)
                break;
            prev = col1;
            p.walked |= LINEWALK_NEG_Z;
        }
        break;
    case LINEH_ALONG_Z | LINEH_NEG_X | LINEH_NEG_Z:
        prev = x0;
        for (z = z1 - 1; z >= z0; z--)
        {
            col0 = (int)((((level->grid_z[z] - from.z) / dir.z) * dir.x + from.x - level->min_x) /
                level->cell_size_x);
            col1 = (int)((((level->grid_z[z + 1] - from.z) / dir.z) * dir.x + from.x - level->min_x) /
                level->cell_size_x + 1.0f);
            col0 = max(col0, x0);
            col1 = min(col1, x1);
            p.walked &= ~LINEWALK_NEG_X;
            for (x = col1 - 1; x >= col0; x--)
            {
                cell = level->cells[x + z * level->width];
                if (x < prev)
                    p.walked &= ~LINEWALK_NEG_Z;
                if (cell->max_y >= min_y)
                    LineInTreeLevel(cell, p);
                p.walked |= LINEWALK_NEG_X;
            }
            if (level->grid_z[z + 1] <= from.z + dir.z * p.t)
                break;
            prev = col0;
            p.walked |= LINEWALK_NEG_Z;
        }
        break;
    }
    if (!p.hit)
        return 0;
    if (pos)
        *pos = from + dir * p.t;
    if (normal)
    {
        // Through a reference: written to *normal directly, the normalization sums the squares in another order
        S_vector &n = *normal;
        n = -p.normal;
        n.Normalize();
    }
    return (int)p.hit;
}

// The line against a vehicle's collisions (nothing for a null line), but those with one of the line's flags and the
// next test's owners: the spheres (0x42, 0x44) by where the line enters them, a box (0x41) by LineInAABox, an oriented
// box (0x40) the same in its own space (the line's ends moved into it by the inverse matrix, the normal turned back by
// the matrix) and one of type 0x45 so from inside (LineInAABoxInverted). True where one was hit nearer than p's hit,
// which then goes into p. PS2's has no owners' test, and capsules and convex objects besides.
// FUNCTION: GAME 0x5c97b0
bool g_collision::LineInVehicle(std::vector<tDynamicCollObject *> *colls, tCLineParam &p)
{
    if (fabs(p.dir_sq) < MRG_ZERO)
        return false;
    bool hit = false;
    for (uint i = 0; i < colls->size(); i++)
    {
        tDynamicCollObject *obj = (*colls)[i];
        if (obj->flags & p.flags)
            continue;
        bool owner = false;
        for (uint j = num_next_coll_owners; j--;)
        {
            if (obj == next_coll_owners[j])
            {
                owner = true;
                break;
            }
        }
        if (owner)
            continue;
        switch (obj->type & 0xff)
        {
        case DYNCOLL_UNKNOWN_42:
        case DYNCOLL_UNKNOWN_44:
        {
            const I3D_bsphere &bs = (I3D_bsphere &)obj->bsphere;
            S_vector d = bs.pos - p.from;
            float b = d.Dot(p.dir);
            float r = bs.radius;
            float disc = b * b - (d.x * d.x + d.y * d.y + d.z * d.z - r * r) * p.dir_sq;
            if (disc < 0.0f)
                continue;
            float t = (b - sqrt(disc)) / p.dir_sq;
            if (t < 0.0f)
                continue;
            if (t >= p.t)
                continue;
            p.normal = bs.pos - (p.from + p.dir * t);
            p.t = t;
            p.hit = (g_collision_header *)obj;
            hit = true;
            break;
        }
        case DYNCOLL_UNKNOWN_41:
            if (LineInAABox(((I3D_bbox &)obj->bbox).min, ((I3D_bbox &)obj->bbox).max, p))
            {
                p.hit = (g_collision_header *)obj;
                hit = true;
            }
            break;
        case DYNCOLL_UNKNOWN_40:
        {
            tCLineParam lp = p;
            lp.from *= *obj->inv_matrix;
            lp.to *= *obj->inv_matrix;
            lp.dir = lp.to - lp.from;
            if (LineInAABox(((I3D_bbox &)obj->bbox).min, ((I3D_bbox &)obj->bbox).max, lp))
            {
                p.t = lp.t;
                p.normal = lp.normal.RotateByMatrix(*obj->matrix);
                hit = true;
                p.hit = (g_collision_header *)obj;
            }
            break;
        }
        case DYNCOLL_UNKNOWN_45:
        {
            tCLineParam lp = p;
            lp.from *= *obj->inv_matrix;
            lp.to *= *obj->inv_matrix;
            lp.dir = lp.to - lp.from;
            if (LineInAABoxInverted(((I3D_bbox &)obj->bbox).min, ((I3D_bbox &)obj->bbox).max, lp))
            {
                p.t = lp.t;
                p.normal = lp.normal.RotateByMatrix(*obj->matrix);
                hit = true;
                p.hit = (g_collision_header *)obj;
            }
            break;
        }
        }
    }
    return hit;
}

// A line from from along dir against the collisions of a vehicle a human is in, but those with one of flags
// (LineInVehicle): the collision hit, NULL for none, its place into pos and the hit's normal negated and normalized into
// normal where they aren't NULL. PS2's adds a flag of its own to flags.
// FUNCTION: GAME 0x5c9b80
int g_collision::TestLineInVehicle(std::vector<tDynamicCollObject *> *colls, const S_vector &from, const S_vector &dir,
    S_vector *pos, S_vector *normal, uint flags)
{
    tCLineParam p;
    p.flags = flags;
    p.from = from;
    p.hit = NULL;
    p.t = 1.0f;
    p.dir = dir;
    p.to = from + dir;
    p.dir_sq = dir.Dot(dir);
    if (LineInVehicle(colls, p))
    {
        if (pos)
            *pos = from + dir * p.t;
        if (normal)
        {
            *normal = -p.normal;
            normal->Normalize();
        }
        return (int)p.hit;
    }
    return 0;
}
