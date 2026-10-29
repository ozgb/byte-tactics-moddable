#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interfaces/host.h"
#include "wasmloader.h"

static void report_trap(void* user, const char* module, const char* message)
{
    (void)user;
    fprintf(stderr, "trap in module %s: %s\n", module, message);
}

static const char* import_double(void* user, wasmtime_val_raw_t* args_and_results)
{
    (void)user;
    args_and_results[0].i32 *= 2;
    return NULL;
}

static const char* import_fail(void* user, wasmtime_val_raw_t* args_and_results)
{
    (void)user;
    (void)args_and_results;
    return "the host refused the call";
}

static wl_loader* start(const char* dll_path, char* error, size_t error_size)
{
    wl_config config;
    memset(&config, 0, sizeof config);
    config.dll_path = dll_path;
    config.open = host_lib_open;
    config.symbol = host_lib_symbol;
    config.close = host_lib_close;
    config.on_trap = report_trap;
    return wl_create(&config, error, error_size);
}

static double seconds(void)
{
    return (double)host_time_ms() / 1000.0;
}

static int read_i32(wl_loader* loader, unsigned address, int* value)
{
    unsigned char* base = wl_memory_base(loader);
    if ((size_t)address + 4 > wl_memory_size(loader)) {
        fprintf(stderr, "address %#x is outside the memory\n", address);
        return 0;
    }
    memcpy(value, base + address, 4);
    return 1;
}

static int call_i32(wl_loader* loader, const wl_module* module, const wasmtime_func_t* func, int argument, int* result)
{
    wasmtime_val_raw_t raw[2];
    raw[0].i32 = argument;
    if (!wl_call(loader, module, func, raw, 2)) {
        return 0;
    }
    *result = raw[0].i32;
    return 1;
}

static int run_fixtures(wl_loader* loader, const char* wasm_dir)
{
    const wl_layout* layout = wl_get_layout(loader);
    const char* names[8];
    wl_module* modules[8];
    int count = 0;
    int failed = 0;
    int i;
    char path[512];

    for (i = 0; i < (int)wl_manifest_module_count(loader) && count < 8; i++) {
        names[count++] = wl_manifest_module_name(loader, (size_t)i);
    }
    if (!wl_create_memory(loader)) {
        fprintf(stderr, "%s\n", wl_error(loader));
        return 1;
    }
    for (i = 0; i < count; i++) {
        sprintf(path, "%s/%s.wasm", wasm_dir, names[i]);
        modules[i] = wl_load_module(loader, names[i], path);
        if (!modules[i]) {
            fprintf(stderr, "%s\n", wl_error(loader));
            return 1;
        }
    }
    for (i = 0; i < count; i++) {
        wasmtime_func_t data_export, slot_export;
        long data_base, table_base, slot_arg, slot_result;
        int data, cell, index, result;
        wasmtime_val_t entry;
        wl_module_int(loader, names[i], "data_base", &data_base);
        wl_module_int(loader, names[i], "table_base", &table_base);
        wl_module_int(loader, names[i], "slot_arg", &slot_arg);
        wl_module_int(loader, names[i], "slot_result", &slot_result);
        if (!wl_export_func(loader, modules[i], "module_data", &data_export) || !wl_export_func(loader, modules[i], "module_slot", &slot_export)
            || !call_i32(loader, modules[i], &data_export, 0, &data) || !call_i32(loader, modules[i], &slot_export, 0, &cell)
            || !read_i32(loader, (unsigned)cell, &index)) {
            fprintf(stderr, "%s\n", wl_error(loader));
            return 1;
        }
        if ((unsigned)data < (unsigned)data_base || (unsigned)data >= (unsigned)data_base + layout->window
            || (unsigned)cell < (unsigned)data_base || (unsigned)cell >= (unsigned)data_base + layout->window) {
            fprintf(stderr, "module %s: data %#x or slot cell %#x is outside its window at %#lx\n", names[i], data, cell, data_base);
            failed = 1;
        }
        if (index != table_base) {
            fprintf(stderr, "module %s: slot at table index %d, its table base is %ld\n", names[i], index, table_base);
            failed = 1;
        }
        if (!wl_table_entry(loader, (unsigned)index, &entry)) {
            fprintf(stderr, "module %s: table index %d is empty\n", names[i], index);
            failed = 1;
            continue;
        }
        if (!call_i32(loader, modules[i], &entry.of.funcref, (int)slot_arg, &result)) {
            return 1;
        }
        printf("%s: data at %#x, slot at table index %d, slot(%ld) = %d\n", names[i], data, index, slot_arg, result);
        if (result != slot_result) {
            fprintf(stderr, "module %s: slot returned %d, expected %ld\n", names[i], result, slot_result);
            failed = 1;
        }
    }
    for (i = 0; i < count; i++) {
        wasmtime_func_t data_export;
        long expected;
        int data, word;
        wl_module_int(loader, names[i], "data_word", &expected);
        if (!wl_export_func(loader, modules[i], "module_data", &data_export) || !call_i32(loader, modules[i], &data_export, 0, &data)
            || !read_i32(loader, (unsigned)data, &word)) {
            return 1;
        }
        if (word != expected) {
            fprintf(stderr, "module %s: data word is %d after all ran, expected %ld\n", names[i], word, expected);
            failed = 1;
        }
    }
    if (failed) {
        return 1;
    }
    printf("ok: %d modules ran their slot through one table, data intact\n", count);
    return 0;
}

