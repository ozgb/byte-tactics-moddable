#ifndef WEAPON_DEF_H
#define WEAPON_DEF_H

#pragma pack(push, 1)

struct Table_00499cd0;

struct WeaponDef {
    char name[0x20];
    char description[0x40];
    void* fire;
    Table_00499cd0* table;
    int speed;
    int startSpeed;
    int acceleration;
    void* model;
    void* explosionSeq;
    void* splash;
    char modelName[0x40];
    float energyPerShot;
    float metalPerShot;
    union {
        float pitch;
        int field_c8;
    };
    int shakeMagnitude;
    int shakeDuration;
    unsigned short damage;
    unsigned short areaOfEffect;
    float edgeDamage;
    int range;
    int coverage;
    unsigned short reloadTime;
    unsigned short lifetime;
    unsigned short turnRate;
    unsigned short burst;
    unsigned short burstRate;
    unsigned short spread;
    unsigned short duration;
    unsigned short lifeRand;
    unsigned short sound;
    unsigned short soundHit;
    unsigned short soundWater;
    unsigned short smokeRate;
    unsigned short flightTime;
    unsigned short holdTime;
    char unknown_100[4];
    short accuracy;
    unsigned short tolerance;
    unsigned short pitchTolerance;
    unsigned char index;
    unsigned char fireStarter;
    unsigned char shotKind;
    unsigned char colour;
    unsigned char colour2;
    char unknown_10f[2];
    union {
        unsigned int value;
        struct {
            unsigned int lineOfSight : 1;
            unsigned int ballistic : 1;
            unsigned int shellWeapon : 1;
            unsigned int beamWeapon : 1;
            unsigned int vLaunch : 1;
            unsigned int meteor : 1;
            unsigned int noRadar : 1;
            unsigned int paralyzer : 1;
            unsigned int dropped : 1;
            unsigned int startSmoke : 1;
            unsigned int endSmoke : 1;
            unsigned int soundTrigger : 1;
            unsigned int guidance : 1;
            unsigned int tracks : 1;
            unsigned int unitsOnly : 1;
            unsigned int groundBounce : 1;
            unsigned int waterWeapon : 1;
            unsigned int toAirWeapon : 1;
            unsigned int smokeTrail : 1;
            unsigned int turret : 1;
            unsigned int selfProp : 1;
            unsigned int propeller : 1;
            unsigned int noExplode : 1;
            unsigned int burnBlow : 1;
            unsigned int twoPhase : 1;
            unsigned int cruise : 1;
            unsigned int commandFire : 1;
            unsigned int noAutoRange : 1;
            unsigned int stockpile : 1;
            unsigned int targetable : 1;
            unsigned int interceptor : 1;
            unsigned int bit31 : 1;
        } b;
    } flags;
};

#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(WeaponDef) == 0x115, "WeaponDef");
#endif

#endif
