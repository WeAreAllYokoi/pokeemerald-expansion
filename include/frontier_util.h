#ifndef GUARD_FRONTIER_UTIL_H
#define GUARD_FRONTIER_UTIL_H

#include "constants/species.h"

void CallFrontierUtilFunc(void);
u8 GetFrontierBrainStatus(void);
void CopyFrontierTrainerText(u8 whichText, u16 trainerId);
void ResetWinStreaks(void);
u32 GetCurrentFacilityWinStreak(void);
void ResetFrontierTrainerIds(void);
u8 GetPlayerSymbolCountForFacility(u8 facility);
void ShowRankingHallRecordsWindow(void);
void ScrollRankingHallRecordsWindow(void);
void ClearRankingHallRecords(void);
void SaveGameFrontier(void);
enum TrainerPicID GetFrontierBrainTrainerPicIndex(void);
enum TrainerClassID GetFrontierBrainTrainerClass(void);
void CopyFrontierBrainTrainerName(u8 *dst);
bool8 IsFrontierBrainFemale(void);
void SetFrontierBrainObjEventGfx_2(void);
void CreateFrontierBrainPokemon(void);
enum Species GetFrontierBrainMonSpecies(u8 monId);
void SetFrontierBrainObjEventGfx(u8 facility);
u16 GetFrontierBrainMonMove(u8 monId, u8 moveSlotId);
u8 GetFrontierBrainMonNature(u8 monId);
u8 GetFrontierBrainMonEvs(u8 monId, u8 evStatId);
s32 GetFronterBrainSymbol(void);
void ClearEnemyPartyAfterChallenge(void);
bool8 IsFrontierTrainerFemale(u16 trainerId);
u8 GetFrontierTrainerFixedIvs(u16 trainerId);
bool8 ADMG_TryGetFrontierTrainerIdRange(u32 challenge, u32 battle, u16 *first, u16 *last);
u16 GetRandomScaledFrontierTrainerId(u8 challengeNum, u8 battleNum);
void SetBattleFacilityTrainerGfxId(u16 trainerId, u8 tempVarId);
u16 GetBattleFacilityTrainerGfxId(u16 trainerId);
u8 GetFrontierTrainerFrontSpriteId(u16 trainerId);
enum TrainerClassID GetFrontierOpponentClass(u16 trainerId);
u8 GetFrontierTrainerFacilityClass(u16 trainerId);
void GetFrontierTrainerName(u8 *dst, u16 trainerId);
u16 GetRandomFrontierMonFromSet(u16 trainerId);
void FrontierSpeechToString(const u16 *words);
u8 SetFacilityPtrsGetLevel(void);
u8 GetFrontierEnemyMonLevel(enum FrontierLevelMode lvlMode);
s32 GetHighestLevelInPlayerParty(void);
u16 FacilityClassToGraphicsId(u8 facilityClass);
void ShowBattleFrontierCaughtBannedSpecies(void);

struct FrontierBrain
{
    u16 trainerId;
    u8 objEventGfx;
    u8 isFemale;
    const u8 *lostTexts[2];
    const u8 *wonTexts[2];
    u16 battledBit[2];
    u8 streakAppearances[4];
    u16 goldSymbolFlag;
    u16 silverSymbolFlag;
};

struct FrontierBrainMon
{
    enum Species species;
    enum Item heldItem;
    u8 fixedIV;
    nature_t nature;
    u8 evs[NUM_STATS];
    enum Move moves[MAX_MON_MOVES];
};

/* Temporary selection only; no saved or mod-persistence layout. */
struct ADMG_FrontierBrainSelection
{
    const struct FrontierBrainMon *mons;
    u32 facility, symbol;
    s32 tournamentId;
};
bool8 ADMG_TryGetFrontierBrainInfo(u32 facility, const struct FrontierBrain **info);
bool8 ADMG_TryGetFrontierBrainMon(u32 facility, u32 symbol, u32 monId, const struct FrontierBrainMon **mon);
bool8 ADMG_TryPrepareFrontierBrainSelection(struct ADMG_FrontierBrainSelection *selection);

extern const struct FrontierBrain gFrontierBrainInfo[];

/* Internal bounded text admission; not a saved layout or public mod SDK. */
bool8 ADMG_TryValidateTrainerText(const u8 *text, u32 extent, u32 identity, u32 *length);

#endif // GUARD_FRONTIER_UTIL_H
