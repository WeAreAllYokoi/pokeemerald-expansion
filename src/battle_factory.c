#include "global.h"
#include "admg_state.h"
#include "battle.h"
#include "battle_factory.h"
#include "battle_factory_screen.h"
#include "event_data.h"
#include "battle_setup.h"
#include "overworld.h"
#include "frontier_util.h"
#include "battle_tower.h"
#include "random.h"
#include "constants/battle_ai.h"
#include "constants/battle_factory.h"
#include "constants/battle_frontier.h"
#include "constants/battle_frontier_mons.h"
#include "constants/battle_tent.h"
#include "constants/frontier_util.h"
#include "constants/layouts.h"
#include "constants/trainers.h"
#include "constants/moves.h"
#include "constants/items.h"

static bool8 sPerformedRentalSwap;

static void InitFactoryChallenge(void);
static void GetBattleFactoryData(void);
static void SetBattleFactoryData(void);
static void SaveFactoryChallenge(void);
static void FactoryDummy1(void);
static void FactoryDummy2(void);
static void SelectInitialRentalMons(void);
static void SwapRentalMons(void);
static void SetPerformedRentalSwap(void);
static void SetRentalsToOpponentParty(void);
static void SetPlayerAndOpponentParties(void);
static void SetOpponentGfxVar(void);
static void GenerateOpponentMons(void);
static void GenerateInitialRentalMons(void);
static void GetOpponentMostCommonMonType(void);
static void GetOpponentBattleStyle(void);
static void RestorePlayerPartyHeldItems(void);
ADMG_NATIVE_ENTRY u16 GetFactoryMonId(enum FrontierLevelMode lvlMode, u8 challengeNum, bool8 useBetterRange);
static enum FactoryStyle GetMoveBattleStyle(enum Move move);

// Number of moves needed on the team to be considered using a certain battle style
static const u8 sRequiredMoveCounts[FACTORY_NUM_STYLES - 1] = {
    [FACTORY_STYLE_PREPARATION - 1]   = 3,
    [FACTORY_STYLE_SLOW_STEADY - 1]   = 3,
    [FACTORY_STYLE_ENDURANCE - 1]     = 3,
    [FACTORY_STYLE_HIGH_RISK - 1]     = 2,
    [FACTORY_STYLE_WEAKENING - 1]     = 2,
    [FACTORY_STYLE_UNPREDICTABLE - 1] = 2,
    [FACTORY_STYLE_WEATHER - 1]       = 2
};

static void (*const sBattleFactoryFunctions[])(void) =
{
    [BATTLE_FACTORY_FUNC_INIT]                   = InitFactoryChallenge,
    [BATTLE_FACTORY_FUNC_GET_DATA]               = GetBattleFactoryData,
    [BATTLE_FACTORY_FUNC_SET_DATA]               = SetBattleFactoryData,
    [BATTLE_FACTORY_FUNC_SAVE]                   = SaveFactoryChallenge,
    [BATTLE_FACTORY_FUNC_NULL]                   = FactoryDummy1,
    [BATTLE_FACTORY_FUNC_NULL2]                  = FactoryDummy2,
    [BATTLE_FACTORY_FUNC_SELECT_RENT_MONS]       = SelectInitialRentalMons,
    [BATTLE_FACTORY_FUNC_SWAP_RENT_MONS]         = SwapRentalMons,
    [BATTLE_FACTORY_FUNC_SET_SWAPPED]            = SetPerformedRentalSwap,
    [BATTLE_FACTORY_FUNC_SET_OPPONENT_MONS]      = SetRentalsToOpponentParty,
    [BATTLE_FACTORY_FUNC_SET_PARTIES]            = SetPlayerAndOpponentParties,
    [BATTLE_FACTORY_FUNC_SET_OPPONENT_GFX]       = SetOpponentGfxVar,
    [BATTLE_FACTORY_FUNC_GENERATE_OPPONENT_MONS] = GenerateOpponentMons,
    [BATTLE_FACTORY_FUNC_GENERATE_RENTAL_MONS]   = GenerateInitialRentalMons,
    [BATTLE_FACTORY_FUNC_GET_OPPONENT_MON_TYPE]  = GetOpponentMostCommonMonType,
    [BATTLE_FACTORY_FUNC_GET_OPPONENT_STYLE]     = GetOpponentBattleStyle,
    [BATTLE_FACTORY_FUNC_RESET_HELD_ITEMS]       = RestorePlayerPartyHeldItems,
};

static const u32 sWinStreakFlags[][2] =
{
    {STREAK_FACTORY_SINGLES_50, STREAK_FACTORY_SINGLES_OPEN},
    {STREAK_FACTORY_DOUBLES_50, STREAK_FACTORY_DOUBLES_OPEN},
};

static const u32 sWinStreakMasks[][2] =
{
    {~(STREAK_FACTORY_SINGLES_50), ~(STREAK_FACTORY_SINGLES_OPEN)},
    {~(STREAK_FACTORY_DOUBLES_50), ~(STREAK_FACTORY_DOUBLES_OPEN)},
};

static const u8 sFixedIVTable[][2] =
{
    {3, 6},
    {6, 9},
    {9, 12},
    {12, 15},
    {15, 18},
    {21, 31},
    {31, 31},
    {31, 31},
};

static const u16 sInitialRentalMonRanges[][2] =
{
    // Level 50
    {FRONTIER_MON_GRIMER,     FRONTIER_MON_FURRET_1},   // 110 - 199
    {FRONTIER_MON_DELCATTY_1, FRONTIER_MON_CLOYSTER_1}, // 162 - 266
    {FRONTIER_MON_DELCATTY_2, FRONTIER_MON_CLOYSTER_2}, // 267 - 371
    {FRONTIER_MON_DUGTRIO_1,  FRONTIER_MON_SLAKING_1},  // 372 - 467
    {FRONTIER_MON_DUGTRIO_2,  FRONTIER_MON_SLAKING_2},  // 468 - 563
    {FRONTIER_MON_DUGTRIO_3,  FRONTIER_MON_SLAKING_3},  // 564 - 659
    {FRONTIER_MON_DUGTRIO_4,  FRONTIER_MON_SLAKING_4},  // 660 - 755
    {FRONTIER_MON_DUGTRIO_1,  FRONTIER_MONS_HIGH_TIER}, // 372 - 849

    // Open level
    {FRONTIER_MON_DUGTRIO_1, FRONTIER_MON_SLAKING_1}, // 372 - 467
    {FRONTIER_MON_DUGTRIO_2, FRONTIER_MON_SLAKING_2}, // 468 - 563
    {FRONTIER_MON_DUGTRIO_3, FRONTIER_MON_SLAKING_3}, // 564 - 659
    {FRONTIER_MON_DUGTRIO_4, FRONTIER_MON_SLAKING_4}, // 660 - 755
    {FRONTIER_MON_DUGTRIO_1, NUM_FRONTIER_MONS - 1},  // 372 - 881
    {FRONTIER_MON_DUGTRIO_1, NUM_FRONTIER_MONS - 1},  // 372 - 881
    {FRONTIER_MON_DUGTRIO_1, NUM_FRONTIER_MONS - 1},  // 372 - 881
    {FRONTIER_MON_DUGTRIO_1, NUM_FRONTIER_MONS - 1},  // 372 - 881
};

