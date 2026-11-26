#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../src/harness/view_block.h"

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

int main()
{
    TestWithinCapacity();
    TestEmptyRuns();
    TestPieces();
    TestOrders();
    TestFeatures();
    TestSight();
    TestFixedTables();
    if (s_failed == 0)
        printf("view_block_test: all passed\n");
    return s_failed;
}
