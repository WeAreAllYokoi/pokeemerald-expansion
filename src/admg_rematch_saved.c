#include "global.h"
#include "admg_rematch_state.h"
#include "admg_state.h"
#include "script.h"
#include "vs_seeker.h"
#include "constants/script_commands.h"
#include "constants/trainer_types.h"

static bool8 RejectBinding(u32 id, const char *operation)
{
    ADMG_StateAccessRejected(id, operation);
    return FALSE;
}

bool8 ADMG_TryGetNativeRematchNpcTrainer(u32 group, u32 number, u32 localId, u16 *trainerId)
{
    const struct ObjectEventTemplate *npc;
    const u8 *operand;
    struct ADMG_TrainerRef ref;
    u32 id;
    if (trainerId == NULL || localId >= ADMG_NATIVE_READINESS_COUNT
     || !ADMG_TryGetDeclaredRematchNpcTemplate(group, number, localId, &npc))
        return RejectBinding(localId, __func__);
    if (npc->trainerType != TRAINER_TYPE_NORMAL && npc->trainerType != TRAINER_TYPE_BURIED)
        return RejectBinding(localId, __func__);
    /* Both forms are existing Expansion script consumers. The source is an
     * exact-target declaration, never a borrowed saved script pointer. */
    if (npc->script[0] == SCR_OP_TRAINERBATTLE)
        operand = npc->script + 3;
    else if (Script_MatchesCallNative(npc->script, NativeVsSeekerRematchId, TRUE))
        operand = npc->script + 5;
    else
        return RejectBinding(localId, __func__);
    id = operand[0] | (operand[1] << 8);
    if (!ADMG_TryResolveTrainer(id, &ref) || ref.owner != ADMG_TRAINER_KANTO)
        return RejectBinding(id, __func__);
    *trainerId = id;
    return TRUE;
}

bool8 ADMG_TryValidateNativeRematchBindings(void)
{
    u32 i, group, number;
    u16 trainer, family;
    u8 position;
    if (gSaveBlock1Ptr == NULL
     || (gADMG_KantoVsSeekerCounter & 0xFF) > 100
     || (gADMG_KantoVsSeekerCounter >> 8) > 100)
        return RejectBinding(0, __func__);
    group = (u32)gSaveBlock1Ptr->location.mapGroup;
    number = (u32)gSaveBlock1Ptr->location.mapNum;
    if (!ADMG_IsMapIdValid(group, number))
        return RejectBinding(group, __func__);
    for (i = 0; i < ADMG_NATIVE_READINESS_COUNT; ++i)
    {
        if (gADMG_KantoNpcReadiness[i] > 5)
            return RejectBinding(i, __func__);
        if (gADMG_KantoNpcReadiness[i] == 0)
            continue;
        if (!ADMG_TryGetNativeRematchNpcTrainer(group, number, i, &trainer)
         || !ADMG_TryFindNativeRematch(trainer, TRUE, &family, &position))
            return RejectBinding(i, __func__);
        /* Original readiness is a positive local-NPC predicate. Battle party
         * selection derives afresh from defeats; do not reinterpret readiness
         * as a progression cache or impose new campaign gating during load. */
    }
    return TRUE;
}

bool8 ADMG_TryResumeNativeRematchBindings(struct ADMG_RematchContext *context)
{
    if (context == NULL || !ADMG_TryValidateNativeRematchBindings())
        return RejectBinding(0, __func__);
    return ADMG_TryResumeNativeRematches(context);
}
