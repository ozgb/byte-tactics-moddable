#include "wasmloader.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "format.h"
#include "json.h"

#define WL_API(X)                                         \
    X(wasm_config_new)                                    \
    X(wasmtime_config_target_set)                         \
    X(wasmtime_config_cranelift_nan_canonicalization_set) \
    X(wasmtime_config_consume_fuel_set)                   \
    X(wasmtime_config_wasm_relaxed_simd_deterministic_set) \
    X(wasm_engine_new_with_config)                        \
    X(wasm_engine_delete)                                 \
    X(wasmtime_store_new)                                 \
    X(wasmtime_store_delete)                              \
    X(wasmtime_store_context)                             \
    X(wasmtime_memorytype_new)                            \
    X(wasmtime_memory_new)                                \
    X(wasmtime_memory_data)                               \
    X(wasmtime_memory_data_size)                          \
    X(wasm_valtype_new)                                   \
    X(wasm_valtype_vec_new_uninitialized)                 \
    X(wasm_functype_new)                                  \
    X(wasm_functype_delete)                               \
    X(wasm_tabletype_new)                                 \
    X(wasmtime_global_get)                                \
    X(wasmtime_table_new)                                 \
    X(wasmtime_table_get)                                 \
    X(wasmtime_table_set)                                 \
    X(wasmtime_module_new)                                \
    X(wasmtime_module_delete)                             \
    X(wasmtime_linker_new)                                \
    X(wasmtime_linker_delete)                             \
    X(wasmtime_linker_allow_shadowing)                    \
    X(wasmtime_linker_define)                             \
    X(wasmtime_linker_define_func_unchecked)              \
    X(wasmtime_linker_define_unknown_imports_as_traps)    \
    X(wasmtime_linker_instantiate)                        \
    X(wasmtime_instance_export_get)                       \
    X(wasmtime_func_call_unchecked)                       \
    X(wasmtime_trap_new)                                  \
    X(wasmtime_error_message)                             \
    X(wasmtime_error_delete)                              \
    X(wasm_trap_message)                                  \
    X(wasm_trap_delete)                                   \
    X(wasm_byte_vec_delete)

typedef struct {
#define WL_FIELD(name) __typeof__(name)* name;
    WL_API(WL_FIELD)
#undef WL_FIELD
} wl_api;

#define WL_MAX_VALUES 16

enum {
    WL_RESULT_PASS,
    WL_RESULT_CONST,
    WL_RESULT_VOID
};

typedef struct {
    unsigned caller_room;
    unsigned callee_room;
    unsigned callee_params;
    int from_caller[WL_MAX_VALUES];
    long constant[WL_MAX_VALUES];
    int result;
    long result_constant;
} wl_adapter;

typedef struct {
    unsigned count;
    const char* kind[WL_MAX_VALUES];
    const char* result;
} wl_sig;

struct wl_module {
    char* name;
    const wl_json* entry;
    wasmtime_instance_t instance;
    size_t slot_count;
    char** slot_names;
    wasmtime_func_t* slots;
    size_t data_count;
    char** data_names;
    unsigned* data_addresses;
    unsigned table_filled;
    wl_adapter** slot_adapters;
    wl_module* next;
};

typedef struct import_record {
    char* module;
    char* name;
    char* signature;
    wl_import_fn function;
    void* user;
    struct import_record* next;
} import_record;

struct wl_loader {
    wl_config config;
    void* library;
    wl_api api;
    char error[512];
    wl_json* manifest;
    wl_layout layout;
    wasm_engine_t* engine;
    wasmtime_store_t* store;
    wasmtime_context_t* context;
    wasmtime_memory_t memory;
    wasmtime_table_t table;
    wasmtime_linker_t* linker;
    int has_memory;
    int trap_missing;
    import_record* imports;
    wl_module* modules;
};

typedef struct {
    wl_loader* loader;
    wl_import_fn function;
    void* user;
} import_env;

typedef struct {
    wl_loader* loader;
    wl_import_fn function;
    void* user;
    wl_adapter adapter;
} adapter_env;

static void set_error(wl_loader* loader, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    wl_vformat(loader->error, sizeof loader->error, format, args);
    va_end(args);
}

static void describe(wl_loader* loader, wasmtime_error_t* error, wasm_trap_t* trap, char* out, size_t size)
{
    wasm_byte_vec_t message;
    message.size = 0;
    message.data = NULL;
    if (error) {
        loader->api.wasmtime_error_message(error, &message);
        loader->api.wasmtime_error_delete(error);
    } else if (trap) {
        loader->api.wasm_trap_message(trap, &message);
        loader->api.wasm_trap_delete(trap);
    }
    wl_format(out, size, "%.*s", (int)message.size, message.data ? message.data : "");
    if (message.data) {
        loader->api.wasm_byte_vec_delete(&message);
    }
}

static int fail_wasmtime(wl_loader* loader, const char* what, wasmtime_error_t* error, wasm_trap_t* trap)
{
    char text[400];
    describe(loader, error, trap, text, sizeof text);
    set_error(loader, "%s failed: %s", what, text);
    return 0;
}

