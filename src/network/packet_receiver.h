#ifndef PACKET_RECEIVER_H
#define PACKET_RECEIVER_H

#include "../../interfaces/ref.h"

struct FrameRing;
class PacketManager;

class FrameQueue {
public:
    int baseTick;
    unsigned int bufferSize;
    int skipCount;
    char* recvBuffer;
    FrameRing* buffer;
    int fromId;
    int toId;

    ~FrameQueue();
    FrameQueue();
    int ResetFrames();
    int QueueFrames(char* src, unsigned int size, int tick, int a4, int a5, int a6);
};

class PlayerFrameInfo {
public:
    int playerNetId;
    int pendingDpToId;
    int frameSeq;
    int pendingBytes;
    int pendingCap;
    char* frame;
    FrameQueue tail;

    PlayerFrameInfo();
    void Initialize(long id);
    ~PlayerFrameInfo();
};

class PacketReceiver {
public:
    PacketReceiver() { }
    PacketReceiver(PacketManager* o);
    virtual ~PacketReceiver();
    int unused;
    Ref<PacketManager> owner;
    int fromId;
    int toId;
    Ref<PlayerFrameInfo> savedFrameEntry;
    char* buffer;
    char* spare;
    PlayerFrameInfo entries[10];
    int capacity;
    int length;
    int spareLength;
    int spareFromId;
    int spareToId;

    PlayerFrameInfo* FindPlayerFrameInfo(long id);
    int ResetReceiveBuffer();
    int ReceiveFrame(void* net, unsigned char* data, int* size);
};

#endif
