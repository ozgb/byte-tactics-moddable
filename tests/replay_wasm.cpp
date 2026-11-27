#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../interfaces/host.h"
#include "../src/state/block.h"
#include "../src/wasm/modules.h"
#include "replay_wasm.h"

namespace {

struct Glue {
    const char* subsystem;
    int(__stdcall* install)(wl_loader* loader, int timing);
    int(__stdcall* bind)(wl_loader* loader, wl_module* module, unsigned fuelBudget);
    void(__stdcall* useSeam)();
    int(__stdcall* fuelClose)(unsigned* used);
    int(__stdcall* fatalPending)();
    int(__stdcall* outOfFuel)(unsigned* budget);
};

const Glue kGlue[] = {
    { "wind_meteors", WindMeteorsInstall, WindMeteorsBind, WindMeteorsUseSeam, WindMeteorsFuelClose, WindMeteorsFatalPending, WindMeteorsOutOfFuel },
};

ModuleMemory s_memory;
const Glue* s_glue;
char s_wasmPath[320];
char s_directory[320];
wl_loader* s_loader;
int s_active;
void (*s_trapHook)(const char* errorClass, const char* text);

}

const char* ReplayWasmRefusal(const char* text)
{
    for (const char* p = strstr(text, "import "); p != nullptr; p = strstr(p + 1, "import "))
        if (p == text || p[-1] == ' ' || p[-1] == '\n')
            return p;
    return nullptr;
}

namespace {

const char* TrapClass(const char* text, bool outOfFuel)
{
    if (outOfFuel || strstr(text, "ran out of fuel") != nullptr)
        return "out_of_fuel";
    if (ReplayWasmRefusal(text) != nullptr)
        return "import";
    if (strstr(text, ": the handle table is full") != nullptr)
        return "glue";
    if (strstr(text, "unreachable") != nullptr)
        return "unreachable";
    if (strstr(text, "out of bounds memory") != nullptr)
        return "memory_access";
    if (strstr(text, "divide by zero") != nullptr || strstr(text, "integer overflow") != nullptr)
        return "divide";
    if (strstr(text, "indirect call") != nullptr || strstr(text, "uninitialized element") != nullptr)
        return "table";
    if (strstr(text, "stack") != nullptr)
        return "stack";
    return "trap";
}

void Stop(const char* what)
{
    printf("FAIL: %s: %s\n", s_wasmPath, what);
    exit(2);
}

void __cdecl OnTrap(void* user, const char* module, const char* message)
{
    char text[700];
    (void)user;
    if (s_glue != nullptr && s_glue->fatalPending())
        return;
    if (s_trapHook != nullptr) {
        unsigned budget;
        s_trapHook(TrapClass(message, s_glue != nullptr && s_glue->outOfFuel(&budget) != 0), message);
    }
    sprintf(text, "%s trapped: %s", module, message);
    Stop(text);
}

unsigned char* ReadModule(size_t* size)
{
    FILE* file = fopen(s_wasmPath, "rb");
    if (file == nullptr)
        Stop("cannot read the module; build the wasm_runtime target (and wasm_runtime_inject for the injected one)");
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    unsigned char* bytes = (unsigned char*)malloc((size_t)length);
    if (length <= 0 || fread(bytes, 1, (size_t)length, file) != (size_t)length)
        Stop("cannot read the module");
    fclose(file);
    *size = (size_t)length;
    return bytes;
}

void Release()
{
    if (s_loader != nullptr) {
        SimBlockAttach(nullptr);
        SimBlockReset();
        wl_destroy(s_loader);
        s_loader = nullptr;
    }
}

void Check(int ok)
{
    if (!ok)
        Stop(s_loader == nullptr ? "the loader failed" : wl_error(s_loader));
}

}

const ModuleMemory* __stdcall ModulesMemory()
{
    return &s_memory;
}

void __stdcall ModulesFatal(const char* text)
{
    if (s_trapHook != nullptr)
        s_trapHook(TrapClass(text, false), text);
    printf("FAIL: %s: %s\n", s_wasmPath, text);
    exit(2);
}

extern "C" void HOST_CALL host_log(const char* text)
{
    fputs(text, stdout);
}

void ReplayWasmSetTrapHook(void (*hook)(const char* errorClass, const char* text))
{
    s_trapHook = hook;
}

void ReplayWasmSelect(const char* wasmPath, const char* subsystem)
{
    if (strlen(wasmPath) >= sizeof s_wasmPath)
        Stop("the module path is too long");
    strcpy(s_wasmPath, wasmPath);
    strcpy(s_directory, wasmPath);
    size_t length = strlen(s_directory);
    while (length > 0 && s_directory[length - 1] != '\\' && s_directory[length - 1] != '/')
        length--;
    s_directory[length] = '\0';
    const char* stem = s_wasmPath + length;
    s_glue = nullptr;
    for (const Glue& glue : kGlue) {
        size_t n = strlen(glue.subsystem);
        if (subsystem != nullptr ? strcmp(subsystem, glue.subsystem) == 0 : strncmp(stem, glue.subsystem, n) == 0 && (stem[n] == '.' || stem[n] == '_'))
            s_glue = &glue;
    }
    if (s_glue == nullptr)
        Stop("no WASM glue entry for the subsystem this module file is named after");
    s_active = 1;
}

const char* ReplayWasmSubsystem()
{
    return s_glue == nullptr ? nullptr : s_glue->subsystem;
}

bool ReplayWasmActive()
{
    return s_active != 0;
}

void ReplayWasmReload()
{
    char dll[360];
    char manifest[360];
    char error[400];
    size_t size = 0;
    unsigned used;
    if (s_loader == nullptr && SimBlockBase() != nullptr)
        SimBlockReset();
    Release();
    unsigned char* bytes = ReadModule(&size);
    sprintf(dll, "%swasmtime_host.dll", s_directory);
    sprintf(manifest, "%smodules.json", s_directory);

    wl_config config;
    memset(&config, 0, sizeof config);
    config.dll_path = dll;
    config.open = host_lib_open;
    config.symbol = host_lib_symbol;
    config.close = host_lib_close;
    config.on_trap = OnTrap;
    s_loader = wl_create(&config, error, sizeof error);
    if (s_loader == nullptr)
        Stop(error);
    Check(wl_load_manifest_file(s_loader, manifest));
    const wl_layout* layout = wl_get_layout(s_loader);
    if (!layout->has_block || layout->block_size != kSimBlockBytes || layout->block_max != kSimBlockCeilingBytes)
        Stop("the manifest's block does not match the sim block");
    Check(wl_create_memory(s_loader));
    s_memory.base = wl_memory_base(s_loader);
    s_memory.size = (unsigned)wl_memory_size(s_loader);
    s_memory.scratch = layout->scratch_base;
    s_memory.scratchSize = layout->scratch_size;
    SimBlockAttach(s_memory.base + layout->block_base);

    Check(s_glue->install(s_loader, 0));
    wl_module* module = wl_load_module_bytes(s_loader, s_glue->subsystem, bytes, size);
    Check(module != nullptr);
    free(bytes);
    long fuelBudget = 0;
    if (!wl_module_int(s_loader, s_glue->subsystem, "fuel_budget", &fuelBudget) || fuelBudget <= 0)
        Stop("the manifest has no fuel_budget");
    Check(s_glue->bind(s_loader, module, (unsigned)fuelBudget));
    s_glue->fuelClose(&used);
}

void ReplayWasmUseSeam()
{
    s_glue->useSeam();
}
