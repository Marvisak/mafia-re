#include "main.h"

C_multithread_protection::C_multithread_protection()
{
    Sleep(3000);
    InitializeCriticalSection(&section);
}

C_multithread_protection::~C_multithread_protection()
{
    DeleteCriticalSection(&section);
}

C_ref_counter::C_ref_counter() : ref_count(0), obj_count(0), dump_memory_leaks(false)
{
}

C_ref_counter::~C_ref_counter()
{
    if (this->dump_memory_leaks)
    {
        char buffer[512];
        buffer[0] = '\0';

        char smaller_buffer[128];
        if (ref_count)
        {
            wsprintfA(smaller_buffer, "Not released:  %d  reference(s) of Dta_read.\n", this->ref_count);
            strcat(buffer, smaller_buffer);
        }

        if (this->obj_count)
        {
            wsprintfA(smaller_buffer, "Not released:  %d  Dta_read(s).\n", this->obj_count);
            strcat(buffer, smaller_buffer);
        }

        if (strlen(buffer))
        {
            MessageBoxA(NULL, buffer, "Dta_read error message", 0);
        }
    }
}
