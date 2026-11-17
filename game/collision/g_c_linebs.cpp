// The collisions' searches (PS2 g_c_linebs.cpp, the third of PS2's collision files; the PC's file is the one at
// 0x5c9e70-0x5cafac, docs/game-files.md), written whole, PS2's functions in PS2's order: the volumes near a place (a
// volume that isn't a face, a cell's volumes, the cells around the place), the volumes of a frame, and the moving
// objects a line hits. Each fills the caller's vector. PS2's _TestLineS, LineInSTreeLevel and LineInSNonface come
// between FindCollsByFrame and FindDynamicCollsInLine; nothing calls them on the PC, so the linker dropped them.

#include "collision/g_c_glob.h"

// A volume that isn't a face, near pos (PS2's): an oriented box, a box with an oriented box in it and an axis-aligned
// box, each by the sphere around it, its half diagonal widened by radius, which has to reach pos; the volume then goes
// into colls unless it is there already. min and max are the caller's bounds, which it doesn't read. The cases come in
// the order the inline budget gives: the axis-aligned box last, where the original calls operator/, Magnitude, the
// difference and Square out of line. Open: ours has 2 to 4 units of budget more there (a dead body that size before
// the case gives the original's calls), and the oriented box's sum loads max.x before min.x in the original.
// FUNCTION: GAME 0x5c9e70
void g_collision::LocalityInNonface(g_volume_triface *vol, const S_vector &pos, const S_vector &min,
    const S_vector &max, float radius, std::vector<g_collision_header *> &colls)
{
    switch (vol->flags & 0xff)
    {
    case VOLTYPE_OBB:
    {
        g_volume_obb *obb = (g_volume_obb *)vol;
        S_vector half = (obb->extents.max - obb->extents.min) / 2.0f;
        S_vector center = (obb->extents.min + obb->extents.max) / 2.0f;
        center *= obb->matrix;
        float r = half.Magnitude() + radius;
        if ((pos - center).Square() > r * r)
            break;
        for (g_collision_header **it = colls.begin(); it < colls.end(); it++)
        {
            if (*it == (g_collision_header *)vol)
                return;
        }
        colls.push_back((g_collision_header *)vol);
        break;
    }
    case VOLTYPE_XTOBB:
    {
        g_volume_xtobb *box = (g_volume_xtobb *)vol;
        S_vector half = (box->extents.max - box->extents.min) / 2.0f;
        S_vector center = (box->extents.min + box->extents.max) / 2.0f;
        center *= box->matrix;
        float r = half.Magnitude() + radius;
        if ((pos - center).Square() > r * r)
            break;
        for (g_collision_header **it = colls.begin(); it < colls.end(); it++)
        {
            if (*it == (g_collision_header *)vol)
                return;
        }
        colls.push_back((g_collision_header *)vol);
        break;
    }
    case VOLTYPE_AABB:
    {
        g_volume_aabb *box = (g_volume_aabb *)vol;
        S_vector half = (box->bbox.max - box->bbox.min) / 2.0f;
        S_vector center = (box->bbox.min + box->bbox.max) / 2.0f;
        float r = half.Magnitude() + radius;
        if ((pos - center).Square() > r * r)
            break;
        for (g_collision_header **it = colls.begin(); it < colls.end(); it++)
        {
            if (*it == (g_collision_header *)vol)
                return;
        }
        colls.push_back((g_collision_header *)vol);
        break;
    }
    }
}

// A cell's volumes near pos (PS2's): those that aren't faces and aren't off, whose flags lie from mat_min to mat_max (the
// material in the high byte), through LocalityInNonface
// FUNCTION: GAME 0x5ca3a0
void g_collision::LocalityInTreeLevel(tCollTreeLevel *cell, const S_vector &pos, const S_vector &min,
    const S_vector &max, float radius, uint mat_min, uint mat_max, std::vector<g_collision_header *> &colls)
{
    int i = cell->num_volumes;
    g_volume_triface **vols = cell->volumes;
    for (; i > 0; i--)
    {
        g_volume_triface *f = *vols++;
        if (!(f->flags & (VOLF_NONFACE | VOLF_DISABLED)))
            continue;
        if (f->flags & VOLF_DISABLED)
            continue;
        if (f->flags < mat_min)
            continue;
        if (f->flags > mat_max)
            continue;
        LocalityInNonface(f, pos, min, max, radius, colls);
    }
}

// The level's static volumes within radius of pos (PS2's): the cells the bounds cover whose volumes reach the lower bound,
// each through LocalityInTreeLevel, with the material range shifted into the flags' high byte. Open: the original
// passes the lower bound for both bounds, which ours can't give without the two vectors trading slots, and its clear()
// calls _Destroy out of line, which in ours takes one more inline call after it.
// FUNCTION: GAME 0x5ca410
void g_collision::FindStaticCollsInLocality(const S_vector &pos, float radius, uint mat_min, uint mat_max,
    std::vector<g_collision_header *> &colls)
{
    colls.clear();
    if (level)
    {
        mat_min <<= 24;
        mat_max = mat_max << 24 | 0xffffff;
        S_vector v_min = pos - S_vector(radius, radius, radius);
        S_vector v_max = pos + S_vector(radius, radius, radius);
        float f = 1.0f / level->cell_size_x;
        int x0 = (int)((v_min.x - level->min_x) * f);
        int x1 = (int)((v_max.x - level->min_x) * f + 1.0f);
        f = 1.0f / level->cell_size_z;
        int z0 = (int)((v_min.z - level->min_z) * f);
        int z1 = (int)((v_max.z - level->min_z) * f + 1.0f);
        if (x0 < 0)
            x0 = 0;
        if (z0 < 0)
            z0 = 0;
        if (x1 > level->width)
            x1 = level->width;
        if (z1 > level->height)
            z1 = level->height;
        for (int z = z0; z < z1; z++)
        {
            for (int x = x0; x < x1; x++)
            {
                tCollTreeLevel *cell = level->cells[x + z * level->width];
                if (cell->max_y >= v_min.y)
                    LocalityInTreeLevel(cell, pos, v_min, v_max, radius, mat_min, mat_max, colls);
            }
        }
    }
}

