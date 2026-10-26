#include <string>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <stddef.h>
#include <new>

#include "../../link/globals.h"
#include "string_ref.h"
#include "tdf.h"
#include "../state/block.h"
#include "../state/sim_globals.h"
#include "../quirks/quirks.h"
#include "../files/file_handle.h"
#include "../files/files_api.h"

#ifdef __clang__
__attribute__((optnone)) void __stdcall EmitTdfFieldDestructor(TdfField* field)
{
    field->~TdfField();
}
#endif

namespace std {
inline void _Destroy(TdfField* p)
{
    p->~TdfField();
}
}

#include "../graphics/object_pool.h"
#include "../debug/debug_lib.h"
#include "math.h"
#include "../graphics/display.h"

static inline bool operator==(const StringRef& a, const StringRef& b)
{
    return strcmp(a.ptr, b.ptr) == 0;
}

static inline bool Ne(const StringRef& a, const StringRef& b)
{
    return !(a == b);
}

static inline bool Less(const char* a, const char* b)
{
    return _strcmpi(a, b) < 0;
}

static inline TdfField MakePair(const StringRef& a, const StringRef& b)
{
    return TdfField(a, b);
}

typedef char TdfRecordListSize[sizeof(std::vector<TdfRecord*>) == sizeof(TdfRecordList) ? 1 : -1];
typedef char TdfFieldListSize[sizeof(std::vector<TdfField>) == sizeof(TdfFieldList) ? 1 : -1];

static inline std::vector<TdfRecord*>& Kids(TdfRecord* r)
{
    return *(std::vector<TdfRecord*>*)&r->children;
}

static inline std::vector<TdfField>& Rows(TdfRecord* r)
{
    return *(std::vector<TdfField>*)&r->entries.v;
}

static inline TdfField* EntriesLowerBound(TdfRecord* r, const char* key)
{
    TdfField* first = Rows(r).begin();
    TdfField* last = Rows(r).end();
    while (first != last) {
        TdfField* mid = first + (last - first) / 2;
        if (Less(mid->key.ptr, key))
            first = mid + 1;
        else
            last = mid;
    }
    return first;
}

static inline StringRef* EntriesInsertNew(TdfRecord* r, TdfField* e, const StringRef& key)
{
    return &Rows(r).insert(e, MakePair(key, StringRef()))->value;
}

inline TdfRecord* TdfRecord::FindChild(char* key)
{
    for (TdfRecord** p = Kids(this).begin(); p < Kids(this).end(); p++) {
        if (_strcmpi((*p)->name, key) == 0)
            return *p;
    }
    return 0;
}

inline TdfRecord* TdfRecord::GetChild(int index)
{
    return index >= Kids(this).size() ? 0 : Kids(this).at(index);
}

// FUNCTION: 0x4c42a0
TdfRecord::~TdfRecord()
{
    if (name)
        GameFreeThunk(name);
    for (std::vector<TdfRecord*>::iterator p = Kids(this).begin(); p < Kids(this).end(); p++)
        delete *p;
    Rows(this).~vector();
    Kids(this).~vector();
}

static inline char* SkipComment(char* p)
{
    if (p[0] == '/' && p[1] == '/') {
        while (1) {
            if (*p == '\n')
                break;
            *p = ' ';
            p++;
            if (*p == 0)
                break;
        }
    } else if (p[0] == '/' && p[1] == '*') {
        p[1] = ' ';
        p[0] = ' ';
        p += 2;
        while (*p) {
            if (p[-1] == '*' && p[0] == '/') {
                p[0] = ' ';
                p[-1] = ' ';
                p++;
                break;
            }
            p[-1] = ' ';
            p++;
        }
    } else {
        p++;
    }
    return p;
}

// FUNCTION: 0x4c2ea0
TdfFile::TdfFile()
{
    root = 0;
    current = 0;
    flags = 0;
}

// FUNCTION: 0x4c32f0 ??_GTdfRecord@@QAEPAXI@Z
// FUNCTION: 0x4c3240
void TdfFile::Unload()
{
    if (root)
        delete root;
    root = 0;
    current = 0;
    flags = 0;
}

// FUNCTION: 0x4c33a0
void TdfFile::StripComments(char* p)
{
    while (*p)
        p = SkipComment(p);
}

// FUNCTION: 0x4c3410
int TdfFile::SelectRecord(char* name)
{
    TdfRecord* node = current;
    if (node == 0)
        node = root;
    current = node->FindChild(name);
    return current != 0;
}