// code
void CallBattleFactoryFunction(void)
{
    u32 function = gSpecialVar_0x8004;
    if (function >= ARRAY_COUNT(sBattleFactoryFunctions))
    { ADMG_StateAccessRejected(function, __func__); return; }
    sBattleFactoryFunctions[function]();
}

static bool8 ADMG_TryGetFactoryContext(struct SaveBlock2 **save, u32 *battleMode,
    enum FrontierLevelMode *levelMode)
{
    struct SaveBlock2 *selected = gSaveBlock2Ptr;
    if (selected == NULL)
    { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    enum FrontierLevelMode level = selected->frontier.lvlMode;
    u64 generation = ADMG_BeginStateAccessCheck();
    u32 mode = VarGet(VAR_FRONTIER_BATTLE_MODE);
    if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (gSaveBlock2Ptr != selected || selected->frontier.lvlMode != level)
    { ADMG_StateAccessRejected(mode, __func__); return FALSE; }
    if (mode >= ARRAY_COUNT(selected->frontier.factoryWinStreaks)
     || (u32)level >= ARRAY_COUNT(selected->frontier.factoryWinStreaks[0]))
    { ADMG_StateAccessRejected(mode, __func__); return FALSE; }
    *save = selected; *battleMode = mode; *levelMode = level;
    return TRUE;
}

static void InitFactoryChallenge(void)
{
    u8 i;
    struct SaveBlock2 *save;
    enum FrontierLevelMode lvlMode;
    u32 battleMode;
    if (!ADMG_TryGetFactoryContext(&save, &battleMode, &lvlMode)) return;

    struct SaveBlock1 *mapSave = gSaveBlock1Ptr;
    if (mapSave == NULL) { ADMG_StateAccessRejected(0, __func__); return; }
    save->frontier.challengeStatus = 0;
    save->frontier.curChallengeBattleNum = 0;
    save->frontier.challengePaused = FALSE;
    save->frontier.disableRecordBattle = FALSE;
    if (!(save->frontier.winStreakActiveFlags & sWinStreakFlags[battleMode][lvlMode]))
    {
        save->frontier.factoryWinStreaks[battleMode][lvlMode] = 0;
        save->frontier.factoryRentsCount[battleMode][lvlMode] = 0;
    }

    sPerformedRentalSwap = FALSE;
    for (i = 0; i < ARRAY_COUNT(save->frontier.rentalMons); i++)
        save->frontier.rentalMons[i].monId = 0xFFFF;
    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
        gFrontierTempParty[i] = 0xFFFF;

    SetDynamicWarp(0, mapSave->location.mapGroup, mapSave->location.mapNum, WARP_ID_NONE);
    TRAINER_BATTLE_PARAM.opponentA = 0;
}

static void GetBattleFactoryData(void)
{
    struct SaveBlock2 *save;
    enum FrontierLevelMode lvlMode;
    u32 battleMode;
    if (!ADMG_TryGetFactoryContext(&save, &battleMode, &lvlMode)) return;

    switch (gSpecialVar_0x8005)
    {
    case FACTORY_DATA_WIN_STREAK:
        gSpecialVar_Result = save->frontier.factoryWinStreaks[battleMode][lvlMode];
        break;
    case FACTORY_DATA_WIN_STREAK_ACTIVE:
        gSpecialVar_Result = ((save->frontier.winStreakActiveFlags & sWinStreakFlags[battleMode][lvlMode]) != 0);
        break;
    case FACTORY_DATA_WIN_STREAK_SWAPS:
        gSpecialVar_Result = save->frontier.factoryRentsCount[battleMode][lvlMode];
        break;
    }
}

static void SetBattleFactoryData(void)
{
    struct SaveBlock2 *save;
    enum FrontierLevelMode lvlMode;
    u32 battleMode;
    if (!ADMG_TryGetFactoryContext(&save, &battleMode, &lvlMode)) return;

    switch (gSpecialVar_0x8005)
    {
    case FACTORY_DATA_WIN_STREAK:
        save->frontier.factoryWinStreaks[battleMode][lvlMode] = gSpecialVar_0x8006;
        break;
    case FACTORY_DATA_WIN_STREAK_ACTIVE:
        if (gSpecialVar_0x8006)
            save->frontier.winStreakActiveFlags |= sWinStreakFlags[battleMode][lvlMode];
        else
            save->frontier.winStreakActiveFlags &= sWinStreakMasks[battleMode][lvlMode];
        break;
    case FACTORY_DATA_WIN_STREAK_SWAPS:
        if (sPerformedRentalSwap == TRUE)
        {
            save->frontier.factoryRentsCount[battleMode][lvlMode] = gSpecialVar_0x8006;
            sPerformedRentalSwap = FALSE;
        }
        break;
    }
}

static void SaveFactoryChallenge(void)
{
    ClearEnemyPartyAfterChallenge();
    gSaveBlock2Ptr->frontier.challengeStatus = gSpecialVar_0x8005;
    VarSet(VAR_TEMP_CHALLENGE_STATUS, 0);
    gSaveBlock2Ptr->frontier.challengePaused = TRUE;
    SaveGameFrontier();
}

static void FactoryDummy1(void)
{

}

static void FactoryDummy2(void)
{

}

static void SelectInitialRentalMons(void)
{
    ZeroPlayerPartyMons();
    DoBattleFactorySelectScreen();
}

static void SwapRentalMons(void)
{
    DoBattleFactorySwapScreen();
}

static void SetPerformedRentalSwap(void)
{
    sPerformedRentalSwap = TRUE;
}

static bool8 ADMG_TryGetFactoryMonRange(enum FrontierLevelMode level, u8 challenge,
    bool8 better, const struct TrainerMon *mons, u16 *first, u16 *last)
{
    u32 index = (level == FRONTIER_LVL_50 ? 0 : 8) + (challenge < 7 ? challenge : 7);
    if (challenge < 7 && better) ++index;
    u32 count;
    if (!ADMG_TryGetFacilityMonTableCount(mons, &count)) return FALSE;
    if (index >= ARRAY_COUNT(sInitialRentalMonRanges)
     || sInitialRentalMonRanges[index][0] > sInitialRentalMonRanges[index][1]
     || sInitialRentalMonRanges[index][1] >= count)
    { ADMG_StateAccessRejected(index, __func__); return FALSE; }
    *first = sInitialRentalMonRanges[index][0]; *last = sInitialRentalMonRanges[index][1];
    return TRUE;
}

static void GenerateOpponentMons(void)
{
    int i, j, k;
    enum Species species[FRONTIER_PARTY_SIZE];
    enum Item heldItems[FRONTIER_PARTY_SIZE];
    int firstMonId = 0;
    u16 trainerId = 0;
    struct SaveBlock2 *save;
    enum FrontierLevelMode lvlMode;
    u32 battleMode, monCount;
    const struct TrainerMon *mons = gFacilityTrainerMons;
    if (!ADMG_TryGetFactoryContext(&save, &battleMode, &lvlMode)
     || !ADMG_TryGetFacilityMonTableCount(mons, &monCount)) return;
    u32 historyCount = save->frontier.curChallengeBattleNum;
    if (historyCount > ARRAY_COUNT(save->frontier.trainerIds))
    { ADMG_StateAccessRejected(historyCount, __func__); return; }
    enum TrainerId history[ARRAY_COUNT(save->frontier.trainerIds)];
    u16 selected[FRONTIER_PARTY_SIZE]; // facility Pokémon
    enum Species rentals[ARRAY_COUNT(save->frontier.rentalMons)];
    for (u32 slot = 0; slot < historyCount; ++slot) history[slot] = save->frontier.trainerIds[slot];
    for (u32 slot = 0; slot < ARRAY_COUNT(rentals); ++slot)
    {
        const struct TrainerMon *row;
        if (!ADMG_TryGetFacilityMonRowFromTable(mons, monCount, save->frontier.rentalMons[slot].monId, &row)) return;
        rentals[slot] = row->species;
    }
    u32 winStreak = save->frontier.factoryWinStreaks[battleMode][lvlMode];
    u32 challengeNum = winStreak / FRONTIER_STAGES_PER_CHALLENGE;
    u16 rangeFirst, rangeLast;
    if (!ADMG_TryGetFrontierTrainerIdRange((u8)challengeNum, (u8)historyCount, &rangeFirst, &rangeLast)) return;
    bool8 available = FALSE;
    for (u32 id = rangeFirst; id <= rangeLast; ++id)
    {
        u32 slot;
        for (slot = 0; slot < historyCount; ++slot) if (history[slot] == id) break;
        if (slot == historyCount) { available = TRUE; break; }
    }
    if (!available) { ADMG_StateAccessRejected(historyCount, __func__); return; }
    if (!ADMG_TryGetFactoryMonRange(lvlMode, (u8)challengeNum, FALSE, mons, &rangeFirst, &rangeLast)) return;
    gFacilityTrainers = gBattleFrontierTrainers;

    do
    {
        // Choose a random trainer, ensuring no repeats in this challenge
        trainerId = GetRandomScaledFrontierTrainerId(challengeNum, historyCount);
        const struct BattleFrontierTrainer *trainer;
        if (!ADMG_TryGetFacilityTrainerRowFromTable(gBattleFrontierTrainers,
            FRONTIER_TRAINERS_COUNT, trainerId, &trainer)) return;
        for (i = 0; i < historyCount; i++)
        {
            if (history[i] == trainerId)
                break;
        }
    } while (i != historyCount);

    i = 0;
    while (i != FRONTIER_PARTY_SIZE)
    {
        u16 monId = GetFactoryMonId(lvlMode, challengeNum, FALSE);

        const struct TrainerMon *row;
        if (!ADMG_TryGetFacilityMonRowFromTable(mons, monCount, monId, &row)) return;
        // Unown (FRONTIER_MON_UNOWN) is forbidden on opponent Factory teams.
        if ((*row).species == SPECIES_UNOWN)
            continue;

        // Ensure none of the opponent's Pokémon are the same as the potential rental Pokémon for the player
        for (j = 0; j < (int)ARRAY_COUNT(rentals); j++)
        {
            if ((*row).species == rentals[j])
                break;
        }
        if (j != (int)ARRAY_COUNT(rentals))
            continue;

        // "High tier" Pokémon are only allowed on open level mode
        if (lvlMode == FRONTIER_LVL_50 && monId > FRONTIER_MONS_HIGH_TIER)
            continue;

        // Ensure this species hasn't already been chosen for the opponent
        for (k = firstMonId; k < firstMonId + i; k++)
        {
            if (species[k] == (*row).species)
                break;
        }
        if (k != firstMonId + i)
            continue;

        // Ensure held items don't repeat on the opponent's team
        for (k = firstMonId; k < firstMonId + i; k++)
        {
            if (heldItems[k] != ITEM_NONE && heldItems[k] == (*row).heldItem)
                break;
        }
        if (k != firstMonId + i)
            continue;

        // Successful selection
        species[i] = (*row).species;
        heldItems[i] = (*row).heldItem;
        selected[i] = monId;
        i++;
    }
    if (gSaveBlock2Ptr != save || save->frontier.lvlMode != lvlMode || save->frontier.curChallengeBattleNum != historyCount)
    { ADMG_StateAccessRejected(trainerId, __func__); return; }
    TRAINER_BATTLE_PARAM.opponentA = trainerId;
    if (historyCount < FRONTIER_STAGES_PER_CHALLENGE - 1) save->frontier.trainerIds[historyCount] = trainerId;
    for (u32 slot = 0; slot < FRONTIER_PARTY_SIZE; ++slot) gFrontierTempParty[slot] = selected[slot];
}

static void SetOpponentGfxVar(void)
{
    SetBattleFacilityTrainerGfxId(TRAINER_BATTLE_PARAM.opponentA, 0);
}

static bool8 ADMG_TryCaptureFactoryMonRows(const struct TrainerMon *mons,
    const u16 *ids, u32 count, const struct TrainerMon **rows)
{
    if (ids == NULL || rows == NULL || count > PARTY_SIZE)
    { ADMG_StateAccessRejected(count, __func__); return FALSE; }
    u32 monCount;
    if (!ADMG_TryGetFacilityMonTableCount(mons, &monCount)) return FALSE;
    for (u32 i = 0; i < count; ++i)
        if (!ADMG_TryGetFacilityMonRowFromTable(mons, monCount, ids[i], &rows[i])) return FALSE;
    return TRUE;
}

static bool8 ADMG_TryGetFactoryRentalBacking(struct SaveBlock2 **save,
    const struct TrainerMon **mons)
{
    if (gSaveBlock2Ptr == NULL)
    { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    *save = gSaveBlock2Ptr;
    *mons = (*save)->frontier.lvlMode == FRONTIER_LVL_TENT
        ? gSlateportBattleTentMons : gBattleFrontierMons;
    return TRUE;
}

static void SetRentalsToOpponentParty(void)
{
    struct SaveBlock2 *save;
    const struct TrainerMon *mons, *rows[FRONTIER_PARTY_SIZE];
    u16 ids[FRONTIER_PARTY_SIZE];
    if (!ADMG_TryGetFactoryRentalBacking(&save, &mons)) return;
    for (u32 i = 0; i < FRONTIER_PARTY_SIZE; ++i) ids[i] = gFrontierTempParty[i];
    if (!ADMG_TryCaptureFactoryMonRows(mons, ids, FRONTIER_PARTY_SIZE, rows)) return;
    gFacilityTrainerMons = mons;
    u64 generation = ADMG_BeginStateAccessCheck();
    struct RentalMon selected[FRONTIER_PARTY_SIZE];
    for (u32 i = 0; i < FRONTIER_PARTY_SIZE; ++i)
    {
        selected[i] = save->frontier.rentalMons[i + FRONTIER_PARTY_SIZE];
        selected[i].monId = ids[i];
        selected[i].ivs = GetBoxMonData(&gParties[B_TRAINER_OPPONENT_A][i].box, MON_DATA_ATK_IV);
        selected[i].personality = GetMonData(&gParties[B_TRAINER_OPPONENT_A][i], MON_DATA_PERSONALITY);
        selected[i].abilityNum = GetBoxMonData(&gParties[B_TRAINER_OPPONENT_A][i].box, MON_DATA_ABILITY_NUM);
    }
    if (!ADMG_EndStateAccessCheck(generation)) return;
    if (gSaveBlock2Ptr != save)
    { ADMG_StateAccessRejected(0, __func__); return; }
    for (u32 i = 0; i < FRONTIER_PARTY_SIZE; ++i)
    {
        save->frontier.rentalMons[i + FRONTIER_PARTY_SIZE] = selected[i];
        SetMonData(&gParties[B_TRAINER_OPPONENT_A][i], MON_DATA_HELD_ITEM, &rows[i]->heldItem);
    }
}

static void SetPlayerAndOpponentParties(void)
{
    struct SaveBlock2 *save;
    const struct TrainerMon *mons;
    if (!ADMG_TryGetFactoryRentalBacking(&save, &mons)) return;
    u32 mode = gSpecialVar_0x8005;
    bool8 player = mode < 2, opponent = mode == 0 || mode == 2;
    struct RentalMon selected[FRONTIER_PARTY_SIZE * 2];
    const struct TrainerMon *rows[FRONTIER_PARTY_SIZE * 2];
    for (u32 target = 0; target < 2; ++target)
    {
        if ((target == 0 && !player) || (target == 1 && !opponent)) continue;
        u16 ids[FRONTIER_PARTY_SIZE];
        for (u32 i = 0; i < FRONTIER_PARTY_SIZE; ++i)
        {
            selected[target * FRONTIER_PARTY_SIZE + i] = save->frontier.rentalMons[target * FRONTIER_PARTY_SIZE + i];
            ids[i] = selected[target * FRONTIER_PARTY_SIZE + i].monId;
        }
        if (!ADMG_TryCaptureFactoryMonRows(mons, ids, FRONTIER_PARTY_SIZE, &rows[target * FRONTIER_PARTY_SIZE])) return;
    }
    u8 monLevel = save->frontier.lvlMode == FRONTIER_LVL_TENT ? TENT_MIN_LEVEL
        : save->frontier.lvlMode == FRONTIER_LVL_50 ? FRONTIER_MAX_LEVEL_50 : FRONTIER_MAX_LEVEL_OPEN;
    u32 ot = T1_READ_32(save->playerTrainerId);
    gFacilityTrainerMons = mons;
    if (player) ZeroPlayerPartyMons();
    for (u32 target = 0; target < 2; ++target)
    {
        if ((target == 0 && !player) || (target == 1 && !opponent)) continue;
        for (u32 i = 0; i < FRONTIER_PARTY_SIZE; ++i)
        {
            u32 index = target * FRONTIER_PARTY_SIZE + i;
            CreateFacilityMon(rows[index], monLevel, selected[index].ivs, ot,
                FLAG_FRONTIER_MON_FACTORY, &gParties[target == 0 ? B_TRAINER_PLAYER : B_TRAINER_OPPONENT_A][i]);
        }
    }
}

static void GenerateInitialRentalMons(void)
{
    int i, j;
    u8 firstMonId;
    u32 battleMode;
    enum FrontierLevelMode lvlMode;
    u8 challengeNum;
    enum FrontierLevelMode factoryLvlMode;
    u8 factoryBattleMode;
    u8 rentalRank;
    u16 monId;
    enum Species currSpecies;
    enum Species species[PARTY_SIZE];
    u16 monIds[PARTY_SIZE];
    enum Item heldItems[PARTY_SIZE];

    struct SaveBlock2 *save;
    const struct TrainerMon *mons = gBattleFrontierMons;
    u32 monCount;
    u16 selected[PARTY_SIZE];
    if (!ADMG_TryGetFactoryContext(&save, &battleMode, &lvlMode)
     || !ADMG_TryGetFacilityMonTableCount(mons, &monCount)) return;
    u64 generation = ADMG_BeginStateAccessCheck();
    gFacilityTrainers = gBattleFrontierTrainers;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        species[i] = SPECIES_NONE;
        monIds[i] = 0;
        heldItems[i] = ITEM_NONE;
    }

    challengeNum = save->frontier.factoryWinStreaks[battleMode][lvlMode] / FRONTIER_STAGES_PER_CHALLENGE;
    u32 secondMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
    if (!ADMG_EndStateAccessCheck(generation)) return;
    if (secondMode != battleMode)
    { ADMG_StateAccessRejected(secondMode, __func__); return; }
    if (secondMode == FRONTIER_MODE_DOUBLES)
        factoryBattleMode = FRONTIER_MODE_DOUBLES;
    else
        factoryBattleMode = FRONTIER_MODE_SINGLES;

    gFacilityTrainerMons = gBattleFrontierMons;
    if (save->frontier.lvlMode != FRONTIER_LVL_50)
    {
        factoryLvlMode = FRONTIER_LVL_OPEN;
        firstMonId = 0;
    }
    else
    {
        factoryLvlMode = FRONTIER_LVL_50;
        firstMonId = 0;
    }
    rentalRank = GetNumPastRentalsRank(factoryBattleMode, factoryLvlMode);

    if (!ADMG_EndStateAccessCheck(generation)) return;
    if (gSaveBlock2Ptr != save || save->frontier.lvlMode != lvlMode)
    { ADMG_StateAccessRejected(battleMode, __func__); return; }
    u16 first, last;
    if (!ADMG_TryGetFactoryMonRange(factoryLvlMode, challengeNum, FALSE, mons, &first, &last)
     || !ADMG_TryGetFactoryMonRange(factoryLvlMode, challengeNum, TRUE, mons, &first, &last)) return;
    currSpecies = SPECIES_NONE;
    i = 0;
    while (i != PARTY_SIZE)
    {
        if (i < rentalRank) // The more times the player has rented, the more initial rentals are generated from a better set of Pokémon
            monId = GetFactoryMonId(factoryLvlMode, challengeNum, TRUE);
        else
            monId = GetFactoryMonId(factoryLvlMode, challengeNum, FALSE);

        const struct TrainerMon *row;
        if (!ADMG_TryGetFacilityMonRowFromTable(mons, monCount, monId, &row)) return;
        if ((*row).species == SPECIES_UNOWN)
            continue;

        // Cannot have two Pokémon of the same species.
        for (j = firstMonId; j < firstMonId + i; j++)
        {
            u16 existingMonId = monIds[j];
            if (existingMonId == monId)
                break;
            if (species[j] == (*row).species)
            {
                if (currSpecies == SPECIES_NONE)
                    currSpecies = (*row).species;
                else
                    break;
            }
        }
        if (j != firstMonId + i)
            continue;

        // Cannot have two same held items.
        for (j = firstMonId; j < firstMonId + i; j++)
        {
            if (heldItems[j] != ITEM_NONE && heldItems[j] == (*row).heldItem)
            {
                if ((*row).species == currSpecies)
                    currSpecies = SPECIES_NONE;
                break;
            }
        }
        if (j != firstMonId + i)
            continue;

        selected[i] = monId;
        species[i] = (*row).species;
        heldItems[i] = (*row).heldItem;
        monIds[i] = monId;
        i++;
    }
    if (gSaveBlock2Ptr != save || save->frontier.lvlMode != lvlMode)
    { ADMG_StateAccessRejected(battleMode, __func__); return; }
    for (u32 slot = 0; slot < PARTY_SIZE; ++slot) save->frontier.rentalMons[slot].monId = selected[slot];
}

// Determines if the upcoming opponent has a single most-common
// type in its party. If there are two different types that are
// tied, then the opponent is deemed to have no preferred type,
// and NUMBER_OF_MON_TYPES is the result.
static void GetOpponentMostCommonMonType(void)
{
    u8 i;
    u8 typeCounts[RUNTIME_TYPES_COUNT];
    u8 mostCommonTypes[2];

    const struct TrainerMon *rows[FRONTIER_PARTY_SIZE];
    if (!ADMG_TryCaptureFactoryMonRows(gBattleFrontierMons, gFrontierTempParty, FRONTIER_PARTY_SIZE, rows)) return;
    gFacilityTrainerMons = gBattleFrontierMons;

    // Count the number of times each type occurs in the opponent's party.
    for (i = 0; i < ARRAY_COUNT(typeCounts); i++)
        typeCounts[i] = 0;
    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        enum Species species = (*rows[i]).species;
        u32 type = GetSpeciesType(species, 0);
        if (type >= ARRAY_COUNT(typeCounts))
        { ADMG_StateAccessRejected(type, __func__); return; }
        typeCounts[type]++;
        if (GetSpeciesType(species, 0) != GetSpeciesType(species, 1))
        {
            u32 type = GetSpeciesType(species, 1);
            if (type >= ARRAY_COUNT(typeCounts))
            { ADMG_StateAccessRejected(type, __func__); return; }
            typeCounts[type]++;
        }
    }

