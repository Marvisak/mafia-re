// The animation files' loader (PS2 Load5ds.cpp), at 0x10061800-0x10061990 in the original: LoadAnimation5ds, then the
// header's zero vector's $E (0x10061970). Load.cpp's LoadAnimation calls it. Names are the PS2 build's.
#include "I3D/I3D.h"
#include "IGraph/IGraph.h"
#include "rw_data.h"

#include <string.h>

// Reads a .5ds file's data into a new buffer, which the animation set keeps (I3D_animation_set::data). With a time the
// file's timestamp has to be that one. PS2's checks version 122 and ignores the timestamp's comparison.
// FUNCTION: LS3DF 0x10061800
// FUNCTION: LS3DF11 0x1005ea70
LS3D_RESULT LoadAnimation5ds(const char *file_name, void **data, const FILETIME *time)
{
    if (!file_name)
        return I3DERR_INVALIDPARAMS;
    int h = dtaOpen(file_name, false);
    if (h < 0)
        return I3DERR_NOFILE;
    char id[4];
    id[0] = 0;
    dtaRead(h, id, 4);
    if (strcmp(id, "5DS")) {
        dtaClose(h);
        return I3DERR_FILECORRUPTED;
    }
    int version = 0;
    dtaRead(h, &version, 2);
    if (version != I3D_5DS_VERSION) {
        dtaClose(h);
        return I3DERR_BADVERSION;
    }
    FILETIME ft;
    dtaRead(h, &ft, sizeof(ft));
    if (time && CompareFileTime(time, &ft)) {
        dtaClose(h);
        return I3DERR_BADTIMESTAMP;
    }
    int size = 0;
    dtaRead(h, &size, 4);
    char *buf = new char[size];
    if (!buf) {
        dbgPrintf("!! Out of memory  load5ds.cpp  LoadAnimation5ds()  %s  datasize:%d", file_name, size);
        dtaClose(h);
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
        return I3DERR_OUTOFMEM;
    }
    dtaRead(h, buf, size);
    dtaClose(h);
    *data = buf;
    return I3D_OK;
}
