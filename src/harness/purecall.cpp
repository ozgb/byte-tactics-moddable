#include <stdio.h>

#include "../state/alloc_hooks.h"
#include "../../interfaces/ref.h"

extern "C" void __cdecl PurecallAmsgExit(int);
#pragma comment(linker, "/alternatename:_PurecallAmsgExit=__amsg_exit")

extern "C" int __cdecl _purecall(void)
{
    if (HarnessTestMode()) {
        FILE* f = fopen("crash.txt", "ab");
        if (f != NULL) {
            fprintf(f, "purecall from 0x%08x\n", (unsigned)_ReturnAddress());
            fclose(f);
        }
    }
    PurecallAmsgExit(25);
    return 0;
}
