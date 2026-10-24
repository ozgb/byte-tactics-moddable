#include <stddef.h>
#include <vector>

void* __cdecl GameAllocShared(int size);
void* __cdecl GameReallocIgnoreTag(void* table, int size);
void* __cdecl operator new(unsigned int size);
void __cdecl operator delete(void* p);

struct Elem_00470f00 {
    void* p;
};

class ObjectPool {
public:
    int vtable;
    std::vector<Elem_00470f00> items;
    Elem_00470f00** slots;
    int slotSize;
    int capacity;
    int used;

    int Grow(int newCapacity, int newSlotSize);
};

// FUNCTION: 0x470c10
int ObjectPool::Grow(int newCapacity, int newSlotSize)
{
    int result = 0;
    if (newCapacity > capacity) {
        Elem_00470f00** table = (Elem_00470f00**)GameReallocIgnoreTag(slots, newCapacity * 4);
        if (table != 0) {
            Elem_00470f00 block;
            block.p = GameAllocShared((newCapacity - capacity) * newSlotSize);
            slots = table;
            if (block.p != 0) {
                int i = capacity;
                if (i < newCapacity) {
                    int offset = 0;
                    do {
                        slots[i] = (Elem_00470f00*)((char*)block.p + offset);
                        offset += newSlotSize;
                        i++;
                    } while (i < newCapacity);
                }
                items.push_back(block);
                capacity = newCapacity;
                slotSize = newSlotSize;
                result = 1;
            }
        }
    }
    else {
        result = 1;
    }
    return result;
}
