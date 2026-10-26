#ifndef ADMG_REMATCH_BATTLE_H
#define ADMG_REMATCH_BATTLE_H

#include "admg_rematch_state.h"

/* Original provider policy is separate from party selection. NPC-local
 * readiness never becomes a second trainer progression authority. */
ADMG_NATIVE_ENTRY bool8 ADMG_TryIsNativeNpcReady(const struct ADMG_RematchContext *context,
    u32 trainerId, u32 localId, bool8 *ready);
ADMG_NATIVE_ENTRY bool8 ADMG_TryShouldRematchNativeNpc(const struct ADMG_RematchContext *context,
    u32 baseTrainerId, u32 localId, bool8 *shouldRematch);
ADMG_NATIVE_ENTRY bool8 ADMG_TryClearNativeNpcReadiness(const struct ADMG_RematchContext *context,
    u32 trainerId, u32 localId);

/* Transient NPC association only. Direct trainer battles may intentionally
 * omit it; canonical identity and defeat flags never depend on this capture. */
ADMG_NATIVE_ENTRY void ADMG_DiscardNativeBattleAssociation(void);
/* Consume one configured capture at battle start. A subsequent direct start
 * cannot inherit an active battle's NPC association. */
ADMG_NATIVE_ENTRY void ADMG_BeginNativeTrainerBattle(u32 trainerId);
ADMG_NATIVE_ENTRY bool8 ADMG_TryBindNativeBattleNpc(const struct ADMG_RematchContext *context,
    u32 trainerId, u32 localId);
ADMG_NATIVE_ENTRY bool8 ADMG_TryGetNativeBattleNpc(u32 trainerId,
    struct ADMG_RematchContext *context, u32 *localId);
ADMG_NATIVE_ENTRY bool8 ADMG_TryCompleteNativeBattleNpc(u32 trainerId, u32 won);

#endif
