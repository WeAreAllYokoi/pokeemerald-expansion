#include "global.h"
#include "admg_feature_bindings.h"
#include "data.h"
#include "event_data.h"
#include "admg_state.h"
#include "admg_trainer.h"
#include "script.h"
#include "constants/battle.h"

enum DifficultyLevel GetCurrentDifficultyLevel(void)
{
    u32 value;
    if (!ADMG_GetFeatureBinding(ADMG_BINDING_B_VAR_DIFFICULTY))
        return DIFFICULTY_NORMAL;
    if (!ADMG_TryReadVar((u32)ADMG_GetFeatureBinding(ADMG_BINDING_B_VAR_DIFFICULTY), &value, FALSE))
        return DIFFICULTY_NORMAL;
    if (value >= DIFFICULTY_COUNT)
    {
        ADMG_StateAccessRejected(value, __func__);
        return DIFFICULTY_NORMAL;
    }
    return value;
}

void SetCurrentDifficultyLevel(enum DifficultyLevel desiredDifficulty)
{
    if (!ADMG_GetFeatureBinding(ADMG_BINDING_B_VAR_DIFFICULTY))
        return;

    if (desiredDifficulty > DIFFICULTY_MAX)
        desiredDifficulty = DIFFICULTY_MAX;

    VarSet(ADMG_GetFeatureBinding(ADMG_BINDING_B_VAR_DIFFICULTY), desiredDifficulty);
}

enum DifficultyLevel GetBattlePartnerDifficultyLevel(enum TrainerId partnerId)
{
    enum DifficultyLevel difficulty = GetCurrentDifficultyLevel();
    const struct Trainer *trainer;
    u32 index;
    if (partnerId < PARTNER_COUNT)
        index = partnerId;
    else if (IsPartnerTrainerId(partnerId))
        index = GetPartnerIdFromTrainerId(partnerId);
    else
    {
        ADMG_StateAccessRejected(partnerId, __func__);
        return DIFFICULTY_NORMAL;
    }
    if (!ADMG_TryGetOwnedTrainer(ADMG_TRAINER_PARTNER, index, difficulty, &trainer))
        return DIFFICULTY_NORMAL;
    if (difficulty == DIFFICULTY_NORMAL || trainer->party == NULL)
        return DIFFICULTY_NORMAL;
    return difficulty;
}

enum DifficultyLevel GetTrainerDifficultyLevel(enum TrainerId trainerId)
{
    enum DifficultyLevel difficulty = GetCurrentDifficultyLevel();
    const struct Trainer *trainer;
    if (!ADMG_TryGetTrainerRow(trainerId, difficulty, &trainer))
        return DIFFICULTY_NORMAL;
    if (difficulty == DIFFICULTY_NORMAL || trainer->party == NULL)
        return DIFFICULTY_NORMAL;
    return difficulty;
}

void Script_IncreaseDifficulty(void)
{
    enum DifficultyLevel currentDifficulty;

    if (!ADMG_GetFeatureBinding(ADMG_BINDING_B_VAR_DIFFICULTY))
        return;

    currentDifficulty = GetCurrentDifficultyLevel();

    if (currentDifficulty++ > DIFFICULTY_MAX)
        return;

    Script_RequestEffects(SCREFF_V1);
    Script_RequestWriteVar(ADMG_GetFeatureBinding(ADMG_BINDING_B_VAR_DIFFICULTY));

    SetCurrentDifficultyLevel(currentDifficulty);
}

void Script_DecreaseDifficulty(void)
{
    enum DifficultyLevel currentDifficulty;

    if (!ADMG_GetFeatureBinding(ADMG_BINDING_B_VAR_DIFFICULTY))
        return;

    currentDifficulty = GetCurrentDifficultyLevel();

    if (!currentDifficulty)
        return;

    Script_RequestEffects(SCREFF_V1);
    Script_RequestWriteVar(ADMG_GetFeatureBinding(ADMG_BINDING_B_VAR_DIFFICULTY));

    SetCurrentDifficultyLevel(--currentDifficulty);
}

void Script_GetDifficulty(void)
{
    Script_RequestEffects(SCREFF_V1);
    gSpecialVar_Result = GetCurrentDifficultyLevel();
}

void Script_SetDifficulty(struct ScriptContext *ctx)
{
    enum DifficultyLevel desiredDifficulty = ScriptReadByte(ctx);

    Script_RequestEffects(SCREFF_V1);
    Script_RequestWriteVar(ADMG_GetFeatureBinding(ADMG_BINDING_B_VAR_DIFFICULTY));

    SetCurrentDifficultyLevel(desiredDifficulty);
}
