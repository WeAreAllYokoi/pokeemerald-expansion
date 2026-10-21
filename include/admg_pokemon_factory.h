#ifndef ADMG_POKEMON_FACTORY_H
#define ADMG_POKEMON_FACTORY_H

#include "admg_native.h"

struct Pokemon;
struct BoxPokemon;
struct PokemonTemplate;
struct OriginalTrainerId;

/* Adapt word-sized arguments and a trainer-ID pointer to native creation functions
 * that take struct OriginalTrainerId by value. Return TRUE after calling the creation function.
 * Return FALSE without changing the output if either pointer is NULL, the trainer-ID
 * method is unknown, the species is SPECIES_NONE, out of range or disabled,
 * or the level is outside MIN_LEVEL..MAX_LEVEL. */
ADMG_NATIVE_ENTRY ADMG_Word ADMG_CreateMon(struct Pokemon *mon, ADMG_Word species, ADMG_Word level, ADMG_Word personality,
                                           const struct OriginalTrainerId *trainerId);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_CreateBoxMon(struct BoxPokemon *boxMon, ADMG_Word species, ADMG_Word level, ADMG_Word personality,
                                              const struct OriginalTrainerId *trainerId);
/* CreateMonFromTemplate with the given original trainer instead of the player. */
ADMG_NATIVE_ENTRY ADMG_Word ADMG_CreateMonFromTemplate(struct Pokemon *mon, const struct PokemonTemplate *monTemplate,
                                                       const struct OriginalTrainerId *trainerId);
/* Set the original trainer's name and gender, and mark the met location as an in-game trade.
 * Return FALSE without changing the record if mon or otName is NULL,
 * the record has no species, or otGender is neither MALE nor FEMALE. */
ADMG_NATIVE_ENTRY ADMG_Word ADMG_SetMonTradeOrigin(struct Pokemon *mon, const u8 *otName, ADMG_Word otGender);

#endif
