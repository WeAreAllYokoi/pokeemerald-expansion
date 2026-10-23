#include "global.h"
#include "script.h"
#include "config_changes.h"
#include "malloc.h"
#include "constants/config_changes.h"
#include "config/pokerus.h"
#include "config/ai.h"
#include "config/battle.h"
#include "config/caps.h"
#include "config/fishing.h"
#include "config/general.h"
#include "config/name_box.h"
#include "config/overworld.h"
#include "config/pokedex_plus_hgss.h"
#include "config/pokemon.h"
#include "config/item.h"
#include "config/summary_screen.h"
#include "config/wild_encounter.h"
#include "config/dexnav.h"
#include "config/follower_npc.h"
#include "config/map_preview_screen.h"
#include "config/save.h"
#include "config/text.h"
#include "constants/characters.h"
#include "constants/rgb.h"
#include "constants/species.h"
#include "admg_retention.h"

#define UNPACK_BATTLE_CONFIG_GEN_CHANGES(_name, _field, ...) ._field = _name,
#define UNPACK_POKEMON_CONFIG_GEN_CHANGES(_name, _field, ...) ._field = P_##_name,
#define UNPACK_AI_CONFIG_CHANGES(_name, _field, ...) ._field = _name,

const struct ConfigChanges sConfigChanges =
{
    BATTLE_CONFIG_DEFINITIONS(UNPACK_BATTLE_CONFIG_GEN_CHANGES)
    POKEMON_CONFIG_DEFINITIONS(UNPACK_POKEMON_CONFIG_GEN_CHANGES)
    AI_CONFIG_DEFINITIONS(UNPACK_AI_CONFIG_CHANGES)
    /* Expands to:
    .critChance     = B_CRIT_CHANCE,
    .critMultiplier = B_CRIT_MULTIPLIER,
    */
};

#if TESTING
EWRAM_DATA struct ConfigChanges *gConfigChangesTestOverride = NULL;
#define UNPACK_CONFIG_OVERRIDE_GETTERS(_name, _field, ...)   case CONFIG_##_name: return gConfigChangesTestOverride->_field;
#define UNPACK_CONFIG_GETTERS(_name, _field, ...) case CONFIG_##_name: return sConfigChanges._field;
#define UNPACK_CONFIG_CLAMPER(_name, _field, _typeMaxValue, ...) case CONFIG_##_name: clampedValue = min(GET_CONFIG_MAXIMUM(_typeMaxValue), newValue); break;
#define UNPACK_CONFIG_SETTERS(_name, _field, _typeMaxValue, ...) case CONFIG_##_name: gConfigChangesTestOverride->_field = clampedValue; break;

#else

#define UNPACK_CONFIG_OVERRIDE_GETTERS(_name, _field, ...) case CONFIG_##_name: return sConfigChanges._field;
#define UNPACK_CONFIG_GETTERS(_name, _field, ...) case CONFIG_##_name: return sConfigChanges._field;
#define UNPACK_CONFIG_CLAMPER(_name, _field, ...) case CONFIG_##_name: return 0;
#define UNPACK_CONFIG_SETTERS(_name, _field, ...) case CONFIG_##_name: return;
#endif

// Gets the value of a volatile status flag for a certain battler
// Primarily used for the debug menu and scripts. Outside of it explicit references are preferred
ARM_FUNC u32 GetConfigInternal(enum ConfigTag _config)
{
#if TESTING
    if (gConfigChangesTestOverride == NULL)
    {
        switch (_config)
        {
        BATTLE_CONFIG_DEFINITIONS(UNPACK_CONFIG_GETTERS)
        POKEMON_CONFIG_DEFINITIONS(UNPACK_CONFIG_GETTERS)
        AI_CONFIG_DEFINITIONS(UNPACK_CONFIG_GETTERS)
        /* Expands to:
        case CONFIG_CRIT_CHANCE:
            return gConfigChangesTestOverride->critChance;
        */
        default:
            return 0;
        }
    }
    else
#endif
    {
#if !TESTING
        return ADMG_GetConfig(_config); // Battle scripts use the same selector as the GetConfig accessors
#endif
        switch (_config)
        {
        BATTLE_CONFIG_DEFINITIONS(UNPACK_CONFIG_OVERRIDE_GETTERS)
        POKEMON_CONFIG_DEFINITIONS(UNPACK_CONFIG_OVERRIDE_GETTERS)
        AI_CONFIG_DEFINITIONS(UNPACK_CONFIG_OVERRIDE_GETTERS)
        /* Expands to:
        case CONFIG_CRIT_CHANCE:
            return sConfigChanges.critChance;
        */
        default: // Invalid config tag
            return 0;
        }
     }
}

