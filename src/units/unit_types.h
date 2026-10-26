#ifndef UNIT_TYPES_H
#define UNIT_TYPES_H

class UnitDef;

int __stdcall LoadUnitInfo();
void __stdcall FreeUnitInfo();
void __stdcall RefreshUnitInfo();
void __stdcall CheckDownloadableFlags();
void __stdcall AddDownloadBuildOptions();
void __stdcall LoadUnitFbi(char* fbi_file, UnitDef* unitdef);
void __stdcall ReloadUnitType(unsigned short index);
void __stdcall LoadUnitTypes();
int __stdcall CompareUnitTypeNames(const char*, const char*);
void __stdcall FreeUnitTypes();
void __stdcall LoadDownloadMenus();
void __stdcall FreeDownloadMenus();

#endif
