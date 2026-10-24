#include "../state/sim_allocator.h"
#include "win_types.h"
#include <memory.h>
#include <math.h>
#include "ta_types.h"

#include <new>
inline void __stdcall CopyOneByte(Elem_0040d4f0* dest, const Elem_0040d4f0* src) { new (dest) Elem_0040d4f0(*src); }
inline void __stdcall CopyDwordPair(Elem_0040cc40* dest, const Elem_0040cc40* src) { new (dest) Elem_0040cc40(*src); }
namespace std {
inline void _Construct(Elem_0040d4f0* dest, const Elem_0040d4f0& src) { CopyOneByte(dest, &src); }
inline void _Construct(Elem_0040cc40* dest, const Elem_0040cc40& src) { CopyDwordPair(dest, &src); }
}

typedef SimVector<Unit*> Vec_0040c530;
typedef SimVector<Elem_0040cc40> Vec_0040c580;
typedef SimVector<Elem_0040cfb0> Vec_0040c5d0;
typedef SimVector<Elem_0040d4f0> Vec_0040c600;
typedef SimVector<Elem_0040d550> Vec_0040c7f0;
typedef SimVector<short> Vec_0040d000;
typedef SimVector<unsigned char> Vec_0040d290;

typedef SimVector<Vec_0040c530> Outer_0040c530;
typedef Outer_0040c530& (Outer_0040c530::*AssignFn_0040c530)(const Outer_0040c530&);
// FUNCTION: 0x40c530 ??1?$vector@PAUUnit@@V?$allocator@PAUUnit@@@std@@@std@@QAE@XZ
AssignFn_0040c530 g_assign_0040c530 = &Outer_0040c530::operator=;

typedef Vec_0040c530::size_type (Vec_0040c530::*SizeFn_0040c560)() const;
// FUNCTION: 0x40c560 ?size@?$vector@PAUUnit@@V?$allocator@PAUUnit@@@std@@@std@@QBEIXZ
SizeFn_0040c560 g_size_0040c560 = &Vec_0040c530::size;

typedef SimVector<Vec_0040c580> Outer_0040c580;
typedef Outer_0040c580& (Outer_0040c580::*AssignFn_0040c580)(const Outer_0040c580&);
// FUNCTION: 0x40c580 ??1?$vector@UElem_0040cc40@@V?$allocator@UElem_0040cc40@@@std@@@std@@QAE@XZ
AssignFn_0040c580 g_assign_0040c580 = &Outer_0040c580::operator=;

typedef Vec_0040c580::size_type (Vec_0040c580::*SizeFn_0040c5b0)() const;
// FUNCTION: 0x40c5b0 ?size@?$vector@UElem_0040cc40@@V?$allocator@UElem_0040cc40@@@std@@@std@@QBEIXZ
SizeFn_0040c5b0 g_size_0040c5b0 = &Vec_0040c580::size;

typedef SimVector<Vec_0040c5d0> Outer_0040c5d0;
typedef Outer_0040c5d0& (Outer_0040c5d0::*AssignFn_0040c5d0)(const Outer_0040c5d0&);
// FUNCTION: 0x40c5d0 ??1?$vector@UElem_0040cfb0@@V?$allocator@UElem_0040cfb0@@@std@@@std@@QAE@XZ
AssignFn_0040c5d0 g_assign_0040c5d0 = &Outer_0040c5d0::operator=;

typedef void (Vec_0040c600::*ResizeFn_0040c600)(unsigned int, const Elem_0040d4f0&);
// FUNCTION: 0x40c600 ?resize@?$vector@UElem_0040d4f0@@V?$allocator@UElem_0040d4f0@@@std@@@std@@QAEXIABUElem_0040d4f0@@@Z
ResizeFn_0040c600 g_resize_0040c600 = &Vec_0040c600::resize;

