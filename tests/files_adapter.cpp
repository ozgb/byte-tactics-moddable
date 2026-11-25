#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <vector>

#include "../interfaces/seams.h"
#include "../interfaces/files.h"
#include "../interfaces/host.h"
#include "../link/globals.h"
#include "../src/util/engine_services.h"
#include "../src/files/files_impl.h"
#include "../src/files/file_handle.h"
#include "../src/files/hpi.h"
#include "../src/graphics/display.h"
#include "../src/harness/replay.h"
#include "../src/state/sim_globals.h"
#include "../src/game/cd_check.h"
#include "../src/game/main.h"

Seams g_seams;
SimGlobals* g_simGlobals = 0;
char* g_squashWindow;

static FilesSeam s_files;
static char s_display[sizeof(Display)];
static char s_gameDir[0x200];
static FILE* s_out;
static const char* s_fault = "";

static bool Fault(const char* name)
{
    return strcmp(s_fault, name) == 0;
}

Display* GetDisplay(void)
{
    return (Display*)s_display;
}

ReplayGate& ReplayGateRef()
{
    static ReplayGate gate;
    return gate;
}

SimGlobals& __stdcall SimCreate()
{
    static char storage[sizeof(SimGlobals)];
    return *(SimGlobals*)storage;
}

void* __cdecl GameAllocIgnoreTag(const char*, unsigned int size)
{
    return malloc(size ? size : 1);
}

void* __cdecl GameReallocTagged(void* p, const char*, unsigned int size)
{
    return realloc(p, size ? size : 1);
}

void __cdecl GameFreeThunk(void* p)
{
    free(p);
}

void* __cdecl SimAlloc(unsigned int size)
{
    return malloc(size ? size : 1);
}

char* __cdecl GameStrdup(const char* s)
{
    char* p = (char*)malloc(strlen(s) + 1);
    strcpy(p, s);
    return p;
}

int __stdcall ComputeChecksum(unsigned char* data, int len)
{
    int sum = 0;
    for (int i = 0; i < len; i++)
        sum += data[i];
    return sum;
}

void __cdecl AllocNotifyNop(int) {}

void __stdcall ChdirToExeDirectory()
{
    _chdir(s_gameDir);
}

StringRef::StringRef()
{
    int* block = (int*)malloc(sizeof(int) + 1);
    *block = 1;
    data = (char*)(block + 1);
    *data = 0;
}

StringRef::StringRef(const StringRef& other)
{
    data = other.data;
    ((int*)data)[-1]++;
}

StringRef::StringRef(const char* text)
{
    if (text == 0)
        text = "";
    int* block = (int*)malloc(strlen(text) + 1 + sizeof(int));
    *block = 1;
    data = (char*)(block + 1);
    strcpy(data, text);
}

void StringRef::ReleaseRef()
{
    if (--((int*)data)[-1] == 0)
        free((int*)data - 1);
}

StringRef* StringRef::Assign(const StringRef* other)
{
    ((int*)other->data)[-1]++;
    ReleaseRef();
    data = other->data;
    return this;
}

StringRef* StringRef::AssignText(const char* text)
{
    StringRef copy(text);
    Assign(&copy);
    return this;
}

static void Emit(const char* text)
{
    fputs(text, s_out);
}

void __stdcall FatalError(const char*)
{
    Emit("fatal\tnull\n");
    fclose(s_out);
    exit(0);
}

extern "C" int __stdcall uncompress(unsigned char* dest, unsigned long* destLen, const unsigned char* source, unsigned long sourceLen);
extern "C" int __stdcall compress(unsigned char* dest, unsigned long* destLen, const unsigned char* source, unsigned long sourceLen);

int __stdcall _uncompress(unsigned char* dest, unsigned long* destLen, unsigned char* source, unsigned long sourceLen)
{
    return uncompress(dest, destLen, source, sourceLen);
}

