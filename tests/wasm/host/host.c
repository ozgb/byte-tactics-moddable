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

int main(int argc, char **argv)
{
    const char *dll_path = "wasmtime_host.dll";
    const char *wasm_path;
    const char *export_name;
    long loops = 1;
    int first_arg;
    int nargs;
    int i;
    size_t wasm_size;
    unsigned char *wasm;
    wasm_config_t *config;
    wasm_engine_t *engine;
    wasmtime_store_t *store;
    wasmtime_context_t *context;
    wasm_memorytype_t *memory_type;
    wasm_tabletype_t *table_type;
    wasm_limits_t table_limits;
    wasmtime_memory_t memory;
    wasmtime_table_t table;
    wasmtime_val_t null_func;
    wasmtime_val_t slot;
    wasmtime_extern_t item;
    wasmtime_linker_t *linker;
    wasmtime_module_t *module;
    wasmtime_instance_t instance;
    wasmtime_extern_t export_item;
    wasmtime_error_t *error;
    wasm_trap_t *trap = NULL;
    wasmtime_val_raw_t raw[8];
    double start;
    double elapsed;
    long n;

    for (i = 1; i + 1 < argc && argv[i][0] == '-'; i += 2) {
        if (!strcmp(argv[i], "--dll")) {
            dll_path = argv[i + 1];
        } else if (!strcmp(argv[i], "--loop")) {
            loops = atol(argv[i + 1]);
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
    wasm = read_file(wasm_path, &wasm_size);

    config = pf_wasm_config_new();
    error = pf_wasmtime_config_target_set(config, "pulley32");
    if (error) {
        die("select pulley32", error, NULL);
    }
    pf_wasmtime_config_cranelift_nan_canonicalization_set(config, 1);
    pf_wasmtime_config_consume_fuel_set(config, 0);
    engine = pf_wasm_engine_new_with_config(config);
    store = pf_wasmtime_store_new(engine, NULL, NULL);
    context = pf_wasmtime_store_context(store);

    error = pf_wasmtime_memorytype_new(1, 1, 16, 0, 0, 16, &memory_type);
    if (error) {
        die("memory type", error, NULL);
    }
    error = pf_wasmtime_memory_new(context, memory_type, &memory);
    if (error) {
        die("memory", error, NULL);
    }

    table_limits.min = 8;
    table_limits.max = 8;
    table_type = pf_wasm_tabletype_new(pf_wasm_valtype_new(WASM_FUNCREF), &table_limits);
    memset(&null_func, 0, sizeof null_func);
    null_func.kind = WASMTIME_FUNCREF;
    error = pf_wasmtime_table_new(context, table_type, &null_func, &table);
    if (error) {
        die("table", error, NULL);
    }

    linker = pf_wasmtime_linker_new(engine);
    memset(&item, 0, sizeof item);
    item.kind = WASMTIME_EXTERN_MEMORY;
    item.of.memory = memory;
    error = pf_wasmtime_linker_define(linker, context, "env", 3, "memory", 6, &item);
    if (error) {
        die("define memory", error, NULL);
    }
    item.kind = WASMTIME_EXTERN_TABLE;
    item.of.table = table;
    error = pf_wasmtime_linker_define(linker, context, "env", 3, "__indirect_function_table", 25, &item);
    if (error) {
        die("define table", error, NULL);
    }

    error = pf_wasmtime_module_new(engine, wasm, wasm_size, &module);
    if (error) {
        die("compile", error, NULL);
    }
    error = pf_wasmtime_linker_instantiate(linker, context, module, &instance, &trap);
    if (error || trap) {
        die("instantiate", error, trap);
    }

    if (!pf_wasmtime_table_get(context, &table, 0, &slot) || slot.kind != WASMTIME_FUNCREF || wasmtime_funcref_is_null(&slot.of.funcref)) {
        fprintf(stderr, "table slot 0 was not filled\n");
        return 1;
    }
    printf("table slot 0 filled by the module\n");

    if (!pf_wasmtime_instance_export_get(context, &instance, export_name, strlen(export_name), &export_item)
        || export_item.kind != WASMTIME_EXTERN_FUNC) {
        fprintf(stderr, "no function export %s\n", export_name);
        return 1;
    }

    start = seconds();
    for (n = 0; n < loops; n++) {
        for (i = 0; i < nargs; i++) {
            raw[i].i32 = atoi(argv[first_arg + i]);
        }
        error = pf_wasmtime_func_call_unchecked(context, &export_item.of.func, raw, 8, &trap);
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
    printf("memory[16] = %d\n", *(int *)(pf_wasmtime_memory_data(context, &memory) + 16));
    if (loops > 1) {
        printf("%ld calls in %.3f s: %.0f calls/s\n", loops, elapsed, (double)loops / elapsed);
    }
    return 0;
}
