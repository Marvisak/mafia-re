// The cutscenes' records of an actor (PC only, the file's name is ours, after its class's; the PC's file is the one at
// 0x5c25b0-0x5c283c, docs/game-files.md): the records' Reset and walk, then C_actor's functions on its records, the
// blocks' allocation and the flags the cutscene asks the actor for. Open: the original's file starts with a zero
// vector's initializer (0x5c25b0, .bss 0x647f38), as C_car.cpp's and C_recorder.cpp's do, which ours leaves to the
// engine's header (CLAUDE.md, "The engine's headers").

#include "actors/C_actor.h"
#include "mission/C_recorder.h"

// Drops the records' blocks from the walks (their memory stays): nothing for no records
// FUNCTION: GAME 0x5c25e0
void C_cutscene_records::Reset()
{
    if (this)
        first_block = NULL;
}

// The size of the records up to the last key, its record included, over all the blocks (the name is ours), which the
// recorder's saves call: C_actor::GetRecordsSize on the records themselves
// FUNCTION: GAME 0x5c25f0
int C_cutscene_records::GetRecordsSize()
{
    int size = 0;
    if (!first_block)
        return 0;
    uchar *block = first_block;
    S_cutscene_record *rec = (S_cutscene_record *)(block + 8);
    while (rec->time < last_key_time)
    {
        rec = (S_cutscene_record *)((uchar *)rec + record_sizes[rec->type]);
        if (rec->time == RECORDS_BLOCK_END)
        {
            size += (uchar *)rec - block - 8;
            block = *(uchar **)block;
            rec = (S_cutscene_record *)(block + 8);
        }
    }
    if (rec->time == last_key_time)
        rec = (S_cutscene_record *)((uchar *)rec + record_sizes[rec->type]);
    return (uchar *)rec - block - 8 + size;
}

// The size of a block's records, up to the last key or the block's end, the last one included
// FUNCTION: GAME 0x5c2650
int C_actor::GetBlockRecordsSize(uchar *block)
{
    S_cutscene_record *rec = (S_cutscene_record *)(block + 8);
    while (rec->time < records->last_key_time)
    {
        rec = (S_cutscene_record *)((uchar *)rec + records->record_sizes[rec->type]);
        if (rec->time == RECORDS_BLOCK_END)
            break;
    }
    return (uchar *)rec + records->record_sizes[rec->type] - block - 8;
}

// The size of the records up to the last key, its record included, over all the blocks
// FUNCTION: GAME 0x5c26a0
int C_actor::GetRecordsSize()
{
    int size = 0;
    C_cutscene_records *r = records;
    uchar *block = r->first_block;
    if (!block)
        return 0;
    S_cutscene_record *rec = (S_cutscene_record *)(block + 8);
    while (rec->time < r->last_key_time)
    {
        rec = (S_cutscene_record *)((uchar *)rec + r->record_sizes[rec->type]);
        if (rec->time == RECORDS_BLOCK_END)
        {
            size += (uchar *)rec - block - 8;
            block = *(uchar **)block;
            rec = (S_cutscene_record *)(block + 8);
        }
    }
    if (rec->time == r->last_key_time)
        rec = (S_cutscene_record *)((uchar *)rec + r->record_sizes[rec->type]);
    return (uchar *)rec - block - 8 + size;
}

// A new block of 0x1800 bytes: the next block's pointer (none yet), the end its records stay below (AddRecord's)
// FUNCTION: GAME 0x5c2700
uchar *C_actor::NewRecordBlock(uchar *prev)
{
    uchar *block = new uchar[0x1800];
    *(uchar **)(block + 4) = block + 0xff0;
    *(uchar **)block = NULL;
    if (prev)
        *(uchar **)prev = block;
    return block;
}

// The first record of the first block (its type 1, whatever the type asked for), or the one after the current one;
// a record that would reach the last block's end starts a new block, the end marked 0xeeeeeeee
// FUNCTION: GAME 0x5c2730
void *C_actor::AddRecord(C_cutscene_records *rec, int type)
{
    if (!rec->first_block)
    {
        rec->first_block = NewRecordBlock(NULL);
        rec->last_block = rec->first_block;
        rec->cur = rec->first_block + 8;
        ((S_cutscene_record *)rec->cur)->type = 1;
        rec->next = rec->cur;
        return rec->cur;
    }
    uchar *p = rec->cur + rec->record_sizes[((S_cutscene_record *)rec->cur)->type];
    rec->next = p;
    rec->cur = p;
    if (p + rec->record_sizes[type] >= *(uchar **)(rec->last_block + 4))
    {
        uchar *block = NewRecordBlock(rec->last_block);
        rec->last_block = block;
        *(uint *)p = RECORDS_BLOCK_END;
        rec->next = block + 8;
        rec->cur = block + 8;
    }
    return rec->cur;
}

// Asks for the recording, unless the actor is recorded already, or takes both back
// FUNCTION: GAME 0x5c27b0
void C_actor::SetCutsceneRecord(bool on)
{
    if (on)
    {
        if (!(act_flags & ACTF_CUTSCENE_RECORDED))
            act_flags |= ACTF_CUTSCENE_RECORD;
    }
    else
        act_flags &= ~(ACTF_CUTSCENE_RECORD | ACTF_CUTSCENE_RECORDED);
}

// The same for the playing
// FUNCTION: GAME 0x5c27d0
void C_actor::SetCutscenePlay(bool on)
{
    if (on)
    {
        if (!(act_flags & ACTF_CUTSCENE_PLAYED))
            act_flags |= ACTF_CUTSCENE_PLAY;
    }
    else
        act_flags &= ~(ACTF_CUTSCENE_PLAY | ACTF_CUTSCENE_PLAYED);
}

// FUNCTION: GAME 0x5c27f0
void C_actor::UpdateCutsceneFlags()
{
    if (act_flags & ACTF_CUTSCENE_RECORD)
        act_flags = act_flags & ~ACTF_CUTSCENE_RECORD | ACTF_CUTSCENE_RECORDED;
    if (act_flags & ACTF_CUTSCENE_PLAY)
        act_flags = act_flags & ~ACTF_CUTSCENE_PLAY | ACTF_CUTSCENE_PLAYED;
}
