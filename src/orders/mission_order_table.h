#ifndef MISSION_ORDER_TABLE_H
#define MISSION_ORDER_TABLE_H

struct MissionOrderTableEntry {
    char unknown_0[0x10];
    unsigned char markerAnim;
    char unknown_11[0x19 - 0x11];
};

#endif
