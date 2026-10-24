struct Unit;
void __stdcall CopyDwordIfNonNull(Unit**, Unit* const*);
namespace std {
inline void _Construct(Unit** dest, Unit* const& src) { CopyDwordIfNonNull(dest, &src); }
}
#include <vector>
#pragma pack(push, 1)
#include "../units/unit_def.h"
struct Owner { char pad0[0x108]; unsigned char allied[0x3e]; unsigned char index; };
#include "../units/unit.h"
#pragma pack(pop)
class GroundAllyVisitor {
public:
    virtual void CollectGroundAlly(Unit*);
    Owner* owner;
    std::vector<Unit*>* units;
    Unit* self;
};

// FUNCTION: 0x410c70
void GroundAllyVisitor::CollectGroundAlly(Unit* unit)
{
    if (((Owner*)unit->player.get())->allied[owner->index] && !(unit->def->flags1 & 0x800) && unit != self)
        units->push_back(unit);
}
