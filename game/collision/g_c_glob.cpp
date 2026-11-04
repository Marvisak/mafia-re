// The collisions' globals (PS2 g_c_glob.cpp, the first of PS2's collision files: g_c_glob.cpp, g_c_line.cpp,
// g_c_linebs.cpp, g_c_sph.cpp and g_colls.cpp, which hold the PC's files at 0x5c2840-0x5d6e4c in that order;
// docs/game-files.md): gifCollision, whose initializer starts the file and whose constructor's copy is its last
// function, the level's loading, the moving objects' grid and the box test. PS2's PreallocPrepare, its first function,
// isn't on the PC.

#include "collision/g_c_glob.h"

#include "mission/Mission.h"
#include "rw_data.h"
#include "stub.h"

// GLOBAL: GAME 0x647f48
g_collision gifCollision;

// Set once LoadCollision has loaded a level, never cleared, read nowhere (PS2 name). Its bucket would put it before
// gifCollision, where the original has it after, with 0x28 bytes between (zero-initialized, unless it is another
// file's; open)
// GLOBAL: GAME 0x648238
bool LoadedLevel = false;

// The level's collisions and the grid freed, the grid's lists emptied (PS2's frees the lists too, and with its bool
// set keeps the level while its own member at 0xc is set, most likely PreallocPrepare's buffer)
// FUNCTION: GAME 0x5c28d0
void g_collision::DeleteCollsLevel()
{
    if (level)
    {
        if (level->cells)
            delete[] level->cells;
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

// The faces' coherence codes (PS2's): for each enabled face of each cell the axis its normal is nearest, and from it and
// the face's first edge its code in the low byte of its flags. The flags are put back afterwards with that byte as it
// is now.
// FUNCTION: GAME 0x5c2980
void GrantCoherence(tCollisionLevel *level)
{
    for (int y = 0; y < level->height; y++)
    {
        for (int x = 0; x < level->width; x++)
        {
            tCollTreeLevel *cell = level->cells[x + y * level->width];
            for (uint i = 0; i < cell->num_volumes; i++)
            {
                g_volume_triface *f = cell->volumes[i];
                if (f->flags & VOLF_NONFACE || f->flags & VOLF_DISABLED)
                    continue;
                uint flags = f->flags;
                float nx = (float)fabs(f->normal.x);
                float ny = (float)fabs(f->normal.y);
                float nz = (float)fabs(f->normal.z);
                int c;
                if (nx > ny)
                    c = nz > nx ? COHERENCE_AXIS_Z : COHERENCE_AXIS_X;
                else
                    c = nz > ny ? COHERENCE_AXIS_Z : COHERENCE_AXIS_Y;
                switch (c)
                {
                case COHERENCE_AXIS_Z:
                    if (fabs(f->vertices[1]->x - f->vertices[0]->x) < 1e-4)
                        c = COHERENCE_Z_UPRIGHT;
                    else
                        c = COHERENCE_Z;
                    break;
                case COHERENCE_AXIS_Y:
                    if (fabs(f->vertices[1]->x - f->vertices[0]->x) < 1e-4)
                        c = COHERENCE_Y_ALONG_Z;
                    else if (fabs(f->vertices[1]->z - f->vertices[0]->z) > -1e-4 &&
                        fabs(f->vertices[1]->z - f->vertices[0]->z) < 1e-4)
                        c = COHERENCE_Y_ALONG_X;
                    else
                        c = COHERENCE_Y;
                    break;
                case COHERENCE_AXIS_X:
                    if (fabs(f->vertices[1]->y - f->vertices[0]->y) > -1e-4 &&
                        fabs(f->vertices[1]->y - f->vertices[0]->y) < 1e-4)
                        c = COHERENCE_X_LEVEL;
                    else
                        c = COHERENCE_X;
                    break;
                }
                f->flags &= ~VOLF_COHERENCE;
                f->flags |= c;
                if ((flags & VOLF_COHERENCE) != (cell->volumes[i]->flags & VOLF_COHERENCE))
                    flags = (flags & ~VOLF_COHERENCE) | (cell->volumes[i]->flags & VOLF_COHERENCE);
                cell->volumes[i]->flags = flags;
            }
        }
    }
}

// A face's vertex from the file's reference: the vertex of the array the face's frame names (the name is ours)
static inline void SetVertex(S_vector *&v, I3D_vertex *verts)
{
    v = &verts[(uint)v & 0xffff].pos;
}

// Bounds widened to hold a value (the name is ours; LoadCollision's box of a face)
static inline void MinMax(float v, float &min, float &max)
{
    if (v < min)
        min = v;
    if (v > max)
        max = v;
}

// Loads a mission's collisions (tree.klz, read whole): the frames the file names found in the scene, then each visual's
// replaced by the vertices of its mesh's first lod (a mirror's and the other frames' by NULL), the level's arrays and
// the grid's cells set from the offsets in the data, the faces' vertices made pointers (a face whose frame has no
// vertices, or whose vertices nearly coincide, turned off), then with dyn the moving objects' grid laid over the level,
// and the faces' coherence. PS2's reads into a buffer of its own where PreallocPrepare gave it one.
// FUNCTION: GAME 0x5c2b70
void g_collision::LoadCollision(const char *name, bool dyn)
{
    step_actor = NULL;
    DeleteCollsLevel();
    int file = dtaOpen(name, false);
    if (file < 0)
        return;
    uint size = dtaSeek(file, 0, SEEK_END);
    dtaSeek(file, 0, SEEK_SET);
    S_klz_header header;
    dtaRead(file, &header, sizeof(header));
    if (header.magic != KLZ_MAGIC)
        return;
    S_klz_frame **frames = new S_klz_frame *[header.num_frames];
    dtaRead(file, frames, header.num_frames * sizeof(S_klz_frame *));
    char *names = new char[header.data_offset - (header.num_frames * sizeof(S_klz_frame *) + sizeof(header))];
    dtaRead(file, names, header.data_offset - (header.num_frames * sizeof(S_klz_frame *) + sizeof(header)));
    size -= dtaSeek(file, 0, SEEK_CUR);
    char *data = new char[size];
    level_size = size;
    dtaRead(file, data, size);
    dtaClose(file);
    // The table's offsets (from the header's start) made pointers into the names
    uint delta = (uint)names - (header.num_frames * sizeof(S_klz_frame *) + sizeof(header));
    uint i = header.num_frames;
    while (i--)
        frames[i] = (S_klz_frame *)((uint)frames[i] + delta);
    i = header.num_frames;
    while (i--)
    {
        uint flags = frames[i]->flags;
        I3D_scene *scene = act_mission->scene;
        frames[i]->frame = driver->FindFrameFast(scene, frames[i]->name);
        if (flags & KLZF_VISUAL && frames[i]->frame && frames[i]->frame->GetType() != FRAME_VISUAL)
            frames[i]->frame = NULL;
        if (flags & KLZF_VISUAL && frames[i]->frame)
            ((I3D_visual *)frames[i]->frame)->SetSpace(true);
    }
    LoadedLevel = true;
    // The arrays follow the header in this order, a dword after the vertices
    level = (tCollisionLevel *)data;
    level->grid_x = (float *)(data + sizeof(tCollisionLevel));
    level->grid_z = level->grid_x + level->width + 1;
    g_volume_triface *faces = (g_volume_triface *)(level->grid_z + level->height + 2);
    level->trifaces = faces;
    char *p = (char *)(faces + level->num_trifaces);
    level->aabbs = level->num_aabbs ? (g_volume_aabb *)p : NULL;
    p += level->num_aabbs * sizeof(g_volume_aabb);
    level->xtobbs = level->num_xtobbs ? (g_volume_xtobb *)p : NULL;
    p += level->num_xtobbs * sizeof(g_volume_xtobb);
    level->cylinders = level->num_cylinders ? (g_volume_cylinder *)p : NULL;
    p += level->num_cylinders * sizeof(g_volume_cylinder);
    level->obbs = level->num_obbs ? (g_volume_obb *)p : NULL;
    p += level->num_obbs * sizeof(g_volume_obb);
    level->spheres = level->num_spheres ? (g_volume_sphere *)p : NULL;
    p += level->num_spheres * sizeof(g_volume_sphere);
    level->vertices = level->num_vertices ? (S_vector *)p : NULL;
    p += level->num_vertices * sizeof(S_vector) + 4;
    for (i = 0; i < level->num_aabbs; i++)
        level->aabbs[i].frame = frames[(uint)level->aabbs[i].frame]->frame;
    for (i = 0; i < level->num_xtobbs; i++)
        level->xtobbs[i].frame = frames[(uint)level->xtobbs[i].frame]->frame;
    for (i = 0; i < level->num_cylinders; i++)
        level->cylinders[i].frame = frames[(uint)level->cylinders[i].frame]->frame;
    for (i = 0; i < level->num_obbs; i++)
        level->obbs[i].frame = frames[(uint)level->obbs[i].frame]->frame;
    for (i = 0; i < level->num_spheres; i++)
        level->spheres[i].frame = frames[(uint)level->spheres[i].frame]->frame;
    i = header.num_frames;
    while (i--)
    {
        I3D_frame *frm = frames[i]->frame;
        if (frm && frm->GetType() == FRAME_VISUAL && ((I3D_visual *)frm)->GetVisualType() != I3D_VISUAL_MIRROR &&
            ((I3D_object *)frm)->GetMesh())
            frames[i]->vertices = ((I3D_object *)frm)->GetMesh()->GetLOD(0)->LockVertices(I3DMESHLOCK_READONLY);
        else
            frames[i]->vertices = NULL;
    }
    for (i = 0; i < level->num_trifaces; i++)
    {
        g_volume_triface &f = faces[i];
        ushort frm = (ushort)((uint)f.vertices[0] >> 16);
        if (frm == KLZ_LEVEL_VERTICES)
        {
            f.vertices[0] = &level->vertices[(uint)f.vertices[0] & 0xffff];
            f.vertices[1] = &level->vertices[(uint)f.vertices[1] & 0xffff];
            f.vertices[2] = &level->vertices[(uint)f.vertices[2] & 0xffff];
        }
        else
        {
            I3D_vertex *verts = frames[frm]->vertices;
            if (verts)
            {
                SetVertex(f.vertices[0], verts);
                SetVertex(f.vertices[1], verts);
                SetVertex(f.vertices[2], verts);
                S_vector min, max;
                min = max = *f.vertices[0];
                MinMax(f.vertices[1]->x, min.x, max.x);
                MinMax(f.vertices[1]->y, min.y, max.y);
                MinMax(f.vertices[1]->z, min.z, max.z);
                MinMax(f.vertices[2]->x, min.x, max.x);
                MinMax(f.vertices[2]->y, min.y, max.y);
                MinMax(f.vertices[2]->z, min.z, max.z);
                S_vector d = max - min;
                uint n = 0;
                if ((*f.vertices[0] - *f.vertices[1]).Square() < MRG_ZERO)
                    n += 2;
                if ((*f.vertices[1] - *f.vertices[2]).Square() < MRG_ZERO)
                    n += 2;
                if ((*f.vertices[2] - *f.vertices[0]).Square() < MRG_ZERO)
                    n += 2;
                if (fabs(d.x) < MRG_ZERO)
                    n++;
                if (fabs(d.y) < MRG_ZERO)
                    n++;
                if (fabs(d.z) < MRG_ZERO)
                    n++;
                if (n > 1)
                    f.flags = VOLF_DISABLED;
            }
            else
                f.flags = VOLF_DISABLED;
        }
    }
    level->cells = new tCollTreeLevel *[level->height * level->width];
    faces = level->trifaces;
    for (int y = 0; y < level->height; y++)
    {
        for (int x = 0; x < level->width; x++)
        {
            // A cell, its volumes and a byte for each, the next cell aligned to 4
            tCollTreeLevel *cell = (tCollTreeLevel *)p;
            p += sizeof(tCollTreeLevel);
            cell->volumes = (g_volume_triface **)p;
            p += cell->num_volumes * sizeof(g_volume_triface *);
            cell->unknown_8 = (uchar *)p;
            p += cell->num_volumes;
            if (cell->num_volumes & 3)
                p += 4 - (cell->num_volumes & 3);
            level->cells[x + y * level->width] = cell;
            for (uint j = 0; j < cell->num_volumes; j++)
            {
                uint &e = (uint &)cell->volumes[j];
                switch (e >> 24)
                {
                case VOLTYPE_TRIFACE:
                    e += (uint)faces;
                    break;
                case VOLTYPE_AABB:
                    e = (e & 0xffffff) + (uint)level->aabbs;
                    break;
                case VOLTYPE_XTOBB:
                    e = (e & 0xffffff) + (uint)level->xtobbs;
                    break;
                case VOLTYPE_CYLINDER:
                    e = (e & 0xffffff) + (uint)level->cylinders;
                    break;
                case VOLTYPE_OBB:
                    e = (e & 0xffffff) + (uint)level->obbs;
                    break;
                case VOLTYPE_SPHERE:
                    e = (e & 0xffffff) + (uint)level->spheres;
                    break;
                }
            }
        }
    }
    delete[] frames;
    delete[] names;
    if (dyn)
    {
        if (dyn_grid)
            delete[] dyn_grid->cells;
        else
            dyn_grid = new S_dyn_grid;
        dyn_grid->width = (int)((level->max_x - level->min_x) / level->dyn_cell_size);
        dyn_grid->height = (int)((level->max_z - level->min_z) / level->dyn_cell_size);
        if (dyn_grid->width < 1)
            dyn_grid->width = 1;
        if (dyn_grid->height < 1)
            dyn_grid->height = 1;
        dyn_grid->cell_size_x = (level->max_x - level->min_x) / dyn_grid->width;
        dyn_grid->cell_size_z = (level->max_z - level->min_z) / dyn_grid->height;
        dyn_grid->min_x = level->min_x;
        dyn_grid->min_z = level->min_z;
        dyn_grid->cells = new tDynamicPolicko *[dyn_grid->height * dyn_grid->width];
        memset(dyn_grid->cells, 0, dyn_grid->height * dyn_grid->width * sizeof(tDynamicPolicko *));
        free_cells.clear();
        dyn_queue.clear();
    }
    GrantCoherence(level);
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

// Puts the object into the cell of its sphere's position, the cell taken from the free ones or created where it has
// none yet
// FUNCTION: GAME 0x5c36c0
void g_collision::DynInsert(tDynamicCollObject *obj)
{
    if (!dyn_grid)
        return;
    if (obj->cell)
        return;
    int x = (int)((((I3D_bsphere &)obj->bsphere).pos.x - dyn_grid->min_x) / dyn_grid->cell_size_x);
    int y = (int)((((I3D_bsphere &)obj->bsphere).pos.z - dyn_grid->min_z) / dyn_grid->cell_size_z);
    if (x >= dyn_grid->width)
        x = dyn_grid->width - 1;
    if (y >= dyn_grid->height)
        y = dyn_grid->height - 1;
    obj->cell = dyn_grid->cells[x + y * dyn_grid->width];
    if (!obj->cell)
    {
        if (free_cells.size())
        {
            obj->cell = free_cells.back();
            free_cells.pop_back();
        }
        else
        {
            obj->cell = new tDynamicPolicko;
            obj->cell->objects.reserve(8);
        }
        dyn_grid->cells[x + y * dyn_grid->width] = obj->cell;
        obj->cell->x = x;
        obj->cell->y = y;
    }
    obj->cell->objects.push_back(obj);
}

// Takes the object out of its cell; a cell left without objects goes onto the cells to check
// FUNCTION: GAME 0x5c38b0
void g_collision::DynErase(tDynamicCollObject *obj)
{
    if (dyn_grid && obj->cell)
    {
        for (tDynamicCollObject **it = obj->cell->objects.begin(); it < obj->cell->objects.end(); it++)
        {
            if (*it == obj)
            {
                obj->cell->objects.erase(it);
                if (!obj->cell->objects.size())
                    dyn_queue.push_back(obj->cell);
                break;
            }
        }
        obj->cell = NULL;
    }
}

// The object moved into the cell of its sphere's position, where that isn't the one it is in
// FUNCTION: GAME 0x5c3ac0
void g_collision::DynUpdate(tDynamicCollObject *obj)
{
    if (!dyn_grid)
        return;
    int x = (int)((((I3D_bsphere &)obj->bsphere).pos.x - dyn_grid->min_x) / dyn_grid->cell_size_x);
    int y = (int)((((I3D_bsphere &)obj->bsphere).pos.z - dyn_grid->min_z) / dyn_grid->cell_size_z);
    if (x >= dyn_grid->width)
        x = dyn_grid->width - 1;
    if (y >= dyn_grid->height)
        y = dyn_grid->height - 1;
    tDynamicPolicko *cell = dyn_grid->cells[x + y * dyn_grid->width];
    if (cell == obj->cell)
        return;
    if (obj->cell)
    {
        for (tDynamicCollObject **it = obj->cell->objects.begin(); it < obj->cell->objects.end(); it++)
        {
            if (*it == obj)
            {
                obj->cell->objects.erase(it);
                if (!obj->cell->objects.size())
                    dyn_queue.push_back(obj->cell);
                break;
            }
        }
    }
    if (!cell)
    {
        if (free_cells.size())
        {
            cell = free_cells.back();
            free_cells.pop_back();
        }
        else
        {
            cell = new tDynamicPolicko;
            cell->objects.reserve(8);
        }
        dyn_grid->cells[x + y * dyn_grid->width] = cell;
        cell->x = x;
        cell->y = y;
    }
    cell->objects.push_back(obj);
    obj->cell = cell;
}

// The one object the next test leaves out, or none, and the callback it asks
// FUNCTION: GAME 0x5c3d10
void g_collision::DynSetNextCollOwner(tDynamicCollObject *obj, void *callback)
{
    if (obj)
    {
        num_next_coll_owners = 1;
        next_coll_owners = &next_coll_owner;
        next_coll_owner = obj;
    }
    else
        num_next_coll_owners = 0;
    next_coll_callback = callback;
}

// The objects the next test leaves out, num of them, and the callback it asks
// FUNCTION: GAME 0x5c3d60
void g_collision::DynSetNextCollOwnerMult(tDynamicCollObject **objs, uint num, void *callback)
{
    next_coll_owners = objs;
    if (objs)
        num_next_coll_owners = num;
    else
        num_next_coll_owners = 0;
    next_coll_callback = callback;
}

// The grid's cells with objects emptied and taken out onto the cells to check, then every cell of the two lists freed
// once (PS2's also clears each object's cell and a flag of the next owners', and keeps the cells)
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

// Takes the object out of the grid into the vector (a vehicle's, whose collisions a human in it is tested against)
// FUNCTION: GAME 0x5c4020
void g_collision::DynGetIn(tDynamicCollObject *obj, std::vector<tDynamicCollObject *> *colls)
{
    if (obj->cell)
        DynErase(obj);
    colls->push_back(obj);
}

// Takes the object out of the vector and puts it back into the grid
// FUNCTION: GAME 0x5c41d0
void g_collision::DynGetOut(tDynamicCollObject *obj, std::vector<tDynamicCollObject *> *colls)
{
    for (tDynamicCollObject **it = colls->begin(); it < colls->end(); it++)
    {
        if (*it == obj)
        {
            colls->erase(it);
            break;
        }
    }
    DynInsert(obj);
}

// The moving objects in the grid's cells under the box and a cell around it, but those with one of the box's flags
// and the next test's owners: those of type 0x41 by their box, the spheres by the distance to the box's center
// FUNCTION: GAME 0x5c4230
bool g_collision::BoxInDynamic(tCBoxParam &p)
{
    if (!dyn_grid)
        return false;
    int x0 = (int)((p.bbox.min.x - dyn_grid->min_x) / dyn_grid->cell_size_x - 1.0f);
    int x1 = (int)((p.bbox.max.x - dyn_grid->min_x) / dyn_grid->cell_size_x + 2.0f);
    int z0 = (int)((p.bbox.min.z - dyn_grid->min_z) / dyn_grid->cell_size_z - 1.0f);
    int z1 = (int)((p.bbox.max.z - dyn_grid->min_z) / dyn_grid->cell_size_z + 2.0f);
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
                tDynamicCollObject *obj = *it;
                if (obj->flags & p.flags)
                    continue;
                bool owner = false;
                for (uint i = num_next_coll_owners; i--;)
                {
                    if (obj == next_coll_owners[i])
                    {
                        owner = true;
                        break;
                    }
                }
                if (owner)
                    continue;
                switch (obj->type & 0xff)
                {
                case DYNCOLL_UNKNOWN_41:
                {
                    const I3D_bbox &bb = (I3D_bbox &)obj->bbox;
                    if (bb.min.x > p.bbox.max.x)
                        break;
                    if (bb.min.y > p.bbox.max.y)
                        break;
                    if (bb.min.z > p.bbox.max.z)
                        break;
                    if (bb.max.x < p.bbox.min.x)
                        break;
                    if (bb.max.y < p.bbox.min.y)
                        break;
                    if (bb.max.z < p.bbox.min.z)
                        break;
                    return true;
                }
                case DYNCOLL_UNKNOWN_40:
                case DYNCOLL_UNKNOWN_42:
                case DYNCOLL_UNKNOWN_44:
                {
                    const I3D_bsphere &bs = (I3D_bsphere &)obj->bsphere;
                    S_vector d = p.center - bs.pos;
                    float r = bs.radius + p.radius;
                    if (d.x * d.x + d.y * d.y + d.z * d.z > r * r)
                        break;
                    return true;
                }
                }
            }
        }
    }
    return false;
}

