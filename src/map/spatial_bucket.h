#ifndef SPATIAL_BUCKET_H
#define SPATIAL_BUCKET_H
#include "../../interfaces/ref.h"

struct Unit;

class SpatialBucket {
public:
    char unknown_0[6];
    Ref<Unit> head;
    void UnlinkUnit(Unit* node);
    void PrependUnit(Unit* node);
};

#endif
