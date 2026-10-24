#include "win_types.h"
#include "ta_types.h"
#include "particles_471de0.h"

inline ParticleLists::~ParticleLists()
{
    for (int i = 0; i < 10; i++) {
        std::vector<Listener_00470fb0*>::iterator it = lists[i].begin();
        while (it != lists[i].end()) {
            delete *it;
            lists[i].erase(it);
        }
    }
}

#include "../../link/globals.h"

// FUNCTION: 0x471de0
void DestroyParticleLists()
{
    if (g_game->lists_38d77) {
        delete (ParticleLists*)g_game->lists_38d77;
        g_game->lists_38d77 = 0;
    }
}
