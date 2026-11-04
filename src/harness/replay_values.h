#ifndef HARNESS_REPLAY_VALUES_H
#define HARNESS_REPLAY_VALUES_H

#include <stdlib.h>
#include <string.h>

#include "../../interfaces/rand_stream.h"
#include "../util/vec3.h"
#include "replay_text.h"

inline void ReplayValue(ReplayText& t, int v)
{
    t.AddInt(v);
}

inline void ReplayValue(ReplayText& t, long v)
{
    t.AddInt((int)v);
}

inline void ReplayValue(ReplayText& t, short v)
{
    t.AddInt(v);
}

inline void ReplayValue(ReplayText& t, char v)
{
    t.AddInt(v);
}

inline void ReplayValue(ReplayText& t, bool v)
{
    t.AddInt(v ? 1 : 0);
}

inline void ReplayValue(ReplayText& t, unsigned v)
{
    t.AddUnsigned(v);
}

inline void ReplayValue(ReplayText& t, unsigned short v)
{
    t.AddUnsigned(v);
}

inline void ReplayValue(ReplayText& t, unsigned char v)
{
    t.AddUnsigned(v);
}

inline void ReplayValue(ReplayText& t, float v)
{
    unsigned bits;
    memcpy(&bits, &v, 4);
    t.AddF32(bits);
}

inline void ReplayValue(ReplayText& t, double v)
{
    unsigned bits[2];
    memcpy(bits, &v, 8);
    t.AddF64(bits[0], bits[1]);
}

inline void ReplayValue(ReplayText& t, Point16 v)
{
    t.AddChar('(');
    t.AddInt(v.x);
    t.AddChar(',');
    t.AddInt(v.y);
    t.AddChar(')');
}

inline void ReplayValue(ReplayText& t, const Vec3& v)
{
    t.AddChar('(');
    t.AddInt(v.x);
    t.AddChar(',');
    t.AddInt(v.y);
    t.AddChar(',');
    t.AddInt(v.z);
    t.AddChar(')');
}

inline void ReplayValue(ReplayText& t, RandStream v)
{
    if (v == RAND_STREAM_MAIN)
        t.Add("main");
    else if (v == RAND_STREAM_LOAD)
        t.Add("load");
    else
        t.AddInt((int)v);
}

inline void ReplayValue(ReplayText& t, const char* v)
{
    t.AddString(v);
}

inline void ReplayValue(ReplayText& t, const Vec3* v)
{
    ReplayValue(t, *v);
}

inline void ReplayBytes(ReplayText& t, const void* p, int length)
{
    t.Add("x:");
    for (int i = 0; p != nullptr && i < length; i++) {
        unsigned char b = ((const unsigned char*)p)[i];
        t.AddChar("0123456789abcdef"[b >> 4]);
        t.AddChar("0123456789abcdef"[b & 15]);
    }
}

inline void ReplayAddTextN(ReplayText& t, const char* p, int n)
{
    if (p == nullptr) {
        t.Add("null");
        return;
    }
    char* copy = (char*)malloc((unsigned)(n < 0 ? 0 : n) + 1);
    memcpy(copy, p, (unsigned)(n < 0 ? 0 : n));
    copy[n < 0 ? 0 : n] = '\0';
    t.AddString(copy);
    free(copy);
}

template <class T> void ReplayAddArgs(ReplayText& t, const T* args)
{
    if (args == nullptr) {
        t.Add("null");
        return;
    }
    t.AddChar('[');
    for (int i = 0; i < args->count && i < 0x14; i++) {
        if (i != 0)
            t.AddChar(',');
        t.AddString(args->args[i]);
    }
    t.AddChar(']');
}

inline void ReplayAddWords(ReplayText& t, const unsigned* p, unsigned count)
{
    t.AddChar('[');
    for (unsigned i = 0; p != nullptr && i < count; i++) {
        if (i != 0)
            t.AddChar(',');
        t.AddUnsigned(p[i]);
    }
    t.AddChar(']');
}

inline void ReplayParseBytes(const char* text, void* dst, int length)
{
    if (strncmp(text, "x:", 2) != 0)
        return;
    for (int i = 0; i < length && text[2 + i * 2] != '\0' && text[3 + i * 2] != '\0'; i++) {
        char hi = text[2 + i * 2];
        char lo = text[3 + i * 2];
        ((unsigned char*)dst)[i] = (unsigned char)((hi <= '9' ? hi - '0' : hi - 'a' + 10) << 4 | (lo <= '9' ? lo - '0' : lo - 'a' + 10));
    }
}

#endif