static int load_api(wl_loader* loader, char* error, size_t error_size)
{
    const char* missing = NULL;
    wl_api* api = &loader->api;
    loader->library = loader->config.open(loader->config.dll_path);
    if (!loader->library) {
        wl_format(error, error_size, "cannot load %s", loader->config.dll_path);
        return 0;
    }
#define WL_LOAD(name)                                                              \
    if (!missing) {                                                                \
        api->name = (__typeof__(name)*)loader->config.symbol(loader->library, #name); \
        if (!api->name) {                                                          \
            missing = #name;                                                       \
        }                                                                          \
    }
    WL_API(WL_LOAD)
#undef WL_LOAD
    if (missing) {
        wl_format(error, error_size, "%s has no export %s", loader->config.dll_path, missing);
        return 0;
    }
    return 1;
}

static int make_engine(wl_loader* loader)
{
    wl_api* api = &loader->api;
    wasm_config_t* config = api->wasm_config_new();
    const char* target = loader->config.target ? loader->config.target : "pulley32";
    wasmtime_error_t* error = api->wasmtime_config_target_set(config, target);
    if (error) {
        return fail_wasmtime(loader, "selecting the target", error, NULL);
    }
    api->wasmtime_config_cranelift_nan_canonicalization_set(config, 1);
    api->wasmtime_config_consume_fuel_set(config, 0);
    api->wasmtime_config_wasm_relaxed_simd_deterministic_set(config, 1);
    loader->engine = api->wasm_engine_new_with_config(config);
    loader->store = api->wasmtime_store_new(loader->engine, NULL, NULL);
    loader->context = api->wasmtime_store_context(loader->store);
    loader->linker = api->wasmtime_linker_new(loader->engine);
    api->wasmtime_linker_allow_shadowing(loader->linker, 1);
    return 1;
}

wl_loader* wl_create(const wl_config* config, char* error, size_t error_size)
{
    wl_loader* loader = (wl_loader*)calloc(1, sizeof *loader);
    loader->config = *config;
    if (!load_api(loader, error, error_size) || !make_engine(loader)) {
        if (loader->error[0]) {
            wl_format(error, error_size, "%s", loader->error);
        }
        wl_destroy(loader);
        return NULL;
    }
    return loader;
}

static char* copy_string(const char* text)
{
    size_t n = strlen(text) + 1;
    char* out = (char*)malloc(n);
    memcpy(out, text, n);
    return out;
}

static void free_names(char** names, size_t count)
{
    size_t i;
    for (i = 0; i < count; i++) {
        free(names[i]);
    }
    free(names);
}

static void free_adapters(wl_module* module)
{
    size_t i;
    if (module->slot_adapters) {
        for (i = 0; i < module->slot_count; i++) {
            free(module->slot_adapters[i]);
        }
        free(module->slot_adapters);
    }
}

void wl_destroy(wl_loader* loader)
{
    wl_module* module;
    if (!loader) {
        return;
    }
    module = loader->modules;
    while (module) {
        wl_module* next = module->next;
        free(module->name);
        free_names(module->slot_names, module->slot_count);
        free(module->slots);
        free_names(module->data_names, module->data_count);
        free(module->data_addresses);
        free_adapters(module);
        free(module);
        module = next;
    }
    while (loader->imports) {
        import_record* next = loader->imports->next;
        free(loader->imports->module);
        free(loader->imports->name);
        free(loader->imports->signature);
        free(loader->imports);
        loader->imports = next;
    }
    if (loader->linker) {
        loader->api.wasmtime_linker_delete(loader->linker);
    }
    if (loader->store) {
        loader->api.wasmtime_store_delete(loader->store);
    }
    if (loader->engine) {
        loader->api.wasm_engine_delete(loader->engine);
    }
    wl_json_free(loader->manifest);
    if (loader->library && loader->config.close) {
        loader->config.close(loader->library);
    }
    free(loader);
}

const char* wl_error(const wl_loader* loader)
{
    return loader->error;
}

static const wl_json* find_entry(const wl_loader* loader, const char* name)
{
    const wl_json* list = wl_json_get(loader->manifest, "modules");
    const wl_json* entry;
    for (entry = list ? list->first : NULL; entry; entry = entry->next) {
        const wl_json* entry_name = wl_json_get(entry, "name");
        if (entry_name && !strcmp(entry_name->text, name)) {
            return entry;
        }
    }
    return NULL;
}

static int read_unsigned(wl_loader* loader, const wl_json* object, const char* key, const char* owner, int required, unsigned* out)
{
    const wl_json* value = wl_json_get(object, key);
    if (!value) {
        if (required) {
            set_error(loader, "%s: %s is missing", owner, key);
            return 0;
        }
        return 1;
    }
    if (value->kind != WL_JSON_NUMBER || value->number < 0) {
        set_error(loader, "%s: %s must be a non-negative integer", owner, key);
        return 0;
    }
    *out = (unsigned)value->number;
    return 1;
}

static int check_strings(wl_loader* loader, const wl_json* entry, const char* key, const char* name)
{
    const wl_json* list = wl_json_get(entry, key);
    const wl_json* item;
    if (!list) {
        return 1;
    }
    if (list->kind != WL_JSON_ARRAY) {
        set_error(loader, "module %s: %s must be a list of names", name, key);
        return 0;
    }
    for (item = list->first; item; item = item->next) {
        if (item->kind != WL_JSON_STRING) {
            set_error(loader, "module %s: %s must be a list of names", name, key);
            return 0;
        }
    }
    return 1;
}

static int check_layout(wl_loader* loader)
{
    wl_layout* l = &loader->layout;
    const wl_json* list = wl_json_get(loader->manifest, "modules");
    const wl_json* a;
    const wl_json* b;
    unsigned long memory_bytes = (unsigned long)l->pages * 65536ul;
    for (a = list->first; a; a = a->next) {
        const char* an = wl_json_get(a, "name")->text;
        unsigned long a_low = (unsigned long)wl_json_get(a, "data_base")->number;
        unsigned long a_high = a_low + l->window;
        unsigned a_table = (unsigned)wl_json_get(a, "table_base")->number;
        if (a_high > memory_bytes || a_table + l->table_slots > l->table_size) {
            set_error(loader, "module %s: window [%#lx, %#lx) or table [%u, %u) does not fit the memory or table",
                      an, a_low, a_high, a_table, a_table + l->table_slots);
            return 0;
        }
        if (l->has_block && a_high > l->block_base) {
            set_error(loader, "module %s: window [%#lx, %#lx) reaches the block at %#x", an, a_low, a_high, l->block_base);
            return 0;
        }
        if (l->scratch_size && a_high > l->scratch_base) {
            set_error(loader, "module %s: window [%#lx, %#lx) reaches the scratch area at %#x", an, a_low, a_high, l->scratch_base);
            return 0;
        }
        for (b = a->next; b; b = b->next) {
            const char* bn = wl_json_get(b, "name")->text;
            unsigned long b_low = (unsigned long)wl_json_get(b, "data_base")->number;
            unsigned long b_high = b_low + l->window;
            unsigned b_table = (unsigned)wl_json_get(b, "table_base")->number;
            if (a_low < b_high && b_low < a_high) {
                set_error(loader, "modules %s and %s overlap in memory: [%#lx, %#lx) and [%#lx, %#lx)",
                          an, bn, a_low, a_high, b_low, b_high);
                return 0;
            }
            if (a_table < b_table + l->table_slots && b_table < a_table + l->table_slots) {
                set_error(loader, "modules %s and %s overlap in the table: [%u, %u) and [%u, %u)",
                          an, bn, a_table, a_table + l->table_slots, b_table, b_table + l->table_slots);
                return 0;
            }
        }
    }
    return 1;
}

int wl_load_manifest_text(wl_loader* loader, const char* text, size_t size, const char* name)
{
    wl_json* root = wl_json_parse(text, size, name, loader->error, sizeof loader->error);
    const wl_json* list;
    const wl_json* entry;
    const wl_json* scratch;
    wl_layout layout;
    char owner[128];
    memset(&layout, 0, sizeof layout);
    if (!root) {
        return 0;
    }
    wl_format(owner, sizeof owner, "%s", name);
    if (root->kind != WL_JSON_OBJECT) {
        set_error(loader, "%s: the manifest must be an object", name);
        goto bad;
    }
    if (!read_unsigned(loader, root, "memory_pages", owner, 1, &layout.pages)
        || !read_unsigned(loader, root, "table_size", owner, 1, &layout.table_size)
        || !read_unsigned(loader, root, "window", owner, 1, &layout.window)
        || !read_unsigned(loader, root, "table_slots", owner, 1, &layout.table_slots)
        || !read_unsigned(loader, root, "stack_size", owner, 0, &layout.stack_size)) {
        goto bad;
    }
    if (layout.pages == 0 || layout.pages > 65536 || layout.window == 0) {
        set_error(loader, "%s: memory_pages and window must be positive, and memory_pages at most 65536", name);
        goto bad;
    }
    if (wl_json_get(root, "block_base")) {
        layout.has_block = 1;
        if (!read_unsigned(loader, root, "block_base", owner, 1, &layout.block_base)
            || !read_unsigned(loader, root, "block_size", owner, 1, &layout.block_size)) {
            goto bad;
        }
        if ((unsigned long)layout.block_base + layout.block_size > (unsigned long)layout.pages * 65536ul) {
            set_error(loader, "%s: the block [%#x, %#x) does not fit the memory", name, layout.block_base, layout.block_base + layout.block_size);
            goto bad;
        }
    }
    scratch = wl_json_get(root, "scratch");
    if (scratch) {
        if (scratch->kind != WL_JSON_ARRAY || wl_json_count(scratch) != 2 || scratch->first->kind != WL_JSON_NUMBER
            || scratch->first->next->kind != WL_JSON_NUMBER || scratch->first->number < 0 || scratch->first->next->number < 0) {
            set_error(loader, "%s: scratch must be [base, size]", name);
            goto bad;
        }
        layout.scratch_base = (unsigned)scratch->first->number;
        layout.scratch_size = (unsigned)scratch->first->next->number;
        if ((unsigned long)layout.scratch_base + layout.scratch_size > (unsigned long)layout.pages * 65536ul
            || (layout.has_block && layout.scratch_base + layout.scratch_size > layout.block_base)) {
            set_error(loader, "%s: scratch [%#x, %#x) does not fit below the block", name, layout.scratch_base, layout.scratch_base + layout.scratch_size);
            goto bad;
        }
    }
    list = wl_json_get(root, "modules");
    if (!list || list->kind != WL_JSON_ARRAY || !list->first) {
        set_error(loader, "%s: modules must be a non-empty list", name);
        goto bad;
    }
    for (entry = list->first; entry; entry = entry->next) {
        const wl_json* entry_name = wl_json_get(entry, "name");
        const wl_json* other;
        unsigned unused = 0;
        if (entry->kind != WL_JSON_OBJECT || !entry_name || entry_name->kind != WL_JSON_STRING || !entry_name->text[0]) {
            set_error(loader, "%s: every module needs a name", name);
            goto bad;
        }
        wl_format(owner, sizeof owner, "%s: module %s", name, entry_name->text);
        if (!read_unsigned(loader, entry, "data_base", owner, 1, &unused)
            || !read_unsigned(loader, entry, "table_base", owner, 1, &unused)
            || !check_strings(loader, entry, "exports", entry_name->text)
            || !check_strings(loader, entry, "data", entry_name->text)) {
            goto bad;
        }
        for (other = list->first; other != entry; other = other->next) {
            if (!strcmp(wl_json_get(other, "name")->text, entry_name->text)) {
                set_error(loader, "%s: module %s is listed twice", name, entry_name->text);
                goto bad;
            }
        }
    }
    wl_json_free(loader->manifest);
    loader->manifest = root;
    loader->layout = layout;
    if (!check_layout(loader)) {
        wl_json_free(loader->manifest);
        loader->manifest = NULL;
        return 0;
    }
    return 1;
bad:
    wl_json_free(root);
    return 0;
}

int wl_load_manifest_file(wl_loader* loader, const char* path)
{
    FILE* file = fopen(path, "rb");
    char* text;
    long length;
    int ok;
    if (!file) {
        set_error(loader, "cannot open %s", path);
        return 0;
    }
    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);
    text = (char*)malloc((size_t)length + 1);
    if (length < 0 || fread(text, 1, (size_t)length, file) != (size_t)length) {
        set_error(loader, "cannot read %s", path);
        fclose(file);
        free(text);
        return 0;
    }
    fclose(file);
    ok = wl_load_manifest_text(loader, text, (size_t)length, path);
    free(text);
    return ok;
}

