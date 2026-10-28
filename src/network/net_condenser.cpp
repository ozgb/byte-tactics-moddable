#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../../interfaces/host.h"
#include "net_condenser.h"

#pragma pack(push, 1)
struct Net_0044f9c0 {
    char unknown_0[0x4b5];
    int from;
};
#pragma pack(pop)

#include "net_stats.h"
#include "../files/files_api.h"

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

#include "../../link/globals.h"

#include "../harness/harness.h"

// FUNCTION: 0x44f720 _$E4
// FUNCTION: 0x44f7a0 _$E2
NetCondenser g_receiveCondenser;

// FUNCTION: 0x44f7e0 _$E9
// FUNCTION: 0x44f860 _$E7
NetCondenser g_sendCondenser;

// FUNCTION: 0x44f8a0
NetCondenser::NetCondenser()
{
    buffer = new char[0x6d60];
    unknown_4 = 0;
    unpackedCached = 0;
    unpackedSize = 0;
    sendBuffer = new char[0xaf0];
    packet = new char[0xaf0];
    buffer_14 = new char[0xaf0];
    sendCount = 0;
    sendSize = 0;
    to = 0;
}

// FUNCTION: 0x44f900
NetCondenser::~NetCondenser()
{
    delete[] buffer;
    delete[] sendBuffer;
    delete[] packet;
    delete[] buffer_14;
}

// FUNCTION: 0x44f940
void NetCondenser::Accumulate(void* data, int size)
{
    char msg[256];
    if (size > 0xaf0) {
        delete sendBuffer;
        sendBuffer = new char[size];
        sprintf(msg, "netCondenser: BUFFER_ACC_SIZE = %ld, requested send size = %ld , resized buffer to accomodate.\n",
                0xaf0, size);
    }
    memcpy(sendBuffer, data, size);
    sendSize = size;
    sendCount++;
}

// FUNCTION: 0x44f9c0
int NetCondenser::ReceivePacket(void* net, char* data, int* size)
{
    char msg[256];
    unsigned int n;
    int clientSize = *size;

    if (unpackedCached != 0) {
        if (unpackedSize > clientSize) {
            sprintf(msg, "netCondenser[1]: r_unpackedSize = %ld, client buffersize = %ld\n",
                    unpackedSize, clientSize);
            unpackedCached = 1;
            *size = unpackedSize;
            return 0x8877001e;
        }
        memcpy(data, buffer, unpackedSize);
        *size = unpackedSize;
        unpackedCached = 0;
        return 0;
    }

    CondensedHeader* received = (CondensedHeader*)data;
    int result = host_net_receive_packet((host_net*)net, data, (unsigned long*)size);
    if (((Net_0044f9c0*)net)->from != 0) {
        if (result == 0) {
            CountPacket(*size, 0, 0);
            if (received->kind != 3 && received->kind != 4)
                return 0;
            if (*size < 4)
                return 0x887700be;
            n = *size - 3;
            unsigned short sum = 0;
            for (unsigned int i = 3; i < n; i++) {
                sum += (unsigned char)data[i];
                data[i] ^= (unsigned char)i;
            }
            if (received->sum != sum)
                return 0x887700be;
            if (received->kind == 4) {
                LzssEnablePreset();
                unpackedSize = LzssExpand((unsigned char*)buffer, (unsigned char*)(data + 3));
                LzssDisablePreset();
            } else {
                memcpy(buffer, data + 3, n);
                unpackedSize = n;
            }
            if (unpackedSize > clientSize) {
                sprintf(msg, "netCondenser[2]: r_unpackedSize = %ld, client buffersize = %ld\n",
                        unpackedSize, clientSize);
                unpackedCached = 1;
                *size = unpackedSize;
                return 0x8877001e;
            }
            memcpy(data, buffer, unpackedSize);
            *size = unpackedSize;
            return 0;
        }
        if (result != 0x887700be) {
            if (result != 0x8877001e)
                return result;
        } else {
            goto common_check;
        }
common_check:
        if (result != 0x8877001e)
            return result;
        sprintf(msg, "netCondenser[3]: needed %ld, had client buffersize = %ld\n",
                clientSize, *size);
    }
    return result;
}

// FUNCTION: 0x44fc10
int NetCondenser::SendPacket(void* session, int from)
{
    unsigned int compressed;
    if (sendSize > 12) {
        LzssEnablePreset();
        compressed = LzssCompress((unsigned char*)(packet + 3), (unsigned char*)sendBuffer, sendSize);
        LzssDisablePreset();
    } else {
        compressed = 0xffff;
    }

    unsigned char flag;
    int total;
    if (compressed + 3 < sendSize && g_game->compressionDisabled == 0) {
        flag = 4;
        total = compressed + 3;
    } else {
        total = sendSize + 3;
        flag = 3;
        memcpy(packet + 3, sendBuffer, sendSize);
    }

    unsigned short sum = 0;
    for (int i = 3; i < total - 3; i++) {
        packet[i] ^= (unsigned char)i;
        sum += (unsigned char)packet[i];
    }
    header->kind = flag;
    header->sum = sum;

    int result;
    if (g_game->netSendDropPct != 0 && (int)((__int64)HarnessViewRand() * 101 / 32768) <= g_game->netSendDropPct)
        result = 0;
    else
        result = host_net_send_packet((host_net*)session, from, to, packet, total);
    if (result == 0)
        CountPacket(sendSize + 3, total, 1);
    sendSize = 0;
    sendCount = 0;
    return result;
}
