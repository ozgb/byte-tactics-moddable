#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../src/harness/view_fill.h"

static PlayerView s_view;
static int s_failed = 0;

static void Check(int ok, const char* what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        s_failed = 1;
    }
}

static void TestWithinCapacity()
{
    ViewBegin(&s_view);
    ViewUnit* u = ViewAddUnit(&s_view);
    Check(u != 0 && ViewAddPieces(&s_view, u, 10) == 10, "pieces within capacity are kept");
    Check(ViewAddOrders(&s_view, u, 3) == 3 && u->firstOrder == 0, "orders within capacity are kept");
    Check(ViewAddFeature(&s_view) != 0, "a feature within capacity is kept");
    ViewSetGrid(&s_view, 1024, 1024);
    Check(ViewSightCells(&s_view) == VIEW_MAX_GRID_CELLS, "a full grid is all kept");
    Check(s_view.truncated == 0, "a fill within capacity sets no bit");
    unsigned expect = (unsigned)offsetof(PlayerView, players) + VIEW_MAX_PLAYERS * (unsigned)sizeof(ViewPlayer)
        + sizeof(ViewUnit) + 10 * sizeof(ViewPiece) + 3 * sizeof(ViewOrder) + sizeof(ViewFeature) + VIEW_MAX_GRID_CELLS;
    Check(ViewSerializedSize(&s_view) == expect, "the serialisation holds the header, players, used rows and sight");
}

static void TestEmptyRuns()
{
    ViewBegin(&s_view);
    ViewUnit* a = ViewAddUnit(&s_view);
    ViewUnit* b = ViewAddUnit(&s_view);
    Check(ViewAddPieces(&s_view, a, 4) == 4 && ViewAddOrders(&s_view, a, 2) == 2, "the first unit's runs fit");
    Check(ViewAddPieces(&s_view, b, 0) == 0 && b->firstPiece == 0, "a unit with no pieces has first piece 0");
    Check(ViewAddOrders(&s_view, b, 0) == 0 && b->firstOrder == 0, "a unit with no orders has first order 0");
    Check(s_view.truncated == 0, "an empty run sets no bit");
}

static void TestPieces()
{
    ViewBegin(&s_view);
    ViewUnit* a = ViewAddUnit(&s_view);
    ViewUnit* b = ViewAddUnit(&s_view);
    ViewUnit* c = ViewAddUnit(&s_view);
    ViewUnit* d = ViewAddUnit(&s_view);
    Check(ViewAddPieces(&s_view, a, VIEW_MAX_PIECES - 10) == VIEW_MAX_PIECES - 10, "the first unit's pieces fit");
    Check(ViewAddPieces(&s_view, b, 11) == 0 && b->pieceCount == 0 && b->firstPiece == 0, "the unit that does not fit has none");
    Check((s_view.truncated & VIEW_TRUNCATED_PIECES) != 0, "pieces set their bit");
    Check(ViewAddPieces(&s_view, c, 5) == 0 && c->pieceCount == 0, "a later unit that would fit has none");
    Check(ViewAddPieces(&s_view, d, 0) == 0, "a later unit has none");
    Check(s_view.pieceCount == VIEW_MAX_PIECES - 10 && a->pieceCount == VIEW_MAX_PIECES - 10, "the first unit keeps its pieces");
    Check((s_view.truncated & ~VIEW_TRUNCATED_PIECES) == 0, "only the pieces bit is set");
}

static void TestOrders()
{
    ViewBegin(&s_view);
    ViewUnit* a = ViewAddUnit(&s_view);
    ViewUnit* b = ViewAddUnit(&s_view);
    ViewUnit* c = ViewAddUnit(&s_view);
    Check(ViewAddOrders(&s_view, a, VIEW_MAX_ORDERS) == VIEW_MAX_ORDERS, "orders may fill the table");
    Check(s_view.truncated == 0, "a full table is not truncated");
    Check(ViewAddOrders(&s_view, b, 1) == 0 && b->orderCount == 0, "the next unit's order is dropped");
    Check((s_view.truncated & VIEW_TRUNCATED_ORDERS) != 0, "orders set their bit");
    Check(ViewAddOrders(&s_view, c, 0) == 0 && c->orderCount == 0, "a later unit has none");
    Check(s_view.orderCount == VIEW_MAX_ORDERS && a->orderCount == VIEW_MAX_ORDERS, "the first unit keeps its orders");
}

