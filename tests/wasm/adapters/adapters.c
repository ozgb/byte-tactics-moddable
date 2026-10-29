#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wasmloader.h"

static int failures;
static int noted;
static int ticks;

static void* open_library(const char* path)
{
    return dlopen(path, RTLD_NOW);
}

static void* find_symbol(void* library, const char* name)
{
    return dlsym(library, name);
}

static void close_library(void* library)
{
    dlclose(library);
}

static const char* host_mix(void* user, wasmtime_val_raw_t* a)
{
    (void)user;
    a[0].i32 = a[0].i32 * 10 + a[1].i32;
    return NULL;
}

static const char* host_neg(void* user, wasmtime_val_raw_t* a)
{
    (void)user;
    a[0].i32 = -a[0].i32;
    return NULL;
}

static const char* host_mul(void* user, wasmtime_val_raw_t* a)
{
    (void)user;
    a[0].i32 = a[0].i32 * a[1].i32;
    return NULL;
}

static const char* host_note(void* user, wasmtime_val_raw_t* a)
{
    (void)user;
    noted = a[0].i32;
    return NULL;
}

static const char* host_tick(void* user, wasmtime_val_raw_t* a)
{
    (void)user;
    ticks++;
    a[0].i32 += 1000;
    return NULL;
}

static const char* host_echo(void* user, wasmtime_val_raw_t* a)
{
    (void)user;
    a[0].i32 += 1000;
    return NULL;
}

static wl_loader* start(const char* dll, int define_imports)
{
    wl_config config;
    char error[512];
    wl_loader* loader;
    memset(&config, 0, sizeof config);
    config.dll_path = dll;
    config.open = open_library;
    config.symbol = find_symbol;
    config.close = close_library;
    config.target = sizeof(void*) == 8 ? "pulley64" : "pulley32";
    loader = wl_create(&config, error, sizeof error);
    if (!loader) {
        fprintf(stderr, "%s\n", error);
        exit(2);
    }
    if (define_imports
        && !(wl_define_import(loader, "calc_host", "Mix", "ii>i", host_mix, NULL) && wl_define_import(loader, "calc_host", "Neg", "i>i", host_neg, NULL)
             && wl_define_import(loader, "calc_host", "Mul", "ii>i", host_mul, NULL) && wl_define_import(loader, "calc_host", "Note", "i>", host_note, NULL)
             && wl_define_import(loader, "calc_host", "Tick", "i>i", host_tick, NULL) && wl_define_import(loader, "calc_host", "Echo", "i>i", host_echo, NULL))) {
        fprintf(stderr, "%s\n", wl_error(loader));
        exit(2);
    }
    return loader;
}

static void expect(const char* label, long got, long want)
{
    if (got != want) {
        fprintf(stderr, "FAIL %s: got %ld, wanted %ld\n", label, got, want);
        failures++;
    } else {
        printf("ok %s = %ld\n", label, got);
    }
}

static int slot(wl_loader* loader, wl_module* module, const char* name, int a0, int a1, int a2, int* result)
{
    wasmtime_func_t func;
    wasmtime_val_raw_t raw[4];
    memset(raw, 0, sizeof raw);
    raw[0].i32 = a0;
    raw[1].i32 = a1;
    raw[2].i32 = a2;
    if (!wl_export_func(loader, module, name, &func) || !wl_call(loader, module, &func, raw, 4)) {
        fprintf(stderr, "FAIL %s: %s\n", name, wl_error(loader));
        failures++;
        return 0;
    }
    *result = raw[0].i32;
    return 1;
}

static void check(wl_loader* loader, wl_module* module, const char* tag, const char* name, int a0, int a1, int a2, long want)
{
    int result = 0;
    char label[96];
    sprintf(label, "%s %s(%d, %d, %d)", tag, name, a0, a1, a2);
    if (slot(loader, module, name, a0, a1, a2, &result)) {
        expect(label, result, want);
    }
}

static int memory_word(wl_loader* loader, unsigned address)
{
    int value;
    memcpy(&value, wl_memory_base(loader) + address, 4);
    return value;
}

