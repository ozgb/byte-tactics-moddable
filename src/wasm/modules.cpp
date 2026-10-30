#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

#include "../../interfaces/host.h"
#include "../state/block.h"
#include "../game/los_helpers.h"
#include "wasmloader.h"
#include "modules.h"

#pragma init_seg(lib)

namespace {

ModuleMemory s_memory;
const char kModuleName[] = "wind_meteors";

int s_wasm;
int s_testRun;
wl_loader* s_loader;
unsigned s_staticsLow;
unsigned s_staticsHigh;
int s_haveBaseline;
unsigned s_baselineLow;
unsigned s_baselineHigh;
unsigned s_fuelMax;
unsigned s_fuelMaxTick;
int s_haveFuel;
int s_timing;
int s_timingStarted;
unsigned s_timingMs;
unsigned s_timingFirstTick;
unsigned long long s_timingCounter;

void WhereText(char* text)
{
    if (g_game != nullptr)
        sprintf(text + strlen(text), " at tick %d", g_game->ticks);
    else
        strcat(text, " before the match");
}

void __cdecl OnTrap(void* user, const char* module, const char* message)
{
    char text[700];
    unsigned budget;
    (void)user;
    if (WindMeteorsFatalPending())
        return;
    if (WindMeteorsOutOfFuel(&budget)) {
        sprintf(text, "%s ran out of fuel", module);
        WhereText(text);
        sprintf(text + strlen(text), ": it burned more than its fuel_budget of %u in one tick", budget);
        ModulesFatal(text);
    }
    sprintf(text, "%s trapped", module);
    WhereText(text);
    strcat(text, ": ");
    strncat(text, message, sizeof text - strlen(text) - 1);
    ModulesFatal(text);
}

void LoaderFatal(const char* what)
{
    char text[700];
    sprintf(text, "%s", kModuleName);
    WhereText(text);
    strcat(text, ": ");
    strncat(text, what, sizeof text - strlen(text) - 1);
    ModulesFatal(text);
}

int HasSwitch(char* cmdLine, const char* name, char* value, int size)
{
    size_t length = strlen(name);
    for (char* p = cmdLine; *p != '\0'; p++) {
        if ((p[0] == '-' || p[0] == '/') && _strnicmp(p + 1, name, length) == 0 && p[1 + length] == '='
            && (p == cmdLine || isspace((unsigned char)p[-1]))) {
            char* q = p + 2 + length;
            char* start = q;
            while (*q != '\0' && !isspace((unsigned char)*q))
                q++;
            int n = (int)(q - start);
            if (n >= size)
                n = size - 1;
            memcpy(value, start, n);
            value[n] = '\0';
            memset(p, ' ', q - p);
            return 1;
        }
    }
    return 0;
}

long ReadSigned(const unsigned char* data, size_t size, size_t* at, int* ok)
{
    long result = 0;
    int shift = 0;
    unsigned char byte = 0;
    while (*at < size) {
        byte = data[(*at)++];
        result |= (long)(byte & 0x7f) << shift;
        shift += 7;
        if ((byte & 0x80) == 0) {
            if (shift < 32 && (byte & 0x40) != 0)
                result |= -(1L << shift);
            return result;
        }
    }
    *ok = 0;
    return 0;
}

unsigned long ReadUnsigned(const unsigned char* data, size_t size, size_t* at, int* ok)
{
    unsigned long result = 0;
    int shift = 0;
    while (*at < size) {
        unsigned char byte = data[(*at)++];
        result |= (unsigned long)(byte & 0x7f) << shift;
        shift += 7;
        if ((byte & 0x80) == 0)
            return result;
    }
    *ok = 0;
    return 0;
}

int StackTop(const unsigned char* data, size_t size, unsigned* top)
{
    size_t at = 8;
    int ok = 1;
    if (size < 8 || memcmp(data, "\0asm\1\0\0\0", 8) != 0)
        return 0;
    while (at < size && ok) {
        unsigned char section = data[at++];
        unsigned long length = ReadUnsigned(data, size, &at, &ok);
        size_t body = at;
        if (section == 6) {
            unsigned long count = ReadUnsigned(data, size, &at, &ok);
            if (!ok || count == 0 || at + 3 > size || data[at + 2] != 0x41)
                return 0;
            at += 3;
            *top = (unsigned)ReadSigned(data, size, &at, &ok);
            return ok;
        }
        at = body + length;
    }
    return 0;
}

unsigned char* ReadFile(const char* path, size_t* size)
{
    FILE* file = fopen(path, "rb");
    if (file == nullptr)
        return nullptr;
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (length <= 0) {
        fclose(file);
        return nullptr;
    }
    unsigned char* data = (unsigned char*)malloc((size_t)length);
    if (fread(data, 1, (size_t)length, file) != (size_t)length) {
        fclose(file);
        free(data);
        return nullptr;
    }
    fclose(file);
    *size = (size_t)length;
    return data;
}

void Start()
{
    char exe[300];
    char dll[320];
    char manifest[320];
    char wasm[320];
    char error[400];
    char text[700];
    unsigned length = host_file_exe_path(exe, sizeof exe - 1);
    while (length > 0 && exe[length - 1] != '\\' && exe[length - 1] != '/')
        length--;
    exe[length] = '\0';
    sprintf(dll, "%swasmtime_host.dll", exe);
    sprintf(manifest, "%smodules.json", exe);
    sprintf(wasm, "%s%s.wasm", exe, kModuleName);

    size_t size = 0;
    unsigned char* bytes = ReadFile(wasm, &size);
    if (bytes == nullptr) {
        sprintf(text, "%s is missing: build the wasm_runtime target (cmake --build <dir> --target wasm_runtime), or run with -modules=native", wasm);
        ModulesFatal(text);
    }
    if (SimBlockBase() != nullptr)
        ModulesFatal("the sim block was reserved before the module loader started");

    wl_config config;
    memset(&config, 0, sizeof config);
    config.dll_path = dll;
    config.open = host_lib_open;
    config.symbol = host_lib_symbol;
    config.close = host_lib_close;
    config.on_trap = OnTrap;
    s_loader = wl_create(&config, error, sizeof error);
    if (s_loader == nullptr) {
        sprintf(text, "%s: %s", kModuleName, error);
        ModulesFatal(text);
    }
    if (!wl_load_manifest_file(s_loader, manifest))
        LoaderFatal(wl_error(s_loader));
    const wl_layout* layout = wl_get_layout(s_loader);
    if (!layout->has_block || layout->block_size != kSimBlockBytes)
        LoaderFatal("the manifest's block does not match the game's sim block");
    if (!wl_create_memory(s_loader))
        LoaderFatal(wl_error(s_loader));
    s_memory.base = wl_memory_base(s_loader);
    s_memory.size = (unsigned)wl_memory_size(s_loader);
    s_memory.scratch = layout->scratch_base;
    s_memory.scratchSize = layout->scratch_size;
    SimBlockAttach(s_memory.base + layout->block_base);

    if (!WindMeteorsInstall(s_loader, s_timing))
        LoaderFatal(wl_error(s_loader));
    wl_module* module = wl_load_module_bytes(s_loader, kModuleName, bytes, size);
    unsigned top = 0;
    long dataBase = 0;
    if (module == nullptr)
        LoaderFatal(wl_error(s_loader));
    if (!StackTop(bytes, size, &top) || !wl_module_int(s_loader, kModuleName, "data_base", &dataBase))
        LoaderFatal("cannot find the module's data base or stack top");
    free(bytes);
    s_staticsLow = (unsigned)dataBase;
    s_staticsHigh = top - layout->stack_size;
    long fuelBudget = 0;
    if (!wl_module_int(s_loader, kModuleName, "fuel_budget", &fuelBudget) || fuelBudget <= 0)
        LoaderFatal("the manifest has no fuel_budget");
    if (!WindMeteorsBind(s_loader, module, (unsigned)fuelBudget))
        LoaderFatal(wl_error(s_loader));
}

}