static void TestFeatures()
{
    ViewBegin(&s_view);
    for (unsigned i = 0; i < VIEW_MAX_FEATURES; i++) {
        ViewFeature* f = ViewAddFeature(&s_view);
        if (f == 0) {
            Check(0, "features fill the table");
            return;
        }
        f->cellX = (unsigned short)i;
        f->featureIndex = (unsigned short)(i >> 16);
    }
    Check(s_view.truncated == 0, "a full feature table is not truncated");
    Check(ViewAddFeature(&s_view) == 0 && ViewAddFeature(&s_view) == 0, "features past the capacity are dropped");
    Check((s_view.truncated & VIEW_TRUNCATED_FEATURES) != 0, "features set their bit");
    Check(s_view.featureCount == VIEW_MAX_FEATURES, "the first features are kept");
    Check(s_view.features[0].cellX == 0 && s_view.features[VIEW_MAX_FEATURES - 1].cellX == (unsigned short)(VIEW_MAX_FEATURES - 1),
          "the kept features are the first ones");
}

static void TestSight()
{
    ViewBegin(&s_view);
    ViewSetGrid(&s_view, 1025, 1024);
    Check((s_view.truncated & VIEW_TRUNCATED_SIGHT) != 0, "a larger grid sets the sight bit");
    Check(s_view.gridWidth == 1025 && s_view.gridHeight == 1024, "the grid keeps its size");
    Check(ViewSightCells(&s_view) == VIEW_MAX_GRID_CELLS, "the cells past the capacity are dropped");
    ViewBegin(&s_view);
    ViewSetGrid(&s_view, 0xffffffffu, 3);
    Check((s_view.truncated & VIEW_TRUNCATED_SIGHT) != 0 && ViewSightCells(&s_view) == VIEW_MAX_GRID_CELLS, "an overflowing grid is truncated");
    ViewBegin(&s_view);
    ViewSetGrid(&s_view, 64, 32);
    Check(s_view.truncated == 0 && ViewSightCells(&s_view) == 2048, "a small grid is whole");
}

static void TestFixedTables()
{
    ViewBegin(&s_view);
    for (unsigned i = 0; i < VIEW_MAX_PROJECTILES; i++)
        ViewAddProjectile(&s_view);
    Check(ViewAddProjectile(&s_view) == 0 && s_view.truncated == 0, "a full fixed table returns no row and sets no bit");
}

static unsigned char s_game[0x14900];
static unsigned char s_gafFile[0x40];
static unsigned char s_gafList[4];
static unsigned char s_entryA[0x28 + 16];
static unsigned char s_entryB[0x28 + 8];
static unsigned char s_logos[0x40];

const unsigned char* ViewHostGame(void) { return s_game; }
const unsigned char* ViewHostRef(unsigned) { return 0; }
void ViewHostUnitAccount(const unsigned char*, float out[4]) { memset(out, 0, 4 * sizeof(float)); }
int ViewHostOrderDrawMask(unsigned) { return 0; }
int ViewHostOrderPosYUnset(const unsigned char*) { return 0; }
const ViewWeaponsReport* ViewHostWeaponsReport(void) { return 0; }
unsigned ViewHostTakeEvents(const ViewRawEvent** events) { *events = 0; return 0; }
void ViewHostFail(const char*) {}

static unsigned Addr(const void* p)
{
    return (unsigned)(unsigned long)p;
}

static void Put32(unsigned char* base, unsigned offset, unsigned value)
{
    memcpy(base + offset, &value, 4);
}

static void Put16(unsigned char* base, unsigned offset, unsigned value)
{
    unsigned short v = (unsigned short)value;
    memcpy(base + offset, &v, 2);
}

static const unsigned PIC_METAL0 = 0x5000;
static const unsigned PIC_METAL1 = 0x5010;
static const unsigned PIC_ARM = 0x6000;