static int run_wind_meteors(wl_loader* loader, const char* wasm_dir)
{
    const wl_layout* layout = wl_get_layout(loader);
    wl_module* module;
    char path[512];
    long table_base = 0;
    size_t i;
    if (!layout->has_block) {
        fprintf(stderr, "the manifest has no block_base\n");
        return 1;
    }
    if (!wl_create_memory(loader)) {
        fprintf(stderr, "%s\n", wl_error(loader));
        return 1;
    }
    wl_trap_missing_imports(loader, 1);
    sprintf(path, "%s/wind_meteors.wasm", wasm_dir);
    module = wl_load_module(loader, "wind_meteors", path);
    if (!module) {
        fprintf(stderr, "%s\n", wl_error(loader));
        return 1;
    }
    printf("memory: %lu pages (%lu bytes) at min = max, block at %#x, %u bytes\n", (unsigned long)(wl_memory_size(loader) / 65536),
           (unsigned long)wl_memory_size(loader), layout->block_base, layout->block_size);
    wl_module_int(loader, "wind_meteors", "table_base", &table_base);
    for (i = 0; i < wl_slot_count(module); i++) {
        const wasmtime_func_t* handle = wl_slot(module, i);
        wasmtime_val_t entry;
        printf("slot %2lu %s: handle store %lu, table entry %lu %s\n", (unsigned long)i, wl_slot_name(module, i),
               (unsigned long)handle->store_id, (unsigned long)(table_base + i),
               wl_table_entry(loader, (unsigned)(table_base + i), &entry) ? "set" : "empty");
    }
    for (i = 0; i < wl_data_count(module); i++) {
        printf("data %s at %#x\n", wl_data_name(module, i), wl_data_address(module, i));
    }
    printf("ok: %lu slots, %lu data exports, %u table entries filled by the loader\n", (unsigned long)wl_slot_count(module),
           (unsigned long)wl_data_count(module), wl_table_filled(module));
    return 0;
}

