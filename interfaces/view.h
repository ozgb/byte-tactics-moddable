#ifndef INTERFACES_VIEW_H
#define INTERFACES_VIEW_H

// Major version of the view, changed when a field moves, changes size or goes away.
#define VIEW_VERSION_MAJOR 2
// Minor version of the view, changed when a field is added in space the block left unused.
#define VIEW_VERSION_MINOR 0

// The players a match can have; index 10, no player, never has a row.
#define VIEW_MAX_PLAYERS 10
// The units of the largest match: 500 a player and the pool's spare entry.
#define VIEW_MAX_UNITS 5001
// The posed model pieces of every unit in the view; past it, VIEW_TRUNCATED_PIECES.
#define VIEW_MAX_PIECES 131072
// The queued orders of the controller's units; past it, VIEW_TRUNCATED_ORDERS.
#define VIEW_MAX_ORDERS 65536
// The radar and sonar blips: at most one a unit.
#define VIEW_MAX_BLIPS 5001
// The features on the map; past it, VIEW_TRUNCATED_FEATURES.
#define VIEW_MAX_FEATURES 65536
// The projectiles in flight, as many as the simulation keeps.
#define VIEW_MAX_PROJECTILES 300
// The debris records, as many as the simulation keeps.
#define VIEW_MAX_DEBRIS 300
// The exploded model pieces in flight, as many as the simulation keeps.
#define VIEW_MAX_EXPLODED_PIECES 100
// The vertices of the debris and exploded pieces: 8 a debris slot, and one per 12 bytes of the debris model cache.
#define VIEW_MAX_DEBRIS_VERTICES 10733
// The faces of the debris and exploded pieces; past it, VIEW_TRUNCATED_DEBRIS.
#define VIEW_MAX_DEBRIS_FACES 16384
// The vertex indices of those faces; past it, VIEW_TRUNCATED_DEBRIS.
#define VIEW_MAX_DEBRIS_INDICES 65536
// The textures those faces name; past it, VIEW_TRUNCATED_DEBRIS.
#define VIEW_MAX_TEXTURES 1024
// The bytes of a texture's name, the null padding included.
#define VIEW_TEXTURE_NAME 32
// The sim events of one tick that reach the viewer: the sim event queue's capacity.
#define VIEW_MAX_MESSAGES 4096
// The cells of the sight grid, at half the map's cell resolution; past it, VIEW_TRUNCATED_SIGHT.
#define VIEW_MAX_GRID_CELLS 1048576

// What the viewer knows of a cell of the sight grid.
enum ViewSight {
    // Never revealed to the viewer: drawn black.
    VIEW_SIGHT_HIDDEN = 0,
    // Revealed once, out of sight now: drawn grey when line of sight is on.
    VIEW_SIGHT_MAPPED = 1,
    // In the viewer's sight now.
    VIEW_SIGHT_IN_SIGHT = 2,
};

// Bits of ViewUnit.relation.
enum ViewRelation {
    // The unit is the viewer's own.
    VIEW_RELATION_OWN = 1,
    // The unit's owner is allied to the viewer.
    VIEW_RELATION_ALLIED = 2,
    // The unit belongs to the controller, so the controller's input may select it.
    VIEW_RELATION_CONTROLLED = 4,
    // The controller's input may select the unit now: it is selectable, finished, not just given away and not carried.
    VIEW_RELATION_SELECTABLE = 8,
};

// Bits of ViewBlip.flags.
enum ViewBlipFlags {
    // Detected by radar or by sight above water.
    VIEW_BLIP_RADAR = 1,
    // Detected by sonar, under water.
    VIEW_BLIP_SONAR = 2,
    // The unit is the viewer's own.
    VIEW_BLIP_OWN = 4,
    // The unit was damaged lately: the blip flashes.
    VIEW_BLIP_DAMAGED = 8,
};

// Bits of PlayerView.truncated: a table whose entries did not all fit its capacity.
enum ViewTruncated {
    // A unit's pieces did not fit: it and every later unit have pieceCount 0.
    VIEW_TRUNCATED_PIECES = 1,
    // A unit's orders did not fit: it and every later unit have orderCount 0.
    VIEW_TRUNCATED_ORDERS = 2,
    // The features past VIEW_MAX_FEATURES, in the order of their anchor cells, are dropped.
    VIEW_TRUNCATED_FEATURES = 4,
    // The sight grid has more cells than VIEW_MAX_GRID_CELLS: the cells past it, row by row, are dropped.
    VIEW_TRUNCATED_SIGHT = 8,
    // A debris or exploded piece's geometry did not fit: it and every later one have no vertices and no faces.
    VIEW_TRUNCATED_DEBRIS = 16,
};

