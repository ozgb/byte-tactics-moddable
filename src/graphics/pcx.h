#ifndef PCX_H
#define PCX_H

int __stdcall WritePcx(void* filename, unsigned char* data, int width, int height, unsigned char* block);
void* __stdcall Load3do(char* param);

#endif
