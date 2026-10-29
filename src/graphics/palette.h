#ifndef PALETTE_H
#define PALETTE_H
struct Display;
struct tagPALETTEENTRY;

struct tagPALETTEENTRY;

int __stdcall GetPaletteColors(unsigned char* dest, int first, int count);
unsigned char __stdcall NearestColor(tagPALETTEENTRY* palette, tagPALETTEENTRY color);
void __stdcall SortByBrightness(unsigned char* data, int* sums, unsigned char* idx);
unsigned char __stdcall NearestColorInBand(tagPALETTEENTRY* palette, int* band, unsigned char* order, tagPALETTEENTRY color);
void __stdcall SetAlphaTable(unsigned int* table);
void __stdcall SetShadeTable(unsigned int* table);
void __stdcall SetLightTable(unsigned int* table);
void __stdcall SetGrayTable(unsigned int* table);
void __stdcall NopRetC(int, int, int);
void __stdcall SetBlueTable(unsigned int* table);
int __stdcall HasExtension(char* name);

int __stdcall SetPaletteColors(unsigned char* src, int start, int count);
int __stdcall AllocAlphaTable(Display* obj);
void __stdcall FreeAlphaTable(Display* param);
int __stdcall AllocShadeTable(Display* obj);
void __stdcall FreeShadeTable(Display* obj);
int __stdcall AllocLightTable(Display* obj);
void __stdcall FreeLightTable(Display* obj);
int __stdcall AllocGrayTable(Display* obj);
void __stdcall FreeGrayTable(Display* obj);
int __stdcall AllocBlueTable(Display* obj);
void __stdcall FreeBlueTable(Display* obj);
unsigned int* __stdcall BuildAlphaTable(unsigned char* data);
unsigned char* __stdcall BuildLightTable(tagPALETTEENTRY* palette);
unsigned char* __stdcall BuildShadeTable(tagPALETTEENTRY* palette);
char* __stdcall ChangeExtension(char* a, char* b, char* c);
char* __stdcall StripExtension(char* name);
char* __stdcall StripFileName(char* path);
char* __stdcall StripPath(char* path);

void __stdcall SetBrightness(float brightness);

#endif
