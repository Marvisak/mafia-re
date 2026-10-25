// The loaders (PS2 Load.cpp), at 0x10060390-0x10060c20 in the original: LoadProgress, LoadHierarchy, LoadAnimation and
// the PC's LoadShadow, then the header's zero vector's $E (0x10060c00). Names are the PS2 build's unless noted.
// Each loader waits while another load is running, keeps the callback for LoadProgress, makes the file's name from the
// name it is given with its own extension (PS2's copy the name up to its extension themselves) and calls the file's
// loader: load4ds.cpp's LoadImage and LoadShadowImage, Load5ds.cpp's LoadAnimation5ds.
#include "I3D/I3D.h"
#include "IGraph/IGraph.h"
#include "rw_data.h"

#include <mmsystem.h>
#include <string.h>

// IGraph.cpp's: while set, a minimized application doesn't sleep in ProcessWinMessages
extern bool m_DisableSleeping;

// The progress last reported and its value for the callback, the callback, its flags (I3DLOAD_*) and its context, and
// whether a hierarchy or shadow or an animation set is being loaded. Their .bss is in the order of VC6's hash of the
// names, with the loaded frames' and materials' among them.
// GLOBAL: LS3DF 0x101b5148
// GLOBAL: LS3DF11 0x101b2258
static float progress;

// GLOBAL: LS3DF 0x101b514c
// GLOBAL: LS3DF11 0x101b225c
I3D_LOAD_CB_PROC *pLoadCallback;

// GLOBAL: LS3DF 0x101b5150
// GLOBAL: LS3DF11 0x101b2260
uint LoadCBflags;

// GLOBAL: LS3DF 0x101b5160
// GLOBAL: LS3DF11 0x101b2270
I3D_frame *LoadFramy[10000];

// GLOBAL: LS3DF 0x101beda0
// GLOBAL: LS3DF11 0x101bbeb0
int LoadFramu;

// GLOBAL: LS3DF 0x101beda4
// GLOBAL: LS3DF11 0x101bbeb4
static float lastProgress;

// GLOBAL: LS3DF 0x101beda8
// GLOBAL: LS3DF11 0x101bbeb8
I3D_material *LoadMaterialy[5000];

// GLOBAL: LS3DF 0x101c3bc8
// GLOBAL: LS3DF11 0x101c0cd8
int LoadMaterialu;

// GLOBAL: LS3DF 0x101c3bcc
// GLOBAL: LS3DF11 0x101c0cdc
int LoadAnimCount;

// GLOBAL: LS3DF 0x101c3bd0
// GLOBAL: LS3DF11 0x101c0ce0
void *LoadCbContext;

// GLOBAL: LS3DF 0x101c3bd4
// GLOBAL: LS3DF11 0x101c0ce4
static bool loading;

// GLOBAL: LS3DF 0x101c3bd5
// GLOBAL: LS3DF11 0x101c0ce5
static bool loadingAnim;

// Reports the progress, in steps of 0.02 at least
// FUNCTION: LS3DF 0x10060390
// FUNCTION: LS3DF11 0x1005d670
void LoadProgress(float p)
{
    if (p < lastProgress + .02f)
        return;
    if (LoadCBflags & I3DLOAD_PROGRESS) {
        progress = lastProgress = p;
        pLoadCallback(CBM_PROGRESS, *(uint *)&progress, 1, LoadCbContext);
    }
}

