#ifndef ACCESSLOG_ACCESS_LOG_H
#define ACCESSLOG_ACCESS_LOG_H

enum AccessWindow {
    ACCESS_WINDOW_TAIL = 8,
    ACCESS_WINDOW_PRESENTATION = 9,
    ACCESS_WINDOW_PENDING = 10,
    ACCESS_WINDOW_OFF = 11
};

void AccessLogInit();

void AccessLogWindow(int window);

void AccessLogMarker(int phase, unsigned tick);

void AccessLogFinish();

void AccessLogEmit(int kind, void* ra);

void AccessLogDraw(int stream, void* ra);

enum AccessPass {
    ACCESS_PASS_NONE,
    ACCESS_PASS_UNITS,
    ACCESS_PASS_PROJECTILES,
    ACCESS_PASS_EXPLOSIONS,
    ACCESS_PASS_PLAYERS,
    ACCESS_PASS_FEATURES,
    ACCESS_PASS_WIND,
    ACCESS_PASS_METEORS,
    ACCESS_PASS_TAIL,
    ACCESS_PASS_FEATURES_ANIM,
    ACCESS_PASS_EXPLOSIONS_PIECES,
    ACCESS_PASS_EXPLOSIONS_DEBRIS,
    ACCESS_PASS_UNITS_PLAYERS,
    ACCESS_PASS_PLAYERS_LOS,
    ACCESS_PASS_COUNT
};

enum AccessMark { ACCESS_MARK_ENTITY, ACCESS_MARK_PASS_END, ACCESS_MARK_PASS, ACCESS_MARK_COUNT = 16 };

struct AccessMarks {
    volatile unsigned char mark[ACCESS_MARK_COUNT];
    volatile unsigned simCaller;
};

extern AccessMarks g_accessMarks;

inline __attribute__((always_inline)) void AccessLogPass(int pass) { g_accessMarks.mark[ACCESS_MARK_PASS + pass] = 0; }

inline __attribute__((always_inline)) void AccessLogEntity() { g_accessMarks.mark[ACCESS_MARK_ENTITY] = 0; }

inline __attribute__((always_inline)) void AccessLogSimEvent(void* ra) { g_accessMarks.simCaller = (unsigned)ra; }

struct AccessLogPassScope {
    __attribute__((always_inline)) AccessLogPassScope(int pass) { AccessLogPass(pass); }
    __attribute__((always_inline)) ~AccessLogPassScope() { g_accessMarks.mark[ACCESS_MARK_PASS_END] = 0; }
};

struct AccessLogScope {
    int after;
    AccessLogScope(int during, int next) : after(next) { AccessLogWindow(during); }
    ~AccessLogScope() { AccessLogWindow(after); }
};

#endif