const wl_layout* wl_get_layout(const wl_loader* loader)
{
    return loader->manifest ? &loader->layout : NULL;
}

size_t wl_manifest_module_count(const wl_loader* loader)
{
    return loader->manifest ? wl_json_count(wl_json_get(loader->manifest, "modules")) : 0;
}

const char* wl_manifest_module_name(const wl_loader* loader, size_t index)
{
    const wl_json* entry = loader->manifest ? wl_json_get(loader->manifest, "modules")->first : NULL;
    while (entry && index--) {
        entry = entry->next;
    }
    return entry ? wl_json_get(entry, "name")->text : NULL;
}

int wl_module_int(const wl_loader* loader, const char* module, const char* key, long* value)
{
    const wl_json* entry = loader->manifest ? find_entry(loader, module) : NULL;
    const wl_json* item = wl_json_get(entry, key);
    if (!item || item->kind != WL_JSON_NUMBER) {
        return 0;
    }
    *value = item->number;
    return 1;
}

int wl_create_memory(wl_loader* loader)
{
    wl_api* api = &loader->api;
    const wl_layout* layout = &loader->layout;
    wasm_memorytype_t* memory_type;
    wasm_tabletype_t* table_type;
    wasm_limits_t limits;
    wasmtime_val_t null_func;
    wasmtime_extern_t item;
    wasmtime_error_t* error;
    if (!loader->manifest) {
        set_error(loader, "no manifest is loaded");
        return 0;
    }
    if (loader->has_memory) {
        set_error(loader, "the memory is already created");
        return 0;
    }
    error = api->wasmtime_memorytype_new(layout->pages, 1, layout->pages, 0, 0, 16, &memory_type);
    if (error) {
        return fail_wasmtime(loader, "memory type", error, NULL);
    }
    error = api->wasmtime_memory_new(loader->context, memory_type, &loader->memory);
    if (error) {
        return fail_wasmtime(loader, "creating the memory", error, NULL);
    }
    limits.min = layout->table_size;
    limits.max = layout->table_size;
    table_type = api->wasm_tabletype_new(api->wasm_valtype_new(WASM_FUNCREF), &limits);
    memset(&null_func, 0, sizeof null_func);
    null_func.kind = WASMTIME_FUNCREF;
    error = api->wasmtime_table_new(loader->context, table_type, &null_func, &loader->table);
    if (error) {
        return fail_wasmtime(loader, "creating the table", error, NULL);
    }
    memset(&item, 0, sizeof item);
    item.kind = WASMTIME_EXTERN_MEMORY;
    item.of.memory = loader->memory;
    error = api->wasmtime_linker_define(loader->linker, loader->context, "env", 3, "memory", 6, &item);
    if (error) {
        return fail_wasmtime(loader, "defining env.memory", error, NULL);
    }
    item.kind = WASMTIME_EXTERN_TABLE;
    item.of.table = loader->table;
    error = api->wasmtime_linker_define(loader->linker, loader->context, "env", 3, "__indirect_function_table", 25, &item);
    if (error) {
        return fail_wasmtime(loader, "defining the table", error, NULL);
    }
    loader->has_memory = 1;
    return 1;
}

