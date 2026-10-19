// The animation sets (PS2 I3d_animation.cpp), at 0x10004890-0x10005180 in the original: I3D_animation_set in PS2's
// order, with the PC's GetAnimLink after GetEndTime and the rotation's, position's and scale's key functions before the
// notes'. The header's zero vector has its $E at the end (0x10005160). Names are the PS2 build's unless noted.
// A set is an animation file loaded into one block: for each frame it animates, the frame's name and keys (ANIMF_* in
// I3D.h), which the interpolators read directly (I3D_IPolSet::SetAnimation) and the game through the key functions.
// Shapes the original needs: the key functions walk one pointer through the tracks and reach a key's value by adding
// the count and the key's offset before the alignment (k += n + key * 8;), and GetAnimIndex walks a pointer of its own,
// stepped before the counter.
// Open: the original's .data has a 4-byte zero of this file that nothing references and ours lacks, between Release's
// message and NumRotKeys' (0x100a6344, where an empty string would go), as the sectors' file has.
#include "I3D/I3D.h"

#include <string.h>

// The loader (Load.cpp, not written yet)
LS3D_RESULT LoadAnimation(const char *file_name, uint flags, void *cb, void *context, I3D_animation_set *set);

// Steps p over a track of keys whose values are size words long: the count, the times, the values, the end aligned
// to 4 bytes. With p a reference, VC6 adds the position track's values as [p + n*14 + 2], as the original; returning
// the new pointer gives [n*14 + p + 2].
static inline void SkipTrack(const ushort *&p, int size)
{
    ushort n = *p;
    p += n * size + 1;
    if (!(n & 1))
        p++;
}

// FUNCTION: LS3DF 0x10004890
I3D_animation_set::I3D_animation_set()
{
    ref = 1;
    options = 1;
    data = NULL;
}

// Out of line on PS2; VC6 inlines both into Release and Open, and the linker drops our copies
I3D_animation_set::~I3D_animation_set()
{
    Close();
}

void I3D_animation_set::Close()
{
    delete[] (char *)data;
    data = NULL;
}

// FUNCTION: LS3DF 0x100048b0
int I3D_animation_set::Release()
{
    if (--ref < 0) {
        dbgPrintf("!! RefCount<0  I3D_animation_set::Release()  this:0x%x", this);
        return 0;
    }
    if (!ref) {
        delete this;
        return 0;
    }
    return ref;
}

