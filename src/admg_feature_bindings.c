#include "global.h"
#include "admg_feature_bindings.h"
#include "admg_retention.h"
#include "admg_state.h"
#include "event_data.h"
#include "script.h"
#include "config/text.h"
#include "config/name_box.h"
#include "config/pokerus.h"

/* Each binding ID also identifies its reserved flag or variable slot.
 * This getter returns the original configuration value unless overridden.
 * The reserved slot remains directly accessible through the flag or variable API. */
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureBinding(ADMG_Word binding)
{
    return ADMG_GetFeatureBindingDefault(binding);
}
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureBindingDefault(ADMG_Word binding)
{
    switch (binding)
    {
    case ADMG_BINDING_B_FLAG_AI_VS_AI_BATTLE: return B_FLAG_AI_VS_AI_BATTLE;
    case ADMG_BINDING_B_FLAG_DYNAMAX_BATTLE: return B_FLAG_DYNAMAX_BATTLE;
    case ADMG_BINDING_B_FLAG_FOLLOWERS_DISABLED: return B_FLAG_FOLLOWERS_DISABLED;
    case ADMG_BINDING_B_FLAG_INVERSE_BATTLE: return B_FLAG_INVERSE_BATTLE;
    case ADMG_BINDING_B_FLAG_NO_WHITEOUT: return B_FLAG_NO_WHITEOUT;
    case ADMG_BINDING_B_FLAG_SKY_BATTLE: return B_FLAG_SKY_BATTLE;
    case ADMG_BINDING_B_FLAG_SLEEP_CLAUSE: return B_FLAG_SLEEP_CLAUSE;
    case ADMG_BINDING_B_FLAG_TERA_ORB_CHARGED: return B_FLAG_TERA_ORB_CHARGED;
    case ADMG_BINDING_B_FLAG_TERA_ORB_NO_COST: return B_FLAG_TERA_ORB_NO_COST;
    case ADMG_BINDING_B_FLAG_VICTORY_CATCH_GUARANTEED: return B_FLAG_VICTORY_CATCH_GUARANTEED;
    case ADMG_BINDING_B_FLAG_VICTORY_CATCH_RANDOM: return B_FLAG_VICTORY_CATCH_RANDOM;
    case ADMG_BINDING_B_LEVEL_CAP_VARIABLE: return B_LEVEL_CAP_VARIABLE;
    case ADMG_BINDING_B_VAR_DIFFICULTY: return B_VAR_DIFFICULTY;
    case ADMG_BINDING_B_VAR_NO_BAG_USE: return B_VAR_NO_BAG_USE;
    case ADMG_BINDING_B_VAR_SKY_BATTLE: return B_VAR_SKY_BATTLE;
    case ADMG_BINDING_B_VAR_WILD_AI_FLAGS: return B_VAR_WILD_AI_FLAGS;
    case ADMG_BINDING_DN_FLAG_DETECTOR_MODE: return DN_FLAG_DETECTOR_MODE;
    case ADMG_BINDING_DN_FLAG_DEXNAV_GET: return DN_FLAG_DEXNAV_GET;
    case ADMG_BINDING_DN_FLAG_SEARCHING: return DN_FLAG_SEARCHING;
    case ADMG_BINDING_DN_VAR_SPECIES: return DN_VAR_SPECIES;
    case ADMG_BINDING_DN_VAR_STEP_COUNTER: return DN_VAR_STEP_COUNTER;
    case ADMG_BINDING_FLAG_TEXT_SPEED_INSTANT: return FLAG_TEXT_SPEED_INSTANT;
    case ADMG_BINDING_FNPC_FLAG_HEAL_AFTER_FOLLOWER_BATTLE: return FNPC_FLAG_HEAL_AFTER_FOLLOWER_BATTLE;
    case ADMG_BINDING_FNPC_FLAG_PARTNER_WILD_BATTLES: return FNPC_FLAG_PARTNER_WILD_BATTLES;
    case ADMG_BINDING_I_EXP_SHARE_FLAG: return I_EXP_SHARE_FLAG;
    case ADMG_BINDING_I_ORAS_DOWSING_FLAG: return I_ORAS_DOWSING_FLAG;
    case ADMG_BINDING_I_VS_SEEKER_CHARGING: return I_VS_SEEKER_CHARGING;
    case ADMG_BINDING_OW_FLAG_MOVE_UNION_ROOM_CHECK: return OW_FLAG_MOVE_UNION_ROOM_CHECK;
    case ADMG_BINDING_OW_FLAG_NO_COLLISION: return OW_FLAG_NO_COLLISION;
    case ADMG_BINDING_OW_FLAG_NO_TRAINER_SEE: return OW_FLAG_NO_TRAINER_SEE;
    case ADMG_BINDING_OW_FLAG_PAUSE_TIME: return OW_FLAG_PAUSE_TIME;
    case ADMG_BINDING_OW_FLAG_POKE_RIDER: return OW_FLAG_POKE_RIDER;
    case ADMG_BINDING_OW_FLAG_SUPPRESS_NAME_BOX: return OW_FLAG_SUPPRESS_NAME_BOX;
    case ADMG_BINDING_P_FLAG_EGG_MOVES: return P_FLAG_EGG_MOVES;
    case ADMG_BINDING_P_FLAG_FORCE_NO_SHINY: return P_FLAG_FORCE_NO_SHINY;
    case ADMG_BINDING_P_FLAG_FORCE_SHINY: return P_FLAG_FORCE_SHINY;
    case ADMG_BINDING_P_FLAG_SUMMARY_SCREEN_IV_EV_INFO: return P_FLAG_SUMMARY_SCREEN_IV_EV_INFO;
    case ADMG_BINDING_P_FLAG_TUTOR_MOVES: return P_FLAG_TUTOR_MOVES;
    case ADMG_BINDING_P_POKERUS_FLAG_INFECTION: return P_POKERUS_FLAG_INFECTION;
    case ADMG_BINDING_VAR_LAST_REPEL_LURE_USED: return VAR_LAST_REPEL_LURE_USED;
    case ADMG_BINDING_WE_FLAG_FORCE_DOUBLE_WILD: return WE_FLAG_FORCE_DOUBLE_WILD;
    case ADMG_BINDING_WE_FLAG_NO_CATCHING: return WE_FLAG_NO_CATCHING;
    case ADMG_BINDING_WE_FLAG_NO_ENCOUNTER: return WE_FLAG_NO_ENCOUNTER;
    case ADMG_BINDING_WE_FLAG_NO_RUNNING: return WE_FLAG_NO_RUNNING;
    case ADMG_BINDING_WE_OWE_FLAG_DISABLED: return WE_OWE_FLAG_DISABLED;
    case ADMG_BINDING_WE_SMART_WILD_AI_FLAG: return WE_SMART_WILD_AI_FLAG;
    default: return 0;
    }
}
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureBindingKind(ADMG_Word binding)
{
    if (binding >= ADMG_STATE_FEATURE_BINDING_FLAGS_FIRST && binding <= ADMG_STATE_FEATURE_BINDING_FLAGS_LAST)
        return ADMG_BINDING_KIND_FLAG;
    if (binding >= ADMG_STATE_FEATURE_BINDING_VARS_FIRST && binding <= ADMG_STATE_FEATURE_BINDING_VARS_LAST)
        return ADMG_BINDING_KIND_VAR;
    return ADMG_BINDING_KIND_NONE;
}
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureBindingSlot(ADMG_Word binding)
{
    return ADMG_GetFeatureBindingKind(binding) != ADMG_BINDING_KIND_NONE ? binding : 0;
}

