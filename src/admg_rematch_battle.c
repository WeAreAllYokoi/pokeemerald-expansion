#include "global.h"
#include "admg_rematch_battle.h"
#include "admg_state.h"

static EWRAM_DATA bool8 sNativeBattleBound = FALSE;
static EWRAM_DATA bool8 sNativeBattleStarted = FALSE;
static EWRAM_DATA struct ADMG_RematchContext sNativeBattleContext;
static EWRAM_DATA u32 sNativeBattleLocalId = 0;
static EWRAM_DATA u16 sNativeBattleFamily = 0;

static bool8 RejectNpcBattle(u32 id, const char *operation)
{
    ADMG_StateAccessRejected(id, operation);
    return FALSE;
}

static bool8 ReadAssociatedReadiness(const struct ADMG_RematchContext *context,
    u32 trainerId, u32 localId, bool8 baseOnly, bool8 *associated, u8 *value)
{
    u16 family, npcFamily, npcTrainer;
    u8 position;
    u64 generation;
    bool8 found;
    if (!ADMG_IsNativeRematchContextCurrent(context)
     || localId == 0 || localId >= ADMG_NATIVE_READINESS_COUNT)
        return RejectNpcBattle(localId, __func__);
    generation = ADMG_BeginStateAccessCheck();
    found = ADMG_TryFindNativeRematch(trainerId, baseOnly, &family, &position);
    if (!ADMG_EndStateAccessCheck(generation))
        return FALSE;
    if (!found)
    {
        /* Known trainers outside this provider have no fabricated readiness. */
        *associated = FALSE;
        *value = 0;
        return TRUE;
    }
    if (!ADMG_TryGetNativeRematchNpcTrainer(context->mapGroup, context->mapNum, localId, &npcTrainer)
     || !ADMG_TryFindNativeRematch(npcTrainer, FALSE, &npcFamily, &position)
     || npcFamily != family)
        return RejectNpcBattle(trainerId, __func__);
    if (!ADMG_TryReadNativeReadiness(context, localId, value))
        return FALSE;
    *associated = TRUE;
    return TRUE;
}

bool8 ADMG_TryIsNativeNpcReady(const struct ADMG_RematchContext *context,
    u32 trainerId, u32 localId, bool8 *ready)
{
    bool8 associated;
    u8 value;
    if (ready == NULL)
        return RejectNpcBattle(trainerId, __func__);
    if (!ReadAssociatedReadiness(context, trainerId, localId, FALSE, &associated, &value))
        return FALSE;
    *ready = associated && value != 0;
    return TRUE;
}

bool8 ADMG_TryShouldRematchNativeNpc(const struct ADMG_RematchContext *context,
    u32 baseTrainerId, u32 localId, bool8 *shouldRematch)
{
    bool8 associated, fought;
    u8 value;
    u16 flag;
    if (shouldRematch == NULL)
        return RejectNpcBattle(baseTrainerId, __func__);
    if (!ReadAssociatedReadiness(context, baseTrainerId, localId, TRUE, &associated, &value))
        return FALSE;
    if (!associated || value != 0)
    {
        *shouldRematch = associated;
        return TRUE;
    }
    if (!ADMG_TryGetTrainerDefeatFlagId(baseTrainerId, &flag)
     || !ADMG_TryReadFlag(flag, &fought))
        return FALSE;
    *shouldRematch = fought;
    return TRUE;
}

bool8 ADMG_TryClearNativeNpcReadiness(const struct ADMG_RematchContext *context,
    u32 trainerId, u32 localId)
{
    bool8 associated;
    u8 value;
    if (!ReadAssociatedReadiness(context, trainerId, localId, FALSE, &associated, &value))
        return FALSE;
    if (!associated)
        return TRUE;
    return ADMG_TryWriteNativeReadiness(context, localId, 0);
}

void ADMG_DiscardNativeBattleAssociation(void)
{
    sNativeBattleBound = FALSE;
    sNativeBattleStarted = FALSE;
}

void ADMG_BeginNativeTrainerBattle(u32 trainerId)
{
    struct ADMG_RematchContext context;
    u32 localId;
    if (!sNativeBattleBound)
        return;
    if (sNativeBattleStarted || trainerId < ADMG_KANTO_TRAINER_FIRST
     || trainerId > ADMG_KANTO_TRAINER_LAST
     || !ADMG_TryGetNativeBattleNpc(trainerId, &context, &localId))
    {
        ADMG_DiscardNativeBattleAssociation();
        return;
    }
    sNativeBattleStarted = TRUE;
}

bool8 ADMG_TryBindNativeBattleNpc(const struct ADMG_RematchContext *context,
    u32 trainerId, u32 localId)
{
    bool8 associated;
    u8 value, position;
    u16 family;
    ADMG_DiscardNativeBattleAssociation();
    if (!ReadAssociatedReadiness(context, trainerId, localId, FALSE, &associated, &value))
        return FALSE;
    if (!associated)
        return TRUE;
    if (!ADMG_TryFindNativeRematch(trainerId, FALSE, &family, &position)
     || !ADMG_IsNativeRematchContextCurrent(context))
        return FALSE;
    sNativeBattleContext = *context;
    sNativeBattleLocalId = localId;
    sNativeBattleFamily = family;
    sNativeBattleBound = TRUE;
    return TRUE;
}

bool8 ADMG_TryGetNativeBattleNpc(u32 trainerId,
    struct ADMG_RematchContext *context, u32 *localId)
{
    u16 family;
    u8 position;
    if (!sNativeBattleBound)
        return FALSE;
    if (context == NULL || localId == NULL
     || !ADMG_IsNativeRematchContextCurrent(&sNativeBattleContext)
     || !ADMG_TryFindNativeRematch(trainerId, FALSE, &family, &position)
     || family != sNativeBattleFamily)
        return RejectNpcBattle(trainerId, __func__);
    *context = sNativeBattleContext;
    *localId = sNativeBattleLocalId;
    return TRUE;
}

bool8 ADMG_TryCompleteNativeBattleNpc(u32 trainerId, u32 won)
{
    struct ADMG_RematchContext context;
    struct ADMG_TrainerRef ref;
    u32 localId;
    bool8 result;
    if (won > TRUE)
        return RejectNpcBattle(won, __func__);
    if (!ADMG_TryResolveTrainer(trainerId, &ref) || ref.owner != ADMG_TRAINER_KANTO)
        return RejectNpcBattle(trainerId, __func__);
    if (!sNativeBattleBound)
        return TRUE;
    if (!won)
    {
        ADMG_DiscardNativeBattleAssociation();
        return TRUE;
    }
    if (!ADMG_TryGetNativeBattleNpc(trainerId, &context, &localId))
    {
        ADMG_DiscardNativeBattleAssociation();
        return FALSE;
    }
    result = ADMG_TryClearNativeNpcReadiness(&context, trainerId, localId);
    ADMG_DiscardNativeBattleAssociation();
    return result;
}
