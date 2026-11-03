#ifndef HARNESS_REPLAY_H
#define HARNESS_REPLAY_H

extern int g_replayCalls;

enum ReplayImportId {
    RI_GAME_RAND,
    RI_RANDOM_INT,
    RI_FATAL_ERROR,
    RI_BUILD_DATA_PATH,
    RI_TDF_CONSTRUCT,
    RI_TDF_DESTRUCT,
    RI_TDF_LOAD_FILE,
    RI_TDF_SELECT_RECORD,
    RI_TDF_GET_INT,
    RI_TDF_GET_DOUBLE,
    RI_TDF_GET_STRING,
    RI_GET_INT_ARG,
    RI_BANK_OPEN_ACCOUNT,
    RI_BANK_SET_INTEGER,
    RI_BANK_GET_INTEGER,
    RI_SPAWN_PROJECTILE,
    RI_FIND_WEAPON,
    RI_COUNT
};

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

struct ReplayImport {
    explicit ReplayImport(int id) : id(id), active(g_replayCalls != 0), count(0)
    {
        if (active) {
            g_replayCalls = 0;
            result.kind = -1;
        }
    }
    ~ReplayImport()
    {
        if (active)
            ReplayImportEnd(*this);
    }
    void Int(int value)
    {
        if (active)
            Push(0)->v.i = value;
    }
    void Ptr(const void* value)
    {
        if (active)
            Push(1)->v.p = value;
    }
    void Dbl(double value)
    {
        if (active)
            Push(2)->v.d = value;
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
    ReplayArg* Push(int kind)
    {
        ReplayArg* arg = &args[count++];
        arg->kind = kind;
        return arg;
    }
    int id;
    bool active;
    int count;
    ReplayArg args[5];
    ReplayArg result;
};

int ReplayConfigure(const char* value, char* error, int size);
void ReplayInstall();
int ReplayWrite();

#endif
