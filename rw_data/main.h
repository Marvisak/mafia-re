#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <vector>
#include <types.h>

class C_multithread_protection
{
public:
    CRITICAL_SECTION section;

    C_multithread_protection();
    ~C_multithread_protection();

    void Lock()
    {
        EnterCriticalSection(&section);
    }

    void Unlock()
    {
        LeaveCriticalSection(&section);
    }
};

class C_ref_counter
{
public:
    C_ref_counter();
    ~C_ref_counter();

    void AddRef()
    {
        this->ref_count++;
    }

    void Release()
    {
        --this->ref_count;
    }

    void AddObject()
    {
        ++this->obj_count;
    }

    void ReleaseObject()
    {
        --this->obj_count;
    }

private:
    int ref_count;
    int obj_count;
    bool dump_memory_leaks;
};

extern C_multithread_protection C_m_p;
extern C_ref_counter C_r_f;