// Loads a .4ds file into root, a model or a scene's primary sector: the frames without a parent are linked to root, and
// the model or the scene owns them. A file with animations gets the animation set of its .5ds file.
// FUNCTION: LS3DF 0x100603e0
// FUNCTION: LS3DF11 0x1005d6c0
LS3D_RESULT LoadHierarchy(const char *file_name, uint flags, I3D_LOAD_CB_PROC *cb, void *context, I3D_frame *root)
{
    if (!root || !file_name || !*file_name) {
        dbgPrintf("!! INVALIDPARAM  load.cpp  ?::Open()  %s  pParent:0x%x", file_name ? file_name : "NULL", root);
        return I3DERR_INVALIDPARAMS;
    }
    if (loading) {
        uint t = timeGetTime();
        do {
            pGlobalIGraph->ProcessWinMessages();
            if (timeGetTime() - t > 4000) {
                dbgPrintf("!! Load deadlock. Terminating load...  %s", file_name);
                return I3DERR_GENERIC;
            }
        } while (loading);
    }
    loading = true;
    LoadCbContext = context;
    pLoadCallback = cb;
    LoadCBflags = flags;
    if (!cb)
        LoadCBflags &= I3DLOAD_LOG;
    lastProgress = 0.0f;
    if (LoadCBflags & I3DLOAD_PROGRESS) {
        progress = 0.0f;
        pLoadCallback(CBM_PROGRESS, *(uint *)&progress, 0, LoadCbContext);
    }
    LoadAnimCount = 0;
    LoadFramu = 0;
    char name[276];
    strncpy(name, file_name, 269);
    name[269] = 0;
    int i = strlen(name) - 1;
    char *ext = &name[i];
    while (i--) {
        if (*ext == '.')
            break;
        ext--;
    }
    if (i <= 0 || !strcmpi(ext, ".4ds") || *ext == '\\') {
        dbgPrintf("!! Bad file name: %s  load.cpp  ?::Open()", file_name);
        if (LoadCBflags & I3DLOAD_PROGRESS) {
            progress = 1.0f;
            pLoadCallback(CBM_PROGRESS, *(uint *)&progress, 2, LoadCbContext);
        }
        loading = false;
        return I3DERR_INVALIDPARAMS;
    }
    strcpy(ext, ".4ds");
    m_DisableSleeping = true;
    LS3D_RESULT r = LoadImage(name, NULL);
    m_DisableSleeping = false;
    I3D_frame **frm = LoadFramy;
    if (root->m_Type == FRAME_MODEL) {
        while (LoadFramu--) {
            if (!(*frm)->m_pParent)
                (*frm)->LinkTo(root, 0);
            ((I3D_model *)root)->AddFrame(*frm);
            (*frm)->Release();
            frm++;
        }
    } else {
        I3D_scene *scene = ((I3D_sector *)root)->scene;
        while (LoadFramu--) {
            if (!(*frm)->m_pParent)
                (*frm)->LinkTo(root, 0);
            scene->AddFrame(*frm);
            (*frm)->Release();
            frm++;
        }
    }
    // The failures report the end inside the switch, the success after it, each with its copy: VC6 merges the two
    // copies' common tail into the later one, the success's. Without the goto (the failures after the switch, an if
    // before the success) it merges both copies into one.
    switch (r) {
    case I3D_OK:
        break;
    case I3DERR_FILESYSTEM:
        dbgPrintf("!! Filesystem error while loading %s, deleting file  load.cpp  ?::Open()", name);
        dtaDelete(name);
        goto fail;
    case I3DERR_BADVERSION:
        dbgPrintf("!! Bad file version load.cpp  ?::Open() %s", file_name);
    default:
    fail:
        if (LoadCBflags & I3DLOAD_PROGRESS) {
            progress = 1.0f;
            pLoadCallback(CBM_PROGRESS, *(uint *)&progress, 2, LoadCbContext);
        }
        loading = false;
        return r;
    }
    if (LoadAnimCount) {
        I3D_animation_set *set = pI3DDriver->CreateAnimationSet();
        if (!set) {
            dbgPrintf(" !! Out of memory  load.cpp  ?::Open()  %s", file_name);
            if (LoadCBflags & I3DLOAD_PROGRESS) {
                progress = 1.0f;
                pLoadCallback(CBM_PROGRESS, *(uint *)&progress, 2, LoadCbContext);
            }
            loading = false;
            return I3DERR_OUTOFMEM;
        }
        r = set->Open(file_name, flags, NULL, NULL);
        if (r == I3D_OK) {
            set->options = 2;
            if (root->m_Type == FRAME_MODEL)
                ((I3D_model *)root)->SetAnimation(set, 0, 1.0f, 0);
            else
                ((I3D_sector *)root)->scene->SetAnimation(set, 0, 1.0f, 0);
        }
        set->Release();
    }
    root->m_FrmFlags = (root->m_FrmFlags & ~FRMFLAGS_HR_BOUND_W_VALID) | FRMFLAGS_UPDATE;
    root->Update();
    if (LoadCBflags & I3DLOAD_PROGRESS) {
        progress = 1.0f;
        pLoadCallback(CBM_PROGRESS, *(uint *)&progress, 2, LoadCbContext);
    }
    loading = false;
    return r;
}

