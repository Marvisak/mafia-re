// The AI's segments (PS2 jp_ai_segments.cpp; the PC's file is most likely the one at 0x53e4b0-0x53ef4c,
// docs/game-files.md), in PS2's order. Written so far: the segment's accessors, AddSegment, Init, Clear and Load, which
// C_game's Init and Done call (Load reads the mission's clusters.seg and adds each segment). Open: AddSegment's inline
// decisions (its comment).

#include "ai/jp_ai_segments.h"

#include "mission/Mission.h"

#include "rw_data.h"

#include <stdio.h>
#include <string.h>

// FUNCTION: GAME 0x53e510
uint check_seg::NumSpecials()
{
    return flags >> 28;
}

// FUNCTION: GAME 0x53e520
void check_seg::GetType(uint *type, void **data)
{
    *type = flags & 0xf;
    *data = this->data;
}

// FUNCTION: GAME 0x53e540
void check_seg::GetSpecial(uint index, uint *type, void **data)
{
    *type = (flags >> (index * 4 + 4)) & 0xf;
    *data = specials[index];
}

// Copies the segment into the last cluster's first free slot, or into a new cluster, and takes its data: the segment
// is left empty. Open: the grown copy's vector clear calls _Destroy out of line and its push_back loads its operands in
// the other order (the final clear written as erase is what keeps the last push_back's insert out of line, as the
// original has it; clear() there inlines it).
// FUNCTION: GAME 0x53e6e0
void check_seg_clusters::AddSegment(check_seg *seg)
{
    check_seg_cluster *last = &clusters[num_clusters - 1];
    check_seg *dst;
    uint i;
    for (i = 0; i < 4; i++) {
        if (!last->segs[i].flags) {
            dst = &last->segs[i];
            goto copy;
        }
    }
    {
        check_seg_cluster *grown = new check_seg_cluster[num_clusters + 1];
        for (i = 0; i < num_clusters; i++) {
            for (uint j = 0; j < 4; j++) {
                grown[i].segs[j].flags = clusters[i].segs[j].flags;
                grown[i].segs[j].pos = clusters[i].segs[j].pos;
                grown[i].segs[j].unknown_20 = clusters[i].segs[j].unknown_20;
                grown[i].segs[j].data = clusters[i].segs[j].data;
                grown[i].segs[j].specials = clusters[i].segs[j].specials;
                grown[i].segs[j].unknown_c.clear();
                for (uint k = 0; k < clusters[i].segs[j].unknown_c.size(); k++)
                    grown[i].segs[j].unknown_c.push_back(clusters[i].segs[j].unknown_c[k]);
            }
        }
        delete[] clusters;
        clusters = grown;
        dst = clusters[num_clusters].segs;
        num_clusters++;
        // The new cluster's first segment is the one added
        for (i = 1; i < 4; i++) {
            clusters[num_clusters - 1].segs[i].flags = 0;
            clusters[num_clusters - 1].segs[i].data = NULL;
            clusters[num_clusters - 1].segs[i].specials = NULL;
            clusters[num_clusters - 1].segs[i].unknown_c.clear();
        }
    }
copy:
    dst->pos = seg->pos;
    dst->unknown_20 = seg->unknown_20;
    dst->flags = seg->flags;
    dst->data = seg->data;
    dst->specials = seg->specials;
    for (i = 0; i < seg->unknown_c.size(); i++)
        dst->unknown_c.push_back(seg->unknown_c[i]);
    seg->flags = 0;
    seg->data = NULL;
    seg->specials = NULL;
    seg->unknown_c.erase(seg->unknown_c.begin(), seg->unknown_c.end());
}

// FUNCTION: GAME 0x53ea40
void check_seg_clusters::Init()
{
    clusters = new check_seg_cluster[1];
    num_clusters = 1;
    for (uint i = 0; i < 4; i++) {
        clusters->segs[i].flags = 0;
        clusters->segs[i].data = NULL;
        clusters->segs[i].specials = NULL;
        clusters->segs[i].unknown_c.clear();
    }
}

