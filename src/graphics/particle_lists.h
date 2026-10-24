#ifndef PARTICLE_LISTS_H
#define PARTICLE_LISTS_H

#include <vector>

class ParticleSystem;

class ParticleLists {
public:
    std::vector<ParticleSystem*> lists[10];

    void Add(short index, ParticleSystem* p)
    {
        if (lists[index].size() > 400) {
            delete lists[index][0];
            lists[index].erase(lists[index].begin());
        }
        lists[index].push_back(p);
    }

    ParticleLists();
    ~ParticleLists();
    void UpdateAll();
    void DrawAll(void* param);
    void DrawList(void* param, short index);
    void AddTeleportParticles(int posA, int posB, int lifetime, short index);
    void AddNanoParticles(int posA, int posB, int lifetime, short index);
    void AddThrustParticles(int posA, int posB, int period, int lifetime, short index);
    void AddWakeParticles(int posA, int posB, int period, int lifetime,
                          short index, int isAscending);
};

#endif
