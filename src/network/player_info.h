#ifndef PLAYER_INFO_H
#define PLAYER_INFO_H

#pragma pack(push, 1)
struct PlayerInfo {
    char map[0x80];
    char password[0xb];
    unsigned short width;
    unsigned short height;
    char unknown_8f;
    int id;
    unsigned char kind;
    unsigned char side;
    unsigned char color;
    union {
        unsigned char flags_97;
        unsigned short flags_97_wide;
        struct {
            unsigned short host : 1;
            unsigned short shareMetal : 1;
            unsigned short shareEnergy : 1;
            unsigned short shareLOS : 1;
            unsigned short unknownBit4 : 1;
            unsigned short shareMapping : 1;
            unsigned short shareRadar : 1;
            unsigned short unknownRest : 9;
        };
    };
    unsigned short memory;
    union {
        unsigned char flags_9b;
        unsigned short flags;
        struct {
            unsigned short low : 4;
            unsigned short started : 1;
            unsigned short bit5 : 1;
            unsigned short bit6 : 1;
            unsigned short watching : 1;
            unsigned short mapping : 1;
            unsigned short los : 1;
            unsigned short losType : 1;
            unsigned short commander : 2;
            unsigned short cheating : 1;
            unsigned short fixedloc : 1;
            unsigned short closed : 1;
        };
    };
    union {
        unsigned char flags_9d;
        unsigned short flags_9d_wide;
        struct {
            unsigned short hasPassword : 1;
            unsigned short f9d_1 : 1;
            unsigned short f9d_2 : 1;
            unsigned short f9d_rest : 13;
        };
    };
    unsigned short pingLimit;
    unsigned short energy;
    unsigned short metal;
    unsigned short maxUnits;
    unsigned char versionMajor;
    unsigned char versionMinor;
    unsigned int mapCrc;
    char unknown_ad[0xb9 - 0xad];
};
#pragma pack(pop)

#endif
