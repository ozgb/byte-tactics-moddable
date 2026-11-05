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

struct AccessLogScope {
    int after;
    AccessLogScope(int during, int next) : after(next) { AccessLogWindow(during); }
    ~AccessLogScope() { AccessLogWindow(after); }
};

#endif
