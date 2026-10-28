#ifndef NET_CONDENSER_H
#define NET_CONDENSER_H

#pragma pack(push, 1)
struct CondensedHeader {
    unsigned char kind;
    unsigned short sum;
};

class NetCondenser {
public:
    char* buffer;
    int unknown_4;
    int unpackedCached;
    int unpackedSize;
    char* sendBuffer;
    char* buffer_14;
    union {
        char* packet;
        CondensedHeader* header;
    };
    unsigned char sendCount;
    int sendSize;
    int to;

    NetCondenser();
    ~NetCondenser();
    void Accumulate(void* data, int size);
    int ReceivePacket(void* net, char* data, int* size);
    int SendPacket(void* session, int from);
    void SendPacketTo(void* session, int from, int value, void* data, int size);
};
#pragma pack(pop)

extern NetCondenser g_receiveCondenser;
extern NetCondenser g_sendCondenser;

#endif
