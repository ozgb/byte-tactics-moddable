#define WASM_API_EXTERN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wasmtime.h>

#define API(name) static __typeof__(name) *pf_##name

API(wasm_config_new);
API(wasmtime_config_target_set);
API(wasmtime_config_cranelift_nan_canonicalization_set);
API(wasmtime_config_consume_fuel_set);
API(wasm_engine_new_with_config);
API(wasmtime_store_new);
API(wasmtime_store_context);
API(wasmtime_memorytype_new);
API(wasmtime_memory_new);
API(wasmtime_memory_data);
API(wasmtime_memory_data_size);
API(wasm_valtype_new);
API(wasm_tabletype_new);
API(wasmtime_table_new);
API(wasmtime_table_get);
API(wasmtime_module_new);
API(wasmtime_linker_new);
API(wasmtime_linker_define);
API(wasmtime_linker_instantiate);
API(wasmtime_instance_export_get);
API(wasmtime_func_call_unchecked);
API(wasmtime_error_message);
API(wasmtime_error_delete);
API(wasm_trap_message);
API(wasm_trap_delete);
API(wasm_byte_vec_delete);

static HMODULE dll;

static void *symbol(const char *name)
{
    void *p = (void *)GetProcAddress(dll, name);
    if (!p) {
        fprintf(stderr, "missing export %s\n", name);
        exit(2);
    }
    return p;
}

#define LOAD(name) pf_##name = (__typeof__(pf_##name))symbol(#name)

static void load_api(const char *path)
{
    dll = LoadLibraryA(path);
    if (!dll) {
        fprintf(stderr, "cannot load %s (error %lu)\n", path, GetLastError());
        exit(2);
    }
    LOAD(wasm_config_new);
    LOAD(wasmtime_config_target_set);
    LOAD(wasmtime_config_cranelift_nan_canonicalization_set);
    LOAD(wasmtime_config_consume_fuel_set);
    LOAD(wasm_engine_new_with_config);
    LOAD(wasmtime_store_new);
    LOAD(wasmtime_store_context);
    LOAD(wasmtime_memorytype_new);
    LOAD(wasmtime_memory_new);
    LOAD(wasmtime_memory_data);
    LOAD(wasmtime_memory_data_size);
    LOAD(wasm_valtype_new);
    LOAD(wasm_tabletype_new);
    LOAD(wasmtime_table_new);
    LOAD(wasmtime_table_get);
    LOAD(wasmtime_module_new);
    LOAD(wasmtime_linker_new);
    LOAD(wasmtime_linker_define);
    LOAD(wasmtime_linker_instantiate);
    LOAD(wasmtime_instance_export_get);
    LOAD(wasmtime_func_call_unchecked);
    LOAD(wasmtime_error_message);
    LOAD(wasmtime_error_delete);
    LOAD(wasm_trap_message);
    LOAD(wasm_trap_delete);
    LOAD(wasm_byte_vec_delete);
}

static void die(const char *what, wasmtime_error_t *error, wasm_trap_t *trap)
{
    wasm_byte_vec_t message;
    if (error) {
        pf_wasmtime_error_message(error, &message);
        pf_wasmtime_error_delete(error);
    } else if (trap) {
        pf_wasm_trap_message(trap, &message);
        pf_wasm_trap_delete(trap);
    } else {
        message.size = 0;
        message.data = NULL;
    }
    fprintf(stderr, "%s failed: %.*s\n", what, (int)message.size, message.data ? message.data : "");
    exit(1);
}

static unsigned char *read_file(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    unsigned char *data;
    long length;
    if (!f) {
        fprintf(stderr, "cannot open %s\n", path);
        exit(2);
    }
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    data = (unsigned char *)malloc((size_t)length);
    if (fread(data, 1, (size_t)length, f) != (size_t)length) {
        fprintf(stderr, "short read of %s\n", path);
        exit(2);
    }
    fclose(f);
    *size = (size_t)length;
    return data;
}

static double seconds(void)
{
    LARGE_INTEGER counter, frequency;
    QueryPerformanceCounter(&counter);
    QueryPerformanceFrequency(&frequency);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
}

typedef struct {
    wasm_engine_t *engine;
    wasmtime_store_t *store;
    wasmtime_context_t *context;
    wasmtime_memory_t memory;
    wasmtime_table_t table;
    wasmtime_linker_t *linker;
} shared_t;