// Loads the file and turns the offsets in it into addresses: the frames' names and keys, then the string notes'.
// Its tracks are skipped with an int count, which VC6 widens once (movzx); SkipTrack's ushort is widened at each use.
// FUNCTION: LS3DF 0x10004900
LS3D_RESULT I3D_animation_set::Open(const char *file_name, uint flags, void *cb, void *context)
{
    Close();
    LS3D_RESULT r = LoadAnimation(file_name, flags, cb, context, this);
    if (r < 0)
        return r;
    uint *p = (uint *)data->anims;
    for (int i = data->num_anims * 2; i--; p++)
        *p += (uint)data;
    S_frame_anim *anim = data->anims;
    for (int j = data->num_anims; j--; anim++) {
        uint key_flags = *(const uint *)anim->keys;
        if (!(key_flags & ANIMF_NOTE_STRING))
            continue;
        const ushort *k = anim->keys + 2;
        int n;
        if (key_flags & ANIMF_ROT) {
            n = *k;
            k += n * 9 + 1;
            if (!(n & 1))
                k++;
        }
        if (key_flags & ANIMF_POS) {
            n = *k;
            k += n * 7 + 1;
            if (!(n & 1))
                k++;
        }
        if (key_flags & ANIMF_SCALE) {
            n = *k;
            k += n * 7 + 1;
            if (!(n & 1))
                k++;
        }
        n = *k;
        k += n + 1;
        if (!(n & 1))
            k++;
        uint *s = (uint *)k;
        uint strings = (uint)(s + n);
        while (n--)
            *s++ += strings;
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10004a00
int I3D_animation_set::NumAnimations() const
{
    if (!data)
        return 0;
    return data->num_anims;
}

// In ms
// FUNCTION: LS3DF 0x10004a20
int I3D_animation_set::GetEndTime() const
{
    if (!data)
        return 0;
    return data->end_time * 40;
}

// The name of the index-th frame
// FUNCTION: LS3DF 0x10004a40
const char *I3D_animation_set::GetAnimLink(int index) const
{
    if (!data)
        return NULL;
    if (index >= data->num_anims)
        return NULL;
    return data->anims[index].frame_name;
}

// FUNCTION: LS3DF 0x10004a70
const S_frame_anim *I3D_animation_set::GetAnims() const
{
    if (!data)
        return NULL;
    return data->anims;
}

// The index of the frame's animation, -1 if it has none
// FUNCTION: LS3DF 0x10004a80
int I3D_animation_set::GetAnimIndex(const char *frame_name) const
{
    if (!data)
        return -1;
    const S_frame_anim *anim = data->anims;
    for (int i = 0; i < data->num_anims; anim++, i++) {
        if (!strcmp(anim->frame_name, frame_name))
            return i;
    }
    return -1;
}

// FUNCTION: LS3DF 0x10004af0
int I3D_animation_set::NumRotKeys(uint link) const
{
    if (data) {
        if (link < data->num_anims) {
            const ushort *k = data->anims[link].keys;
            if (*(const uint *)k & ANIMF_ROT)
                return k[2];
        } else
            dbgPrintf("!! index out of range  I3D_animation_set::NumRotKeys()  this:0x%x  index:%d", this, link);
    }
    return 0;
}

// FUNCTION: LS3DF 0x10004b30
LS3D_RESULT I3D_animation_set::GetRotKey(uint link, uint key, int &time, S_quat &rot) const
{
    if (!data)
        return I3DERR_NOTLOADED;
    if (link >= data->num_anims) {
        dbgPrintf("!! LinkIndex out of range  I3D_animation_set::GetRotKey()  this:0x%x  index:%d", this, link);
        return I3DERR_INVALIDPARAMS;
    }
    const ushort *k = data->anims[link].keys;
    if (!(*(const uint *)k & ANIMF_ROT))
        return I3DERR_INVALIDPARAMS;
    k += 2;
    int n = *k;
    if (key >= n) {
        dbgPrintf("!! KeyIndex out of range  I3D_animation_set::GetRotKey()  this:0x%x  index:%d", this, key);
        return I3DERR_INVALIDPARAMS;
    }
    k++;
    time = k[key] * 40;
    k += n + key * 8;
    if (!(n & 1))
        k++;
    rot = *(const S_quat *)k;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10004bf0
int I3D_animation_set::NumPosKeys(uint link) const
{
    if (data) {
        if (link < data->num_anims) {
            const ushort *k = data->anims[link].keys;
            uint flags = *(const uint *)k;
            if (flags & ANIMF_POS) {
                k += 2;
                if (flags & ANIMF_ROT)
                    SkipTrack(k, 9);
                return *k;
            }
        } else
            dbgPrintf("!! index out of range  I3D_animation_set::NumPosKeys()  this:0x%x  index:%d", this, link);
    }
    return 0;
}

// FUNCTION: LS3DF 0x10004c50
LS3D_RESULT I3D_animation_set::GetPosKey(uint link, uint key, int &time, S_vector &pos) const
{
    if (!data)
        return I3DERR_NOTLOADED;
    if (link >= data->num_anims) {
        dbgPrintf("!! LinkIndex out of range  I3D_animation_set::GetPosKey()  this:0x%x  index:%d", this, link);
        return I3DERR_INVALIDPARAMS;
    }
    const ushort *k = data->anims[link].keys;
    uint flags = *(const uint *)k;
    if (!(flags & ANIMF_POS))
        return I3DERR_INVALIDPARAMS;
    k += 2;
    if (flags & ANIMF_ROT)
        SkipTrack(k, 9);
    int n = *k;
    if (key >= n) {
        dbgPrintf("!! KeyIndex out of range  I3D_animation_set::GetPosKey()  this:0x%x  index:%d", this, key);
        return I3DERR_INVALIDPARAMS;
    }
    k++;
    time = k[key] * 40;
    k += n + key * 6;
    if (!(n & 1))
        k++;
    pos = *(const S_vector *)k;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10004d30
int I3D_animation_set::NumSclKeys(uint link) const
{
    if (data) {
        if (link < data->num_anims) {
            const ushort *k = data->anims[link].keys;
            uint flags = *(const uint *)k;
            if (flags & ANIMF_SCALE) {
                k += 2;
                if (flags & ANIMF_ROT)
                    SkipTrack(k, 9);
                if (flags & ANIMF_POS)
                    SkipTrack(k, 7);
                return *k;
            }
        } else
            dbgPrintf("!! index out of range  I3D_animation_set::NumSclKeys()  this:0x%x  index:%d", this, link);
    }
    return 0;
}

// FUNCTION: LS3DF 0x10004db0
LS3D_RESULT I3D_animation_set::GetSclKey(uint link, uint key, int &time, S_vector &scale) const
{
    if (!data)
        return I3DERR_NOTLOADED;
    if (link >= data->num_anims) {
        dbgPrintf("!! LinkIndex out of range  I3D_animation_set::GetSclKey()  this:0x%x  index:%d", this, link);
        return I3DERR_INVALIDPARAMS;
    }
    const ushort *k = data->anims[link].keys;
    uint flags = *(const uint *)k;
    if (!(flags & ANIMF_SCALE))
        return I3DERR_INVALIDPARAMS;
    k += 2;
    if (flags & ANIMF_ROT)
        SkipTrack(k, 9);
    if (flags & ANIMF_POS)
        SkipTrack(k, 7);
    int n = *k;
    if (key >= n) {
        dbgPrintf("!! KeyIndex out of range  I3D_animation_set::GetSclKey()  this:0x%x  index:%d", this, key);
        return I3DERR_INVALIDPARAMS;
    }
    k++;
    time = k[key] * 40;
    k += n + key * 6;
    if (!(n & 1))
        k++;
    scale = *(const S_vector *)k;
    return I3D_OK;
}

// The number of the frame's note keys; is_note, if given, tells whether they are notes (or string notes)
// FUNCTION: LS3DF 0x10004ea0
int I3D_animation_set::NumNoteKeys(uint link, int *is_note) const
{
    if (data) {
        if (link < data->num_anims) {
            const ushort *k = data->anims[link].keys;
            uint flags = *(const uint *)k;
            if (flags & (ANIMF_NOTE | ANIMF_NOTE_STRING)) {
                if (is_note)
                    *is_note = (flags & ANIMF_NOTE) != 0;
                k += 2;
                if (flags & ANIMF_ROT)
                    SkipTrack(k, 9);
                if (flags & ANIMF_POS)
                    SkipTrack(k, 7);
                if (flags & ANIMF_SCALE)
                    SkipTrack(k, 7);
                return *k;
            }
        } else
            dbgPrintf("!! index out of range  I3D_animation_set::NumNoteKeys()  this:0x%x  index:%d", this, link);
    }
    return 0;
}

// The key index message is GetSclKey's, in the original too
// FUNCTION: LS3DF 0x10004f50
LS3D_RESULT I3D_animation_set::GetNoteKey(uint link, uint key, int &time, short &note) const
{
    if (!data)
        return I3DERR_NOTLOADED;
    if (link >= data->num_anims) {
        dbgPrintf("!! LinkIndex out of range  I3D_animation_set::GetNoteKey()  this:0x%x  index:%d", this, link);
        return I3DERR_INVALIDPARAMS;
    }
    const ushort *k = data->anims[link].keys;
    uint flags = *(const uint *)k;
    if (!(flags & ANIMF_NOTE))
        return I3DERR_INVALIDPARAMS;
    k += 2;
    if (flags & ANIMF_ROT)
        SkipTrack(k, 9);
    if (flags & ANIMF_POS)
        SkipTrack(k, 7);
    if (flags & ANIMF_SCALE)
        SkipTrack(k, 7);
    int n = *k;
    if (key >= n) {
        dbgPrintf("!! KeyIndex out of range  I3D_animation_set::GetSclKey()  this:0x%x  index:%d", this, key);
        return I3DERR_INVALIDPARAMS;
    }
    k++;
    time = k[key] * 40;
    k += n + key;
    note = *k;
    return I3D_OK;
}

// The key index message is GetSclKey's
// FUNCTION: LS3DF 0x10005050
LS3D_RESULT I3D_animation_set::GetNoteKeyStr(uint link, uint key, int &time, const char *&note) const
{
    if (!data)
        return I3DERR_NOTLOADED;
    if (link >= data->num_anims) {
        dbgPrintf("!! LinkIndex out of range  I3D_animation_set::GetNoteKeyStr()  this:0x%x  index:%d", this, link);
        return I3DERR_INVALIDPARAMS;
    }
    const ushort *k = data->anims[link].keys;
    uint flags = *(const uint *)k;
    if (!(flags & ANIMF_NOTE_STRING))
        return I3DERR_INVALIDPARAMS;
    k += 2;
    if (flags & ANIMF_ROT)
        SkipTrack(k, 9);
    if (flags & ANIMF_POS)
        SkipTrack(k, 7);
    if (flags & ANIMF_SCALE)
        SkipTrack(k, 7);
    int n = *k;
    if (key >= n) {
        dbgPrintf("!! KeyIndex out of range  I3D_animation_set::GetSclKey()  this:0x%x  index:%d", this, key);
        return I3DERR_INVALIDPARAMS;
    }
    k++;
    time = k[key] * 40;
    k += n + key * 2;
    if (!(n & 1))
        k++;
    note = *(const char *const *)k;
    return I3D_OK;
}

// The functions of other files the animation set calls, until those are written. They come after the file's functions
// (see I3d_driver.cpp).
#pragma auto_inline(off)

// STUB: LS3DF 0x10060840
LS3D_RESULT LoadAnimation(const char *file_name, uint flags, void *cb, void *context, I3D_animation_set *set)
{
    return (LS3D_RESULT)renderState[15];
}

#pragma auto_inline(on)
