#ifndef ADMG_BATTLE_GIMMICKS_H
#define ADMG_BATTLE_GIMMICKS_H
#include "admg_native.h"

/* Select an eligible gimmick while HandleTurnActionSelectionState is active. */
ADMG_NATIVE_ENTRY ADMG_Word ADMG_SelectBattleGimmick(ADMG_Word battler, ADMG_Word gimmick);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_CanPrimalRevert(ADMG_Word battler);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_CanGigantamax(ADMG_Word battler);
#endif
