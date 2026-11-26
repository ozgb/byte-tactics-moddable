#ifndef VIEW_FILL_H
#define VIEW_FILL_H

#include "view_block.h"

#define VIEW_REPORT_PROJECTILES 300
#define VIEW_REPORT_EXPLODED 100
#define VIEW_REPORT_PIECES 300
#define VIEW_REPORT_DEBRIS 300
#define VIEW_REPORT_EVENT_ARGS 12
#define VIEW_REPORT_PIECE_VERTICES 8
#define VIEW_REPORT_PIECE_FACES 6

struct ViewReportProjectile {
    int startX;
    int startY;
    int startZ;
    short bank;
    short heading;
    short pitch;
    short propellerSpin;
    short radius;
    short active;
    unsigned spawnTick;
    unsigned time;
};

struct ViewReportExploded {
    unsigned used;
    unsigned unit;
    unsigned desc;
    int index;
    unsigned flags;
    short angle10;
    short angle12;
    short angle14;
    int x;
    int y;
    int z;
};

struct ViewReportDebris {
    unsigned used;
    unsigned obj;
    unsigned short frame;
    unsigned short shadowFrame;
    unsigned spriteSource;
    unsigned shadowSource;
    int x;
    int y;
    int z;
    short angleX;
    short angleY;
    short angleZ;
};

struct ViewReportFace {
    int color;
    int count;
    unsigned indices;
    unsigned pic;
    unsigned src;
    unsigned flags;
};

struct ViewReportPiece {
    unsigned used;
    int vertexCount;
    int firstFace;
    int faceCount;
    int vertices[VIEW_REPORT_PIECE_VERTICES * 3];
    ViewReportFace faces[VIEW_REPORT_PIECE_FACES];
};

struct ViewWeaponsReport {
    ViewReportProjectile projectiles[VIEW_REPORT_PROJECTILES];
    unsigned projectileUsed[VIEW_REPORT_PROJECTILES];
    ViewReportExploded exploded[VIEW_REPORT_EXPLODED];
    ViewReportDebris debris[VIEW_REPORT_DEBRIS];
    ViewReportPiece pieces[VIEW_REPORT_PIECES];
};

struct ViewRawEvent {
    unsigned short kind;
    unsigned short flags;
    unsigned count;
    unsigned args[VIEW_REPORT_EVENT_ARGS];
};

const unsigned char* ViewHostGame(void);
const unsigned char* ViewHostRef(unsigned stored);
void ViewHostUnitAccount(const unsigned char* unit, float out[4]);
int ViewHostOrderDrawMask(unsigned kind);
int ViewHostOrderPosYUnset(const unsigned char* order);
const ViewWeaponsReport* ViewHostWeaponsReport(void);
unsigned ViewHostTakeEvents(const ViewRawEvent** events);
void ViewHostFail(const char* message);

int ViewIsMessageKind(unsigned kind);

int ViewFillDebrisGeometry(PlayerView* v, ViewDebris* out, const ViewReportPiece* piece);
int ViewFillExplodedGeometry(PlayerView* v, ViewExplodedPiece* out, const unsigned char* desc, int teamColour);

void ViewFill(PlayerView* v);

#endif