static int run_export(wl_loader* loader, const char* wasm_path, const char* export_name, int nargs, char** args, long loops)
{
    static const char manifest[] =
        "{\"memory_pages\":1,\"table_size\":8,\"window\":65536,\"table_slots\":8,"
        "\"modules\":[{\"name\":\"add\",\"data_base\":0,\"table_base\":0}]}";
    wasmtime_val_raw_t raw[8];
    wasmtime_val_t slot;
    wasmtime_func_t func;
    wl_module* module;
    double start_time;
    double elapsed;
    long n;
    int i;
    int value;

    if (!wl_load_manifest_text(loader, manifest, sizeof manifest - 1, "built-in manifest") || !wl_create_memory(loader)) {
        fprintf(stderr, "%s\n", wl_error(loader));
        return 1;
    }
    if (!wl_define_import(loader, "env", "double", "i>i", import_double, NULL) || !wl_define_import(loader, "env", "fail", ">", import_fail, NULL)) {
        fprintf(stderr, "%s\n", wl_error(loader));
        return 1;
    }
    module = wl_load_module(loader, "add", wasm_path);
    if (!module) {
        fprintf(stderr, "%s\n", wl_error(loader));
        return 1;
    }
    if (!wl_table_entry(loader, 0, &slot)) {
        fprintf(stderr, "table slot 0 was not filled\n");
        return 1;
    }
    printf("table slot 0 filled by the module\n");
    if (!wl_export_func(loader, module, export_name, &func)) {
        fprintf(stderr, "%s\n", wl_error(loader));
        return 1;
    }
    start_time = seconds();
    for (n = 0; n < loops; n++) {
        for (i = 0; i < nargs; i++) {
            raw[i].i32 = atoi(args[i]);
        }
        if (!wl_call(loader, module, &func, raw, 8)) {
            return 1;
        }
    }
    elapsed = seconds() - start_time;
    printf("%s(", export_name);
    for (i = 0; i < nargs; i++) {
        printf(i ? ", %s" : "%s", args[i]);
    }
    printf(") = %d\n", raw[0].i32);
    if (!read_i32(loader, 16, &value)) {
        return 1;
    }
    printf("memory[16] = %d\n", value);
    if (loops > 1) {
        printf("%ld calls in %.3f s: %.0f calls/s\n", loops, elapsed, (double)loops / elapsed);
    }
    return 0;
}

int main(int argc, char** argv)
{
    const char* dll_path = "wasmtime_host.dll";
    const char* manifest = NULL;
    const char* wasm_dir = ".";
    int mode = 0;
    long loops = 1;
    int nargs;
    int i;
    char error[512];
    wl_loader* loader;
    int result;

    for (i = 1; i < argc && argv[i][0] == '-'; i++) {
        if (i + 1 < argc && !strcmp(argv[i], "--dll")) {
            dll_path = argv[++i];
        } else if (i + 1 < argc && !strcmp(argv[i], "--loop")) {
            loops = atol(argv[++i]);
        } else if (i + 1 < argc && !strcmp(argv[i], "--manifest")) {
            manifest = argv[++i];
        } else if (i + 1 < argc && !strcmp(argv[i], "--wasm-dir")) {
            wasm_dir = argv[++i];
        } else if (!strcmp(argv[i], "--fixtures")) {
            mode = 1;
        } else if (!strcmp(argv[i], "--wind-meteors")) {
            mode = 2;
        } else {
            fprintf(stderr, "unknown option %s\n", argv[i]);
            return 2;
        }
    }
    if (mode == 0 && argc - i < 2) {
        fprintf(stderr, "usage: host [--dll path] [--loop n] module.wasm export [i32 args]\n"
                        "       host [--dll path] --manifest modules.json --wasm-dir dir (--fixtures | --wind-meteors)\n");
        return 2;
    }
    if (mode == 0 && argc - i - 2 > 8) {
        fprintf(stderr, "at most 8 arguments\n");
        return 2;
    }
    if (mode != 0 && !manifest) {
        fprintf(stderr, "--fixtures and --wind-meteors need --manifest\n");
        return 2;
    }

    loader = start(dll_path, error, sizeof error);
    if (!loader) {
        fprintf(stderr, "%s\n", error);
        return 2;
    }
    if (mode == 0) {
        nargs = argc - i - 2;
        result = run_export(loader, argv[i], argv[i + 1], nargs, argv + i + 2, loops);
    } else {
        if (!wl_load_manifest_file(loader, manifest)) {
            fprintf(stderr, "%s\n", wl_error(loader));
            return 1;
        }
        result = mode == 1 ? run_fixtures(loader, wasm_dir) : run_wind_meteors(loader, wasm_dir);
    }
    return result;
}
