// The rendering (PS2 render.cpp), at 0x10061cd0-0x10065a10 in the original: I3D_driver::Render and what it calls,
// with the scene's sound environment and the areas' sounds, which the PC updates there too. Names are the PS2 build's
// unless noted. The radix sort before it, which PS2 has at the start of this file's code, is a file of its own on PC
// (RadixSort.cpp).
// Open: the zero vector's $E comes right after Render, the original's after NextFFrameProc (0x100658d0). 70 to 89 more
// small inline functions before the vector in the headers put it there (tools/autopch.py); 30 or 40 leave it after
// Render, 50 and 60 put it after VisualProc.
#include "I3D/I3D.h"
#include "I3D/render/IBManager.h"
#include "I3D/render/render.h"
#include "I3D/render/VBManager.h"
#include "IGraph/IGraph.h"
#include "ISound/ISound.h"
#include <mmsystem.h>

// I3d_driver.cpp
extern bool RecomputeAllGeometry;

// The globals, in PS2's order. The ones PS2 initializes are zero-initialized here too, which keeps them out of VC6's
// hash: they follow the others in .bss, in this order, where the others are in their names' buckets' order.

// The areas the camera is in (the scene's Close empties it)
// GLOBAL: LS3DF 0x101c4ce4
C_list<I3D_area *> CurrentActiveAreaList;

// The sounds of the areas the camera left, which fade out
// GLOBAL: LS3DF 0x101c4c40
C_list<I3D_sound *> SoundsForDeactivation;

// GLOBAL: LS3DF 0x101c4c60
ADD_FG_HELP_STRUCT AddFGHlpStruct;

// GLOBAL: LS3DF 0x101c4cb0
PARTICLE_HLP_STRUCT ParticleElementsList;

// GLOBAL: LS3DF 0x101c4cf4
I3D_scene *pCurrentScene = NULL;

// The camera rendered with
// GLOBAL: LS3DF 0x101c4cf8
I3D_camera *pCurrentCamera = NULL;

// The sector the camera was in at the last rendering, and the one before it
// GLOBAL: LS3DF 0x101c4cfc
I3D_sector *pPrevLastCameraSector = NULL;

// GLOBAL: LS3DF 0x101c4d00
I3D_sector *pLastCameraSector = NULL;

// GLOBAL: LS3DF 0x101c4cd0
S_matrix *pViewProjMatrix;

// GLOBAL: LS3DF 0x101c4ce0
bool renderCameraChanged;

// A constant divided by the current camera's field of view, which scales the distances, and its square (Render sets
// them)
// GLOBAL: LS3DF 0x101c4ca0
float FOVDistCorrection;

// GLOBAL: LS3DF 0x101c4cd4
float FOVDistCorrection2;

// RS_LOD_QUALITY scaled by the current camera's field of view, which the levels' distances are compared with (Render
// sets it)
// GLOBAL: LS3DF 0x101c4ca4
float RSLodQuality;

// Nothing references the 16 bytes of zero-initialized globals between the sectors and the flags: PS2 declares
// NearFOVFactor there, which the PC doesn't use.
uint unknown_4D04[4] = {0};

// The rendering runs a callback of the scene's (Render may be called from it), and it runs (PS2 names)
// GLOBAL: LS3DF 0x101c4d14
bool renderCB = false;

// GLOBAL: LS3DF 0x101c4d15
bool rendering = false;

// The weather is rendered after the opaque face groups (m_DrawAllSortedFG)
// GLOBAL: LS3DF 0x101c4cd8
bool GlobalDrawWeather;

// RS_LOD_INDEX, copied by Render (PC only, the name is Insanity3D's force_lod_index's); nothing reads it
// GLOBAL: LS3DF 0x101c4cdc
int ForceLodIndex;

// Nothing references these 8 bytes of the hashed globals, between RSLodQuality and ParticleElementsList (the name is
// chosen for the bucket)
uint unknown_4CA8[2];

// The level of an object's mesh to render at the squared distance dist_2: the first whose distance (scaled by
// RSLodQuality and the object's scale) is as far or farther, or one past the last. The frame's callback is told of a
// change. With VISF_UNKNOWN_10 (PC only) a level whose first texture isn't loaded yet gives way to one whose is, the
// level is only stored, and the result is 0.
// The count is decremented in place (num--, then num + 1 for one past the last), and the texture search assigns lod
// inside its loop (then if (i >= last) lod = last;), which lets the exhausted loop jump straight to that assignment.
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

