#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "replay_text.h"

void ReplayText::Free()
{
    free(m_data);
    m_data = 0;
    m_size = 0;
    m_capacity = 0;
}

void ReplayText::Clear()
{
    m_size = 0;
    if (m_data != 0)
        m_data[0] = '\0';
}

void ReplayText::Reserve(unsigned extra)
{
    if (m_size + extra + 1 <= m_capacity)
        return;
    unsigned capacity = m_capacity == 0 ? 4096 : m_capacity;
    while (capacity < m_size + extra + 1)
        capacity *= 2;
    char* grown = (char*)realloc(m_data, capacity);
    if (grown == 0)
        abort();
    m_data = grown;
    m_capacity = capacity;
}

void ReplayText::Add(const char* text)
{
    unsigned length = (unsigned)strlen(text);
    Reserve(length);
    memcpy(m_data + m_size, text, length);
    m_size += length;
    m_data[m_size] = '\0';
}

void ReplayText::AddChar(char c)
{
    Reserve(1);
    m_data[m_size++] = c;
    m_data[m_size] = '\0';
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
    if (text == 0) {
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
    if (other.m_size == 0)
        return;
    Reserve(other.m_size);
    memcpy(m_data + m_size, other.m_data, other.m_size);
    m_size += other.m_size;
    m_data[m_size] = '\0';
}

const char* ReplayText::Data() const
{
    return m_data == 0 ? "" : m_data;
}

unsigned ReplayText::Size() const
{
    return m_size;
}

void ReplayKnown::Reset()
{
    m_count = 0;
}

int ReplayKnown::Find(const char* name) const
{
    for (int i = 0; i < m_count; i++) {
        if (strcmp(m_name[i], name) == 0)
            return i;
    }
    return -1;
}

bool ReplayKnown::Changed(const char* name, const char* value)
{
    int at = Find(name);
    if (at >= 0 && strcmp(m_value[at], value) == 0)
        return false;
    Set(name, value);
    return true;
}

void ReplayKnown::Set(const char* name, const char* value)
{
    int at = Find(name);
    if (at < 0) {
        if (m_count == CAPACITY || strlen(name) >= NAME)
            abort();
        at = m_count++;
        strcpy(m_name[at], name);
    }
    if (strlen(value) >= VALUE)
        abort();
    strcpy(m_value[at], value);
}

void ReplayOrdinals::Reset()
{
    for (int k = 0; k < KINDS; k++)
        m_count[k] = 0;
}

unsigned ReplayOrdinals::Of(unsigned kind, const void* object)
{
    if (object == 0)
        return 0;
    if (kind >= KINDS)
        abort();
    for (unsigned i = 0; i < m_count[kind]; i++) {
        if (m_object[kind][i] == object)
            return i + 1;
    }
    if (m_count[kind] == CAPACITY)
        abort();
    m_object[kind][m_count[kind]++] = object;
    return m_count[kind];
}
