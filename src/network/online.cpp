#include "../../interfaces/host.h"

#include "../../link/globals.h"
#include "online.h"

void __stdcall Translate(unsigned char* text);

// FUNCTION: 0x45b480
void __stdcall OnlineTranslate(unsigned char* text)
{
    Translate(text);
}

// FUNCTION: 0x45b640
void OnlineUnload(void)
{
    host_net_online_unload();
}

// FUNCTION: 0x45b660
int IsOnlineConfigLoaded(void)
{
    return g_onlineConfigLoaded;
}
