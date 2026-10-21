#include "global.h"
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
    default: return 0;
    }
}
ADMG_NATIVE_ENTRY ADMG_Word ADMG_IsFeatureSettingKnown(ADMG_Word setting)
{
    return setting == ADMG_SETTING_ITEM_DESCRIPTION_MODE;
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