typedef void (Vec_0040c7f0::*ResizeFn_0040c7f0)(unsigned int, const Elem_0040d550&);
// FUNCTION: 0x40c7f0 ?resize@?$vector@UElem_0040d550@@V?$allocator@UElem_0040d550@@@std@@@std@@QAEXIABUElem_0040d550@@@Z
ResizeFn_0040c7f0 g_resize_0040c7f0 = &Vec_0040c7f0::resize;

typedef Vec_0040c530::iterator (Vec_0040c530::*EraseFn_0040c9f0)(
    Vec_0040c530::iterator, Vec_0040c530::iterator);
// FUNCTION: 0x40c9f0 ?erase@?$vector@PAUUnit@@V?$allocator@PAUUnit@@@std@@@std@@QAEPAPAUUnit@@PAPAU3@0@Z
EraseFn_0040c9f0 g_erase_0040c9f0 = &Vec_0040c530::erase;

typedef Vec_0040c580::size_type (Vec_0040c580::*CapacityFn_0040ca30)() const;
// FUNCTION: 0x40ca30 ?capacity@?$vector@UElem_0040cc40@@V?$allocator@UElem_0040cc40@@@std@@@std@@QBEIXZ
CapacityFn_0040ca30 g_capacity_0040ca30 = &Vec_0040c580::capacity;

typedef Vec_0040c580::iterator (Vec_0040c580::*InsertFn_0040ca50)(Vec_0040c580::iterator, const Elem_0040cc40&);
// FUNCTION: 0x40ca50 ?insert@?$vector@UElem_0040cc40@@V?$allocator@UElem_0040cc40@@@std@@@std@@QAEPAUElem_0040cc40@@PAU3@ABU3@@Z
InsertFn_0040ca50 g_insert_0040ca50 = &Vec_0040c580::insert;

typedef void (Vec_0040c580::*DestroyFn_0040cc30)(Vec_0040c580::iterator, Vec_0040c580::iterator);
typedef Vec_0040c580::iterator (Vec_0040c580::*UcopyFn_0040cc40)(
    Vec_0040c580::const_iterator, Vec_0040c580::const_iterator, Vec_0040c580::iterator);
typedef void (Vec_0040c580::*UfillFn_0040d5b0)(
    Vec_0040c580::iterator, Vec_0040c580::size_type, const Elem_0040cc40&);

struct Access_0040c580 : Vec_0040c580 {
    static DestroyFn_0040cc30 destroy;
    static UcopyFn_0040cc40 ucopy;
    static UfillFn_0040d5b0 ufill;
};

// FUNCTION: 0x40cc30 ?_Destroy@?$vector@UElem_0040cc40@@V?$allocator@UElem_0040cc40@@@std@@@std@@IAEXPAUElem_0040cc40@@0@Z
DestroyFn_0040cc30 Access_0040c580::destroy = &Access_0040c580::_Destroy;

// FUNCTION: 0x40cc40 ?_Ucopy@?$vector@UElem_0040cc40@@V?$allocator@UElem_0040cc40@@@std@@@std@@IAEPAUElem_0040cc40@@PBU3@0PAU3@@Z
UcopyFn_0040cc40 Access_0040c580::ucopy = &Access_0040c580::_Ucopy;

typedef Vec_0040c5d0::size_type (Vec_0040c5d0::*SizeFn_0040cc80)() const;
// FUNCTION: 0x40cc80 ?size@?$vector@UElem_0040cfb0@@V?$allocator@UElem_0040cfb0@@@std@@@std@@QBEIXZ
SizeFn_0040cc80 g_size_0040cc80 = &Vec_0040c5d0::size;

typedef void (Vec_0040c5d0::*InsertFn_0040cca0)(Vec_0040c5d0::iterator, unsigned int, const Elem_0040cfb0&);

void __stdcall Grow_0040cca0(Vec_0040c5d0* v, int extra)
{
    v->reserve(extra + v->size());
}

