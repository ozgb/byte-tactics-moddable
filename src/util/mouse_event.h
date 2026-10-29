#ifndef MOUSE_EVENT_H
#define MOUSE_EVENT_H

struct MouseEvent {
    int x;
    int y;
    unsigned int keyFlags;
    int scaledTick;
    int msg;
    int dblClick;
};

#endif
