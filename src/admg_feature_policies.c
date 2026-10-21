#include "global.h"
#include "config/wild_encounter.h"
#include "config/map_preview_screen.h"
#include "admg_feature_policies.h"
#include "admg_retention.h"
#include "config/fishing.h"
#include "config/dexnav.h"
#include "config/pokemon.h"
#include "config/contest.h"
#include "config/summary_screen.h"
#include "config/pokedex_plus_hgss.h"
#include "config/overworld.h"
#include "script.h"
#include "constants/admg_feature_policy_ids.h"

_Static_assert(ADMG_SCRIPT_POLICY_BERRY_MULCH == ADMG_POLICY_BERRY_MULCH, "script policy ID");
_Static_assert(ADMG_SCRIPT_POLICY_BERRY_WEEDS == ADMG_POLICY_BERRY_WEEDS, "script policy ID");
_Static_assert(ADMG_SCRIPT_POLICY_BERRY_PESTS == ADMG_POLICY_BERRY_PESTS, "script policy ID");
_Static_assert(ADMG_SCRIPT_POLICY_BERRY_SIX_STAGES == ADMG_POLICY_BERRY_SIX_STAGES, "script policy ID");

/* Keep this a real call (never inlined) so mods can override feature policies at runtime.
 * Without an override, it returns the corresponding configuration macro's value. */
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeaturePolicy(ADMG_Word policy)
{
    return ADMG_GetFeaturePolicyDefault(policy);
}

ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeaturePolicyDefault(ADMG_Word policy)
{
    switch (policy)
    {
    case ADMG_POLICY_SUMMARY_MOVE_RELEARNER: return P_SUMMARY_SCREEN_MOVE_RELEARNER;
    case ADMG_POLICY_SUMMARY_NATURE_COLORS: return P_SUMMARY_SCREEN_NATURE_COLORS;
    case ADMG_POLICY_SUMMARY_RENAME: return P_SUMMARY_SCREEN_RENAME;
    case ADMG_POLICY_CONTEST_INFORMATION: return !C_HIDE_CONTEST_DATA;
    case ADMG_POLICY_DYNAMIC_MOVE_TYPES: return P_SHOW_DYNAMIC_TYPES;
    case ADMG_POLICY_FISHING_CHAIN: return I_FISHING_CHAIN;
    case ADMG_POLICY_FORGET_HIDDEN_MOVES: return P_CAN_FORGET_HIDDEN_MOVE;
    case ADMG_POLICY_HGSS_POKEDEX: return POKEDEX_PLUS_HGSS;
    case ADMG_POLICY_SUMMARY_IV_EV: return P_SUMMARY_SCREEN_IV_EV_INFO;
    case ADMG_POLICY_BERRY_MUTATIONS: return OW_BERRY_MUTATIONS;
    case ADMG_POLICY_FAKE_RTC: return OW_USE_FAKE_RTC;
    case ADMG_POLICY_NPC_FOLLOWERS: return FNPC_ENABLE_NPC_FOLLOWERS;
    case ADMG_POLICY_ENEMY_HP_PERCENTAGE: return B_HP_PERCENTAGE_DISPLAY;
    case ADMG_POLICY_DEXNAV: return DEXNAV_ENABLED;
    case ADMG_POLICY_DEXNAV_SEARCH_LEVELS: return USE_DEXNAV_SEARCH_LEVELS;
    case ADMG_POLICY_OVERWORLD_ENCOUNTERS: return WE_OW_ENCOUNTERS;
    case ADMG_POLICY_TIME_ENCOUNTERS: return OW_TIME_OF_DAY_ENCOUNTERS;
    case ADMG_POLICY_DISABLE_TIME_ENCOUNTER_FALLBACK: return OW_TIME_OF_DAY_DISABLE_FALLBACK;
    case ADMG_POLICY_MAP_PREVIEWS: return MPS_ENABLE_MAP_PREVIEWS;
    case ADMG_POLICY_BERRY_MOISTURE: return OW_BERRY_MOISTURE;
    case ADMG_POLICY_BERRY_ALWAYS_WATERABLE: return OW_BERRY_ALWAYS_WATERABLE;
    case ADMG_POLICY_BERRY_MULCH: return OW_BERRY_MULCH_USAGE;
    case ADMG_POLICY_BERRY_WEEDS: return OW_BERRY_WEEDS;
    case ADMG_POLICY_BERRY_PESTS: return OW_BERRY_PESTS;
    case ADMG_POLICY_BERRY_SIX_STAGES: return OW_BERRY_SIX_STAGES;
    case ADMG_POLICY_BERRY_IMMORTAL: return OW_BERRY_IMMORTAL;
    default: return 0;
    }
}

ADMG_NATIVE_ENTRY ADMG_Word ADMG_IsFeaturePolicyKnown(ADMG_Word policy)
{
    return (policy >= ADMG_POLICY_SUMMARY_MOVE_RELEARNER && policy <= ADMG_POLICY_DYNAMIC_MOVE_TYPES)
        || (policy >= ADMG_POLICY_FISHING_CHAIN && policy <= ADMG_POLICY_ENEMY_HP_PERCENTAGE)
        || policy == ADMG_POLICY_DEXNAV || policy == ADMG_POLICY_DEXNAV_SEARCH_LEVELS
        || policy == ADMG_POLICY_OVERWORLD_ENCOUNTERS
        || policy == ADMG_POLICY_TIME_ENCOUNTERS || policy == ADMG_POLICY_DISABLE_TIME_ENCOUNTER_FALLBACK
        || policy == ADMG_POLICY_MAP_PREVIEWS
        || (policy >= ADMG_POLICY_BERRY_MOISTURE && policy <= ADMG_POLICY_BERRY_IMMORTAL);
}

void ADMG_ScriptTestFeaturePolicy(struct ScriptContext *ctx)
{
    u32 policy = ScriptReadWord(ctx);
    Script_RequestEffects(SCREFF_V1);
    ctx->comparisonResult = ADMG_GetFeaturePolicy(policy) != 0 ? 1 : 0;
}

ADMG_RETAIN_FUNCTION(ADMG_GetFeaturePolicy);
ADMG_RETAIN_FUNCTION(ADMG_GetFeaturePolicyDefault);
ADMG_RETAIN_FUNCTION(ADMG_IsFeaturePolicyKnown);