static void make_shared(shared_t *shared, unsigned pages, unsigned table_size)
{
    wasm_config_t *config = pf_wasm_config_new();
    wasmtime_error_t *error = pf_wasmtime_config_target_set(config, "pulley32");
    wasm_memorytype_t *memory_type;
    wasm_tabletype_t *table_type;
    wasm_limits_t table_limits;
    wasmtime_val_t null_func;
    wasmtime_extern_t item;
    if (error) {
        die("select pulley32", error, NULL);
    }
    pf_wasmtime_config_cranelift_nan_canonicalization_set(config, 1);
    pf_wasmtime_config_consume_fuel_set(config, 0);
    shared->engine = pf_wasm_engine_new_with_config(config);
    shared->store = pf_wasmtime_store_new(shared->engine, NULL, NULL);
    shared->context = pf_wasmtime_store_context(shared->store);

    error = pf_wasmtime_memorytype_new(pages, 1, pages, 0, 0, 16, &memory_type);
    if (error) {
        die("memory type", error, NULL);
    }
    error = pf_wasmtime_memory_new(shared->context, memory_type, &shared->memory);
    if (error) {
        die("memory", error, NULL);
    }

    table_limits.min = table_size;
    table_limits.max = table_size;
    table_type = pf_wasm_tabletype_new(pf_wasm_valtype_new(WASM_FUNCREF), &table_limits);
    memset(&null_func, 0, sizeof null_func);
    null_func.kind = WASMTIME_FUNCREF;
    error = pf_wasmtime_table_new(shared->context, table_type, &null_func, &shared->table);
    if (error) {
        die("table", error, NULL);
    }

    shared->linker = pf_wasmtime_linker_new(shared->engine);
    memset(&item, 0, sizeof item);
    item.kind = WASMTIME_EXTERN_MEMORY;
    item.of.memory = shared->memory;
    error = pf_wasmtime_linker_define(shared->linker, shared->context, "env", 3, "memory", 6, &item);
    if (error) {
        die("define memory", error, NULL);
    }
    item.kind = WASMTIME_EXTERN_TABLE;
    item.of.table = shared->table;
    error = pf_wasmtime_linker_define(shared->linker, shared->context, "env", 3, "__indirect_function_table", 25, &item);
    if (error) {
        die("define table", error, NULL);
    }
}

static wasmtime_instance_t instantiate(shared_t *shared, const char *path)
{
    size_t size;
    unsigned char *wasm = read_file(path, &size);
    wasmtime_module_t *module;
    wasmtime_instance_t instance;
    wasm_trap_t *trap = NULL;
    wasmtime_error_t *error = pf_wasmtime_module_new(shared->engine, wasm, size, &module);
    if (error) {
        die("compile", error, NULL);
    }
    error = pf_wasmtime_linker_instantiate(shared->linker, shared->context, module, &instance, &trap);
    if (error || trap) {
        die("instantiate", error, trap);
    }
    return instance;
}

static wasmtime_func_t export_func(shared_t *shared, wasmtime_instance_t *instance, const char *name)
{
    wasmtime_extern_t item;
    if (!pf_wasmtime_instance_export_get(shared->context, instance, name, strlen(name), &item)
        || item.kind != WASMTIME_EXTERN_FUNC) {
        fprintf(stderr, "no function export %s\n", name);
        exit(1);
    }
    return item.of.func;
}

static int call_i32(shared_t *shared, const wasmtime_func_t *func, int argument)
{
    wasmtime_val_raw_t raw[2];
    wasm_trap_t *trap = NULL;
    wasmtime_error_t *error;
    raw[0].i32 = argument;
    error = pf_wasmtime_func_call_unchecked(shared->context, func, raw, 2, &trap);
    if (error || trap) {
        die("call", error, trap);
    }
    return raw[0].i32;
}

static int read_i32(shared_t *shared, unsigned address)
{
    unsigned char *base = pf_wasmtime_memory_data(shared->context, &shared->memory);
    int value;
    if ((size_t)address + 4 > pf_wasmtime_memory_data_size(shared->context, &shared->memory)) {
        fprintf(stderr, "address %#x is outside the memory\n", address);
        exit(1);
    }
    memcpy(&value, base + address, 4);
    return value;
}

