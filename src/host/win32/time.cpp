#include <windows.h>
#include <time.h>

#include "../../../interfaces/host.h"

extern "C" unsigned int HOST_CALL host_time_ms(void)
{
    return GetTickCount();
}

extern "C" unsigned long long HOST_CALL host_time_counter(void)
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (unsigned long long)counter.QuadPart;
}

extern "C" unsigned int HOST_CALL host_wall_seconds(void)
{
    return (unsigned int)time(nullptr);
}

// FUNCTION: 0x4b6b50
extern "C" void HOST_CALL host_sleep_ms(unsigned int ms)
{
    Sleep(ms);
}
