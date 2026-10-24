// GLOBAL: 0x4fc978
extern const float DAT_004fc978 = 2.0f;
// GLOBAL: 0x4fcc48
extern const double DAT_004fcc48 = 2.0;
// GLOBAL: 0x4fd2e4
extern const float DAT_004fd2e4 = 2.0f;
// GLOBAL: 0x4fda70
extern const double DAT_004fda70 = 2.0;

// GLOBAL: 0x4fdbe0
extern const unsigned char DAT_004fdbe0[8] = {0};

// GLOBAL: 0x4fc474
extern const unsigned char DAT_004fc474[4] = {0x00, 0x02, 0xfc, 0x07};

// GLOBAL: 0x502f98
int DAT_00502f98 = 16;

// GLOBAL: 0x507b64
int DAT_00507b64 = 0;
// GLOBAL: 0x507b68
int DAT_00507b68 = 1;

struct OptionChoice {
    const char* label;
    int value;
};

// GLOBAL: 0x5066f8
OptionChoice DAT_005066f8[4] = {
    {"%d  Slow", -16},
    {"%d  Normal", 16},
    {"%d  Fast", 64},
    {0, -1},
};

// GLOBAL: 0x506718
OptionChoice DAT_00506718[4] = {
    {"%d  Sluggish", -5},
    {"%d  Normal", 5},
    {"%d  Rapid Fire", 20},
    {0, -1},
};

// GLOBAL: 0x506738
OptionChoice DAT_00506738[7] = {
    {"No Chat", 0},
    {"Only important info", 2},
    {"Medium chat", 4},
    {"Detailed chat", 6},
    {"High chat level", 8},
    {"All info chatted", 10},
    {0, -2},
};

extern char g_notrakGadgetName[8];

// GLOBAL: 0x506770
const char* DAT_00506770[6] = {g_notrakGadgetName, "NORMTRAK", "RANDTRAK", "REPTTRAK", "SPECTRAK", 0};

#pragma pack(push, 1)
struct WaveFormatName {
    unsigned int flag;
    const char* name;
    unsigned char bits;
    unsigned char channels;
    unsigned int rate;
};
#pragma pack(pop)

// GLOBAL: 0x50b6e0
WaveFormatName DAT_0050b6e0[13] = {
    {0x1, "11.025 kHz, mono, 8-bit", 8, 1, 11025},
    {0x4, "11.025 kHz, mono, 16-bit", 16, 1, 11025},
    {0x2, "11.025 kHz, stereo, 8-bit", 8, 2, 11025},
    {0x8, "11.025 kHz, stereo, 16-bit", 16, 2, 11025},
    {0x10, "22.05 kHz, mono, 8-bit", 8, 1, 22050},
    {0x40, "22.05 kHz, mono, 16-bit", 16, 1, 22050},
    {0x20, "22.05 kHz, stereo, 8-bit", 8, 2, 22050},
    {0x80, "22.05 kHz, stereo, 16-bit", 16, 2, 22050},
    {0x100, "44.1 kHz, mono, 8-bit", 8, 1, 44100},
    {0x400, "44.1 kHz, mono, 16-bit", 16, 1, 44100},
    {0x200, "44.1 kHz, stereo, 8-bit", 8, 2, 44100},
    {0x800, "44.1 kHz, stereo, 16-bit", 16, 2, 44100},
    {0, 0, 0, 0, 0},
};