static void SetupGame()
{
    memset(s_game, 0, sizeof s_game);
    memset(s_gafFile, 0, sizeof s_gafFile);
    memset(s_entryA, 0, sizeof s_entryA);
    memset(s_entryB, 0, sizeof s_entryB);
    memset(s_logos, 0, sizeof s_logos);
    Put16(s_entryA, 0, 2);
    memcpy(s_entryA + 8, "metal3a", 8);
    Put32(s_entryA, 0x28, PIC_METAL0);
    Put32(s_entryA, 0x30, PIC_METAL1);
    Put16(s_entryB, 0, 1);
    memcpy(s_entryB + 8, "Arm01b", 7);
    Put32(s_entryB, 0x28, PIC_ARM);
    Put16(s_gafFile, 4, 2);
    Put32(s_gafFile, 0xc, Addr(s_entryA));
    Put32(s_gafFile, 0x10, Addr(s_entryB));
    Put32(s_gafList, 0, Addr(s_gafFile));
    Put32(s_game, 0x148e3, Addr(s_gafList));
    Put32(s_game, 0x148df, 1);
    Put32(s_game, 0x148d7, Addr(s_logos));
}

static const unsigned char VERTEX_ROW[96] = {
    0xbf, 0x14, 0xfc, 0xff, 0xb4, 0x2d, 0xfe, 0xff, 0x10, 0x8d, 0x13, 0x00, 0xd7, 0x05, 0xfe, 0xff,
    0x23, 0x37, 0x02, 0x00, 0xfd, 0x0f, 0x12, 0x00, 0xfa, 0xc7, 0x02, 0x00, 0x23, 0x37, 0x02, 0x00,
    0x33, 0x0b, 0xee, 0xff, 0xe6, 0x4a, 0x01, 0x00, 0xb4, 0x2d, 0xfe, 0xff, 0x1b, 0x1a, 0xec, 0xff,
    0x2b, 0x34, 0x02, 0x00, 0xdd, 0xc8, 0xfd, 0xff, 0xec, 0x38, 0xec, 0xff, 0x3f, 0xb1, 0x03, 0x00,
    0x4c, 0xd2, 0x01, 0x00, 0x04, 0x2a, 0xee, 0xff, 0x1c, 0xef, 0xfe, 0xff, 0x4c, 0xd2, 0x01, 0x00,
    0xce, 0x2e, 0x12, 0x00, 0x04, 0xfe, 0xfc, 0xff, 0xdd, 0xc8, 0xfd, 0xff, 0xe1, 0xab, 0x13, 0x00,
};

static const unsigned short INDEX_ROWS[6][4] = {{0, 1, 2, 3}, {2, 1, 6, 5}, {0, 3, 4, 7}, {1, 0, 7, 6}, {3, 2, 5, 4}, {4, 5, 6, 7}};

static void SetupPiece(ViewReportPiece* piece, int firstFace)
{
    memset(piece, 0, sizeof *piece);
    piece->used = 1;
    piece->vertexCount = 8;
    piece->faceCount = 6;
    piece->firstFace = firstFace;
    memcpy(piece->vertices, VERTEX_ROW, sizeof VERTEX_ROW);
    for (unsigned q = 0; q < 6; q++) {
        piece->faces[q].count = 4;
        piece->faces[q].indices = Addr(INDEX_ROWS[q]);
    }
    piece->faces[0].flags = 1;
    piece->faces[0].color = 0x3d000272;
    piece->faces[1].pic = PIC_METAL0;
    piece->faces[2].pic = PIC_METAL1;
    piece->faces[3].pic = PIC_ARM;
    piece->faces[4].pic = 0x9999;
    piece->faces[5].pic = PIC_METAL0;
}

static int TextureIs(const ViewDebrisFace* f, const char* name, unsigned frame)
{
    return f->kind == VIEW_FACE_TEXTURED && f->texture < s_view.textureCount
        && strcmp(s_view.textures[f->texture].name, name) == 0 && f->frame == frame;
}

static void ResetView()
{
    memset(&s_view, 0, sizeof s_view);
    ViewBegin(&s_view);
}