// Loads a .5ds file's data into the animation set
// FUNCTION: LS3DF 0x10060840
// FUNCTION: LS3DF11 0x1005db20
LS3D_RESULT LoadAnimation(const char *file_name, uint flags, I3D_LOAD_CB_PROC *cb, void *context,
    I3D_animation_set *set)
{
    if (!set || !file_name || !*file_name) {
        dbgPrintf("!! INVALIDPARAMS  load.cpp  ?::Open()  %s", file_name ? file_name : "fName:NULL");
        return I3DERR_INVALIDPARAMS;
    }
    if (loadingAnim) {
        uint t = timeGetTime();
        do {
            pGlobalIGraph->ProcessWinMessages();
            if (timeGetTime() - t > 4000) {
                dbgPrintf("!! Load deadlock. Terminating anim. load...  %s", file_name);
                return I3DERR_GENERIC;
            }
        } while (loadingAnim);
    }
    loadingAnim = true;
    pGlobalIGraph->ProcessWinMessages();
    LoadCbContext = context;
    pLoadCallback = cb;
    LoadCBflags = flags;
    if (!cb)
        LoadCBflags &= I3DLOAD_LOG;
    char name[276];
    strncpy(name, file_name, 269);
    name[269] = 0;
    int i = strlen(name) - 1;
    char *ext = &name[i];
    while (i--) {
        if (*ext == '.')
            break;
        ext--;
    }
    if (i <= 0 || !strcmpi(ext, ".5ds") || *ext == '\\') {
        dbgPrintf("!! Bad file name: %s  I3D_animation_set::Open()", file_name);
        loadingAnim = false;
        return I3DERR_INVALIDPARAMS;
    }
    strcpy(ext, ".5ds");
    LS3D_RESULT r = LoadAnimation5ds(name, (void **)&set->data, NULL);
    switch (r) {
    case I3D_OK:
    case I3DERR_USERBREAK:
        break;
    case I3DERR_FILESYSTEM:
        dbgPrintf("!! Filesystem error while loading %s, deleting this file I3D_animation_set::Open()", name);
        dtaDelete(name);
        break;
    default:
        dbgPrintf("!! %s  load.cpp  I3D_animation_set::Open()  %s", I3DGetResultMessage(r), file_name);
    }
    loadingAnim = false;
    return r;
}

// Loads a .6ds file into the shadow's geometry (PC only, the name is a guess)
// FUNCTION: LS3DF 0x10060a50
// FUNCTION: LS3DF11 0x1005dd30
LS3D_RESULT LoadShadow(const char *file_name, uint flags, I3D_LOAD_CB_PROC *cb, void *context, ShadowDef *def)
{
    if (!def || !file_name || !*file_name) {
        dbgPrintf("!! INVALIDPARAM  I3D_shadow::Open()   no fName");
        return I3DERR_INVALIDPARAMS;
    }
    if (loading) {
        uint t = timeGetTime();
        do {
            pGlobalIGraph->ProcessWinMessages();
            if (timeGetTime() - t > 4000) {
                dbgPrintf("!! Load deadlock. Terminating load...  %s", file_name);
                return I3DERR_GENERIC;
            }
        } while (loading);
    }
    loading = true;
    LoadCbContext = context;
    pLoadCallback = cb;
    LoadCBflags = flags;
    char name[276];
    strncpy(name, file_name, 269);
    name[269] = 0;
    int i = strlen(name) - 1;
    char *ext = &name[i];
    while (i--) {
        if (*ext == '.')
            break;
        ext--;
    }
    if (i <= 0 || !strcmpi(ext, ".6ds") || *ext == '\\') {
        dbgPrintf("!! Bad file name: %s  I3D_shadow::Open()", file_name);
        loading = false;
        return I3DERR_INVALIDPARAMS;
    }
    strcpy(ext, ".6ds");
    LS3D_RESULT r = LoadShadowImage(def, name, NULL);
    if (r == I3DERR_BADVERSION)
        dbgPrintf("!! Bad file version  I3D_shadow::Open() %s", file_name);
    loading = false;
    return r;
}
