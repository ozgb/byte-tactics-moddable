#ifndef METEORS_H
#define METEORS_H

class HapiBank;
class MeteorParams;

void __stdcall InitMeteors();
void __stdcall EmptyShutdownPreCleanup(void);
void __stdcall EnableMeteors();
void __stdcall DisableMeteors();
void __stdcall SetMeteorParams(MeteorParams* p);
void __stdcall UpdateMeteors();
void __stdcall StartMeteorShower();
void __stdcall SaveMeteors(HapiBank* file);
void __stdcall LoadMeteors(HapiBank* file);

#endif
