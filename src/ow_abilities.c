#include "global.h"
#include "ow_abilities.h"
#include "pokemon.h"
#include "random.h"
#include "constants/pokemon.h"

const static enum Ability sForceNatureAbilities[] = {ABILITY_SYNCHRONIZE, ABILITY_NONE};
const static enum Ability sForceOppositeGenderAbilities[] = {ABILITY_CUTE_CHARM, ABILITY_NONE};
const static enum Ability sIncreaseHatchingSpeedAbilities[] = {ABILITY_MAGMA_ARMOR, ABILITY_FLAME_BODY, ABILITY_STEAM_ENGINE, ABILITY_NONE};

static UNUSED bool32 HasHalfChance(enum Species species);
static UNUSED bool32 HasTwoThirdsChance(enum Species species);
static UNUSED bool32 IsFalse(enum Species species);
static UNUSED bool32 IsTrue(enum Species species);
static UNUSED bool32 IsTrueIfUndiscoveredEggGroup(enum Species species);

// Keep all Synchronize rule sets; SynchronizeRules chooses one from OW_SYNCHRONIZE_NATURE
// Each row covers wild encounters, static encounters, roamers and gifts
enum { SYNC_RULES_GEN_3, SYNC_RULES_UP_TO_GEN_5, SYNC_RULES_GEN_6, SYNC_RULES_GEN_7, SYNC_RULES_GEN_8, SYNC_RULES_LATER, SYNC_RULES_COUNT };

static const bool32 (*const sSynchronizeModes[SYNC_RULES_COUNT][GIFTMON_ORIGIN + 1])(enum Species) =
{
    [SYNC_RULES_GEN_3] =
    {
        [WILDMON_ORIGIN] = HasHalfChance,
        [STATIC_WILDMON_ORIGIN] = IsFalse,
        [ROAMER_ORIGIN] = IsFalse,
        [GIFTMON_ORIGIN] = IsFalse,
    },
    [SYNC_RULES_UP_TO_GEN_5] =
    {
        [WILDMON_ORIGIN] = HasHalfChance,
        [STATIC_WILDMON_ORIGIN] = HasHalfChance,
        [ROAMER_ORIGIN] = IsFalse,
        [GIFTMON_ORIGIN] = IsFalse,
    },
    [SYNC_RULES_GEN_6] =
    {
        [WILDMON_ORIGIN] = HasHalfChance,
        [STATIC_WILDMON_ORIGIN] = HasHalfChance,
        [ROAMER_ORIGIN] = IsFalse,
        [GIFTMON_ORIGIN] = IsTrueIfUndiscoveredEggGroup,
    },
    [SYNC_RULES_GEN_7] =
    {
        [WILDMON_ORIGIN] = HasHalfChance,
        [STATIC_WILDMON_ORIGIN] = HasHalfChance,
        [ROAMER_ORIGIN] = IsFalse,
        [GIFTMON_ORIGIN] = IsTrue,
    },
    [SYNC_RULES_GEN_8] =
    {
        [WILDMON_ORIGIN] = IsTrue,
        [STATIC_WILDMON_ORIGIN] = IsFalse,
        [ROAMER_ORIGIN] = IsTrue,
        [GIFTMON_ORIGIN] = IsFalse,
    },
    [SYNC_RULES_LATER] =
    {
        [WILDMON_ORIGIN] = IsFalse,
        [STATIC_WILDMON_ORIGIN] = IsFalse,
        [ROAMER_ORIGIN] = IsFalse,
        [GIFTMON_ORIGIN] = IsFalse,
    },
};

static u32 SynchronizeRules(void)
{
    u32 generation = OW_SYNCHRONIZE_NATURE;
    if (generation == GEN_3)
        return SYNC_RULES_GEN_3;
    if (generation <= GEN_5)
        return SYNC_RULES_UP_TO_GEN_5;
    if (generation == GEN_6)
        return SYNC_RULES_GEN_6;
    if (generation == GEN_7)
        return SYNC_RULES_GEN_7;
    if (generation == GEN_8)
        return SYNC_RULES_GEN_8;
    return SYNC_RULES_LATER;
}

static const bool32 (*const sCuteCharmModes[])(enum Species) = 
{
    [WILDMON_ORIGIN] = HasTwoThirdsChance,
    [STATIC_WILDMON_ORIGIN] = HasTwoThirdsChance,
    [ROAMER_ORIGIN] = IsFalse,
    [GIFTMON_ORIGIN] = IsFalse,
};

static UNUSED bool32 HasHalfChance(enum Species species)
{
    return Random() % 2;
}

static UNUSED bool32 HasTwoThirdsChance(enum Species species)
{
    return Random() % 3;
}

static UNUSED bool32 IsFalse(enum Species species)
{
    return FALSE;
}

static UNUSED bool32 IsTrue(enum Species species)
{
    return TRUE;
}

static UNUSED bool32 IsTrueIfUndiscoveredEggGroup(enum Species species)
{
    return (gSpeciesInfo[species].eggGroups[0] == EGG_GROUP_NO_EGGS_DISCOVERED);
}

bool32 DoesLeadingMonHaveAbilityEffect(const enum Ability *abilityArray)
{
    if (GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SANITY_IS_EGG))
        return FALSE;
    enum Ability leadingMonAbility = GetMonAbility(&gParties[B_TRAINER_PLAYER][0]);
    for (u32 i = 0; abilityArray[i] != ABILITY_NONE; i++)
    {
        if (leadingMonAbility == abilityArray[i])
            return TRUE;
    }
    return FALSE;
}

bool32 DoesPartyMemberHaveAbilityEffect(const enum Ability *abilityArray)
{
    for (u32 j = 0; j < gPartiesCount[B_TRAINER_PLAYER]; j++)
    {
        if (GetMonData(&gParties[B_TRAINER_PLAYER][j], MON_DATA_SANITY_IS_EGG))
            continue;
        enum Ability monAbility = GetMonAbility(&gParties[B_TRAINER_PLAYER][j]);
        for (u32 i = 0; abilityArray[i] != ABILITY_NONE; i++)
        {
            if (monAbility == abilityArray[i])
                return TRUE;
        }
    }
    return FALSE;
}

u32 GetSynchronizedNature(enum GeneratedMonOrigin origin, enum Species species)
{
    if (!DoesLeadingMonHaveAbilityEffect(sForceNatureAbilities))
        return NATURE_RANDOM;
    if (!(sSynchronizeModes[SynchronizeRules()][origin](species)))
        return NATURE_RANDOM;
    return GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_PERSONALITY) % NUM_NATURES;
}

u32 GetSynchronizedGender(enum GeneratedMonOrigin origin, enum Species species)
{
    if (!DoesLeadingMonHaveAbilityEffect(sForceOppositeGenderAbilities))
        return MON_GENDER_RANDOM;
    if (!(sCuteCharmModes[origin](species)))
        return MON_GENDER_RANDOM;
    u8 leadingMonGender = GetMonGender(&gParties[B_TRAINER_PLAYER][0]);
    // misses mon is genderless check, although no genderless mon can have cute charm as ability
    if (leadingMonGender == MON_FEMALE)
        return MON_MALE;
    else
        return MON_FEMALE;
}

bool32 DoesPartyHaveIncubatorMon(void)
{
    return DoesPartyMemberHaveAbilityEffect(sIncreaseHatchingSpeedAbilities);
}
