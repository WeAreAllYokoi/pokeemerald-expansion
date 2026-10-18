#include "global.h"

// Built-in table sizes, for the native registry to update in ROM before startup
const struct ADMG_RegistryCounts gADMG_RegistryCounts =
{
    .species = NUM_SPECIES + 1,
    .moves = MOVES_COUNT_ALL,
    .items = ITEMS_COUNT,
    .abilities = ABILITIES_COUNT,
    .types = NUMBER_OF_MON_TYPES,
    .natures = NUM_NATURES,
    .trainerClasses = TRAINER_CLASS_COUNT,
    .trainerPics = TRAINER_PIC_COUNT,
    .addedTrainers = 0,
    .nationalDex = NATIONAL_DEX_COUNT + 1,
};
