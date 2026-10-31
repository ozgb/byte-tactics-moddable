#ifndef WEAPONS_H
#define WEAPONS_H
struct Vec3;

struct Unit;
struct UnitWeaponSlot;

struct Projectile;
struct RemoteFireEventView;
struct RockUnitView;
struct Unit;
struct UnitWeaponSlot;
struct Vec3;

struct Projectile;
struct Unit;

struct Unit;
struct UnitWeaponSlot;
struct Vec3;

struct Projectile;
struct Unit;
struct Vec3;
struct WeaponDef;

void __stdcall ApplyAreaDamage(Projectile* weapon, Vec3* pos);
void __stdcall UpdateProjectiles();
void __stdcall AllocWeaponArray(void);
void __stdcall FreeWeaponArray(void);
void __stdcall ApplyWeaponHit(Projectile* weapon, Unit* target);
int __stdcall ApplyWeaponDamage(Projectile* weapon, Unit* target, float scale);
void __stdcall UntrackProjectile(Projectile* projectile);
int __stdcall WeaponCanReachUnit(Unit* unit1, Unit* unit2, unsigned char weapon);
void __stdcall CompactProjectiles();
void __stdcall DetonateUnitWeapon(Unit* unit, int second);
Vec3* __stdcall GetProjectileAimPoint(Projectile* p);
int __stdcall TurnUnitTowardsPoint(Projectile* p, Vec3* target);
void __stdcall ComputeVelocityFromAngles(Projectile* obj);
Projectile* __stdcall AllocProjectile();
void __stdcall ComputeProjectileTime(Projectile* obj);
void __stdcall CopyWeaponVelocityParam(Projectile* obj, WeaponDef* src);
int __stdcall SpawnProjectile(WeaponDef* weapon, Vec3* a, Vec3* b, int flag);
void __stdcall SetWeaponFireHandler(WeaponDef* weapon);
void __stdcall InitUnitWeaponSlots(Unit* unit);
void __stdcall UpdateUnitWeapons(Unit* unit);
char* __stdcall FindWeaponByName(char* name);
void __stdcall CheckProjectileCollision(WeaponDef* type, Projectile* proj);

int __stdcall FireTurretWeapon(Unit* fire, UnitWeaponSlot* unit, Unit* target, Vec3* point);

void __stdcall DetonateProjectile(Projectile* projectile, Unit* unit);

void __stdcall SendWeaponFirePacket(UnitWeaponSlot* unit, Unit* source, Unit* target, Vec3* a, Vec3* b);
void __stdcall BroadcastWeaponFire(char, Vec3* a, Vec3* b);
void __stdcall RockUnit(RockUnitView* obj, UnitWeaponSlot* src);
void __stdcall ApplyAreaDamageAt(void* owner, Vec3* pos);
void __stdcall RemoveUnitProjectiles(Unit* owner);
int __stdcall SpawnProjectileFromUnitMotion(UnitWeaponSlot* shot, Unit* unit, Vec3* pos);
int __stdcall FireWeaponByFlags(UnitWeaponSlot* obj, int a, int b, int d, int c);
Projectile* __stdcall FindRemoteProjectile(RemoteFireEventView* ev);

void __stdcall DrawProjectiles(void* surface);

int __stdcall AimWithinTolerance(Unit* unit, UnitWeaponSlot* aim, short angle1, short angle2);

int __stdcall VectorLength(Vec3* v);

#endif
