#include <stdio.h>
#include <string.h>
#include "../../interfaces/host.h"

#include "../../link/globals.h"
#include "link_info.h"
#include "online.h"

void __stdcall Translate(unsigned char* text);
char* __stdcall Translate(char* text);

// FUNCTION: 0x45b250
int __stdcall OnlineProcessButtonCommand(int button, char* message, unsigned int size)
{
    int result = 2;
    try {
        if (button != -1)
            result = host_net_online_button(button, message, size, (void*)OnlineTranslate);
    } catch (...) {
        if (message && size > 0) {
            *message = 0;
            strncat(message, Translate("Fault while trying to execute button action."), size - 1);
        }
        result = 2;
    }
    if (result == 1) {
        char text[0x78];
        char* format = Translate("Please insert the Installation CD (Disc 1) and select%s\"%s\" again.");
        if (_snprintf(text, sizeof(text), format, "\n\n", message) < 0)
            text[sizeof(text) - 1] = 0;
        *message = 0;
        strncat(message, text, size - 1);
    }
    return result;
}

// FUNCTION: 0x45b480
void __stdcall OnlineTranslate(unsigned char* text)
{
    Translate(text);
}

// FUNCTION: 0x45b490
unsigned int __stdcall OnlineGetLinkInfo(LinkInfo* links)
{
    unsigned int count = 0;
    try {
        count = host_net_online_link_info(links);
        for (unsigned int i = 0; i < count; i++) {
            if (links[i].id != -1) {
                char* name = Translate(links[i].name);
                if (name != links[i].name) {
                    strncpy(links[i].name, name, sizeof(links[i].name));
                    links[i].name[sizeof(links[i].name) - 1] = 0;
                }
            }
        }
    } catch (...) {
        count = 0;
    }
    return count;
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

// FUNCTION: 0x45b670
int __stdcall Onl_LoadConfigFile(char* file)
{
    g_onlineConfigLoaded = 0;
    memset(DAT_00512c80, 0, sizeof(DAT_00512c80));
    try {
        g_onlineConfigLoaded = host_net_online_load_config(file, DAT_00512c80, sizeof(DAT_00512c80));
    } catch (...) {
        g_onlineConfigLoaded = 0;
    }
    return g_onlineConfigLoaded;
}
