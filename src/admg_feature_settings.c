#include "global.h"
#include "config/wild_encounter.h"
#include "admg_feature_settings.h"
#include "admg_feature_state.h"
#include "admg_retention.h"
#include "script.h"

ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureSetting(ADMG_Word setting)
{
    return ADMG_GetFeatureSettingDefault(setting);
}
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureSettingDefault(ADMG_Word setting)
{
    switch (setting)
    {
    case ADMG_SETTING_ITEM_DESCRIPTION_MODE: return OW_SHOW_ITEM_DESCRIPTIONS;
    case ADMG_SETTING_VANILLA_RANDOM: return WE_VANILLA_RANDOM;
    case ADMG_SETTING_OWE_SPECIAL_ONLY: return WE_OWE_SPECIAL_ONLY;
    case ADMG_SETTING_OWE_BATTLE_PIKE: return WE_OWE_BATTLE_PIKE;
    case ADMG_SETTING_OWE_BATTLE_PYRAMID: return WE_OWE_BATTLE_PYRAMID;
    case ADMG_SETTING_OWE_RESTRICT_METATILE: return WE_OWE_RESTRICT_METATILE;
    case ADMG_SETTING_OWE_RESTRICT_MAP: return WE_OWE_RESTRICT_MAP;
    case ADMG_SETTING_OWE_UNRESTRICT_SIGHT: return WE_OWE_UNRESTRICT_SIGHT;
    case ADMG_SETTING_OWE_SPAWN_REPLACEMENT: return WE_OWE_SPAWN_REPLACEMENT;
    case ADMG_SETTING_OWE_FLEE_DESPAWN: return WE_OWE_FLEE_DESPAWN;
    case ADMG_SETTING_OWE_SHINY_SPARKLE: return WE_OWE_SHINY_SPARKLE;
    case ADMG_SETTING_OWE_FEEBAS_SPOTS: return WE_OWE_FEEBAS_SPOTS;
    case ADMG_SETTING_OWE_DESPAWN_SOUND: return WE_OWE_DESPAWN_SOUND;
    case ADMG_SETTING_OWE_APPROACH_FOR_BATTLE: return WE_OWE_APPROACH_FOR_BATTLE;
    case ADMG_SETTING_OWE_PREVENT_SHINY_DESPAWN: return WE_OWE_PREVENT_SHINY_DESPAWN;
    case ADMG_SETTING_OWE_PREVENT_FEEBAS_DESPAWN: return WE_OWE_PREVENT_FEEBAS_DESPAWN;
    case ADMG_SETTING_OWE_PREVENT_SPECIAL_MOVEMENT_DESPAWN: return WE_OWE_PREVENT_SPECIAL_MOVEMENT_DESPAWN;
    case ADMG_SETTING_OWE_DESPAWN_ON_ENTER_TOWN: return WE_OWE_DESPAWN_ON_ENTER_TOWN;
    case ADMG_SETTING_OWE_NO_REPEL_DEXNAV_COLLISION: return WE_OWE_NO_REPEL_DEXNAV_COLLISION;

    default: return 0;
    }
}
ADMG_NATIVE_ENTRY ADMG_Word ADMG_IsFeatureSettingKnown(ADMG_Word setting)
{
    return setting == ADMG_SETTING_ITEM_DESCRIPTION_MODE
        || (setting >= ADMG_SETTING_VANILLA_RANDOM && setting <= ADMG_SETTING_OWE_NO_REPEL_DEXNAV_COLLISION);
}
ADMG_NATIVE_ENTRY ADMG_Word ADMG_ItemDescriptionsEnabled(void)
{
    u32 mode = ADMG_GetFeatureSetting(ADMG_SETTING_ITEM_DESCRIPTION_MODE);
    if (mode == OW_ITEM_DESCRIPTIONS_ALWAYS)
        return TRUE;
    return mode == OW_ITEM_DESCRIPTIONS_FIRST_TIME && ADMG_GetFeatureStateBridgeVersion() == 1
        && ADMG_GetFeatureState(ADMG_FEATURE_STATE_ITEMS_SEEN) != 0;
}
void ADMG_ScriptTestItemDescriptions(struct ScriptContext *ctx)
{
    Script_RequestEffects(SCREFF_V1);
    ctx->comparisonResult = ADMG_ItemDescriptionsEnabled() != 0 ? 1 : 0;
}
ADMG_RETAIN_FUNCTION(ADMG_GetFeatureSetting);
ADMG_RETAIN_FUNCTION(ADMG_GetFeatureSettingDefault);
ADMG_RETAIN_FUNCTION(ADMG_IsFeatureSettingKnown);
ADMG_RETAIN_FUNCTION(ADMG_ItemDescriptionsEnabled);
