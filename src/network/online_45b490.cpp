#include <string.h>
#include "../../interfaces/host.h"
#include "online_45b490.h"

char* __stdcall Translate(char* text);

#include "link_info.h"

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
