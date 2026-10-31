#ifndef TDF_H
#define TDF_H

#include "../../interfaces/services.h"

void __stdcall ReleasePair(char*);

void __stdcall AssignPairRange(TdfField* first, TdfField* last, const TdfField& x);
TdfField* __stdcall AssignPairRangeBack(TdfField* first, TdfField* last, TdfField* dest);
void __stdcall ConstructPair(TdfField* p, const TdfField& value);

void __stdcall LoadTranslations(char* filename, char* section);
int __stdcall FindTranslation(char* name);
char* __stdcall Translate(const char* key);

#endif