unsigned char* wl_memory_base(wl_loader* loader)
{
    return loader->has_memory ? loader->api.wasmtime_memory_data(loader->context, &loader->memory) : NULL;
}

size_t wl_memory_size(wl_loader* loader)
{
    return loader->has_memory ? loader->api.wasmtime_memory_data_size(loader->context, &loader->memory) : 0;
}

static wasm_trap_t* trampoline(void* env, wasmtime_caller_t* caller, wasmtime_val_raw_t* args_and_results, size_t count)
{
    import_env* import = (import_env*)env;
    const char* message = import->function(import->user, args_and_results);
    (void)caller;
    (void)count;
    return message ? import->loader->api.wasmtime_trap_new(message, strlen(message)) : NULL;
}

static int value_kind(char letter)
{
    switch (letter) {
    case 'i':
        return WASM_I32;
    case 'l':
        return WASM_I64;
    case 'f':
        return WASM_F32;
    case 'd':
        return WASM_F64;
    }
    return -1;
}

static int fill_types(wl_loader* loader, wasm_valtype_vec_t* vec, const char* letters, size_t count, const char* what)
{
    size_t i;
    loader->api.wasm_valtype_vec_new_uninitialized(vec, count);
    for (i = 0; i < count; i++) {
        int kind = value_kind(letters[i]);
        if (kind < 0) {
            set_error(loader, "%s: unknown value type '%c' (use i, l, f or d)", what, letters[i]);
            return 0;
        }
        vec->data[i] = loader->api.wasm_valtype_new((wasm_valkind_t)kind);
    }
    return 1;
}

static int define_function(wl_loader* loader, const char* module, const char* name, const char* signature,
                           wasmtime_func_unchecked_callback_t callback, void* env, const char* what)
{
    const char* arrow = strchr(signature, '>');
    wasm_valtype_vec_t params;
    wasm_valtype_vec_t results;
    wasm_functype_t* type;
    wasmtime_error_t* error;
    if (!arrow || strlen(arrow + 1) > 1) {
        set_error(loader, "%s: signature \"%s\" must read params>result with at most one result", what, signature);
        free(env);
        return 0;
    }
    if (!fill_types(loader, &params, signature, (size_t)(arrow - signature), what)
        || !fill_types(loader, &results, arrow + 1, strlen(arrow + 1), what)) {
        free(env);
        return 0;
    }
    type = loader->api.wasm_functype_new(&params, &results);
    error = loader->api.wasmtime_linker_define_func_unchecked(loader->linker, module, strlen(module), name, strlen(name), type, callback, env, free);
    loader->api.wasm_functype_delete(type);
    if (error) {
        return fail_wasmtime(loader, what, error, NULL);
    }
    return 1;
}

static import_record* find_import(const wl_loader* loader, const char* module, const char* name)
{
    import_record* record;
    for (record = loader->imports; record; record = record->next) {
        if (!strcmp(record->module, module) && !strcmp(record->name, name)) {
            return record;
        }
    }
    return NULL;
}

static int define_host_import(wl_loader* loader, const import_record* record)
{
    import_env* env = (import_env*)malloc(sizeof *env);
    char what[160];
    wl_format(what, sizeof what, "import %s.%s", record->module, record->name);
    env->loader = loader;
    env->function = record->function;
    env->user = record->user;
    return define_function(loader, record->module, record->name, record->signature, trampoline, env, what);
}

int wl_define_import(wl_loader* loader, const char* module, const char* name, const char* signature, wl_import_fn function, void* user)
{
    import_record* record;
    if (find_import(loader, module, name)) {
        set_error(loader, "import %s.%s is already defined", module, name);
        return 0;
    }
    record = (import_record*)calloc(1, sizeof *record);
    record->module = copy_string(module);
    record->name = copy_string(name);
    record->signature = copy_string(signature);
    record->function = function;
    record->user = user;
    if (!define_host_import(loader, record)) {
        free(record->module);
        free(record->name);
        free(record->signature);
        free(record);
        return 0;
    }
    record->next = loader->imports;
    loader->imports = record;
    return 1;
}

static int is_kind(const char* kind)
{
    return !strcmp(kind, "int") || !strcmp(kind, "f64") || !strcmp(kind, "linear") || !strcmp(kind, "block")
        || !strcmp(kind, "scratch") || !strcmp(kind, "handle");
}