// The level's volumes that aren't faces whose frame is frm (PS2's), by kind
// FUNCTION: GAME 0x5ca5c0
uint g_collision::FindCollsByFrame(I3D_frame *frm, std::vector<g_collision_header *> &colls)
{
    if (!level)
        return 0;
    colls.clear();
    uint i;
    for (i = 0; i < level->num_xtobbs; i++)
    {
        if (level->xtobbs[i].frame == frm)
            colls.push_back((g_collision_header *)&level->xtobbs[i]);
    }
    for (i = 0; i < level->num_aabbs; i++)
    {
        if (level->aabbs[i].frame == frm)
            colls.push_back((g_collision_header *)&level->aabbs[i]);
    }
    for (i = 0; i < level->num_spheres; i++)
    {
        if (level->spheres[i].frame == frm)
            colls.push_back((g_collision_header *)&level->spheres[i]);
    }
    for (i = 0; i < level->num_obbs; i++)
    {
        if (level->obbs[i].frame == frm)
            colls.push_back((g_collision_header *)&level->obbs[i]);
    }
    for (i = 0; i < level->num_cylinders; i++)
    {
        if (level->cylinders[i].frame == frm)
            colls.push_back((g_collision_header *)&level->cylinders[i]);
    }
    return colls.size();
}

// The moving objects the line from from along dir hits, into colls (PS2's), and their number: those in the grid's
// cells under the line's bounds and a cell around them, but those with one of flags and the next test's owners, whose
// bounding sphere the line passes within its radius and overlaps in x and z, tested as LineInDynamics tests them (the
// tracks left out), each against the whole line. Open: the original keeps DistanceToLine's quotient, whose address the
// out-of-line operator* takes, in len_2's slot (LineInDynamicsInnerDyn, with the same inline, gives it a slot of its
// own), so ours has a frame 4 bytes larger, and the sphere's dot product sums its terms in another order.
// FUNCTION: GAME 0x5ca790
uint g_collision::FindDynamicCollsInLine(const S_vector &from, const S_vector &dir,
    std::vector<g_collision_header *> &colls, uint flags)
{
    colls.clear();
    if (!dyn_grid)
        return 0;
    tCLineParam p;
    p.hit = NULL;
    p.from = from;
    p.dir = dir;
    p.to = from + dir;
    p.flags2 = VOLF_DISABLED;
    p.flags = flags;
    p.min_x = Min(from.x, p.to.x);
    p.min_z = Min(from.z, p.to.z);
    p.max_x = Max(from.x, p.to.x);
    p.max_z = Max(from.z, p.to.z);
    p.dir_sq = dir.Dot(dir);
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
            for (uint i = 0; i < cell->objects.size(); i++)
            {
                tDynamicCollObject *obj = cell->objects[i];
                p.t = 1.0f;
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
                if (((I3D_bsphere &)obj->bsphere).pos.DistanceToLine(p.from, p.dir) >
                    ((I3D_bsphere &)obj->bsphere).radius)
                    continue;
                if (((I3D_bsphere &)obj->bsphere).pos.x + ((I3D_bsphere &)obj->bsphere).radius < p.min_x)
                    continue;
                if (((I3D_bsphere &)obj->bsphere).pos.x - ((I3D_bsphere &)obj->bsphere).radius > p.max_x)
                    continue;
                if (((I3D_bsphere &)obj->bsphere).pos.z + ((I3D_bsphere &)obj->bsphere).radius < p.min_z)
                    continue;
                if (((I3D_bsphere &)obj->bsphere).pos.z - ((I3D_bsphere &)obj->bsphere).radius > p.max_z)
                    continue;
                switch (obj->type & 0xff)
                {
                case DYNCOLL_UNKNOWN_42:
                case DYNCOLL_UNKNOWN_44:
                {
                    S_vector d = ((I3D_bsphere &)obj->bsphere).pos - p.from;
                    float b = d.Dot(p.dir);
                    float r = ((I3D_bsphere &)obj->bsphere).radius;
                    float disc = b * b - (d.x * d.x + d.y * d.y + d.z * d.z - r * r) * p.dir_sq;
                    if (disc < 0.0f)
                        break;
                    float t = (b - sqrt(disc)) / p.dir_sq;
                    if (t < 0.0f)
                        break;
                    if (t >= p.t)
                        break;
                    colls.push_back((g_collision_header *)obj);
                    break;
                }
                case DYNCOLL_UNKNOWN_41:
                    if (LineInAABox(((I3D_bbox &)obj->bbox).min, ((I3D_bbox &)obj->bbox).max, p))
                        colls.push_back((g_collision_header *)obj);
                    break;
                case DYNCOLL_UNKNOWN_40:
                {
                    tCLineParam lp = p;
                    lp.from *= *obj->inv_matrix;
                    lp.to *= *obj->inv_matrix;
                    lp.dir = lp.to - lp.from;
                    if (LineInAABox(((I3D_bbox &)obj->bbox).min, ((I3D_bbox &)obj->bbox).max, lp))
                        colls.push_back((g_collision_header *)obj);
                    break;
                }
                }
            }
        }
    }
    return colls.size();
}
