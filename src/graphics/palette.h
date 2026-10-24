#ifndef PALETTE_H
#define PALETTE_H

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

#endif
