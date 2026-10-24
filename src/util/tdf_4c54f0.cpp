#include <vector>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <new>

#include "../../link/globals.h"

#include "../debug/debug_lib.h"

#include "string_ref.h"
#include "tdf.h"
#include "tdf_4c54f0.h"

int RIReport(int, int, int, int, int, int, int, int, int, int);

static inline bool operator==(const StringRef& a, const StringRef& b)
{
    return strcmp(a.ptr, b.ptr) == 0;
}

static inline bool Ne(const StringRef& a, const StringRef& b)
{
    return !(a == b);
}

namespace std {
inline void _Destroy(TdfField* p)
{
    ((StringRef*)&p->value)->ReleaseRef();
    ((StringRef*)&p->key)->ReleaseRef();
}
}

static inline TdfField MakeElem(const StringRef& a, const StringRef& b)
{
    return TdfField(a, b);
}

class TdfFieldVector : public std::vector<TdfField> {
public:
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
        _First = 0;
        _Last = 0;
        _End = 0;
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
    return (StringRef*)&((TdfFieldVector*)&s->v)->Insert(e, MakeElem(key, StringRef()))->value;
}

#include "../quirks/quirks.h"
#include "../state/sim_globals.h"

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
    if (key == 0)
        return 0;
    TranslationTable* c = g_translations;
    if (c == 0)
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
        found = before ? 0 : &first->value.ptr;
    } else {
        found = 0;
    }
    if (found != 0)
        return *found;
    return key;
}

// FUNCTION: 0x4c5840
int __stdcall FindTranslation(char* name)
{
    if (name == 0 || g_translations == 0)
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

typedef std::vector<TdfField> Vec_004c5b70;
typedef void (Vec_004c5b70::*DestroyFn_004c5b70)(Vec_004c5b70::iterator, Vec_004c5b70::iterator);
typedef Vec_004c5b70::iterator (Vec_004c5b70::*UcopyFn_004c5b70)(
    Vec_004c5b70::const_iterator, Vec_004c5b70::const_iterator, Vec_004c5b70::iterator);
typedef void (Vec_004c5b70::*UfillFn_004c5b70)(
    Vec_004c5b70::iterator, Vec_004c5b70::size_type, const TdfField&);

struct Access_004c5b70 : Vec_004c5b70 {
    static DestroyFn_004c5b70 destroy;
    static UcopyFn_004c5b70 ucopy;
    static UfillFn_004c5b70 ufill;
};

// FUNCTION: 0x4c5b70 ?_Destroy@?$vector@UTdfField@@V?$allocator@UTdfField@@@std@@@std@@IAEXPAUTdfField@@0@Z
DestroyFn_004c5b70 Access_004c5b70::destroy = &Access_004c5b70::_Destroy;

// FUNCTION: 0x4c5bc0 ?_Ucopy@?$vector@UTdfField@@V?$allocator@UTdfField@@@std@@@std@@IAEPAUTdfField@@PBU3@0PAU3@@Z
UcopyFn_004c5b70 Access_004c5b70::ucopy = &Access_004c5b70::_Ucopy;

// FUNCTION: 0x4c5c20 ?_Ufill@?$vector@UTdfField@@V?$allocator@UTdfField@@@std@@@std@@IAEXPAUTdfField@@IABU3@@Z
UfillFn_004c5b70 Access_004c5b70::ufill = &Access_004c5b70::_Ufill;

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
