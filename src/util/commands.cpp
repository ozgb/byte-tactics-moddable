#include <vector>
#include "commands.h"

class CommandEntry;

class StringRef;

int GetLocalizedString(void*, char*, const char*, int, char*, char*);

struct CommandHandlerInit {
    int fn;
    int mask;
    CommandHandlerInit(int f, int m) : fn(f), mask(m) {}
};

int __cdecl atoi(const char* str);

// FUNCTION: 0x4b75a0 _$E5
// FUNCTION: 0x4b75d0 _$E3
static std::vector<CommandEntry> s_commandTable;

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

extern "C" int __cdecl _strcmpi(const char* str1, const char* str2);

#include "../../link/globals.h"

#include "../../interfaces/services.h"
#include "../state/sim_globals.h"

static inline bool operator==(const StringRef& a, const StringRef& b)
{
    return strcmp(a.data, b.data) == 0;
}

struct CommandNameLess {
    bool operator()(const char* a, const char* b) const
    {
        return _strcmpi(a, b) < 0;
    }
};

struct CommandNameNotEqual {
    bool operator()(const StringRef& a, const StringRef& b) const
    {
        return !(a == b);
    }
};

typedef void (__stdcall *CommandHandlerFn)(void*);

class CommandEntry {
public:
    StringRef handle;
    int value1;
    int value2;

    CommandEntry(const StringRef& h, CommandHandlerInit pp);
    CommandEntry(const CommandEntry& other);
    CommandEntry& operator=(const CommandEntry& other)
    {
        AssignEntry((int*)&other);
        return *this;
    }
    void* AssignEntry(int* source);
};

CommandEntry::CommandEntry(const StringRef& h, CommandHandlerInit pp) : handle(h)
{
    *(CommandHandlerInit*)&value1 = pp;
}

#pragma auto_inline(off)

// FUNCTION: 0x4b73b0
CommandArgs* CommandArgs::InitArgs()
{
    count = 0;
    return this;
}

// FUNCTION: 0x4b73c0
char* CommandArgs::GetArg(int index, char* default_val)
{
    if (index < 0 || index >= count) {
        return default_val;
    }
    return args[index];
}

// FUNCTION: 0x4b73e0
int CommandArgs::GetIntArg(int index, int default_val)
{
    if (index < 0 || index >= count) {
        return default_val;
    }
    const char* str = args[index];
    return atoi(str);
}

// FUNCTION: 0x4b7410
float CommandArgs::GetFloatArg(int index, float default_val)
{
    if (index < 0 || index >= count) {
        return default_val;
    }
    char* str = args[index];
    return (float)atof(str);
}

// FUNCTION: 0x4b7440
void CommandArgs::Tokenize(char* text, char* end)
{
    char* p = buffer;

    if (end == 0)
        end = text + strlen(text);

    count = 0;

    while (1) {
        while (text != end && isspace(*text))
            text++;

        if (text == end)
            return;

        if (*text == '#')
            return;

        if (count < 0x14) {
            args[count] = p;
            count++;
        }

        while (text != end) {
            if (isspace(*text))
                break;
            if (*text == '#')
                break;
            if (p >= &buffer[0x7e])
                break;
            *p = *text;
            p++;
            text++;
        }

        *p = 0;
        p++;
    }
}

// FUNCTION: 0x4b7540
void CommandArgs::ShiftArgs(int n)
{
    if (count >= n) {
        count = 0;
        return;
    }
    char** end = &args[count];
    char** p = &args[n];
    char** dst = args;
    while (p != end)
        *dst++ = *p++;
    count -= n;
}

#pragma auto_inline(off)
// FUNCTION: 0x4b74f0
void CommandArgs::SubstituteArgs(CommandArgs* other)
{
    for (int i = 0; i < count; i++) {
        if (*args[i] == '%') {
            int n = atoi(args[i] + 1);
            if (n >= 0 && n < other->count) {
                args[i] = other->GetArgPtr(n, 0);
            }
        }
    }
}

#pragma auto_inline(on)

// FUNCTION: 0x4b7590
void ClearDefaultCommandHandler()
{
    g_defaultCommandHandler = 0;
    g_defaultCommandMask = 0;
}

// FUNCTION: 0x4b7620
void __stdcall RegisterCommand(const char* name, CommandHandlerFn fn, int mask)
{
    CommandHandlerInit p((int)fn, mask);
    StringRef key(name);
    CommandEntry* first = s_commandTable.begin();
    CommandEntry* last = s_commandTable.end();
    CommandNameLess less;
    const char* k = key.data;
    while (first != last) {
        CommandEntry* mid = first + (last - first) / 2;
        if (less(mid->handle.data, k))
            first = mid + 1;
        else
            last = mid;
    }
    int* slot;
    if (first == s_commandTable.end() || CommandNameNotEqual()(first->handle, key)) {
        CommandEntry e(key, CommandHandlerInit(0, 0));
        int index = first - s_commandTable.begin();
        s_commandTable.insert(first, e);
        slot = &(s_commandTable.begin() + index)->value1;
    } else {
        slot = &first->value1;
    }
    *(CommandHandlerInit*)slot = p;
}