static int read_sig(const wl_json* node, wl_sig* sig, char* reason, size_t reason_size)
{
    const wl_json* params;
    const wl_json* item;
    memset(sig, 0, sizeof *sig);
    if (!node || node->kind != WL_JSON_ARRAY || wl_json_count(node) != 2) {
        wl_format(reason, reason_size, "a signature must read [[parameter kinds], result kind]");
        return 0;
    }
    params = node->first;
    sig->result = params->next->kind == WL_JSON_STRING ? params->next->text : NULL;
    if (params->kind != WL_JSON_ARRAY || !sig->result || !(is_kind(sig->result) || !strcmp(sig->result, "void"))) {
        wl_format(reason, reason_size, "a signature must read [[parameter kinds], result kind]");
        return 0;
    }
    if (wl_json_count(params) > WL_MAX_VALUES) {
        wl_format(reason, reason_size, "a signature has at most %d parameters", WL_MAX_VALUES);
        return 0;
    }
    for (item = params->first; item; item = item->next) {
        if (item->kind != WL_JSON_STRING || !is_kind(item->text)) {
            wl_format(reason, reason_size, "unknown parameter kind");
            return 0;
        }
        sig->kind[sig->count++] = item->text;
    }
    return 1;
}

static int same_sig(const wl_sig* a, const wl_sig* b)
{
    unsigned i;
    if (a->count != b->count || strcmp(a->result, b->result)) {
        return 0;
    }
    for (i = 0; i < a->count; i++) {
        if (strcmp(a->kind[i], b->kind[i])) {
            return 0;
        }
    }
    return 1;
}

static char kind_letter(const char* kind)
{
    return !strcmp(kind, "f64") ? 'd' : 'i';
}

static void sig_letters(const wl_sig* sig, char* out, size_t size)
{
    unsigned i;
    size_t n = 0;
    for (i = 0; i < sig->count && n + 3 < size; i++) {
        out[n++] = kind_letter(sig->kind[i]);
    }
    out[n++] = '>';
    if (strcmp(sig->result, "void")) {
        out[n++] = kind_letter(sig->result);
    }
    out[n] = 0;
}

static unsigned room_of(const wl_sig* sig)
{
    unsigned results = strcmp(sig->result, "void") ? 1u : 0u;
    return sig->count > results ? sig->count : results;
}

static int build_adapter(const wl_json* spec, const wl_sig* caller, const wl_sig* callee, wl_adapter* out, char* reason, size_t reason_size)
{
    const wl_json* args = wl_json_get(spec, "args");
    const wl_json* result = wl_json_get(spec, "result");
    const wl_json* item;
    unsigned i = 0;
    memset(out, 0, sizeof *out);
    out->caller_room = room_of(caller);
    out->callee_room = room_of(callee);
    out->callee_params = callee->count;
    if (!args || args->kind != WL_JSON_ARRAY) {
        wl_format(reason, reason_size, "args must be a list with one entry per parameter of the callee");
        return 0;
    }
    if (wl_json_count(args) != callee->count) {
        wl_format(reason, reason_size, "args has %lu entries, the callee takes %u parameters", (unsigned long)wl_json_count(args), callee->count);
        return 0;
    }
    for (item = args->first; item; item = item->next, i++) {
        if (item->kind == WL_JSON_NUMBER) {
            if (item->number < 0 || (unsigned long)item->number >= caller->count) {
                wl_format(reason, reason_size, "args[%u] reads parameter %ld, the caller passes %u", i, item->number, caller->count);
                return 0;
            }
            if (strcmp(caller->kind[item->number], callee->kind[i])) {
                wl_format(reason, reason_size, "args[%u]: the caller's %s parameter %ld cannot become the callee's %s parameter without code",
                          i, caller->kind[item->number], item->number, callee->kind[i]);
                return 0;
            }
            out->from_caller[i] = (int)item->number;
        } else if (item->kind == WL_JSON_OBJECT && wl_json_get(item, "const") && wl_json_get(item, "const")->kind == WL_JSON_NUMBER) {
            if (strcmp(callee->kind[i], "int")) {
                wl_format(reason, reason_size, "args[%u]: a constant can only fill an int parameter, the callee's is %s", i, callee->kind[i]);
                return 0;
            }
            out->from_caller[i] = -1;
            out->constant[i] = wl_json_get(item, "const")->number;
        } else {
            wl_format(reason, reason_size, "args[%u] must be a parameter index or {\"const\": n}", i);
            return 0;
        }
    }
    if (result && result->kind == WL_JSON_STRING && !strcmp(result->text, "pass")) {
        out->result = WL_RESULT_PASS;
        if (!strcmp(caller->result, "void") || !strcmp(callee->result, "void")) {
            wl_format(reason, reason_size, "result: pass needs a result on both sides, the caller's is %s and the callee's %s", caller->result, callee->result);
            return 0;
        }
        if (strcmp(caller->result, callee->result)) {
            wl_format(reason, reason_size, "result: the callee's %s cannot become the caller's %s without code", callee->result, caller->result);
            return 0;
        }
    } else if (result && result->kind == WL_JSON_STRING && !strcmp(result->text, "void")) {
        out->result = WL_RESULT_VOID;
        if (strcmp(caller->result, "void")) {
            wl_format(reason, reason_size, "result: void, but the caller expects %s", caller->result);
            return 0;
        }
    } else if (result && result->kind == WL_JSON_OBJECT && wl_json_get(result, "const") && wl_json_get(result, "const")->kind == WL_JSON_NUMBER) {
        out->result = WL_RESULT_CONST;
        out->result_constant = wl_json_get(result, "const")->number;
        if (strcmp(caller->result, "int")) {
            wl_format(reason, reason_size, "result: a constant can only be an int result, the caller's is %s", caller->result);
            return 0;
        }
    } else {
        wl_format(reason, reason_size, "result must be \"pass\", \"void\" or {\"const\": n}");
        return 0;
    }
    return 1;
}

