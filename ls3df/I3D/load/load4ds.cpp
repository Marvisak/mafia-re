// The 4ds loader (PS2 load4ds.cpp), at 0x10060c40-0x10061800 in the original: the header's zero vector's $E, the target
// list's, then LoadImage, LoadMeshImage, the PC's LoadShadowImage and C_list<S_vector>::reserve, out of line for it.
// Names are the PS2 build's unless noted.
// - No precompiled header: the zero vector's $E is the file's first function (0x10060c40), before the target list's
//   (0x10060c60), as in .CRT$XCU. #pragma hdrstop before the includes leaves the file without one (see I3d_area.cpp).
// - The file's .bss is its globals' and the zero vector's in VC6's hash order (ImageErrFlg, ShadowInfo, the vector,
//   LoadImageHandle, ImageFileName, LoadTargetList). Open: a dword nothing references at 0x101c3c00, between
//   LoadImageHandle and ImageFileName, a global with external linkage (VC6 gives an unreferenced static none) whose
//   name hashes between theirs.
#pragma hdrstop
#include "I3D/I3D.h"
#include "I3D/mesh/I3d_material.h"
#include "IGraph/IGraph.h"
#include "rw_data.h"

#include <string.h>

// The 4ds loader's state, which the frames' LoadImage functions read from and set: the error, the file and its name
// GLOBAL: LS3DF 0x101c3be4
// GLOBAL: LS3DF11 0x101c0cf4
LS3D_RESULT ImageErrFlg;

// GLOBAL: LS3DF 0x101c3bfc
// GLOBAL: LS3DF11 0x101c0d0c
int LoadImageHandle;

// GLOBAL: LS3DF 0x101c3c04
// GLOBAL: LS3DF11 0x101c0d14
const char *ImageFileName;

// GLOBAL: LS3DF 0x101c3c08
// GLOBAL: LS3DF11 0x101c0d18
C_list<I3D_target *> LoadTargetList;

// A .6ds file's counts (PC only, the names are guesses), read at once: its vertices, its faces' indices and its parts.
// The name is chosen for the .bss order (VC6's hash puts it before the zero vector, after ImageErrFlg).
struct S_shadow_info {
    ushort num_verts;
    ushort num_indices;
    ushort num_parts;
};

// GLOBAL: LS3DF 0x101c3be8
// GLOBAL: LS3DF11 0x101c0cf8
static S_shadow_info ShadowInfo;

// The visuals' types by their index in the file (PC only, the name is a guess; PS2's types are the indices)
// GLOBAL: LS3DF 0x100a915c
// GLOBAL: LS3DF11 0x100a62c4
static I3D_VISUAL_TYPE VisualTypes[] = {
    I3D_VISUAL_OBJECT, I3D_VISUAL_LIT_OBJECT, I3D_VISUAL_SINGLEMESH, I3D_VISUAL_SINGLEMORPH, I3D_VISUAL_BILLBOARD,
    I3D_VISUAL_MORPH, I3D_VISUAL_LENSF, I3D_VISUAL_PROJECTOR, I3D_VISUAL_MIRROR,
};

