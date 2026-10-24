// FLAGS: /Gi
#include <vector>

struct Element_00475ef0 {
    int data[0x11];
};

typedef std::vector<Element_00475ef0> Vec_00475ef0;
typedef void (Vec_00475ef0::*InsertFn_00475ef0)(
    Vec_00475ef0::iterator, Vec_00475ef0::size_type, Element_00475ef0 const&);

void __stdcall Assign_00475ef0(Vec_00475ef0* to, const Vec_00475ef0* from)
{
    *to = *from;
}

// FUNCTION: 0x475ef0 ?insert@?$vector@UElement_00475ef0@@V?$allocator@UElement_00475ef0@@@std@@@std@@QAEXPAUElement_00475ef0@@IABU3@@Z
InsertFn_00475ef0 g_insert_00475ef0 = &Vec_00475ef0::insert;