#if !TESTING
#define UNPACK_CONFIG_MAXIMUM(_name, _field, _typeMaxValue, ...) case CONFIG_##_name: return GET_CONFIG_MAXIMUM(_typeMaxValue);
#define UNPACK_ADMG_CONFIG_EXTRA_DEFAULT(_name, _maximum) case CONFIG_##_name: return ADMG_DEFAULT_##_name;
#define UNPACK_ADMG_CONFIG_EXTRA_MAXIMUM(_name, _maximum) case CONFIG_##_name: return _maximum;

ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetConfigDefault(ADMG_Word tag)
{
    switch (tag)
    {
    BATTLE_CONFIG_DEFINITIONS(UNPACK_CONFIG_GETTERS)
    POKEMON_CONFIG_DEFINITIONS(UNPACK_CONFIG_GETTERS)
    AI_CONFIG_DEFINITIONS(UNPACK_CONFIG_GETTERS)
    ADMG_CONFIG_EXTRA_DEFINITIONS(UNPACK_ADMG_CONFIG_EXTRA_DEFAULT)
    default:
        return 0;
    }
}

ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetConfig(ADMG_Word tag)
{
    return ADMG_GetConfigDefault(tag);
}

// The option's declared maximum, or its type's maximum if none was given
// SetConfig uses the same limit in test builds
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetConfigMaximum(ADMG_Word tag)
{
    switch (tag)
    {
    BATTLE_CONFIG_DEFINITIONS(UNPACK_CONFIG_MAXIMUM)
    POKEMON_CONFIG_DEFINITIONS(UNPACK_CONFIG_MAXIMUM)
    AI_CONFIG_DEFINITIONS(UNPACK_CONFIG_MAXIMUM)
    ADMG_CONFIG_EXTRA_DEFINITIONS(UNPACK_ADMG_CONFIG_EXTRA_MAXIMUM)
    default:
        return 0;
    }
}

ADMG_NATIVE_ENTRY ADMG_Word ADMG_IsConfigKnown(ADMG_Word tag)
{
    return tag < CONFIG_COUNT || (tag > ADMG_CONFIG_EXTRA_FIRST && tag < ADMG_CONFIG_EXTRA_END);
}

ADMG_RETAIN_FUNCTION(ADMG_GetConfig);
ADMG_RETAIN_FUNCTION(ADMG_GetConfigDefault);
ADMG_RETAIN_FUNCTION(ADMG_GetConfigMaximum);
ADMG_RETAIN_FUNCTION(ADMG_IsConfigKnown);
#endif

#if TESTING
u32 GetClampedValue(enum ConfigTag _config, u32 newValue)
{
    u32 clampedValue = 0;
    switch (_config)
    {
    BATTLE_CONFIG_DEFINITIONS(UNPACK_CONFIG_CLAMPER)
    POKEMON_CONFIG_DEFINITIONS(UNPACK_CONFIG_CLAMPER)
    AI_CONFIG_DEFINITIONS(UNPACK_CONFIG_CLAMPER)
    default:
        return 0;
    }
    return clampedValue;
}
#endif

void SetConfig(enum ConfigTag _config, u32 _value)
{
#if TESTING
    // Clamping is done here instead of the switch due to an internal compiler error!
    u32 clampedValue = GetClampedValue(_config, _value);
    switch (_config)
    {
    BATTLE_CONFIG_DEFINITIONS(UNPACK_CONFIG_SETTERS)
    POKEMON_CONFIG_DEFINITIONS(UNPACK_CONFIG_SETTERS)
    AI_CONFIG_DEFINITIONS(UNPACK_CONFIG_SETTERS)
    /* Expands to:
    #if TESTING
    case CONFIG_CRIT_CHANCE:
        gConfigChangesTestOverride->critChance = clampedValue;
        break;
    #else
    case CONFIG_CRIT_CHANCE:
        return;
    #endif
    */
    default: // Invalid config tag
        return;
    }
#endif
}

#if TESTING
void TestInitConfigData(void)
{
    Free(gConfigChangesTestOverride);
    gConfigChangesTestOverride = Alloc(sizeof(sConfigChanges));
    memcpy(gConfigChangesTestOverride, &sConfigChanges, sizeof(sConfigChanges));
}

void TestFreeConfigData(void)
{
    FREE_AND_SET_NULL(gConfigChangesTestOverride);
}
#endif

// Keep the assembler's ADMG_CONFIG_TAG_<name> values in sync with CONFIG_<name>
#define ADMG_CHECK_EXTRA_TAG(_name, _maximum) STATIC_ASSERT(ADMG_CONFIG_TAG_##_name == CONFIG_##_name, AdmgConfigTag_##_name)
ADMG_CONFIG_EXTRA_DEFINITIONS(ADMG_CHECK_EXTRA_TAG)
#undef ADMG_CHECK_EXTRA_TAG

#if !TESTING
// Field-script config test: read the next word as a tag from config_changes.h or admg_config_extra.h
// Set comparisonResult to 1 if the selected value is nonzero, or 0 otherwise
void ADMG_ScriptTestConfig(struct ScriptContext *ctx)
{
    u32 tag = ScriptReadWord(ctx);
    Script_RequestEffects(SCREFF_V1);
    ctx->comparisonResult = ADMG_GetConfig(tag) != 0 ? 1 : 0;
}

ADMG_RETAIN_FUNCTION(ADMG_ScriptTestConfig);
#endif
