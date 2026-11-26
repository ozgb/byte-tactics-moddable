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

int ViewGeometryBegin(PlayerView* v, ViewGeometryMark* mark)
{
    mark->vertices = v->debrisVertexCount;
    mark->faces = v->debrisFaceCount;
    mark->indices = v->debrisIndexCount;
    mark->textures = v->textureCount;
    return (v->truncated & VIEW_TRUNCATED_DEBRIS) == 0;
}

ViewDebrisVertex* ViewGeometryVertices(PlayerView* v, unsigned count)
{
    ViewDebrisVertex* rows;
    if (count > VIEW_MAX_DEBRIS_VERTICES - v->debrisVertexCount)
        return 0;
    rows = &v->debrisVertices[v->debrisVertexCount];
    memset(rows, 0, count * sizeof(ViewDebrisVertex));
    v->debrisVertexCount += count;
    return rows;
}

ViewDebrisFace* ViewGeometryFace(PlayerView* v)
{
    return (ViewDebrisFace*)AddRow(v->debrisFaces, &v->debrisFaceCount, VIEW_MAX_DEBRIS_FACES, sizeof(ViewDebrisFace));
}

unsigned short* ViewGeometryIndices(PlayerView* v, ViewDebrisFace* face, unsigned count)
{
    unsigned short* rows;
    if (count > 0xffff || count > VIEW_MAX_DEBRIS_INDICES - v->debrisIndexCount)
        return 0;
    rows = &v->debrisIndices[v->debrisIndexCount];
    memset(rows, 0, count * sizeof(unsigned short));
    face->firstIndex = count != 0 ? v->debrisIndexCount : 0;
    face->indexCount = (unsigned short)count;
    v->debrisIndexCount += count;
    return rows;
}

unsigned ViewGeometryTexture(PlayerView* v, const char* name)
{
    ViewTexture key;
    unsigned i;
    memset(&key, 0, sizeof key);
    for (i = 0; i < VIEW_TEXTURE_NAME && name[i] != 0; i++)
        key.name[i] = name[i];
    for (i = 0; i < v->textureCount; i++) {
        if (memcmp(v->textures[i].name, key.name, VIEW_TEXTURE_NAME) == 0)
            return i;
    }
    if (v->textureCount >= VIEW_MAX_TEXTURES)
        return VIEW_NO_TEXTURE;
    v->textures[v->textureCount] = key;
    return v->textureCount++;
}

void ViewGeometryEnd(PlayerView* v, const ViewGeometryMark* mark, unsigned* firstVertex, unsigned* vertexCount,
                     unsigned* firstFace, unsigned* faceCount)
{
    *vertexCount = v->debrisVertexCount - mark->vertices;
    *firstVertex = *vertexCount != 0 ? mark->vertices : 0;
    *faceCount = v->debrisFaceCount - mark->faces;
    *firstFace = *faceCount != 0 ? mark->faces : 0;
}

void ViewGeometryFail(PlayerView* v, const ViewGeometryMark* mark, unsigned* firstVertex, unsigned* vertexCount,
                      unsigned* firstFace, unsigned* faceCount)
{
    v->debrisVertexCount = mark->vertices;
    v->debrisFaceCount = mark->faces;
    v->debrisIndexCount = mark->indices;
    v->textureCount = mark->textures;
    v->truncated = (unsigned short)(v->truncated | VIEW_TRUNCATED_DEBRIS);
    *firstVertex = 0;
    *vertexCount = 0;
    *firstFace = 0;
    *faceCount = 0;
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
        + v->debrisVertexCount * (unsigned)sizeof(ViewDebrisVertex) + v->debrisFaceCount * (unsigned)sizeof(ViewDebrisFace)
        + v->debrisIndexCount * (unsigned)sizeof(unsigned short) + v->textureCount * (unsigned)sizeof(ViewTexture)
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
    at = Put(at, v->debrisVertices, v->debrisVertexCount * (unsigned)sizeof(ViewDebrisVertex));
    at = Put(at, v->debrisFaces, v->debrisFaceCount * (unsigned)sizeof(ViewDebrisFace));
    at = Put(at, v->debrisIndices, v->debrisIndexCount * (unsigned)sizeof(unsigned short));
    at = Put(at, v->textures, v->textureCount * (unsigned)sizeof(ViewTexture));
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
