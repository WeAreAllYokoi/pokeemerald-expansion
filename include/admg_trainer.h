#ifndef ADMG_TRAINER_H
#define ADMG_TRAINER_H

#include "admg_native.h"
#include "constants/admg_trainer_ids.h"

struct Trainer;
enum ADMG_TrainerOwner
{
    ADMG_TRAINER_HOENN,
    ADMG_TRAINER_KANTO,
    ADMG_TRAINER_PARTNER,
    ADMG_TRAINER_ADDED, // FIRST_ADDED_TRAINER and up: gADMG_AddedTrainers
};

/* Source identity is independent of map, region, campaign and eligibility.
 * Existing facility/player/network domains retain their separate contexts. */
struct ADMG_TrainerRef
{
    u32 canonicalId;
    u32 nativeIndex;
    enum ADMG_TrainerOwner owner;
};

ADMG_NATIVE_ENTRY bool8 ADMG_TryResolveTrainer(u32 trainerId, struct ADMG_TrainerRef *ref);
ADMG_NATIVE_ENTRY bool8 ADMG_TryGetOwnedTrainer(u32 owner, u32 index, u32 difficulty, const struct Trainer **trainer);
ADMG_NATIVE_ENTRY bool8 ADMG_TryGetTrainerRow(u32 trainerId, u32 difficulty, const struct Trainer **trainer);
ADMG_NATIVE_ENTRY bool8 ADMG_TryGetTrainerStruct(u32 trainerId, const struct Trainer **trainer);
ADMG_NATIVE_ENTRY bool8 ADMG_TryGetTrainerDefeatFlagId(u32 trainerId, u32 *flagId);

#endif
