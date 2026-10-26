#ifndef GUARD_BATTLE_FACTORY_H
#define GUARD_BATTLE_FACTORY_H

#include "battle_tower.h"

void CallBattleFactoryFunction(void);
bool8 InBattleFactory(void);
u8 GetFactoryMonFixedIV(u8 challengeNum, bool8 isLastBattle);
void FillFactoryBrainParty(void);
u8 GetNumPastRentalsRank(u8 battleMode, enum FrontierLevelMode lvlMode);
u64 GetAiScriptsInBattleFactory(void);
/* Borrowed constructor inputs; never serialized or stored in a save. */
struct ADMG_FactoryPartyInput
{
    struct ADMG_TrainerActorRef actor;
    struct SaveBlock2 *save;
    enum FrontierLevelMode mode;
    bool8 tent;
    u8 firstMon, level, fixedIV, challenge;
    u32 otId, monCount;
    const struct TrainerMon *mons, *rows[FRONTIER_PARTY_SIZE];
    u16 rentalIds[PARTY_SIZE];
#if FREE_BATTLE_TOWER_E_READER == FALSE
    struct BattleTowerEReaderTrainer ereader;
#endif
};
bool8 ADMG_TryPrepareFactoryParty(u32 identity, bool8 tent, u32 firstMon, struct ADMG_FactoryPartyInput *input);
const struct ADMG_FactoryPartyInput *ADMG_BorrowFactoryParty(const struct ADMG_FactoryPartyInput *input);

void FillFactoryTrainerParty(void);

#endif // GUARD_BATTLE_FACTORY_H
