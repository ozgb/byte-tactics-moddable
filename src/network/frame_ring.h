#ifndef FRAME_RING_H
#define FRAME_RING_H

struct Frame {
    int tick;
    void* data;
    int size;
};

struct FrameRing {
    int count;
    int head;
    int tail;
    Frame frames[0x200];

    FrameRing() { count = 0; head = 0; tail = -1; }

    Frame* Pop()
    {
        if (count > 0) {
            count--;
            Frame* f = &frames[head];
            if (++head >= 0x200)
                head = 0;
            return f;
        }
        return 0;
    }

    int Push(int tick, void* data, int size)
    {
        if (count >= 0x200)
            return 0;
        if (++tail >= 0x200)
            tail = 0;
        frames[tail].data = data;
        frames[tail].tick = tick;
        frames[tail].size = size;
        count++;
        return 1;
    }
};

#endif
