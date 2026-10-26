#ifndef ONLINE_H
#define ONLINE_H

struct LinkInfo;

int __stdcall OnlineProcessButtonCommand(int button, char* message, unsigned int size);
void __stdcall OnlineTranslate(unsigned char* text);
unsigned int __stdcall OnlineGetLinkInfo(LinkInfo* links);
void __stdcall OnlineUnload(void);
int __stdcall IsOnlineConfigLoaded(void);
int __stdcall Onl_LoadConfigFile(char* file);

#endif
