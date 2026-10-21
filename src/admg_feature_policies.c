#include "global.h"
#include "admg_feature_policies.h"
#include "admg_retention.h"
#include "config/fishing.h"
#include "config/pokemon.h"
#include "config/summary_screen.h"
#include "config/pokedex_plus_hgss.h"
#include "config/overworld.h"

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
    case ADMG_POLICY_FISHING_CHAIN: return I_FISHING_CHAIN;
    case ADMG_POLICY_FORGET_HIDDEN_MOVES: return P_CAN_FORGET_HIDDEN_MOVE;
    case ADMG_POLICY_HGSS_POKEDEX: return POKEDEX_PLUS_HGSS;
    case ADMG_POLICY_SUMMARY_IV_EV: return P_SUMMARY_SCREEN_IV_EV_INFO;
    case ADMG_POLICY_BERRY_MUTATIONS: return OW_BERRY_MUTATIONS;
    case ADMG_POLICY_FAKE_RTC: return OW_USE_FAKE_RTC;
    case ADMG_POLICY_NPC_FOLLOWERS: return FNPC_ENABLE_NPC_FOLLOWERS;
    case ADMG_POLICY_ENEMY_HP_PERCENTAGE: return B_HP_PERCENTAGE_DISPLAY;
    default: return 0;
    }
}

ADMG_NATIVE_ENTRY ADMG_Word ADMG_IsFeaturePolicyKnown(ADMG_Word policy)
{
    return policy >= ADMG_POLICY_FISHING_CHAIN && policy <= ADMG_POLICY_ENEMY_HP_PERCENTAGE;
}

ADMG_RETAIN_FUNCTION(ADMG_GetFeaturePolicy);
ADMG_RETAIN_FUNCTION(ADMG_GetFeaturePolicyDefault);
ADMG_RETAIN_FUNCTION(ADMG_IsFeaturePolicyKnown);
