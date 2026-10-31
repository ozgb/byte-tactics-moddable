#include "win_types.h"
#include <stdio.h>
#include <io.h>

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

#include "net_stats.h"
#include "../../link/globals.h"

#include "bit_writer.h"

#include "bit_reader.h"

void __stdcall RegisterOrderTypes(void* table, int id);
void __stdcall StepGafSequence(void* item);

unsigned int __cdecl GetTicks();

// GLOBAL: 0x511de8
extern Game* g_game;
extern char g_vtolOrders[];

// FUNCTION: 0x415b20
void RegisterVtolOrders()
{
    RegisterOrderTypes(g_vtolOrders, 0x16);
}

// FUNCTION: 0x415b30
void StepAllGafSequences(void)
{
    for (int i = g_game->count_148e7 - 1; i >= 0; i--)
        StepGafSequence(((void **)g_game->data_148eb)[i]);
}

// FUNCTION: 0x415b60
BitWriter::BitWriter()
{
    index = 0;
    bit = 0;
    capacity = 0x100;
    data = buffer;
    *data = 0;
}

// FUNCTION: 0x415b90
void BitWriter::FreeBuffer()
{
    if (data != buffer) {
        operator delete(data);
    }
}

static inline void CopyWords(unsigned int* first, unsigned int* last, unsigned int* dst)
{
    for (; first != last; first++) {
        *dst = *first;
        dst++;
    }
}

// FUNCTION: 0x415bb0
void BitWriter::GrowBuffer()
{
    int newCapacity = capacity * 2;
    unsigned int* grown = new unsigned int(newCapacity);
    CopyWords(data, data + capacity, grown);
    if (data != buffer) {
        delete data;
    }
    capacity = newCapacity;
    data = grown;
}

// FUNCTION: 0x415c10
void BitWriter::WriteBits(int value, int bits)
{
    if (bits + bit < 0x20) {
        data[index] |= (value & ((1 << bits) - 1)) << bit;
        bit += bits;
        return;
    }
    if (bit == 0) {
        data[index] = value;
        index++;
        if (index == capacity) {
            GrowBuffer();
        }
        data[index] = 0;
        return;
    }
    int n = 0x20 - bit;
    data[index] |= (value & ((1 << n) - 1)) << bit;
    index++;
    if (index == capacity) {
        GrowBuffer();
    }
    bit = bits - n;
    data[index] = ((unsigned int)value >> n) & ((1 << bit) - 1);
}

// FUNCTION: 0x415da0
void BitWriter::SetByteAt(int offset, unsigned char value)
{
    ((unsigned char*)data)[offset] = value;
}

// FUNCTION: 0x415dc0
int BitReader::ReadBits(int bits)
{
    if (bit + bits < 32) {
        int r = (data[index] >> bit) & ((1 << bits) - 1);
        bit += bits;
        return r;
    }
    if (bit == 0) {
        return data[index++];
    }
    int low = 32 - bit;
    int r = (data[index] >> bit) & ((1 << low) - 1);
    bit = bits - low;
    index++;
    r |= (data[index] & ((1 << bit) - 1)) << low;
    return r;
}

// FUNCTION: 0x415e60
int BitReader::ReadSignedBits(int bits)
{
    int r = ReadBits(bits);
    if (r & (1 << (bits - 1))) {
        r |= -1 << bits;
    }
    return r;
}

// FUNCTION: 0x415e90
void ResetNetStats()
{
    DAT_00511c20 = g_game->ticks;
    g_messageBytesReceived = 0;
    g_messageBytesSent = 0;
    MessageByteCounter* p = (MessageByteCounter*)&g_messageBytesByType[1];
    for (int i = 0; i < 44; i++) {
        g_messageCountByType[i + 1][0] = 0;
        g_messageCountByType[i + 1][1] = 0;
        g_messageBytesByType[i + 1][0] = 0;
        p->b = 0;
        p++;
    }
    g_packetsSent = 0;
    g_packetBytesSent = 0;
    g_packetsReceived = 0;
    g_packetBytesReceived = 0;
}

