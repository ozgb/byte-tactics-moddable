// FLAGS: /Gi
#include <vector>

typedef std::vector<int> Vec_0046e640;
typedef void (Vec_0046e640::*InsertFn_0046e640)(
    Vec_0046e640::iterator, Vec_0046e640::size_type, int const&);

void __stdcall Grow_0046e640(Vec_0046e640* v, int extra)
{
    v->reserve(extra + v->size());
}

// FUNCTION: 0x46e640 ?insert@?$vector@HV?$allocator@H@std@@@std@@QAEXPAHIABH@Z
InsertFn_0046e640 g_insert_0046e640 = &Vec_0046e640::insert;
