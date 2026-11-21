#ifndef SEAMS_SCRIPTS_WORLD_H
#define SEAMS_SCRIPTS_WORLD_H

void ScriptsInbox(const void* event, unsigned int size);
void ScriptsOnWeaponsStartScript(const void* event, unsigned int size);
void ScriptsOnWeaponsDetachPiece(const void* event, unsigned int size);
void ScriptsOnOrdersStartScript(const void* event, unsigned int size);
void ScriptsOnMovementStartScript(const void* event, unsigned int size);
void ScriptsOnEconomyExtractor(const void* event, unsigned int size);
void ScriptsOnFeaturesFreeState(const void* event, unsigned int size);
void ScriptsOnFeaturesInitState(const void* event, unsigned int size);
void ScriptsOnTargetingCleared(const void* event, unsigned int size);

#endif
