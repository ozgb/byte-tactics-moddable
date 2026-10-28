#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <memory>
#include "../state/block.h"
#include "../state/sim_allocator.h"
#include "win_types.h"
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#pragma pack(push, 1)
#include "../../include/ta_types.h"

static_assert(sizeof(CobChannelView) == 164, "");
static_assert(offsetof(CobChannelView, used) == 0, "");
static_assert(offsetof(CobChannelView, id) == 4, "");
static_assert(offsetof(CobChannelView, unknown_8) == 8, "");
static_assert(offsetof(CobChannelView, value) == 32, "");
static_assert(offsetof(CobChannelView, unknown_24) == 36, "");
static_assert(sizeof(CobScriptView) == 1344, "");
static_assert(offsetof(CobScriptView, unknown_4) == 4, "");
static_assert(offsetof(CobScriptView, table) == 8, "");
static_assert(offsetof(CobScriptView, unknown_c) == 12, "");
static_assert(offsetof(CobScriptView, channels) == 28, "");
static_assert(offsetof(CobScriptView, activeCount) == 1340, "");
static_assert(sizeof(Handle) == 12, "");
static_assert(offsetof(Handle, index) == 0, "");
static_assert(offsetof(Handle, value) == 2, "");
static_assert(offsetof(Handle, kind) == 4, "");
static_assert(offsetof(Handle, unknown_5) == 5, "");
static_assert(offsetof(Handle, src) == 8, "");
static_assert(sizeof(Object3doView) == 52, "");
static_assert(offsetof(Object3doView, unknown_0) == 0, "");
static_assert(offsetof(Object3doView, unknown_4) == 4, "");
static_assert(offsetof(Object3doView, unknown_8) == 8, "");
static_assert(offsetof(Object3doView, unknown_c) == 12, "");
static_assert(offsetof(Object3doView, x) == 16, "");
static_assert(offsetof(Object3doView, y) == 20, "");
static_assert(offsetof(Object3doView, z) == 24, "");
static_assert(offsetof(Object3doView, unknown_1c) == 28, "");
static_assert(offsetof(Object3doView, unknown_20) == 32, "");
static_assert(offsetof(Object3doView, points) == 36, "");
static_assert(offsetof(Object3doView, unknown_28) == 40, "");
static_assert(offsetof(Object3doView, unknown_2c) == 44, "");
static_assert(offsetof(Object3doView, unknown_30) == 48, "");
static_assert(sizeof(ScriptNameTableView) == 32, "");
static_assert(offsetof(ScriptNameTableView, unknown_0) == 0, "");
static_assert(offsetof(ScriptNameTableView, count) == 4, "");
static_assert(offsetof(ScriptNameTableView, unknown_8) == 8, "");
static_assert(offsetof(ScriptNameTableView, names) == 28, "");
static_assert(sizeof(SpatialBucketView) == 10, "");
static_assert(offsetof(SpatialBucketView, a) == 0, "");
static_assert(offsetof(SpatialBucketView, b) == 1, "");
static_assert(offsetof(SpatialBucketView, flags) == 2, "");
static_assert(offsetof(SpatialBucketView, head) == 6, "");
static_assert(sizeof(SpotBitmap) == 25, "");
static_assert(offsetof(SpotBitmap, width) == 0, "");
static_assert(offsetof(SpotBitmap, height) == 2, "");
static_assert(offsetof(SpotBitmap, dx) == 4, "");
static_assert(offsetof(SpotBitmap, dy) == 6, "");
static_assert(offsetof(SpotBitmap, colour) == 8, "");
static_assert(offsetof(SpotBitmap, flag9) == 9, "");
static_assert(offsetof(SpotBitmap, count) == 10, "");
static_assert(offsetof(SpotBitmap, kind) == 11, "");
static_assert(offsetof(SpotBitmap, unknown_c) == 12, "");
static_assert(offsetof(SpotBitmap, data) == 16, "");
static_assert(offsetof(SpotBitmap, depth) == 20, "");
static_assert(offsetof(SpotBitmap, pixels) == 24, "");
