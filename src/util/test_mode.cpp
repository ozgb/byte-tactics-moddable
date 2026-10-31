#include "../../interfaces/services.h"

static int s_testMode = 0;

int HarnessTestMode()
{
    return s_testMode;
}

void EnterHarnessTestMode()
{
    s_testMode = 1;
}
