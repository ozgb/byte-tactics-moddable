#ifndef SEAMS_ORDERS_WORLD_H
#define SEAMS_ORDERS_WORLD_H

void OrdersInbox(const void* event, unsigned int size);
void OrdersOnCommand(const void* event, unsigned int size);
void OrdersOnGroupCommand(const void* event, unsigned int size);
void OrdersOnAiAddOrder(const void* event, unsigned int size);
void OrdersOnAiQueueBuild(const void* event, unsigned int size);
void OrdersOnAiStop(const void* event, unsigned int size);
void OrdersOnWeaponsAttacked(const void* event, unsigned int size);
void OrdersOnWeaponsParalyze(const void* event, unsigned int size);
void OrdersOnMovementOrderFlags(const void* event, unsigned int size);

#endif
