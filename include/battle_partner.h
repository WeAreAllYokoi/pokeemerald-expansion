#ifndef BATTLE_PARTNER_H
#define BATTLE_PARTNER_H

#include "difficulty.h"
#include "constants/battle_partner.h"

/* Internal admission only; no team-policy calls, party writes or saved state. */
bool8 ADMG_TryAdmitPartnerParty(u32 trainerId);
void FillPartnerParty(enum TrainerId trainerId);

#endif // BATTLE_PARTNER_H
