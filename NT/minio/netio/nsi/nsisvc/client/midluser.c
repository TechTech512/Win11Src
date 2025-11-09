#include <windows.h>

void * __cdecl MIDL_user_allocate(size_t size)
{
    void *memory;
    
    memory = LocalAlloc(LPTR, size);
    return memory;
}

void __cdecl MIDL_user_free(void *memory)
{
    LocalFree(memory);
}