    // Determine which are the two most-common types.
    // The second most-common type is only updated if
    // its count is equal to the most-common type.
    mostCommonTypes[0] = 0;
    mostCommonTypes[1] = 0;
    for (i = 1; i < RUNTIME_TYPES_COUNT; i = NEXT_TYPE_ID(i))
    {
        if (typeCounts[mostCommonTypes[0]] < typeCounts[i])
            mostCommonTypes[0] = i;
        else if (typeCounts[mostCommonTypes[0]] == typeCounts[i])
            mostCommonTypes[1] = i;
    }

    if (typeCounts[mostCommonTypes[0]] != 0)
    {
        // The most-common type must be strictly greater than
        // the second-most-common type, or the top two must be
        // the same type.
        if (typeCounts[mostCommonTypes[0]] > typeCounts[mostCommonTypes[1]])
            gSpecialVar_Result = mostCommonTypes[0];
        else if (mostCommonTypes[0] == mostCommonTypes[1])
            gSpecialVar_Result = mostCommonTypes[0];
        else
            gSpecialVar_Result = NUMBER_OF_MON_TYPES;
    }
    else
    {
        gSpecialVar_Result = NUMBER_OF_MON_TYPES;
    }
}

static void GetOpponentBattleStyle(void)
{
    u8 i, j, count;
    u8 stylePoints[FACTORY_NUM_STYLES];

    count = 0;
    const struct TrainerMon *rows[FRONTIER_PARTY_SIZE];
    if (!ADMG_TryCaptureFactoryMonRows(gBattleFrontierMons, gFrontierTempParty, FRONTIER_PARTY_SIZE, rows)) return;
    gFacilityTrainerMons = gBattleFrontierMons;
    for (i = 0; i < FACTORY_NUM_STYLES; i++)
        stylePoints[i] = 0;

    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {

        for (j = 0; j < MAX_MON_MOVES; j++)
        {
            u32 battleStyle = GetMoveBattleStyle((*rows[i]).moves[j]);
            if (battleStyle >= ARRAY_COUNT(stylePoints))
            { ADMG_StateAccessRejected(battleStyle, __func__); return; }
            stylePoints[battleStyle]++;
        }
    }

    gSpecialVar_Result = FACTORY_STYLE_NONE;
    for (i = 1; i < FACTORY_NUM_STYLES; i++)
    {
        if (stylePoints[i] >= sRequiredMoveCounts[i - 1])
        {
            gSpecialVar_Result = i;
            count++;
        }
    }

    // Has no singular style
    if (count > 2)
        gSpecialVar_Result = FACTORY_NUM_STYLES;
}

