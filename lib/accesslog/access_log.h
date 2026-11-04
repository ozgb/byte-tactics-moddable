#ifndef ACCESSLOG_ACCESS_LOG_H
#define ACCESSLOG_ACCESS_LOG_H

// The windows of a tick that accesses are filed under: phases 0 to 7 are HarnessPhase's, the rest follow.
enum AccessWindow {
    ACCESS_WINDOW_TAIL = 8,
    ACCESS_WINDOW_PRESENTATION = 9,
    ACCESS_WINDOW_PENDING = 10,
    ACCESS_WINDOW_OFF = 11
};

// Starts the log: loads the tables beside the exe and hooks the sim allocator.
void AccessLogInit();

// Files the accesses that follow under a window.
void AccessLogWindow(int window);

// HarnessPhase has reached marker phase of a tick: the window before it was that phase's work.
void AccessLogMarker(int phase, unsigned tick);

// Writes the edge summary and ends the log.
void AccessLogFinish();

// Notes the sound or UI record being emitted: kind 0 is a sound, 1 a UI record; ra is the emitting function's return address.
void AccessLogEmit(int kind, void* ra);

// Notes a draw from a game-logic random stream by the function that returns to ra.
void AccessLogDraw(int stream, void* ra);

// Switches the window for the length of a scope.
struct AccessLogScope {
    int after;
    AccessLogScope(int during, int next) : after(next) { AccessLogWindow(during); }
    ~AccessLogScope() { AccessLogWindow(after); }
};

#endif
