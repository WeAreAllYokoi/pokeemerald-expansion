#ifndef GUARD_BATTLE_TOWER_H
#define GUARD_BATTLE_TOWER_H

#include "data.h"
#include "admg_trainer_actor.h"
#include "battle_frontier.h"
#include "frontier_util.h"

struct RSBattleTowerRecord
{
    /*0x00*/ u8 lvlMode; // 0 = level 50, 1 = level 100
    /*0x01*/ u8 facilityClass;
    /*0x02*/ u16 winStreak;
    /*0x04*/ u8 name[PLAYER_NAME_LENGTH + 1];
    /*0x0C*/ u8 trainerId[TRAINER_ID_LENGTH];
    /*0x10*/ u16 greeting[EASY_CHAT_BATTLE_WORDS_COUNT];
    /*0x1C*/ struct BattleTowerPokemon party[FRONTIER_PARTY_SIZE];
    /*0xA0*/ u32 checksum;
};

struct FacilityClass {
    u16 class;
    u16 gfxId;
};

#define FACILITY_CLASSES_MALE   30
#define FACILITY_CLASSES_FEMALE 20

extern const struct FacilityClass gTowerMaleFacilityClasses[FACILITY_CLASSES_MALE];
extern const struct FacilityClass gTowerFemaleFacilityClasses[FACILITY_CLASSES_FEMALE];
extern const struct TrainerMon gSlateportBattleTentMons[];
extern const struct BattleFrontierTrainer gSlateportBattleTentTrainers[];

void CallBattleTowerFunc(void);
void PutNewBattleTowerRecord(struct EmeraldBattleTowerRecord *newRecordEm);
void CalcEmeraldBattleTowerChecksum(struct EmeraldBattleTowerRecord *record);
void CalcRubyBattleTowerChecksum(struct RSBattleTowerRecord *record);
u16 GetCurrentBattleTowerWinStreak(enum FrontierLevelMode lvlMode, u8 battleMode);
void TryHideBattleTowerReporter(void);
bool32 RubyBattleTowerRecordToEmerald(struct RSBattleTowerRecord *src, struct EmeraldBattleTowerRecord *dst);
bool32 EmeraldBattleTowerRecordToRuby(struct EmeraldBattleTowerRecord *src, struct RSBattleTowerRecord *dst);
void CalcApprenticeChecksum(struct Apprentice *apprentice);
void GetBattleTowerTrainerLanguage(u8 *dst, u16 trainerId);
bool8 ADMG_TrySelectFacilityTrainerTables(bool8 tent, const struct BattleFrontierTrainer **trainers, const struct TrainerMon **mons, u32 *count);
bool8 ADMG_TryGetFacilityTrainerRow(u32 trainerId, const struct BattleFrontierTrainer **trainer);
bool8 ADMG_TryGetFacilityTrainerRowFromTable(const struct BattleFrontierTrainer *table, u32 count, u32 trainerId, const struct BattleFrontierTrainer **trainer);
bool8 ADMG_TryGetActiveFacilityTrainerRow(u32 trainerId, const struct BattleFrontierTrainer **trainer);
bool8 ADMG_TryGetFacilityMonTableCount(const struct TrainerMon *table, u32 *count);
bool8 ADMG_TryGetFacilityMonRowFromTable(const struct TrainerMon *table, u32 count, u32 monId, const struct TrainerMon **mon);
bool8 ADMG_TryGetFacilityMonSetFromSpan(const u16 *set, u32 extent, u32 monCount, const u16 **selectedSet, u32 *selectedCount);
bool8 ADMG_TryGetFacilityTrainerMonSet(const struct BattleFrontierTrainer *table, u32 trainerId, const struct TrainerMon **mons, const u16 **set, u32 *count);
/* Ephemeral constructor inputs only: this is not a saved layout. */
struct ADMG_FacilityPartyInput
{
    struct ADMG_TrainerActorRef actor;
    struct SaveBlock2 *save;
    enum FrontierLevelMode mode;
    enum BattleTrainer target;
    u8 requestedCount, firstMon, level;
    bool8 tent;
    const struct TrainerMon *mons;
    const struct BattleFrontierTrainer *table;
    const u16 *monSet;
    u32 choiceCount;
    union
    {
        struct EmeraldBattleTowerRecord record;
        struct Apprentice apprentice;
        struct BattleTowerEReaderTrainer ereader;
        struct ADMG_FrontierBrainSelection brain;
    } stored;
};
/* Pure internal admission; reuses the existing constructor input contract. */
ADMG_NATIVE_ENTRY bool8 ADMG_TryAdmitFacilityParty(u32 identity, u32 target, u32 count, bool8 tent, u32 firstMon, struct ADMG_FacilityPartyInput *input);
bool8 ADMG_TryPrepareFacilityParty(u32 identity, u32 target, u32 count, bool8 tent, u32 firstMon, struct ADMG_FacilityPartyInput *input);
const struct ADMG_FacilityPartyInput *ADMG_GetBorrowedFacilityParty(void);
const struct ADMG_FacilityPartyInput *ADMG_BorrowFacilityParty(const struct ADMG_FacilityPartyInput *input);
void ADMG_FillFacilityPartyFromArgs(u16 identity, enum BattleTrainer target, u8 count, bool8 tent, u8 firstMon);

u8 SetTentPtrsGetLevel(void);
void TrySetLinkBattleTowerEnemyPartyLevel(void);
void FillTentTrainerParty(u8 monsCount);


#endif //GUARD_BATTLE_TOWER_H
