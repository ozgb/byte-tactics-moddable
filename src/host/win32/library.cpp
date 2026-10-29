#include <windows.h>

#include "../../../interfaces/host.h"

extern "C" void* HOST_CALL host_lib_open(const char* path)
{
    return (void*)LoadLibraryA(path);
}

extern "C" void* HOST_CALL host_lib_symbol(void* lib, const char* name)
{
    return (void*)GetProcAddress((HMODULE)lib, name);
}

extern "C" void HOST_CALL host_lib_close(void* lib)
{
    FreeLibrary((HMODULE)lib);
}