// FUNCTION: 0x40cca0 ?insert@?$vector@UElem_0040cfb0@@V?$allocator@UElem_0040cfb0@@@std@@@std@@QAEXPAUElem_0040cfb0@@IABU3@@Z
InsertFn_0040cca0 g_insert_0040cca0 = &Vec_0040c5d0::insert;

typedef Vec_0040c5d0::iterator (Vec_0040c5d0::*EraseFn_0040cfb0)(
    Vec_0040c5d0::iterator, Vec_0040c5d0::iterator);
// FUNCTION: 0x40cfb0 ?erase@?$vector@UElem_0040cfb0@@V?$allocator@UElem_0040cfb0@@@std@@@std@@QAEPAUElem_0040cfb0@@PAU3@0@Z
EraseFn_0040cfb0 g_erase_0040cfb0 = &Vec_0040c5d0::erase;

typedef Vec_0040d000::size_type (Vec_0040d000::*SizeFn_0040d000)() const;
// FUNCTION: 0x40d000 ?size@?$vector@FV?$allocator@F@std@@@std@@QBEIXZ
SizeFn_0040d000 g_size_0040d000 = &Vec_0040d000::size;

typedef void (Vec_0040d000::*InsertFn_0040d020)(
    Vec_0040d000::iterator, Vec_0040d000::size_type, const short&);
// FUNCTION: 0x40d020 ?insert@?$vector@FV?$allocator@F@std@@@std@@QAEXPAFIABF@Z
InsertFn_0040d020 g_insert_0040d020 = &Vec_0040d000::insert;

typedef Vec_0040d000::iterator (Vec_0040d000::*EraseFn_0040d240)(
    Vec_0040d000::iterator, Vec_0040d000::iterator);
// FUNCTION: 0x40d240 ?erase@?$vector@FV?$allocator@F@std@@@std@@QAEPAFPAF0@Z
EraseFn_0040d240 g_erase_0040d240 = &Vec_0040d000::erase;

typedef void (Vec_0040d000::*DestroyFn_0040d280)(Vec_0040d000::iterator, Vec_0040d000::iterator);

struct Access_0040d000 : Vec_0040d000 {
    static DestroyFn_0040d280 destroy;
};

// FUNCTION: 0x40d280 ?_Destroy@?$vector@FV?$allocator@F@std@@@std@@IAEXPAF0@Z
DestroyFn_0040d280 Access_0040d000::destroy = &Access_0040d000::_Destroy;

// FUNCTION: 0x40d290 ?insert@?$vector@EV?$allocator@E@std@@@std@@QAEXPAEIABE@Z
void (SimVector<unsigned char>::*g_insert_0040d290)(unsigned char*, unsigned int,
                                                      const unsigned char&) = &SimVector<unsigned char>::insert;

typedef Vec_0040d290::iterator (Vec_0040d290::*EraseFn_0040d470)(
    Vec_0040d290::iterator, Vec_0040d290::iterator);
// FUNCTION: 0x40d470 ?erase@?$vector@EV?$allocator@E@std@@@std@@QAEPAEPAE0@Z
EraseFn_0040d470 g_erase_0040d470 = &Vec_0040d290::erase;

typedef void (Vec_0040d290::*DestroyFn_0040d4a0)(Vec_0040d290::iterator, Vec_0040d290::iterator);

struct Access_0040d290 : Vec_0040d290 {
    static DestroyFn_0040d4a0 destroy;
};

// FUNCTION: 0x40d4a0 ?_Destroy@?$vector@EV?$allocator@E@std@@@std@@IAEXPAE0@Z
DestroyFn_0040d4a0 Access_0040d290::destroy = &Access_0040d290::_Destroy;

typedef void (Vec_0040c600::*DestroyFn_0040d4b0)(Vec_0040c600::iterator, Vec_0040c600::iterator);
typedef Vec_0040c600::iterator (Vec_0040c600::*UcopyFn_0040d4f0)(
    Vec_0040c600::const_iterator, Vec_0040c600::const_iterator, Vec_0040c600::iterator);
