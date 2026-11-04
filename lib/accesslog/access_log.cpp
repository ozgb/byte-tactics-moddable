#include <windows.h>
#include "access_log.h"

#define NS __attribute__((no_sanitize("address")))

namespace {

const int kWindows = 12;
volatile int g_on;
int g_window = ACCESS_WINDOW_OFF;
unsigned g_loads[kWindows];
unsigned g_stores[kWindows];
unsigned g_simThread;

NS inline unsigned ThreadId()
{
    unsigned id;
    __asm__("movl %%fs:0x24, %0" : "=r"(id));
    return id;
}

NS inline void Touch(int store, void* p, unsigned size, void* ra)
{
    (void)p;
    (void)size;
    (void)ra;
    if (!g_on || ThreadId() != g_simThread)
        return;
    if (store)
        g_stores[g_window]++;
    else
        g_loads[g_window]++;
}

NS void Range(int store, const void* p, unsigned n, void* ra)
{
    if (n != 0)
        Touch(store, (void*)p, n, ra);
}

}

#define ACCESS_CALLBACK(N)                                                                \
    extern "C" NS void __cdecl __asan_load##N(void* p) { Touch(0, p, N, __builtin_return_address(0)); } \
    extern "C" NS void __cdecl __asan_store##N(void* p) { Touch(1, p, N, __builtin_return_address(0)); }
ACCESS_CALLBACK(1)
ACCESS_CALLBACK(2)
ACCESS_CALLBACK(4)
ACCESS_CALLBACK(8)
ACCESS_CALLBACK(16)

extern "C" NS void __cdecl __asan_loadN(void* p, unsigned n) { Range(0, p, n, __builtin_return_address(0)); }
extern "C" NS void __cdecl __asan_storeN(void* p, unsigned n) { Range(1, p, n, __builtin_return_address(0)); }

extern "C" NS void* __cdecl __asan_memcpy(void* d, const void* s, unsigned n)
{
    Range(0, s, n, __builtin_return_address(0));
    Range(1, d, n, __builtin_return_address(0));
    void* r = d;
    __asm__ volatile("rep movsb" : "+D"(d), "+S"(s), "+c"(n)::"memory");
    return r;
}

extern "C" NS void* __cdecl __asan_memset(void* d, int c, unsigned n)
{
    Range(1, d, n, __builtin_return_address(0));
    void* r = d;
    __asm__ volatile("rep stosb" : "+D"(d), "+c"(n) : "a"(c) : "memory");
    return r;
}

extern "C" NS void* __cdecl __asan_memmove(void* d, const void* s, unsigned n)
{
    Range(0, s, n, __builtin_return_address(0));
    Range(1, d, n, __builtin_return_address(0));
    void* r = d;
    if ((unsigned)d <= (unsigned)s) {
        __asm__ volatile("rep movsb" : "+D"(d), "+S"(s), "+c"(n)::"memory");
    } else {
        char* dd = (char*)d + n - 1;
        const char* ss = (const char*)s + n - 1;
        __asm__ volatile("std\n rep movsb\n cld" : "+D"(dd), "+S"(ss), "+c"(n)::"memory");
    }
    return r;
}

extern "C" NS void __cdecl __asan_init(void) {}
extern "C" NS void __cdecl __asan_version_mismatch_check_v8(void) {}
extern "C" NS void __cdecl __asan_report_load1(void*) {}
extern "C" NS void __cdecl __asan_after_dynamic_init(void) {}
extern "C" NS void __cdecl __asan_before_dynamic_init(const char*) {}
extern "C" NS void __cdecl __asan_alloca_poison(unsigned, unsigned) {}
extern "C" NS void __cdecl __asan_allocas_unpoison(unsigned, unsigned) {}
extern "C" NS void __cdecl __asan_handle_no_return(void) {}
extern "C" NS void __cdecl __asan_set_shadow_00(unsigned, unsigned) {}
extern "C" NS void __cdecl __asan_set_shadow_f8(unsigned, unsigned) {}

NS void AccessLogInit() {}

NS void AccessLogWindow(int window)
{
    g_window = window;
    g_on = window != ACCESS_WINDOW_OFF;
    if (g_on && g_simThread == 0)
        g_simThread = ThreadId();
}

NS void AccessLogMarker(int phase, unsigned tick)
{
    (void)tick;
    if (g_window == ACCESS_WINDOW_PENDING)
        g_window = phase;
    g_on = 0;
}

NS void AccessLogFinish() { g_on = 0; }

NS void AccessLogEmit(int kind, void* ra)
{
    (void)kind;
    (void)ra;
}

NS void AccessLogDraw(int stream, void* ra)
{
    (void)stream;
    (void)ra;
}