void __stdcall _compress(char* out, int* outSize, char* in, int size)
{
    unsigned long n = (unsigned long)*outSize;
    compress((unsigned char*)out, &n, (const unsigned char*)in, (unsigned long)size);
    *outSize = (int)n;
}

struct ArchiveDirectory;
void __stdcall HAPI_RelocateDirectory(ArchiveDirectory* h, int delta);

void __stdcall HAPI_RelocateDirectory(int name, int base)
{
    HAPI_RelocateDirectory((ArchiveDirectory*)name, base);
}

static void __stdcall ListDirectoryThunk(const char* path, int dirsOnly, FileNameList* list)
{
    ListDirectory(path, dirsOnly, (std::vector<StringRef>*)list);
}

static void __stdcall FindFilesRecursiveThunk(const char* path, const char* pat, FileNameList* tree, int state, int recursive)
{
    FindFilesRecursive(path, pat, (std::vector<StringRef>*)tree, state, recursive);
}

static void InitSeam()
{
    s_files.OpenRead = HAPI_OpenFileRead;
    s_files.Close = HAPI_CloseFile;
    s_files.IsInArchive = HAPI_IsInArchive;
    s_files.Seek = HAPI_SeekFile;
    s_files.Tell = HAPI_TellFile;
    s_files.Read = HAPI_readfromfile;
    s_files.Length = HAPI_FileLength;
    s_files.LengthByName = HAPI_FileLengthByName;
    s_files.ReadAt = HAPI_ReadFileAt;
    s_files.Load = HAPI_LoadFile;
    s_files.MatchWildcard = MatchWildcard;
    s_files.FindFirst = HAPI_FindFirst;
    s_files.FindNext = HAPI_FindNext;
    s_files.FindClose = HAPI_FindClose;
    s_files.CountDirectoryEntries = CountDirectoryEntries;
    s_files.ListDirectory = ListDirectoryThunk;
    s_files.FindFilesRecursive = FindFilesRecursiveThunk;
    s_files.AddArchive = HAPI_AddArchive;
    s_files.CloseArchive = HAPI_CloseArchive;
    s_files.DropMissingArchives = HAPI_DropMissingArchives;
    s_files.ResolveShadowedFiles = HAPI_ResolveShadowedFiles;
    g_seams.files = &s_files;
}

struct Text {
    std::vector<char> buf;
    void Add(const char* s) { buf.insert(buf.end(), s, s + strlen(s)); }
    void Char(char c) { buf.push_back(c); }
    void Int(long long v)
    {
        char tmp[32];
        sprintf(tmp, "%I64d", v);
        Add(tmp);
    }
    void Str(const char* s)
    {
        Char('"');
        for (const unsigned char* p = (const unsigned char*)s; *p; p++) {
            if (*p == '"' || *p == '\\') {
                Char('\\');
                Char((char)*p);
            } else if (*p < 0x20 || *p >= 0x7f) {
                char tmp[8];
                sprintf(tmp, "\\u%04x", *p);
                Add(tmp);
            } else {
                Char((char)*p);
            }
        }
        Char('"');
    }
    void Null() { Add("null"); }
};

static void EmitResult(const char* op, Text& value)
{
    value.Char(0);
    fprintf(s_out, "%s\t%s\n", op, &value.buf[0]);
    fflush(s_out);
}

static unsigned int Rotr(unsigned int x, int n)
{
    return (x >> n) | (x << (32 - n));
}