/* Field-script helpers for testing bindings and accessing their flags or variables.
 * The test stores its result in ctx->comparisonResult, leaving VAR_RESULT unchanged.
 * The write helpers request their effects before calling FlagSet, FlagClear or VarSet. */
void ADMG_ScriptTestFeatureBinding(struct ScriptContext *ctx)
{
    u32 binding = ScriptReadHalfword(ctx);
    Script_RequestEffects(SCREFF_V1);
    ctx->comparisonResult = ADMG_GetFeatureBinding(binding) != 0 ? 1 : 0;
}
void ADMG_ScriptWriteFeatureFlag(struct ScriptContext *ctx)
{
    u32 binding = ScriptReadHalfword(ctx);
    bool8 value = ScriptReadByte(ctx);
    u32 id = ADMG_GetFeatureBinding(binding);
    Script_RequestEffects(SCREFF_V1);
    if (ADMG_GetFeatureBindingKind(binding) != ADMG_BINDING_KIND_FLAG || id == 0)
        return;
    Script_RequestEffects(SCREFF_V1 | SCREFF_SAVE);
    if (value) FlagSet(id); else FlagClear(id);
}
void ADMG_ScriptCopyFeatureVar(struct ScriptContext *ctx)
{
    u32 binding = ScriptReadHalfword(ctx);
    u16 destination = ScriptReadHalfword(ctx);
    u32 id = ADMG_GetFeatureBinding(binding);
    u32 value = 0;
    Script_RequestEffects(SCREFF_V1);
    if (ADMG_GetFeatureBindingKind(binding) != ADMG_BINDING_KIND_VAR)
        return;
    if (id != 0 && !ADMG_TryReadVar(id, &value, FALSE))
        return;
    Script_RequestWriteVar(destination);
    VarSet(destination, value);
}
ADMG_RETAIN_FUNCTION(ADMG_GetFeatureBinding);
ADMG_RETAIN_FUNCTION(ADMG_GetFeatureBindingDefault);
ADMG_RETAIN_FUNCTION(ADMG_GetFeatureBindingSlot);
ADMG_RETAIN_FUNCTION(ADMG_GetFeatureBindingKind);
