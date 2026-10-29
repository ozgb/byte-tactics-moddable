#ifndef WASMLOADER_H
#define WASMLOADER_H

#include <stddef.h>

#ifndef WASM_API_EXTERN
#define WASM_API_EXTERN
#endif
#include <wasmtime.h>

#ifdef _WIN32
#define WL_CALL __cdecl
#else
#define WL_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wl_loader wl_loader;
typedef struct wl_module wl_module;

typedef void* (WL_CALL *wl_open_fn)(const char* path);
typedef void* (WL_CALL *wl_symbol_fn)(void* library, const char* name);
typedef void (WL_CALL *wl_close_fn)(void* library);
typedef void (WL_CALL *wl_trap_fn)(void* user, const char* module, const char* message);
typedef const char* (WL_CALL *wl_import_fn)(void* user, wasmtime_val_raw_t* args_and_results);

typedef struct wl_config {
    const char* dll_path;
    wl_open_fn open;
    wl_symbol_fn symbol;
    wl_close_fn close;
    wl_trap_fn on_trap;
    void* trap_user;
} wl_config;

typedef struct wl_layout {
    unsigned pages;
    unsigned table_size;
    unsigned window;
    unsigned table_slots;
    unsigned stack_size;
    int has_block;
    unsigned block_base;
    unsigned block_size;
    unsigned scratch_base;
    unsigned scratch_size;
} wl_layout;

wl_loader* WL_CALL wl_create(const wl_config* config, char* error, size_t error_size);
void WL_CALL wl_destroy(wl_loader* loader);
const char* WL_CALL wl_error(const wl_loader* loader);

int WL_CALL wl_load_manifest_file(wl_loader* loader, const char* path);
int WL_CALL wl_load_manifest_text(wl_loader* loader, const char* text, size_t size, const char* name);
const wl_layout* WL_CALL wl_get_layout(const wl_loader* loader);
size_t WL_CALL wl_manifest_module_count(const wl_loader* loader);
const char* WL_CALL wl_manifest_module_name(const wl_loader* loader, size_t index);
int WL_CALL wl_module_int(const wl_loader* loader, const char* module, const char* key, long* value);

int WL_CALL wl_create_memory(wl_loader* loader);
unsigned char* WL_CALL wl_memory_base(wl_loader* loader);
size_t WL_CALL wl_memory_size(wl_loader* loader);

int WL_CALL wl_define_import(wl_loader* loader, const char* module, const char* name, const char* signature, wl_import_fn function, void* user);

void WL_CALL wl_trap_missing_imports(wl_loader* loader, int enable);

wl_module* WL_CALL wl_load_module(wl_loader* loader, const char* name, const char* wasm_path);
wl_module* WL_CALL wl_load_module_bytes(wl_loader* loader, const char* name, const unsigned char* wasm, size_t size);

const char* WL_CALL wl_module_name(const wl_module* module);
size_t WL_CALL wl_slot_count(const wl_module* module);
const char* WL_CALL wl_slot_name(const wl_module* module, size_t index);
const wasmtime_func_t* WL_CALL wl_slot(const wl_module* module, size_t index);
size_t WL_CALL wl_data_count(const wl_module* module);
const char* WL_CALL wl_data_name(const wl_module* module, size_t index);
unsigned WL_CALL wl_data_address(const wl_module* module, size_t index);
int WL_CALL wl_export_func(wl_loader* loader, const wl_module* module, const char* name, wasmtime_func_t* func);

int WL_CALL wl_table_entry(wl_loader* loader, unsigned index, wasmtime_val_t* value);
int WL_CALL wl_call(wl_loader* loader, const wl_module* module, const wasmtime_func_t* func, wasmtime_val_raw_t* args_and_results, size_t count);
unsigned WL_CALL wl_table_filled(const wl_module* module);

#ifdef __cplusplus
}
#endif

#endif