typedef void (Vec_0040c600::*UfillFn_0040d520)(
    Vec_0040c600::iterator, Vec_0040c600::size_type, const Elem_0040d4f0&);

struct Access_0040c600 : Vec_0040c600 {
    static DestroyFn_0040d4b0 destroy;
    static UcopyFn_0040d4f0 ucopy;
    static UfillFn_0040d520 ufill;
};

// FUNCTION: 0x40d4b0 ?_Destroy@?$vector@UElem_0040d4f0@@V?$allocator@UElem_0040d4f0@@@std@@@std@@IAEXPAUElem_0040d4f0@@0@Z
DestroyFn_0040d4b0 Access_0040c600::destroy = &Access_0040c600::_Destroy;

typedef Vec_0040c7f0::size_type (Vec_0040c7f0::*SizeFn_0040d4c0)() const;
// FUNCTION: 0x40d4c0 ?size@?$vector@UElem_0040d550@@V?$allocator@UElem_0040d550@@@std@@@std@@QBEIXZ
SizeFn_0040d4c0 g_size_0040d4c0 = &Vec_0040c7f0::size;

typedef void (Vec_0040c7f0::*DestroyFn_0040d4e0)(Vec_0040c7f0::iterator, Vec_0040c7f0::iterator);
typedef Vec_0040c7f0::iterator (Vec_0040c7f0::*UcopyFn_0040d550)(
    Vec_0040c7f0::const_iterator, Vec_0040c7f0::const_iterator, Vec_0040c7f0::iterator);
typedef void (Vec_0040c7f0::*UfillFn_0040d580)(
    Vec_0040c7f0::iterator, Vec_0040c7f0::size_type, const Elem_0040d550&);

struct Access_0040c7f0 : Vec_0040c7f0 {
    static DestroyFn_0040d4e0 destroy;
    static UcopyFn_0040d550 ucopy;
    static UfillFn_0040d580 ufill;
};

// FUNCTION: 0x40d4e0 ?_Destroy@?$vector@UElem_0040d550@@V?$allocator@UElem_0040d550@@@std@@@std@@IAEXPAUElem_0040d550@@0@Z
DestroyFn_0040d4e0 Access_0040c7f0::destroy = &Access_0040c7f0::_Destroy;

// FUNCTION: 0x40d4f0 ?_Ucopy@?$vector@UElem_0040d4f0@@V?$allocator@UElem_0040d4f0@@@std@@@std@@IAEPAUElem_0040d4f0@@PBU3@0PAU3@@Z
UcopyFn_0040d4f0 Access_0040c600::ucopy = &Access_0040c600::_Ucopy;

// FUNCTION: 0x40d520 ?_Ufill@?$vector@UElem_0040d4f0@@V?$allocator@UElem_0040d4f0@@@std@@@std@@IAEXPAUElem_0040d4f0@@IABU3@@Z
UfillFn_0040d520 Access_0040c600::ufill = &Access_0040c600::_Ufill;

// FUNCTION: 0x40d550 ?_Ucopy@?$vector@UElem_0040d550@@V?$allocator@UElem_0040d550@@@std@@@std@@IAEPAUElem_0040d550@@PBU3@0PAU3@@Z
UcopyFn_0040d550 Access_0040c7f0::ucopy = &Access_0040c7f0::_Ucopy;

// FUNCTION: 0x40d580 ?_Ufill@?$vector@UElem_0040d550@@V?$allocator@UElem_0040d550@@@std@@@std@@IAEXPAUElem_0040d550@@IABU3@@Z
UfillFn_0040d580 Access_0040c7f0::ufill = &Access_0040c7f0::_Ufill;

// FUNCTION: 0x40d5b0 ?_Ufill@?$vector@UElem_0040cc40@@V?$allocator@UElem_0040cc40@@@std@@@std@@IAEXPAUElem_0040cc40@@IABU3@@Z
UfillFn_0040d5b0 Access_0040c580::ufill = &Access_0040c580::_Ufill;