static void run_modules(const char* dll, const char* manifest, const char* wasm_dir)
{
    wl_loader* loader = start(dll, 1);
    wl_module* old;
    wl_module* current;
    char path[512];
    wasmtime_func_t func;
    wasmtime_val_raw_t raw[1];
    int ignored;
    if (!wl_load_manifest_file(loader, manifest) || !wl_create_memory(loader)) {
        fprintf(stderr, "%s\n", wl_error(loader));
        exit(1);
    }
    sprintf(path, "%s/old.wasm", wasm_dir);
    old = wl_load_module(loader, "old", path);
    if (!old) {
        fprintf(stderr, "FAIL old: %s\n", wl_error(loader));
        exit(1);
    }
    check(loader, old, "old", "Swap", 1, 2, 0, 2001);
    check(loader, old, "old", "Add3", 1, 10, 100, 101);
    check(loader, old, "old", "Scale", 5, 0, 0, 15);
    check(loader, old, "old", "Count", 4, 0, 0, 7);
    slot(loader, old, "Poke", 9, 0, 0, &ignored);
    expect("old Poke stored", memory_word(loader, 65552), 9);
    check(loader, old, "old", "Same", 5, 0, 0, 6);
    check(loader, old, "old", "RunMix", 1, 2, 0, 21);
    check(loader, old, "old", "RunNeg", 5, 0, 0, -5);
    check(loader, old, "old", "RunMul", 5, 0, 0, 20);
    check(loader, old, "old", "RunNote", 8, 0, 0, 5);
    expect("old Note reached the host", noted, 8);
    check(loader, old, "old", "RunTick", 3, 0, 0, 3);
    expect("old Tick reached the host", ticks, 1);
    check(loader, old, "old", "RunEcho", 1, 0, 0, 1001);
    expect("old table entries filled (unchanged slots only)", wl_table_filled(old), 7);
    if (wl_export_func(loader, old, "Swap", &func)) {
        raw[0].i32 = 1;
        if (wl_call(loader, old, &func, raw, 1)) {
            fprintf(stderr, "FAIL a call with too little room was accepted\n");
            failures++;
        } else if (!strstr(wl_error(loader), "room for 1 values, its current signature needs 2")) {
            fprintf(stderr, "FAIL room error: %s\n", wl_error(loader));
            failures++;
        } else {
            printf("ok too little room: %s\n", wl_error(loader));
        }
    }

    sprintf(path, "%s/current.wasm", wasm_dir);
    current = wl_load_module(loader, "current", path);
    if (!current) {
        fprintf(stderr, "FAIL current: %s\n", wl_error(loader));
        exit(1);
    }
    check(loader, current, "current", "Swap", 1, 2, 0, 1002);
    check(loader, current, "current", "Add3", 1, 10, 100, 111);
    check(loader, current, "current", "Scale", 5, 0, 0, 15);
    check(loader, current, "current", "Count", 4, 0, 0, 8);
    slot(loader, current, "Poke", 9, 0, 0, &ignored);
    expect("current Poke stored", memory_word(loader, 131088), 9);
    check(loader, current, "current", "RunMix", 1, 2, 0, 12);
    check(loader, current, "current", "RunNeg", 5, 0, 0, -5);
    check(loader, current, "current", "RunMul", 5, 0, 0, 20);
    check(loader, current, "current", "RunNote", 6, 0, 0, 6);
    expect("current Note reached the host", noted, 6);
    check(loader, current, "current", "RunTick", 3, 0, 0, 3);
    expect("current Tick reached the host", ticks, 2);
    check(loader, current, "current", "RunEcho", 1, 0, 0, 1001);
    expect("current table entries filled", wl_table_filled(current), 12);
    wl_destroy(loader);
}

#define HEAD "{\"memory_pages\":4,\"table_size\":64,\"window\":65536,\"table_slots\":16,\"majors\":{\"calc\":2,\"calc_host\":3},"
#define MODULE(majors, slots, imports)                                                                                           \
    "\"modules\":[{\"name\":\"m\",\"interface\":\"calc\",\"majors\":" majors ",\"data_base\":65536,\"table_base\":16,\"exports\":[],\"slots\":" slots \
    ",\"imports\":" imports "}]}"
#define OLD_CALC "{\"calc\":1,\"calc_host\":3}"
#define OLD_HOST "{\"calc\":2,\"calc_host\":2}"