typedef struct {
    const char *name;
    const char *path;
    unsigned data_base;
    unsigned table_base;
    int slot_arg;
    int slot_result;
    int data_word;
    wasmtime_instance_t instance;
} module_t;

static int run_modules(const char *dll_path, int argc, char **argv)
{
    unsigned pages;
    unsigned table_size;
    unsigned window;
    unsigned slots;
    int count;
    module_t modules[8];
    shared_t shared;
    int i;
    int j;
    int failed = 0;

    if (argc < 4 + 7 || (argc - 4) % 7) {
        fprintf(stderr, "usage: host --modules PAGES TABLE_SIZE WINDOW SLOTS (NAME PATH DATA_BASE TABLE_BASE ARG RESULT WORD)...\n");
        return 2;
    }
    pages = (unsigned)atol(argv[0]);
    table_size = (unsigned)atol(argv[1]);
    window = (unsigned)atol(argv[2]);
    slots = (unsigned)atol(argv[3]);
    count = (argc - 4) / 7;
    if (count > 8) {
        fprintf(stderr, "at most 8 modules\n");
        return 2;
    }
    for (i = 0; i < count; i++) {
        char **f = argv + 4 + 7 * i;
        modules[i].name = f[0];
        modules[i].path = f[1];
        modules[i].data_base = (unsigned)atol(f[2]);
        modules[i].table_base = (unsigned)atol(f[3]);
        modules[i].slot_arg = atoi(f[4]);
        modules[i].slot_result = atoi(f[5]);
        modules[i].data_word = atoi(f[6]);
    }

    for (i = 0; i < count; i++) {
        if ((unsigned long)modules[i].data_base + window > (unsigned long)pages * 65536ul
            || modules[i].table_base + slots > table_size) {
            fprintf(stderr, "module %s: window [%#x, %#x) or table [%u, %u) does not fit the memory or table\n",
                    modules[i].name, modules[i].data_base, modules[i].data_base + window,
                    modules[i].table_base, modules[i].table_base + slots);
            failed = 1;
        }
        for (j = i + 1; j < count; j++) {
            if (modules[i].data_base < modules[j].data_base + window && modules[j].data_base < modules[i].data_base + window) {
                fprintf(stderr, "modules %s and %s overlap in memory: [%#x, %#x) and [%#x, %#x)\n",
                        modules[i].name, modules[j].name, modules[i].data_base, modules[i].data_base + window,
                        modules[j].data_base, modules[j].data_base + window);
                failed = 1;
            }
            if (modules[i].table_base < modules[j].table_base + slots && modules[j].table_base < modules[i].table_base + slots) {
                fprintf(stderr, "modules %s and %s overlap in the table: [%u, %u) and [%u, %u)\n",
                        modules[i].name, modules[j].name, modules[i].table_base, modules[i].table_base + slots,
                        modules[j].table_base, modules[j].table_base + slots);
                failed = 1;
            }
        }
    }
    if (failed) {
        return 1;
    }

    load_api(dll_path);
    make_shared(&shared, pages, table_size);
    for (i = 0; i < count; i++) {
        modules[i].instance = instantiate(&shared, modules[i].path);
    }

    for (i = 0; i < count; i++) {
        module_t *m = &modules[i];
        wasmtime_func_t data_export = export_func(&shared, &m->instance, "module_data");
        wasmtime_func_t slot_export = export_func(&shared, &m->instance, "module_slot");
        unsigned data = (unsigned)call_i32(&shared, &data_export, 0);
        unsigned cell = (unsigned)call_i32(&shared, &slot_export, 0);
        unsigned index = (unsigned)read_i32(&shared, cell);
        wasmtime_val_t entry;
        int result;
        if (data < m->data_base || data >= m->data_base + window || cell < m->data_base || cell >= m->data_base + window) {
            fprintf(stderr, "module %s: data %#x or slot cell %#x is outside its window at %#x\n", m->name, data, cell, m->data_base);
            failed = 1;
        }
        if (index != m->table_base) {
            fprintf(stderr, "module %s: slot at table index %u, its table base is %u\n", m->name, index, m->table_base);
            failed = 1;
        }
        if (!pf_wasmtime_table_get(shared.context, &shared.table, index, &entry) || entry.kind != WASMTIME_FUNCREF
            || wasmtime_funcref_is_null(&entry.of.funcref)) {
            fprintf(stderr, "module %s: table index %u is empty\n", m->name, index);
            failed = 1;
            continue;
        }
        result = call_i32(&shared, &entry.of.funcref, m->slot_arg);
        printf("%s: data at %#x, slot at table index %u, slot(%d) = %d\n", m->name, data, index, m->slot_arg, result);
        if (result != m->slot_result) {
            fprintf(stderr, "module %s: slot returned %d, expected %d\n", m->name, result, m->slot_result);
            failed = 1;
        }
    }
    for (i = 0; i < count; i++) {
        wasmtime_func_t data_export = export_func(&shared, &modules[i].instance, "module_data");
        int word = read_i32(&shared, (unsigned)call_i32(&shared, &data_export, 0));
        if (word != modules[i].data_word) {
            fprintf(stderr, "module %s: data word is %d after all ran, expected %d\n", modules[i].name, word, modules[i].data_word);
            failed = 1;
        }
    }
    if (failed) {
        return 1;
    }
    printf("ok: %d modules ran their slot through one table, data intact\n", count);
    return 0;
}

