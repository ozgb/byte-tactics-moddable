#ifndef HARNESS_SHARE_PACKETS_H
#define HARNESS_SHARE_PACKETS_H

enum { SHARE_PACKET_SECTION = 35, SHARE_PACKET_BYTES = 17, SHARE_PACKET_CAPACITY = 64 };

struct SharePacket {
    unsigned int type;
    unsigned int subtype;
    unsigned int from;
    unsigned int to;
    unsigned int amount;
};

struct SharePackets {
    SharePacket items[SHARE_PACKET_CAPACITY];
    unsigned int count;
};

#endif
