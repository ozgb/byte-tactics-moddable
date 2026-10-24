#include <math.h>
#include <stdlib.h>
#include "../state/game_rand.h"

#include "../graphics/gaf_frame.h"

#include "../graphics/gaf.h"

// FUNCTION: 0x420d20
GafFrame* BuildExplosionFrame(int size)
{
    double half = size / 2;
    GafFrame* img = AllocFrame("ExplosionFrame", size, size);
    img->yOffset = img->xOffset = (short)half;
    for (int y = 0; y < size; y++) {
        double dy = half - y;
        double dy2 = dy * dy * 1.33;
        for (int x = 0; x < size; x++) {
            double dx = half - x;
            double d = (double)(int)(GameRand(RAND_STREAM_LOAD) * (__int64)10 / 0x8000) + sqrt(dx * dx + dy2);
            unsigned char c = 32 - (unsigned char)(int)(d / half * 32.0);
            if (c >= 0x22)
                img->pixelsOrLayers[y * size + x] = 0xff;
            else if (c >= 0x20)
                img->pixelsOrLayers[y * size + x] = 0x6e;
            else
                img->pixelsOrLayers[y * size + x] = c + 0x4f;
        }
    }
    img->transparency = 0xff;
    return img;
}
