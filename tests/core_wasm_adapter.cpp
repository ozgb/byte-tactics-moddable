#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../interfaces/host.h"
#include "wasmloader.h"

namespace {

const int kMaxModules = 16;
const int kMaxArgs = 16;
const size_t kMaxLine = 4096;

struct Loaded {
    char name[64];
    wl_module* module;
    unsigned char* bytes;
    size_t size;
    bool fueled;
};

wl_loader* s_loader;
FILE* s_out;
Loaded s_modules[kMaxModules];
int s_moduleCount;
const char* s_fault = "";

bool Fault(const char* name)
{
    return strcmp(s_fault, name) == 0;
}

void WL_CALL Trap(void*, const char*, const char*)
{
}

const char* WL_CALL Mix(void*, wasmtime_val_raw_t* a)
{
    a[0].i32 = a[0].i32 + 10 * a[1].i32 + 100 * a[2].i32;
    return 0;
}

void Fail(const char* what)
{
    fprintf(stderr, "core_wasm_adapter: %s\n", what);
    exit(2);
}

struct Names {
    char text[2][64];
    int count;
};

void CopyName(char* out, const char* from, size_t length)
{
    if (length > 63)
        length = 63;
    memcpy(out, from, length);
    out[length] = '\0';
}

void NameAfter(Names* names, const char* text, const char* prefix)
{
    size_t length = strlen(prefix);
    if (strncmp(text, prefix, length) != 0)
        return;
    const char* start = text + length;
    const char* end = start;
    while (*end && *end != ':' && *end != ' ')
        end++;
    CopyName(names->text[names->count++], start, (size_t)(end - start));
}

bool Has(const char* text, const char* part)
{
    return strstr(text, part) != 0;
}

const char* ClassOfLoaderError(const char* text, Names* names)
{
    names->count = 0;
    if (strncmp(text, "modules ", 8) == 0) {
        const char* first = text + 8;
        const char* space = strchr(first, ' ');
        if (space) {
            CopyName(names->text[names->count++], first, (size_t)(space - first));
            const char* second = space + 5;
            const char* end = strchr(second, ' ');
            if (end)
                CopyName(names->text[names->count++], second, (size_t)(end - second));
        }
        return "overlap";
    }
    if (strncmp(text, "no module ", 10) == 0) {
        NameAfter(names, text, "no module ");
        return "not_in_manifest";
    }
    const char* module = strstr(text, "module ");
    bool entry = module && (module == text || module[-1] == ' ');
    if (entry && module > text && strncmp(module - 2, ": ", 2) == 0)
        NameAfter(names, module, "module ");
    else if (entry && module == text)
        NameAfter(names, module, "module ");
    if (Has(text, "is already loaded"))
        return "loaded_twice";
    if (Has(text, "the memory is not created"))
        return "no_memory";
    if (Has(text, ": compile failed:"))
        return "compile";
    if (Has(text, ": instantiate failed:"))
        return "instantiate";
    if (Has(text, "no function export") || Has(text, "no global export") || Has(text, "is not an i32 address"))
        return "export";
    if (Has(text, "reaches the block") || Has(text, "reaches the scratch"))
        return "window_reaches";
    if (Has(text, "does not fit the memory or table") || Has(text, "is outside the memory"))
        return "window";
    if (Has(text, "does not fit the memory") || Has(text, "does not fit below the block") || Has(text, "block_max is below"))
        return "layout";
    if (Has(text, "modules must be a non-empty list") || Has(text, "every module needs a name") || Has(text, "is listed twice")
        || Has(text, "must be a list of names"))
        return "module_entry";
    if ((Has(text, "data_base is missing") || Has(text, "table_base is missing")))
        return "module_entry";
    names->count = 0;
    return "manifest";
}

const char* ClassOfTrap(const char* text)
{
    const char* cause = strstr(text, "wasm trap: ");
    if (!cause)
        return Has(text, "wasm backtrace") ? "stack" : 0;
    if (Has(cause, "call stack exhausted"))
        return "stack";
    if (Has(cause, "`unreachable`"))
        return "unreachable";
    if (Has(cause, "out of bounds memory access"))
        return "memory_access";
    if (Has(cause, "divide by zero") || Has(cause, "integer overflow") || Has(cause, "invalid conversion to integer"))
        return "divide";
    if (Has(cause, "uninitialized element") || Has(cause, "indirect call type mismatch") || Has(cause, "table access"))
        return "table";
    return 0;
}

void Line(const char* call, const char* json)
{
    fprintf(s_out, "%s\t%s\n", call, json);
    fflush(s_out);
}

void ErrorLine(const char* call, const char* klass, const Names& names)
{
    fprintf(s_out, "%s\t{\"error\": \"%s\", \"modules\": [", call, klass);
    for (int i = 0; i < names.count; i++)
        fprintf(s_out, "%s\"%s\"", i ? ", " : "", names.text[i]);
    fprintf(s_out, "]}\n");
    fflush(s_out);
    fclose(s_out);
    exit(0);
}

void LoaderErrorLine(const char* call)
{
    Names names;
    const char* klass = ClassOfLoaderError(wl_error(s_loader), &names);
    if (Fault("loader_class") && strcmp(klass, "overlap") == 0)
        klass = "window";
    ErrorLine(call, klass, names);
}

Loaded* Find(const char* name)
{
    for (int i = 0; i < s_moduleCount; i++)
        if (strcmp(s_modules[i].name, name) == 0)
            return &s_modules[i];
    return 0;
}

unsigned char* ReadAll(const char* path, size_t* size)
{
    FILE* file = fopen(path, "rb");
    if (!file)
        return 0;
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    unsigned char* bytes = (unsigned char*)malloc((size_t)length + 1);
    if (length < 0 || fread(bytes, 1, (size_t)length, file) != (size_t)length) {
        fclose(file);
        free(bytes);
        return 0;
    }
    fclose(file);
    *size = (size_t)length;
    return bytes;
}

struct Reader {
    const unsigned char* p;
    const unsigned char* end;
    bool bad;

