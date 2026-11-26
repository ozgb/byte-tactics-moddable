#ifndef VIEW_BLOCK_H
#define VIEW_BLOCK_H

#include "../../interfaces/view.h"

typedef char ViewSizePlayer[sizeof(ViewPlayer) == 104 ? 1 : -1];
typedef char ViewSizeUnit[sizeof(ViewUnit) == 84 ? 1 : -1];
typedef char ViewSizePiece[sizeof(ViewPiece) == 20 ? 1 : -1];
typedef char ViewSizeOrder[sizeof(ViewOrder) == 24 ? 1 : -1];
typedef char ViewSizeBlip[sizeof(ViewBlip) == 16 ? 1 : -1];
typedef char ViewSizeFeature[sizeof(ViewFeature) == 32 ? 1 : -1];
typedef char ViewSizeProjectile[sizeof(ViewProjectile) == 48 ? 1 : -1];
typedef char ViewSizeDebris[sizeof(ViewDebris) == 44 ? 1 : -1];
typedef char ViewSizeExploded[sizeof(ViewExplodedPiece) == 40 ? 1 : -1];
typedef char ViewSizeMessage[sizeof(ViewMessage) == 28 ? 1 : -1];
typedef char ViewSizeDebrisVertex[sizeof(ViewDebrisVertex) == 12 ? 1 : -1];
typedef char ViewSizeDebrisFace[sizeof(ViewDebrisFace) == 16 ? 1 : -1];
typedef char ViewSizeTexture[sizeof(ViewTexture) == 32 ? 1 : -1];
typedef char ViewSizeView[sizeof(PlayerView) == 8542344 ? 1 : -1];

#define VIEW_FILE_HEADER_BYTES 8

void ViewBegin(PlayerView* v);

ViewUnit* ViewAddUnit(PlayerView* v);
ViewBlip* ViewAddBlip(PlayerView* v);
ViewProjectile* ViewAddProjectile(PlayerView* v);
ViewDebris* ViewAddDebris(PlayerView* v);
ViewExplodedPiece* ViewAddExplodedPiece(PlayerView* v);
ViewMessage* ViewAddMessage(PlayerView* v);

ViewFeature* ViewAddFeature(PlayerView* v);

unsigned ViewAddPieces(PlayerView* v, ViewUnit* unit, unsigned count);
unsigned ViewAddOrders(PlayerView* v, ViewUnit* unit, unsigned count);

#define VIEW_NO_TEXTURE 0xffff

struct ViewGeometryMark {
    unsigned vertices;
    unsigned faces;
    unsigned indices;
    unsigned textures;
};

int ViewGeometryBegin(PlayerView* v, ViewGeometryMark* mark);
ViewDebrisVertex* ViewGeometryVertices(PlayerView* v, unsigned count);
ViewDebrisFace* ViewGeometryFace(PlayerView* v);
unsigned short* ViewGeometryIndices(PlayerView* v, ViewDebrisFace* face, unsigned count);
unsigned ViewGeometryTexture(PlayerView* v, const char* name);
void ViewGeometryEnd(PlayerView* v, const ViewGeometryMark* mark, unsigned* firstVertex, unsigned* vertexCount,
                     unsigned* firstFace, unsigned* faceCount);
void ViewGeometryFail(PlayerView* v, const ViewGeometryMark* mark, unsigned* firstVertex, unsigned* vertexCount,
                      unsigned* firstFace, unsigned* faceCount);

void ViewSetGrid(PlayerView* v, unsigned width, unsigned height);
unsigned ViewSightCells(const PlayerView* v);

unsigned ViewSerializedSize(const PlayerView* v);
unsigned ViewSerialize(const PlayerView* v, unsigned char* out);

int ViewWriteFile(const char* path, const unsigned char* serialised, unsigned size);

#endif