// A volume that isn't a face against the box: the two boxes by their bounds, the oriented one by the box's center in
// its space (its height from 0) and the cylinder and the sphere by the distance to the box's center
// FUNCTION: GAME 0x5c44b0
bool g_collision::BoxInNonFace(g_volume_triface *vol, tCBoxParam &p)
{
    switch (vol->flags & 0xff)
    {
    case VOLTYPE_XTOBB:
    case VOLTYPE_AABB:
    {
        g_volume_aabb *box = (g_volume_aabb *)vol;
        if (box->bbox.min.x < p.bbox.max.x && box->bbox.max.x > p.bbox.min.x && box->bbox.min.z < p.bbox.max.z &&
            box->bbox.max.z > p.bbox.min.z && box->bbox.min.y < p.bbox.max.y && box->bbox.max.y > p.bbox.min.y)
            return true;
        return false;
    }
    case VOLTYPE_OBB:
    {
        g_volume_obb *obb = (g_volume_obb *)vol;
        S_vector c = p.center * obb->inv_matrix;
        float r = p.radius;
        if (c.x + r > obb->extents.min.x && c.x - r < obb->extents.max.x && c.z + r > obb->extents.min.z &&
            c.z - r < obb->extents.max.z && c.y + r > 0.0f && c.y - r < 0.0f)
            return true;
        return false;
    }
    case VOLTYPE_CYLINDER:
    {
        g_volume_cylinder *cyl = (g_volume_cylinder *)vol;
        S_vector v(cyl->x, p.center.y, cyl->z);
        S_vector d = v - p.center;
        float r = cyl->radius + p.radius;
        if (d.Square() < r * r)
            return true;
        return false;
    }
    case VOLTYPE_SPHERE:
    {
        g_volume_sphere *sph = (g_volume_sphere *)vol;
        S_vector d = sph->pos - p.center;
        float r = sph->radius + p.radius;
        if (d.Square() < r * r)
            return true;
        return false;
    }
    }
    return false;
}

