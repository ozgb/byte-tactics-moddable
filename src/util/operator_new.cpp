#include <stddef.h>

#include "../state/block.h"

void* __cdecl GameAllocThunk(size_t size);
#include "../debug/debug_lib.h"

// FUNCTION: 0x4b4f10
__declspec(noinline) void* __cdecl operator new(size_t size)
{
    AllocSite site(_ReturnAddress(), 0);
    return GameAllocThunk(size);
}

// FUNCTION: 0x4b4f20
void __cdecl operator delete(void* p)
{
    if (SimOwns(p)) {
        SimFree(p);
        return;
    }
    GameFreeIndirectThunk((int *)p);
}

// FUNCTION: 0x4b4f30 ??_U@YAPAXI@Z
__declspec(noinline) void* __cdecl operator new[](size_t size)
{
    AllocSite site(_ReturnAddress(), 0);
    return GameAllocThunk(size);
}

// FUNCTION: 0x4b4f40 ??_V@YAXPAX@Z
void __cdecl operator delete[](void* p)
{
    if (SimOwns(p)) {
        SimFree(p);
        return;
    }
    GameFreeIndirectThunk((int *)p);
}
