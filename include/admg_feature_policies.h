#ifndef ADMG_FEATURE_POLICIES_H
#define ADMG_FEATURE_POLICIES_H

#include "admg_native.h"

/* Stable feature IDs used by ADMG_GetFeaturePolicy. Mods refer to these numeric values. */
enum ADMG_FeaturePolicyId
{
    ADMG_POLICY_FISHING_CHAIN = 1,
    ADMG_POLICY_FORGET_HIDDEN_MOVES = 2,
    ADMG_POLICY_HGSS_POKEDEX = 3,
    ADMG_POLICY_SUMMARY_IV_EV = 4,
    ADMG_POLICY_BERRY_MUTATIONS = 5,
};

ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeaturePolicy(ADMG_Word policy);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeaturePolicyDefault(ADMG_Word policy);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_IsFeaturePolicyKnown(ADMG_Word policy);

#endif