// Loads a .4ds file: its materials, then its frames, each linked to its parent (by its 1-based index) and loading its
// own data (the frames' LoadImage functions), then the targets' frames. The frames stay in LoadFramy for the caller;
// the materials are released, the meshes keep them. PS2's also builds the single meshes; the PC's never compares the
// timestamp.
// FUNCTION: LS3DF 0x10060cb0
// FUNCTION: LS3DF11 0x1005df90
LS3D_RESULT LoadImage(const char *file_name, const FILETIME *time)
{
    LoadFramu = 0;
    LoadMaterialu = 0;
    LoadAnimCount = 0;
    LoadTargetList.reset();
    if (!file_name || !*file_name)
        return I3DERR_INVALIDPARAMS;
    LoadImageHandle = dtaOpen(file_name, false);
    if (LoadImageHandle < 0)
        return I3DERR_NOFILE;
    char id[4];
    id[0] = 0;
    dtaRead(LoadImageHandle, id, 4);
    if (strcmp(id, "4DS")) {
        dtaClose(LoadImageHandle);
        return I3DERR_FILECORRUPTED;
    }
    int version = 0;
    dtaRead(LoadImageHandle, &version, 2);
    if (version != I3D_4DS_VERSION) {
        dtaClose(LoadImageHandle);
        return I3DERR_BADVERSION;
    }
    FILETIME ft;
    dtaRead(LoadImageHandle, &ft, sizeof(ft));
    ImageFileName = file_name;
    ImageErrFlg = I3D_OK;
    dtaRead(LoadImageHandle, &LoadMaterialu, 2);
    int i;
    for (i = 0; i < LoadMaterialu; i++) {
        LoadProgress((float)(i / 2) / LoadMaterialu);
        uint flags;
        dtaRead(LoadImageHandle, &flags, 4);
        I3D_material *mat;
        if (flags & MATF_ANIMATED) {
            I3D_anim_material *amat = pI3DDriver->CreateAnimMaterial();
            if (!amat) {
                dbgPrintf("!! Anim. Material not created  LoadImage() %s", file_name);
                ImageErrFlg = I3DERR_OUTOFMEM;
                break;
            }
            amat->flags = flags;
            amat->LoadImage();
            mat = amat;
        } else {
            mat = pI3DDriver->CreateMaterial();
            if (!mat) {
                dbgPrintf("!! Material not created  LoadImage() %s", file_name);
                ImageErrFlg = I3DERR_OUTOFMEM;
                break;
            }
            mat->flags = flags;
            mat->LoadImage();
        }
        LoadMaterialy[i] = mat;
        if (ImageErrFlg) {
            dbgPrintf("!! Error: %s while loading material ID:%d  %s", I3DGetResultMessage(ImageErrFlg), i + 1,
                file_name);
            break;
        }
    }
    LoadMaterialu = i;
    if (!ImageErrFlg) {
        dtaRead(LoadImageHandle, &LoadFramu, 2);
        int j;
        for (j = 0; j < LoadFramu; j++) {
            LoadProgress((float)(j / 2) / LoadFramu + .5f);
            int type = 0;
            dtaRead(LoadImageHandle, &type, 1);
            I3D_frame *frm;
            if (type == FRAME_VISUAL) {
                uint vtype = 0;
                dtaRead(LoadImageHandle, &vtype, 1);
                if (vtype >= sizeof(VisualTypes) / sizeof(VisualTypes[0])) {
                    dbgPrintf("!! Invalid visual type:%d  LoadImage() %s", vtype, file_name);
                    ImageErrFlg = I3DERR_FILECORRUPTED;
                    break;
                }
                I3D_visual *vis = pI3DDriver->CreateVisual(VisualTypes[vtype]);
                if (!vis) {
                    ImageErrFlg = I3DERR_GENERIC;
                    break;
                }
                // The file's flags word, vis_flags' low half
                dtaRead(LoadImageHandle, &vis->vis_flags, 2);
                vis->vis_flags &= ~(VISF_UNKNOWN_80000 | VISF_INV_MAT_VALID);
                frm = vis;
            } else {
                if (type >= FRAME_LAST) {
                    ImageErrFlg = I3DERR_FILECORRUPTED;
                    break;
                }
                frm = pI3DDriver->CreateFrame((I3D_FRAME_TYPE)type);
                if (!frm) {
                    ImageErrFlg = I3DERR_GENERIC;
                    break;
                }
            }
            int parent = 0;
            dtaRead(LoadImageHandle, &parent, 2);
            if (parent) {
                if (parent > j) {
                    dbgPrintf("!! Invalid parent ID:%d  LoadImage() %s", parent, file_name);
                    ImageErrFlg = I3DERR_FILECORRUPTED;
                    break;
                }
                frm->LinkTo(LoadFramy[parent - 1], 0);
            }
            LoadFramy[j] = frm;
            frm->LoadImage();
            if (ImageErrFlg)
                break;
        }
        LoadFramu = j;
        dtaRead(LoadImageHandle, &LoadAnimCount, 1);
    }
    // The targets' lists hold their frames' indices: an index past the frames loaded is an error, 0 none. Removing one
    // moves the list's last frame into its place, so the pointer stays. The list is tested before its count is read
    // for the loop, as each target's count is read before its frames.
    if (LoadTargetList.size()) {
        I3D_target **pt = &LoadTargetList[0];
        for (int n = LoadTargetList.size(); n--; pt++) {
            int k = (*pt)->frames.size();
            I3D_frame **pf = &(*pt)->frames[0];
            while (k--) {
                int index = (int)*pf;
                if (index > LoadFramu) {
                    ImageErrFlg = I3DERR_FILECORRUPTED;
                    *pf = NULL;
                    (*pt)->DeleteFromTarget(NULL);
                } else if (!index) {
                    (*pt)->DeleteFromTarget(NULL);
                } else {
                    *pf = LoadFramy[index - 1];
                    LoadFramy[index - 1]->m_FrmFlags |= FRMFLAGS_TARGET;
                    pf++;
                }
            }
        }
    }
    dtaClose(LoadImageHandle);
    LoadImageHandle = -1;
    for (i = 0; i < LoadMaterialu; i++)
        LoadMaterialy[i]->Release();
    return ImageErrFlg;
}