static void adapt_args(const wl_adapter* adapter, const wasmtime_val_raw_t* caller, wasmtime_val_raw_t* callee)
{
    unsigned i;
    for (i = 0; i < adapter->callee_params; i++) {
        if (adapter->from_caller[i] >= 0) {
            callee[i] = caller[adapter->from_caller[i]];
        } else {
            memset(&callee[i], 0, sizeof callee[i]);
            callee[i].i32 = (int)adapter->constant[i];
        }
    }
}

static void adapt_result(const wl_adapter* adapter, const wasmtime_val_raw_t* callee, wasmtime_val_raw_t* caller)
{
    if (adapter->result == WL_RESULT_PASS) {
        caller[0] = callee[0];
    } else if (adapter->result == WL_RESULT_CONST) {
        memset(&caller[0], 0, sizeof caller[0]);
        caller[0].i32 = (int)adapter->result_constant;
    }
}

static wasm_trap_t* adapter_trampoline(void* env, wasmtime_caller_t* caller, wasmtime_val_raw_t* args_and_results, size_t count)
{
    adapter_env* adapted = (adapter_env*)env;
    wasmtime_val_raw_t callee[WL_MAX_VALUES];
    const char* message;
    (void)caller;
    (void)count;
    adapt_args(&adapted->adapter, args_and_results, callee);
    message = adapted->function(adapted->user, callee);
    if (message) {
        return adapted->loader->api.wasmtime_trap_new(message, strlen(message));
    }
    adapt_result(&adapted->adapter, callee, args_and_results);
    return NULL;
}

typedef struct planned {
    int is_slot;
    char* interface_name;
    char* name;
    wl_adapter adapter;
    char* old_signature;
    struct planned* next;
} planned;

static void free_plan(planned* plan)
{
    while (plan) {
        planned* next = plan->next;
        free(plan->interface_name);
        free(plan->name);
        free(plan->old_signature);
        free(plan);
        plan = next;
    }
}

static const wl_json* find_adapter(const wl_loader* loader, const char* interface_name, long from_major, const char* name)
{
    const wl_json* list = wl_json_get(loader->manifest, "adapters");
    const wl_json* spec;
    for (spec = list ? list->first : NULL; spec; spec = spec->next) {
        const wl_json* i = wl_json_get(spec, "interface");
        const wl_json* m = wl_json_get(spec, "from_major");
        const wl_json* n = wl_json_get(spec, "name");
        if (i && n && m && i->kind == WL_JSON_STRING && n->kind == WL_JSON_STRING && m->kind == WL_JSON_NUMBER
            && !strcmp(i->text, interface_name) && !strcmp(n->text, name) && m->number == from_major) {
            return spec;
        }
    }
    return NULL;
}

static int plan_changed(wl_loader* loader, const wl_json* entry, const char* module, const char* interface_name, long from_major, long to_major,
                        planned** plan)
{
    const wl_json* own = wl_json_get(entry, "interface");
    int is_slot = own && own->kind == WL_JSON_STRING && !strcmp(own->text, interface_name);
    const wl_json* table = is_slot ? wl_json_get(entry, "slots") : wl_json_get(wl_json_get(entry, "imports"), interface_name);
    const wl_json* current = wl_json_get(wl_json_get(loader->manifest, "signatures"), interface_name);
    const wl_json* item;
    const char* what = is_slot ? "slot" : "import";
    char reason[300];
    for (item = table ? table->first : NULL; item; item = item->next) {
        const wl_json* now = wl_json_get(current, item->key);
        const wl_json* spec;
        wl_sig old_sig, new_sig;
        wl_adapter adapter;
        planned* added;
        if (!now) {
            set_error(loader, "module %s: %s %s of interface %s has no current signature in the manifest (major %ld to %ld)", module, what, item->key,
                      interface_name, from_major, to_major);
            return 0;
        }
        if (!read_sig(item, &old_sig, reason, sizeof reason) || !read_sig(now, &new_sig, reason, sizeof reason)) {
            set_error(loader, "module %s: %s %s of interface %s: %s", module, what, item->key, interface_name, reason);
            return 0;
        }
        if (same_sig(&old_sig, &new_sig)) {
            continue;
        }
        spec = find_adapter(loader, interface_name, from_major, item->key);
        if (!spec) {
            set_error(loader, "module %s: %s %s of interface %s changed between major %ld and %ld, and the manifest has no adapter for it", module, what,
                      item->key, interface_name, from_major, to_major);
            return 0;
        }
        if (!(is_slot ? build_adapter(spec, &new_sig, &old_sig, &adapter, reason, sizeof reason)
                      : build_adapter(spec, &old_sig, &new_sig, &adapter, reason, sizeof reason))) {
            set_error(loader, "module %s: the adapter for %s %s of interface %s (major %ld to %ld) is refused: %s", module, what, item->key,
                      interface_name, from_major, to_major, reason);
            return 0;
        }
        added = (planned*)calloc(1, sizeof *added);
        added->is_slot = is_slot;
        added->interface_name = copy_string(interface_name);
        added->name = copy_string(item->key);
        added->adapter = adapter;
        added->next = *plan;
        *plan = added;
        if (!is_slot) {
            char letters[64];
            const import_record* host = find_import(loader, interface_name, item->key);
            sig_letters(&new_sig, letters, sizeof letters);
            if (!host || strcmp(host->signature, letters)) {
                set_error(loader, "module %s: import %s.%s is adapted from major %ld to %ld, but the host %s", module, interface_name, item->key,
                          from_major, to_major, host ? "defines it with another signature than the manifest's" : "does not define it");
                return 0;
            }
            sig_letters(&old_sig, letters, sizeof letters);
            added->old_signature = copy_string(letters);
        }
    }
    return 1;
}

static int plan_adapters(wl_loader* loader, const wl_json* entry, const char* module, planned** plan)
{
    const wl_json* majors = wl_json_get(entry, "majors");
    const wl_json* current = wl_json_get(loader->manifest, "majors");
    const wl_json* item;
    for (item = majors ? majors->first : NULL; item; item = item->next) {
        const wl_json* now = wl_json_get(current, item->key);
        if (item->kind != WL_JSON_NUMBER || !now || now->kind != WL_JSON_NUMBER) {
            set_error(loader, "module %s: majors.%s and the manifest's majors.%s must both be numbers", module, item->key, item->key);
            return 0;
        }
        if (item->number > now->number) {
            set_error(loader, "module %s: built against interface %s major %ld, the manifest has %ld", module, item->key, item->number, now->number);
            return 0;
        }
        if (item->number < now->number && !plan_changed(loader, entry, module, item->key, item->number, now->number, plan)) {
            return 0;
        }
    }
    return 1;
}

