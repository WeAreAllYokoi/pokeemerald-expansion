#ifndef GUARD_RANDOM_MON_GENERATION_H
#define GUARD_RANDOM_MON_GENERATION_H

#include "gba/types.h"

#include "constants/hold_effects.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/pokeball.h"
#include "constants/species.h"

#define FILTER_FUNC_ARG_NONE 0xFFFF
#define RANDOM_MON_MAX_FORMS 70

struct FilterFuncArgs
{
    u32 arg1;
    u32 arg2;
};

enum RandomSpeciesDexMode
{
    RANDOM_MON_DEX_NATIONAL,
    RANDOM_MON_DEX_HOENN,
};

struct RandomSpeciesGeneratorOptions
{
    const enum Species *bannedSpecies;
    const enum Species *speciesPool;
    bool32 (*filterFunc)(enum Species species, const struct FilterFuncArgs *filterFuncArgs);
    u32 speciesPoolCount:11;
    u32 bannedSpeciesCount:11;
    u32 banLegendary:1;
    u32 banMythical:1;
    u32 banSubLegendary:1;
    u32 banUltraBeast:1;
    u32 banParadox:1;
    u32 randomizeForms:1;
    enum RandomSpeciesDexMode dexMode:1;
    u32 padding:3;
};

struct RandomItemGeneratorOptions
{
    const enum Item *heldItemPool;
    const enum HoldEffect *bannedHoldEffects;
    bool32 (*filterFunc)(enum Item item, const struct FilterFuncArgs *filterFuncArgs);
    u16 heldItemPoolCount;
    u16 bannedHoldEffectsCount;
};

enum Species GetRandomSpecies(u32 optionId, const struct FilterFuncArgs *filterFuncArgs);
enum Item GetRandomItem(u32 optionId, const struct FilterFuncArgs *filterFuncArgs);
enum PokeBall GetRandomBall(void);
void ResolveMoves(enum Species species, u32 level, const u32 *movesTemplate, enum Move *moves);

// Random-generation option providers and candidate counts without a random selection
#include "admg_native.h"
ADMG_NATIVE_ENTRY const struct RandomSpeciesGeneratorOptions *ADMG_GetRandomSpeciesOptions(ADMG_Word optionId);
ADMG_NATIVE_ENTRY const struct RandomItemGeneratorOptions *ADMG_GetRandomItemOptions(ADMG_Word optionId);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_CountRandomSpeciesCandidates(ADMG_Word optionId, const struct FilterFuncArgs *filterFuncArgs);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_CountRandomItemCandidates(ADMG_Word optionId, const struct FilterFuncArgs *filterFuncArgs);

#endif // GUARD_RANDOM_MON_GENERATION_H