static enum FactoryStyle GetMoveBattleStyle(enum Move move)
{
    enum FactoryStyle style = gBattleMoveEffects[GetMoveEffect(move)].battleFactoryStyle;

    if (style != FACTORY_STYLE_NONE)
        return style;

    // Conditional effects
    switch (GetMoveEffect(move))
    {
    case EFFECT_TWO_TURNS_ATTACK:
        // Potential to miss a two-turn move
        if (GetMoveAccuracy(move) < 100 && GetMoveAccuracy(move) != 0)
            return FACTORY_STYLE_HIGH_RISK;
        break;
    case EFFECT_RECOIL:
        // Only higher recoil moves are considered risky
        if (GetMoveRecoil(move) >= 33)
            return FACTORY_STYLE_HIGH_RISK;
        break;
    default:
        break;
    }
    // Bad secondary effects for the user
    if (MoveHasAdditionalEffectSelf(move, MOVE_EFFECT_RECHARGE)
     || MoveHasAdditionalEffectSelf(move, MOVE_EFFECT_STAT_MINUS))
        return FACTORY_STYLE_HIGH_RISK;

    // Non-volatile effects
    if (GetMoveNonVolatileStatus(move) != MOVE_EFFECT_NONE)
        return FACTORY_STYLE_SLOW_STEADY;

