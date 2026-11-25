#ifndef SPATIAL_BUCKET_H
#define SPATIAL_BUCKET_H
#include "../../interfaces/ref.h"

struct Unit;

class SpatialBucket {
public:
    char bytes[6];
    Ref<Unit> head;
    void UnlinkUnit(Unit* node);
    void PrependUnit(Unit* node);
};

#endif
