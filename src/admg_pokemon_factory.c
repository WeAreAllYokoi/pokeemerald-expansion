#include "global.h"
#include "admg_pokemon_factory.h"
#include "admg_registry.h"
#include "admg_retention.h"
#include "pokemon.h"
#include "constants/region_map_sections.h"

static bool32 IsCreatableSpecies(u32 species)
{
    return species != SPECIES_NONE && IsSpeciesIdValid(species) && IsSpeciesEnabled(species);
}

static bool32 IsTrainerIdValid(const struct OriginalTrainerId *trainerId)
{
    if (trainerId == NULL)
        return FALSE;
    switch (trainerId->method)
    {
    case OT_ID_PLAYER_ID:
    case OT_ID_PRESET:
    case OT_ID_RANDOM_NO_SHINY:
        return TRUE;
    default:
        return FALSE;
    }
}

static bool32 AreCreationArgumentsValid(const void *output, u32 species, u32 level, const struct OriginalTrainerId *trainerId)
{
    return output != NULL && IsCreatableSpecies(species) && level >= MIN_LEVEL && level <= MAX_LEVEL && IsTrainerIdValid(trainerId);
}

ADMG_NATIVE_ENTRY ADMG_Word ADMG_CreateMon(struct Pokemon *mon, ADMG_Word species, ADMG_Word level, ADMG_Word personality,
                                           const struct OriginalTrainerId *trainerId)
{
    if (!AreCreationArgumentsValid(mon, species, level, trainerId))
        return FALSE;
    CreateMon(mon, species, level, personality, *trainerId);
    return TRUE;
}

ADMG_NATIVE_ENTRY ADMG_Word ADMG_CreateBoxMon(struct BoxPokemon *boxMon, ADMG_Word species, ADMG_Word level, ADMG_Word personality,
                                              const struct OriginalTrainerId *trainerId)
{
    if (!AreCreationArgumentsValid(boxMon, species, level, trainerId))
        return FALSE;
    CreateBoxMon(boxMon, species, level, personality, *trainerId);
    return TRUE;
}

ADMG_NATIVE_ENTRY ADMG_Word ADMG_CreateMonFromTemplate(struct Pokemon *mon, const struct PokemonTemplate *monTemplate,
                                                       const struct OriginalTrainerId *trainerId)
{
    /* Reject invalid species and levels instead of relying on the template resolvers'
     * assertion fallbacks to SPECIES_PORYGON and MIN_LEVEL. */
    if (monTemplate == NULL || !AreCreationArgumentsValid(mon, monTemplate->species, monTemplate->level, trainerId))
        return FALSE;
    CreateMonFromTemplateWithTrainerId(mon, monTemplate, *trainerId);
    return TRUE;
}

ADMG_NATIVE_ENTRY ADMG_Word ADMG_SetMonTradeOrigin(struct Pokemon *mon, const u8 *otName, ADMG_Word otGender)
{
    u8 gender;
    metloc_u8_t metLocation = METLOC_IN_GAME_TRADE;

    if (mon == NULL || otName == NULL || (otGender != MALE && otGender != FEMALE) || !GetMonData(mon, MON_DATA_SANITY_HAS_SPECIES))
        return FALSE;
    gender = otGender;
    SetMonData(mon, MON_DATA_OT_NAME, otName);
    SetMonData(mon, MON_DATA_OT_GENDER, &gender);
    SetMonData(mon, MON_DATA_MET_LOCATION, &metLocation);
    return TRUE;
}

ADMG_RETAIN_FUNCTION(ADMG_CreateMon);
ADMG_RETAIN_FUNCTION(ADMG_CreateBoxMon);
ADMG_RETAIN_FUNCTION(ADMG_CreateMonFromTemplate);
ADMG_RETAIN_FUNCTION(ADMG_SetMonTradeOrigin);
