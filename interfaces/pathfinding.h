#ifndef INTERFACES_PATHFINDING_H
#define INTERFACES_PATHFINDING_H

#include "state_writer.h"

// Major version of the pathfinding interface, changed when a slot is removed or its signature changes.
#define PATHFINDING_VERSION_MAJOR 3
// Minor version of the pathfinding interface, changed when a slot is added.
#define PATHFINDING_VERSION_MINOR 0

// A unit, as an opaque handle the world view gives (0 for none).
typedef unsigned int PathUnit;
// A movement class, as an opaque handle the world view gives (0 for none).
typedef unsigned int PathClass;
// An object of the requester's that pathfinding stores and hands back in events, and never reads.
typedef unsigned int PathHandle;

// A point: in cells for the world view and the centre of a goal area, in world units (16 per cell) for waypoints.
struct PathPoint {
    // East.
    short x;
    // South.
    short y;
};

// The kinds of goal area.
enum PathGoalKind {
    // No cell is in the goal, and every distance to it is 0.
    PATH_GOAL_NONE = 0,
    // The cells within a radius of a centre.
    PATH_GOAL_CIRCLE = 1,
    // The cells between an inner and an outer radius of a centre.
    PATH_GOAL_RING = 2,
    // The border cells of a rectangle.
    PATH_GOAL_RECT = 3
};

// Where a search may end, in cells.
struct PathGoalArea {
    // A PathGoalKind.
    int kind;
    // The centre of a circle or a ring, in cells.
    PathPoint centre;
    // The radius of a circle in radius[0], and the inner and outer radii of a ring in radius[0] and radius[1], in world units (16 per cell).
    int radius[2];
    // A rectangle's first and last column.
    int left, right;
    // A rectangle's first and last row.
    int top, bottom;
};

// A request for a path.
struct PathRequest {
    // The unit to move. Its cell, heading and movement class when the search starts are the start.
    PathUnit unit;
    // The first tick the search may start in.
    unsigned int notBefore;
    // Where the path should end.
    PathGoalArea goal;
    // Names the request in Cancel and in events: one request per requester.
    PathHandle requester;
    // A second handle of the requester's, carried with the request.
    PathHandle target;
    // Whether start is used instead of the unit's cell, to continue a route from where an earlier one ended; the default implementation ignores it, because 3.1c's orders never ask for it.
    int useStart;
    // The cell the search starts from when useStart is set.
    PathPoint start;
};

// The kinds of event a step produces.
enum PathEventKind {
    // A search started; status is a PathStart.
    PATH_EVENT_STARTED = 0,
    // A search has a route so far and goes on; points hold it.
    PATH_EVENT_PROGRESS = 1,
    // A search ended; status is a PathStatus and points hold the route.
    PATH_EVENT_FINISHED = 2
};

// How a search started.
enum PathStart {
    // The start is in the goal, or a straight walk from it reaches the goal.
    PATH_START_DIRECT = 0,
    // A straight walk does not reach the goal.
    PATH_START_INDIRECT = 1
};

// How a search ended.
enum PathStatus {
    // The route reaches the goal.
    PATH_FOUND = 0,
    // The route was cut short and leads towards the goal.
    PATH_PARTIAL = 1,
    // There is no route; count is 0.
    PATH_FAILED = 2,
    // The goal cannot be reached; the route ends at the reachable cell nearest to it, measured as the straight-line distance in cells to the nearest cell of the goal area, among the cells the search expanded.
    PATH_NEAREST = 3
};

// What happened to a request in the last step; its waypoints are in world units, and the default movement follows only segments along the eight grid directions (the default search and the demo emit cell * 16 + footprint * 8).
struct PathEvent {
    // A PathEventKind.
    int kind;
    // A PathStart or a PathStatus, by kind.
    int status;
    // The request's unit.
    PathUnit unit;
    // The request's requester.
    PathHandle requester;
    // The request's second handle.
    PathHandle target;
    // The tick of the step.
    unsigned int tick;
    // The number of waypoints of the route; Poll copies as many as fit the caller's buffer, from the start of the route.
    int count;
};

// The map a match starts on.
struct PathMapInfo {
    // The size of the grid of cells.
    int width, height;
    // The number of unit slots.
    int unitSlots;
    // The number of movement classes the map uses.
    int moveClassCount;
};

// Why a rectangle of cells changed.
enum PathChange {
    // Terrain or features changed, so every unit's route may.
    PATH_CHANGE_TERRAIN = 1,
    // The stamps of units on the pass maps were refreshed; TerrainPass did not change.
    PATH_CHANGE_UNITS = 2
};

// Where a unit is and how it moves.
struct PathUnitState {
    // The player who owns it.
    int player;
    // Its cell.
    PathPoint cell;
    // Its heading, a full turn in 65536.
    unsigned short heading;
    // Its movement class.
    PathClass moveClass;
    // The size of the footprint of its movement class, in half cells.
    int footprintX, footprintY;
};