// FUNCTION: LS3DF 0x10062010
LS3D_RESULT I3D_driver::initRender()
{
    closeRender();
    InitParticle();
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10062020
void I3D_driver::closeRender()
{
    CloseParticle();
}

static void VisualProc(I3D_visual *vis);

// Queues the face groups of a sector's visuals that the portals let the camera see this frame, lit by the sector, or by
// the camera's for the near sector (PC only, the name is a guess; PS2 has the same code written out at each place)
inline void AddSectorFGroups(I3D_sector *sct)
{
    AddFGHlpStruct.sector = sct == pCurrentScene->near_sector ? pCurrentCamera->sector : sct;
    for (I3D_visual *vis = sct->next_visual; vis; vis = vis->next_visual) {
        if (vis->render_number == RenderNumber)
            VisualProc(vis);
    }
}

// Renders the scene through its active camera: the sound environment and the areas' sounds first, then the backdrop,
// the mirrors' reflections, the sectors, the landscapes, the particles, the near sector and the lens flares, each
// pass sorting the face groups queued by the visuals (AddFGHlpStruct) and drawing them (m_DrawAllSortedFG). The
// scene's callback is told of the stages in its cb_flags.
// FUNCTION: LS3DF 0x10062030
LS3D_RESULT I3D_driver::Render(I3D_scene *scene)
{
    if (!scene)
        return I3DERR_GENERIC;
    if (!I3DIsInit)
        return I3DERR_NOTINITIALIZED;
    // The FPU's state: its exception summary is reported, and it computes in single precision, rounding to nearest
    uint fpu;
    __asm fnstsw word ptr fpu
    if (fpu & 0x80)
        dbgPrintf("!! FPU exception flag set on entry I3D_driver::Render()  status:0x%x", (ushort)fpu);
    __asm fnstcw word ptr fpu
    fpu &= 0xf0ff;
    __asm fldcw word ptr fpu
    if (rendering && !renderCB) {
        dbgPrintf("!! Rendering is NOT reentrant   Render()");
        return I3DERR_GENERIC;
    }
    rendering = true;
    pCurrentScene = scene;
    RenderNumberMinus1 = scene->render_number;
    RenderNumber = ++scene->render_number;
    pCurrentCamera = scene->current_camera;
    if (!pCurrentCamera) {
        uint clear = renderState[RS_CLEAR] != 0;
        pGlobalIGraph->ResetRenderProps();
        if (renderState[RS_CLEAR_ZBUFFER])
            clear |= ((m_HardwareCaps & HWCAPS_STENCIL) | D3DCLEAR_ZBUFFER << 2) >> 2;
        m_pD3DDevice8->Clear(0, NULL, clear, ((((uint)-1 << 8 | FloatToInt(scene->bg_color.x * 255.0f)) << 8 |
            FloatToInt(scene->bg_color.y * 255.0f)) << 8) | FloatToInt(scene->bg_color.z * 255.0f), 1.0f, 0);
        if (!renderCB)
            rendering = false;
        return I3DERR_GENERIC;
    }
    pCurrentCamera->UpdateSector();
    uint clear;
    if (renderCB)
        clear = 0;
    else {
        clear = renderState[RS_CLEAR] != 0;
        pGlobalIGraph->ResetRenderProps();
    }
    if (renderState[RS_CLEAR_ZBUFFER])
        clear |= ((m_HardwareCaps & HWCAPS_STENCIL) | D3DCLEAR_ZBUFFER << 2) >> 2;
    // The viewport: the scene's position and size
    RECT rc;
    rc.left = scene->viewport.left;
    rc.top = scene->viewport.top;
    rc.right = scene->viewport.right - scene->viewport.left;
    rc.bottom = scene->viewport.bottom - scene->viewport.top;
    if (!EqualRect(&rc, (RECT *)&m_D3DViewPort8)) {
        CopyRect((RECT *)&m_D3DViewPort8, &rc);
        if (FAILED(m_pD3DDevice8->SetViewport(&m_D3DViewPort8))) {
            if (!renderCB)
                rendering = false;
            return I3DERR_NOTINITIALIZED;
        }
    }
    if (clear)
        m_pD3DDevice8->Clear(0, NULL, clear, ((((uint)-1 << 8 | FloatToInt(scene->bg_color.x * 255.0f)) << 8 |
            FloatToInt(scene->bg_color.y * 255.0f)) << 8) | FloatToInt(scene->bg_color.z * 255.0f), 1.0f, 0);
    if (!renderCB) {
        if (FAILED(m_pD3DDevice8->BeginScene())) {
            if (!renderCB)
                rendering = false;
            return I3DERR_GENERIC;
        }
        if (scene->cb_flags & 1 << SCN_CALLBACK_PRE_RENDER) {
            I3D_camera *cam = pCurrentCamera;
            renderCB = true;
            scene->callback(SCN_CALLBACK_PRE_RENDER, 0, 0);
            pCurrentCamera = cam;
            renderCB = false;
            pGlobalIGraph->ResetRenderProps();
        }
    }

    FOVDistCorrection = PI / 2 / pCurrentCamera->fov_a;
    FOVDistCorrection2 = FOVDistCorrection * FOVDistCorrection;
    ForceLodIndex = renderState[RS_LOD_INDEX];
    // The state holds a float's bits: stored as they are and read back as the float (I3DIntAsFloat goes through a
    // temporary, a float read of the state leaves the store dead)
    *(int *)&RSLodQuality = renderState[RS_LOD_QUALITY];
    RSLodQuality *= FOVDistCorrection;
    // The camera moved, or its sector did
    renderCameraChanged = pCurrentCamera->update_count != pCurrentCamera->unknown_3fc ||
        pCurrentCamera->sector->update_count != pCurrentCamera->unknown_400 || RecomputeAllGeometry;
    RecomputeAllGeometry = false;
    if (renderCameraChanged) {
        pCurrentCamera->unknown_3fc = pCurrentCamera->update_count;
        pCurrentCamera->unknown_400 = pCurrentCamera->sector->update_count;
        if (!renderCB && scene->listener) {
            scene->listener->SetPos(pCurrentCamera->GetWorldPos());
            scene->listener->SetDir(pCurrentCamera->GetMatrix()(2), pCurrentCamera->GetMatrix()(1));
            scene->listener->Update();
        }
    }

    if (!renderCB) {
        uint time = timeGetTime();
        // The areas the camera entered and left: the sounds of those it is in fade in, the others' fade out
        if (scene->areas) {
            S_vector cam_pos = pCurrentCamera->GetWorldPos();
            I3D_area *area = scene->areas;
            int i = scene->num_areas;
            while (i--) {
                if (renderCameraChanged || area->update_count != area->unknown_164) {
                    area->unknown_164 = area->update_count;
                    if (area->CheckPoint(cam_pos))
                        CurrentActiveAreaList.push_back_unique(area);
                    else {
                        CurrentActiveAreaList.remove(area);
                        for (int j = area->sounds.size(); j--; ) {
                            I3D_sound *snd = area->sounds[j];
                            if (snd->state != SOUNDSTATE_PAUSE && !(snd->snd_flags & SOUNDF_FADE_OUT))
                                SoundsForDeactivation.push_back(snd);
                        }
                    }
                }
                area = area->next_in_scene;
            }
            I3D_area **pa = &CurrentActiveAreaList[0];
            for (int k = CurrentActiveAreaList.size(); k--; pa++) {
                for (int j = (*pa)->sounds.size(); j--; ) {
                    I3D_sound *snd = (*pa)->sounds[j];
                    SoundsForDeactivation.remove(snd);
                    if (!(snd->snd_flags & SOUNDF_FADE_IN)) {
                        if (snd->state == SOUNDSTATE_PAUSE)
                            snd->m_SetState(SOUNDSTATE_RESUME);
                        snd->snd_flags = (snd->snd_flags & ~SOUNDF_FADE_OUT) | SOUNDF_FADE_IN;
                        snd->fade_time = time;
                        if (snd->fade < 0.0f) {
                            snd->fade = 0.0f;
                            snd->fade_length = (*pa)->switch_fade_time;
                        }
                    }
                }
            }
            for (int n = SoundsForDeactivation.size(); n--; ) {
                I3D_sound *snd = SoundsForDeactivation[n];
                snd->fade_time = time;
                if (snd->snd_flags & SOUNDF_FADE_IN) {
                    snd->snd_flags = (snd->snd_flags & ~SOUNDF_FADE_IN) | SOUNDF_FADE_OUT;
                    if (snd->fade < 0.0f) {
                        snd->fade = 1.0f;
                        snd->fade_length = area->switch_fade_time;
                    }
                } else {
                    snd->snd_flags |= SOUNDF_FADE_OUT;
                    snd->fade = 1.0f;
                    snd->fade_length = area->switch_fade_time;
                }
            }
            SoundsForDeactivation.reset();
        }

        // The sector the camera is in: the sound environment blends from the sector it left into it, and the sounds
        // of the sector it left that the new one doesn't have follow the old one's blend
        if (!pLastCameraSector && pCurrentCamera->sector) {
            pLastCameraSector = pCurrentCamera->sector;
            pLastCameraSector->sct_flags &= ~SCTF_SOUND_FADE_IN;
            pLastCameraSector->unknown_1ec = time;
            if (!sectors.find(pLastCameraSector))
                sectors.push_back(pLastCameraSector);
            for (int i = pLastCameraSector->sounds.size(); i--; ) {
                I3D_sound *snd = pLastCameraSector->sounds[i];
                if (!(snd->snd_flags & SOUNDF_CAMERA_SECTOR)) {
                    snd->fade_sector = pLastCameraSector;
                    if (snd->state == SOUNDSTATE_PAUSE)
                        snd->m_SetState(SOUNDSTATE_RESUME);
                    snd->snd_flags |= SOUNDF_CAMERA_SECTOR;
                }
            }
        } else if (pCurrentCamera->sector != pLastCameraSector) {
            pPrevLastCameraSector = pLastCameraSector;
            for (int i = pLastCameraSector->sounds.size(); i--; ) {
                I3D_sound *snd = pLastCameraSector->sounds[i];
                if (snd->snd_flags & SOUNDF_CAMERA_SECTOR && !pCurrentCamera->sector->sounds.find(snd)) {
                    snd->fade_sector = pLastCameraSector;
                    snd->snd_flags &= ~SOUNDF_CAMERA_SECTOR;
                }
            }
            pLastCameraSector->sct_flags |= SCTF_SOUND_FADE_IN;
            pLastCameraSector->unknown_1ec = time;
            if (!sectors.find(pLastCameraSector))
                sectors.push_back(pLastCameraSector);
            pLastCameraSector = pCurrentCamera->sector;
            pLastCameraSector->sct_flags &= ~SCTF_SOUND_FADE_IN;
            pLastCameraSector->unknown_1ec = time;
            if (!sectors.find(pLastCameraSector))
                sectors.push_back(pLastCameraSector);
            // The first loop's counter again (VC6's for scope), as below for the sectors' blends: the original's frame
            // has one slot for the two loops' counters, area's
            for (i = 0; i < pLastCameraSector->sounds.size(); i++) {
                I3D_sound *snd = pLastCameraSector->sounds[i];
                if (!(snd->snd_flags & SOUNDF_CAMERA_SECTOR)) {
                    snd->fade_sector = pLastCameraSector;
                    if (snd->state == SOUNDSTATE_PAUSE)
                        snd->m_SetState(SOUNDSTATE_RESUME);
                    snd->snd_flags |= SOUNDF_CAMERA_SECTOR;
                }
            }
        }

        for (int i = sounds.size(); i--; ) {
            I3D_sound *snd = sounds[i];
            if (snd->frm_flags & FRMFLAGS_ON && snd->state != SOUNDSTATE_STOP) {
                if (snd->update_count != snd->unknown_14c) {
                    snd->update_count = snd->unknown_14c;
                    snd->snd_flags |= SOUNDF_POS | SOUNDF_DIR | SOUNDF_SCALE;
                    snd->m_UpdateSound();
                } else if (renderCameraChanged || snd->fade_sector || snd->snd_flags > 0)
                    snd->m_UpdateSound();
            }
        }

        // The sectors' blends into and out of their sound environments, over their blend times
        for (i = 0; i < sectors.size(); i++) {
            I3D_sector *sct = sectors[i];
            int dt = time - sct->unknown_1ec;
            if (dt > 0) {
                sct->unknown_1ec = time;
                // The reciprocal first, then times dt (fdivr, fimul): without the parentheses VC6 divides dt instead
                float step = (1.0 / sct->sound_props.blend_time) * dt;
                if (sct->sct_flags & SCTF_SOUND_FADE_IN) {
                    if (IsMrgZeroLess(sct->unknown_1e8)) {
                        sectors.erase(i);
                        i--;
                        continue;
                    }
                    sct->unknown_1e8 = max(sct->unknown_1e8 - step, 0.0);
                } else {
                    if (IsMrgZeroLess(1.0 - sct->unknown_1e8)) {
                        sectors.erase(i);
                        i--;
                        continue;
                    }
                    sct->unknown_1e8 = min(sct->unknown_1e8 + step, 1.0);
                }
                if (sct == pLastCameraSector && pPrevLastCameraSector)
                    scene->m_UpdateSoundProperties(sct, pPrevLastCameraSector, sct->unknown_1e8);
            }
        }
        if (pCurrentCamera->sector->sct_flags & SCTF_SOUND_CHANGED)
            scene->m_UpdateSoundProperties(pCurrentCamera->sector, NULL, 1.0f);

        if (m_HardwareCaps & HWCAPS_HWTL) {
            I3D_mirror **pm = &scene->mirrors[0];
            for (int i = scene->mirrors.size(); i--; pm++)
                (*pm)->Tick(0);
        }
    }

    ParticleElementsList.Clear();
    AddFGHlpStruct.Clear();
    AddFGHlpStruct.camera = pCurrentCamera;
    AddFGHlpStruct.cam_pos = pCurrentCamera->GetWorldPos();
    AddFGHlpStruct.scene = scene;
    AddFGHlpStruct.sector = NULL;
    if (renderState[RS_USESHADOWS]) {
        I3D_shadow **ps = &scene->shadows[0];
        for (int i = scene->shadows.size(); i--; ps++)
            (*ps)->Prepare();
    }
    pViewProjMatrix = &pCurrentCamera->m_view_proj;
    ProcessPortals(scene);
    ProcessOccluders(scene);

    if (!renderCB && renderState[RS_DRAWVISUALS]) {
        // The backdrop, behind everything, sorted far to near from the camera
        if (scene->primary_sector->sct_flags & SCTF_VISIBLE) {
            pViewProjMatrix = &pCurrentCamera->m_bd_view_proj;
            scene->SetViewProjMatrix(*pViewProjMatrix);
            m_pD3DDevice8->SetTransform(D3DTS_VIEW, &pCurrentCamera->m_bd_view);
            m_pD3DDevice8->SetTransform(D3DTS_PROJECTION, &pCurrentCamera->m_bd_proj);
            AddSectorFGroups(scene->backdrop_sector);
            // Each sort reads the count once, for the reserve and the loop: the original doesn't load it again after
            // the reserve's calls. Dot, not Square: Square sums the squares in another order
            int i = AddFGHlpStruct.fgroups.size();
            AddFGHlpStruct.sort.Reserve(i);
            while (i--) {
                S_vector pos = AddFGHlpStruct.fgroups[i].vis->GetWBoundVolume().bsphere.pos;
                AddFGHlpStruct.sort.Add(i, -FloatToInt(pos.Dot(pos) * 65536.0f));
            }
            AddFGHlpStruct.sort.Sort();
            cVBMan.m_UnlockAllVBuffers();
            cIBMan.m_UnlockAllIBuffers();
            m_pD3DDevice8->SetRenderState(D3DRS_ZENABLE, FALSE);
            m_DrawAllSortedFG();
            AddFGHlpStruct.Clear();
            ParticleElementsList.Clear();
        }
        if (renderState[RS_USEZB]) {
            if (pCurrentCamera->orthogonal)
                m_pD3DDevice8->SetRenderState(D3DRS_ZENABLE, TRUE);
            else
                m_pD3DDevice8->SetRenderState(D3DRS_ZENABLE, ZBufferEnable);
        }

        // The mirrors' reflections, each through its mirror's faces, which are drawn after them. The count is read once:
        // the original keeps it across the setup's calls and its loop has no guard of its own
        int n = scene->mirrors.size();
        if (n) {
            bool visible = false;
            pViewProjMatrix = &pCurrentCamera->m_view_proj;
            scene->SetViewProjMatrix(*pViewProjMatrix);
            m_pD3DDevice8->SetTransform(D3DTS_VIEW, &pCurrentCamera->m_view);
            m_pD3DDevice8->SetTransform(D3DTS_PROJECTION, &pCurrentCamera->m_proj);
            I3D_mirror **pm = &scene->mirrors[0];
            for (int i = n; i--; pm++) {
                (*pm)->UpdateVisibility();
                if ((*pm)->mirror_flags & MIRF_VISIBLE) {
                    visible = true;
                    if ((*pm)->mirror_flags & MIRF_REFLECT && (*pm)->PrepareReflection()) {
                        m_CullMode = D3DCULL_CW;
                        SetCullState(D3DCULL_NONE);
                        cVBMan.m_UnlockAllVBuffers();
                        cIBMan.m_UnlockAllIBuffers();
                        int j = AddFGHlpStruct.fgroups.size();
                        AddFGHlpStruct.sort.Reserve(j);
                        while (j--)
                            AddFGHlpStruct.sort.Add(j, AddFGHlpStruct.fgroups[j].sort_value);
                        AddFGHlpStruct.sort.Sort();
                        m_DrawAllSortedFG();
                        ParticleElementsList.Clear();
                        AddFGHlpStruct.Clear();
                    }
                }
            }
            m_CullMode = D3DCULL_CCW;
            SetCullState(D3DCULL_NONE);
            if (visible) {
                m_pD3DDevice8->Clear(0, NULL, ((m_HardwareCaps & HWCAPS_STENCIL) | D3DCLEAR_ZBUFFER << 2) >> 2, 0, 1.0f,
                    0);
                m_pD3DDevice8->SetRenderState(D3DRS_ALPHAREF, 0);
                I3D_mirror **pm2 = &scene->mirrors[0];
                for (int j = scene->mirrors.size(); j--; pm2++) {
                    if ((*pm2)->mirror_flags & MIRF_VISIBLE) {
                        if ((*pm2)->mirror_flags & MIRF_REFLECT)
                            (*pm2)->Draw(ZeroSVector, 0xfe);
                        else
                            (*pm2)->Draw((*pm2)->color, 0);
                    }
                }
                m_pD3DDevice8->SetRenderState(D3DRS_ALPHAREF, 1);
                SetAlphaState(1);
            }
        }
    }

    // The scene
    pViewProjMatrix = &pCurrentCamera->m_view_proj;
    scene->SetViewProjMatrix(*pViewProjMatrix);
    m_pD3DDevice8->SetTransform(D3DTS_VIEW, &pCurrentCamera->m_view);
    m_pD3DDevice8->SetTransform(D3DTS_PROJECTION, &pCurrentCamera->m_proj);
    if (renderState[RS_USEZB]) {
        if (pCurrentCamera->orthogonal)
            m_pD3DDevice8->SetRenderState(D3DRS_ZENABLE, TRUE);
        else
            m_pD3DDevice8->SetRenderState(D3DRS_ZENABLE, ZBufferEnable);
    }
    if (renderState[RS_DRAWVISUALS]) {
        I3D_sector *sct = scene->sectors;
        int i = scene->num_sectors;
        while (i--) {
            if (sct->sct_flags & SCTF_VISIBLE)
                AddSectorFGroups(sct);
            sct = sct->next_in_scene;
        }
        if (scene->primary_sector->sct_flags & SCTF_VISIBLE)
            AddSectorFGroups(scene->primary_sector);
        AddSectorFGroups(scene->omni_sector);
    }
    if (renderState[RS_USEPROJECTORS])
        ProcessProjectors();
    if (renderState[RS_USESHADOWS])
        ProcessShadows();
    // The landscapes, in the primary sector's fog
    if (landscapes.size() > 0) {
        I3D_light *fog = scene->primary_sector->fog_light;
        if (fog && renderState[RS_FOG]) {
            switch (fog->ltype) {
            case I3DLIGHT_FOG: {
                float end = pCurrentCamera->fcp * fog->range_f;
                float start = end * fog->range_n;
                SetFog(TRUE, 0, start, end, ((((uint)-1 << 8 | FloatToInt(fog->diffuse.x * 255.0f)) << 8 |
                    FloatToInt(fog->diffuse.y * 255.0f)) << 8) | FloatToInt(fog->diffuse.z * 255.0f));
            } break;
            case I3DLIGHT_POINTAMBIENT:
                SetFog(TRUE, 1, fog->near_range, 0.0f, ((((uint)-1 << 8 | FloatToInt(fog->diffuse.x * 255.0f)) << 8 |
                    FloatToInt(fog->diffuse.y * 255.0f)) << 8) | FloatToInt(fog->diffuse.z * 255.0f));
                break;
            case I3DLIGHT_POINTFOG:
                SetFog(TRUE, 2, fog->near_range, 0.0f, ((((uint)-1 << 8 | FloatToInt(fog->diffuse.x * 255.0f)) << 8 |
                    FloatToInt(fog->diffuse.y * 255.0f)) << 8) | FloatToInt(fog->diffuse.z * 255.0f));
                break;
            }
        }
        for (int j = landscapes.size(); j--; ) {
            landscapes[j]->Reset();
            landscapes[j]->Tessellate();
            landscapes[j]->Render();
        }
    }
    scene->m_UpdateWeatherSystem();
    cVBMan.m_UnlockAllVBuffers();
    cIBMan.m_UnlockAllIBuffers();
    pParticleVisual->m_AddFGroups();
    int k = AddFGHlpStruct.fgroups.size();
    AddFGHlpStruct.sort.Reserve(k);
    while (k--)
        AddFGHlpStruct.sort.Add(k, AddFGHlpStruct.fgroups[k].sort_value);
    AddFGHlpStruct.sort.Sort();
    GlobalDrawWeather = true;
    m_DrawAllSortedFG();
    GlobalDrawWeather = false;

    if (!renderCB) {
        SetAlphaOp(D3DTOP_MODULATE);
        SetColorOp(D3DTOP_MODULATE);
        SetColorOp1(D3DTOP_DISABLE);
        SetAlphaState(1);
        SetBlendState(0);
        SetClipping(TRUE);
        if (scene->cb_flags & 1 << SCN_CALLBACK_POST_SCENE) {
            I3D_camera *cam = pCurrentCamera;
            renderCB = true;
            SetZBias(0);
            scene->callback(SCN_CALLBACK_POST_SCENE, 0, 0);
            pCurrentCamera = cam;
            pCurrentScene = scene;
            renderCB = false;
        }

        // The near sector, in front of everything, with the near projection
        if (scene->near_sector->child && renderState[RS_DRAWVISUALS]) {
            AddFGHlpStruct.Clear();
            ParticleElementsList.Clear();
            m_pD3DDevice8->Clear(0, NULL, ((m_HardwareCaps & HWCAPS_STENCIL) | D3DCLEAR_ZBUFFER << 2) >> 2, 0, 1.0f, 0);
            pViewProjMatrix = &pCurrentCamera->m_near_view_proj;
            scene->SetViewProjMatrix(*pViewProjMatrix);
            m_pD3DDevice8->SetTransform(D3DTS_PROJECTION, &pCurrentCamera->m_near_proj);
            AddSectorFGroups(scene->near_sector);
            // The scene's sort's counter again: a new one swaps the registers of the sort keys' address
            k = AddFGHlpStruct.fgroups.size();
            AddFGHlpStruct.sort.Reserve(k);
            while (k--)
                AddFGHlpStruct.sort.Add(k, AddFGHlpStruct.fgroups[k].sort_value);
            AddFGHlpStruct.sort.Sort();
            cVBMan.m_UnlockAllVBuffers();
            cIBMan.m_UnlockAllIBuffers();
            m_DrawAllSortedFG();
            pViewProjMatrix = &pCurrentCamera->m_view_proj;
            scene->SetViewProjMatrix(*pViewProjMatrix);
            m_pD3DDevice8->SetTransform(D3DTS_PROJECTION, &pCurrentCamera->m_proj);
        }

        // The lens flares the visuals met, which fade out once they aren't met again
        if (scene->lensflares.size()) {
            m_pD3DDevice8->SetRenderState(D3DRS_ZENABLE, FALSE);
            SetColorOp(D3DTOP_MODULATE);
            SetAlphaOp(D3DTOP_MODULATE);
            SetColorOp1(D3DTOP_DISABLE);
            SetAlphaState(1);
            SetBlendState(1);
            SetFog(FALSE, 0, 0.0f, 0.0f, 0);
            SetClipping(TRUE);
            for (int i = 0; i < scene->lensflares.size(); i++) {
                I3D_lensflare *lf = scene->lensflares[i];
                if (lf->unknown_224 & 1) {
                    lf->Draw(true);
                    lf->unknown_224 &= ~1;
                } else {
                    lf->Draw(false);
                    if (IsAbsMrgZero(lf->visibility)) {
                        lf->Release();
                        scene->lensflares.erase(i);
                        i--;
                    }
                }
            }
        }
    }
    if (!renderCB) {
        if (scene->cb_flags & 1 << SCN_CALLBACK_POST_RENDER) {
            SetAlphaOp(D3DTOP_MODULATE);
            SetColorOp(D3DTOP_MODULATE);
            SetColorOp1(D3DTOP_DISABLE);
            SetAlphaState(1);
            SetBlendState(0);
            SetClipping(TRUE);
            SetFog(FALSE, 0, 0.0f, 0.0f, 0);
            renderCB = true;
            SetZBias(0);
            scene->callback(SCN_CALLBACK_POST_RENDER, 0, 0);
            renderCB = false;
        }
        m_pD3DDevice8->EndScene();
        rendering = false;
    }
    pCurrentScene = NULL;
    return I3D_OK;
}

// Queues a visual's face groups: the particles for the particles' renderer, the lens flares into the scene's list,
// the others through their m_AddFGroups (the projectors only with RS_USEPROJECTORS). After Render, its first caller.
// FUNCTION: LS3DF 0x10065560
static void VisualProc(I3D_visual *vis)
{
    if (vis->owner)
        vis->owner->render_number = RenderNumber;
    if (m_ProblemFlags & 2)
        vis->frm_flags |= FRMFLAGS_CLIP;
    switch (vis->GetVisualType()) {
    case I3D_VISUAL_PART_BASE:
    case I3D_VISUAL_PART_ELEMENT: {
        I3D_part_base *part = (I3D_part_base *)vis;
        if (part->part_flags & 0x600) {
            S_particle_item item;
            item.vis = vis;
            item.sort_value = (uint)part->sort_group << 16;
            if (part->part_flags & 3)
                item.sort_value |= FloatToInt(part->sort_dist);
            ParticleElementsList.visuals.push_back(item);
        } else
            vis->m_AddFGroups();
    } break;
    case I3D_VISUAL_PROJECTOR:
        if (!renderState[RS_USEPROJECTORS])
            return;
    default:
        vis->m_AddFGroups();
        break;
    case I3D_VISUAL_LENSF: {
        I3D_lensflare *lf = (I3D_lensflare *)vis;
        if (!AddFGHlpStruct.scene->lensflares.find(lf)) {
            lf->ref++;
            AddFGHlpStruct.scene->lensflares.push_back(lf);
            lf->last_check_col_time = 0;
            lf->last_update = timeGetTime();
        }
        lf->unknown_224 |= 1;
    } break;
    }
}

// Queues the face groups of every visual in the branch from frm, those of an object's mesh once for each level, as
// if seen from each level's distance (BuildAllVBuffers)
// FUNCTION: LS3DF 0x10065770
void NextFFrameProc(I3D_frame *frm)
{
    for (; frm; frm = frm->next) {
        if (frm->frm_flags & FRMFLAGS_ON) {
            if (frm->type == FRAME_VISUAL) {
                I3D_visual *vis = (I3D_visual *)frm;
                if (vis->vis_flags & VISF_UNKNOWN_800 && ((I3D_object *)vis)->mesh) {
                    I3D_mesh_object *mesh = ((I3D_object *)vis)->mesh;
                    float prev = 0.0f;
                    int num = 1;
                    if (!(vis->vis_flags & VISF_UNKNOWN_10))
                        num = mesh->num_lods;
                    for (int i = 0; i < num; i++) {
                        AddFGHlpStruct.cam_pos = vis->GetWBoundVolume().bsphere.pos;
                        RSLodQuality = 1.0f / vis->world_scale;
                        float d = (float)sqrt(sqrt(((I3D_object *)vis)->mesh->GetLOD(i)->lod_dist));
                        AddFGHlpStruct.cam_pos.z += (d + prev) * .5f;
                        vis->m_AddFGroups();
                        prev = d;
                    }
                } else {
                    AddFGHlpStruct.cam_pos = vis->GetWBoundVolume().bsphere.pos;
                    RSLodQuality = 1.0f / vis->world_scale;
                    vis->m_AddFGroups();
                }
                AddFGHlpStruct.fgroups.reset();
            }
            NextFFrameProc(frm->child);
        }
    }
}

// Builds every visual's vertex buffers by queueing their face groups, those of meshes for each level (PS2's second
// pass, which frees memory, isn't on PC)
// FUNCTION: LS3DF 0x100658f0
void I3D_driver::BuildAllVBuffers(I3D_scene *scene)
{
    I3D_frame *frm = CreateFrame(FRAME_SECTOR);
    AddFGHlpStruct.Clear();
    AddFGHlpStruct.scene = scene;
    AddFGHlpStruct.sector = (I3D_sector *)frm;
    AddFGHlpStruct.camera = scene->current_camera;
    NextFFrameProc(scene->primary_sector->child);
    NextFFrameProc(scene->omni_sector->child);
    NextFFrameProc(scene->backdrop_sector->child);
    NextFFrameProc(scene->near_sector->child);
    frm->Release();
    AddFGHlpStruct.Clear();
}

// Draws the queued face groups in their sorted order: the opaque ones first, then the weather, then the transparent
// ones from the last back, then the weather again, each in its sector's fog (PS2 name; inlined everywhere)
inline void I3D_driver::m_DrawAllSortedFG()
{
    for (int i = 0; i < AddFGHlpStruct.num_opaque; i++) {
        RENDER_FG_HELP_STRUCT *rfg = &AddFGHlpStruct.fgroups[AddFGHlpStruct.sort[i]];
        // A group without a sector turns the fog off in a branch of its own, as on PS2. The two calls, which VC6 merges
        // into one, give the original's pushes and inline budget; one condition for all four tests with one call gives
        // neither
        if (rfg->sector) {
            I3D_light *fog = rfg->sector->fog_light;
            if (fog && renderState[RS_FOG] && !(rfg->vis->vis_flags & 0x8000)) {
                switch (fog->ltype) {
                case I3DLIGHT_FOG: {
                    float end = pCurrentCamera->fcp * fog->range_f;
                    float start = end * fog->range_n;
                    SetFog(TRUE, 0, start, end, ((((uint)-1 << 8 | FloatToInt(fog->diffuse.x * 255.0f)) << 8 |
                        FloatToInt(fog->diffuse.y * 255.0f)) << 8) | FloatToInt(fog->diffuse.z * 255.0f));
                } break;
                case I3DLIGHT_POINTAMBIENT:
                    SetFog(TRUE, 1, fog->near_range, 0.0f, ((((uint)-1 << 8 | FloatToInt(fog->diffuse.x * 255.0f)) << 8 |
                        FloatToInt(fog->diffuse.y * 255.0f)) << 8) | FloatToInt(fog->diffuse.z * 255.0f));
                    break;
                case I3DLIGHT_POINTFOG:
                    SetFog(TRUE, 2, fog->near_range, 0.0f, ((((uint)-1 << 8 | FloatToInt(fog->diffuse.x * 255.0f)) << 8 |
                        FloatToInt(fog->diffuse.y * 255.0f)) << 8) | FloatToInt(fog->diffuse.z * 255.0f));
                    break;
                }
            } else
                SetFog(FALSE, 0, 0.0f, 0.0f, 0);
        } else
            SetFog(FALSE, 0, 0.0f, 0.0f, 0);
        if (rfg->clip)
            SetClipping(TRUE);
        else
            SetClipping(FALSE);
        SetZBias(rfg->vis->vis_flags & 0x100 ? 2 : 0);
        rfg->vis->m_RenderFGroup(rfg);
    }
    if (GlobalDrawWeather && !renderCB)
        pCurrentScene->m_RenderWeatherSystem();
    if (AddFGHlpStruct.num_transparent) {
        for (int j = AddFGHlpStruct.fgroups.size() - 1; j >= AddFGHlpStruct.num_opaque; j--) {
            RENDER_FG_HELP_STRUCT *rfg = &AddFGHlpStruct.fgroups[AddFGHlpStruct.sort[j]];
            I3D_light *fog = rfg->sector->fog_light;
            if (fog && renderState[RS_FOG] && !(rfg->vis->vis_flags & 0x8000)) {
                switch (fog->ltype) {
                case I3DLIGHT_FOG: {
                    float end = pCurrentCamera->fcp * fog->range_f;
                    float start = end * fog->range_n;
                    SetFog(TRUE, 0, start, end, ((((uint)-1 << 8 | FloatToInt(fog->diffuse.x * 255.0f)) << 8 |
                        FloatToInt(fog->diffuse.y * 255.0f)) << 8) | FloatToInt(fog->diffuse.z * 255.0f));
                } break;
                case I3DLIGHT_POINTAMBIENT:
                    SetFog(TRUE, 1, fog->near_range, 0.0f, ((((uint)-1 << 8 | FloatToInt(fog->diffuse.x * 255.0f)) <<
                        8 | FloatToInt(fog->diffuse.y * 255.0f)) << 8) | FloatToInt(fog->diffuse.z * 255.0f));
                    break;
                case I3DLIGHT_POINTFOG:
                    SetFog(TRUE, 2, fog->near_range, 0.0f, ((((uint)-1 << 8 | FloatToInt(fog->diffuse.x * 255.0f)) <<
                        8 | FloatToInt(fog->diffuse.y * 255.0f)) << 8) | FloatToInt(fog->diffuse.z * 255.0f));
                    break;
                }
            } else
                SetFog(FALSE, 0, 0.0f, 0.0f, 0);
            if (rfg->clip)
                SetClipping(TRUE);
            else
                SetClipping(FALSE);
            SetZBias(rfg->vis->vis_flags & 0x100 ? 2 : 0);
            rfg->vis->m_RenderFGroup(rfg);
        }
    }
    if (GlobalDrawWeather)
        pCurrentScene->m_RenderWeatherSystem();
}