// What a face of ViewDebrisFace draws.
enum ViewFaceKind {
    // Nothing: a textured face of other than four vertices.
    VIEW_FACE_NONE = 0,
    // A polygon filled with the face's colour.
    VIEW_FACE_FILLED = 1,
    // A quad with a frame of the face's texture.
    VIEW_FACE_TEXTURED = 2,
};

// One player of the match, as the viewer sees it.
struct ViewPlayer {
    // 1 when the slot holds a player of the match.
    unsigned char active;
    // The player's kind: 1 human on this machine, 2 human elsewhere, 3 computer.
    unsigned char type;
    // The player's side, an index of the side definitions.
    unsigned char side;
    // The player's colour, an index of the team colours.
    unsigned char color;
    // 1 when the player is allied to the viewer.
    unsigned char allied;
    // 1 when the player only watches the match.
    unsigned char watching;
    // The player's place on the score panel, 0 first.
    unsigned char rank;
    // Unused, zero.
    unsigned char reserved;
    // Units killed.
    unsigned short kills;
    // Units lost.
    unsigned short losses;
    // Commanders killed.
    unsigned short commanderKills;
    // Commanders lost.
    unsigned short commanderLosses;
    // Units the player has made in the match.
    unsigned int unitsCreated;
    // Units the player has now.
    unsigned short unitCount;
    // Unused, zero.
    unsigned short reserved2;
    // Metal stored; zero for a player other than the viewer.
    float metal;
    // Energy stored; zero for a player other than the viewer.
    float energy;
    // Metal storage; zero for a player other than the viewer.
    float metalCapacity;
    // Energy storage; zero for a player other than the viewer.
    float energyCapacity;
    // Metal made a tick; zero for a player other than the viewer.
    float metalIncome;
    // Metal used a tick; zero for a player other than the viewer.
    float metalUsage;
    // Energy made a tick; zero for a player other than the viewer.
    float energyIncome;
    // Energy used a tick; zero for a player other than the viewer.
    float energyUsage;
    // The metal level above which the viewer shares metal with allies; zero for another player.
    float shareMetal;
    // The energy level above which the viewer shares energy with allies; zero for another player.
    float shareEnergy;
    // The tick at which the income and usage readout next refreshes; zero for a player other than the viewer.
    unsigned int displayTimer;
    // Unused, zero.
    unsigned int reserved3;
    // Metal made in the match, for the end-game screen.
    double totalMetalProduced;
    // Energy made in the match, for the end-game screen.
    double totalEnergyProduced;
    // Metal wasted in the match, for the end-game screen.
    double metalWasted;
    // Energy wasted in the match, for the end-game screen.
    double energyWasted;
};

// One unit the viewer may draw: own units, and other units in the viewer's sight.
struct ViewUnit {
    // The unit's id, the same while it lives.
    unsigned short id;
    // The unit's definition, an index of the unit definitions.
    unsigned short unitDefIndex;
    // The owning player.
    unsigned char owner;
    // ViewRelation bits.
    unsigned char relation;
    // The unit's control group, 0 for none; zero for a unit not the viewer's own.
    unsigned char group;
    // 1 when the unit draws in the first pass, under projectiles; 0 when it draws over them.
    unsigned char ground;
    // The unit's flags, cut by ViewUnit's rule of visibility (docs/view.md).
    unsigned int flags;
    // The unit's activation bits: activated, cloaked.
    unsigned char activateFlags;
    // Ticks left of the damage flash of its blip.
    unsigned char recentlyDamaged;
    // The progress of a stockpiled weapon in percent; zero for a unit not the viewer's own.
    unsigned char weaponBuild;
    // The kind of the unit's current order; zero for a unit not the viewer's own.
    unsigned char orderKind;
    // The unit's move order (hold, manoeuvre, roam); zero for a unit not the viewer's own.
    unsigned char moveOrder;
    // The unit's fire order (hold, return, at will); zero for a unit not the viewer's own.
    unsigned char fireOrder;
    // 1 when the unit is ordered to cloak; zero for a unit not the viewer's own.
    unsigned char cloakOrder;
    // 1 when the unit cannot move.
    unsigned char immobile;
    // Position x, 16.16 fixed point.
    int x;
    // Position y (height), 16.16 fixed point.
    int y;
    // Position z, 16.16 fixed point.
    int z;
    // Heading, a 16-bit angle.
    short heading;
    // Pitch, a 16-bit angle.
    short pitch;
    // Health points.
    short health;
    // Kills; zero for a unit not the viewer's own.
    unsigned short killCount;
    // Build left, 1 when unstarted and 0 when finished.
    float buildLeft;
    // The carrying unit's id plus one, 0 when not carried.
    unsigned short carrier;
    // The id of the target of the unit's current order plus one, 0 for none or a unit not the viewer's own.
    unsigned short orderTarget;
    // The first of the unit's pieces in PlayerView.pieces.
    unsigned int firstPiece;
    // The number of the unit's pieces.
    unsigned int pieceCount;
    // The first of the unit's queued orders in PlayerView.orders; zero for a unit not the controller's.
    unsigned int firstOrder;
    // The number of the unit's queued orders; zero for a unit not the controller's.
    unsigned int orderCount;
    // Metal the unit makes a tick; zero for a unit not the viewer's own.
    float metalMake;
    // Metal the unit uses a tick; zero for a unit not the viewer's own.
    float metalUse;
    // Energy the unit makes a tick; zero for a unit not the viewer's own.
    float energyMake;
    // Energy the unit uses a tick; zero for a unit not the viewer's own.
    float energyUse;
    // The shots its first weapon has stockpiled; zero for a unit not the viewer's own.
    unsigned char stockpile;
    // Unused, zero.
    unsigned char reserved;
    // Unused, zero.
    unsigned short reserved2;
};

