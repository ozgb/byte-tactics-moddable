#ifndef PALETTE_TABLES_H
#define PALETTE_TABLES_H

unsigned int* __stdcall LoadAlphaTable(void* palette);
unsigned int* __stdcall LoadShadeTable(void* palette);
unsigned int* __stdcall LoadLightTable(void* palette);

#endif