// An object's mesh from the file: a reference to an earlier object's (the frame's index), or its levels, each with its
// distance, vertices (0xffff: texture coordinates for the first level's vertices) and face groups. Errors set
// ImageErrFlg and return NULL; a bad material ends the level's face groups.
// FUNCTION: LS3DF 0x10061120
// FUNCTION: LS3DF11 0x1005e400
I3D_mesh_object *LoadMeshImage()
{
    int index = 0;
    dtaRead(LoadImageHandle, &index, 2);
    if (index) {
        I3D_mesh_object *mesh = ((I3D_object *)LoadFramy[index - 1])->mesh;
        mesh->AddRef();
        return mesh;
    }
    int num_lods = 0;
    dtaRead(LoadImageHandle, &num_lods, 1);
    if (!num_lods)
        return NULL;
    I3D_mesh_object *mesh = pI3DDriver->CreateMesh();
    if (!mesh) {
        dbgPrintf("!! Mesh not created  LoadMeshImage()  %s", ImageFileName);
        ImageErrFlg = I3DERR_OUTOFMEM;
        return NULL;
    }
    for (int i = 0; i < num_lods; i++) {
        I3D_mesh_level *level = pI3DDriver->CreateMeshLevel();
        if (!level) {
            dbgPrintf("!! Mesh_level not created  LoadMeshImage()  %s", ImageFileName);
            ImageErrFlg = I3DERR_OUTOFMEM;
            return NULL;
        }
        dtaRead(LoadImageHandle, &level->lod_dist, 4);
        int num_vertices = 0;
        dtaRead(LoadImageHandle, &num_vertices, 2);
        if (num_vertices == 0xffff) {
            // An older level's m_pUV, which the PC's level has its vertex buffer's pointer in place of
            num_vertices = mesh->levels[0]->num_vertices;
            level->vb.buffer = (C_VBuffer *)new I3D_text_coor[num_vertices];
            if (!level->vb.buffer) {
                dbgPrintf("!! m_pUV not created  LoadMeshImage()  %s", ImageFileName);
                ImageErrFlg = I3DERR_OUTOFMEM;
                return NULL;
            }
            dtaRead(LoadImageHandle, level->vb.buffer, num_vertices * sizeof(I3D_text_coor));
        } else if (num_vertices) {
            I3D_vertex *verts = level->CreateVertexMesh(num_vertices);
            dtaRead(LoadImageHandle, verts, num_vertices * sizeof(I3D_vertex));
            level->UpdateBBox();
        }
        int num_fgroups = 0;
        dtaRead(LoadImageHandle, &num_fgroups, 1);
        while (num_fgroups--) {
            I3D_face_group *fg = pI3DDriver->CreateFaceGroup();
            if (!fg) {
                dbgPrintf("!! Face_group not created  LoadMeshImage()  %s", ImageFileName);
                ImageErrFlg = I3DERR_OUTOFMEM;
                return NULL;
            }
            int num_faces = 0;
            dtaRead(LoadImageHandle, &num_faces, 2);
            if (num_faces) {
                I3D_triface *faces = fg->CreateFaces(num_faces);
                if (!faces) {
                    dbgPrintf("!! Faces not created  LoadMeshImage()  %s", ImageFileName);
                    ImageErrFlg = I3DERR_OUTOFMEM;
                    return NULL;
                }
                dtaRead(LoadImageHandle, faces, num_faces * sizeof(I3D_triface));
            }
            int mat = 0;
            dtaRead(LoadImageHandle, &mat, 2);
            if (mat) {
                if (mat > LoadMaterialu) {
                    dbgPrintf("!! Invalid material ID:%d  materialu:%d  LoadImage() %s", mat, LoadMaterialu,
                        ImageFileName);
                    ImageErrFlg = I3DERR_FILECORRUPTED;
                    break;
                }
                fg->SetMaterial(LoadMaterialy[mat - 1]);
            }
            level->AddFGroup(fg, true);
        }
        mesh->SetLOD(level, i);
        level->Release();
    }
    return mesh;
}