// FUNCTION: 0x415ef0
void __stdcall CountMessage(unsigned char kind, int amount, int player)
{
    (&g_messageBytesReceived)[player] += amount;
    if (kind > 1 && kind < 0x2d) {
        g_messageCountByType[kind][player]++;
        g_messageBytesByType[kind][player] += amount;
    }
}

// FUNCTION: 0x415f40
void __stdcall CountPacket(int size, int overhead, int sent)
{
    if (overhead > 0) {
        g_compressedBytesSent += overhead;
    }
    if (sent != 0) {
        g_packetsSent++;
        g_packetBytesSent += size;
    } else {
        g_packetsReceived++;
        g_packetBytesReceived += size;
    }
}

// FUNCTION: 0x415fa0
void __stdcall FormatNetStats(char* text)
{
    unsigned int now = GetTicks();
    unsigned int elapsed = now - g_netStatsTick;
    if (elapsed > 30) {
        g_netStatsTick = now;
        g_messageBytesReceivedRate = (g_messageBytesReceived * 30 - g_lastMessageBytesReceived * 30) / elapsed;
        int packets = g_messageBytesSent - g_lastMessageBytesSent;
        g_messageBytesSentRate = packets * 30 / elapsed;
        g_lastMessageBytesReceived = g_messageBytesReceived;
        g_lastMessageBytesSent = g_messageBytesSent;
        if (packets > 0)
            g_compressionPercent = (g_lastCompressedBytesSent + packets - g_compressedBytesSent) * 100 / packets;
        else
            g_compressionPercent = 0;
        g_lastCompressedBytesSent = g_compressedBytesSent;
        g_packetBytesSentRate = (g_packetBytesSent * 30 - g_lastPacketBytesSent * 30) / elapsed;
        g_packetBytesReceivedRate = (g_packetBytesReceived * 30 - g_lastPacketBytesReceived * 30) / elapsed;
        g_packetsSentRate = (g_packetsSent * 30 - g_lastPacketsSent * 30) / elapsed;
        g_packetsReceivedRate = (g_packetsReceived * 30 - g_lastPacketsReceived * 30) / elapsed;
        g_lastPacketBytesSent = g_packetBytesSent;
        g_lastPacketBytesReceived = g_packetBytesReceived;
        g_lastPacketsSent = g_packetsSent;
        g_lastPacketsReceived = g_packetsReceived;
    }
    sprintf(text, "pS=%4d pR=%4d (S=%d/%4d, R=%d/%4d) C=%3d%%\n", g_messageBytesSentRate, g_messageBytesReceivedRate,
            g_packetsSentRate, g_packetBytesSentRate, g_packetsReceivedRate, g_packetBytesReceivedRate, g_compressionPercent);
}


#include "../network/packet_manager.h"
#include "../state/sim_globals.h"


// FUNCTION: 0x416150
void __stdcall GetByteRates(unsigned int* sent, unsigned int* received)
{
    unsigned int now = GetTicks();
    unsigned int elapsed = now - g_byteRatesTick;
    if (elapsed > 30) {
        g_byteRatesTick = now;
        g_bytesSentPerSecond = (g_packetBytesSent * 30 - g_byteRatesLastSent * 30) / elapsed;
        g_bytesReceivedPerSecond = (g_packetBytesReceived * 30 - g_byteRatesLastReceived * 30) / elapsed;
        g_byteRatesLastSent = g_packetBytesSent;
        g_byteRatesLastReceived = g_packetBytesReceived;
    }
    *sent = g_bytesSentPerSecond;
    *received = g_bytesReceivedPerSecond;
}

// FUNCTION: 0x4161f0
void SumMessageStats()
{
    int sum3 = 0, sum4 = 0, sum1 = 0, sum2 = 0;
    MessageByteCounter* p = (MessageByteCounter*)&g_messageBytesByType[1];
    for (int i = 0; i < 44; i++) {
        sum1 += g_messageCountByType[i + 1][0];
        sum2 += g_messageCountByType[i + 1][1];
        sum3 += g_messageBytesByType[i + 1][0];
        sum4 += p->b;
        p++;
    }
    g_packetManager.NopRet_D();
}
