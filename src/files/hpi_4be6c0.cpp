#include <vector>

#include "../util/string_ref.h"

typedef std::vector<StringRef> Vec_004be6c0;
typedef void (Vec_004be6c0::*InsertFn_004be6c0)(
    Vec_004be6c0::iterator, Vec_004be6c0::size_type, const StringRef&);

// FUNCTION: 0x4be6c0 ?insert@?$vector@VStringRef@@V?$allocator@VStringRef@@@std@@@std@@QAEXPAVStringRef@@IABV3@@Z
InsertFn_004be6c0 g_insert_004be6c0 = &Vec_004be6c0::insert;
