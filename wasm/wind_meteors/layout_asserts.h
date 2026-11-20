#undef g_meteorNextStrikeTime
#undef g_meteorWeaponName
#undef g_meteorSpawnMagnitude
#undef g_meteorSpawnInterval
#undef g_meteorActive
#undef g_meteorStrikeEndTime
#undef g_meteorOrigin
#undef g_meteorShowerDuration
#undef g_meteorWeapon
#undef g_meteorsEnabled
#undef g_meteorNextHitTime
#undef g_meteorTarget
#undef g_meteorScheduleGap
static_assert(sizeof(SimGlobals) == 49212, "");
static_assert(offsetof(SimGlobals, g_meteorNextStrikeTime) == 564, "");
static_assert(offsetof(SimGlobals, g_meteorWeaponName) == 572, "");
static_assert(offsetof(SimGlobals, g_meteorSpawnMagnitude) == 604, "");
static_assert(offsetof(SimGlobals, g_meteorSpawnInterval) == 608, "");
static_assert(offsetof(SimGlobals, g_meteorActive) == 612, "");
static_assert(offsetof(SimGlobals, g_meteorStrikeEndTime) == 616, "");
static_assert(offsetof(SimGlobals, g_meteorOrigin) == 620, "");
static_assert(offsetof(SimGlobals, g_meteorShowerDuration) == 624, "");
static_assert(offsetof(SimGlobals, g_meteorWeapon) == 628, "");
static_assert(offsetof(SimGlobals, g_meteorsEnabled) == 632, "");
static_assert(offsetof(SimGlobals, g_meteorNextHitTime) == 636, "");
static_assert(offsetof(SimGlobals, g_meteorTarget) == 640, "");
static_assert(offsetof(SimGlobals, g_meteorScheduleGap) == 644, "");
static_assert(sizeof(Ref<WeaponDef>) == 4, "");
static_assert(sizeof(MeteorShowerParams) == 48, "");
static_assert(offsetof(MeteorShowerParams, radius) == 32, "");
static_assert(offsetof(MeteorShowerParams, density) == 36, "");
static_assert(offsetof(MeteorShowerParams, duration) == 40, "");
static_assert(offsetof(MeteorShowerParams, interval) == 44, "");
static_assert(sizeof(WindMeteorsEventHeader) == 4, "");
static_assert(sizeof(WindMeteorsParams) == 52, "");
static_assert(offsetof(WindMeteorsParams, params) == 4, "");
static_assert(sizeof(WindMeteorsSpawn) == 36, "");
static_assert(offsetof(WindMeteorsSpawn, weapon) == 4, "");
static_assert(offsetof(WindMeteorsSpawn, pos) == 8, "");
static_assert(offsetof(WindMeteorsSpawn, vel) == 20, "");
static_assert(offsetof(WindMeteorsSpawn, announce) == 32, "");
static_assert(sizeof(HapiBank) == 4, "");
static_assert(sizeof(Seams) == 72, "");
static_assert(offsetof(Seams, weapons) == 20, "");
static_assert(offsetof(Seams, files) == 60, "");
static_assert(sizeof(WindMeteorsSeam) == 32, "");
