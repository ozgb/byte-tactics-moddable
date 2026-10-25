#ifndef OBJECT_POOL_H
#define OBJECT_POOL_H

#include <vector>

struct PoolBlock {
    int unknown_0;
};
void __cdecl GameFreeThunk(void* p);

class ObjectPool {
public:
    std::vector<PoolBlock*> items;
    void* slots;
    int slotSize;
    int capacity;
    int used;

    ObjectPool(int initialCapacity, int initialSlotSize)
    {
        slots = 0;
        slotSize = 0;
        capacity = 0;
        used = 0;
        if (initialCapacity != 0 && initialSlotSize != 0)
            Grow(initialCapacity, initialSlotSize);
    }
    virtual ~ObjectPool()
    {
        if (slots != 0)
            GameFreeThunk(slots);
        std::vector<PoolBlock*>::iterator it = items.begin();
        while (it != items.end()) {
            GameFreeThunk(*it);
            items.erase(it);
        }
    }
    void FreeBlocks();
    int Grow(int newCapacity, int newSlotSize);
    int AllocSlot(int unused);
    void FreeSlot(int slot);
    ObjectPool* Construct(int initialCapacity, int initialSlotSize);
    void Destroy();
};

#endif