// FUNCTION: 0x4c3490
int TdfFile::SelectRecordAt(int index)
{
    TdfRecord* n = current ? current : root;
    TdfRecord* node = n->GetChild(index);
    current = node;
    return node != 0;
}

// FUNCTION: 0x4c35c0 ??0logic_error@std@@QAE@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z
// FUNCTION: 0x4c3730 ?what@logic_error@std@@UBEPBDXZ
// FUNCTION: 0x4c3740 ?_Doraise@logic_error@std@@MBEXXZ
// FUNCTION: 0x4c38f0 ??_Glogic_error@std@@UAEPAXI@Z
// FUNCTION: 0x4c3950 ??0logic_error@std@@QAE@ABV01@@Z
// FUNCTION: 0x4c3af0 ?_Doraise@out_of_range@std@@MBEXXZ
// FUNCTION: 0x4c3c60 ??_Gout_of_range@std@@UAEPAXI@Z
// FUNCTION: 0x4c3cc0 ??0out_of_range@std@@QAE@ABV01@@Z

// FUNCTION: 0x4c3e10
void TdfFile::ResetCurrentRecord()
{
    current = 0;
}

// FUNCTION: 0x4c3e20
int TdfFile::GetCurrentRecord()
{
    return (int)current;
}

// FUNCTION: 0x4c3e30
void TdfFile::SetCurrentRecord(int val)
{
    current = (TdfRecord*)val;
}

static inline char* SkipSpace(char* p)
{
    while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'))
        p++;
    return p;
}

// FUNCTION: 0x4c3e40
TdfRecord::TdfRecord(char* name, char* text, char** nextblock, char* filename)
{
    children.first = 0;
    children.last = 0;
    children.end = 0;
    entries.v.first = 0;
    entries.v.last = 0;
    entries.v.end = 0;
    char error[2000] = "Parse error in .TDF File! ";
    this->name = GameStrdup(name);
    char* current = text;
    for (;;) {
        current = SkipSpace(current);
        switch (*current) {
        case '[': {
            char* close = strchr(current, ']');
            if (!close) {
                strcat(error, "Sub-record - closing ']' not found");
                goto fail;
            }
            StringRef subname = MakeTrimmedString(current + 1, close);
            current = SkipSpace(close + 1);
            if (*current != '{') {
                strcat(error, "Sub-record - opening '{' not found");
                goto fail;
            }
            Kids(this).push_back(new TdfRecord(subname.ptr, current + 1, &current, filename));
            break;
        }
        case '}': {
            if (nextblock)
                *nextblock = current + 1;
            char* end = current - 1;
            if (text <= end)
                this->text = (char *)ComputeChecksum((unsigned char *)text, end - text - 1);
            else
                this->text = 0;
            return;
        }
        case 0: {
            if (nextblock) {
                strcat(error, "End of file - nextblock not zero");
                goto fail;
            }
            char* end = current - 1;
            if (text <= end)
                this->text = (char *)ComputeChecksum((unsigned char *)text, end - text - 1);
            else
                this->text = 0;
            return;
        }
        default: {
            char* eq = strchr(current, '=');
            if (!eq) {
                strcat(error, "Data field - '=' not found");
                goto fail;
            }
            StringRef key = MakeTrimmedString(current, eq);
            current = eq + 1;
            char* semi = strchr(eq + 1, ';');
            if (!semi) {
                strcat(error, "Data field - ';' not found");
                goto fail;
            }
            StringRef value = MakeTrimmedString(current, semi);
            current = semi + 1;
            TdfField* e = EntriesLowerBound(this, key.ptr);
            StringRef* r;
            if (e == Rows(this).end() || Ne(e->key, key))
                r = EntriesInsertNew(this, e, key);
            else
                r = &e->value;
            *r = value;
            break;
        }
        }
    }
fail:
    if (name)
        sprintf(error + strlen(error), " - name = '%s' from file %s", name, filename);
    FatalError(error);
}

// FUNCTION: 0x4c4340
StringRef TdfRecord::MakeTrimmedString(char* start, char* end)
{
    char* p = start;
    while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'))
        p++;
    start = p;
    if (start >= end) {
        StringRef empty(DAT_005119b8);
        return empty;
    }
    char* last = end - 1;
    while (*last == ' ' || *last == '\t' || *last == '\r' || *last == '\n')
        last--;
    char saved = last[1];
    last[1] = 0;
    StringRef tmp(start);
    last[1] = saved;
    return tmp;
}

// FUNCTION: 0x4c43f0
void TdfRecord::ComputeRecordChecksum(char* start, char* end)
{
    if (start <= end) {
        text = (char *)ComputeChecksum((unsigned char *)start, end - start - 1);
    } else {
        text = 0;
    }
}

