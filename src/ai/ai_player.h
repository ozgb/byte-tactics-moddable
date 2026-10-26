#ifndef AI_PLAYER_H
#define AI_PLAYER_H
struct Player;
struct Vec3;

class UnitDef;
struct Unit;
struct Vec3;
struct Vec3_40b0d0;

class CommandArgs;
class UnitDef;
struct Unit;
struct _iobuf;

void __stdcall CmdPlan(CommandArgs* args);
void __stdcall EnableAICommands();
void __stdcall CmdWeight(CommandArgs* args);
void __stdcall CmdLimit(CommandArgs* args);
void __stdcall RegisterAICommands();
void __stdcall FUN_00406f40(void);
void __stdcall ReactToAttack(Unit* attacker, Unit* unit, int unused);
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
void __stdcall DumpPlayerAI(int player, _iobuf* file);
int __stdcall HasSpecialUnit(int);

bool __stdcall HasReadyUnitInRange(int player, Vec3_40b0d0* p, int range);
int __stdcall GetBuildRating(int player, unsigned short type);
unsigned short __stdcall ChooseBuildOption(unsigned int player, Unit* unit);
int __stdcall FindBuildPosition(int player, const Vec3* from, UnitDef* type, Vec3* out);
int __stdcall GetBuilderCount(int);

int __stdcall IsPointVisible(Player* player, Vec3* point);
int __stdcall SumUnitRatingsInRange(int player, Vec3* pos, int range);

#endif