const ModuleMemory* __stdcall ModulesMemory()
{
    return &s_memory;
}

void __stdcall ModulesFatal(const char* text)
{
    host_log(text);
    host_log("\n");
    if (s_testRun)
        host_exit(2);
    host_fatal(nullptr, "Byte Tactics", text);
}

void __stdcall ModulesHideSwitch(char* cmdLine)
{
    char mode[16];
    HasSwitch(cmdLine, "modules", mode, sizeof mode);
    if (s_wasm)
        WindMeteorsUseSeam();
}

namespace {

struct Starter {
    Starter()
    {
        char line[1024];
        char mode[16];
        strncpy(line, host_command_line(), sizeof line - 1);
        line[sizeof line - 1] = '\0';
        s_testRun = strstr(line, "testmode") != nullptr;
        if (s_testRun) {
            FILE* on = fopen("moduletime.on", "rb");
            s_timing = on != nullptr;
            if (on != nullptr)
                fclose(on);
        }
        s_wasm = 1;
        if (HasSwitch(line, "modules", mode, sizeof mode)) {
            if (_stricmp(mode, "native") == 0)
                s_wasm = 0;
            else if (_stricmp(mode, "wasm") != 0)
                ModulesFatal("-modules: expected native or wasm");
        }
        if (s_wasm)
            Start();
    }
};

Starter s_starter;

}

