#include "../util/hapi_bank.h"

char* HarnessBankString(HapiBank* bank, char* item, char* def)
{
    return bank->GetStringItem(item, def);
}