static void refuse(const char* dll, const char* wasm_dir, const char* label, const char* manifest, int define_imports, const char* needle)
{
    wl_loader* loader = start(dll, define_imports);
    char path[512];
    sprintf(path, "%s/old.wasm", wasm_dir);
    if (!wl_load_manifest_text(loader, manifest, strlen(manifest), label) || !wl_create_memory(loader)) {
        fprintf(stderr, "FAIL %s: the manifest was refused: %s\n", label, wl_error(loader));
        failures++;
    } else if (wl_load_module(loader, "m", path)) {
        fprintf(stderr, "FAIL %s: the module loaded\n", label);
        failures++;
    } else if (!strstr(wl_error(loader), needle)) {
        fprintf(stderr, "FAIL %s: the error is \"%s\", wanted \"%s\"\n", label, wl_error(loader), needle);
        failures++;
    } else {
        printf("ok refused %s: %s\n", label, wl_error(loader));
    }
    wl_destroy(loader);
}

static void run_refusals(const char* dll, const char* wasm_dir)
{
    refuse(dll, wasm_dir, "slot without an adapter",
           HEAD "\"signatures\":{\"calc\":{\"Scale\":[[\"int\"],\"int\"]}},\"adapters\":[]," MODULE(OLD_CALC, "{\"Scale\":[[\"int\",\"int\"],\"int\"]}", "{}"), 1,
           "module m: slot Scale of interface calc changed between major 1 and 2, and the manifest has no adapter for it");
    refuse(dll, wasm_dir, "import without an adapter",
           HEAD "\"signatures\":{\"calc_host\":{\"Neg\":[[\"int\"],\"int\"]}},\"adapters\":[]," MODULE(OLD_HOST, "{}", "{\"calc_host\":{\"Neg\":[[\"int\",\"int\"],\"int\"]}}"), 1,
           "module m: import Neg of interface calc_host changed between major 2 and 3, and the manifest has no adapter for it");
    refuse(dll, wasm_dir, "slot adapter joining a linear to a handle",
           HEAD "\"signatures\":{\"calc\":{\"Take\":[[\"handle\"],\"int\"]}},\"adapters\":["
           "{\"interface\":\"calc\",\"from_major\":1,\"name\":\"Take\",\"args\":[0],\"result\":\"pass\"}],"
           MODULE(OLD_CALC, "{\"Take\":[[\"linear\"],\"int\"]}", "{}"),
           1, "module m: the adapter for slot Take of interface calc (major 1 to 2) is refused: args[0]: the caller's handle parameter 0 cannot become the callee's linear parameter without code");
    refuse(dll, wasm_dir, "import adapter joining an int to an f64",
           HEAD "\"signatures\":{\"calc_host\":{\"Neg\":[[\"f64\"],\"int\"]}},\"adapters\":["
           "{\"interface\":\"calc_host\",\"from_major\":2,\"name\":\"Neg\",\"args\":[0],\"result\":\"pass\"}],"
           MODULE(OLD_HOST, "{}", "{\"calc_host\":{\"Neg\":[[\"int\"],\"int\"]}}"),
           1, "the adapter for import Neg of interface calc_host (major 2 to 3) is refused: args[0]: the caller's int parameter 0 cannot become the callee's f64 parameter without code");
    refuse(dll, wasm_dir, "constant into a linear parameter",
           HEAD "\"signatures\":{\"calc\":{\"Scale\":[[\"int\"],\"int\"]}},\"adapters\":["
           "{\"interface\":\"calc\",\"from_major\":1,\"name\":\"Scale\",\"args\":[0,{\"const\":3}],\"result\":\"pass\"}],"
           MODULE(OLD_CALC, "{\"Scale\":[[\"int\",\"linear\"],\"int\"]}", "{}"),
           1, "args[1]: a constant can only fill an int parameter, the callee's is linear");
    refuse(dll, wasm_dir, "argument index out of range",
           HEAD "\"signatures\":{\"calc\":{\"Scale\":[[\"int\"],\"int\"]}},\"adapters\":["
           "{\"interface\":\"calc\",\"from_major\":1,\"name\":\"Scale\",\"args\":[0,5],\"result\":\"pass\"}],"
           MODULE(OLD_CALC, "{\"Scale\":[[\"int\",\"int\"],\"int\"]}", "{}"),
           1, "args[1] reads parameter 5, the caller passes 1");
    refuse(dll, wasm_dir, "wrong number of arguments",
           HEAD "\"signatures\":{\"calc\":{\"Scale\":[[\"int\"],\"int\"]}},\"adapters\":["
           "{\"interface\":\"calc\",\"from_major\":1,\"name\":\"Scale\",\"args\":[0],\"result\":\"pass\"}],"
           MODULE(OLD_CALC, "{\"Scale\":[[\"int\",\"int\"],\"int\"]}", "{}"),
           1, "args has 1 entries, the callee takes 2 parameters");
    refuse(dll, wasm_dir, "pass without a result",
           HEAD "\"signatures\":{\"calc\":{\"Count\":[[\"int\"],\"void\"]}},\"adapters\":["
           "{\"interface\":\"calc\",\"from_major\":1,\"name\":\"Count\",\"args\":[0],\"result\":\"pass\"}],"
           MODULE(OLD_CALC, "{\"Count\":[[\"int\"],\"int\"]}", "{}"),
           1, "result: pass needs a result on both sides, the caller's is void and the callee's int");
    refuse(dll, wasm_dir, "void for a caller that expects a result",
           HEAD "\"signatures\":{\"calc\":{\"Count\":[[\"int\"],\"int\"]}},\"adapters\":["
           "{\"interface\":\"calc\",\"from_major\":1,\"name\":\"Count\",\"args\":[0],\"result\":\"void\"}],"
           MODULE(OLD_CALC, "{\"Count\":[[\"int\"],\"void\"]}", "{}"),
           1, "result: void, but the caller expects int");
    refuse(dll, wasm_dir, "result joining an int to an f64",
           HEAD "\"signatures\":{\"calc\":{\"Score\":[[\"int\"],\"f64\"]}},\"adapters\":["
           "{\"interface\":\"calc\",\"from_major\":1,\"name\":\"Score\",\"args\":[0],\"result\":\"pass\"}],"
           MODULE(OLD_CALC, "{\"Score\":[[\"int\"],\"int\"]}", "{}"),
           1, "result: the callee's int cannot become the caller's f64 without code");
    refuse(dll, wasm_dir, "module ahead of the manifest",
           HEAD "\"signatures\":{},\"adapters\":[]," MODULE("{\"calc\":3,\"calc_host\":3}", "{}", "{}"), 1,
           "module m: built against interface calc major 3, the manifest has 2");
    refuse(dll, wasm_dir, "adapted import the host does not define",
           HEAD "\"signatures\":{\"calc_host\":{\"Zap\":[[\"int\"],\"void\"]}},\"adapters\":["
           "{\"interface\":\"calc_host\",\"from_major\":2,\"name\":\"Zap\",\"args\":[0],\"result\":{\"const\":1}}],"
           MODULE(OLD_HOST, "{}", "{\"calc_host\":{\"Zap\":[[\"int\"],\"int\"]}}"),
           1, "import calc_host.Zap is adapted from major 2 to 3, but the host does not define it");
    refuse(dll, wasm_dir, "adapted import the host defines with another signature",
           HEAD "\"signatures\":{\"calc_host\":{\"Neg\":[[\"int\",\"int\"],\"int\"]}},\"adapters\":["
           "{\"interface\":\"calc_host\",\"from_major\":2,\"name\":\"Neg\",\"args\":[0,{\"const\":1}],\"result\":\"pass\"}],"
           MODULE(OLD_HOST, "{}", "{\"calc_host\":{\"Neg\":[[\"int\"],\"int\"]}}"),
           1, "import calc_host.Neg is adapted from major 2 to 3, but the host defines it with another signature than the manifest's");
}

int main(int argc, char** argv)
{
    if (argc != 4) {
        fprintf(stderr, "usage: adapters libwasmtime.so manifest.json wasm-dir\n");
        return 2;
    }
    run_modules(argv[1], argv[2], argv[3]);
    run_refusals(argv[1], argv[3]);
    if (failures) {
        fprintf(stderr, "%d checks failed\n", failures);
        return 1;
    }
    printf("ok: adapters and refusals\n");
    return 0;
}
