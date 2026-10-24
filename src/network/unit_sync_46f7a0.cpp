// FLAGS: /Gi
#include <vector>

struct SyncPlayerRecord {
public:
    char unknown_0[0x5c];
    SyncPlayerRecord& operator=(const SyncPlayerRecord& rhs);
};

class UnitSyncPlayer : public SyncPlayerRecord {
public:
    UnitSyncPlayer(const UnitSyncPlayer& other);
    ~UnitSyncPlayer();
    UnitSyncPlayer& operator=(const UnitSyncPlayer& rhs)
    {
        return (UnitSyncPlayer&)SyncPlayerRecord::operator=(rhs);
    }
};

typedef std::vector<UnitSyncPlayer> Vec_0046f7a0;
typedef void (Vec_0046f7a0::*InsertFn_0046f7a0)(
    Vec_0046f7a0::iterator, Vec_0046f7a0::size_type,
    const UnitSyncPlayer&);

// FUNCTION: 0x46f7a0 ?insert@?$vector@VUnitSyncPlayer@@V?$allocator@VUnitSyncPlayer@@@std@@@std@@QAEXPAVUnitSyncPlayer@@IABV3@@Z
InsertFn_0046f7a0 g_insert_0046f7a0 = &Vec_0046f7a0::insert;