// One posed piece of a unit's model.
struct ViewPiece {
    // Offset x from the parent piece, 16.16 fixed point.
    int x;
    // Offset y from the parent piece, 16.16 fixed point.
    int y;
    // Offset z from the parent piece, 16.16 fixed point.
    int z;
    // Rotation about x, a 16-bit angle.
    short rotX;
    // Rotation about y, a 16-bit angle.
    short rotY;
    // Rotation about z, a 16-bit angle.
    short rotZ;
    // 1 when the piece is shown.
    unsigned char shown;
    // Unused, zero.
    unsigned char reserved;
};

// One queued order of a unit of the controller, for the order overlays.
struct ViewOrder {
    // The order's kind, which picks the overlays it draws.
    unsigned char kind;
    // Unused, zero.
    unsigned char reserved;
    // The unit definition the order builds; zero for another kind.
    unsigned short buildDefIndex;
    // The id of the order's target unit plus one, 0 for none.
    unsigned short target;
    // Unused, zero.
    unsigned short reserved2;
    // The tick the order was given, which times its path animation.
    unsigned int created;
    // The order's position x, 16.16 fixed point.
    int x;
    // The order's position y, 16.16 fixed point.
    int y;
    // The order's position z, 16.16 fixed point.
    int z;
};

// One radar or sonar blip on the minimap.
struct ViewBlip {
    // The unit's id.
    unsigned short id;
    // The owning player, which gives the blip's colour.
    unsigned char owner;
    // ViewBlipFlags bits.
    unsigned char flags;
    // Position x, 16.16 fixed point.
    int x;
    // Position y (height), 16.16 fixed point.
    int y;
    // Position z, 16.16 fixed point.
    int z;
};

// One feature on the map.
struct ViewFeature {
    // The feature's anchor cell, x.
    unsigned short cellX;
    // The feature's anchor cell, z.
    unsigned short cellZ;
    // The feature's definition, an index of the feature definitions.
    unsigned short featureIndex;
    // The frame of the feature's animation.
    unsigned short frame;
    // The frame of the feature's shadow animation.
    unsigned short shadowFrame;
    // 1 when the feature draws as a model at the position below, 0 when as a sprite at its cell.
    unsigned char model;
    // 1 when the viewer may draw it; 0 when only its cell and definition are known, and the rest is zero.
    unsigned char drawn;
    // Model position x, 16.16 fixed point.
    int x;
    // Model position y, 16.16 fixed point.
    int y;
    // Model position z, 16.16 fixed point.
    int z;
    // Model rotation about x, a 16-bit angle.
    short rotX;
    // Model rotation about y, a 16-bit angle.
    short rotY;
    // Model rotation about z, a 16-bit angle.
    short rotZ;
    // Unused, zero.
    unsigned short reserved2;
};