static void TestDebrisGeometry()
{
    static ViewReportPiece piece;
    ViewDebris a;
    ViewDebris b;
    int vertices[24];
    memset(&a, 0xff, sizeof a);
    memset(&b, 0, sizeof b);
    SetupGame();
    ResetView();
    SetupPiece(&piece, -1);
    memcpy(vertices, VERTEX_ROW, sizeof vertices);
    Check(ViewFillDebrisGeometry(&s_view, &a, &piece) == 1, "a debris piece that fits is filled");
    Check(a.firstVertex == 0 && a.vertexCount == 8 && a.firstFace == 0 && a.faceCount == 6, "the piece's runs start at 0");
    Check(s_view.truncated == 0, "a fitting piece sets no bit");
    int vertexOk = 1;
    for (unsigned q = 0; q < 8; q++) {
        vertexOk &= s_view.debrisVertices[q].x == vertices[q * 3] && s_view.debrisVertices[q].y == vertices[q * 3 + 1]
            && s_view.debrisVertices[q].z == vertices[q * 3 + 2];
    }
    Check(vertexOk, "the vertices are copied");
    Check(s_view.debrisVertices[0].x == -0x3eb41 && s_view.debrisVertices[7].z == 0x13abe1, "the vertex values are the recorded ones");
    const ViewDebrisFace* f = s_view.debrisFaces;
    Check(f[0].kind == VIEW_FACE_FILLED && f[0].colour == 0x72 && f[0].texture == VIEW_NO_TEXTURE, "a flat face is filled with its palette index");
    Check(TextureIs(&f[1], "metal3a", 0), "a face finds its texture by its picture");
    Check(TextureIs(&f[2], "metal3a", 1), "a second frame of the entry is found");
    Check(TextureIs(&f[3], "Arm01b", 0), "a face in another entry is found");
    Check(f[4].kind == VIEW_FACE_TEXTURED && f[4].texture == VIEW_NO_TEXTURE, "an unmatched picture is textured with no texture");
    Check(TextureIs(&f[5], "metal3a", 0) && f[5].texture == f[1].texture, "faces of one entry share a texture row");
    Check(s_view.textureCount == 2, "the textures are deduplicated");
    int indexOk = 1;
    for (unsigned q = 0; q < 6; q++) {
        indexOk &= f[q].indexCount == 4 && f[q].firstIndex == q * 4;
        for (unsigned k = 0; k < 4; k++)
            indexOk &= s_view.debrisIndices[q * 4 + k] == INDEX_ROWS[q][k];
    }
    Check(indexOk, "the indices are copied");
    Check(s_view.debrisVertexCount == 8 && s_view.debrisFaceCount == 6 && s_view.debrisIndexCount == 24, "the counts cover the piece");

    Check(ViewFillDebrisGeometry(&s_view, &b, &piece) == 1, "a second piece is filled");
    Check(b.firstVertex == 8 && b.vertexCount == 8 && b.firstFace == 6 && b.faceCount == 6, "the second piece follows the first");
    Check(s_view.debrisFaces[6].firstIndex == 24 && s_view.textureCount == 2, "the second piece's indices follow and its textures are reused");
    Check(TextureIs(&s_view.debrisFaces[9], "Arm01b", 0), "the second piece's texture is found");
}

static void TestDebrisFirstFace()
{
    static ViewReportPiece piece;
    ViewDebris out;
    memset(&out, 0, sizeof out);
    SetupGame();
    ResetView();
    SetupPiece(&piece, 0);
    Check(ViewFillDebrisGeometry(&s_view, &out, &piece) == 1, "a piece with a first face is filled");
    Check(out.vertexCount == 8 && out.faceCount == 5, "face 0 is skipped");
    Check(TextureIs(&s_view.debrisFaces[0], "metal3a", 0), "the first kept face is face 1");
}

