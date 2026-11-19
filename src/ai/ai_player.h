#ifndef AI_PLAYER_H
#define AI_PLAYER_H
struct Vec3;

struct MirroredBasePos;
struct MovableVec3;

struct Player;
struct Vec3;

class UnitDef;
struct Unit;
struct Vec3;

struct AiEvent;
struct StateWriter;
class UnitDef;
struct Unit;

void __stdcall EnableAICommands();
void __stdcall FUN_00406f40(void);
int __stdcall AiApply(const AiEvent* event);
void __stdcall AiWriteState(const StateWriter* writer);
void __stdcall UpdateConverter(Unit* unit);
void __stdcall RetargetWeapon(Unit* unit, unsigned int weapon);
int __stdcall RateWeapons(UnitDef* p);
int __stdcall RateUnitType(UnitDef* p);
int __stdcall IsUnderLimit(int player, unsigned short index, int value);
void __stdcall ParseDownloadableAiWeightScripts(int player);
void __stdcall ReparseAiWeightScriptsIfLimitNotSticky(int player);
void __stdcall ResetAIPlayers();
void __stdcall ProbeUnitDefEnergyRate(int unused1, int unused2, int);
void __stdcall UpdatePlayerAI(int player);
void __stdcall CreatePlayerAI(int player);
void __stdcall RebuildFeatureCells(int);
void __stdcall DestroyPlayerAI(int player);
int __stdcall GetUnitCount(int, unsigned int);
void __stdcall DumpPlayerAI(int player, void* file);

int __stdcall GetBuildRating(int player, unsigned short type);
unsigned short __stdcall ChooseBuildOption(unsigned int player, Unit* unit);
int __stdcall FindBuildPosition(int player, const Vec3* from, UnitDef* type, Vec3* out);
int __stdcall GetBuilderCount(int);

int __stdcall SumUnitRatingsInRange(int player, Vec3* pos, int range);

void __stdcall GetMirroredBasePosition(int index, int unused, MirroredBasePos* out);
MovableVec3 __stdcall MoveToward(const MovableVec3* from, const MovableVec3* to, int maxLen);

void __stdcall GetBasePosition(int index, Vec3* out);

#endif
