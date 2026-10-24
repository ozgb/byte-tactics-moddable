#include "packet_ring.h"

inline PacketRing::PacketRing() { count = 0; readIdx = 0; writeIdx = -1; }

#include "packet_receiver.h"

inline PacketReceiver::PacketReceiver(PacketManager* o)
    : unused(0), owner(o), fromId(-1), toId(-1), savedFrameEntry(0), buffer(0), spare(0),
      capacity(0), length(0), spareLength(0), spareFromId(-1), spareToId(-1)
{
}

inline PacketReceiver::~PacketReceiver()
{
    void* p = spare;
    if (!p)
        p = buffer;
    operator delete(p);
}

#include "packet_channel.h"

inline PacketChannel::PacketChannel()
    : bufferIndex(-1), sendPacingTicks(0), buffers(0), bufferCount(0), frameNumber(-2), dpid(-1), timeoutTicks(0),
      packetIndex(-1), queuedBytes(0), nextSendTick(0), packets(0), count(0), firstPacket(0), lastPacket(0)
{
    SetMinRetainMs(4000);
    SetSendPacingMs(200);
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

#include "packet_manager.h"

inline PacketManager::~PacketManager() { }


PacketManager::PacketManager() : m_defaultSendPacingMs(200), buffer(0), size(0), capacity(0), member(this) { }
