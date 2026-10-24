#ifndef MAP_LIST_H
#define MAP_LIST_H

void __stdcall SetMissionType(int owner);
void __stdcall FreeOtaEnumCacheAndMission();

int __stdcall LoadMapList(void** out, int, int);

#endif