static void Sha256(const unsigned char* data, unsigned int len, char* hex)
{
    static const unsigned int k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
    unsigned int h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    unsigned int total = (len + 9 + 63) / 64 * 64;
    unsigned char* msg = (unsigned char*)calloc(total, 1);
    memcpy(msg, data, len);
    msg[len] = 0x80;
    unsigned long long bits = (unsigned long long)len * 8;
    for (int i = 0; i < 8; i++)
        msg[total - 1 - i] = (unsigned char)(bits >> (8 * i));
    for (unsigned int off = 0; off < total; off += 64) {
        unsigned int w[64];
        for (int i = 0; i < 16; i++)
            w[i] = (msg[off + 4 * i] << 24) | (msg[off + 4 * i + 1] << 16) | (msg[off + 4 * i + 2] << 8) | msg[off + 4 * i + 3];
        for (int i = 16; i < 64; i++) {
            unsigned int s0 = Rotr(w[i - 15], 7) ^ Rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            unsigned int s1 = Rotr(w[i - 2], 17) ^ Rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        unsigned int a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; i++) {
            unsigned int t1 = hh + (Rotr(e, 6) ^ Rotr(e, 11) ^ Rotr(e, 25)) + ((e & f) ^ (~e & g)) + k[i] + w[i];
            unsigned int t2 = (Rotr(a, 2) ^ Rotr(a, 13) ^ Rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
            hh = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }
        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
        h[5] += f;
        h[6] += g;
        h[7] += hh;
    }
    free(msg);
    for (int i = 0; i < 8; i++)
        sprintf(hex + 8 * i, "%08x", h[i]);
}

static void Digest(Text& t, const void* data, int len)
{
    char hex[65];
    Sha256((const unsigned char*)data, len > 0 ? (unsigned int)len : 0, hex);
    t.Str(hex);
}

static std::vector<FileHandle*> s_handles;

static FileHandle* HandleOf(const char* id)
{
    int i = atoi(id);
    if (i < 1 || i > (int)s_handles.size())
        return 0;
    return s_handles[i - 1];
}

static bool Missing(const char* name)
{
    if (!Fault("case"))
        return false;
    for (const char* p = name; *p; p++) {
        if (*p >= 'A' && *p <= 'Z')
            return true;
    }
    return false;
}

static void SwapFirst(std::vector<std::vector<char> >& items)
{
    if (Fault("swap") && items.size() >= 2) {
        std::vector<char> t = items[0];
        items[0] = items[1];
        items[1] = t;
    }
}

static void Array(Text& t, std::vector<std::vector<char> >& items)
{
    t.Char('[');
    for (unsigned int i = 0; i < items.size(); i++) {
        if (i)
            t.Char(',');
        t.buf.insert(t.buf.end(), items[i].begin(), items[i].end());
    }
    t.Char(']');
}

static std::vector<char> StrItem(const char* s)
{
    Text t;
    t.Str(s);
    return t.buf;
}

static const char* BaseName(const char* path)
{
    const char* slash = strrchr(path, '\\');
    return slash ? slash + 1 : path;
}

static const char* ArchiveName(OPENHAPIFILE* archive)
{
    return BaseName((const char*)archive + OPENHAPIFILE_NAME_OFFSET);
}

static void OpMount(Text& t)
{
    RegisterDataArchives();
    Display* d = GetDisplay();
    if (Fault("budget") && d->count > 0)
        d->count--;
    std::vector<std::vector<char> > items;
    for (int i = 0; i < d->count; i++)
        items.push_back(StrItem(ArchiveName(d->files[i])));
    if (Fault("mountall")) {
        struct _finddata_t fd;
        long h = _findfirst("*.hpi", &fd);
        if (h != -1) {
            do {
                bool seen = false;
                for (int i = 0; i < d->count; i++)
                    seen = seen || _strcmpi(ArchiveName(d->files[i]), fd.name) == 0;
                if (!seen)
                    items.push_back(StrItem(fd.name));
            } while (_findnext(h, &fd) == 0);
            _findclose(h);
        }
    }
    Array(t, items);
    if (Fault("winner")) {
        for (int i = 0, j = d->count - 1; i < j; i++, j--) {
            OPENHAPIFILE* x = d->files[i];
            d->files[i] = d->files[j];
            d->files[j] = x;
        }
    }
}

static void OpFind(Text& t, const char* path, int state, int recursive)
{
    struct _finddata_t fd;
    std::vector<std::vector<char> > items;
    int h = g_seams.files->FindFirst(path, &fd, state, (char)recursive);
    if (h != -1) {
        do {
            Text e;
            e.Char('[');
            e.Str(fd.name);
            e.Char(',');
            e.Int(fd.attrib & 0x11);
            e.Char(',');
            e.Int((long long)(unsigned long)fd.size);
            e.Char(']');
            items.push_back(e.buf);
        } while (g_seams.files->FindNext(h, &fd) == 0);
        g_seams.files->FindClose(h);
    }
    SwapFirst(items);
    Array(t, items);
}

static void Names(Text& t, std::vector<StringRef>& names)
{
    std::vector<std::vector<char> > items;
    for (unsigned int i = 0; i < names.size(); i++)
        items.push_back(StrItem(names[i].ptr));
    SwapFirst(items);
    Array(t, items);
}

static void Double(Text& t, double v)
{
    unsigned int w[2];
    memcpy(w, &v, 8);
    char tmp[32];
    sprintf(tmp, "%08x%08x", w[1], w[0]);
    t.Str(tmp);
}

static void Record(Text& t, TdfRecord* r)
{
    t.Add("{\"name\":");
    t.Str(r->name);
    t.Add(",\"fields\":[");
    int n = r->GetFieldCount();
    for (int i = 0; i < n; i++) {
        int k = Fault("tdforder") ? n - 1 - i : i;
        char* key = r->entries.v.first[k].key.ptr;
        if (i)
            t.Char(',');
        t.Char('[');
        t.Str(key);
        t.Char(',');
        t.Str(r->entries.v.first[k].value.ptr);
        t.Char(',');
        t.Str((char*)r->FindFieldValue(key));
        t.Char(',');
        t.Int(r->GetFieldInt(key, 0));
        t.Char(',');
        int fixed = 0;
        r->GetFieldFixed(&fixed, key, 0);
        t.Int(fixed);
        t.Char(',');
        Double(t, r->GetFieldDouble(key, 0.0));
        t.Char(']');
    }
    t.Add("],\"records\":[");
    int m = r->GetSubRecordCount();
    for (int i = 0; i < m; i++) {
        if (i)
            t.Char(',');
        Record(t, r->GetSubRecord(i));
    }
    t.Add("]}");
}

static int Run(const char* op, char** a, int n)
{
    Text t;
    if (strcmp(op, "mount") == 0) {
        OpMount(t);
    } else if (strcmp(op, "open") == 0 && n >= 1) {
        FileHandle* f = Missing(a[0]) ? 0 : g_seams.files->OpenRead(a[0]);
        if (f) {
            s_handles.push_back(f);
            t.Int((int)s_handles.size());
        } else {
            t.Null();
        }
    } else if (strcmp(op, "close") == 0 && n >= 1) {
        FileHandle* f = HandleOf(a[0]);
        if (!f)
            return 0;
        s_handles[atoi(a[0]) - 1] = 0;
        t.Int(g_seams.files->Close(f));
    } else if (strcmp(op, "inarchive") == 0 && n >= 1) {
        FileHandle* f = HandleOf(a[0]);
        if (!f)
            return 0;
        t.Int(g_seams.files->IsInArchive(f));
    } else if (strcmp(op, "seek") == 0 && n >= 2) {
        FileHandle* f = HandleOf(a[0]);
        if (!f)
            return 0;
        t.Int(g_seams.files->Seek(f, atoi(a[1])));
    } else if (strcmp(op, "tell") == 0 && n >= 1) {
        FileHandle* f = HandleOf(a[0]);
        if (!f)
            return 0;
        t.Int(g_seams.files->Tell(f));
    } else if (strcmp(op, "length") == 0 && n >= 1) {
        FileHandle* f = HandleOf(a[0]);
        if (!f)
            return 0;
        t.Int(g_seams.files->Length(f));
    } else if (strcmp(op, "read") == 0 && n >= 2) {
        FileHandle* f = HandleOf(a[0]);
        int size = atoi(a[1]);
        if (!f || size < 0)
            return 0;
        unsigned char* buf = (unsigned char*)calloc(size + 16, 1);
        int got = g_seams.files->Read(f, buf, size);
        if (Fault("read") && got > 1)
            got--;
        t.Char('[');
        t.Int(got);
        t.Char(',');
        Digest(t, buf, got);
        t.Char(']');
        free(buf);
    } else if (strcmp(op, "lengthbyname") == 0 && n >= 1) {
        t.Int(Missing(a[0]) ? 0 : g_seams.files->LengthByName(a[0]));
    } else if (strcmp(op, "readat") == 0 && n >= 3) {
        int size = atoi(a[2]);
        if (size < 0)
            return 0;
        unsigned char* buf = (unsigned char*)calloc(size + 16, 1);
        void* r = Missing(a[0]) ? 0 : g_seams.files->ReadAt(a[0], buf, atoi(a[1]), (unsigned int)size);
        if (r)
            Digest(t, buf, size);
        else
            t.Null();
        free(buf);
    } else if (strcmp(op, "load") == 0 && n >= 1) {
        int size = 0;
        void* data = Missing(a[0]) ? 0 : g_seams.files->Load(a[0], &size);
        if (Missing(a[0]))
            size = -1;
        t.Char('[');
        t.Int(size);
        t.Char(',');
        if (data) {
            Digest(t, data, size);
            free(data);
        } else {
            t.Null();
        }
        t.Char(']');
    } else if (strcmp(op, "find") == 0 && n >= 3) {
        OpFind(t, a[0], atoi(a[1]), atoi(a[2]));
    } else if (strcmp(op, "count") == 0 && n >= 2) {
        t.Int(g_seams.files->CountDirectoryEntries(a[0], atoi(a[1])));
    } else if (strcmp(op, "list") == 0 && n >= 2) {
        std::vector<StringRef> names;
        g_seams.files->ListDirectory(a[0], atoi(a[1]), (FileNameList*)&names);
        Names(t, names);
    } else if (strcmp(op, "tree") == 0 && n >= 4) {
        std::vector<StringRef> names;
        g_seams.files->FindFilesRecursive(a[0], a[1], (FileNameList*)&names, atoi(a[2]), atoi(a[3]));
        Names(t, names);
    } else if (strcmp(op, "match") == 0 && n >= 2) {
        t.Int(g_seams.files->MatchWildcard(a[0], a[1]));
    } else if (strcmp(op, "tdf") == 0 && n >= 1) {
        TdfFile file;
        if (!Missing(a[0]) && file.LoadFile(a[0]))
            Record(t, file.root);
        else
            t.Null();
        file.Unload();
    } else {
        return 0;
    }
    EmitResult(op, t);
    return 1;
}

static int Usage()
{
    fprintf(stderr, "usage: files_adapter files --game DIR CALLS OUT\n");
    return 2;
}

int main(int argc, char** argv)
{
    if (argc < 6 || strcmp(argv[1], "files") != 0 || strcmp(argv[2], "--game") != 0)
        return Usage();
    if (getenv("BT_FILES_FAULT"))
        s_fault = getenv("BT_FILES_FAULT");
    if (!_fullpath(s_gameDir, argv[3], sizeof(s_gameDir)))
        return Usage();
    FILE* calls = fopen(argv[4], "rb");
    if (!calls)
        return Usage();
    s_out = fopen(argv[5], "wb");
    if (!s_out)
        return Usage();
    InitSeam();
    if (_chdir(s_gameDir) != 0)
        return Usage();
    char line[0x1000];
    while (fgets(line, sizeof(line), calls)) {
        char* args[8];
        int n = 0;
        line[strcspn(line, "\r\n")] = 0;
        char* op = line;
        char* p = strchr(line, '\t');
        while (p && n < 8) {
            *p++ = 0;
            args[n++] = p;
            p = strchr(p, '\t');
        }
        if (!Run(op, args, n)) {
            fprintf(s_out, "%s\terror\n", op);
            fflush(s_out);
        }
    }
    fclose(calls);
    fclose(s_out);
    return 0;
}
