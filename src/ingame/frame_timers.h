#ifndef FRAME_TIMERS_H
#define FRAME_TIMERS_H

struct FrameTimers {
    unsigned long last;
    int total;
    int values[9];
    int acc[9];
    void AccumulateProfileTime(int i);
};

#endif
