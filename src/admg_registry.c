#include "global.h"
#include "random.h"
#include "battle_main.h"

u32 ADMG_RandomSpecies(u32 first, u32 baseEnd)
{
    // Combine the requested base range with the added species range
    u32 added = RUNTIME_SPECIES_COUNT > FIRST_ADDED_SPECIES ? RUNTIME_SPECIES_COUNT - FIRST_ADDED_SPECIES : 0;
    u32 range = baseEnd - first + added;
    u32 species = first + ((range <= 0x10000) ? Random() % range : Random32() % range);
    return species < baseEnd ? species : species - baseEnd + FIRST_ADDED_SPECIES;
}

u16 ADMG_TypeChartEntry(u32 atkType, u32 defType)
{
    return ((const uq4_12_t *)gTypeEffectivenessTable)[atkType * RUNTIME_TYPES_COUNT + defType];
}
