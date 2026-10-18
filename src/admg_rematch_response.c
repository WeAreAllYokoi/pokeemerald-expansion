#include "global.h"
#include "admg_rematch_response.h"
#include "admg_state.h"
#include "random.h"

static bool8 RejectResponse(u32 id, const char *operation)
{
    ADMG_StateAccessRejected(id, operation);
    return FALSE;
}

bool8 ADMG_TryGetNativeRematchResponse(const struct ADMG_RematchContext *context,
    const struct ADMG_RematchActor *actors, u32 count, u32 index, u64 earlierYes,
    u32 *response, u8 *position)
{
    u32 i, j, action;
    u32 flagId, family;
    u8 next;
    bool8 fought;
    struct ADMG_TrainerRef ref;
    u64 generation;
    bool8 hasProgress;
    if (actors == NULL || response == NULL || position == NULL || count > ADMG_REMATCH_COHORT_COUNT
     || index >= count || (earlierYes & ~(((u64)1 << index) - 1))
     || !ADMG_IsNativeRematchContextCurrent(context))
        return RejectResponse(index, __func__);
    /* Validate the complete captured actor cohort before RNG or state effects.
     * Offscreen actors remain in the cohort because the original duplicate
     * response predicate explicitly tests visibility. */
    for (i = 0; i < count; ++i)
    {
        if (actors[i].localId == 0 || actors[i].localId >= ADMG_NATIVE_READINESS_COUNT
         || actors[i].objectId > ADMG_REMATCH_NO_OBJECT
         || actors[i].visible > TRUE || (actors[i].visible && actors[i].objectId == ADMG_REMATCH_NO_OBJECT)
         || !ADMG_TryResolveTrainer(actors[i].trainerId, &ref)
         || ref.owner != ADMG_TRAINER_KANTO
         || !ADMG_TryGetTrainerDefeatFlagId(actors[i].trainerId, &flagId))
            return RejectResponse(i, __func__);
        for (j = 0; j < i; ++j)
            if (actors[j].localId == actors[i].localId
             || (actors[i].objectId != ADMG_REMATCH_NO_OBJECT && actors[j].objectId == actors[i].objectId))
                return RejectResponse(i, __func__);
    }
    next = 0;
    if (!actors[index].visible)
        action = ADMG_REMATCH_INVISIBLE;
    else
    {
        if (!ADMG_TryGetTrainerDefeatFlagId(actors[index].trainerId, &flagId)
         || !ADMG_TryReadFlag(flagId, &fought))
            return FALSE;
        if (!fought)
            action = ADMG_REMATCH_UNFOUGHT;
        else
        {
            generation = ADMG_BeginStateAccessCheck();
            hasProgress = ADMG_TryGetNativeRematchProgress(actors[index].trainerId, &family, &next);
            if (!ADMG_EndStateAccessCheck(generation))
                return FALSE;
            if (!hasProgress || next == 0)
            {
                *response = ADMG_REMATCH_NO;
                *position = 0;
                return TRUE;
            }
            /* Original code always advances Random before checking whether a
             * prior visible occurrence forces the same trainer's response. */
            u32 roll = Random() % 100;
            for (i = 0; i < index; ++i)
            {
                if (actors[i].visible && actors[i].trainerId == actors[index].trainerId)
                {
                    bool8 yes = FALSE;
                    for (j = 0; j < index; ++j)
                        if ((earlierYes & ((u64)1 << j)) && actors[j].trainerId == actors[i].trainerId)
                            yes = TRUE;
                    roll = yes ? 100 : 0;
                    break;
                }
            }
            action = roll < 30 ? ADMG_REMATCH_NO : ADMG_REMATCH_YES;
        }
    }
    *response = action;
    *position = next;
    return TRUE;
}
