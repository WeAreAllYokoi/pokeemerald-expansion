#ifndef GUARD_MOVE_RELEARNER_H
#define GUARD_MOVE_RELEARNER_H

#include "constants/move_relearner.h"

void TeachMoveRelearnerMove(void);
void MoveRelearnerShowHideHearts(s32 move);
void MoveRelearnerShowHideCategoryIcon(s32);
void CB2_InitLearnMove(void);
bool32 CanBoxMonRelearnMoves(struct BoxPokemon *boxMon, enum MoveRelearnerStates state);
bool32 HasMoveToRelearn(struct BoxPokemon *boxMon, enum MoveRelearnerStates state);

extern enum MoveRelearnerStates gMoveRelearnerState;
extern enum RelearnMode gRelearnMode;

// Collect a relearner list's moves or check whether that list is enabled
u32 ADMG_CollectRelearnerMoves(struct BoxPokemon *boxMon, u32 list, u32 scriptMode, enum Move *moves, u32 capacity);
u32 ADMG_IsRelearnerListActive(u32 list);

#endif //GUARD_MOVE_RELEARNER_H
