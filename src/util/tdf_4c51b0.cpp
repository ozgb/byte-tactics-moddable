#include <vector>

#include "string_ref.h"

#include <new>
#include "tdf.h"

// FUNCTION: 0x4c2eb0
// FUNCTION: 0x4c51b0 ??_GTdfField@@QAEPAXI@Z
TdfFile::~TdfFile()
{
    delete root;
    root = 0;
    current = 0;
    flags = 0;
}