// A cell's volumes against the box: a face by its bounds and its plane's distance from center2 (its vertices in the
// order of their x, and so the cell's faces: a face past the box ends the search), the others through BoxInNonFace
// FUNCTION: GAME 0x5c46c0
bool g_collision::BoxInTreeLevel(tCollTreeLevel *cell, tCBoxParam &p)
{
    for (uint i = 0; i < cell->num_volumes; i++)
    {
        g_volume_triface *vol = cell->volumes[i];
        if (vol->flags & (VOLF_NONFACE | VOLF_DISABLED))
        {
            if (!(vol->flags & VOLF_DISABLED) && BoxInNonFace(vol, p))
                return true;
            continue;
        }
        if (vol->vertices[2]->x < p.bbox.min.x)
            continue;
        if (vol->vertices[0]->x > p.bbox.max.x)
            return false;
        float lo = vol->vertices[0]->y, hi = lo;
        float v = vol->vertices[2]->y;
        if (v < lo)
            lo = v;
        else if (v > hi)
            hi = v;
        v = vol->vertices[1]->y;
        if (v < lo)
            lo = v;
        else if (v > hi)
            hi = v;
        if (lo > p.bbox.max.y)
            continue;
        if (hi < p.bbox.min.y)
            continue;
        lo = vol->vertices[0]->z;
        hi = lo;
        v = vol->vertices[2]->z;
        if (v < lo)
            lo = v;
        else if (v > hi)
            hi = v;
        v = vol->vertices[1]->z;
        if (v < lo)
            lo = v;
        else if (v > hi)
            hi = v;
        if (lo > p.bbox.max.z)
            continue;
        if (hi < p.bbox.min.z)
            continue;
        float d = (float)fabs((p.center2 - *vol->vertices[0]).Dot(vol->normal));
        if (d * d > p.radius2)
            continue;
        return true;
    }
    return false;
}

