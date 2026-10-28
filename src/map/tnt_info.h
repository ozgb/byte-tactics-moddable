#ifndef TNT_INFO_H
#define TNT_INFO_H

#pragma pack(push, 1)
struct TntFeatureName {
    char unknown_0[4];
    char name[0x80];
};

struct TntInfo {
    int version;
    int width;
    int height;
    int flag;
    int sea_a;
    int sea_b;
    int sea_d;
    int featureCount;
    TntFeatureName* featureNames;
    int* tile_set_src;
    unsigned int tile_set_count;
    int* tile_map_src;
    unsigned char* attr_b;
    unsigned char* attr_a;
    int attr_limit;
    unsigned int feature_flags;
    unsigned short* feature_data;
};
#pragma pack(pop)

#endif