// One projectile the viewer may draw.
struct ViewProjectile {
    // The weapon, an index of the weapon definitions.
    unsigned short weaponIndex;
    // The owning player.
    unsigned char owner;
    // Unused, zero.
    unsigned char reserved;
    // Position x, 16.16 fixed point.
    int x;
    // Position y, 16.16 fixed point.
    int y;
    // Position z, 16.16 fixed point.
    int z;
    // Where the shot started, x, for beams and lightning.
    int startX;
    // Where the shot started, y.
    int startY;
    // Where the shot started, z.
    int startZ;
    // Rotation about x, a 16-bit angle.
    short rotX;
    // Rotation about y, a 16-bit angle.
    short rotY;
    // Rotation about z, a 16-bit angle.
    short rotZ;
    // The spin of a propeller model, a 16-bit angle.
    short propellerSpin;
    // The ground height under the shot in pixels, for its shadow.
    int groundY;
    // The tick the shot was fired.
    unsigned int spawnTick;
    // The shot's age in ticks, which picks the model of a two-stage shot and the frame of a flame.
    unsigned int time;
};

// One debris or explosion record.
struct ViewDebris {
    // The explosion's sprite, an index of the sprite catalogue; 0xffff for none.
    unsigned short sprite;
    // The frame of the sprite.
    unsigned short frame;
    // The shadow's sprite, an index of the shadow catalogue; 0xffff for none.
    unsigned short shadowSprite;
    // The frame of the shadow.
    unsigned short shadowFrame;
    // The first of the debris' vertices in PlayerView.debrisVertices; 0 when the count is 0.
    unsigned int firstVertex;
    // The number of those vertices, 0 for an explosion with no model.
    unsigned int vertexCount;
    // The first of the debris' faces in PlayerView.debrisFaces; 0 when the count is 0.
    unsigned int firstFace;
    // The number of those faces, in the order they are drawn.
    unsigned int faceCount;
    // Position x, 16.16 fixed point.
    int x;
    // Position y, 16.16 fixed point.
    int y;
    // Position z, 16.16 fixed point.
    int z;
    // Rotation about x, a 16-bit angle.
    short rotX;
    // Rotation about y, a 16-bit angle.
    short rotY;
    // Rotation about z, a 16-bit angle.
    short rotZ;
    // Unused, zero.
    unsigned short reserved2;
};

// One model piece blown off a unit.
struct ViewExplodedPiece {
    // The player whose colour the piece's team-coloured faces take.
    unsigned char owner;
    // 1 when it trails smoke, 2 when it trails fire, 0 for neither.
    unsigned char trail;
    // Unused, zero.
    unsigned short reserved;
    // The first of the piece's vertices in PlayerView.debrisVertices; 0 when the count is 0.
    unsigned int firstVertex;
    // The number of those vertices.
    unsigned int vertexCount;
    // The first of the piece's faces in PlayerView.debrisFaces; 0 when the count is 0.
    unsigned int firstFace;
    // The number of those faces, in the order they are drawn.
    unsigned int faceCount;
    // Position x, 16.16 fixed point.
    int x;
    // Position y, 16.16 fixed point.
    int y;
    // Position z, 16.16 fixed point.
    int z;
    // Rotation about x, a 16-bit angle.
    short rotX;
    // Rotation about y, a 16-bit angle.
    short rotY;
    // Rotation about z, a 16-bit angle.
    short rotZ;
    // Unused, zero.
    unsigned short reserved2;
};

// One vertex of a debris or exploded piece, in the piece's own frame.
struct ViewDebrisVertex {
    // x, 16.16 fixed point.
    int x;
    // y, 16.16 fixed point.
    int y;
    // z, 16.16 fixed point.
    int z;
};

// One face of a debris or exploded piece.
struct ViewDebrisFace {
    // The first of the face's vertex indices in PlayerView.debrisIndices; 0 when the count is 0.
    unsigned int firstIndex;
    // The number of those indices, each an index of the piece's vertices from its firstVertex.
    unsigned short indexCount;
    // What the face draws, a ViewFaceKind.
    unsigned char kind;
    // Unused, zero.
    unsigned char reserved;
    // The palette index a filled face is drawn in; zero for another kind.
    unsigned int colour;
    // The texture of a textured face, an index of PlayerView.textures; 0xffff for none or another kind.
    unsigned short texture;
    // The frame of that texture.
    unsigned short frame;
};

// One texture that a face of a debris or exploded piece names.
struct ViewTexture {
    // The name of its GAF entry, found as a model's texture is (docs/view.md); the bytes after the name zero.
    char name[VIEW_TEXTURE_NAME];
};

// One sim event of the tick that reaches the viewer: a sound, a speech line, a message or a notice.
struct ViewMessage {
    // A SimEventKind of interfaces/sim_events.h.
    unsigned short kind;
    // The event's flags; bit 15 marks a record that carries the arguments past the sixth of the one before it.
    unsigned short flags;
    // The event's arguments, as sim_events.h gives them for the kind; the rest zero.
    unsigned int args[6];
};

