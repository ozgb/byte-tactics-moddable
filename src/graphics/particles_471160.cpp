#include <vector>

class Listener_00471120 {
public:
    virtual ~Listener_00471120();
    virtual void Notify(void* param);
};

struct Elem_00473500 {
    Listener_00471120* p;
};

class ParticleLists {
public:
    std::vector<Elem_00473500> lists[1];

    void DrawList(void* param, short index);
    void AddToList(Elem_00473500 x, short index);
};

// FUNCTION: 0x471160
void ParticleLists::AddToList(Elem_00473500 x, short index)
{
    if (lists[index].size() > 400) {
        delete lists[index][0].p;
        lists[index].erase(lists[index].begin());
    }
    lists[index].push_back(x);
}
