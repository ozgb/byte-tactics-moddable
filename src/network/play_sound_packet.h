#ifndef PLAY_SOUND_PACKET_H
#define PLAY_SOUND_PACKET_H

#pragma pack(push, 1)
struct PlaySoundPacket {
    unsigned char type;
    unsigned char flag;
    int index;
    int pos[3];
};
#pragma pack(pop)

#endif
