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
    wl_module* next;
};

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
    wl_module* modules;
};

typedef struct {
    wl_loader* loader;
    wl_import_fn function;
    void* user;
} import_env;

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
    wasmtime_error_t* error = api->wasmtime_config_target_set(config, "pulley32");
    if (error) {
        return fail_wasmtime(loader, "selecting pulley32", error, NULL);
    }
    api->wasmtime_config_cranelift_nan_canonicalization_set(config, 1);
    api->wasmtime_config_consume_fuel_set(config, 0);
    api->wasmtime_config_wasm_relaxed_simd_deterministic_set(config, 1);
    loader->engine = api->wasm_engine_new_with_config(config);
    loader->store = api->wasmtime_store_new(loader->engine, NULL, NULL);
    loader->context = api->wasmtime_store_context(loader->store);
    loader->linker = api->wasmtime_linker_new(loader->engine);
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

static void free_names(char** names, size_t count)
{
    size_t i;
    for (i = 0; i < count; i++) {
        free(names[i]);
    }
    free(names);
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
        free(module);
        module = next;
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

int wl_define_import(wl_loader* loader, const char* module, const char* name, const char* signature, wl_import_fn function, void* user)
{
    const char* arrow = strchr(signature, '>');
    wasm_valtype_vec_t params;
    wasm_valtype_vec_t results;
    wasm_functype_t* type;
    wasmtime_error_t* error;
    import_env* env;
    char what[160];
    wl_format(what, sizeof what, "import %s.%s", module, name);
    if (!arrow || strlen(arrow + 1) > 1) {
        set_error(loader, "%s: signature \"%s\" must read params>result with at most one result", what, signature);
        return 0;
    }
    if (!fill_types(loader, &params, signature, (size_t)(arrow - signature), what)
        || !fill_types(loader, &results, arrow + 1, strlen(arrow + 1), what)) {
        return 0;
    }
    type = loader->api.wasm_functype_new(&params, &results);
    env = (import_env*)malloc(sizeof *env);
    env->loader = loader;
    env->function = function;
    env->user = user;
    error = loader->api.wasmtime_linker_define_func_unchecked(loader->linker, module, strlen(module), name, strlen(name), type, trampoline, env, free);
    loader->api.wasm_functype_delete(type);
    if (error) {
        return fail_wasmtime(loader, what, error, NULL);
    }
    return 1;
}

static char* copy_string(const char* text)
{
    size_t n = strlen(text) + 1;
    char* out = (char*)malloc(n);
    memcpy(out, text, n);
    return out;
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
    memset(memory + data_base, 0, loader->layout.window);
    error = api->wasmtime_module_new(loader->engine, wasm, size, &compiled);
    if (error) {
        char what[160];
        wl_format(what, sizeof what, "module %s: compile", name);
        fail_wasmtime(loader, what, error, NULL);
        return NULL;
    }
    if (loader->trap_missing) {
        error = api->wasmtime_linker_define_unknown_imports_as_traps(loader->linker, compiled);
        if (error) {
            api->wasmtime_module_delete(compiled);
            fail_wasmtime(loader, "stubbing the missing imports", error, NULL);
            return NULL;
        }
    }
    module = (wl_module*)calloc(1, sizeof *module);
    module->name = copy_string(name);
    module->entry = entry;
    error = api->wasmtime_linker_instantiate(loader->linker, loader->context, compiled, &module->instance, &trap);
    api->wasmtime_module_delete(compiled);
    if (error || trap) {
        char what[160];
        wl_format(what, sizeof what, "module %s: instantiate", name);
        fail_wasmtime(loader, what, error, trap);
        free(module->name);
        free(module);
        return NULL;
    }
    collect_names(entry, "exports", &module->slot_names, &module->slot_count);
    collect_names(entry, "data", &module->data_names, &module->data_count);
    module->slots = (wasmtime_func_t*)calloc(module->slot_count + 1, sizeof *module->slots);
    module->data_addresses = (unsigned*)calloc(module->data_count + 1, sizeof(unsigned));
    for (i = 0; i < module->slot_count; i++) {
        wasmtime_val_t entry_value;
        const char* slot = module->slot_names[i];
        if (!api->wasmtime_instance_export_get(loader->context, &module->instance, slot, strlen(slot), &item) || item.kind != WASMTIME_EXTERN_FUNC) {
            set_error(loader, "module %s: no function export %s", name, slot);
            goto bad;
        }
        module->slots[i] = item.of.func;
        if (table_base + i < loader->layout.table_size && api->wasmtime_table_get(loader->context, &loader->table, table_base + i, &entry_value)
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

int wl_call(wl_loader* loader, const wl_module* module, const wasmtime_func_t* func, wasmtime_val_raw_t* args_and_results, size_t count)
{
    wasm_trap_t* trap = NULL;
    wasmtime_error_t* error = loader->api.wasmtime_func_call_unchecked(loader->context, func, args_and_results, count, &trap);
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
