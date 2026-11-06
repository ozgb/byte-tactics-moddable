#ifndef INTERFACES_PATHFINDING_H
#define INTERFACES_PATHFINDING_H

// Major version of the pathfinding interface, changed when a slot is removed or its signature changes.
#define PATHFINDING_VERSION_MAJOR 2
// Minor version of the pathfinding interface, changed when a slot is added.
#define PATHFINDING_VERSION_MINOR 0

// A unit, as an opaque handle the world view gives (0 for none).
typedef unsigned int PathUnit;
// A movement class, as an opaque handle the world view gives (0 for none).
typedef unsigned int PathClass;
// An object of the requester's that pathfinding stores and hands back in events, and never reads.
typedef unsigned int PathHandle;

// The most waypoints one event carries.
#define PATH_MAX_WAYPOINTS 64

// A point: in cells for the world view and goal areas, in world units (16 per cell) for waypoints.
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
    // The centre of a circle or a ring.
    PathPoint centre;
    // The radius of a circle in radius[0], and the inner and outer radii of a ring in radius[0] and radius[1]: in the units of the distance estimate (about 18 per cell).
    int radius[2];
    // The same radii squared in cells, as the containment tests compare them.
    int radiusSq[2];
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
    PATH_FAILED = 2
};

// What happened to a request in the last step.
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
    // The number of waypoints.
    int count;
    // The waypoints from the start, in world units; they need not lie on cell centres.
    PathPoint points[PATH_MAX_WAYPOINTS];
};

// The map a match starts on.
struct PathMapInfo {
    // The size of the grid of cells.
    int width, height;
    // The number of unit slots.
    int unitSlots;
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
    // Brings the pass state of a unit's movement class up to date before a search for it, leaving the unit's own cells as they were.
    void (__stdcall* BeginSearch)(PathUnit unit);
    // Brings the unit's own cells up to date after its search.
    void (__stdcall* EndSearch)(PathUnit unit);
};

// The state the trace hashes (sections 16 and 17 of docs/trace-format.md), in the layouts given there.
struct PathTraceView {
    // The pathfinder record, or null when there is none.
    const void* record;
    // The ten per-player heuristic weights.
    const int* budgetCap;
    // The ten per-player search counts of the current window.
    const int* tickLoad;
    // The ticks since the weights were last set.
    const int* refreshCounter;
};

// Path searches for units.
struct PathfindingSeam {
    // Builds the state for a map that has just loaded, in a region of the state block it allocates; called once per match, before the first step.
    void (__stdcall* Init)(const PathMapInfo* map);
    // Frees that state when the match ends.
    void (__stdcall* Shutdown)(void);
    // Adds a request, or replaces the pending one of the same requester; ignored while that requester's search runs. It is first considered at the next step.
    void (__stdcall* Submit)(const PathRequest* request);
    // Drops a requester's pending or running request; no event of it is polled afterwards.
    void (__stdcall* Cancel)(PathHandle requester);
    // Tells the implementation that the cells of a rectangle changed blocking or cost; called when they change, so before the next step.
    void (__stdcall* CellsChanged)(int left, int top, int right, int bottom);
    // Does this tick's search work, as much as the implementation chooses; called once per tick, in the players phase.
    void (__stdcall* Step)(void);
    // Takes the next event of the last step, in the order they happened; returns 0 when none is left. Called after Step, in the same phase.
    int (__stdcall* Poll)(PathEvent* out);
    // Fills the view of the state the trace hashes; it reads only, and keeps nothing.
    void (__stdcall* Project)(PathTraceView* out);
};

#endif