    unsigned Leb()
    {
        unsigned value = 0;
        int shift = 0;
        for (;;) {
            if (p >= end) {
                bad = true;
                return 0;
            }
            unsigned char b = *p++;
            if (shift < 32)
                value |= (unsigned)(b & 0x7f) << shift;
            shift += 7;
            if (!(b & 0x80))
                return value;
        }
    }

    unsigned char Byte()
    {
        if (p >= end) {
            bad = true;
            return 0;
        }
        return *p++;
    }

    void Skip(unsigned n)
    {
        if ((size_t)(end - p) < n) {
            bad = true;
            p = end;
        } else {
            p += n;
        }
    }
};

void SkipLimits(Reader* r)
{
    unsigned flags = r->Leb();
    r->Leb();
    if (flags & 1)
        r->Leb();
}

bool Signature(const Loaded& m, const char* name, unsigned* params, unsigned* results)
{
    if (!m.bytes || m.size < 8)
        return false;
    Reader top = {m.bytes + 8, m.bytes + m.size, false};
    unsigned typeParams[256], typeResults[256];
    unsigned typeCount = 0;
    unsigned funcTypes[1024];
    unsigned funcCount = 0;
    unsigned exportIndex = 0xffffffffu;
    bool found = false;
    while (!top.bad && top.p < top.end) {
        unsigned id = top.Byte();
        unsigned size = top.Leb();
        if (top.bad || (size_t)(top.end - top.p) < size)
            return false;
        Reader s = {top.p, top.p + size, false};
        top.p += size;
        if (id == 1) {
            unsigned n = s.Leb();
            for (unsigned i = 0; i < n && !s.bad && typeCount < 256; i++) {
                s.Byte();
                typeParams[typeCount] = s.Leb();
                s.Skip(typeParams[typeCount]);
                typeResults[typeCount] = s.Leb();
                s.Skip(typeResults[typeCount]);
                typeCount++;
            }
        } else if (id == 2) {
            unsigned n = s.Leb();
            for (unsigned i = 0; i < n && !s.bad; i++) {
                s.Skip(s.Leb());
                s.Skip(s.Leb());
                unsigned kind = s.Byte();
                if (kind == 0) {
                    unsigned type = s.Leb();
                    if (funcCount < 1024)
                        funcTypes[funcCount++] = type;
                } else if (kind == 1) {
                    s.Byte();
                    SkipLimits(&s);
                } else if (kind == 2) {
                    SkipLimits(&s);
                } else {
                    s.Byte();
                    s.Byte();
                }
            }
        } else if (id == 3) {
            unsigned n = s.Leb();
            for (unsigned i = 0; i < n && !s.bad && funcCount < 1024; i++)
                funcTypes[funcCount++] = s.Leb();
        } else if (id == 7) {
            unsigned n = s.Leb();
            for (unsigned i = 0; i < n && !s.bad; i++) {
                unsigned length = s.Leb();
                const unsigned char* text = s.p;
                s.Skip(length);
                unsigned kind = s.Byte();
                unsigned index = s.Leb();
                if (!s.bad && kind == 0 && length == strlen(name) && memcmp(text, name, length) == 0) {
                    exportIndex = index;
                    found = true;
                }
            }
        }
    }
    if (!found || exportIndex >= funcCount || funcTypes[exportIndex] >= typeCount)
        return false;
    *params = typeParams[funcTypes[exportIndex]];
    *results = typeResults[funcTypes[exportIndex]];
    return true;
}

bool Invoke(Loaded* m, const char* name, wasmtime_val_raw_t* raw)
{
    wasmtime_func_t func;
    if (!wl_export_func(s_loader, m->module, name, &func))
        return false;
    return wl_call(s_loader, m->module, &func, raw, kMaxArgs) != 0;
}

bool HasExport(Loaded* m, const char* name)
{
    wasmtime_func_t func;
    return wl_export_func(s_loader, m->module, name, &func) != 0;
}

long long Fuel(Loaded* m)
{
    wasmtime_val_raw_t raw[kMaxArgs];
    memset(raw, 0, sizeof raw);
    if (!Invoke(m, "__fuel_get", raw))
        Fail("fuel: no __fuel_get");
    return raw[0].i64;
}

void SetFuel(Loaded* m)
{
    long budget = 0;
    if (!wl_module_int(s_loader, m->name, "fuel_budget", &budget) || !HasExport(m, "__fuel_set"))
        return;
    wasmtime_val_raw_t raw[kMaxArgs];
    memset(raw, 0, sizeof raw);
    raw[0].i64 = (long long)budget + (Fault("fuel_budget") ? 1 : 0);
    if (!Invoke(m, "__fuel_set", raw))
        Fail("__fuel_set trapped");
}

void DoCall(int argc, char** argv)
{
    if (argc < 3)
        Fail("call: name and export");
    Loaded* m = Find(argv[1]);
    if (!m)
        Fail("call: module is not loaded");
    if (!m->fueled) {
        SetFuel(m);
        m->fueled = true;
    }
    wasmtime_val_raw_t raw[kMaxArgs];
    memset(raw, 0, sizeof raw);
    for (int i = 3; i < argc && i - 3 < kMaxArgs; i++)
        raw[i - 3].i32 = (int)strtol(argv[i], 0, 10);
    unsigned params = 0, results = 1;
    Signature(*m, argv[2], &params, &results);
    wasmtime_func_t func;
    if (wl_export_func(s_loader, m->module, argv[2], &func) && wl_slot_result(m->module, &func) >= 0)
        results = (unsigned)wl_slot_result(m->module, &func);
    if (Invoke(m, argv[2], raw)) {
        char json[64];
        if (results)
            sprintf(json, "{\"result\": %d}", raw[0].i32);
        else
            strcpy(json, "{\"result\": null}");
        Line("call", json);
        return;
    }
    const char* text = wl_error(s_loader);
    const char* klass = ClassOfTrap(text);
    Names names;
    names.count = 0;
    CopyName(names.text[names.count++], m->name, strlen(m->name));
    if (!klass) {
        Names none;
        klass = ClassOfLoaderError(text, &none);
    }
    if (HasExport(m, "__fuel_get") && Fuel(m) < 0)
        klass = "out_of_fuel";
    ErrorLine("call", klass, names);
}

void DoLoad(int argc, char** argv)
{
    if (argc < 3)
        Fail("load: name and module");
    if (s_moduleCount >= kMaxModules)
        Fail("load: too many modules");
    Loaded* m = &s_modules[s_moduleCount];
    memset(m, 0, sizeof *m);
    CopyName(m->name, argv[1], strlen(argv[1]));
    m->bytes = ReadAll(argv[2], &m->size);
    m->module = wl_load_module(s_loader, argv[1], argv[2]);
    if (!m->module)
        LoaderErrorLine("load");
    s_moduleCount++;
    Line("load", "null");
}

void Run(const char* callsPath)
{
    FILE* calls = fopen(callsPath, "rb");
    if (!calls)
        Fail("cannot open the calls file");
    char* line = (char*)malloc(kMaxLine);
    while (fgets(line, (int)kMaxLine, calls)) {
        size_t length = strlen(line);
        while (length && (line[length - 1] == '\n' || line[length - 1] == '\r'))
            line[--length] = '\0';
        if (!length)
            continue;
        char* argv[kMaxArgs + 4];
        int argc = 0;
        char* cursor = line;
        for (;;) {
            argv[argc++] = cursor;
            char* tab = strchr(cursor, '\t');
            if (!tab || argc == kMaxArgs + 4)
                break;
            *tab = '\0';
            cursor = tab + 1;
        }
        const char* call = argv[0];
        if (strcmp(call, "manifest") == 0) {
            if (argc < 2)
                Fail("manifest: path");
            if (!wl_load_manifest_file(s_loader, argv[1]))
                LoaderErrorLine("manifest");
            Line("manifest", "null");
        } else if (strcmp(call, "memory") == 0) {
            if (!wl_create_memory(s_loader) || !wl_define_import(s_loader, "fixture", "mix", "iii>i", Mix, 0))
                LoaderErrorLine("memory");
            Line("memory", "null");
        } else if (strcmp(call, "load") == 0) {
            DoLoad(argc, argv);
        } else if (strcmp(call, "tick") == 0) {
            for (int i = 0; i < s_moduleCount; i++)
                s_modules[i].fueled = false;
            Line("tick", "null");
        } else if (strcmp(call, "call") == 0) {
            DoCall(argc, argv);
        } else if (strcmp(call, "fuel") == 0) {
            if (argc < 2 || !Find(argv[1]))
                Fail("fuel: module is not loaded");
            char json[32];
            sprintf(json, "%I64d", Fuel(Find(argv[1])));
            Line("fuel", json);
        } else if (strcmp(call, "word") == 0) {
            if (argc < 2)
                Fail("word: address");
            unsigned address = (unsigned)strtoul(argv[1], 0, 10);
            if ((size_t)address + 4 > wl_memory_size(s_loader)) {
                Names none;
                none.count = 0;
                ErrorLine("word", "memory_access", none);
            }
            int value;
            memcpy(&value, wl_memory_base(s_loader) + address, 4);
            char json[32];
            sprintf(json, "%d", value);
            Line("word", json);
        } else {
            Fail("unknown call");
        }
    }
    free(line);
    fclose(calls);
}

}

int main(int argc, char** argv)
{
    if (argc != 4 || (strcmp(argv[1], "loader") != 0 && strcmp(argv[1], "fuel") != 0)) {
        fprintf(stderr, "usage: core_wasm_adapter loader|fuel CALLS OUT\n");
        return 2;
    }
    const char* fault = getenv("BT_CORE_FAULT");
    if (fault)
        s_fault = fault;
    s_out = fopen(argv[3], "wb");
    if (!s_out)
        Fail("cannot open the output file");
    wl_config config;
    memset(&config, 0, sizeof config);
    config.dll_path = "wasmtime_host.dll";
    config.open = host_lib_open;
    config.symbol = host_lib_symbol;
    config.close = host_lib_close;
    config.on_trap = Trap;
    char error[512];
    s_loader = wl_create(&config, error, sizeof error);
    if (!s_loader) {
        fprintf(stderr, "%s\n", error);
        return 2;
    }
    Run(argv[2]);
    fclose(s_out);
    return 0;
}
