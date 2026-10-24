// FLAGS: /Od /Gy
#include <stdio.h>
#include <stdlib.h>
#include "../../interfaces/host.h"

char* __cdecl FindCommandLineSwitch(const char* option);
#include "debug_lib.h"

#include "../data/exe_only.h"

// FUNCTION: 0x4da1d0
void __cdecl InitDebugSupport(unsigned int flags)
{
    if (g_debugSupportInited)
        return;
    g_debugSupportInited = 1;
    char* option = FindCommandLineSwitch("-debughelper");
    if (option) {
        int value = 0;
        sscanf(option, "=%d", &value);
        host_debug_helper(value);
        AbortProgram();
    }
    getenv("windir");
    if (!(flags & 2))
        host_crash_install();
    if (!(flags & 4))
        SetFpuFussy();
}
