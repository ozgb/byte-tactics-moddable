#ifndef SETTINGS_H
#define SETTINGS_H

#pragma pack(push, 1)

struct Settings {
    int field_471;
    union {
        int flags_475;
        struct {
            unsigned int unknown_475_0 : 5;
            unsigned int flag_475_5 : 1;
            unsigned int unknown_475_6 : 26;
        } bits_475;
    };
    char unknown_479[0x48];
};

#pragma pack(pop)

#endif
