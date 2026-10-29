#ifndef GADGET_H
#define GADGET_H

#pragma pack(push, 1)

struct GafEntry;
struct GafFrame;
struct Gui;
struct UnitRestrictEntry;

struct Gadget {
    unsigned char type;
    unsigned char team;
    char name[0x11];
    short x;
    short y;
    short width;
    short height;
    int attribs;
    int colours;
    union {
        int image;
        int colour;
    };
    char textureNumber;
    signed char tab;
    unsigned char active;
    char commonAttribs;
    void* archive;
    GafEntry* gaf;
    char helpKey[0x60 - 0x33];
    int index;
    char unknown_64[0xb4 - 0x64];
    unsigned short shown : 1;
    unsigned short bits_b4 : 15;
    union {
        short count;
        char text[0x80];
        struct {
            char unknown_b6[2];
            void* saveUnder;
            void* surface;
            void* archive;
            GafEntry* background;
        } assets;
        struct {
            char unknown_b6[2];
            GafFrame* glyph;
            unsigned short flag;
        } frame;
        struct {
            int sortKey;
            short selected;
            short firstRow;
            short maxFirstRow;
            short rowCount;
            char* lines;
            void* rows;
            GafEntry* gaf;
            void (__stdcall* callback)(Gui*, Gadget*);
            union {
                void* records;
                UnitRestrictEntry* restrictions;
            };
            union {
                int language;
                void* filebuf;
                char* flags;
            };
            short scroll;
        } list;
        struct {
            char unknown_b6[0x16];
            char choice[0x10];
            char choice2[0x10];
            char focusName[0x10];
        } names;
        struct {
            void (__stdcall* callback)(Gui*, Gadget*);
            char unknown_ba[4];
            GafEntry* gaf;
            GafFrame* frame;
            short index;
            union {
                unsigned char flags;
                struct {
                    unsigned char clickable : 1;
                    unsigned char : 7;
                };
            };
        } hotspot;
        struct {
            int total;
            int value;
            int max;
            int interval;
            int next;
            float step;
            int active;
            int showText;
        } anim;
    } u;
    union {
        short range;
        struct {
            unsigned char stages;
            unsigned char stageIndex;
        };
    };
    short status;
    union {
        struct {
            unsigned char quickKey;
            unsigned char frameBase;
            union {
                unsigned char grayedout;
                unsigned short flag;
                struct {
                    unsigned short flag_bit : 1;
                    unsigned short unknown_13c_1 : 15;
                };
                int max;
            };
        };
        GafEntry* inputGaf;
        struct {
            char unknown_13a_b[0x14e - 0x13a];
            GafEntry* sliderGaf;
            unsigned char sliderStyle;
        };
        struct {
            char unknown_13a_c[0x140 - 0x13a];
            short knobPos;
            short knobSize;
            void (__stdcall* sliderCallback)(Gui*, int);
            short unknown_148;
            int sliderUser;
        };
        struct {
            char unknown_13a_d[0x147 - 0x13a];
            unsigned char hotkey;
            unsigned int flag_148;
        };
    };
    char unknown_153[0x157 - 0x153];
    int sliderLocked;
};

#pragma pack(pop)

#endif