// The box's eight corners into c (the name is ours)
static inline void GetCorners(const I3D_bbox &bbox, S_vector *c)
{
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            c->x = i ? bbox.min.x : bbox.max.x;
            c->y = j ? bbox.min.y : bbox.max.y;
            c->z = bbox.min.z;
            c++;
            c->x = i ? bbox.min.x : bbox.max.x;
            c->y = j ? bbox.min.y : bbox.max.y;
            c->z = bbox.max.z;
            c++;
        }
    }
}

// The box at both matrices: its corners' bounds, its center at the second matrix and that center moved along v2,
// against the moving objects (unless flags is COLF_ALL) and the level's cells under the box (unless flags2 is)
// FUNCTION: GAME 0x5c48b0
bool g_collision::_TestDBox(const I3D_bbox &bbox, const S_matrix &m0, const S_matrix &m1, const S_vector &dir,
    const S_vector &v1, const S_vector &v2, uint flags, uint flags2)
{
    if (!level)
        return false;
    tCBoxParam p;
    p.flags = flags;
    p.flags2 = flags2;
    // Axes along the movement, normalized, which nothing reads (PS2's the same; the original keeps only the
    // normalizations' first fabs)
    S_vector e0 = dir;
    S_vector h = (v1 + v2) * 0.5f;
    S_vector e1 = dir.Cross(h);
    S_vector e2 = e1.Cross(e0);
    e0.Normalize();
    e1.Normalize();
    e2.Normalize();
    S_vector corners[8];
    int k;
    GetCorners(bbox, corners);
    corners[0] *= m0;
    p.bbox.min = corners[0];
    p.bbox.max = corners[0];
    for (k = 1; k < 8; k++)
    {
        corners[k] *= m0;
        MinMax(corners[k].x, p.bbox.min.x, p.bbox.max.x);
        MinMax(corners[k].z, p.bbox.min.z, p.bbox.max.z);
        MinMax(corners[k].y, p.bbox.min.y, p.bbox.max.y);
    }
    GetCorners(bbox, corners);
    for (k = 0; k < 8; k++)
    {
        corners[k] *= m1;
        MinMax(corners[k].x, p.bbox.min.x, p.bbox.max.x);
        MinMax(corners[k].z, p.bbox.min.z, p.bbox.max.z);
        MinMax(corners[k].y, p.bbox.min.y, p.bbox.max.y);
    }
    p.center = (bbox.max + bbox.min) * 0.5f * m1;
    p.radius = (p.center - corners[7]).Dot(p.center - corners[7]);
    p.center2 = p.center + v2 * 20.0f;
    p.radius2 = (p.center2 - corners[7]).Dot(p.center2 - corners[7]);
    bool hit = false;
    if (flags != COLF_ALL && BoxInDynamic(p))
        return true;
    if (flags2 == COLF_ALL)
        return false;
    int x0 = (int)((p.bbox.min.x - level->min_x) / level->cell_size_x);
    int x1 = (int)((p.bbox.max.x - level->min_x) / level->cell_size_x + 1.0f);
    int z0 = (int)((p.bbox.min.z - level->min_z) / level->cell_size_z);
    int z1 = (int)((p.bbox.max.z - level->min_z) / level->cell_size_z + 1.0f);
    if (x0 < 0)
        x0 = 0;
    if (z0 < 0)
        z0 = 0;
    if (x1 > level->width)
        x1 = level->width;
    if (z1 > level->height)
        z1 = level->height;
    for (k = z0; k < z1; k++)
    {
        for (int x = x0; x < x1; x++)
        {
            tCollTreeLevel *cell = level->cells[x + k * level->width];
            if (cell->max_y > p.bbox.min.y)
                hit |= BoxInTreeLevel(cell, p);
        }
    }
    if (!hit)
        return false;
    return true;
}