int main(int argc, char **argv)
{
    const char *dll_path = "wasmtime_host.dll";
    const char *wasm_path;
    const char *export_name;
    long loops = 1;
    int first_arg;
    int nargs;
    int i;
    shared_t shared;
    wasmtime_instance_t instance;
    wasmtime_extern_t export_item;
    wasmtime_error_t *error;
    wasm_trap_t *trap = NULL;
    wasmtime_val_raw_t raw[8];
    wasmtime_val_t slot;
    double start;
    double elapsed;
    long n;

    for (i = 1; i + 1 < argc && argv[i][0] == '-'; i += 2) {
        if (!strcmp(argv[i], "--dll")) {
            dll_path = argv[i + 1];
        } else if (!strcmp(argv[i], "--loop")) {
            loops = atol(argv[i + 1]);
        } else if (!strcmp(argv[i], "--modules")) {
            return run_modules(dll_path, argc - i - 1, argv + i + 1);
        } else {
            fprintf(stderr, "unknown option %s\n", argv[i]);
            return 2;
        }
    }
    if (argc - i < 2) {
        fprintf(stderr, "usage: host [--dll path] [--loop n] module.wasm export [i32 args]\n");
        return 2;
    }
    wasm_path = argv[i];
    export_name = argv[i + 1];
    first_arg = i + 2;
    nargs = argc - first_arg;
    if (nargs > 8) {
        fprintf(stderr, "at most 8 arguments\n");
        return 2;
    }

    load_api(dll_path);
    make_shared(&shared, 1, 8);
    instance = instantiate(&shared, wasm_path);

    if (!pf_wasmtime_table_get(shared.context, &shared.table, 0, &slot) || slot.kind != WASMTIME_FUNCREF || wasmtime_funcref_is_null(&slot.of.funcref)) {
        fprintf(stderr, "table slot 0 was not filled\n");
        return 1;
    }
    printf("table slot 0 filled by the module\n");

    if (!pf_wasmtime_instance_export_get(shared.context, &instance, export_name, strlen(export_name), &export_item)
        || export_item.kind != WASMTIME_EXTERN_FUNC) {
        fprintf(stderr, "no function export %s\n", export_name);
        return 1;
    }

    start = seconds();
    for (n = 0; n < loops; n++) {
        for (i = 0; i < nargs; i++) {
            raw[i].i32 = atoi(argv[first_arg + i]);
        }
        error = pf_wasmtime_func_call_unchecked(shared.context, &export_item.of.func, raw, 8, &trap);
        if (error || trap) {
            die("call", error, trap);
        }
    }
    elapsed = seconds() - start;

    printf("%s(", export_name);
    for (i = 0; i < nargs; i++) {
        printf(i ? ", %s" : "%s", argv[first_arg + i]);
    }
    printf(") = %d\n", raw[0].i32);
    printf("memory[16] = %d\n", read_i32(&shared, 16));
    if (loops > 1) {
        printf("%ld calls in %.3f s: %.0f calls/s\n", loops, elapsed, (double)loops / elapsed);
    }
    return 0;
}