// FUNCTION: 0x4c4420
void TdfRecord::CopyRecordName(char* dest, size_t count)
{
    strncpy(dest, name, count);
}

// FUNCTION: 0x4c4440
int TdfRecord::GetRecordName()
{
    return (int)name;
}

class Class_004c90b0;
class TranslationTable;
int RIReport(int, int, int, int, int, int, int, int, int, int);
int WalkFrameChain(int, int, int, int, int, int, int, int, int, int);
void RegisterUnitOrders();
void FlipScreen();
void ShowSoftwareCursor();

// FUNCTION: 0x4c4450
int TdfRecord::GetSubRecordCount()
{
    return Kids(this).size();
}

// FUNCTION: 0x4c4470
TdfRecord* TdfRecord::FindSubRecord(const char* name)
{
    for (std::vector<TdfRecord*>::iterator p = Kids(this).begin(); p < Kids(this).end(); p++) {
        if (_strcmpi((*p)->name, name) == 0)
            return *p;
    }
    return 0;
}

// FUNCTION: 0x4c44c0
TdfRecord* TdfRecord::GetSubRecord(int index)
{
    if ((unsigned int)index >= Kids(this).size())
        return 0;
    return Kids(this).at(index);
}

// FUNCTION: 0x4c45c0
int TdfRecord::GetFieldCount()
{
    return Rows(this).size();
}

// FUNCTION: 0x4c45e0
char* TdfRecord::GetFieldName(int index)
{
    if (index < 0 || (unsigned int)index >= Rows(this).size())
        return 0;
    unsigned int i = 0;
    for (std::vector<TdfField>::iterator p = Rows(this).begin(); p < Rows(this).end(); p++, i++) {
        if (i == (unsigned int)index)
            return p->key.ptr;
    }
    return 0;
}

struct NameLess {
    bool operator()(const char* a, const char* b) const
    {
        return _strcmpi(a, b) < 0;
    }
};

static inline TdfField* LowerBound(TdfField* first, TdfField* last, const char* name)
{
    NameLess less;
    while (first != last) {
        TdfField* mid = first + (last - first) / 2;
        if (less(mid->key.ptr, name))
            first = mid + 1;
        else
            last = mid;
    }
    return first;
}

static inline char** Find(TdfRecord* table, const char* name)
{
    NameLess less;
    TdfField* it = LowerBound(Rows(table).begin(), Rows(table).end(), name);
    if (it == Rows(table).end() || less(name, it->key.ptr))
        return 0;
    return &it->value.ptr;
}

static inline char* GetString(TdfRecord* table, const char* name)
{
    char** p = Find(table, name);
    if (p)
        return *p;
    return 0;
}

// FUNCTION: 0x4c4630
int TdfRecord::FindFieldValue(char* name)
{
    char** p = Find(this, name);
    if (p)
        return (int)*p;
    return 0;
}

// FUNCTION: 0x4c46c0
int TdfRecord::GetFieldInt(const char* name, int def)
{
    char* s = GetString(this, name);
    if (s)
        return atoi(s);
    return def;
}

// FUNCTION: 0x4c4760
double TdfRecord::GetFieldDouble(const char* name, double def)
{
    char* s = GetString(this, name);
    if (s)
        return atof(s);
    return def;
}

