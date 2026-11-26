#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../../interfaces/view.h"
#include "view_block.h"

static unsigned char* AddRow(void* table, unsigned* count, unsigned capacity, unsigned stride)
{
    unsigned char* row;
    if (*count >= capacity)
        return 0;
    row = (unsigned char*)table + *count * stride;
    memset(row, 0, stride);
    (*count)++;
    return row;
}

static unsigned AddRun(void* table, unsigned* count, unsigned capacity, unsigned stride, unsigned n,
                       unsigned short* truncated, unsigned short bit, unsigned* first)
{
    if ((*truncated & bit) != 0 || n > capacity - *count) {
        *truncated = (unsigned short)(*truncated | bit);
        *first = 0;
        return 0;
    }
    *first = n != 0 ? *count : 0;
    memset((unsigned char*)table + *count * stride, 0, n * stride);
    *count += n;
    return n;
}

void ViewBegin(PlayerView* v)
{
    memset(v, 0, offsetof(PlayerView, units));
}

ViewUnit* ViewAddUnit(PlayerView* v)
{
    return (ViewUnit*)AddRow(v->units, &v->unitCount, VIEW_MAX_UNITS, sizeof(ViewUnit));
}

ViewBlip* ViewAddBlip(PlayerView* v)
{
    return (ViewBlip*)AddRow(v->blips, &v->blipCount, VIEW_MAX_BLIPS, sizeof(ViewBlip));
}

ViewProjectile* ViewAddProjectile(PlayerView* v)
{
    return (ViewProjectile*)AddRow(v->projectiles, &v->projectileCount, VIEW_MAX_PROJECTILES, sizeof(ViewProjectile));
}

ViewDebris* ViewAddDebris(PlayerView* v)
{
    return (ViewDebris*)AddRow(v->debris, &v->debrisCount, VIEW_MAX_DEBRIS, sizeof(ViewDebris));
}

ViewExplodedPiece* ViewAddExplodedPiece(PlayerView* v)
{
    return (ViewExplodedPiece*)AddRow(v->explodedPieces, &v->explodedPieceCount, VIEW_MAX_EXPLODED_PIECES,
                                      sizeof(ViewExplodedPiece));
}

ViewMessage* ViewAddMessage(PlayerView* v)
{
    return (ViewMessage*)AddRow(v->messages, &v->messageCount, VIEW_MAX_MESSAGES, sizeof(ViewMessage));
}

ViewFeature* ViewAddFeature(PlayerView* v)
{
    ViewFeature* f = (ViewFeature*)AddRow(v->features, &v->featureCount, VIEW_MAX_FEATURES, sizeof(ViewFeature));
    if (f == 0)
        v->truncated = (unsigned short)(v->truncated | VIEW_TRUNCATED_FEATURES);
    return f;
}

unsigned ViewAddPieces(PlayerView* v, ViewUnit* unit, unsigned count)
{
    unit->pieceCount = AddRun(v->pieces, &v->pieceCount, VIEW_MAX_PIECES, sizeof(ViewPiece), count, &v->truncated,
                              VIEW_TRUNCATED_PIECES, &unit->firstPiece);
    return unit->pieceCount;
}

unsigned ViewAddOrders(PlayerView* v, ViewUnit* unit, unsigned count)
{
    unit->orderCount = AddRun(v->orders, &v->orderCount, VIEW_MAX_ORDERS, sizeof(ViewOrder), count, &v->truncated,
                              VIEW_TRUNCATED_ORDERS, &unit->firstOrder);
    return unit->orderCount;
}

unsigned ViewSightCells(const PlayerView* v)
{
    unsigned w = v->gridWidth;
    unsigned h = v->gridHeight;
    if (w == 0 || h == 0)
        return 0;
    if (w > VIEW_MAX_GRID_CELLS / h || w * h > VIEW_MAX_GRID_CELLS)
        return VIEW_MAX_GRID_CELLS;
    return w * h;
}

void ViewSetGrid(PlayerView* v, unsigned width, unsigned height)
{
    v->gridWidth = width;
    v->gridHeight = height;
    if (height != 0 && (width > VIEW_MAX_GRID_CELLS / height || width * height > VIEW_MAX_GRID_CELLS))
        v->truncated = (unsigned short)(v->truncated | VIEW_TRUNCATED_SIGHT);
}

unsigned ViewSerializedSize(const PlayerView* v)
{
    return (unsigned)offsetof(PlayerView, players) + VIEW_MAX_PLAYERS * (unsigned)sizeof(ViewPlayer)
        + v->unitCount * (unsigned)sizeof(ViewUnit) + v->pieceCount * (unsigned)sizeof(ViewPiece)
        + v->orderCount * (unsigned)sizeof(ViewOrder) + v->blipCount * (unsigned)sizeof(ViewBlip)
        + v->featureCount * (unsigned)sizeof(ViewFeature) + v->projectileCount * (unsigned)sizeof(ViewProjectile)
        + v->debrisCount * (unsigned)sizeof(ViewDebris) + v->explodedPieceCount * (unsigned)sizeof(ViewExplodedPiece)
        + v->messageCount * (unsigned)sizeof(ViewMessage) + ViewSightCells(v);
}

static unsigned char* Put(unsigned char* out, const void* from, unsigned bytes)
{
    memcpy(out, from, bytes);
    return out + bytes;
}

unsigned ViewSerialize(const PlayerView* v, unsigned char* out)
{
    unsigned char* at = out;
    at = Put(at, v, (unsigned)offsetof(PlayerView, players));
    at = Put(at, v->players, (unsigned)sizeof v->players);
    at = Put(at, v->units, v->unitCount * (unsigned)sizeof(ViewUnit));
    at = Put(at, v->pieces, v->pieceCount * (unsigned)sizeof(ViewPiece));
    at = Put(at, v->orders, v->orderCount * (unsigned)sizeof(ViewOrder));
    at = Put(at, v->blips, v->blipCount * (unsigned)sizeof(ViewBlip));
    at = Put(at, v->features, v->featureCount * (unsigned)sizeof(ViewFeature));
    at = Put(at, v->projectiles, v->projectileCount * (unsigned)sizeof(ViewProjectile));
    at = Put(at, v->debris, v->debrisCount * (unsigned)sizeof(ViewDebris));
    at = Put(at, v->explodedPieces, v->explodedPieceCount * (unsigned)sizeof(ViewExplodedPiece));
    at = Put(at, v->messages, v->messageCount * (unsigned)sizeof(ViewMessage));
    at = Put(at, v->sight, ViewSightCells(v));
    return (unsigned)(at - out);
}

int ViewWriteFile(const char* path, const unsigned char* serialised, unsigned size)
{
    static const unsigned char header[VIEW_FILE_HEADER_BYTES] = { 'B', 'T', 'V', 'W', 1, 0, 0, 0 };
    FILE* f = fopen(path, "wb");
    int ok;
    if (f == 0)
        return 0;
    ok = fwrite(header, sizeof header, 1, f) == 1 && fwrite(serialised, 1, size, f) == size;
    if (fclose(f) != 0)
        ok = 0;
    return ok;
}
