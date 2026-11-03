#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "replay_text.h"

void ReplayText::Free()
{
    free(buffer);
    buffer = nullptr;
    used = 0;
    capacity = 0;
}

void ReplayText::Clear()
{
    used = 0;
    if (buffer != nullptr)
        buffer[0] = '\0';
}

void ReplayText::Reserve(unsigned extra)
{
    if (used + extra + 1 <= capacity)
        return;
    unsigned wanted = capacity == 0 ? 4096 : capacity;
    while (wanted < used + extra + 1)
        wanted *= 2;
    char* grown = (char*)realloc(buffer, wanted);
    if (grown == nullptr)
        abort();
    buffer = grown;
    capacity = wanted;
}

void ReplayText::Add(const char* text)
{
    unsigned length = (unsigned)strlen(text);
    Reserve(length);
    memcpy(buffer + used, text, length);
    used += length;
    buffer[used] = '\0';
}

void ReplayText::AddChar(char c)
{
    Reserve(1);
    buffer[used++] = c;
    buffer[used] = '\0';
}

void ReplayText::AddInt(int value)
{
    char buffer[16];
    sprintf(buffer, "%d", value);
    Add(buffer);
}

void ReplayText::AddUnsigned(unsigned value)
{
    char buffer[16];
    sprintf(buffer, "%u", value);
    Add(buffer);
}

void ReplayText::AddHex32(unsigned value)
{
    char buffer[16];
    sprintf(buffer, "0x%08x", value);
    Add(buffer);
}

void ReplayText::AddF32(unsigned bits)
{
    char buffer[24];
    sprintf(buffer, "f32:0x%08x", bits);
    Add(buffer);
}

void ReplayText::AddF64(unsigned low, unsigned high)
{
    char buffer[40];
    sprintf(buffer, "f64:0x%08x%08x", high, low);
    Add(buffer);
}

void ReplayText::AddString(const char* text)
{
    if (text == nullptr) {
        Add("null");
        return;
    }
    AddChar('"');
    for (const unsigned char* p = (const unsigned char*)text; *p != '\0'; p++) {
        if (*p == '\\' || *p == '"') {
            AddChar('\\');
            AddChar((char)*p);
        } else if (*p == '\n') {
            Add("\\n");
        } else if (*p == '\r') {
            Add("\\r");
        } else if (*p == '\t') {
            Add("\\t");
        } else if (*p < 0x20 || *p >= 0x7f) {
            char buffer[8];
            sprintf(buffer, "\\x%02x", *p);
            Add(buffer);
        } else {
            AddChar((char)*p);
        }
    }
    AddChar('"');
}

void ReplayText::AddText(const ReplayText& other)
{
    if (other.used == 0)
        return;
    Reserve(other.used);
    memcpy(buffer + used, other.buffer, other.used);
    used += other.used;
    buffer[used] = '\0';
}

const char* ReplayText::Data() const
{
    return buffer == nullptr ? "" : buffer;
}

unsigned ReplayText::Size() const
{
    return used;
}

static unsigned KnownHash(const char* name)
{
    unsigned h = 2166136261u;
    for (const unsigned char* p = (const unsigned char*)name; *p != 0; p++)
        h = (h ^ *p) * 16777619u;
    return h;
}

static char* KnownCopy(const char* text)
{
    unsigned n = (unsigned)strlen(text) + 1;
    char* copy = (char*)malloc(n);
    if (copy == nullptr)
        abort();
    memcpy(copy, text, n);
    return copy;
}

void ReplayKnown::Reset()
{
    for (int i = 0; i < count; i++) {
        free(entries[i].name);
        free(entries[i].value);
    }
    count = 0;
    if (slots != nullptr)
        memset(slots, 0, slotCount * sizeof(int));
}

int ReplayKnown::Find(const char* name) const
{
    if (slots == nullptr)
        return -1;
    for (unsigned i = KnownHash(name) & (slotCount - 1);; i = (i + 1) & (slotCount - 1)) {
        int at = slots[i];
        if (at == 0)
            return -1;
        if (strcmp(entries[at - 1].name, name) == 0)
            return at - 1;
    }
}

void ReplayKnown::Grow()
{
    if (count == capacity) {
        capacity = capacity == 0 ? 1024 : capacity * 2;
        entries = (Entry*)realloc(entries, (unsigned)capacity * sizeof(Entry));
        if (entries == nullptr)
            abort();
    }
    if (slotCount < (unsigned)capacity * 2) {
        slotCount = (unsigned)capacity * 2;
        free(slots);
        slots = (int*)calloc(slotCount, sizeof(int));
        if (slots == nullptr)
            abort();
        for (int at = 0; at < count; at++) {
            unsigned i = KnownHash(entries[at].name) & (slotCount - 1);
            while (slots[i] != 0)
                i = (i + 1) & (slotCount - 1);
            slots[i] = at + 1;
        }
    }
}

const char* ReplayKnown::Get(const char* name) const
{
    int at = Find(name);
    return at < 0 ? nullptr : entries[at].value;
}

bool ReplayKnown::Changed(const char* name, const char* value)
{
    int at = Find(name);
    if (at >= 0 && strcmp(entries[at].value, value) == 0)
        return false;
    Set(name, value);
    return true;
}

void ReplayKnown::Set(const char* name, const char* value)
{
    int at = Find(name);
    if (at >= 0) {
        if (strcmp(entries[at].value, value) != 0) {
            free(entries[at].value);
            entries[at].value = KnownCopy(value);
        }
        return;
    }
    Grow();
    at = count++;
    entries[at].name = KnownCopy(name);
    entries[at].value = KnownCopy(value);
    unsigned i = KnownHash(name) & (slotCount - 1);
    while (slots[i] != 0)
        i = (i + 1) & (slotCount - 1);
    slots[i] = at + 1;
}

void ReplayOrdinals::Reset()
{
    for (int k = 0; k < KINDS; k++)
        count[k] = 0;
}

unsigned ReplayOrdinals::Of(unsigned kind, const void* object)
{
    if (object == nullptr)
        return 0;
    if (kind >= KINDS)
        abort();
    for (unsigned i = 0; i < count[kind]; i++) {
        if (objects[kind][i] == object)
            return i + 1;
    }
    if (count[kind] == CAPACITY)
        abort();
    objects[kind][count[kind]++] = object;
    return count[kind];
}
