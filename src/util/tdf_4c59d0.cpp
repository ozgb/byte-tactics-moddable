#include <stddef.h>
#include <vector>

#include "string_ref.h"
#include "tdf.h"

namespace std {
inline void _Destroy(TdfField* p)
{
    ((StringRef*)&p->value)->ReleaseRef();
    ((StringRef*)&p->key)->ReleaseRef();
}
}

class TdfFieldVector : public std::vector<TdfField> {
public:
    int GetPairCount(void);

    iterator Insert(iterator p, const TdfField& x);
};

// FUNCTION: 0x4c59d0
TdfField* TdfFieldVector::Insert(iterator p, const TdfField& x)
{
    return insert(p, x);
}
#pragma auto_inline(off)
// FUNCTION: 0x4c5ba0
int TdfFieldVector::GetPairCount(void)
{
    return size();
}
#pragma auto_inline(on)
