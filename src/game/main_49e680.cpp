#include <excpt.h>
#include "win_types.h"
#include "../../interfaces/host.h"
#include "main_49e680.h"

// FUNCTION: 0x49e680
void __stdcall ReportViaException(const char* message)
{
    int zero = 0;
    __try {
        __asm {
            push eax
            push edx
            div zero
            pop edx
            pop eax
        }
    } __except (host_crash_report(GetExceptionInformation(), message), EXCEPTION_EXECUTE_HANDLER) {
    }
}
