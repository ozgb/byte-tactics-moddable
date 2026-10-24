#include <excpt.h>
#include "win_types.h"

#include "game_load.h"
int __stdcall ExceptionFilter(EXCEPTION_POINTERS* exception, const char* thread);

// FUNCTION: 0x497c70
void __cdecl LoadThreadMain(void* param)
{
    __try {
        LoadMatch(param);
    } __except (ExceptionFilter(GetExceptionInformation(), "Load Thread")) {
    }
}
