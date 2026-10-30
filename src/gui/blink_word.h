#ifndef BLINK_WORD_H
#define BLINK_WORD_H

#pragma pack(push, 1)
struct BlinkWord {
    char text[0x80];
    int x;
    int y;
    int colourA;
    float periodA;
    int colourB;
    float periodB;
    int phase;
    float nextToggle;
    int value;
};
#pragma pack(pop)

#endif
