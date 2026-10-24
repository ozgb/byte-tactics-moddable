#ifndef GAF_FRAME_H
#define GAF_FRAME_H

struct GafFrame {
    unsigned short width;
    unsigned short height;
    short xOffset;
    short yOffset;
    unsigned char transparency;
    unsigned char compressed;
    unsigned char layers;
    unsigned char blend;
    int reserved;
    unsigned char* pixelsOrLayers;
    unsigned char* scratch;
};

#endif
