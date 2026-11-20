#ifndef ORDERS_WEAPONS_EFFECTS_H
#define ORDERS_WEAPONS_EFFECTS_H

void OrdersOnWeaponsAttacked(const void* event, unsigned int size);
void OrdersOnWeaponsHit(const void* event, unsigned int size);
void OrdersOnWeaponsParalyze(const void* event, unsigned int size);

#endif
