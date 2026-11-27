#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <excpt.h>
#include <new>
#include <wasmtime.h>
#include "wasmloader.h"

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

#include "../files/files_bank.h"
#include "../../interfaces/host.h"
#include "../util/engine_services.h"
#include "../../interfaces/services.h"
#include "../../interfaces/sim_events.h"
#include "../state/sim_globals.h"
#include "../game/los_helpers.h"
#include "../state/block.h"
#include "../../interfaces/rand_stream.h"
#include "modules.h"
#include "module_glue.h"

namespace {

wl_loader* s_loader;
char s_error[240];

const unsigned kHandleCapacity = 32;
void* s_handles[kHandleCapacity];
unsigned s_handleCount;
int s_slotDepth;
unsigned s_slotOwner;
int s_importDepth;
unsigned s_importThread;
int s_fatalPending;
char s_fatalText[700];
GlueModule* s_active;
unsigned s_scratchUsed;
int s_timing;

const char* RecordFatal(const char* text)
{
    if (!s_fatalPending) {
        strncpy(s_fatalText, text, sizeof s_fatalText - 1);
        s_fatalText[sizeof s_fatalText - 1] = '\0';
        s_fatalPending = 1;
    }
    return s_fatalText;
}

const char* ActiveName()
{
    return s_active != nullptr ? s_active->def->name : "module";
}

void ReleaseHandle(void* native)
{
    for (unsigned i = 0; i < s_handleCount; i++)
        if (s_handles[i] == native)
            s_handles[i] = nullptr;
    while (s_handleCount > 0 && s_handles[s_handleCount - 1] == nullptr)
        s_handleCount--;
}

void ReleaseRecords(TdfRecord* record)
{
    if (record == nullptr)
        return;
    ReleaseHandle(record);
    for (TdfRecord** child = record->children.first; child != nullptr && child < record->children.last; child++)
        ReleaseRecords(*child);
}

const char* FileToNative(const char* import, TdfFile* file)
{
    void* root;
    void* current;
    const char* error = GlueNativeOf(import, (unsigned)file->root, &root);
    if (error == nullptr)
        error = GlueNativeOf(import, (unsigned)file->current, &current);
    if (error != nullptr)
        return error;
    file->root = (TdfRecord*)root;
    file->current = (TdfRecord*)current;
    return nullptr;
}

void FileToHandles(TdfFile* file)
{
    file->root = (TdfRecord*)GlueHandleOf(file->root);
    file->current = (TdfRecord*)GlueHandleOf(file->current);
}

const char* BufferError(const char* import, unsigned p, int size)
{
    if (size < 1)
        return GlueImportError(import, "the buffer size is below 1");
    if ((unsigned)size > ModulesMemory()->size - p)
        return GlueImportError(import, "the buffer runs past the module memory");
    return nullptr;
}

#define NEED(import, slot) GLUE_NEED(import, slot)

#define NEED_BUFFER(import, slot, sizeSlot) \
    do { \
        NEED(import, slot); \
        const char* bufferError = BufferError(import, (unsigned)a[slot].i32, a[sizeSlot].i32); \
        if (bufferError != nullptr) \
            return bufferError; \
    } while (0)

#define IMPORT(name) const char* __cdecl name(wasmtime_val_raw_t* a)

IMPORT(GameRandImport)
{
    a[0].i32 = GameRand((RandStream)a[0].i32);
    return nullptr;
}

IMPORT(RandomIntImport)
{
    a[0].i32 = RandomInt(a[0].i32);
    return nullptr;
}

IMPORT(FatalErrorImport)
{
    NEED("FatalError", 0);
    return RecordFatal((char*)GlueLinear(a[0].i32));
}

IMPORT(BuildDataPathImport)
{
    NEED("BuildDataPath", 0);
    NEED("BuildDataPath", 1);
    NEED("BuildDataPath", 2);
    NEED("BuildDataPath", 3);
    char* result = BuildDataPath((char*)GlueLinear(a[0].i32), (const char*)GlueLinear(a[1].i32), (const char*)GlueLinear(a[2].i32), (const char*)GlueLinear(a[3].i32));
    if (result != (char*)GlueLinear(a[0].i32))
        return GlueImportError("BuildDataPath", "returned a buffer other than the one it was given");
    return nullptr;
}

IMPORT(TdfFileConstructImport)
{
    NEED("TdfFile_Construct", 0);
    new (GlueLinear(a[0].i32)) TdfFile();
    return nullptr;
}

IMPORT(TdfFileDestructImport)
{
    NEED("TdfFile_Destruct", 0);
    TdfFile* file = (TdfFile*)GlueLinear(a[0].i32);
    const char* error = FileToNative("TdfFile_Destruct", file);
    if (error != nullptr)
        return error;
    ReleaseRecords(file->root);
    ReleaseHandle(file->current);
    file->~TdfFile();
    FileToHandles(file);
    return nullptr;
}

IMPORT(TdfFileLoadFileImport)
{
    NEED("TdfFile_LoadFile", 0);
    NEED("TdfFile_LoadFile", 1);
    TdfFile* file = (TdfFile*)GlueLinear(a[0].i32);
    const char* error = FileToNative("TdfFile_LoadFile", file);
    if (error != nullptr)
        return error;
    a[0].i32 = file->LoadFile((char*)GlueLinear(a[1].i32));
    FileToHandles(file);
    return nullptr;
}

IMPORT(TdfFileSelectRecordImport)
{
    NEED("TdfFile_SelectRecord", 0);
    NEED("TdfFile_SelectRecord", 1);
    TdfFile* file = (TdfFile*)GlueLinear(a[0].i32);
    const char* error = FileToNative("TdfFile_SelectRecord", file);
    if (error != nullptr)
        return error;
    a[0].i32 = file->SelectRecord((char*)GlueLinear(a[1].i32));
    FileToHandles(file);
    return nullptr;
}

IMPORT(TdfRecordGetFieldIntImport)
{
    NEED("TdfRecord_GetFieldInt", 1);
    void* record;
    const char* error = GlueLiveOf("TdfRecord_GetFieldInt", (unsigned)a[0].i32, &record);
    if (error != nullptr)
        return error;
    a[0].i32 = ((TdfRecord*)record)->GetFieldInt((const char*)GlueLinear(a[1].i32), a[2].i32);
    return nullptr;
}

IMPORT(TdfRecordGetFieldDoubleImport)
{
    NEED("TdfRecord_GetFieldDouble", 1);
    void* record;
    const char* error = GlueLiveOf("TdfRecord_GetFieldDouble", (unsigned)a[0].i32, &record);
    if (error != nullptr)
        return error;
    a[0].f64 = ((TdfRecord*)record)->GetFieldDouble((const char*)GlueLinear(a[1].i32), a[2].f64);
    return nullptr;
}

IMPORT(TdfRecordGetFieldStringImport)
{
    NEED_BUFFER("TdfRecord_GetFieldString", 1, 3);
    NEED("TdfRecord_GetFieldString", 2);
    NEED("TdfRecord_GetFieldString", 4);
    void* record;
    const char* error = GlueLiveOf("TdfRecord_GetFieldString", (unsigned)a[0].i32, &record);
    if (error != nullptr)
        return error;
    a[0].i32 = ((TdfRecord*)record)->GetFieldString((char*)GlueLinear(a[1].i32), (char*)GlueLinear(a[2].i32), a[3].i32, (char*)GlueLinear(a[4].i32));
    return nullptr;
}

IMPORT(HapiBankOpenAccountImport)
{
    NEED("HapiBank_OpenAccount", 1);
    void* bank;
    const char* error = GlueLiveOf("HapiBank_OpenAccount", (unsigned)a[0].i32, &bank);
    if (error != nullptr)
        return error;
    a[0].i32 = ((HapiBank*)bank)->OpenAccount((char*)GlueLinear(a[1].i32));
    return nullptr;
}

IMPORT(HapiBankSetIntegerItemImport)
{
    NEED("HapiBank_SetIntegerItem", 1);
    void* bank;
    const char* error = GlueLiveOf("HapiBank_SetIntegerItem", (unsigned)a[0].i32, &bank);
    if (error != nullptr)
        return error;
    a[0].i32 = ((HapiBank*)bank)->SetIntegerItem((const char*)GlueLinear(a[1].i32), a[2].i32);
    return nullptr;
}

IMPORT(HapiBankGetIntegerItemImport)
{
    NEED("HapiBank_GetIntegerItem", 1);
    void* bank;
    const char* error = GlueLiveOf("HapiBank_GetIntegerItem", (unsigned)a[0].i32, &bank);
    if (error != nullptr)
        return error;
    a[0].i32 = ((HapiBank*)bank)->GetIntegerItem((char*)GlueLinear(a[1].i32), a[2].i32);
    return nullptr;
}

IMPORT(SimAllocImport)
{
    unsigned address;
    const char* error = GlueAddressOf("SimAlloc", SimAlloc((unsigned)a[0].i32), 1, &address, "pointer");
    a[0].i32 = (int)address;
    return error;
}

IMPORT(SimAllocTaggedImport)
{
    NEED("SimAllocTagged", 0);
    unsigned address;
    const char* error = GlueAddressOf("SimAllocTagged", SimAllocTagged((const char*)GlueLinear(a[0].i32), (unsigned)a[1].i32), 1, &address, "pointer");
    a[0].i32 = (int)address;
    return error;
}

struct LiveProbe {
    const void* address;
    int found;
};

void __cdecl ProbeLive(void* payload, unsigned int, void* context)
{
    LiveProbe* probe = (LiveProbe*)context;
    if (payload == probe->address)
        probe->found = 1;
}

IMPORT(SimFreeImport)
{
    void* p = GlueLinear((unsigned)a[0].i32);
    if (p == nullptr)
        return nullptr;
    if (!SimOwns(p))
        return GlueImportError("SimFree", "the pointer is outside the sim block");
    LiveProbe probe = {p, 0};
    SimBlockForEach(ProbeLive, &probe);
    if (probe.found)
        SimFree(p);
    return nullptr;
}

IMPORT(RecordSimEventImport)
{
    unsigned count = (unsigned)a[3].i32;
    if (count != 0) {
        NEED("RecordSimEvent", 2);
        if (count > (ModulesMemory()->size - (unsigned)a[2].i32) / sizeof(unsigned))
            return GlueImportError("RecordSimEvent", "the buffer runs past the module memory");
    }
    RecordSimEvent((SimEventKind)a[0].i32, (unsigned)a[1].i32, (const unsigned*)GlueLinear(a[2].i32), count);
    return nullptr;
}

IMPORT(SimCaptureAllocImport)
{
    NEED("SimCaptureAlloc", 1);
    if (a[1].i32 == 0) {
        a[0].i32 = 0;
        return nullptr;
    }
    if (ModulesMemory()->size - (unsigned)a[1].i32 < sizeof(unsigned))
        return GlueImportError("SimCaptureAlloc", "the buffer runs past the module memory");
    unsigned address;
    const char* error = GlueAddressOf("SimCaptureAlloc", SimCaptureAlloc((unsigned)a[0].i32, (unsigned*)GlueLinear(a[1].i32)), 0, &address, "pointer");
    a[0].i32 = (int)address;
    return error;
}

IMPORT(GetSlotDpidImport)
{
    a[0].i32 = (unsigned)a[0].i32 > 10 ? -1 : GetSlotDpid((unsigned char)a[0].i32);
    return nullptr;
}

IMPORT(FindSlotByDpidImport)
{
    a[0].i32 = FindSlotByDpid(a[0].i32);
    return nullptr;
}

const unsigned kEffectCapacity = 128;

int Routed(int outbox, int kind)
{
    unsigned count;
    const GlueRoute* routes = GlueRoutes(&count);
    for (unsigned i = 0; i < count; i++)
        if (routes[i].outbox == outbox && routes[i].kind == kind)
            return 1;
    return 0;
}

IMPORT(EmitImport)
{
    NEED("Emit", 0);
    GlueModule* glue = s_active;
    unsigned size = (unsigned)a[1].i32;
    if (size < sizeof(int) || size > kEffectCapacity)
        return GlueImportError("Emit", "the size is outside an effect");
    if (size > ModulesMemory()->size - (unsigned)a[0].i32)
        return GlueImportError("Emit", "the effect runs past the module memory");
    int effect[kEffectCapacity / sizeof(int)];
    memcpy(effect, GlueLinear(a[0].i32), size);
    long expected;
    if (glue == nullptr || !wl_module_publishes(glue->loader, glue->def->name, effect[0], &expected))
        return GlueImportError("Emit", "the kind is not in the module's publishes table");
    if (!Routed(glue->def->outbox, effect[0]))
        return GlueImportError("Emit", "the kind is not routed from the module's outbox");
    if ((long)size != expected)
        return GlueImportError("Emit", "the size is not the payload's size of its kind");
    unsigned hookCount;
    const GlueEffectHook* hooks = GlueEffectHooks(&hookCount);
    for (unsigned i = 0; i < hookCount; i++) {
        if (hooks[i].kind != effect[0])
            continue;
        const char* error = hooks[i].check(effect);
        if (error != nullptr)
            return error;
    }
    if (glue->def->schedPublish)
        SchedPublish(glue->def->outbox, effect, size);
    else
        EmitFrom(glue->def->outbox, effect, size);
    return nullptr;
}

const GlueImport kServiceImports[] = {
    {"services", "GameRand", "i>i", GameRandImport},
    {"services", "RandomInt", "i>i", RandomIntImport},
    {"services", "FatalError", "i>", FatalErrorImport},
    {"services", "BuildDataPath", "iiii>i", BuildDataPathImport},
    {"services", "TdfFile_Construct", "i>", TdfFileConstructImport},
    {"services", "TdfFile_Destruct", "i>", TdfFileDestructImport},
    {"services", "TdfFile_LoadFile", "ii>i", TdfFileLoadFileImport},
    {"services", "TdfFile_SelectRecord", "ii>i", TdfFileSelectRecordImport},
    {"services", "TdfRecord_GetFieldInt", "iii>i", TdfRecordGetFieldIntImport},
    {"services", "TdfRecord_GetFieldDouble", "iid>d", TdfRecordGetFieldDoubleImport},
    {"services", "TdfRecord_GetFieldString", "iiiii>i", TdfRecordGetFieldStringImport},
    {"services", "HapiBank_OpenAccount", "ii>i", HapiBankOpenAccountImport},
    {"services", "HapiBank_SetIntegerItem", "iii>i", HapiBankSetIntegerItemImport},
    {"services", "HapiBank_GetIntegerItem", "iii>i", HapiBankGetIntegerItemImport},
    {"services", "SimAlloc", "i>i", SimAllocImport},
    {"services", "SimAllocTagged", "ii>i", SimAllocTaggedImport},
    {"services", "SimFree", "i>", SimFreeImport},
    {"services", "RecordSimEvent", "iiii>", RecordSimEventImport},
    {"services", "SimCaptureAlloc", "ii>i", SimCaptureAllocImport},
    {"services", "GetSlotDpid", "i>i", GetSlotDpidImport},
    {"services", "FindSlotByDpid", "i>i", FindSlotByDpidImport},
    {"services", "Emit", "ii>", EmitImport},
};

const unsigned kServiceImportCount = sizeof kServiceImports / sizeof kServiceImports[0];
const unsigned kImportLimit = 1024;
unsigned long long s_importTime[kImportLimit];
unsigned s_importCalls[kImportLimit];
unsigned long long s_emptyTime;
unsigned s_emptyCalls;
unsigned long long s_warmTime;
unsigned s_warmCalls;

unsigned ImportCount()
{
    unsigned count;
    GlueInterfaceImports(&count);
    return kServiceImportCount + count;
}

const GlueImport* ImportAt(unsigned index)
{
    unsigned count;
    const GlueImport* imports = GlueInterfaceImports(&count);
    return index < kServiceImportCount ? &kServiceImports[index] : &imports[index - kServiceImportCount];
}

const char* RunImport(const GlueImport* def, wasmtime_val_raw_t* a)
{
    unsigned code = 0;
    const char* error = nullptr;
    if (s_importDepth++ == 0)
        s_importThread = host_thread_id();
    __try {
        error = def->function(a);
    } __except (code = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {
        char text[200];
        sprintf(text, "import %s: exception 0x%08x", def->name, code);
        error = RecordFatal(text);
    }
    s_importDepth--;
    if (s_fatalPending)
        return s_fatalText;
    return error;
}

const char* __cdecl GuardedImport(void* user, wasmtime_val_raw_t* a)
{
    unsigned index = (unsigned)(size_t)user;
    const GlueImport* def = ImportAt(index);
    if (!s_timing)
        return RunImport(def, a);
    unsigned long long start = host_time_counter();
    const char* error = RunImport(def, a);
    s_importTime[index] += host_time_counter() - start;
    s_importCalls[index]++;
    return error;
}

long long FuelLeft(GlueModule* glue)
{
    wasmtime_val_raw_t a[1];
    a[0].i64 = 0;
    if (!wl_call(glue->loader, glue->module, &glue->fuelGet, a, 1))
        return 0;
    return a[0].i64;
}

void FuelSet(GlueModule* glue, long long value)
{
    wasmtime_val_raw_t a[1];
    a[0].i64 = value;
    unsigned long long start = s_timing ? host_time_counter() : 0;
    wl_call(glue->loader, glue->module, &glue->fuelSet, a, 1);
    if (s_timing) {
        s_emptyTime += host_time_counter() - start;
        s_emptyCalls++;
    }
}

void FuelBegin(GlueModule* glue)
{
    int tick = g_game != nullptr ? g_game->ticks : -1;
    if (glue->fuelOpen && glue->fuelTick == tick)
        return;
    FuelSet(glue, glue->fuelBudget);
    glue->fuelOpen = 1;
    glue->fuelTick = tick;
}

const char* const kWordNames[GLUE_WORD_COUNT] = {"g_game", "g_simGlobals", "g_extTdf", "g_blockBase", "g_region"};

unsigned LinearOffset(const void* native)
{
    return (unsigned)((const unsigned char*)native - ModulesMemory()->base);
}

void WriteWord(GlueModule* glue, GlueWord word, const void* value)
{
    if (glue->words[word] != 0)
        memcpy(ModulesMemory()->base + glue->words[word], value, 4);
}

void PrepareCall(GlueModule* glue, unsigned slot)
{
    if (glue->words[GLUE_WORD_GAME] != 0) {
        if (!SimOwns(g_game))
            GlueStop(glue, slot, "g_game is not in the sim block");
        unsigned game = LinearOffset(g_game);
        WriteWord(glue, GLUE_WORD_GAME, &game);
    }
    if (glue->words[GLUE_WORD_SIM] != 0) {
        void* sim = &Sim();
        if (!SimOwns(sim))
            GlueStop(glue, slot, "g_simGlobals is not in the sim block");
        unsigned sims = LinearOffset(sim);
        WriteWord(glue, GLUE_WORD_SIM, &sims);
    }
    WriteWord(glue, GLUE_WORD_EXT_TDF, g_extTdf);
    unsigned blockBase = LinearOffset(SimBlockBase());
    WriteWord(glue, GLUE_WORD_BLOCK_BASE, &blockBase);
    if (glue->words[GLUE_WORD_REGION] != 0) {
        if (glue->region != nullptr && !SimOwns(glue->region))
            GlueStop(glue, slot, "the region is not in the sim block");
        unsigned region = glue->region == nullptr ? 0 : LinearOffset(glue->region);
        WriteWord(glue, GLUE_WORD_REGION, &region);
    }
}

}

GlueModule* GlueActive()
{
    return s_active;
}

int GlueInMemory(unsigned address)
{
    return address < ModulesMemory()->size;
}

void* GlueLinear(unsigned address)
{
    return address == 0 ? nullptr : ModulesMemory()->base + address;
}

const char* GlueImportError(const char* import, const char* what)
{
    sprintf(s_error, "import %s: %s", import, what);
    return s_error;
}

unsigned GlueHandleOf(void* native)
{
    if (native == nullptr)
        return 0;
    unsigned free = kHandleCapacity;
    for (unsigned i = 0; i < s_handleCount; i++) {
        if (s_handles[i] == native)
            return i + 1;
        if (s_handles[i] == nullptr && free == kHandleCapacity)
            free = i;
    }
    if (free == kHandleCapacity) {
        if (s_handleCount == kHandleCapacity) {
            char text[200];
            sprintf(text, "%s: the handle table is full", ActiveName());
            if (s_importDepth == 0)
                ModulesFatal(text);
            RecordFatal(text);
            return 0;
        }
        free = s_handleCount++;
    }
    s_handles[free] = native;
    return free + 1;
}

const char* GlueNativeOf(const char* import, unsigned handle, void** native)
{
    if (handle > s_handleCount || (handle != 0 && s_handles[handle - 1] == nullptr)) {
        sprintf(s_error, "import %s: unknown handle %u", import, handle);
        return s_error;
    }
    *native = handle == 0 ? nullptr : s_handles[handle - 1];
    return nullptr;
}

const char* GlueLiveOf(const char* import, unsigned handle, void** native)
{
    const char* error = GlueNativeOf(import, handle, native);
    if (error == nullptr && *native == nullptr) {
        sprintf(s_error, "import %s: null handle", import);
        return s_error;
    }
    return error;
}

const char* GlueAddressOf(const char* import, const void* native, int inBlock, unsigned* address, const char* noun)
{
    *address = 0;
    if (native == nullptr)
        return nullptr;
    if (inBlock ? !SimOwns(native) : (const unsigned char*)native < ModulesMemory()->base || LinearOffset(native) >= ModulesMemory()->size) {
        sprintf(s_error, "import %s: returned a%s %s outside the %s", import, noun[0] == 'a' || noun[0] == 'o' ? "n" : "", noun, inBlock ? "sim block" : "module memory");
        return s_error;
    }
    *address = LinearOffset(native);
    return nullptr;
}

void GlueStop(GlueModule* glue, unsigned slot, const char* what)
{
    char text[200];
    sprintf(text, "slot %s: %s", glue->def->slots[slot], what);
    ModulesFatal(text);
}

unsigned GlueScratchMark()
{
    return s_scratchUsed;
}

void GlueScratchRelease(unsigned mark)
{
    s_scratchUsed = mark;
}

unsigned GlueIn(GlueModule* glue, unsigned slot, const void* native, unsigned size, int copyIn)
{
    if (native == nullptr)
        return 0;
    if (SimOwns(native))
        return LinearOffset(native);
    const ModuleMemory* memory = ModulesMemory();
    unsigned at = (s_scratchUsed + 7) & ~7u;
    if (size > memory->scratchSize || at > memory->scratchSize - size)
        GlueStop(glue, slot, "the argument does not fit the scratch area");
    s_scratchUsed = at + size;
    if (copyIn)
        memcpy(memory->base + memory->scratch + at, native, size);
    return memory->scratch + at;
}

void GlueOut(void* native, unsigned address, unsigned size)
{
    if (native != nullptr && !SimOwns(native))
        memcpy(native, ModulesMemory()->base + address, size);
}

unsigned GlueObject(GlueModule* glue, unsigned slot, const void* native)
{
    if (native == nullptr)
        return 0;
    if (!SimOwns(native))
        GlueStop(glue, slot, "an object argument is not in the sim block");
    return LinearOffset(native);
}

unsigned GlueText(GlueModule* glue, unsigned slot, const char* native)
{
    if (native == nullptr)
        return 0;
    return GlueIn(glue, slot, native, (unsigned)strlen(native) + 1, 1);
}

void* GlueResult(GlueModule* glue, unsigned slot, unsigned address, int inBlock)
{
    void* native = GlueLinear(address);
    if (native != nullptr && (inBlock ? !SimOwns(native) : !GlueInMemory(address)))
        GlueStop(glue, slot, inBlock ? "returned a pointer outside the sim block" : "returned a pointer outside the module memory");
    return native;
}

void GlueTimed(GlueModule* glue, unsigned slot, unsigned long long start)
{
    glue->def->timing[slot].slotTime += host_time_counter() - start;
    glue->def->timing[slot].calls++;
}

void GlueCall(GlueModule* glue, unsigned slot, wasmtime_val_raw_t* a, size_t count)
{
    if (s_importDepth > 0) {
        char text[200];
        sprintf(text, "slot %s: called inside an import", glue->def->slots[slot]);
        RecordFatal(text);
        return;
    }
    unsigned long long start = s_timing ? host_time_counter() : 0;
    unsigned thread = host_thread_id();
    if (s_slotDepth != 0 && thread != s_slotOwner)
        GlueStop(glue, slot, "called from a second thread while another thread's call is active");
    PrepareCall(glue, slot);
    if (s_slotDepth == 0)
        s_slotOwner = thread;
    FuelBegin(glue);
    GlueModule* outer = s_active;
    s_active = glue;
    s_slotDepth++;
    unsigned long long callStart = s_timing ? host_time_counter() : 0;
    wl_call(glue->loader, glue->module, &glue->def->funcs[slot], a, count);
    unsigned long long callEnd = s_timing ? host_time_counter() : 0;
    --s_slotDepth;
    s_active = outer;
    if (s_fatalPending && s_slotDepth == 0) {
        char text[200];
        sprintf(text, "%s: wl_call returned with a fatal pending\n", glue->def->name);
        host_log(text);
        ModulesFatal(s_fatalText);
    }
    if (s_slotDepth == 0) {
        s_handleCount = 0;
        if (s_timing) {
            GlueTiming& timing = glue->def->timing[slot];
            timing.slotTime += host_time_counter() - start;
            timing.callTime += callEnd - callStart;
            timing.calls++;
            wasmtime_val_raw_t b[1];
            b[0].i64 = 0;
            unsigned long long warm = host_time_counter();
            wl_call(glue->loader, glue->module, &glue->fuelGet, b, 1);
            s_warmTime += host_time_counter() - warm;
            s_warmCalls++;
        }
    }
}

GlueModule* __stdcall GlueFind(const char* name)
{
    unsigned count;
    GlueModule* const* modules = GlueModules(&count);
    for (unsigned i = 0; i < count; i++)
        if (strcmp(modules[i]->def->name, name) == 0)
            return modules[i];
    return nullptr;
}

int __stdcall GlueInstall(wl_loader* loader, int timing)
{
    s_loader = loader;
    s_timing = timing;
    s_active = nullptr;
    s_scratchUsed = 0;
    if (ImportCount() > kImportLimit)
        return 0;
    for (unsigned i = 0; i < ImportCount(); i++) {
        const GlueImport* def = ImportAt(i);
        if (!wl_define_import(loader, def->module, def->name, def->signature, GuardedImport, (void*)(size_t)i))
            return 0;
    }
    return 1;
}

int __stdcall GlueBind(GlueModule* glue, wl_loader* loader, wl_module* module, unsigned fuelBudget)
{
    glue->loader = loader;
    glue->module = module;
    glue->fuelBudget = fuelBudget;
    glue->fuelOpen = 0;
    glue->region = nullptr;
    glue->world = nullptr;
    const char* interfaceName = wl_module_text(loader, wl_module_name(module), "interface");
    if (interfaceName == nullptr || strcmp(interfaceName, glue->def->name) != 0)
        return 0;
    if (!wl_export_func(loader, module, "__fuel_set", &glue->fuelSet) || !wl_export_func(loader, module, "__fuel_get", &glue->fuelGet))
        return 0;
    for (unsigned i = 0; i < glue->def->slotCount; i++) {
        if (!wl_export_func(loader, module, glue->def->slots[i], &glue->def->funcs[i]))
            return 0;
    }
    memset(glue->words, 0, sizeof glue->words);
    for (size_t i = 0; i < wl_data_count(module); i++) {
        const char* name = wl_data_name(module, i);
        int known = 0;
        for (int w = 0; w < GLUE_WORD_COUNT; w++) {
            if (strcmp(name, kWordNames[w]) == 0) {
                glue->words[w] = wl_data_address(module, i);
                known = glue->words[w] != 0;
            }
        }
        if (!known)
            return 0;
    }
    glue->bound = 1;
    return 1;
}

void __stdcall GlueUseSeam(GlueModule* glue)
{
    glue->def->useSeam(glue);
}

int __stdcall GlueDeferFatal(const char* text)
{
    if (s_importDepth == 0 || s_importThread != host_thread_id())
        return 0;
    RecordFatal(text);
    return 1;
}

int __stdcall GlueFatalPending()
{
    return s_fatalPending;
}

void __stdcall GlueDataRanges(GlueModule* glue, unsigned* addresses, unsigned* count)
{
    *count = 0;
    for (int w = 0; w < GLUE_WORD_COUNT; w++)
        if (glue->words[w] != 0)
            addresses[(*count)++] = glue->words[w];
}

int __stdcall GlueFuelClose(GlueModule* glue, unsigned* used)
{
    if (!glue->fuelOpen)
        return 0;
    glue->fuelOpen = 0;
    *used = (unsigned)(glue->fuelBudget - FuelLeft(glue));
    return 1;
}

int __stdcall GlueOutOfFuel(unsigned* budget, const char** module)
{
    unsigned count;
    GlueModule* const* modules = GlueModules(&count);
    for (unsigned i = 0; i < count; i++) {
        GlueModule* glue = modules[i];
        if (glue->bound && glue->fuelOpen && FuelLeft(glue) < 0) {
            *budget = (unsigned)glue->fuelBudget;
            *module = glue->def->name;
            return 1;
        }
    }
    return 0;
}

void __stdcall GlueTimeNative(GlueModule* glue)
{
    glue->def->timeNative();
}

void __stdcall GlueTimingReport(char* text)
{
    text[0] = '\0';
    unsigned count;
    GlueModule* const* modules = GlueModules(&count);
    for (unsigned m = 0; m < count; m++) {
        const GlueModuleDef* def = modules[m]->def;
        for (unsigned i = 0; i < def->slotCount; i++)
            if (def->timing[i].calls != 0)
                sprintf(text + strlen(text), "slot\t%s.%s\t%u\t%I64u\t%I64u\n", def->name, def->slots[i], def->timing[i].calls, def->timing[i].slotTime, def->timing[i].callTime);
    }
    for (unsigned i = 0; i < ImportCount(); i++)
        if (s_importCalls[i] != 0)
            sprintf(text + strlen(text), "import\t%s.%s\t%u\t%I64u\n", ImportAt(i)->module, ImportAt(i)->name, s_importCalls[i], s_importTime[i]);
    sprintf(text + strlen(text), "empty\t%u\t%I64u\n", s_emptyCalls, s_emptyTime);
    sprintf(text + strlen(text), "warm\t%u\t%I64u\n", s_warmCalls, s_warmTime);
}
