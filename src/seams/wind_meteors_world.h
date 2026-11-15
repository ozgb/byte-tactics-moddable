#ifndef WIND_METEORS_WORLD_H
#define WIND_METEORS_WORLD_H

struct MeteorShowerParams;

void PublishWindMeteors(int kind, const MeteorShowerParams* params);
void WindMeteorsInbox(const void* event);
void WeaponsOnMeteorSpawn(const void* event);

#endif