static void TestExplodedGeometry()
{
    static unsigned char desc[0x40];
    static unsigned char vertices[3 * 12];
    static unsigned char faces[4 * 32];
    ViewExplodedPiece out;
    memset(&out, 0, sizeof out);
    memset(desc, 0, sizeof desc);
    memset(faces, 0, sizeof faces);
    SetupGame();
    ResetView();
    for (unsigned q = 0; q < 9; q++)
        Put32(vertices, q * 4, (q + 1) * 0x10000);
    Put32(desc, 4, 3);
    Put32(desc, 8, 4);
    Put32(desc, 0xc, (unsigned)-1);
    Put32(desc, 0x24, Addr(vertices));
    Put32(desc, 0x28, Addr(faces));
    for (unsigned q = 0; q < 4; q++) {
        unsigned char* f = faces + q * 32;
        Put32(f, 4, 4);
        Put32(f, 0xc, Addr(INDEX_ROWS[q]));
        Put32(f, 0x18, Addr(s_entryA));
    }
    Put32(faces, 0x10, 0x1234);
    Put32(faces, 0x1c, 6);
    Put32(faces + 32, 0x10, 0xabcd0003);
    Put32(faces + 32, 0x1c, 2);
    Put32(faces + 64, 4, 3);
    Put32(faces + 64, 0x1c, 0);
    Put32(faces + 96, 0x10, 0x0007);
    Put32(faces + 96, 0x1c, 6);
    Check(ViewFillExplodedGeometry(&s_view, &out, desc, 5) == 1, "an exploded piece is filled");
    Check(out.firstVertex == 0 && out.vertexCount == 3 && out.firstFace == 0 && out.faceCount == 4, "the exploded piece's runs are set");
    Check(s_view.debrisVertices[2].x == 7 * 0x10000 && s_view.debrisVertices[2].z == 9 * 0x10000, "the exploded vertices are copied");
    const ViewDebrisFace* f = s_view.debrisFaces;
    Check(TextureIs(&f[0], "metal3a", 5), "a team-colour face takes the team colour as its frame");
    Check(TextureIs(&f[1], "metal3a", 3), "a sequence face takes the low half of its picture as its frame");
    Check(f[2].kind == VIEW_FACE_NONE && f[2].indexCount == 3, "a three-index face draws nothing");
    Check(s_view.debrisIndices[f[1].firstIndex + 1] == 1 && s_view.debrisIndices[f[1].firstIndex + 3] == 5, "the exploded indices are copied");
}

static void TestGeometryOverflow()
{
    static ViewReportPiece piece;
    ViewDebris out;
    ViewDebris later;
    memset(&out, 0xff, sizeof out);
    memset(&later, 0xff, sizeof later);
    SetupGame();
    ResetView();
    SetupPiece(&piece, -1);
    s_view.debrisVertexCount = VIEW_MAX_DEBRIS_VERTICES - 4;
    Check(ViewFillDebrisGeometry(&s_view, &out, &piece) == 0, "a piece that does not fit fails");
    Check((s_view.truncated & VIEW_TRUNCATED_DEBRIS) != 0, "the debris bit is set");
    Check(s_view.debrisVertexCount == VIEW_MAX_DEBRIS_VERTICES - 4 && s_view.debrisFaceCount == 0 && s_view.debrisIndexCount == 0
              && s_view.textureCount == 0,
          "the counts are rolled back");
    Check(out.firstVertex == 0 && out.vertexCount == 0 && out.firstFace == 0 && out.faceCount == 0, "the piece's runs are zeroed");
    piece.vertexCount = 2;
    piece.faceCount = 1;
    Check(ViewFillDebrisGeometry(&s_view, &later, &piece) == 0, "a later piece is not added");
    Check(s_view.debrisVertexCount == VIEW_MAX_DEBRIS_VERTICES - 4 && s_view.debrisFaceCount == 0, "a later piece leaves the counts");
}

int main()
{
    TestWithinCapacity();
    TestEmptyRuns();
    TestPieces();
    TestOrders();
    TestFeatures();
    TestSight();
    TestFixedTables();
    TestDebrisGeometry();
    TestDebrisFirstFace();
    TestExplodedGeometry();
    TestGeometryOverflow();
    if (s_failed == 0)
        printf("view_block_test: all passed\n");
    return s_failed;
}