// Loads a .6ds file into a shadow's geometry: the vertices, the faces' indices, then each part's counts and the name of
// its frame (PC only, the name is its message's). With a time the file's timestamp has to be that one.
// FUNCTION: LS3DF 0x10061450
// FUNCTION: LS3DF11 0x1005e730
LS3D_RESULT LoadShadowImage(ShadowDef *def, const char *file_name, const FILETIME *time)
{
    if (!file_name || !*file_name)
        return I3DERR_INVALIDPARAMS;
    LoadImageHandle = dtaOpen(file_name, false);
    if (LoadImageHandle < 0)
        return I3DERR_NOFILE;
    char id[4];
    id[0] = 0;
    dtaRead(LoadImageHandle, id, 4);
    if (strcmp(id, "6DS")) {
        dtaClose(LoadImageHandle);
        return I3DERR_FILECORRUPTED;
    }
    int version = 0;
    dtaRead(LoadImageHandle, &version, 2);
    if (version != I3D_6DS_VERSION) {
        dtaClose(LoadImageHandle);
        return I3DERR_BADVERSION;
    }
    FILETIME ft;
    dtaRead(LoadImageHandle, &ft, sizeof(ft));
    if (time && CompareFileTime(time, &ft)) {
        dtaClose(LoadImageHandle);
        return I3DERR_BADTIMESTAMP;
    }
    dtaRead(LoadImageHandle, &ShadowInfo, sizeof(ShadowInfo));
    if (ShadowInfo.num_verts) {
        def->verts.reserve(ShadowInfo.num_verts);
        dtaRead(LoadImageHandle, &def->verts[0], ShadowInfo.num_verts * sizeof(S_vector));
        def->verts.used_size = ShadowInfo.num_verts;
    }
    if (ShadowInfo.num_indices) {
        def->faces.reserve(ShadowInfo.num_indices);
        dtaRead(LoadImageHandle, &def->faces[0], ShadowInfo.num_indices * sizeof(ushort));
        def->faces.used_size = ShadowInfo.num_indices;
    }
    if (ShadowInfo.num_parts) {
        def->parts.reserve(ShadowInfo.num_parts);
        def->parts.used_size = ShadowInfo.num_parts;
        S_shadow_part *part = &def->parts[0];
        while (ShadowInfo.num_parts--) {
            // num_verts and num_faces
            dtaRead(LoadImageHandle, &part->num_verts, 8);
            int len = 0;
            dtaRead(LoadImageHandle, &len, 1);
            part->name = new char[len + 1];
            if (!part->name) {
                dbgPrintf("!! Out of memory  LoadShadowImage()  %s", file_name);
                ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                return I3DERR_OUTOFMEM;
            }
            if (len)
                dtaRead(LoadImageHandle, part->name, len);
            part->name[len] = 0;
            part++;
        }
    }
    dtaClose(LoadImageHandle);
    LoadImageHandle = -1;
    return I3D_OK;
}
