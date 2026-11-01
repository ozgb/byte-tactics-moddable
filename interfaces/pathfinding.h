#ifndef INTERFACES_PATHFINDING_H
#define INTERFACES_PATHFINDING_H

class Pathfinder;
class AiSearchGoal;

// Major version of the pathfinding interface, changed when a slot is removed or its signature changes.
#define PATHFINDING_VERSION_MAJOR 1
// Minor version of the pathfinding interface, changed when a slot is added.
#define PATHFINDING_VERSION_MINOR 2

// Unit movement planning.
struct PathfindingSeam {
    // Spends this tick's step budget on the players' pending path searches (thiscall in 3.1c). Hooks: none.
    void (__fastcall* RunSearches)(Pathfinder* self);
    // Stops the search for a goal when it is the one being searched (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* AbortIfGoalMatch)(Pathfinder* self, AiSearchGoal* goal);
    // Destroys a pathfinder and releases its memory. Hooks: none.
    void (__stdcall* Destroy)(Pathfinder* self);
    // Allocates the pathfinder of a game from the sim block, builds it and stores it in the game state. Hooks: none.
    void (__stdcall* Create)(void);
};

#endif
