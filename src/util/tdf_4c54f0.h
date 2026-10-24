#ifndef TDF_4C54F0_H
#define TDF_4C54F0_H

struct TdfField;

void __stdcall AssignPairRange(TdfField* first, TdfField* last, const TdfField& x);
TdfField* __stdcall AssignPairRangeBack(TdfField* first, TdfField* last, TdfField* dest);
void __stdcall ConstructPair(TdfField* p, const TdfField& value);

void __stdcall LoadTranslations(char* filename, char* section);
int __stdcall FindTranslation(char* name);

#endif