    if (IsExplosionMove(move))
        return FACTORY_STYLE_HIGH_RISK;

    return FACTORY_STYLE_NONE;
}

bool8 InBattleFactory(void)
{
    return gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_FACTORY_PRE_BATTLE_ROOM
        || gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_FACTORY_BATTLE_ROOM;
}

static void RestorePlayerPartyHeldItems(void)
{
    struct SaveBlock2 *save;
    const struct TrainerMon *mons, *rows[FRONTIER_PARTY_SIZE];
    u16 ids[FRONTIER_PARTY_SIZE];
    if (!ADMG_TryGetFactoryRentalBacking(&save, &mons)) return;
    for (u32 i = 0; i < FRONTIER_PARTY_SIZE; ++i) ids[i] = save->frontier.rentalMons[i].monId;
    if (!ADMG_TryCaptureFactoryMonRows(mons, ids, FRONTIER_PARTY_SIZE, rows)) return;
    gFacilityTrainerMons = mons;
    for (u32 i = 0; i < FRONTIER_PARTY_SIZE; ++i)
        SetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_HELD_ITEM, &rows[i]->heldItem);
}

// Get the IV to use for the opponent's pokémon.
// The IVs get higher for each subsequent challenge and for
// the last trainer in each challenge. Noland is an exception
// to this, as he uses the IVs that would be used by the regular
// trainers 2 challenges ahead of the current one.
// Due to a mistake in FillFactoryFrontierTrainerParty, the
// challenge number used to determine the IVs for regular trainers
// is Battle Tower's instead of Battle Factory's.
u8 GetFactoryMonFixedIV(u8 challengeNum, bool8 isLastBattle)
{
    u8 ivSet;
    bool8 useHigherIV = isLastBattle ? TRUE : FALSE;

// The Factory has an out-of-bounds access when generating the rental draft for round 9 (challengeNum==8),
// or the "elevated" rentals from round 8 (challengeNum+1==8)
// This happens to land on a number higher than 31, which is interpreted as "random IVs"
#ifdef BUGFIX
    if (challengeNum >= ARRAY_COUNT(sFixedIVTable))
#else
    if (challengeNum > ARRAY_COUNT(sFixedIVTable))
#endif
        ivSet = ARRAY_COUNT(sFixedIVTable) - 1;
    else
        ivSet = challengeNum;

    return sFixedIVTable[ivSet][useHigherIV];
}

