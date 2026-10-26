#include "global.h"
#include "data.h"
#include "admg_state.h"
#include "admg_trainer.h"
#include "constants/battle.h"
#include "constants/flags.h"

static bool8 ADMG_RejectTrainer(u32 identifier, const char *operation)
{
    ADMG_StateAccessRejected(identifier, operation);
    return FALSE;
}

ADMG_NATIVE_ENTRY bool8 ADMG_TryResolveTrainer(u32 trainerId, struct ADMG_TrainerRef *ref)
{
    struct ADMG_TrainerRef resolved;
    if (ref == NULL || trainerId > 0xFFFF)
        return ADMG_RejectTrainer(trainerId, __func__);
    resolved.canonicalId = trainerId;
    if (trainerId < TRAINERS_COUNT_EMERALD)
    {
        resolved.owner = ADMG_TRAINER_HOENN;
        resolved.nativeIndex = trainerId;
    }
    else if (trainerId > ADMG_KANTO_TRAINER_FIRST
          && trainerId < ADMG_KANTO_TRAINER_FIRST + ADMG_KANTO_TRAINERS_COUNT)
    {
        resolved.owner = ADMG_TRAINER_KANTO;
        resolved.nativeIndex = trainerId - ADMG_KANTO_TRAINER_FIRST;
    }
    else if (trainerId > MAX_TRAINERS_COUNT_EMERALD + PARTNER_NONE
          && trainerId < MAX_TRAINERS_COUNT_EMERALD + PARTNER_COUNT)
    {
        resolved.owner = ADMG_TRAINER_PARTNER;
        resolved.nativeIndex = trainerId - MAX_TRAINERS_COUNT_EMERALD;
    }
    else
        return ADMG_RejectTrainer(trainerId, __func__);
    *ref = resolved;
    return TRUE;
}

ADMG_NATIVE_ENTRY bool8 ADMG_TryGetOwnedTrainer(u32 owner, u32 index, u32 difficulty, const struct Trainer **trainer)
{
    const struct Trainer *selected;
    if (trainer == NULL || difficulty >= DIFFICULTY_COUNT)
        return ADMG_RejectTrainer(difficulty, __func__);
    switch (owner)
    {
    case ADMG_TRAINER_HOENN:
        if (index >= TRAINERS_COUNT_EMERALD)
            return ADMG_RejectTrainer(index, __func__);
        selected = &gADMG_HoennTrainers[difficulty][index];
        break;
    case ADMG_TRAINER_KANTO:
        if (index >= ADMG_KANTO_TRAINERS_COUNT)
            return ADMG_RejectTrainer(index, __func__);
        selected = &gADMG_KantoTrainers[difficulty][index];
        break;
    case ADMG_TRAINER_PARTNER:
        if (index >= PARTNER_COUNT)
            return ADMG_RejectTrainer(index, __func__);
        selected = &gBattlePartners[difficulty][index];
        break;
    default:
        return ADMG_RejectTrainer(owner, __func__);
    }
    *trainer = selected;
    return TRUE;
}

ADMG_NATIVE_ENTRY bool8 ADMG_TryGetTrainerRow(u32 trainerId, u32 difficulty, const struct Trainer **trainer)
{
    struct ADMG_TrainerRef ref;
    if (!ADMG_TryResolveTrainer(trainerId, &ref))
        return FALSE;
    if (trainer == NULL || difficulty >= DIFFICULTY_COUNT)
        return ADMG_RejectTrainer(difficulty, __func__);
    /* Preserve the existing TESTING override for legacy named accessors.
     * Explicit owned queries can still inspect the complete original table. */
    if (ref.owner == ADMG_TRAINER_HOENN)
    {
        *trainer = &gTrainers[difficulty][ref.nativeIndex];
        return TRUE;
    }
    return ADMG_TryGetOwnedTrainer(ref.owner, ref.nativeIndex, difficulty, trainer);
}

ADMG_NATIVE_ENTRY bool8 ADMG_TryGetTrainerStruct(u32 trainerId, const struct Trainer **trainer)
{
    struct ADMG_TrainerRef ref;
    const struct Trainer *selected;
    u32 difficulty;
    u64 generation;
    if (trainer == NULL)
        return ADMG_RejectTrainer(trainerId, __func__);
    if (!ADMG_TryResolveTrainer(trainerId, &ref))
        return FALSE;
    if (gIsDebugBattle)
    {
        selected = GetDebugAiTrainer();
        if (selected == NULL)
            return ADMG_RejectTrainer(trainerId, __func__);
        *trainer = selected;
        return TRUE;
    }
    generation = ADMG_BeginStateAccessCheck();
    if (ref.owner == ADMG_TRAINER_PARTNER)
        difficulty = GetBattlePartnerDifficultyLevel(trainerId);
    else
        difficulty = GetTrainerDifficultyLevel(trainerId);
    if (!ADMG_TryGetTrainerRow(trainerId, difficulty, &selected)
     || !ADMG_EndStateAccessCheck(generation))
        return FALSE;
    *trainer = selected;
    return TRUE;
}

ADMG_NATIVE_ENTRY bool8 ADMG_TryGetTrainerDefeatFlagId(u32 trainerId, u16 *flagId)
{
    u32 first, index, count;
    if (flagId == NULL || trainerId > 0xFFFF)
        return ADMG_RejectTrainer(trainerId, __func__);
    if (trainerId < MAX_TRAINERS_COUNT_EMERALD)
    {
        first = TRAINER_FLAGS_START;
        index = trainerId;
        count = MAX_TRAINERS_COUNT_EMERALD;
    }
    else if (trainerId > ADMG_KANTO_TRAINER_FIRST && trainerId <= ADMG_KANTO_TRAINER_LAST)
    {
        first = ADMG_KANTO_DEFEAT_FLAG_FIRST;
        index = trainerId - ADMG_KANTO_TRAINER_FIRST;
        count = ADMG_KANTO_DEFEAT_FLAG_COUNT;
    }
    else
        return ADMG_RejectTrainer(trainerId, __func__);
    return ADMG_TryGetIndexedFlagId(first, index, count, 1, flagId);
}
