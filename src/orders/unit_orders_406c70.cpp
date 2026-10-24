#include "unit_orders_406c70.h"

// FUNCTION: 0x406c70
void __stdcall CopyDwordIfNonNull(int* dest, int* src)
{
    if (dest != 0) {
        *dest = *src;
    }
}