static const struct ADMG_FactoryPartyInput *sADMG_BorrowedFactoryParty;

ADMG_NATIVE_ENTRY bool8 ADMG_TryPrepareFactoryParty(u32 identity, bool8 tent, u32 firstMon,
    struct ADMG_FactoryPartyInput *input)
{
    if (input == NULL) { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    struct ADMG_FactoryPartyInput selected = {0};
    if (!ADMG_TryClassifyTrainerActor(identity, ADMG_ACTOR_FACILITY,
        tent ? ADMG_ACTOR_UNUSED : ADMG_ACTOR_OPPONENT, &selected.actor)) return FALSE;
    if (selected.actor.kind != ADMG_ACTOR_BRAIN
     && (firstMon > PARTY_SIZE || FRONTIER_PARTY_SIZE > PARTY_SIZE - firstMon))
    { ADMG_StateAccessRejected(firstMon, __func__); return FALSE; }
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    if (save == NULL) { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    selected.save = save; selected.mode = save->frontier.lvlMode; selected.tent = tent;
    selected.firstMon = selected.actor.kind == ADMG_ACTOR_BRAIN ? 0 : firstMon;
    selected.otId = T1_READ_32(save->playerTrainerId);
    u64 generation = ADMG_BeginStateAccessCheck();
    if (!tent && selected.actor.kind == ADMG_ACTOR_EREADER)
    {
#if FREE_BATTLE_TOWER_E_READER == FALSE
        memcpy(&selected.ereader, &save->frontier.ereaderTrainer, sizeof(selected.ereader));
#endif
    }
    else
    {
        selected.fixedIV = tent ? 0 : MAX_PER_STAT_IVS;
        if (!tent && (selected.actor.kind == ADMG_ACTOR_FRONTIER || selected.actor.kind == ADMG_ACTOR_BRAIN))
        {
            u32 battleMode;
            enum FrontierLevelMode mode;
            struct SaveBlock2 *context;
            if (selected.actor.kind == ADMG_ACTOR_BRAIN)
            {
                if (!ADMG_TryGetFactoryContext(&context, &battleMode, &mode)) return FALSE;
            }
            else
            {
#ifdef BUGFIX
                if (!ADMG_TryGetFactoryContext(&context, &battleMode, &mode)) return FALSE;
#else
                context = save; mode = selected.mode;
                battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
                if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
                if (battleMode >= ARRAY_COUNT(save->frontier.towerWinStreaks))
                { ADMG_StateAccessRejected(battleMode, __func__); return FALSE; }
#endif
            }
            if (context != save || mode != selected.mode)
            { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
            if (selected.actor.kind == ADMG_ACTOR_BRAIN)
            {
                selected.challenge = save->frontier.factoryWinStreaks[battleMode][mode] / FRONTIER_STAGES_PER_CHALLENGE;
                selected.fixedIV = GetFactoryMonFixedIV(selected.challenge + 2, FALSE);
                for (u32 i = 0; i < ARRAY_COUNT(selected.rentalIds); ++i)
                    selected.rentalIds[i] = save->frontier.rentalMons[i].monId;
            }
            else
            {
#ifdef BUGFIX
                u8 challenge = save->frontier.factoryWinStreaks[battleMode][mode] / FRONTIER_STAGES_PER_CHALLENGE;
#else
                u8 challenge = save->frontier.towerWinStreaks[battleMode][FRONTIER_LVL_50] / FRONTIER_STAGES_PER_CHALLENGE;
#endif
                selected.fixedIV = GetFactoryMonFixedIV(challenge,
                    save->frontier.curChallengeBattleNum >= FRONTIER_STAGES_PER_CHALLENGE - 1);
            }
        }
        if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
        selected.mons = tent ? gFacilityTrainerMons : gBattleFrontierMons;
        if (!tent && selected.mode == FRONTIER_LVL_TENT)
        {
            const struct BattleFrontierTrainer *table;
            u32 trainerCount;
            if (!ADMG_TrySelectFacilityTrainerTables(TRUE, &table, &selected.mons, &trainerCount)) return FALSE;
        }
        if (!ADMG_TryGetFacilityMonTableCount(selected.mons, &selected.monCount)) return FALSE;
        if (selected.actor.kind == ADMG_ACTOR_BRAIN)
        {
            u16 first, last;
            if (!ADMG_TryGetFactoryMonRange(selected.mode, selected.challenge, FALSE,
                selected.mons, &first, &last)) return FALSE;
        }
        else
        {
            u16 ids[FRONTIER_PARTY_SIZE];
            for (u32 i = 0; i < ARRAY_COUNT(ids); ++i) ids[i] = gFrontierTempParty[i];
            if (!ADMG_TryCaptureFactoryMonRows(selected.mons, ids, ARRAY_COUNT(ids), selected.rows)) return FALSE;
        }
        if (!tent)
        {
            selected.level = SetFacilityPtrsGetLevel();
            if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
            if (gSaveBlock2Ptr != save || save->frontier.lvlMode != selected.mode
             || gFacilityTrainerMons != selected.mons)
            { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
        }
        else selected.level = TENT_MIN_LEVEL;
    }
    if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (gSaveBlock2Ptr != save || save->frontier.lvlMode != selected.mode)
    { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    memcpy(input, &selected, sizeof(selected));
    return TRUE;
}

ADMG_NATIVE_ENTRY const struct ADMG_FactoryPartyInput *ADMG_BorrowFactoryParty(const struct ADMG_FactoryPartyInput *input)
{
    const struct ADMG_FactoryPartyInput *previous = sADMG_BorrowedFactoryParty;
    sADMG_BorrowedFactoryParty = input;
    return previous;
}

static void ADMG_FillAdmittedFactoryParty(const struct ADMG_FactoryPartyInput *input)
{
    if (input->actor.kind == ADMG_ACTOR_BRAIN)
    { FillFactoryBrainParty(); return; }
    if (input->actor.kind == ADMG_ACTOR_EREADER)
    {
#if FREE_BATTLE_TOWER_E_READER == FALSE
        for (u32 i = 0; i < FRONTIER_PARTY_SIZE; ++i)
        {
            struct BattleTowerPokemon mon = input->ereader.party[i];
            CreateBattleTowerMon(&gParties[B_TRAINER_OPPONENT_A][input->firstMon + i], &mon);
        }
#endif
        return;
    }
    for (u32 i = 0; i < FRONTIER_PARTY_SIZE; ++i)
        CreateFacilityMon(input->rows[i], input->level, input->fixedIV, input->otId,
            input->tent ? 0 : FLAG_FRONTIER_MON_FACTORY,
            &gParties[B_TRAINER_OPPONENT_A][input->firstMon + i]);
}

static void ADMG_FillFactoryPartyFromArgs(enum TrainerId identity, bool8 tent, u8 firstMon)
{
    struct ADMG_FactoryPartyInput local;
    const struct ADMG_FactoryPartyInput *input = sADMG_BorrowedFactoryParty;
    if (input == NULL || input->actor.identifier != identity || input->tent != tent
     || (input->actor.kind != ADMG_ACTOR_BRAIN && input->firstMon != firstMon))
    {
        if (!ADMG_TryPrepareFactoryParty(identity, tent, firstMon, &local)) return;
        input = &local;
    }
    const struct ADMG_FactoryPartyInput *previous = ADMG_BorrowFactoryParty(input);
    ADMG_FillAdmittedFactoryParty(input);
    ADMG_BorrowFactoryParty(previous);
}

ADMG_NATIVE_ENTRY void FillFactoryBrainParty(void)
{
    int i, j, k;
    enum Species species[FRONTIER_PARTY_SIZE];
    enum Item heldItems[FRONTIER_PARTY_SIZE];
    int monLevel;
    u8 fixedIV;
    u32 otId;

    struct ADMG_FactoryPartyInput local;
    const struct ADMG_FactoryPartyInput *input = sADMG_BorrowedFactoryParty;
    if (input == NULL || input->tent || input->actor.kind != ADMG_ACTOR_BRAIN)
    {
        if (!ADMG_TryPrepareFactoryParty(TRAINER_FRONTIER_BRAIN, FALSE, 0, &local)) return;
        input = &local;
    }
    enum FrontierLevelMode lvlMode = input->mode;
    u8 challengeNum = input->challenge;
    const u16 *rentalIds = input->rentalIds;
    const struct TrainerMon *mons = input->mons;
    u32 monCount = input->monCount;
    fixedIV = input->fixedIV;
    monLevel = input->level;
    i = 0;
    otId = input->otId;

    while (i != FRONTIER_PARTY_SIZE)
    {
        u16 monId = GetFactoryMonId(lvlMode, challengeNum, FALSE);

        const struct TrainerMon *row;
        if (!ADMG_TryGetFacilityMonRowFromTable(mons, monCount, monId, &row)) return;
        if ((*row).species == SPECIES_UNOWN)
            continue;
        if (monLevel == FRONTIER_MAX_LEVEL_50 && monId > FRONTIER_MONS_HIGH_TIER)
            continue;

        for (j = 0; j < (int)PARTY_SIZE; j++)
        {
            if (monId == rentalIds[j])
                break;
        }
        if (j != (int)PARTY_SIZE)
            continue;

        for (k = 0; k < i; k++)
        {
            if (species[k] == (*row).species)
                break;
        }
        if (k != i)
            continue;

        for (k = 0; k < i; k++)
        {
            if (heldItems[k] != ITEM_NONE && heldItems[k] == (*row).heldItem)
                break;
        }
        if (k != i)
            continue;

        species[i] = (*row).species;
        heldItems[i] = (*row).heldItem;
        CreateFacilityMon(&(*row),
                monLevel, fixedIV, otId, FLAG_FRONTIER_MON_FACTORY,
                &gParties[B_TRAINER_OPPONENT_A][i]);
        i++;
    }
}

ADMG_NATIVE_ENTRY u16 GetFactoryMonId(enum FrontierLevelMode lvlMode, u8 challengeNum, bool8 useBetterRange)
{
    u16 first, last;
    if (!ADMG_TryGetFactoryMonRange(lvlMode, challengeNum, useBetterRange, gBattleFrontierMons, &first, &last)) return 0xFFFF;
    return first + Random() % ((u32)last - first + 1);
}

u8 GetNumPastRentalsRank(u8 battleMode, enum FrontierLevelMode lvlMode)
{
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    if (save == NULL || (u32)battleMode >= ARRAY_COUNT(save->frontier.factoryRentsCount)
     || (u32)lvlMode >= ARRAY_COUNT(save->frontier.factoryRentsCount[0]))
    { ADMG_StateAccessRejected(battleMode, __func__); return 0; }
    u8 ret;
    u8 rents = save->frontier.factoryRentsCount[battleMode][lvlMode];

    if (rents < 15)
        ret = 0;
    else if (rents < 22)
        ret = 1;
    else if (rents < 29)
        ret = 2;
    else if (rents < 36)
        ret = 3;
    else if (rents < 43)
        ret = 4;
    else
        ret = 5;

    return ret;
}

u64 GetAiScriptsInBattleFactory(void)
{
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    if (save == NULL) { ADMG_StateAccessRejected(0, __func__); return 0; }
    enum FrontierLevelMode lvlMode = save->frontier.lvlMode;

    if (lvlMode == FRONTIER_LVL_TENT)
    {
        return 0;
    }
    else
    {
        u32 battleMode;
        if (!ADMG_TryGetFactoryContext(&save, &battleMode, &lvlMode)) return 0;
        int challengeNum = save->frontier.factoryWinStreaks[battleMode][lvlMode] / FRONTIER_STAGES_PER_CHALLENGE;

        if (TRAINER_BATTLE_PARAM.opponentA == TRAINER_FRONTIER_BRAIN)
            return AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_TRY_TO_FAINT | AI_FLAG_CHECK_VIABILITY;
        else if (challengeNum < 2)
            return 0;
        else if (challengeNum < 4)
            return AI_FLAG_CHECK_BAD_MOVE;
        else
            return AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_TRY_TO_FAINT | AI_FLAG_CHECK_VIABILITY;
    }
}

ADMG_NATIVE_ENTRY void FillFactoryFrontierTrainerParty(enum TrainerId trainerId, u8 firstMonId)
{
    ADMG_FillFactoryPartyFromArgs(trainerId, FALSE, firstMonId);
}

ADMG_NATIVE_ENTRY void FillFactoryTentTrainerParty(enum TrainerId trainerId, u8 firstMonId)
{
    ADMG_FillFactoryPartyFromArgs(trainerId, TRUE, firstMonId);
}

ADMG_NATIVE_ENTRY void FillFactoryTrainerParty(void)
{
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    if (save == NULL) { ADMG_StateAccessRejected(0, __func__); return; }
    enum TrainerId identity = TRAINER_BATTLE_PARAM.opponentA;
    bool8 tent = save->frontier.lvlMode == FRONTIER_LVL_TENT;
    struct ADMG_FactoryPartyInput admitted;
    const struct ADMG_FactoryPartyInput *input = sADMG_BorrowedFactoryParty;
    if (input == NULL || input->actor.identifier != identity || input->tent != tent || input->firstMon != 0)
    {
        if (!ADMG_TryPrepareFactoryParty(identity, tent, 0, &admitted)) return;
        input = &admitted;
    }
    if (input->save != save || input->mode != save->frontier.lvlMode)
    { ADMG_StateAccessRejected(identity, __func__); return; }
    ZeroEnemyPartyMons();
    const struct ADMG_FactoryPartyInput *previous = ADMG_BorrowFactoryParty(input);
    if (tent) FillFactoryTentTrainerParty(identity, 0);
    else FillFactoryFrontierTrainerParty(identity, 0);
    ADMG_BorrowFactoryParty(previous);
}
