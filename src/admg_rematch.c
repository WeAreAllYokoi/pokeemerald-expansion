#include "global.h"
#include "admg_rematch.h"
#include "admg_state.h"
#include "admg_trainer.h"
#include "data/admg_rematch_families.h"

_Static_assert(ARRAY_COUNT(gADMG_KantoRematchFamilies) == ADMG_NATIVE_REMATCH_FAMILIES, "complete native families");
_Static_assert(ARRAY_COUNT(gADMG_KantoRematchFamilies[0]) == ADMG_NATIVE_REMATCH_SLOTS, "six native positions");

static bool8 RejectRematch(u32 id, const char *operation)
{
    ADMG_StateAccessRejected(id, operation);
    return FALSE;
}

bool8 ADMG_TryFindNativeRematch(u32 trainerId, bool8 baseOnly, u16 *family, u8 *position)
{
    struct ADMG_TrainerRef ref;
    u32 i, j;
    if (family == NULL || position == NULL || baseOnly > TRUE)
        return RejectRematch(trainerId, __func__);
    if (!ADMG_TryResolveTrainer(trainerId, &ref))
        return FALSE;
    if (ref.owner != ADMG_TRAINER_KANTO)
        return RejectRematch(trainerId, __func__);
    for (i = 0; i < ADMG_NATIVE_REMATCH_FAMILIES; ++i)
    {
        for (j = 0; j < (baseOnly ? 1 : ADMG_NATIVE_REMATCH_SLOTS); ++j)
        {
            u16 candidate = gADMG_KantoRematchFamilies[i][j];
            if (candidate == 0)
                break;
            if (candidate != 0xFFFF && candidate == trainerId)
            {
                *family = i;
                *position = j;
                return TRUE;
            }
        }
    }
    /* No declared relationship is an ordinary negative result. Retention of a
     * trainer does not fabricate membership or mutate its independent state. */
    return FALSE;
}

bool8 ADMG_TryGetNativeRematchProgress(u32 baseTrainerId, u16 *family, u8 *position)
{
    u16 found, flagId;
    u8 basePosition, i;
    bool8 fought;
    if (family == NULL || position == NULL)
        return RejectRematch(baseTrainerId, __func__);
    if (!ADMG_TryFindNativeRematch(baseTrainerId, TRUE, &found, &basePosition))
        return FALSE;
    for (i = 1; i < ADMG_NATIVE_REMATCH_SLOTS; ++i)
    {
        u16 candidate = gADMG_KantoRematchFamilies[found][i];
        if (candidate == 0)
            break;
        if (candidate == 0xFFFF)
            continue;
        if (!ADMG_TryGetTrainerDefeatFlagId(candidate, &flagId)
         || !ADMG_TryReadFlag(flagId, &fought))
            return FALSE;
        if (!fought)
        {
            *family = found;
            *position = i;
            return TRUE;
        }
    }
    *family = found;
    *position = i - 1;
    return TRUE;
}

static u8 ApplyNativeEligibility(u16 family, u8 position, u32 permittedStages)
{
    /* Original policy checks the derived position once, then chooses the
     * preceding non-SKIP position. It does not recursively gate that fallback. */
    if (position != 0 && !(permittedStages & (1u << position)))
    {
        while (--position != 0)
            if (gADMG_KantoRematchFamilies[family][position] != 0xFFFF)
                break;
    }
    return position;
}

bool8 ADMG_TrySelectNativeRematch(u32 baseTrainerId, u32 permittedStages, u16 *trainerId)
{
    u16 family;
    u8 position;
    if (trainerId == NULL || (permittedStages & ~0x3Fu))
        return RejectRematch(baseTrainerId, __func__);
    if (!ADMG_TryGetNativeRematchProgress(baseTrainerId, &family, &position))
        return FALSE;
    /* Original zero means no available rematch, not an automatic base battle. */
    if (position == 0)
        *trainerId = 0;
    else
        *trainerId = gADMG_KantoRematchFamilies[family][ApplyNativeEligibility(family, position, permittedStages)];
    return TRUE;
}

bool8 ADMG_TryGetNativeRematchTrainer(u32 baseTrainerId, u16 *trainerId)
{
    static const u16 gates[ADMG_NATIVE_REMATCH_SLOTS] = {
        0, ADMG_KANTO_FLAG_GOT_VS_SEEKER,
        ADMG_KANTO_FLAG_WORLD_MAP_CELADON_CITY,
        ADMG_KANTO_FLAG_WORLD_MAP_FUCHSIA_CITY,
        ADMG_KANTO_FLAG_SYS_GAME_CLEAR,
        /* Original FLAG_SYS_CAN_LINK_WITH_RS: the retained target uses the
         * historical spelling FLAG_IS_CHAMPION for its network-unlocked state.
         * This is the existing Kanto authority at 0x11A4, not Hoenn completion
         * and not the distinct Kanto FLAG_SYS_GAME_CLEAR authority. */
        ADMG_KANTO_FLAG_IS_CHAMPION,
    };
    u16 family;
    u8 position;
    bool8 permitted;
    if (trainerId == NULL)
        return RejectRematch(baseTrainerId, __func__);
    if (!ADMG_TryGetNativeRematchProgress(baseTrainerId, &family, &position))
        return FALSE;
    if (position == 0)
    {
        *trainerId = 0;
        return TRUE;
    }
    if (!ADMG_TryReadFlag(gates[position], &permitted))
        return FALSE;
    *trainerId = gADMG_KantoRematchFamilies[family][ApplyNativeEligibility(family, position,
                                    permitted ? 0x3F : 0)];
    return TRUE;
}
