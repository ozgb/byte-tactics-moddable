void __cdecl operator delete(void*);

#include "packet_manager.h"

inline FrameQueue::~FrameQueue()
{
    operator delete(buffer);
    operator delete(recvBuffer);
}

inline PlayerFrameInfo::~PlayerFrameInfo()
{
    operator delete(frame);
}

inline PacketReceiver::~PacketReceiver()
{
    void* p = spare;
    if (!p)
        p = buffer;
    operator delete(p);
}

inline PacketChannel::~PacketChannel()
{
    if (buffers) {
        for (unsigned i = 0; i < bufferCount; i++)
            operator delete(buffers[i]);
        operator delete(buffers);
    }
    operator delete(packets);
}

inline PacketManager::~PacketManager() { }

