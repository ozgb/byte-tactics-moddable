#ifndef KEYS_H
#define KEYS_H

void __stdcall SelectStopOrder(void);
void __stdcall SetOrSelectStopOrder(int set);
void __stdcall BuildScreenshotPath(char* out, const char* dir, const char* name, const char* ext);
void __stdcall HandleGameKey(void);

#endif
