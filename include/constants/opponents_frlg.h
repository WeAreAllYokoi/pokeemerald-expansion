#ifndef GUARD_CONSTANTS_OPPONENTS_FRLG_H
#define GUARD_CONSTANTS_OPPONENTS_FRLG_H

#include "constants/battle_partner.h"

/* Canonical references and owned array indices share one reviewed generator. */
#include "constants/admg_trainer_ids.h"

// NOTE: Because each Trainer uses a flag to determine when they are defeated, there is
//       only space for 25 additional trainers before trainer flag space overflows.
//       MAX_TRAINERS_COUNT_FRLG can be increased but will take up additional saveblock space

#define TRAINERS_COUNT_FRLG                      624
#define MAX_TRAINERS_COUNT_FRLG                  768

#endif  // GUARD_CONSTANTS_OPPONENTS_FRLG_H
