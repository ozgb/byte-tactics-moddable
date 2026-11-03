#ifndef HARNESS_REPLAY_VALUES_H
#define HARNESS_REPLAY_VALUES_H

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

#endif
