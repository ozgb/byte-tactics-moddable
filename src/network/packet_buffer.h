#ifndef PACKET_BUFFER_H
#define PACKET_BUFFER_H

#include "../../interfaces/ref.h"

class PacketBuffer;
class PacketChannel;

struct NetPacketPendingEntry {
    int frame;
    int offset;
    int size;
    PacketBuffer* owner;
    int queued;
    int sentTime;
    NetPacketPendingEntry* prev;
    NetPacketPendingEntry* next;

    void RemoveFromBuffer();
    int GetData();
};

class PacketBuffer {
public:
    Ref<PacketChannel> pool;
    int start;
    int count;
    int length;
    NetPacketPendingEntry* first;
    unsigned char data[0x42a];

    int AppendPacket(NetPacketPendingEntry* packet, int index, const void* src, unsigned int size, NetPacketPendingEntry* prev);
    void FreePackets();
    void FindOwnerChainTail();
    int IsReusable(int minRetain);
    void RemovePacket(NetPacketPendingEntry* packet);
};

#endif
