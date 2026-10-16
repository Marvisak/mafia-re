// The rendering (PS2 render.cpp), at 0x100619b0-0x10065a10 in the original: I3D_driver::Render and what it calls. Only
// GetObjectLOD is written yet. Names are the PS2 build's unless noted.
#include "I3D/I3D.h"
#include "IGraph/IGraph.h"

// GLOBAL: LS3DF 0x101c4cf4
I3D_scene *pCurrentScene;

// The camera rendered with
// GLOBAL: LS3DF 0x101c4cf8
I3D_camera *pCurrentCamera;

// GLOBAL: LS3DF 0x101c4c60
ADD_FG_HELP_STRUCT AddFGHlpStruct;

// GLOBAL: LS3DF 0x101c4ce0
bool renderCameraChanged;

// GLOBAL: LS3DF 0x101c4cd0
S_matrix *pViewProjMatrix;

// RS_FLOAT_LOD scaled by the current camera's field of view, which the levels' distances are compared with (Render
// sets it)
// GLOBAL: LS3DF 0x101c4ca4
float RSLodQuality;

// The level of an object's mesh to render at the squared distance dist_2: the first whose distance (scaled by
// RSLodQuality and the object's scale) is as far or farther, or one past the last. The frame's callback is told of a
// change. With VISF_UNKNOWN_10 (PC only) a level whose first texture isn't loaded yet gives way to one whose is, the
// level is only stored, and the result is 0.
// FUNCTION: LS3DF 0x10061e30
int GetObjectLOD(I3D_object *obj, const float &dist_2)
{
    I3D_mesh_object *mesh = obj->mesh;
    int lod;
    int num = mesh->num_lods;
    if (num == 1) {
        I3D_mesh_level *level = mesh->levels[0];
        if (!level)
            return 0;
        if (level->lod_dist <= 0.0f)
            return 0;
        float s = RSLodQuality * obj->world_scale;
        if (dist_2 / (s * s) > level->lod_dist)
            lod = 1;
        else
            lod = 0;
    } else {
        float s = RSLodQuality * obj->world_scale;
        float d = dist_2 / (s * s);
        num--;
        for (lod = 0; lod < num; lod++) {
            I3D_mesh_level *level = mesh->levels[lod];
            if (level && d <= level->lod_dist)
                goto found;
        }
        {
            I3D_mesh_level *level = mesh->levels[lod];
            if (level && level->lod_dist > 0.0f && d > level->lod_dist)
                lod = num + 1;
        }
    found:;
    }

    if (lod != obj->cur_lod && obj->frm_flags & LS3D_CALLBACK_LOD)
        obj->callback(obj, LS3D_CALLBACK_LOD, obj->cur_lod, lod);
    if (obj->vis_flags & VISF_UNKNOWN_10) {
        int last = mesh->num_lods - 1;
        if (lod <= last) {
            if (renderState[RS_LOD_COARSER] && lod < last)
                lod++;
            ITexture *tex = mesh->levels[lod]->fgroups->material->GetTexture();
            if (tex && !tex->Request()) {
                int i;
                for (i = 0; i < last; i++) {
                    if (i == lod)
                        continue;
                    if (mesh->levels[i]->fgroups->material->GetTexture()->flags & ITC_LOADED) {
                        lod = i;
                        break;
                    }
                }
                if (i >= last)
                    lod = last;
            }
            if (obj->cur_lod != lod) {
                obj->cur_lod = lod;
                obj->vis_flags |= VISF_LOD_CHANGED;
            }
            return 0;
        }
    } else
        obj->cur_lod = lod;
    return lod;
}
