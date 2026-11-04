#ifndef HARNESS_REPLAY_H
#define HARNESS_REPLAY_H

inline int& ReplayCalls()
{
    static int calls;
    return calls;
}

#include "replay_services.h"

struct ReplayArg {
    int kind;
    union {
        int i;
        const void* p;
        double d;
    } v;
};

struct ReplayImport;
void ReplayImportEnd(ReplayImport& call);

typedef bool (*ReplayAnswerFn)(ReplayImport& call);

inline ReplayAnswerFn& ReplayAnswerHook()
{
    static ReplayAnswerFn hook;
    return hook;
}

inline bool& ReplayAnswerBusy()
{
    static bool busy;
    return busy;
}

struct ReplayImport {
    explicit ReplayImport(int id) : id(id), active(ReplayCalls() != 0), answering(ReplayAnswerHook() != nullptr && !ReplayAnswerBusy()), count(0)
    {
        result.kind = -1;
        if (active)
            ReplayCalls() = 0;
    }
    ~ReplayImport()
    {
        if (active)
            ReplayImportEnd(*this);
    }
    void Int(int value)
    {
        if (active || answering)
            AddArg(0)->v.i = value;
    }
    void Ptr(const void* value)
    {
        if (active || answering)
            AddArg(1)->v.p = value;
    }
    void Dbl(double value)
    {
        if (active || answering)
            AddArg(2)->v.d = value;
    }
    bool Answered()
    {
        if (!answering)
            return false;
        ReplayAnswerBusy() = true;
        bool answered = ReplayAnswerHook()(*this);
        ReplayAnswerBusy() = false;
        return answered;
    }
    void Return(int value)
    {
        result.kind = 0;
        result.v.i = value;
    }
    void Return(double value)
    {
        result.kind = 2;
        result.v.d = value;
    }
    void Return(const void* value)
    {
        result.kind = 1;
        result.v.p = value;
    }
    int Result(int value)
    {
        result.kind = 0;
        result.v.i = value;
        return value;
    }
    double Result(double value)
    {
        result.kind = 2;
        result.v.d = value;
        return value;
    }
    template <class T> T* Result(T* value)
    {
        result.kind = 1;
        result.v.p = value;
        return value;
    }
    ReplayArg* AddArg(int kind)
    {
        ReplayArg* arg = &args[count++];
        arg->kind = kind;
        return arg;
    }
    int id;
    bool active;
    bool answering;
    int count;
    ReplayArg args[8];
    ReplayArg result;
};

int ReplayConfigure(const char* value, char* error, int size);
void ReplayInstall();
int ReplayWrite(char* error, int size);

#endif
