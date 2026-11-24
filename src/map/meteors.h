#ifndef METEORS_H
#define METEORS_H

#include "../../interfaces/wind_meteors.h"

class HapiBank;

#ifndef BT_REPLAY_INJECT_GAP_SCALE
#define BT_REPLAY_INJECT_GAP_SCALE 30.0
#endif

void __stdcall InitMeteors();
void __stdcall EmptyShutdownPreCleanup(void);
void __stdcall EnableMeteors();
void __stdcall DisableMeteors();
void __stdcall SetMeteorParams(const MeteorShowerParams* p);
void __stdcall UpdateMeteors();
void __stdcall WindMeteorsApply(const WindMeteorsEventHeader* event, unsigned int size);
void __stdcall LoadMeteorDefaults(MeteorShowerParams* params);
void __stdcall StartMeteorShower();
void __stdcall SaveMeteors(HapiBank* file);
void __stdcall LoadMeteors(HapiBank* file);
void __stdcall WindMeteorsEmit(const WindMeteorsEventHeader* effect, unsigned int size);

#endif
