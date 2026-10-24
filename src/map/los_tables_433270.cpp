#include <vector>
#include "los_tables.h"

struct Elem_00434020 {
    unsigned short a;
    unsigned short b;
};

struct Wrap_00433270 {
    std::vector<Elem_00434020> v;
};

struct Elem_00434360 {
    Wrap_00433270 w;
};

typedef std::vector<Elem_00434360> Column_00433270;

// FUNCTION: 0x433270
void LosTables::ResizeTables(short n)
{
    Column_00433270 x;
    ((std::vector<Column_00433270>*)this)->resize(n, x);
}