static int define_adapted_imports(wl_loader* loader, const planned* plan)
{
    const planned* one;
    for (one = plan; one; one = one->next) {
        const import_record* host = find_import(loader, one->interface_name, one->name);
        adapter_env* env;
        char what[160];
        if (one->is_slot) {
            continue;
        }
        env = (adapter_env*)malloc(sizeof *env);
        env->loader = loader;
        env->function = host->function;
        env->user = host->user;
        env->adapter = one->adapter;
        wl_format(what, sizeof what, "adapted import %s.%s", one->interface_name, one->name);
        if (!define_function(loader, one->interface_name, one->name, one->old_signature, adapter_trampoline, env, what)) {
            return 0;
        }
    }
    return 1;
}

static int restore_imports(wl_loader* loader, const planned* plan)
{
    const planned* one;
    int ok = 1;
    for (one = plan; one; one = one->next) {
        if (!one->is_slot && !define_host_import(loader, find_import(loader, one->interface_name, one->name))) {
            ok = 0;
        }
    }
    return ok;
}

static int collect_names(const wl_json* entry, const char* key, char*** names, size_t* count)
{
    const wl_json* list = wl_json_get(entry, key);
    const wl_json* item;
    size_t i = 0;
    *count = wl_json_count(list);
    *names = (char**)calloc(*count + 1, sizeof(char*));
    for (item = list ? list->first : NULL; item; item = item->next) {
        (*names)[i++] = copy_string(item->text);
    }
    return 1;
}

void wl_trap_missing_imports(wl_loader* loader, int enable)
{
    loader->trap_missing = enable;
}

wl_module* wl_load_module_bytes(wl_loader* loader, const char* name, const unsigned char* wasm, size_t size)
{
    wl_api* api = &loader->api;
    const wl_json* entry = loader->manifest ? find_entry(loader, name) : NULL;
    wl_module* module;
    wasmtime_module_t* compiled;
    wasmtime_error_t* error;
    wasm_trap_t* trap = NULL;
    unsigned data_base;
    unsigned table_base;
    size_t i;
    unsigned char* memory;
    wasmtime_extern_t item;
    planned* plan = NULL;
    planned* one;
    int restored;
    if (!entry) {
        set_error(loader, "no module %s in the manifest", name);
        return NULL;
    }
    if (!loader->has_memory) {
        set_error(loader, "module %s: the memory is not created", name);
        return NULL;
    }
    for (module = loader->modules; module; module = module->next) {
        if (!strcmp(module->name, name)) {
            set_error(loader, "module %s is already loaded", name);
            return NULL;
        }
    }
    data_base = (unsigned)wl_json_get(entry, "data_base")->number;
    table_base = (unsigned)wl_json_get(entry, "table_base")->number;
    memory = wl_memory_base(loader);
    if ((size_t)data_base + loader->layout.window > wl_memory_size(loader)) {
        set_error(loader, "module %s: window [%#x, %#x) is outside the memory", name, data_base, data_base + loader->layout.window);
        return NULL;
    }
    if (!plan_adapters(loader, entry, name, &plan)) {
        free_plan(plan);
        return NULL;
    }
    memset(memory + data_base, 0, loader->layout.window);
    error = api->wasmtime_module_new(loader->engine, wasm, size, &compiled);
    if (error) {
        char what[160];
        wl_format(what, sizeof what, "module %s: compile", name);
        fail_wasmtime(loader, what, error, NULL);
        free_plan(plan);
        return NULL;
    }
    if (!define_adapted_imports(loader, plan)) {
        api->wasmtime_module_delete(compiled);
        restore_imports(loader, plan);
        free_plan(plan);
        return NULL;
    }
    if (loader->trap_missing) {
        error = api->wasmtime_linker_define_unknown_imports_as_traps(loader->linker, compiled);
        if (error) {
            api->wasmtime_module_delete(compiled);
            fail_wasmtime(loader, "stubbing the missing imports", error, NULL);
            restore_imports(loader, plan);
            free_plan(plan);
            return NULL;
        }
    }
    module = (wl_module*)calloc(1, sizeof *module);
    module->name = copy_string(name);
    module->entry = entry;
    error = api->wasmtime_linker_instantiate(loader->linker, loader->context, compiled, &module->instance, &trap);
    api->wasmtime_module_delete(compiled);
    restored = restore_imports(loader, plan);
    if (error || trap) {
        char what[160];
        wl_format(what, sizeof what, "module %s: instantiate", name);
        fail_wasmtime(loader, what, error, trap);
        free(module->name);
        free(module);
        free_plan(plan);
        return NULL;
    }
    if (!restored) {
        free(module->name);
        free(module);
        free_plan(plan);
        return NULL;
    }
    collect_names(entry, "exports", &module->slot_names, &module->slot_count);
    collect_names(entry, "data", &module->data_names, &module->data_count);
    module->slots = (wasmtime_func_t*)calloc(module->slot_count + 1, sizeof *module->slots);
    module->data_addresses = (unsigned*)calloc(module->data_count + 1, sizeof(unsigned));
    module->slot_adapters = (wl_adapter**)calloc(module->slot_count + 1, sizeof(wl_adapter*));
    for (one = plan; one; one = one->next) {
        for (i = 0; i < module->slot_count && one->is_slot; i++) {
            if (!strcmp(module->slot_names[i], one->name)) {
                module->slot_adapters[i] = (wl_adapter*)malloc(sizeof(wl_adapter));
                *module->slot_adapters[i] = one->adapter;
            }
        }
    }
    free_plan(plan);
    plan = NULL;
    for (i = 0; i < module->slot_count; i++) {
        wasmtime_val_t entry_value;
        const char* slot = module->slot_names[i];
        if (!api->wasmtime_instance_export_get(loader->context, &module->instance, slot, strlen(slot), &item) || item.kind != WASMTIME_EXTERN_FUNC) {
            set_error(loader, "module %s: no function export %s", name, slot);
            goto bad;
        }
        module->slots[i] = item.of.func;
        if (!module->slot_adapters[i] && table_base + i < loader->layout.table_size && api->wasmtime_table_get(loader->context, &loader->table, table_base + i, &entry_value)
            && entry_value.kind == WASMTIME_FUNCREF && wasmtime_funcref_is_null(&entry_value.of.funcref)) {
            memset(&entry_value, 0, sizeof entry_value);
            entry_value.kind = WASMTIME_FUNCREF;
            entry_value.of.funcref = item.of.func;
            error = api->wasmtime_table_set(loader->context, &loader->table, table_base + i, &entry_value);
            if (error) {
                fail_wasmtime(loader, "filling a table slot", error, NULL);
                goto bad;
            }
            module->table_filled++;
        }
    }
    for (i = 0; i < module->data_count; i++) {
        const char* data = module->data_names[i];
        wasmtime_val_t value;
        if (!api->wasmtime_instance_export_get(loader->context, &module->instance, data, strlen(data), &item) || item.kind != WASMTIME_EXTERN_GLOBAL) {
            set_error(loader, "module %s: no global export %s", name, data);
            goto bad;
        }
        api->wasmtime_global_get(loader->context, &item.of.global, &value);
        if (value.kind != WASMTIME_I32) {
            set_error(loader, "module %s: export %s is not an i32 address", name, data);
            goto bad;
        }
        module->data_addresses[i] = (unsigned)value.of.i32;
    }
    module->next = loader->modules;
    loader->modules = module;
    return module;
bad:
    free(module->name);
    free_names(module->slot_names, module->slot_count);
    free(module->slots);
    free_names(module->data_names, module->data_count);
    free(module->data_addresses);
    free_adapters(module);
    free(module);
    return NULL;
}

