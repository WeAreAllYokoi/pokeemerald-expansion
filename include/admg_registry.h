#ifndef GUARD_ADMG_REGISTRY_H
#define GUARD_ADMG_REGISTRY_H

// ADMG-101 extensible content registries.
//
// Each content table uses 32-bit IDs, with built-in rows followed by rows added at run time
// FIRST_ADDED_<KIND> leaves the base count reserved; species also reserve SPECIES_EGG
// The native registry relocates the tables and writes these counts before the game starts
// With no added content, the default counts cover the built-in rows

#include "constants/moves.h"
#include "constants/items.h"
#include "constants/abilities.h"
#include "constants/trainers.h"
#include "constants/pokedex.h"

struct ADMG_RegistryCounts
{
    u32 species;        // Rows of gSpeciesInfo, including SPECIES_EGG (= NUM_SPECIES)
    u32 moves;          // Rows of gMovesInfo
    u32 items;          // Rows of gItemsInfo
    u32 abilities;      // Rows of gAbilitiesInfo
    u32 types;          // Rows of gTypesInfo, and the row length of gTypeEffectivenessTable
    u32 natures;        // Rows of gNaturesInfo
    u32 trainerClasses; // Rows of gTrainerClasses
    u32 trainerPics;    // Rows of gTrainerPicInfo
    u32 addedTrainers;  // Rows per difficulty of gADMG_AddedTrainers: the trainers added at run time
    u32 nationalDex;    // National Pokédex numbers, including NATIONAL_DEX_NONE
};

// Keep the defaults in a separate source file so callers read the counts from ROM
extern const struct ADMG_RegistryCounts gADMG_RegistryCounts;

#define RUNTIME_SPECIES_COUNT          (gADMG_RegistryCounts.species)
#define RUNTIME_MOVES_COUNT            (gADMG_RegistryCounts.moves)
#define RUNTIME_ITEMS_COUNT            (gADMG_RegistryCounts.items)
#define RUNTIME_ABILITIES_COUNT        (gADMG_RegistryCounts.abilities)
#define RUNTIME_TYPES_COUNT            (gADMG_RegistryCounts.types)
#define RUNTIME_NATURES_COUNT          (gADMG_RegistryCounts.natures)
#define RUNTIME_TRAINER_CLASSES_COUNT  (gADMG_RegistryCounts.trainerClasses)
#define RUNTIME_TRAINER_PICS_COUNT     (gADMG_RegistryCounts.trainerPics)
#define RUNTIME_ADDED_TRAINERS_COUNT   (gADMG_RegistryCounts.addedTrainers)
#define RUNTIME_NATIONAL_DEX_COUNT     (gADMG_RegistryCounts.nationalDex)

#define FIRST_ADDED_SPECIES        (NUM_SPECIES + 1)
#define FIRST_ADDED_MOVE           (MOVES_COUNT_ALL + 1)
#define FIRST_ADDED_ITEM           (ITEMS_COUNT + 1)
#define FIRST_ADDED_ABILITY        (ABILITIES_COUNT + 1)
#define FIRST_ADDED_TYPE           (NUMBER_OF_MON_TYPES + 1)
#define FIRST_ADDED_NATURE         (NUM_NATURES + 1)
#define FIRST_ADDED_TRAINER_CLASS  (TRAINER_CLASS_COUNT + 1)
#define FIRST_ADDED_TRAINER_PIC    (TRAINER_PIC_COUNT + 1)
// Trainer IDs share 16 bits with partners and the special IDs of other battle kinds, so added trainers start above
// all of them.
#define FIRST_ADDED_TRAINER        0x10000
#define FIRST_ADDED_NATIONAL_DEX   (NATIONAL_DEX_COUNT + 1)

static inline bool32 IsSpeciesIdValid(u32 species) { return species < RUNTIME_SPECIES_COUNT; }
static inline bool32 IsMoveIdValid(u32 move) { return move < RUNTIME_MOVES_COUNT && move != MOVES_COUNT_ALL; }
// Skip the built-in Z-Move and Max Move ranges; allow registered added moves
static inline bool32 IsRegularMoveId(u32 move) { return move < MOVES_COUNT || (move >= FIRST_ADDED_MOVE && move < RUNTIME_MOVES_COUNT); }
static inline bool32 IsItemIdValid(u32 item) { return item < RUNTIME_ITEMS_COUNT && item != ITEMS_COUNT; }
static inline bool32 IsAbilityIdValid(u32 ability) { return ability < RUNTIME_ABILITIES_COUNT && ability != ABILITIES_COUNT; }
static inline bool32 IsTypeIdValid(u32 type) { return type < RUNTIME_TYPES_COUNT && type != NUMBER_OF_MON_TYPES; }
static inline bool32 IsNatureIdValid(u32 nature) { return nature < RUNTIME_NATURES_COUNT && nature != NUM_NATURES; }
static inline bool32 IsTrainerClassIdValid(u32 trainerClass) { return trainerClass < RUNTIME_TRAINER_CLASSES_COUNT && trainerClass != TRAINER_CLASS_COUNT; }
static inline bool32 IsTrainerPicIdValid(u32 pic) { return pic < RUNTIME_TRAINER_PICS_COUNT && pic != TRAINER_PIC_COUNT; }

// Advance past SPECIES_EGG in species loops, or the reserved base count for types and items
#define NEXT_SPECIES_ID(i) ((i) + 1 == SPECIES_EGG ? SPECIES_EGG + 1 : (i) + 1)
#define NEXT_TYPE_ID(i)    ((i) + 1 == NUMBER_OF_MON_TYPES ? (i) + 2 : (i) + 1)
#define NEXT_ITEM_ID(i)    ((i) + 1 == ITEMS_COUNT ? (i) + 2 : (i) + 1)

// Choose from [first, baseEnd), then the added species starting at FIRST_ADDED_SPECIES
// Keep the original Random() draw when no species have been added
u32 ADMG_RandomSpecies(u32 first, u32 baseEnd);
// Look up [attacker][defender] using the registered type count as the row width
u16 ADMG_TypeChartEntry(u32 atkType, u32 defType);

#endif // GUARD_ADMG_REGISTRY_H