// FUNCTION: 0x4b7760
void __stdcall RegisterCommands(ConsoleCommand* rec)
{
    for (; rec->name; rec++) {
        int mask = rec->mask;
        CommandHandlerFn fn = (CommandHandlerFn)rec->run;
        StringRef key(rec->name);
        CommandEntry* first = s_commandTable.begin();
        CommandEntry* last = s_commandTable.end();
        CommandNameLess less;
        const char* k = key.data;
        while (first != last) {
            CommandEntry* mid = first + (last - first) / 2;
            if (less(mid->handle.data, k))
                first = mid + 1;
            else
                last = mid;
        }
        int* slot;
        if (first == s_commandTable.end() || CommandNameNotEqual()(first->handle, key)) {
            CommandEntry e(key, CommandHandlerInit(0, 0));
            int index = first - s_commandTable.begin();
            s_commandTable.insert(first, e);
            slot = &(s_commandTable.begin() + index)->value1;
        } else {
            slot = &first->value1;
        }
        slot[0] = (int)fn;
        slot[1] = mask;
    }
}

// FUNCTION: 0x4b78e0
void __stdcall SetDefaultCommandHandler(int callback, int mask)
{
    g_defaultCommandHandler = callback;
    g_defaultCommandMask = mask;
}

struct CommandHandlerSlot {
    CommandHandlerFn fn;
    int mask;
};

static inline CommandHandlerSlot* FindCommandSlot(const char* key)
{
    CommandNameLess less;
    CommandEntry* first = s_commandTable.begin();
    CommandEntry* last = s_commandTable.end();
    while (first != last) {
        CommandEntry* mid = first + (last - first) / 2;
        if (less(mid->handle.data, key))
            first = mid + 1;
        else
            last = mid;
    }
    if (first == s_commandTable.end() || less(key, first->handle.data))
        return 0;
    return (CommandHandlerSlot*)&first->value1;
}

#pragma auto_inline(off)
// FUNCTION: 0x4b7900
int __stdcall ExecuteCommand(CommandArgs* obj, int mask)
{
    int result = 0;
    if (obj->count >= 1) {
        StringRef key(obj->GetArg(0, DAT_005119b8));
        key.MakeLower();
        CommandHandlerSlot* h = FindCommandSlot(key.data);
        if (h != 0 && (h->mask & mask)) {
            result = h->mask;
            h->fn(obj);
        } else if (g_defaultCommandHandler != 0 && (g_defaultCommandMask & mask)) {
            result = g_defaultCommandMask;
            ((void (__stdcall *)(void*))g_defaultCommandHandler)(obj);
        }
    }
    return result;
}

#pragma auto_inline(on)

// FUNCTION: 0x4b7a10
int __stdcall FindLineEnd(char* text, int length)
{
    for (int i = 0; i < length; i++) {
        if (text[i] == '\n') {
            return i;
        }
    }
    return length;
}

// FUNCTION: 0x4b7a30
int __stdcall ExecuteCommandText(char* text, int len, CommandArgs* vars, int mask)
{
    int result = 0;
    CommandArgs cmd;
    cmd.InitArgs();
    while (len > 0) {
        int n = FindLineEnd(text, len);
        if (n > len)
            break;
        char* end = text + n;
        cmd.Tokenize(text, end);
        cmd.SubstituteArgs(vars);
        result |= ExecuteCommand(&cmd, mask);
        text = end + 1;
        len -= n + 1;
    }
    return result;
}

// FUNCTION: 0x4b7ac0
void NopRet(void)
{
}

// FUNCTION: 0x4b7ad0
void ClearCommandTable()
{
    s_commandTable.clear();
}

typedef std::vector<CommandEntry> CommandVector;
typedef void (CommandVector::*CommandVectorInsertFn)(
    CommandVector::iterator, CommandVector::size_type, const CommandEntry&);

// FUNCTION: 0x4b7b00 ?insert@?$vector@VCommandEntry@@V?$allocator@VCommandEntry@@@std@@@std@@QAEXPAVCommandEntry@@IABV3@@Z
CommandVectorInsertFn g_insert_004b7b00 = &CommandVector::insert;

// FUNCTION: 0x4b7e00
void* CommandEntry::AssignEntry(int* source)
{
    handle.Assign((StringRef*)source);
    value1 = source[1];
    value2 = source[2];
    return this;
}

// FUNCTION: 0x4b7e30 ??0CommandEntry@@QAE@ABV0@@Z
CommandEntry::CommandEntry(const CommandEntry& other)
    : handle(other.handle), value1(other.value1), value2(other.value2)
{
}