// The world as pathfinding sees it, read only apart from BeginSearch and EndSearch; given by the engine.
struct PathWorldView {
    // The tick being played.
    unsigned int (__stdcall* Tick)(void);
    // The most search work per tick the match setup allows, in the implementation's own units of work, or 0 when the match sets no limit and the implementation uses its own default budget.
    unsigned int (__stdcall* SearchStepLimit)(void);
    // The number of players in the match.
    int (__stdcall* PlayerCount)(void);
    // Whether a player (0 to 9) is in the match and plays.
    int (__stdcall* IsPlaying)(int player);
    // The number of unit slots each player has.
    int (__stdcall* UnitsPerPlayer)(void);
    // The first unit slot of a player.
    PathUnit (__stdcall* FirstUnit)(int player);
    // The last unit slot of a player.
    PathUnit (__stdcall* LastUnit)(int player);
    // The unit slot after a slot.
    PathUnit (__stdcall* NextUnit)(PathUnit unit);
    // Fills a unit's state; returns 0 when the slot is empty or the unit has no movement class.
    int (__stdcall* UnitState)(PathUnit unit, PathUnitState* out);
    // Movement class number 0 to 31, or 0 when that number is unused.
    PathClass (__stdcall* MoveClass)(int index);
    // Whether a cell is on the map for a movement class.
    int (__stdcall* InBounds)(PathClass moveClass, int x, int y);
    // A cell's pass state for a movement class: 0 blocked, 1 passable at a cost, 2 or more open.
    int (__stdcall* Pass)(PathClass moveClass, int x, int y);
    // The size of the visibility grid, whose cells are 2 by 2 cells.
    void (__stdcall* SeenSize)(int* width, int* height);
    // Whether a player has seen a cell of the visibility grid.
    int (__stdcall* Seen)(int player, int x, int y);
    // Whether a unit of a movement class can go straight between two cells: every cell the segment crosses is in bounds and not blocked.
    int (__stdcall* LineOfSight)(PathClass moveClass, int x0, int y0, int x1, int y1);
    // A cell's pass state for a movement class from terrain and features alone, as Pass without the stamps of units: 0 blocked, 1 passable at a cost, 2 or more open.
    int (__stdcall* TerrainPass)(PathClass moveClass, int x, int y);
    // Brings the pass state of a unit's movement class up to date; every implementation calls it before it reads the pass state for a search of the unit, because movement reads the maps it refreshes.
    void (__stdcall* BeginSearch)(PathUnit unit);
    // Brings the unit's own cells up to date; every implementation calls it when a search of the unit ends or is dropped, because movement reads the maps it refreshes.
    void (__stdcall* EndSearch)(PathUnit unit);
};

// Path searches for units.
struct PathfindingSeam {
    // The bytes of private state the implementation needs for a map; it depends on the map alone, and is called before Init.
    unsigned int (__stdcall* RegionSize)(const PathMapInfo* map);
    // Builds the state for a map that has just loaded, in a region the engine allocated of the size RegionSize gave; called once per match, before the first step.
    void (__stdcall* Init)(const PathMapInfo* map, void* region, unsigned int size);
    // Ends the match and returns the region, which the engine frees.
    void* (__stdcall* Shutdown)(void);
    // Adds a request, or replaces the pending one of the same requester; ignored while that requester's search runs. It is first considered at the next step.
    void (__stdcall* Submit)(const PathRequest* request);
    // Drops a requester's pending or running request; no event of it is polled afterwards.
    void (__stdcall* Cancel)(PathHandle requester);
    // Tells the implementation that the cells of a rectangle changed in the pass maps of the movement classes whose index bits are set in classes, for a PathChange; called when they change, so before the next step, and possibly during one.
    void (__stdcall* CellsChanged)(int left, int top, int right, int bottom, unsigned int classes, int cause);
    // Does this tick's search work, as much as the implementation chooses; called once per tick, in the players phase.
    void (__stdcall* Step)(void);
    // Takes the next event of the last step, in the order they happened, and copies the first min(count, capacity) waypoints of its route to points; returns 0 when none is left. Called after Step, in the same phase.
    int (__stdcall* Poll)(PathEvent* out, PathPoint* points, int capacity);
    // Writes the state that decides what the implementation does next and that the engine does not already hash elsewhere; it reads only, and keeps nothing.
    void (__stdcall* WriteState)(const StateWriter* writer);
    // Optional, null when the implementation has none: applies the second number of the search console command, in 16.16 fixed point.
    void (__stdcall* SetSearchScale)(int scale);
    // Optional, null when the implementation has none: fills what the debug overlay draws on a cell. Bits 0 and 1 of flags are 0 not visited, 1 open, 2 closed or 3 blocked, bit 2 marks a goal cell, and dir is a grid direction (0 north, then counter-clockwise).
    void (__stdcall* DebugCell)(int x, int y, int* flags, int* dir);
};

#endif
