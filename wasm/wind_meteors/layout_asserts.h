#undef DAT_005119b8
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
static_assert(sizeof(SimGlobals) == 48712, "");
static_assert(offsetof(SimGlobals, DAT_005119b8) == 4, "");
static_assert(offsetof(SimGlobals, g_meteorNextStrikeTime) == 568, "");
static_assert(offsetof(SimGlobals, g_meteorWeaponName) == 576, "");
static_assert(offsetof(SimGlobals, g_meteorSpawnMagnitude) == 608, "");
static_assert(offsetof(SimGlobals, g_meteorSpawnInterval) == 612, "");
static_assert(offsetof(SimGlobals, g_meteorActive) == 616, "");
static_assert(offsetof(SimGlobals, g_meteorStrikeEndTime) == 620, "");
static_assert(offsetof(SimGlobals, g_meteorOrigin) == 624, "");
static_assert(offsetof(SimGlobals, g_meteorShowerDuration) == 628, "");
static_assert(offsetof(SimGlobals, g_meteorWeapon) == 632, "");
static_assert(offsetof(SimGlobals, g_meteorsEnabled) == 636, "");
static_assert(offsetof(SimGlobals, g_meteorNextHitTime) == 640, "");
static_assert(offsetof(SimGlobals, g_meteorTarget) == 644, "");
static_assert(offsetof(SimGlobals, g_meteorScheduleGap) == 648, "");
static_assert(sizeof(Ref<WeaponDef>) == 4, "");
static_assert(sizeof(MeteorParams) == 48, "");
static_assert(offsetof(MeteorParams, radius) == 32, "");
static_assert(offsetof(MeteorParams, density) == 36, "");
static_assert(offsetof(MeteorParams, duration) == 40, "");
static_assert(offsetof(MeteorParams, interval) == 44, "");
static_assert(sizeof(HapiBank) == 4, "");
static_assert(sizeof(CommandArgs) == 212, "");
static_assert(offsetof(CommandArgs, buffer) == 80, "");
static_assert(offsetof(CommandArgs, count) == 208, "");
static_assert(sizeof(Seams) == 52, "");
static_assert(offsetof(Seams, weapons) == 20, "");
static_assert(offsetof(Seams, files) == 48, "");
static_assert(sizeof(WindMeteorsSeam) == 48, "");
