#ifndef FEATURES_H
#define FEATURES_H
struct Cell;
struct Feature;
struct Point16;
struct Unit;
struct Vec3;
struct Weapon;

class HapiBank;
class TdfFile;
struct Cell;
struct Feature;
struct Obj;
struct Point16;

unsigned short __stdcall GetCellFeature(Cell* cell);
void __stdcall StampFeatureMetal(void);
void __stdcall FreeFeaturePool();
void* __stdcall FindOptionalGafEntry(void* list, int unused, char* name);
void __stdcall LoadFeatureFileList();
void __stdcall FreeFeatureFileList();
TdfFile* __stdcall FindFeatureFile(char* name);
short __stdcall FindFeatureType(char* name);
unsigned short __stdcall FindOrLoadFeatureType(char* name);
void __stdcall PlaceMissionFeatures(void);
int __stdcall AllocFeatureSpot();
void __stdcall MoveFeatureSpot(int index, int* head);
void __stdcall StartFeatureBurning(int x, int z, int flag);
void __stdcall KillFeature(int x, int z, int flag);
void __stdcall ReplaceFeatureWithDead(int x, int y, int flag);
void __stdcall SpreadFire(Feature* f, Point16* cell);
void __stdcall ReplaceFeatureWithBurnt(Obj* obj);
void __stdcall UpdateFeatures();
void __stdcall RemoveAllFeatures(void);
void __stdcall SaveFeatures(HapiBank* file);

Vec3 __stdcall GetFootprintCentre(Point16* cell, Feature* def);
int __stdcall ReclaimFeature(Unit* unit, Vec3* pos);
void __stdcall DamageFeature(Cell* cell, int x, int z, Weapon* weapon);
int __stdcall RemoveFeature(Cell* cell, int flag);

#endif
