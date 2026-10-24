// FUNCTION: 0x4d8310
void __cdecl CheckFillPattern(unsigned int* block, unsigned int fill, unsigned int size)
{
    unsigned int* p = block;
    while (size >= 4) {
        if (*p != fill)
            __asm int 3
        p++;
        size -= 4;
    }
    unsigned char* bytes = (unsigned char*)p;
    unsigned char* pattern = (unsigned char*)&fill;
    for (unsigned int n = size; n > 0; n--, bytes++, pattern++)
        if (*bytes != *pattern)
            __asm int 3
}