// FUNCTION: 0x4c4800
int* TdfRecord::GetFieldFixed(int* dst, char* key, int def)
{
    TdfField* lo = Rows(this).begin();
    TdfField* hi = Rows(this).end();
    while (lo != hi) {
        TdfField* mid = lo + (hi - lo) / 2;
        if (Less(mid->key.ptr, key)) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    char** p;
    if (lo == Rows(this).end() || Less(key, lo->key.ptr)) {
        p = 0;
    } else {
        p = &lo->value.ptr;
    }
    char* value = p ? *p : 0;
    if (value) {
        *dst = (int)(atof(value) * 65536.0);
        return dst;
    }
    *dst = def;
    return dst;
}

// FUNCTION: 0x4c48c0
int TdfRecord::GetFieldString(char* dst, char* key, size_t size, char* def)
{
    TdfField* lo = Rows(this).begin();
    TdfField* hi = Rows(this).end();
    while (lo != hi) {
        TdfField* mid = lo + (hi - lo) / 2;
        if (Less(mid->key.ptr, key)) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    char** p;
    if (lo == Rows(this).end() || Less(key, lo->key.ptr)) {
        p = 0;
    } else {
        p = &lo->value.ptr;
    }
    char* value = p ? *p : 0;
    if (value) {
        strncpy(dst, value, size);
        dst[size - 1] = 0;
        return 1;
    }
    if (def) {
        strcpy(dst, def);
    }
    return 0;
}

// FUNCTION: 0x4c49a0 ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV01@@Z
template class std::basic_string<char, std::char_traits<char>, std::allocator<char> >;

// FUNCTION: 0x4c4d70 ?insert@?$vector@PAVTdfRecord@@V?$allocator@PAVTdfRecord@@@std@@@std@@QAEXPAPAVTdfRecord@@IABQAV3@@Z
// FUNCTION: 0x4c51e0 ?insert@?$vector@UTdfField@@V?$allocator@UTdfField@@@std@@@std@@QAEXPAUTdfField@@IABU3@@Z

// FUNCTION: 0x4c5170
void __stdcall ReleasePair(char* param_1)
{
    ((StringRef*)(param_1 + 4))->ReleaseRef();
    ((StringRef*)(param_1))->ReleaseRef();
}

struct HapiBank;

// FUNCTION: 0x4c5190 ??1TdfField@@QAE@XZ
TdfField::~TdfField()
{
}

// FUNCTION: 0x4c5470
void* TdfField::AssignPair(int* param_1)
{
    ((StringRef*)this)->Assign((StringRef*)param_1);
    ((StringRef*)&value)->Assign((StringRef*)(param_1 + 1));
    return this;
}

// FUNCTION: 0x4c54d0 ??0TdfField@@QAE@ABVStringRef@@0@Z
TdfField::TdfField(const StringRef& a, const StringRef& b)
    : key(a), value(b)
{
}

// FUNCTION: 0x4c2f60
int TdfFile::LoadFile(char* path)
{
    FileHandle* file = HAPI_OpenFileRead(path);
    if (!file)
        return 0;
    int size = HAPI_FileLength(file);
    int result = 0;
    if (size > 0) {
        char* buf = (char*)GameAllocIgnoreTag(path, size);
        HAPI_SeekFile(file, 0);
        int got = HAPI_readfromfile(file, buf, size);
        if (got >= 0) {
            int flag = HAPI_IsInArchive(file);
            result = flag;
            if (root)
                delete root;
            root = nullptr;
            current = nullptr;
            flags = result;
            char* text = (char*)GameAllocIgnoreTag("TDF file", size + 1);
            memcpy(text, buf, size);
            text[size] = 0;
            this->StripComments(text);
            TdfRecord* node = (TdfRecord*)operator new(0x29);
            root = node ? new (node) TdfRecord("root", text, nullptr, path) : nullptr;
            GameFreeThunk((int*)text);
            result = 1;
        }
        GameFreeThunk((int*)buf);
    }
    HAPI_CloseFile(file);
    return result;
}

// FUNCTION: 0x4c3120
void TdfFile::LoadBuffer(char* data, int size, int flag, char* path)
{
    delete root;
    root = nullptr;
    current = nullptr;
    flags = flag;
    char* text = (char*)GameAllocIgnoreTag("TDF file", size + 1);
    memcpy(text, data, size);
    text[size] = 0;
    this->StripComments(text);
    TdfRecord* node = (TdfRecord*)SimAlloc(0x29);
    root = node ? new (node) TdfRecord("root", text, nullptr, path) : nullptr;
    GameFreeThunk((int*)text);
}

// FUNCTION: 0x4c2eb0
// FUNCTION: 0x4c51b0 ??_GTdfField@@QAEPAXI@Z
TdfFile::~TdfFile()
{
    delete root;
    root = nullptr;
    current = nullptr;
    flags = 0;
}

class TdfFieldVector : public std::vector<TdfField> {
public:
    int GetPairCount(void);

    iterator Insert(iterator p, const TdfField& x);

    void DestroyAll()
    {
        TdfField* e = _Last;
        TdfField* p = _First;
        while (p != e) {
            p->~TdfField();
            p++;
        }
        ::operator delete(_First);
        _First = nullptr;
        _Last = nullptr;
        _End = nullptr;
    }
};

#pragma pack(push, 1)
class TranslationTable {
public:
    char unknown_0;
    std::vector<TdfField> v;

    TranslationTable(const std::allocator<TdfField>& a) : v(a) {}
    TdfField* FindLowerBound(const char* key);
};
#pragma pack(pop)

static inline StringRef* InsertNew(TranslationTable* s, TdfField* e, const StringRef& key)
{
    return (StringRef*)&((TdfFieldVector*)&s->v)->Insert(e, MakePair(key, StringRef()))->value;
}

// FUNCTION: 0x4c54f0
void __stdcall LoadTranslations(char* filename, char* section)
{
    std::allocator<TdfField> alloc;
    TranslationTable* s;

    if (_strcmpi(section, g_language) == 0)
        return;
    if (g_translations) {
        TranslationTable* old = g_translations;
        ((TdfFieldVector*)&g_translations->v)->DestroyAll();
        ::operator delete(old);
    }
    g_translations = new TranslationTable(alloc);
    AllocNotifyNop((int)g_translations);
    if (g_quirks.boundLanguageCopy) {
        strncpy(g_language, section, 0x3f);
        g_language[0x3f] = 0;
    } else {
        strcpy(g_language, section);
    }
    {
        TdfFile f;
        char value[256];
        char name[256];
        if (f.LoadFile(filename)) {
            int index;
            index = 0;
            while (f.SelectRecordAt(index)) {
                f.current->CopyRecordName(name, 0xff);
                f.current->GetFieldString(value, g_language, 0xff, DAT_005119b8);
                if (strlen(value) != 0) {
                    StringRef key(name);
                    TdfField* e;
                    s = g_translations;
                    e = g_translations->FindLowerBound(key.ptr);
                    StringRef* r;
                    if (e == s->v.end() || Ne(e->key, key)) {
                        r = InsertNew(s, e, key);
                    } else {
                        r = (StringRef*)&e->value;
                    }
                    r->AssignText(value);
                }
                f.ResetCurrentRecord();
                index++;
            }
            f.Unload();
        }
    }
}

// FUNCTION: 0x4c5740
char* __stdcall Translate(char* key)
{
    if (key == nullptr)
        return nullptr;
    TranslationTable* c = g_translations;
    if (c == nullptr)
        return key;

    TdfField* first = c->v.begin();
    TdfField* last = c->v.end();
    TdfField* end = c->v.end();

    while (first != last) {
        TdfField* mid = first + (last - first) / 2;
        bool less = strcmp(mid->key.ptr, key) < 0;
        if (less)
            first = mid + 1;
        else
            last = mid;
    }

    char** found;
    if (first != end) {
        bool before = strcmp(key, first->key.ptr) < 0;
        found = before ? nullptr : &first->value.ptr;
    } else {
        found = nullptr;
    }
    if (found != nullptr)
        return *found;
    return key;
}

// FUNCTION: 0x4c5840
int __stdcall FindTranslation(char* name)
{
    if (name == nullptr || g_translations == nullptr)
        return 0;
    for (std::vector<TdfField>::iterator p = g_translations->v.begin(); p < g_translations->v.end(); p++) {
        if (_strcmpi(p->value.ptr, name) == 0)
            return (int)p->key.ptr;
    }
    return 0;
}

// FUNCTION: 0x4c58a0
int __stdcall GetLocalizedString(TdfFile* file, char* dst, char* key, size_t size,
                           char* def)
{
    char full[256];
    strcpy(full, g_language);
    strcat(full, key);
    int r;
    if (def) {
        r = file->current->GetFieldString(dst, full, size, def);
        if (r) return 1;
        return file->current->GetFieldString(dst, key, size, def);
    }
    r = file->current->GetFieldString(dst, full, size, DAT_005119b8);
    if (r) return 1;
    return file->current->GetFieldString(dst, key, size, DAT_005119b8);
}

// FUNCTION: 0x4c59d0
TdfField* TdfFieldVector::Insert(iterator p, const TdfField& x)
{
    return insert(p, x);
}

#pragma auto_inline(off)
// FUNCTION: 0x4c5ba0
int TdfFieldVector::GetPairCount(void)
{
    return size();
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x4c5c60
TdfField* TranslationTable::FindLowerBound(const char* key)
{
    TdfField* first = v.begin();
    TdfField* last = v.end();

    while (first != last) {
        TdfField* mid = first + (last - first) / 2;
        bool less = strcmp(mid->key.ptr, key) < 0;
        if (less)
            first = mid + 1;
        else
            last = mid;
    }
    return first;
}
#pragma auto_inline(on)

// FUNCTION: 0x4c5cd0
void __stdcall AssignPairRange(TdfField* first, TdfField* last, const TdfField& x)
{
    for (; first != last; ++first)
        *first = x;
}

// FUNCTION: 0x4c5d10
TdfField* __stdcall AssignPairRangeBack(TdfField* first, TdfField* last, TdfField* dest)
{
    while (first != last)
        *--dest = *--last;
    return dest;
}

// FUNCTION: 0x4c5d60
void __stdcall ConstructPair(TdfField* p, const TdfField& value)
{
    new ((void*)p) TdfField(value);
}