namespace {

void WriteTiming(unsigned tick)
{
    static char body[8192];
    unsigned long long overheadStart = host_time_counter();
    for (int i = 0; i < 20000; i++)
        host_time_counter();
    unsigned long long overhead = (host_time_counter() - overheadStart) / 20000;
    unsigned long long counter = host_time_counter() - s_timingCounter;
    unsigned ms = host_time_ms() - s_timingMs;
    WindMeteorsTimingReport(body);
    FILE* file = fopen("moduletime.txt", "wb");
    if (file != nullptr) {
        fprintf(file, "bt-moduletime 1\nmode\t%s\nticks\t%u\nwall_counter\t%I64u\nwall_ms\t%u\ntimer_cost\t%I64u\n%s",
                s_wasm ? "wasm" : "native", tick - s_timingFirstTick + 1, counter, ms, overhead, body);
        fclose(file);
    } else {
        ModulesFatal("testmode: cannot write moduletime.txt");
    }
}

void TimingTick(unsigned tick)
{
    if (!s_timingStarted) {
        s_timingStarted = 1;
        s_timingFirstTick = tick;
        s_timingMs = host_time_ms();
        s_timingCounter = host_time_counter();
        if (!s_wasm)
            WindMeteorsTimeNative();
    }
    if (tick % 600 == 0)
        WriteTiming(tick);
}

}

void __stdcall ModulesCheckStatics(unsigned tick)
{
    if (s_timing)
        TimingTick(tick);
    if (!s_wasm)
        return;
    unsigned used;
    if (WindMeteorsFuelClose(&used) && (!s_haveFuel || used > s_fuelMax)) {
        s_haveFuel = 1;
        s_fuelMax = used;
        s_fuelMaxTick = tick;
        FILE* file = fopen("fuel.txt", "wb");
        if (file != nullptr) {
            fprintf(file, "bt-fuel 1\n%s\t%u\t%u\n", kModuleName, s_fuelMax, s_fuelMaxTick);
            fclose(file);
        } else {
            ModulesFatal("testmode: cannot write fuel.txt");
        }
    }
    unsigned skip[4];
    unsigned skipCount;
    WindMeteorsDataRanges(skip, &skipCount);
    unsigned lo = 0x84222325;
    unsigned hi = 0xcbf29ce4;
    for (unsigned at = s_staticsLow; at < s_staticsHigh; at++) {
        int skipped = 0;
        for (unsigned i = 0; i < skipCount; i++)
            skipped |= at >= skip[i] && at < skip[i] + 4;
        if (skipped)
            continue;
        lo ^= s_memory.base[at];
        unsigned long long product = (unsigned long long)lo * 0x1b3u;
        lo = (unsigned)product;
        hi = hi * 0x1b3u + (unsigned)(product >> 32);
    }
    if (!s_haveBaseline) {
        s_haveBaseline = 1;
        s_baselineLow = lo;
        s_baselineHigh = hi;
        return;
    }
    if (lo != s_baselineLow || hi != s_baselineHigh) {
        char text[200];
        sprintf(text, "%s kept state between ticks: its statics changed at tick %u", kModuleName, tick);
        ModulesFatal(text);
    }
}