// FUNCTION: GAME 0x53eb00
void check_seg_clusters::Clear()
{
    for (uint i = 0; i < num_clusters; i++) {
        for (int j = 0; j < 4; j++) {
            if (clusters[i].segs[j].data) {
                uint type;
                void *data;
                clusters[i].segs[j].GetType(&type, &data);
                switch (type) {
                case CHECK_SEG_UNKNOWN_1:
                    delete (check_seg_data_1 *)clusters[i].segs[j].data;
                    break;
                case CHECK_SEG_UNKNOWN_2:
                    delete (check_seg_data_2 *)clusters[i].segs[j].data;
                    break;
                case CHECK_SEG_UNKNOWN_4:
                    delete (check_seg_data_4 *)clusters[i].segs[j].data;
                    break;
                }
                clusters[i].segs[j].data = NULL;
            }
            uint num = clusters[i].segs[j].NumSpecials();
            for (uint k = 0; k < num; k++)
                delete clusters[i].segs[j].specials[k];
            delete clusters[i].segs[j].specials;
            clusters[i].segs[j].specials = NULL;
            clusters[i].segs[j].flags = 0;
            clusters[i].segs[j].unknown_c.clear();
        }
    }
    delete[] clusters;
    clusters = NULL;
    num_clusters = 0;
}

// FUNCTION: GAME 0x53ec30
bool check_seg_clusters::Load()
{
    check_seg seg;
    char name[64];
    // STRING: GAME 0x636184
    sprintf(name, "Missions\\%s\\clusters.seg", act_mission->mission_name);
    Clear();
    Init();
    int file = dtaOpen(name, true);
    char header[16];
    dtaRead(file, header, sizeof(header));
    // STRING: GAME 0x636170
    if (_strcmpi(header, "AISEG-V1.1-JIPO")) {
        dtaClose(file);
        return false;
    }
    uint num;
    dtaRead(file, &num, sizeof(num));
    for (uint i = 0; i < num; i++) {
        dtaRead(file, &seg.flags, sizeof(seg.flags));
        dtaRead(file, &seg.unknown_20, sizeof(seg.unknown_20));
        dtaRead(file, &seg.pos, sizeof(seg.pos));
        uint count;
        dtaRead(file, &count, sizeof(count));
        seg.unknown_c.clear();
        uint value;
        uint j;
        for (j = 0; j < count; j++) {
            dtaRead(file, &value, sizeof(value));
            seg.unknown_c.push_back(value);
        }
        void *data;
        seg.GetType(&value, &data);
        switch (value) {
        case CHECK_SEG_UNKNOWN_1:
            seg.data = new uchar[sizeof(check_seg_data_1)];
            dtaRead(file, seg.data, sizeof(check_seg_data_1));
            break;
        case CHECK_SEG_UNKNOWN_2:
            seg.data = new uchar[sizeof(check_seg_data_2)];
            dtaRead(file, seg.data, sizeof(check_seg_data_2));
            break;
        case CHECK_SEG_UNKNOWN_4:
            seg.data = new uchar[sizeof(check_seg_data_4)];
            dtaRead(file, seg.data, sizeof(check_seg_data_4));
            break;
        }
        count = seg.NumSpecials();
        seg.specials = new void *[count];
        for (j = 0; j < count; j++) {
            seg.GetSpecial(j, &value, &data);
            switch (value) {
            case CHECK_SEG_UNKNOWN_1:
                seg.specials[j] = new uchar[sizeof(check_seg_special_1)];
                dtaRead(file, seg.specials[j], sizeof(check_seg_special_1));
                break;
            case CHECK_SEG_UNKNOWN_2:
                seg.specials[j] = new uchar[sizeof(check_seg_special_2)];
                dtaRead(file, seg.specials[j], sizeof(check_seg_special_2));
                ((check_seg_special_2 *)seg.specials[j])->unknown_30 = 0;
                ((check_seg_special_2 *)seg.specials[j])->unknown_34 = 0;
                break;
            case CHECK_SEG_UNKNOWN_4:
                seg.specials[j] = new uchar[sizeof(check_seg_special_4)];
                dtaRead(file, seg.specials[j], sizeof(check_seg_special_4));
                break;
            case CHECK_SEG_UNKNOWN_8:
                seg.specials[j] = new uchar[sizeof(check_seg_special_8)];
                dtaRead(file, seg.specials[j], sizeof(check_seg_special_8));
                break;
            }
        }
        AddSegment(&seg);
    }
    dtaClose(file);
    return true;
}
