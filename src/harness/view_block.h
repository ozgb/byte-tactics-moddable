#ifndef VIEW_BLOCK_H
#define VIEW_BLOCK_H

#include "../../interfaces/view.h"

typedef char ViewSizePlayer[sizeof(ViewPlayer) == 104 ? 1 : -1];
typedef char ViewSizeUnit[sizeof(ViewUnit) == 80 ? 1 : -1];
typedef char ViewSizePiece[sizeof(ViewPiece) == 20 ? 1 : -1];
typedef char ViewSizeOrder[sizeof(ViewOrder) == 24 ? 1 : -1];
typedef char ViewSizeBlip[sizeof(ViewBlip) == 16 ? 1 : -1];
typedef char ViewSizeFeature[sizeof(ViewFeature) == 32 ? 1 : -1];
typedef char ViewSizeProjectile[sizeof(ViewProjectile) == 48 ? 1 : -1];
typedef char ViewSizeDebris[sizeof(ViewDebris) == 36 ? 1 : -1];
typedef char ViewSizeExploded[sizeof(ViewExplodedPiece) == 28 ? 1 : -1];
typedef char ViewSizeMessage[sizeof(ViewMessage) == 28 ? 1 : -1];
typedef char ViewSizeView[sizeof(PlayerView) == 7963944 ? 1 : -1];

#define VIEW_FILE_HEADER_BYTES 8

// Clears the header and the player rows and empties every table; the rows of a table are cleared as they are added.
void ViewBegin(PlayerView* v);

// Each adds a zeroed row and returns it, or returns 0 when the table is full (the fixed-capacity tables).
ViewUnit* ViewAddUnit(PlayerView* v);
ViewBlip* ViewAddBlip(PlayerView* v);
ViewProjectile* ViewAddProjectile(PlayerView* v);
ViewDebris* ViewAddDebris(PlayerView* v);
ViewExplodedPiece* ViewAddExplodedPiece(PlayerView* v);
ViewMessage* ViewAddMessage(PlayerView* v);

// Adds a zeroed feature, or returns 0 and sets VIEW_TRUNCATED_FEATURES when the table is full.
ViewFeature* ViewAddFeature(PlayerView* v);

// Gives a unit count zeroed pieces (or orders) and sets its first and count; returns the count it was given, which is 0
// when they do not fit or an earlier unit's did not (VIEW_TRUNCATED_PIECES, VIEW_TRUNCATED_ORDERS).
unsigned ViewAddPieces(PlayerView* v, ViewUnit* unit, unsigned count);
unsigned ViewAddOrders(PlayerView* v, ViewUnit* unit, unsigned count);

// Sets the sight grid's size, and VIEW_TRUNCATED_SIGHT when it has more cells than the block holds.
void ViewSetGrid(PlayerView* v, unsigned width, unsigned height);
unsigned ViewSightCells(const PlayerView* v);

// The bytes of the serialisation of the filled block, and the serialisation itself into out.
unsigned ViewSerializedSize(const PlayerView* v);
unsigned ViewSerialize(const PlayerView* v, unsigned char* out);

// Writes the file of the block: BTVW, version 1, 0, then the serialisation. Returns 1 when it was written whole.
int ViewWriteFile(const char* path, const unsigned char* serialised, unsigned size);

#endif
