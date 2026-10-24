#include <stdio.h>
#include <string.h>
#include "../../interfaces/host.h"

char* __stdcall Translate(char* text);
#include "online.h"

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
