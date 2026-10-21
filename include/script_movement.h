#ifndef GUARD_SCRIPT_MOVEMENT_H
#define GUARD_SCRIPT_MOVEMENT_H

bool8 ScriptMovement_StartObjectMovementScript(u8 localId, MapId mapId, const u8 *movementScript);
bool8 ScriptMovement_IsObjectMovementFinished(u8 localId, MapId mapId);
bool32 ScriptMovement_IsAllObjectMovementFinished(void);
void ScriptMovement_UnfreezeObjectEvents(void);

#endif // GUARD_SCRIPT_MOVEMENT_H