// The view of one player: every fact of the simulation that presentation reads, cut to what that player may see.
struct PlayerView {
    // VIEW_VERSION_MAJOR of the writer.
    unsigned short versionMajor;
    // VIEW_VERSION_MINOR of the writer.
    unsigned short versionMinor;
    // The tick the view was filled after.
    unsigned int tick;
    // The player whose sight, economy and units the view holds.
    unsigned char viewer;
    // The player whose units the machine's input commands.
    unsigned char controller;
    // The kind of match: 1 campaign, 2 skirmish, 3 multiplayer.
    unsigned char gameType;
    // 2 when kills count only commanders, as the score panel shows them.
    unsigned char commanderDeath;
    // The match's options of sight: 0x1 mapping on (the map starts unexplored), 0x2 line of sight on, 0x4 true line of sight.
    unsigned short sightOptions;
    // The end of the match: 0x4 over, 0x20 won, 0x40 lost.
    unsigned short endFlags;
    // The most units a player may have.
    unsigned short maxUnits;
    // The tables that did not fit their capacity, ViewTruncated bits.
    unsigned short truncated;
    // The map's width in cells of 16 pixels.
    unsigned int mapWidth;
    // The map's height in cells of 16 pixels.
    unsigned int mapHeight;
    // The sight grid's width in cells of 32 pixels.
    unsigned int gridWidth;
    // The sight grid's height in cells of 32 pixels.
    unsigned int gridHeight;
    // The sea level, a height in pixels.
    int seaLevel;
    // The wind's drift a tick along x, 16.16 fixed point, which smoke follows.
    int windX;
    // The wind's drift a tick along z, 16.16 fixed point, which smoke follows.
    int windZ;
    // Gravity, 16.16 fixed point, which smoke follows.
    int gravity;
    // The rows of units in use.
    unsigned int unitCount;
    // The rows of pieces in use.
    unsigned int pieceCount;
    // The rows of orders in use.
    unsigned int orderCount;
    // The rows of blips in use.
    unsigned int blipCount;
    // The rows of features in use.
    unsigned int featureCount;
    // The rows of projectiles in use.
    unsigned int projectileCount;
    // The rows of debris in use.
    unsigned int debrisCount;
    // The rows of exploded pieces in use.
    unsigned int explodedPieceCount;
    // The rows of debris vertices in use.
    unsigned int debrisVertexCount;
    // The rows of debris faces in use.
    unsigned int debrisFaceCount;
    // The rows of debris face indices in use.
    unsigned int debrisIndexCount;
    // The rows of textures in use.
    unsigned int textureCount;
    // The rows of messages in use.
    unsigned int messageCount;
    // The players, by player index.
    ViewPlayer players[VIEW_MAX_PLAYERS];
    // The units, in unit id order.
    ViewUnit units[VIEW_MAX_UNITS];
    // The pieces of the units' models.
    ViewPiece pieces[VIEW_MAX_PIECES];
    // The queued orders, each unit's in the order they run.
    ViewOrder orders[VIEW_MAX_ORDERS];
    // The blips, in unit id order.
    ViewBlip blips[VIEW_MAX_BLIPS];
    // The features, in cell order.
    ViewFeature features[VIEW_MAX_FEATURES];
    // The projectiles, in the simulation's order.
    ViewProjectile projectiles[VIEW_MAX_PROJECTILES];
    // The debris, in the simulation's order.
    ViewDebris debris[VIEW_MAX_DEBRIS];
    // The exploded pieces, in the simulation's order.
    ViewExplodedPiece explodedPieces[VIEW_MAX_EXPLODED_PIECES];
    // The vertices of the debris and exploded pieces, each piece's in one run.
    ViewDebrisVertex debrisVertices[VIEW_MAX_DEBRIS_VERTICES];
    // The faces of the debris and exploded pieces, each piece's in one run.
    ViewDebrisFace debrisFaces[VIEW_MAX_DEBRIS_FACES];
    // The vertex indices of those faces, each face's in one run.
    unsigned short debrisIndices[VIEW_MAX_DEBRIS_INDICES];
    // The textures the faces name, in the order the fill first meets them.
    ViewTexture textures[VIEW_MAX_TEXTURES];
    // The tick's messages, in the order the simulation raised them.
    ViewMessage messages[VIEW_MAX_MESSAGES];
    // The viewer's sight of each grid cell, a ViewSight, row by row over gridWidth.
    unsigned char sight[VIEW_MAX_GRID_CELLS];
};

#endif