wl_module* wl_load_module(wl_loader* loader, const char* name, const char* wasm_path)
{
    FILE* file = fopen(wasm_path, "rb");
    unsigned char* wasm;
    long length;
    wl_module* module;
    if (!file) {
        set_error(loader, "module %s: cannot open %s", name, wasm_path);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);
    wasm = (unsigned char*)malloc((size_t)length + 1);
    if (length < 0 || fread(wasm, 1, (size_t)length, file) != (size_t)length) {
        set_error(loader, "module %s: cannot read %s", name, wasm_path);
        fclose(file);
        free(wasm);
        return NULL;
    }
    fclose(file);
    module = wl_load_module_bytes(loader, name, wasm, (size_t)length);
    free(wasm);
    return module;
}

const char* wl_module_name(const wl_module* module)
{
    return module->name;
}

size_t wl_slot_count(const wl_module* module)
{
    return module->slot_count;
}

const char* wl_slot_name(const wl_module* module, size_t index)
{
    return index < module->slot_count ? module->slot_names[index] : NULL;
}

const wasmtime_func_t* wl_slot(const wl_module* module, size_t index)
{
    return index < module->slot_count ? &module->slots[index] : NULL;
}

size_t wl_data_count(const wl_module* module)
{
    return module->data_count;
}

const char* wl_data_name(const wl_module* module, size_t index)
{
    return index < module->data_count ? module->data_names[index] : NULL;
}

unsigned wl_data_address(const wl_module* module, size_t index)
{
    return index < module->data_count ? module->data_addresses[index] : 0;
}

unsigned wl_table_filled(const wl_module* module)
{
    return module->table_filled;
}

int wl_export_func(wl_loader* loader, const wl_module* module, const char* name, wasmtime_func_t* func)
{
    wasmtime_extern_t item;
    if (!loader->api.wasmtime_instance_export_get(loader->context, &module->instance, name, strlen(name), &item)
        || item.kind != WASMTIME_EXTERN_FUNC) {
        set_error(loader, "module %s: no function export %s", module->name, name);
        return 0;
    }
    *func = item.of.func;
    return 1;
}

int wl_export_memory(wl_loader* loader, const wl_module* module, const char* name, unsigned char** base, size_t* size)
{
    wasmtime_extern_t item;
    if (!loader->api.wasmtime_instance_export_get(loader->context, &module->instance, name, strlen(name), &item)
        || item.kind != WASMTIME_EXTERN_MEMORY) {
        set_error(loader, "module %s: no memory export %s", module->name, name);
        return 0;
    }
    *base = loader->api.wasmtime_memory_data(loader->context, &item.of.memory);
    *size = loader->api.wasmtime_memory_data_size(loader->context, &item.of.memory);
    return 1;
}

int wl_table_entry(wl_loader* loader, unsigned index, wasmtime_val_t* value)
{
    return loader->has_memory && index < loader->layout.table_size
        && loader->api.wasmtime_table_get(loader->context, &loader->table, index, value)
        && value->kind == WASMTIME_FUNCREF && !wasmtime_funcref_is_null(&value->of.funcref);
}

static const wl_adapter* slot_adapter(const wl_module* module, const wasmtime_func_t* func)
{
    size_t i;
    for (i = 0; i < module->slot_count; i++) {
        if (module->slot_adapters[i] && module->slots[i].store_id == func->store_id && module->slots[i].__private == func->__private) {
            return module->slot_adapters[i];
        }
    }
    return NULL;
}

int wl_call(wl_loader* loader, const wl_module* module, const wasmtime_func_t* func, wasmtime_val_raw_t* args_and_results, size_t count)
{
    wasm_trap_t* trap = NULL;
    wasmtime_val_raw_t callee[WL_MAX_VALUES];
    const wl_adapter* adapter = slot_adapter(module, func);
    wasmtime_error_t* error;
    if (adapter) {
        if (count < adapter->caller_room) {
            set_error(loader, "module %s: an adapted slot is called with room for %lu values, its current signature needs %u", module->name,
                      (unsigned long)count, adapter->caller_room);
            return 0;
        }
        adapt_args(adapter, args_and_results, callee);
        error = loader->api.wasmtime_func_call_unchecked(loader->context, func, callee, adapter->callee_room, &trap);
        if (!error && !trap) {
            adapt_result(adapter, callee, args_and_results);
        }
    } else {
        error = loader->api.wasmtime_func_call_unchecked(loader->context, func, args_and_results, count, &trap);
    }
    if (error || trap) {
        char text[400];
        describe(loader, error, trap, text, sizeof text);
        set_error(loader, "module %s: %s", module->name, text);
        if (loader->config.on_trap) {
            loader->config.on_trap(loader->config.trap_user, module->name, text);
        }
        return 0;
    }
    return 1;
}
