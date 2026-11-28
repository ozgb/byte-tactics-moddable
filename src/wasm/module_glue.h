#ifndef WASM_MODULE_GLUE_H
#define WASM_MODULE_GLUE_H

#include <wasmtime.h>
#include "wasmloader.h"

enum GlueWord {
    GLUE_WORD_GAME,
    GLUE_WORD_SIM,
    GLUE_WORD_EXT_TDF,
    GLUE_WORD_BLOCK_BASE,
    GLUE_WORD_REGION,
    GLUE_WORD_COUNT
};

struct GlueTiming {
    unsigned long long slotTime;
    unsigned long long callTime;
    unsigned calls;
};

typedef const char*(__cdecl* GlueImportFn)(wasmtime_val_raw_t* a);

struct GlueImport {
    const char* module;
    const char* name;
    const char* signature;
    GlueImportFn function;
};

struct GlueModule;

struct GlueEffectHook {
    int kind;
    const char* (*check)(const void* effect);
};

struct GlueRoute {
    int outbox;
    int kind;
};

struct GlueModuleDef {
    const char* name;
    int outbox;
    const char* const* slots;
    unsigned slotCount;
    wasmtime_func_t* funcs;
    GlueTiming* timing;
    void (*useSeam)(GlueModule* glue);
    void (*timeNative)();
    int blocked;
    int schedPublish;
};

struct GlueModule {
    const GlueModuleDef* def;
    wl_loader* loader;
    wl_module* module;
    wasmtime_func_t fuelSet;
    wasmtime_func_t fuelGet;
    long long fuelBudget;
    int fuelOpen;
    int fuelTick;
    unsigned words[GLUE_WORD_COUNT];
    void* region;
    const void* world;
    int bound;
};

GlueModule* const* GlueModules(unsigned* count);
const GlueImport* GlueInterfaceImports(unsigned* count);
const GlueRoute* GlueRoutes(unsigned* count);
const GlueEffectHook* GlueEffectHooks(unsigned* count);

GlueModule* __stdcall GlueFind(const char* name);
int __stdcall GlueInstall(wl_loader* loader, int timing);
int __stdcall GlueBind(GlueModule* glue, wl_loader* loader, wl_module* module, unsigned fuelBudget);
void __stdcall GlueUseSeam(GlueModule* glue);
int __stdcall GlueDeferFatal(const char* text);
int __stdcall GlueFatalPending();
int __stdcall GlueFuelClose(GlueModule* glue, unsigned* used);
int __stdcall GlueOutOfFuel(unsigned* budget, const char** module);
void __stdcall GlueDataRanges(GlueModule* glue, unsigned* addresses, unsigned* count);
void __stdcall GlueTimeNative(GlueModule* glue);
void __stdcall GlueTimingReport(char* text);

void GlueCall(GlueModule* glue, unsigned slot, wasmtime_val_raw_t* a, size_t count);
void GlueStop(GlueModule* glue, unsigned slot, const char* what);
unsigned GlueScratchMark();
void GlueScratchRelease(unsigned mark);
unsigned GlueIn(GlueModule* glue, unsigned slot, const void* native, unsigned size, int copyIn);
void GlueOut(void* native, unsigned address, unsigned size);
void GlueZero(const void* native, unsigned address, unsigned size);
void GlueInPointer(GlueModule* glue, unsigned slot, const void* native, unsigned address, unsigned offset, int text);
void GlueOutPointer(GlueModule* glue, unsigned slot, void* native, unsigned offset);
unsigned GlueObject(GlueModule* glue, unsigned slot, const void* native);
unsigned GlueText(GlueModule* glue, unsigned slot, const char* native);
void* GlueResult(GlueModule* glue, unsigned slot, unsigned address, int inBlock);
unsigned GlueHandleOf(void* native);
void GlueTimed(GlueModule* glue, unsigned slot, unsigned long long start);
GlueModule* GlueActive();

int GlueInMemory(unsigned address);
void* GlueLinear(unsigned address);
const char* GlueImportError(const char* import, const char* what);
const char* GlueNativeOf(const char* import, unsigned handle, void** native);
const char* GlueLiveOf(const char* import, unsigned handle, void** native);
const char* GlueAddressOf(const char* import, const void* native, int inBlock, unsigned* address, const char* noun);

#define GLUE_NEED(import, slot) \
    if (!GlueInMemory((unsigned)a[slot].i32)) \
        return GlueImportError(import, "pointer argument " #slot " is outside the module memory")

#endif
