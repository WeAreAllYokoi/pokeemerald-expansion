#ifndef ADMG_FEATURE_SETTINGS_H
#define ADMG_FEATURE_SETTINGS_H
#include "admg_native.h"
struct ScriptContext;

/* Each multi-valued setting has its own ID.
 * Its value is one of that setting's named configuration options,
 * such as OW_ITEM_DESCRIPTIONS_OFF or OW_ITEM_DESCRIPTIONS_ALWAYS. */
enum ADMG_FeatureSettingId
{
    ADMG_SETTING_ITEM_DESCRIPTION_MODE = 1,
};
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureSetting(ADMG_Word setting);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureSettingDefault(ADMG_Word setting);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_IsFeatureSettingKnown(ADMG_Word setting);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_ItemDescriptionsEnabled(void);
void ADMG_ScriptTestItemDescriptions(struct ScriptContext *ctx);
#endif
