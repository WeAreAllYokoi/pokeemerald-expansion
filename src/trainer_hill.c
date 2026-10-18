#include "global.h"
#include "admg_state.h"
#include "malloc.h"
#include "battle.h"
#include "battle_tower.h"
#include "battle_setup.h"
#include "ereader_helpers.h"
#include "event_data.h"
#include "event_scripts.h"
#include "frontier_util.h"
#include "fieldmap.h"
#include "field_message_box.h"
#include "international_string_util.h"
#include "item.h"
#include "main.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "pokemon.h"
#include "script.h"
#include "string_util.h"
#include "strings.h"
#include "text.h"
#include "trainer_hill.h"
#include "window.h"
#include "util.h"
#include "constants/battle_ai.h"
#include "constants/event_object_movement.h"
#include "constants/event_objects.h"
#include "constants/items.h"
#include "constants/layouts.h"
#include "constants/moves.h"
#include "constants/trainers.h"
#include "constants/trainer_hill.h"
#include "constants/trainer_types.h"

#define HILL_MAX_TIME 215999 // 60 * 60 * 60 - 1

struct FloorTrainers
{
    u8 name[HILL_TRAINERS_PER_FLOOR][TRAINER_NAME_LENGTH + 1];
    u8 facilityClass[HILL_TRAINERS_PER_FLOOR];
};

static EWRAM_DATA struct {
    u8 floorId;
    struct TrainerHillChallenge challenge;
    struct TrainerHillFloor floors[NUM_TRAINER_HILL_FLOORS];
} *sHillData = NULL;

static EWRAM_DATA struct FloorTrainers *sFloorTrainers = NULL;
EWRAM_DATA u32 *gTrainerHillVBlankCounter = NULL;

// This file's functions.
static void TrainerHillStartChallenge(void);
static void GetOwnerState(void);
static void GiveChallengePrize(void);
static void CheckFinalTime(void);
static void TrainerHillResumeTimer(void);
static void TrainerHillSetPlayerLost(void);
static void TrainerHillGetChallengeStatus(void);
static void BufferChallengeTime(void);
static void GetAllFloorsUsed(void);
static void GetInEReaderMode(void);
static void IsTrainerHillChallengeActive(void);
static void ShowTrainerHillPostBattleText(void);
static void SetAllTrainerFlags(void);
static void GetGameSaved(void);
static void SetGameSaved(void);
static void ClearGameSaved(void);
static void GetChallengeWon(void);
static void TrainerHillSetMode(void);
static void SetUpDataStruct(void);
static void FreeDataStruct(void);
static void TrainerHillDummy(void);
#if FREE_TRAINER_HILL == FALSE
static void SetTimerValue(u32 *dst, u32 val);
static u32 GetTimerValue(u32 *src);
#endif //FREE_TRAINER_HILL
static void SetTrainerHillMonLevel(struct Pokemon *mon, u8 level);
#if FREE_TRAINER_HILL == FALSE
static enum Item GetPrizeItemId(void);
#endif //FREE_TRAINER_HILL

// const data
#include "data/battle_frontier/trainer_hill.h"

struct
{
    enum TrainerClassID trainerClass;
    u8 musicId;
} static const sTrainerClassesAndMusic[] =
{
    {TRAINER_CLASS_TEAM_AQUA, TRAINER_ENCOUNTER_MUSIC_AQUA},
    {TRAINER_CLASS_AQUA_ADMIN, TRAINER_ENCOUNTER_MUSIC_AQUA},
    {TRAINER_CLASS_AQUA_LEADER, TRAINER_ENCOUNTER_MUSIC_AQUA},
    {TRAINER_CLASS_AROMA_LADY, TRAINER_ENCOUNTER_MUSIC_FEMALE},
    {TRAINER_CLASS_BATTLE_GIRL, TRAINER_ENCOUNTER_MUSIC_INTENSE},
    {TRAINER_CLASS_SWIMMER_F, TRAINER_ENCOUNTER_MUSIC_FEMALE},
    {TRAINER_CLASS_POKEFAN, TRAINER_ENCOUNTER_MUSIC_TWINS},
    {TRAINER_CLASS_DRAGON_TAMER, TRAINER_ENCOUNTER_MUSIC_INTENSE},
    {TRAINER_CLASS_COOLTRAINER, TRAINER_ENCOUNTER_MUSIC_COOL},
    {TRAINER_CLASS_GUITARIST, TRAINER_ENCOUNTER_MUSIC_INTENSE},
    {TRAINER_CLASS_SAILOR, TRAINER_ENCOUNTER_MUSIC_MALE},
    {TRAINER_CLASS_TWINS, TRAINER_ENCOUNTER_MUSIC_TWINS},
    {TRAINER_CLASS_INTERVIEWER, TRAINER_ENCOUNTER_MUSIC_INTERVIEWER},
    {TRAINER_CLASS_RUIN_MANIAC, TRAINER_ENCOUNTER_MUSIC_HIKER},
    {TRAINER_CLASS_GENTLEMAN, TRAINER_ENCOUNTER_MUSIC_RICH},
    {TRAINER_CLASS_SWIMMER_M, TRAINER_ENCOUNTER_MUSIC_FEMALE},
    {TRAINER_CLASS_POKEMANIAC, TRAINER_ENCOUNTER_MUSIC_SUSPICIOUS},
    {TRAINER_CLASS_BLACK_BELT, TRAINER_ENCOUNTER_MUSIC_INTENSE},
    {TRAINER_CLASS_OLD_COUPLE, TRAINER_ENCOUNTER_MUSIC_INTENSE},
    {TRAINER_CLASS_BUG_MANIAC, TRAINER_ENCOUNTER_MUSIC_SUSPICIOUS},
    {TRAINER_CLASS_CAMPER, TRAINER_ENCOUNTER_MUSIC_MALE},
    {TRAINER_CLASS_KINDLER, TRAINER_ENCOUNTER_MUSIC_HIKER},
    {TRAINER_CLASS_TEAM_MAGMA, TRAINER_ENCOUNTER_MUSIC_MAGMA},
    {TRAINER_CLASS_MAGMA_ADMIN, TRAINER_ENCOUNTER_MUSIC_MAGMA},
    {TRAINER_CLASS_MAGMA_LEADER, TRAINER_ENCOUNTER_MUSIC_MAGMA},
    {TRAINER_CLASS_LASS, TRAINER_ENCOUNTER_MUSIC_FEMALE},
    {TRAINER_CLASS_BUG_CATCHER, TRAINER_ENCOUNTER_MUSIC_MALE},
    {TRAINER_CLASS_NINJA_BOY, TRAINER_ENCOUNTER_MUSIC_SUSPICIOUS},
    {TRAINER_CLASS_RICH_BOY, TRAINER_ENCOUNTER_MUSIC_RICH},
    {TRAINER_CLASS_HEX_MANIAC, TRAINER_ENCOUNTER_MUSIC_SUSPICIOUS},
    {TRAINER_CLASS_BEAUTY, TRAINER_ENCOUNTER_MUSIC_FEMALE},
    {TRAINER_CLASS_LADY, TRAINER_ENCOUNTER_MUSIC_FEMALE},
    {TRAINER_CLASS_PARASOL_LADY, TRAINER_ENCOUNTER_MUSIC_FEMALE},
    {TRAINER_CLASS_PICNICKER, TRAINER_ENCOUNTER_MUSIC_GIRL},
    {TRAINER_CLASS_PKMN_BREEDER, TRAINER_ENCOUNTER_MUSIC_FEMALE},
    {TRAINER_CLASS_COLLECTOR, TRAINER_ENCOUNTER_MUSIC_SUSPICIOUS},
    {TRAINER_CLASS_PKMN_RANGER, TRAINER_ENCOUNTER_MUSIC_COOL},
    {TRAINER_CLASS_RIVAL, TRAINER_ENCOUNTER_MUSIC_MALE},
    {TRAINER_CLASS_YOUNG_COUPLE, TRAINER_ENCOUNTER_MUSIC_GIRL},
    {TRAINER_CLASS_PSYCHIC, TRAINER_ENCOUNTER_MUSIC_INTENSE},
    {TRAINER_CLASS_SR_AND_JR, TRAINER_ENCOUNTER_MUSIC_TWINS},
    {TRAINER_CLASS_ELITE_FOUR, TRAINER_ENCOUNTER_MUSIC_FEMALE},
    {TRAINER_CLASS_YOUNGSTER, TRAINER_ENCOUNTER_MUSIC_MALE},
    {TRAINER_CLASS_EXPERT, TRAINER_ENCOUNTER_MUSIC_INTENSE},
    {TRAINER_CLASS_TRIATHLETE, TRAINER_ENCOUNTER_MUSIC_MALE},
    {TRAINER_CLASS_BIRD_KEEPER, TRAINER_ENCOUNTER_MUSIC_COOL},
    {TRAINER_CLASS_FISHERMAN, TRAINER_ENCOUNTER_MUSIC_HIKER},
    {TRAINER_CLASS_CHAMPION, TRAINER_ENCOUNTER_MUSIC_MALE},
    {TRAINER_CLASS_TUBER_M, TRAINER_ENCOUNTER_MUSIC_MALE},
    {TRAINER_CLASS_TUBER_F, TRAINER_ENCOUNTER_MUSIC_GIRL},
    {TRAINER_CLASS_SIS_AND_BRO, TRAINER_ENCOUNTER_MUSIC_SWIMMER},
    {TRAINER_CLASS_HIKER, TRAINER_ENCOUNTER_MUSIC_HIKER},
    {TRAINER_CLASS_LEADER, TRAINER_ENCOUNTER_MUSIC_FEMALE},
    {TRAINER_CLASS_SCHOOL_KID, TRAINER_ENCOUNTER_MUSIC_MALE},
};

static const enum Item sPrizeListRareCandy1[]  = {ITEM_RARE_CANDY,       ITEM_ETHER, ITEM_MAX_POTION, ITEM_REVIVE, ITEM_FLUFFY_TAIL, ITEM_GREAT_BALL};
static const enum Item sPrizeListLuxuryBall1[] = {ITEM_LUXURY_BALL,      ITEM_ETHER, ITEM_MAX_POTION, ITEM_REVIVE, ITEM_FLUFFY_TAIL, ITEM_GREAT_BALL};
static const enum Item sPrizeListMaxRevive1[]  = {ITEM_MAX_REVIVE,       ITEM_ETHER, ITEM_MAX_POTION, ITEM_REVIVE, ITEM_FLUFFY_TAIL, ITEM_GREAT_BALL};
static const enum Item sPrizeListMaxEther1[]   = {ITEM_MAX_ETHER,        ITEM_ETHER, ITEM_MAX_POTION, ITEM_REVIVE, ITEM_FLUFFY_TAIL, ITEM_GREAT_BALL};
static const enum Item sPrizeListElixir1[]     = {ITEM_ELIXIR,           ITEM_ETHER, ITEM_MAX_POTION, ITEM_REVIVE, ITEM_FLUFFY_TAIL, ITEM_GREAT_BALL};
static const enum Item sPrizeListRoar[]        = {ITEM_TM_ROAR,          ITEM_ETHER, ITEM_MAX_POTION, ITEM_REVIVE, ITEM_FLUFFY_TAIL, ITEM_GREAT_BALL};
static const enum Item sPrizeListSludgeBomb[]  = {ITEM_TM_SLUDGE_BOMB,   ITEM_ETHER, ITEM_MAX_POTION, ITEM_REVIVE, ITEM_FLUFFY_TAIL, ITEM_GREAT_BALL};
static const enum Item sPrizeListToxic[]       = {ITEM_TM_TOXIC,         ITEM_ETHER, ITEM_MAX_POTION, ITEM_REVIVE, ITEM_FLUFFY_TAIL, ITEM_GREAT_BALL};
static const enum Item sPrizeListSunnyDay[]    = {ITEM_TM_SUNNY_DAY,     ITEM_ETHER, ITEM_MAX_POTION, ITEM_REVIVE, ITEM_FLUFFY_TAIL, ITEM_GREAT_BALL};
static const enum Item sPrizeListEarthQuake[]  = {ITEM_TM_EARTHQUAKE,    ITEM_ETHER, ITEM_MAX_POTION, ITEM_REVIVE, ITEM_FLUFFY_TAIL, ITEM_GREAT_BALL};

static const enum Item sPrizeListRareCandy2[]  = {ITEM_RARE_CANDY,       ITEM_ETHER, ITEM_MAX_POTION, ITEM_REVIVE, ITEM_FLUFFY_TAIL, ITEM_GREAT_BALL};
static const enum Item sPrizeListLuxuryBall2[] = {ITEM_LUXURY_BALL,      ITEM_ETHER, ITEM_MAX_POTION, ITEM_REVIVE, ITEM_FLUFFY_TAIL, ITEM_GREAT_BALL};
static const enum Item sPrizeListMaxRevive2[]  = {ITEM_MAX_REVIVE,       ITEM_ETHER, ITEM_MAX_POTION, ITEM_REVIVE, ITEM_FLUFFY_TAIL, ITEM_GREAT_BALL};
static const enum Item sPrizeListMaxEther2[]   = {ITEM_MAX_ETHER,        ITEM_ETHER, ITEM_MAX_POTION, ITEM_REVIVE, ITEM_FLUFFY_TAIL, ITEM_GREAT_BALL};
static const enum Item sPrizeListElixir2[]     = {ITEM_ELIXIR,           ITEM_ETHER, ITEM_MAX_POTION, ITEM_REVIVE, ITEM_FLUFFY_TAIL, ITEM_GREAT_BALL};
static const enum Item sPrizeListBrickBreak[]  = {ITEM_TM_BRICK_BREAK,   ITEM_ETHER, ITEM_MAX_POTION, ITEM_REVIVE, ITEM_FLUFFY_TAIL, ITEM_GREAT_BALL};
static const enum Item sPrizeListTorment[]     = {ITEM_TM_TORMENT,       ITEM_ETHER, ITEM_MAX_POTION, ITEM_REVIVE, ITEM_FLUFFY_TAIL, ITEM_GREAT_BALL};
static const enum Item sPrizeListSkillSwap[]   = {ITEM_TM_SKILL_SWAP,    ITEM_ETHER, ITEM_MAX_POTION, ITEM_REVIVE, ITEM_FLUFFY_TAIL, ITEM_GREAT_BALL};
static const enum Item sPrizeListGigaDrain[]   = {ITEM_TM_GIGA_DRAIN,    ITEM_ETHER, ITEM_MAX_POTION, ITEM_REVIVE, ITEM_FLUFFY_TAIL, ITEM_GREAT_BALL};
static const enum Item sPrizeListAttract[]     = {ITEM_TM_ATTRACT,       ITEM_ETHER, ITEM_MAX_POTION, ITEM_REVIVE, ITEM_FLUFFY_TAIL, ITEM_GREAT_BALL};

static const enum Item *const sPrizeLists1[NUM_TRAINER_HILL_PRIZE_LISTS] =
{
    sPrizeListRareCandy1,
    sPrizeListLuxuryBall1,
    sPrizeListMaxRevive1,
    sPrizeListMaxEther1,
    sPrizeListElixir1,
    sPrizeListRoar,
    sPrizeListSludgeBomb,
    sPrizeListToxic,
    sPrizeListSunnyDay,
    sPrizeListEarthQuake
};

static const enum Item *const sPrizeLists2[NUM_TRAINER_HILL_PRIZE_LISTS] =
{
    sPrizeListRareCandy2,
    sPrizeListLuxuryBall2,
    sPrizeListMaxRevive2,
    sPrizeListMaxEther2,
    sPrizeListElixir2,
    sPrizeListBrickBreak,
    sPrizeListTorment,
    sPrizeListSkillSwap,
    sPrizeListGigaDrain,
    sPrizeListAttract
};

static const enum Item *const *const sPrizeListSets[] =
{
    sPrizeLists1,
    sPrizeLists2
};

static const u16 sEReader_Pal[] = INCGFX_U16("graphics/trainer_hill/ereader.pal", ".gbapal");
static const u8 sRecordWinColors[] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY};

static const struct TrainerHillChallenge *const sChallengeData[NUM_TRAINER_HILL_MODES] =
{
    [HILL_MODE_NORMAL]  = &sChallenge_Normal,
    [HILL_MODE_VARIETY] = &sChallenge_Variety,
    [HILL_MODE_UNIQUE]  = &sChallenge_Unique,
    [HILL_MODE_EXPERT]  = &sChallenge_Expert,
};

static const struct TrainerHillFloor *const sFloorData[NUM_TRAINER_HILL_MODES] =
{
    [HILL_MODE_NORMAL]  = &sFloors_Normal[0],
    [HILL_MODE_VARIETY] = &sFloors_Variety[0],
    [HILL_MODE_UNIQUE]  = &sFloors_Unique[0],
    [HILL_MODE_EXPERT]  = &sFloors_Expert[0],
};

// Unused.
static const u8 *const sFloorStrings[] =
{
    gText_TrainerHill1F,
    gText_TrainerHill2F,
    gText_TrainerHill3F,
    gText_TrainerHill4F,
};

static void (*const sHillFunctions[])(void) =
{
    [TRAINER_HILL_FUNC_START]                 = TrainerHillStartChallenge,
    [TRAINER_HILL_FUNC_GET_OWNER_STATE]       = GetOwnerState,
    [TRAINER_HILL_FUNC_GIVE_PRIZE]            = GiveChallengePrize,
    [TRAINER_HILL_FUNC_CHECK_FINAL_TIME]      = CheckFinalTime,
    [TRAINER_HILL_FUNC_RESUME_TIMER]          = TrainerHillResumeTimer,
    [TRAINER_HILL_FUNC_SET_LOST]              = TrainerHillSetPlayerLost,
    [TRAINER_HILL_FUNC_GET_CHALLENGE_STATUS]  = TrainerHillGetChallengeStatus,
    [TRAINER_HILL_FUNC_GET_CHALLENGE_TIME]    = BufferChallengeTime,
    [TRAINER_HILL_FUNC_GET_ALL_FLOORS_USED]   = GetAllFloorsUsed,
    [TRAINER_HILL_FUNC_GET_IN_EREADER_MODE]   = GetInEReaderMode,
    [TRAINER_HILL_FUNC_IN_CHALLENGE]          = IsTrainerHillChallengeActive,
    [TRAINER_HILL_FUNC_POST_BATTLE_TEXT]      = ShowTrainerHillPostBattleText,
    [TRAINER_HILL_FUNC_SET_ALL_TRAINER_FLAGS] = SetAllTrainerFlags,
    [TRAINER_HILL_FUNC_GET_GAME_SAVED]        = GetGameSaved,
    [TRAINER_HILL_FUNC_SET_GAME_SAVED]        = SetGameSaved,
    [TRAINER_HILL_FUNC_CLEAR_GAME_SAVED]      = ClearGameSaved,
    [TRAINER_HILL_FUNC_GET_WON]               = GetChallengeWon,
    [TRAINER_HILL_FUNC_SET_MODE]              = TrainerHillSetMode,
};

static const u8 *const sModeStrings[NUM_TRAINER_HILL_MODES] =
{
    [HILL_MODE_NORMAL]  = gText_NormalTagMatch,
    [HILL_MODE_VARIETY] = gText_VarietyTagMatch,
    [HILL_MODE_UNIQUE]  = gText_UniqueTagMatch,
    [HILL_MODE_EXPERT]  = gText_ExpertTagMatch,
};

static const struct ObjectEventTemplate sTrainerObjectEventTemplate =
{
    .graphicsId = OBJ_EVENT_GFX_RIVAL_BRENDAN_NORMAL,
    .elevation = ELEVATION_DEFAULT,
    .movementType = MOVEMENT_TYPE_LOOK_AROUND,
    .movementRangeX = 1,
    .movementRangeY = 1,
    .trainerType = TRAINER_TYPE_NORMAL,
};

static const u32 sNextFloorMapNum[NUM_TRAINER_HILL_FLOORS] =
{
    [TRAINER_HILL_1F - 1] = MAP_NUM(MAP_TRAINER_HILL_2F),
    [TRAINER_HILL_2F - 1] = MAP_NUM(MAP_TRAINER_HILL_3F),
    [TRAINER_HILL_3F - 1] = MAP_NUM(MAP_TRAINER_HILL_4F),
    [TRAINER_HILL_4F - 1] = MAP_NUM(MAP_TRAINER_HILL_ROOF)
};
static const u8 sTrainerPartySlots[HILL_TRAINERS_PER_FLOOR][PARTY_SIZE / 2] =
{
    {0, 1, 2},
    {3, 4, 5}
};

void CallTrainerHillFunction(void)
{
    SetUpDataStruct();
    sHillFunctions[gSpecialVar_0x8004]();
    FreeDataStruct();
}

void ResetTrainerHillResults(void)
{
#if FREE_TRAINER_HILL == FALSE
    s32 i;
#endif //FREE_TRAINER_HILL

    gSaveBlock2Ptr->frontier.savedGame = 0;
    gSaveBlock2Ptr->frontier.unk_EF9 = 0;
#if FREE_TRAINER_HILL == FALSE
    gSaveBlock1Ptr->trainerHill.bestTime = 0;
    for (i = 0; i < NUM_TRAINER_HILL_MODES; i++)
        SetTimerValue(&gSaveBlock1Ptr->trainerHillTimes[i], HILL_MAX_TIME);
#endif //FREE_TRAINER_HILL
}

static u8 GetFloorId(void)
{
    return gMapHeader.mapLayoutId - LAYOUT_TRAINER_HILL_1F;
}

/* Hill ordinals belong to this explicit service, not a content namespace. */
static bool8 ADMG_TryHillTrainerSlot(u32 identifier, u32 *slot)
{
    if (slot == NULL || identifier == 0 || identifier > HILL_TRAINERS_PER_FLOOR)
    { ADMG_StateAccessRejected(identifier, __func__); return FALSE; }
    *slot = identifier - 1;
    return TRUE;
}

static bool8 ADMG_TryHillTrainerData(void)
{
    u64 generation = ADMG_BeginStateAccessCheck();
    typeof(sHillData) previous = sHillData;
    struct SaveBlock1 *save = gSaveBlock1Ptr;
    u32 layout = gMapHeader.mapLayoutId;
#if FREE_TRAINER_HILL == FALSE
    u32 mode = save == NULL ? NUM_TRAINER_HILL_MODES : save->trainerHill.mode;
#endif
    if (sHillData == NULL)
    {
#if FREE_TRAINER_HILL == FALSE
        if (save == NULL || mode >= ARRAY_COUNT(sChallengeData)
            || mode >= ARRAY_COUNT(sFloorData)
            || layout < LAYOUT_TRAINER_HILL_1F || layout - LAYOUT_TRAINER_HILL_1F >= NUM_TRAINER_HILL_FLOORS)
        { ADMG_StateAccessRejected(layout, __func__); return FALSE; }
#else
        ADMG_StateAccessRejected(0, __func__); return FALSE;
#endif
    }
    SetUpDataStruct();
    if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (gSaveBlock1Ptr != save || gMapHeader.mapLayoutId != layout
        || (previous != NULL && sHillData != previous)
#if FREE_TRAINER_HILL == FALSE
        || (previous == NULL && save->trainerHill.mode != mode)
#endif
        )
    { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    if (sHillData == NULL || sHillData->floorId >= ARRAY_COUNT(sHillData->floors))
    { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    return TRUE;
}

static bool8 ADMG_TryHillTrainerFacilityClass(u32 identifier, u32 *facilityClass)
{
    u32 slot;
    if (facilityClass == NULL || !ADMG_TryHillTrainerSlot(identifier, &slot)) return FALSE;
    u64 generation = ADMG_BeginStateAccessCheck();
    if (!ADMG_TryHillTrainerData()) return FALSE;
    u32 value = sHillData->floors[sHillData->floorId].trainers[slot].facilityClass;
    FreeDataStruct();
    if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (value >= FACILITY_CLASSES_COUNT)
    { ADMG_StateAccessRejected(value, __func__); return FALSE; }
    *facilityClass = value;
    return TRUE;
}

enum TrainerClassID GetTrainerHillOpponentClass(enum TrainerId trainerId)
{
    u32 slot;
    if (!ADMG_TryHillTrainerSlot(trainerId, &slot)) return TRAINER_CLASS_PKMN_TRAINER_1;
    if (sFloorTrainers == NULL || sFloorTrainers->facilityClass[slot] >= FACILITY_CLASSES_COUNT)
    { ADMG_StateAccessRejected(trainerId, __func__); return TRAINER_CLASS_PKMN_TRAINER_1; }
    return gFacilityClassToTrainerClass[sFloorTrainers->facilityClass[slot]];
}

void GetTrainerHillTrainerName(u8 *dst, enum TrainerId trainerId)
{
    u32 slot;
    if (dst == NULL || !ADMG_TryHillTrainerSlot(trainerId, &slot))
    { if (dst == NULL) ADMG_StateAccessRejected(trainerId, __func__); return; }
    if (sFloorTrainers == NULL)
    { ADMG_StateAccessRejected(trainerId, __func__); return; }
    for (u32 i = 0; i < sizeof(sFloorTrainers->name[slot]); i++)
        dst[i] = sFloorTrainers->name[slot][i];
}

u8 GetTrainerHillTrainerFrontSpriteId(enum TrainerId trainerId)
{
    u32 facilityClass;
    if (!ADMG_TryHillTrainerFacilityClass(trainerId, &facilityClass)) return TRAINER_PIC_HIKER;
    return gFacilityClassToPicIndex[facilityClass];
}

void InitTrainerHillBattleStruct(void)
{
    struct SaveBlock1 *save = gSaveBlock1Ptr;
    if (save == NULL)
    { ADMG_StateAccessRejected(0, __func__); return; }
    u32 layout = gMapHeader.mapLayoutId;
    typeof(sHillData) previousData = sHillData;
    struct FloorTrainers *originalCache = sFloorTrainers;
    u64 generation = ADMG_BeginStateAccessCheck();
    if (!ADMG_TryHillTrainerData()) return;
    typeof(sHillData) data = sHillData;
    struct FloorTrainers staged;
    for (u32 i = 0; i < HILL_TRAINERS_PER_FLOOR; i++)
    {
        const struct TrainerHillTrainer *trainer = &data->floors[data->floorId].trainers[i];
        if (trainer->facilityClass >= FACILITY_CLASSES_COUNT)
        { ADMG_StateAccessRejected(trainer->facilityClass, __func__); goto ADMG_reject_data; }
        staged.facilityClass[i] = trainer->facilityClass;
        for (u32 j = 0; j < sizeof(staged.name[i]); j++) staged.name[i][j] = trainer->name[j];
    }
    struct FloorTrainers *created = AllocZeroed(sizeof(*created));
    if (created == NULL)
    { ADMG_StateAccessRejected(0, __func__); goto ADMG_reject_data; }
    if (!ADMG_EndStateAccessCheck(generation) || sFloorTrainers != originalCache || sHillData != data
        || gSaveBlock1Ptr != save || gMapHeader.mapLayoutId != layout)
    { ADMG_StateAccessRejected(0, __func__); Free(created); goto ADMG_reject_data; }
    *created = staged;
    struct FloorTrainers *previous = originalCache;
    u32 *previousCounter = gTrainerHillVBlankCounter;
    sFloorTrainers = created;
#if FREE_TRAINER_HILL == FALSE
    SetTrainerHillVBlankCounter(&save->trainerHill.timer);
#endif
    if (!ADMG_EndStateAccessCheck(generation) || sFloorTrainers != created || sHillData != data
        || gSaveBlock1Ptr != save || gMapHeader.mapLayoutId != layout) goto ADMG_reject_cache;
    FreeDataStruct();
    if (!ADMG_EndStateAccessCheck(generation) || sFloorTrainers != created
        || gSaveBlock1Ptr != save || gMapHeader.mapLayoutId != layout) goto ADMG_reject_cache;
    if (previous != NULL && previous != created) Free(previous);
    return;
ADMG_reject_cache:
    ADMG_StateAccessRejected(0, __func__);
    if (sFloorTrainers == created) sFloorTrainers = previous;
    gTrainerHillVBlankCounter = previousCounter;
    Free(created);
ADMG_reject_data:
    if (previousData == NULL && sHillData == data) FreeDataStruct();
}

void FreeTrainerHillBattleStruct(void)
{
    TRY_FREE_AND_SET_NULL(sFloorTrainers);
}

static void SetUpDataStruct(void)
{
#if FREE_TRAINER_HILL == FALSE
    if (sHillData != NULL) return;
    struct SaveBlock1 *save = gSaveBlock1Ptr;
    if (save == NULL || save->trainerHill.mode >= ARRAY_COUNT(sChallengeData)
        || save->trainerHill.mode >= ARRAY_COUNT(sFloorData))
    { ADMG_StateAccessRejected(0, __func__); return; }
    u32 mode = save->trainerHill.mode, layout = gMapHeader.mapLayoutId;
    u64 generation = ADMG_BeginStateAccessCheck();
    sHillData = AllocZeroed(sizeof(*sHillData));
    typeof(sHillData) allocated = sHillData;
    if (allocated == NULL)
    { ADMG_StateAccessRejected(0, __func__); return; }
    if (!ADMG_EndStateAccessCheck(generation) || gSaveBlock1Ptr != save
        || save->trainerHill.mode != mode || gMapHeader.mapLayoutId != layout) goto ADMG_reject;
    allocated->floorId = layout - LAYOUT_TRAINER_HILL_1F;
    CpuCopy32(sChallengeData[mode], &allocated->challenge, sizeof(allocated->challenge));
    if (!ADMG_EndStateAccessCheck(generation) || sHillData != allocated || gSaveBlock1Ptr != save
        || save->trainerHill.mode != mode || gMapHeader.mapLayoutId != layout) goto ADMG_reject;
    CpuCopy32(sFloorData[mode], &allocated->floors, sizeof(allocated->floors));
    if (!ADMG_EndStateAccessCheck(generation) || sHillData != allocated || gSaveBlock1Ptr != save
        || save->trainerHill.mode != mode || gMapHeader.mapLayoutId != layout) goto ADMG_reject;
    return;
ADMG_reject:
    ADMG_StateAccessRejected(0, __func__);
    if (sHillData == allocated) sHillData = NULL;
    Free(allocated);
#endif // FREE_TRAINER_HILL
}

static void FreeDataStruct(void)
{
    TRY_FREE_AND_SET_NULL(sHillData);
}

void CopyTrainerHillTrainerText(u8 which, enum TrainerId localId)
{
    u32 slot;
    if (!ADMG_TryHillTrainerSlot(localId, &slot)) return;
    if (which < TRAINER_HILL_TEXT_INTRO || which > TRAINER_HILL_TEXT_AFTER)
    { ADMG_StateAccessRejected(which, __func__); return; }
    u32 layout = gMapHeader.mapLayoutId;
    if (layout < LAYOUT_TRAINER_HILL_1F || layout - LAYOUT_TRAINER_HILL_1F >= NUM_TRAINER_HILL_FLOORS)
    { ADMG_StateAccessRejected(layout, __func__); return; }
    typeof(sHillData) previous = sHillData;
    struct SaveBlock1 *save = gSaveBlock1Ptr;
    u64 generation = ADMG_BeginStateAccessCheck();
    if (!ADMG_TryHillTrainerData()) return;
    typeof(sHillData) data = sHillData;
    const struct TrainerHillTrainer *trainer = &data->floors[layout - LAYOUT_TRAINER_HILL_1F].trainers[slot];
    const u16 *speech = which == TRAINER_HILL_TEXT_INTRO ? trainer->speechBefore
        : which == TRAINER_HILL_TEXT_PLAYER_LOST ? trainer->speechWin
        : which == TRAINER_HILL_TEXT_PLAYER_WON ? trainer->speechLose : trainer->speechAfter;
    FrontierSpeechToString(speech);
    if (!ADMG_EndStateAccessCheck(generation) || sHillData != data
        || gSaveBlock1Ptr != save || gMapHeader.mapLayoutId != layout)
    {
        ADMG_StateAccessRejected(localId, __func__);
        if (previous == NULL && sHillData == data) FreeDataStruct();
        return;
    }
    FreeDataStruct();
    if (gSaveBlock1Ptr != save || gMapHeader.mapLayoutId != layout)
        ADMG_StateAccessRejected(localId, __func__);
}

static void TrainerHillStartChallenge(void)
{
    TrainerHillDummy();
#if FREE_TRAINER_HILL == FALSE
    if (!ReadTrainerHillAndValidate())
        gSaveBlock1Ptr->trainerHill.field_3D6E_0f = 1;
    else
        gSaveBlock1Ptr->trainerHill.field_3D6E_0f = 0;

    gSaveBlock1Ptr->trainerHill.unk_3D6C = 0;
    SetTrainerHillVBlankCounter(&gSaveBlock1Ptr->trainerHill.timer);
    gSaveBlock1Ptr->trainerHill.timer = 0;
    gSaveBlock1Ptr->trainerHill.spokeToOwner = 0;
    gSaveBlock1Ptr->trainerHill.checkedFinalTime = 0;
    gSaveBlock1Ptr->trainerHill.maybeECardScanDuringChallenge = 0;
    gSaveBlock2Ptr->frontier.trainerFlags = 0;
    gBattleOutcome = 0;
    gSaveBlock1Ptr->trainerHill.receivedPrize = 0;
#endif //FREE_TRAINER_HILL
}

static void GetOwnerState(void)
{
#if FREE_TRAINER_HILL == FALSE
    ClearTrainerHillVBlankCounter();
    gSpecialVar_Result = 0;
    if (gSaveBlock1Ptr->trainerHill.spokeToOwner)
        gSpecialVar_Result++;
    if (gSaveBlock1Ptr->trainerHill.receivedPrize && gSaveBlock1Ptr->trainerHill.checkedFinalTime)
        gSpecialVar_Result++;

    gSaveBlock1Ptr->trainerHill.spokeToOwner = TRUE;
#endif //FREE_TRAINER_HILL
}

static void GiveChallengePrize(void)
{
#if FREE_TRAINER_HILL == FALSE
    enum Item itemId = GetPrizeItemId();

    if (sHillData->challenge.numFloors != NUM_TRAINER_HILL_FLOORS || gSaveBlock1Ptr->trainerHill.receivedPrize)
    {
        gSpecialVar_Result = 2;
    }
    else if (AddBagItem(itemId, 1) == TRUE)
    {
        CopyItemName(itemId, gStringVar2);
        gSaveBlock1Ptr->trainerHill.receivedPrize = TRUE;
        gSaveBlock2Ptr->frontier.unk_EF9 = 0;
        gSpecialVar_Result = 0;
    }
    else
    {
        gSpecialVar_Result = 1;
    }
#endif //FREE_TRAINER_HILL
}

// If bestTime > timer, the challenge was completed faster and its a new record
// Otherwise the owner says it was a slow time and to complete it faster next time
static void CheckFinalTime(void)
{
#if FREE_TRAINER_HILL == FALSE
    if (gSaveBlock1Ptr->trainerHill.checkedFinalTime)
    {
        gSpecialVar_Result = 2;
    }
    else if (GetTimerValue(&gSaveBlock1Ptr->trainerHill.bestTime) > gSaveBlock1Ptr->trainerHill.timer)
    {
        SetTimerValue(&gSaveBlock1Ptr->trainerHill.bestTime, gSaveBlock1Ptr->trainerHill.timer);
        gSaveBlock1Ptr->trainerHillTimes[gSaveBlock1Ptr->trainerHill.mode] = gSaveBlock1Ptr->trainerHill.bestTime;
        gSpecialVar_Result = 0;
    }
    else
    {
        gSpecialVar_Result = 1;
    }

    gSaveBlock1Ptr->trainerHill.checkedFinalTime = TRUE;
#endif //FREE_TRAINER_HILL
}

static void TrainerHillResumeTimer(void)
{
#if FREE_TRAINER_HILL == FALSE
    if (!gSaveBlock1Ptr->trainerHill.spokeToOwner)
    {
        if (gSaveBlock1Ptr->trainerHill.timer >= HILL_MAX_TIME)
            gSaveBlock1Ptr->trainerHill.timer = HILL_MAX_TIME;
        else
            SetTrainerHillVBlankCounter(&gSaveBlock1Ptr->trainerHill.timer);
    }
#endif //FREE_TRAINER_HILL
}

static void TrainerHillSetPlayerLost(void)
{
#if FREE_TRAINER_HILL == FALSE
    gSaveBlock1Ptr->trainerHill.hasLost = TRUE;
#endif //FREE_TRAINER_HILL
}

static void TrainerHillGetChallengeStatus(void)
{
#if FREE_TRAINER_HILL == FALSE
    if (gSaveBlock1Ptr->trainerHill.hasLost)
    {
        // The player lost their last match.
        gSaveBlock1Ptr->trainerHill.hasLost = FALSE;
        gSpecialVar_Result = TRAINER_HILL_PLAYER_STATUS_LOST;
    }
    else if (gSaveBlock1Ptr->trainerHill.maybeECardScanDuringChallenge)
    {
        // Unreachable code. Something relating to eCards?
        gSaveBlock1Ptr->trainerHill.maybeECardScanDuringChallenge = 0;
        gSpecialVar_Result = TRAINER_HILL_PLAYER_STATUS_ECARD_SCANNED;
    }
    else
    {
        // Continue playing.
        gSpecialVar_Result = TRAINER_HILL_PLAYER_STATUS_NORMAL;
    }
#endif //FREE_TRAINER_HILL
}

static void BufferChallengeTime(void)
{
#if FREE_TRAINER_HILL == FALSE
    s32 total, minutes, secondsWhole, secondsFraction;

    total = gSaveBlock1Ptr->trainerHill.timer;
    if (total >= HILL_MAX_TIME)
        total = HILL_MAX_TIME;

    minutes = total / (60 * 60);
    total %= (60 * 60);
    secondsWhole = total / 60;
    total %= 60;
    secondsFraction = (total * 168) / 100;

    ConvertIntToDecimalStringN(gStringVar1, minutes, STR_CONV_MODE_RIGHT_ALIGN, 2);
    ConvertIntToDecimalStringN(gStringVar2, secondsWhole, STR_CONV_MODE_RIGHT_ALIGN, 2);
    ConvertIntToDecimalStringN(gStringVar3, secondsFraction, STR_CONV_MODE_LEADING_ZEROS, 2);
#endif //FREE_TRAINER_HILL
}

// Returns TRUE if all 4 floors are used
// Returns FALSE otherwise, and buffers the number of floors used
// The only time fewer than all 4 floors are used is for the JP-exclusive E-Reader and Default modes
static void GetAllFloorsUsed(void)
{
    SetUpDataStruct();
    if (sHillData->challenge.numFloors != NUM_TRAINER_HILL_FLOORS)
    {
        ConvertIntToDecimalStringN(gStringVar1, sHillData->challenge.numFloors, STR_CONV_MODE_LEFT_ALIGN, 1);
        gSpecialVar_Result = FALSE;
    }
    else
    {
        gSpecialVar_Result = TRUE;
    }

    FreeDataStruct();
}

// May have been dummied. Every time this is called a conditional for var result occurs afterwards
// Relation to E-Reader is an assumption, most dummied Trainer Hill code seems to be JP E-Reader mode related
static void GetInEReaderMode(void)
{
    SetUpDataStruct();
    gSpecialVar_Result = FALSE;
    FreeDataStruct();
}

bool8 InTrainerHillChallenge(void)
{
#if FREE_TRAINER_HILL == FALSE
    if (VarGet(VAR_TRAINER_HILL_IS_ACTIVE) == 0)
        return FALSE;
    else if (gSaveBlock1Ptr->trainerHill.spokeToOwner)
        return FALSE;
    else if (GetCurrentTrainerHillMapId() != 0)
        return TRUE;
    else
        return FALSE;
#else
    return FALSE;
#endif //FREE_TRAINER_HILL
}

static void IsTrainerHillChallengeActive(void)
{
    if (!InTrainerHillChallenge())
        gSpecialVar_Result = FALSE;
    else
        gSpecialVar_Result = TRUE;
}

static void TrainerHillDummy(void)
{

}

void PrintOnTrainerHillRecordsWindow(void)
{
#if FREE_TRAINER_HILL == FALSE
    s32 i, x, y;
    u32 total, minutes, secondsWhole, secondsFraction;

    SetUpDataStruct();
    FillWindowPixelBuffer(0, PIXEL_FILL(0));
    x = GetStringCenterAlignXOffset(FONT_NORMAL, gText_TimeBoard, 0xD0);
    AddTextPrinterParameterized3(0, FONT_NORMAL, x, 2, sRecordWinColors, TEXT_SKIP_DRAW, gText_TimeBoard);

    y = 18;
    for (i = 0; i < NUM_TRAINER_HILL_MODES; i++)
    {
        AddTextPrinterParameterized3(0, FONT_NORMAL, 0, y, sRecordWinColors, TEXT_SKIP_DRAW, sModeStrings[i]);
        y += 15;
        total = GetTimerValue(&gSaveBlock1Ptr->trainerHillTimes[i]);
        minutes = total / (60 * 60);
        total %= (60 * 60);
        ConvertIntToDecimalStringN(gStringVar1, minutes, STR_CONV_MODE_RIGHT_ALIGN, 2);
        secondsWhole = total / 60;
        total %= 60;
        ConvertIntToDecimalStringN(gStringVar2, secondsWhole, STR_CONV_MODE_RIGHT_ALIGN, 2);
        secondsFraction = (total * 168) / 100;
        ConvertIntToDecimalStringN(gStringVar3, secondsFraction, STR_CONV_MODE_LEADING_ZEROS, 2);
        StringExpandPlaceholders(StringCopy(gStringVar4, gText_TimeCleared), gText_XMinYDotZSec);
        x = GetStringRightAlignXOffset(FONT_NORMAL, gStringVar4, 0xD0);
        AddTextPrinterParameterized3(0, FONT_NORMAL, x, y, sRecordWinColors, TEXT_SKIP_DRAW, gStringVar4);
        y += 17;
    }

    PutWindowTilemap(0);
    CopyWindowToVram(0, COPYWIN_FULL);
    FreeDataStruct();
#endif //FREE_TRAINER_HILL
}

// Leftover from Fire Red / Leaf Green as in these games,
// the timer had to be xored by the encryption key in Sav2.
#if FREE_TRAINER_HILL == FALSE
static u32 GetTimerValue(u32 *src)
{
    return *src;
}

static void SetTimerValue(u32 *dst, u32 val)
{
    *dst = val;
}
#endif //FREE_TRAINER_HILL

void LoadTrainerHillObjectEventTemplates(void)
{
    u8 i, floorId;
    struct ObjectEventTemplate *eventTemplates = gSaveBlock1Ptr->objectEventTemplates;

    if (!LoadTrainerHillFloorObjectEventScripts())
        return;

    SetUpDataStruct();
    for (i = 0; i < HILL_TRAINERS_PER_FLOOR; i++)
        gSaveBlock2Ptr->frontier.trainerIds[i] = 0xFFFF;
    CpuFill32(0, gSaveBlock1Ptr->objectEventTemplates, sizeof(gSaveBlock1Ptr->objectEventTemplates));

    floorId = GetFloorId();
    for (i = 0; i < HILL_TRAINERS_PER_FLOOR; i++)
    {
        u8 bits;

        eventTemplates[i] = sTrainerObjectEventTemplate;
        eventTemplates[i].localId = i + 1;
        eventTemplates[i].graphicsId = FacilityClassToGraphicsId(sHillData->floors[floorId].trainers[i].facilityClass);
        eventTemplates[i].x = sHillData->floors[floorId].map.trainerCoords[i] & 0xF;
        eventTemplates[i].y = ((sHillData->floors[floorId].map.trainerCoords[i] >> 4) & 0xF) + HILL_FLOOR_HEIGHT_MARGIN;
        bits = i << 2;
        eventTemplates[i].movementType = ((sHillData->floors[floorId].map.trainerDirections >> bits) & 0xF) + MOVEMENT_TYPE_FACE_UP;
        eventTemplates[i].trainerRange_berryTreeId = (sHillData->floors[floorId].map.trainerRanges >> bits) & 0xF;
        eventTemplates[i].script = TrainerHill_EventScript_TrainerBattle;
        gSaveBlock2Ptr->frontier.trainerIds[i] = i + 1;
    }

    FreeDataStruct();
}

bool32 LoadTrainerHillFloorObjectEventScripts(void)
{
    SetUpDataStruct();
    // Something may have been dummied here
    FreeDataStruct();
    return TRUE;
}

static u16 GetMapDataForFloor(u8 floorId, u32 x, u32 y, u32 floorWidth) // floorWidth is always 16
{
    bool8 impassable;
    u16 metatileId;
    u16 elevation;

    impassable = (sHillData->floors[floorId].map.collisionData[y] >> (15 - x) & 1);
    metatileId = sHillData->floors[floorId].map.metatileData[floorWidth * y + x] + NUM_METATILES_IN_PRIMARY;
    elevation = PACK_ELEVATION(ELEVATION_DEFAULT);

    return PACK_COLLISION(impassable) | elevation | PACK_METATILE(metatileId);
}

void GenerateTrainerHillFloorLayout(u16 *mapArg)
{
    s32 y, x;
    const u16 *src;
    u16 *dst;
    u8 mapId = GetCurrentTrainerHillMapId();

    if (mapId == TRAINER_HILL_ENTRANCE)
    {
        InitMapFromSavedGame();
        return;
    }

    SetUpDataStruct();
    if (mapId == TRAINER_HILL_ROOF)
    {
        InitMapFromSavedGame();
        FreeDataStruct();
        return;
    }

    mapId = GetFloorId();
    src = gMapHeader.mapLayout->map;
    gBackupMapLayout.map = mapArg;
    // Dimensions include border area loaded beyond map
    gBackupMapLayout.width = HILL_FLOOR_WIDTH + 15;
    gBackupMapLayout.height = HILL_FLOOR_HEIGHT + 14;
    dst = mapArg + 224;

    // First 5 rows of the map (Entrance / Exit) are always the same
    for (y = 0; y < HILL_FLOOR_HEIGHT_MARGIN; y++)
    {
        for (x = 0; x < HILL_FLOOR_WIDTH; x++)
            dst[x] = src[x];
        dst += 31;
        src += 16;
    }

    // Load the 16x16 floor-specific layout
    for (y = 0; y < HILL_FLOOR_HEIGHT_MAIN; y++)
    {
        for (x = 0; x < HILL_FLOOR_WIDTH; x++)
            dst[x] = GetMapDataForFloor(mapId, x, y, HILL_FLOOR_WIDTH);
        dst += 31;
    }

    RunOnLoadMapScript();
    FreeDataStruct();
}

bool32 InTrainerHill(void)
{
    bool32 ret;

    if (gMapHeader.mapLayoutId == LAYOUT_TRAINER_HILL_1F
        || gMapHeader.mapLayoutId == LAYOUT_TRAINER_HILL_2F
        || gMapHeader.mapLayoutId == LAYOUT_TRAINER_HILL_3F
        || gMapHeader.mapLayoutId == LAYOUT_TRAINER_HILL_4F)
        ret = TRUE;
    else
        ret = FALSE;

    return ret;
}

u8 GetCurrentTrainerHillMapId(void)
{
    u8 mapId;

    if (gMapHeader.mapLayoutId == LAYOUT_TRAINER_HILL_1F)
        mapId = TRAINER_HILL_1F;
    else if (gMapHeader.mapLayoutId == LAYOUT_TRAINER_HILL_2F)
        mapId = TRAINER_HILL_2F;
    else if (gMapHeader.mapLayoutId == LAYOUT_TRAINER_HILL_3F)
        mapId = TRAINER_HILL_3F;
    else if (gMapHeader.mapLayoutId == LAYOUT_TRAINER_HILL_4F)
        mapId = TRAINER_HILL_4F;
    else if (gMapHeader.mapLayoutId == LAYOUT_TRAINER_HILL_ROOF)
        mapId = TRAINER_HILL_ROOF;
    else if (gMapHeader.mapLayoutId == LAYOUT_TRAINER_HILL_ENTRANCE)
        mapId = TRAINER_HILL_ENTRANCE;
    else
        mapId = 0;

    return mapId;
}

static bool32 UNUSED OnTrainerHillRoof(void)
{
    bool32 onRoof;

    if (gMapHeader.mapLayoutId == LAYOUT_TRAINER_HILL_ROOF)
        onRoof = TRUE;
    else
        onRoof = FALSE;

    return onRoof;
}

const struct WarpEvent* SetWarpDestinationTrainerHill4F(void)
{
    const struct MapHeader *header = Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_TRAINER_HILL_4F), MAP_NUM(MAP_TRAINER_HILL_4F));

    return &header->events->warps[1];
}

// For warping from the roof in challenges where the 4F is not the final challenge floor
// This would only occur in the JP-exclusive Default and E-Reader challenges
const struct WarpEvent* SetWarpDestinationTrainerHillFinalFloor(u8 warpEventId)
{
    u8 numFloors;
    const struct MapHeader *header;

    if (warpEventId == 1)
        return &gMapHeader.events->warps[1];

    numFloors = GetNumFloorsInTrainerHillChallenge();
    if (numFloors == 0 || numFloors > NUM_TRAINER_HILL_FLOORS)
        numFloors = NUM_TRAINER_HILL_FLOORS;

    header = Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_TRAINER_HILL_4F), sNextFloorMapNum[numFloors - 1]);
    return &header->events->warps[0];
}

static bool8 ADMG_TryHillMapSlot(u32 localId, u32 *slot, u32 *bit)
{
    if (gSaveBlock2Ptr == NULL || bit == NULL || !ADMG_TryHillTrainerSlot(localId, slot))
    { if (gSaveBlock2Ptr == NULL || bit == NULL) ADMG_StateAccessRejected(localId, __func__); return FALSE; }
    u32 layout = gMapHeader.mapLayoutId;
    if (layout < LAYOUT_TRAINER_HILL_1F || layout - LAYOUT_TRAINER_HILL_1F >= NUM_TRAINER_HILL_FLOORS)
    { ADMG_StateAccessRejected(layout, __func__); return FALSE; }
    *bit = (layout - LAYOUT_TRAINER_HILL_1F) * HILL_TRAINERS_PER_FLOOR + *slot;
    if (*bit >= sizeof(gSaveBlock2Ptr->frontier.trainerFlags) * CHAR_BIT)
    { ADMG_StateAccessRejected(*bit, __func__); return FALSE; }
    return TRUE;
}

enum TrainerId LocalIdToHillTrainerId(u8 localId)
{
    u32 slot;
    if (gSaveBlock2Ptr == NULL || !ADMG_TryHillTrainerSlot(localId, &slot))
    { if (gSaveBlock2Ptr == NULL) ADMG_StateAccessRejected(localId, __func__); return 0; }
    enum TrainerId identity = gSaveBlock2Ptr->frontier.trainerIds[slot];
    u32 nativeSlot;
    if (!ADMG_TryHillTrainerSlot(identity, &nativeSlot)) return 0;
    return identity;
}

bool8 GetHillTrainerFlag(u8 objectEventId)
{
    if ((u32)objectEventId >= OBJECT_EVENTS_COUNT)
    { ADMG_StateAccessRejected(objectEventId, __func__); return FALSE; }
    u32 slot, bit;
    if (!ADMG_TryHillMapSlot(gObjectEvents[objectEventId].localId, &slot, &bit)) return FALSE;
    return gSaveBlock2Ptr->frontier.trainerFlags & (1u << bit);
}

/* One native authority for first-match Hill marking and owned rollback bits. */
ADMG_NATIVE_ENTRY bool8 ADMG_TryGetHillTrainerFlagMask(u32 actorA, u32 actorB, bool8 second, u8 *mask)
{
    u32 slot, bit, nativeSlot;
    if (mask == NULL)
    { ADMG_StateAccessRejected(actorA, __func__); return FALSE; }
    if (!ADMG_TryHillMapSlot(1, &slot, &bit)
        || !ADMG_TryHillTrainerSlot(actorA, &nativeSlot)
        || (second && !ADMG_TryHillTrainerSlot(actorB, &nativeSlot))) return FALSE;
    u32 value = 0;
    for (u32 i = 0; i < HILL_TRAINERS_PER_FLOOR; i++)
        if (gSaveBlock2Ptr->frontier.trainerIds[i] == actorA)
        { value |= 1u << (bit + i); break; }
    if (second)
        for (u32 i = 0; i < HILL_TRAINERS_PER_FLOOR; i++)
            if (gSaveBlock2Ptr->frontier.trainerIds[i] == actorB)
            { value |= 1u << (bit + i); break; }
    *mask = value;
    return TRUE;
}

void SetHillTrainerFlag(void)
{
    u8 mask;
    if (!ADMG_TryGetHillTrainerFlagMask(TRAINER_BATTLE_PARAM.opponentA, TRAINER_BATTLE_PARAM.opponentB,
        (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS) != 0, &mask)) return;
    gSaveBlock2Ptr->frontier.trainerFlags |= mask;
}

const u8 *GetTrainerHillTrainerScript(void)
{
    return TrainerHill_EventScript_TrainerBattle;
}

static void ShowTrainerHillPostBattleText(void)
{
    u16 localId = gSpecialVar_LastTalked;
    u64 generation = ADMG_BeginStateAccessCheck();
    CopyTrainerHillTrainerText(TRAINER_HILL_TEXT_AFTER, gSpecialVar_LastTalked);
    if (!ADMG_EndStateAccessCheck(generation)) return;
    if (gSpecialVar_LastTalked != localId)
    { ADMG_StateAccessRejected(localId, __func__); return; }
    ShowFieldMessageFromBuffer();
}

static void CreateNPCTrainerHillParty(u16 trainerId, enum BattleTrainer trainer);

/* This scope is transient call state, never a save/persistence record. */
struct ADMG_HillPartyScope
{
    typeof(sHillData) data;
    struct SaveBlock1 *save;
    u64 generation;
    u32 layout, floor, flags, actorA, actorB;
    bool8 bound, second;
};
static const struct ADMG_HillPartyScope *sADMG_BorrowedHillParty;

static bool8 ADMG_HillPartyCurrent(const struct ADMG_HillPartyScope *scope, bool8 live)
{
    if (!ADMG_EndStateAccessCheck(scope->generation)) return FALSE;
    if (sADMG_BorrowedHillParty != scope || gSaveBlock1Ptr != scope->save
        || gMapHeader.mapLayoutId != scope->layout || gBattleTypeFlags != scope->flags
        || (live && (sHillData != scope->data || scope->data->floorId != scope->floor))
        || (!live && sHillData != NULL)
        || (scope->bound && (TRAINER_BATTLE_PARAM.opponentA != scope->actorA
            || (scope->second && TRAINER_BATTLE_PARAM.opponentB != scope->actorB))))
    { ADMG_StateAccessRejected(scope->actorA, __func__); return FALSE; }
    return TRUE;
}

ADMG_NATIVE_ENTRY bool8 ADMG_TryAdmitHillParty(u32 identifier, u32 target)
{
    u32 slot;
    if (!ADMG_TryHillTrainerSlot(identifier, &slot)) return FALSE;
    if (target >= ARRAY_COUNT(gParties)
        || MULTI_PARTY_SIZE > ARRAY_COUNT(sTrainerPartySlots[slot]))
    { ADMG_StateAccessRejected(target, __func__); return FALSE; }
    u32 layout = gMapHeader.mapLayoutId;
    if (layout < LAYOUT_TRAINER_HILL_1F || layout - LAYOUT_TRAINER_HILL_1F >= NUM_TRAINER_HILL_FLOORS)
    { ADMG_StateAccessRejected(layout, __func__); return FALSE; }
    for (u32 i = 0; i < MULTI_PARTY_SIZE; i++)
        if (sTrainerPartySlots[slot][i] >= PARTY_SIZE)
        { ADMG_StateAccessRejected(sTrainerPartySlots[slot][i], __func__); return FALSE; }
    return TRUE;
}

static bool8 ADMG_CaptureHillPartyScope(struct ADMG_HillPartyScope *scope, bool8 bound, bool8 second)
{
    typeof(sHillData) previousData = sHillData;
    scope->save = gSaveBlock1Ptr; scope->layout = gMapHeader.mapLayoutId;
    scope->flags = gBattleTypeFlags; scope->generation = ADMG_BeginStateAccessCheck();
    scope->bound = bound; scope->second = second;
    scope->actorA = bound ? TRAINER_BATTLE_PARAM.opponentA : 0;
    scope->actorB = bound && second ? TRAINER_BATTLE_PARAM.opponentB : 0;
    if (!ADMG_TryHillTrainerData() || !ADMG_EndStateAccessCheck(scope->generation)) goto ADMG_reject;
    scope->data = sHillData; scope->floor = sHillData->floorId;
    if (gSaveBlock1Ptr != scope->save || gMapHeader.mapLayoutId != scope->layout
        || gBattleTypeFlags != scope->flags
        || (bound && (TRAINER_BATTLE_PARAM.opponentA != scope->actorA
            || (second && TRAINER_BATTLE_PARAM.opponentB != scope->actorB))))
    { ADMG_StateAccessRejected(scope->actorA, __func__); goto ADMG_reject; }
    return TRUE;
ADMG_reject:
    if (previousData == NULL && sHillData != NULL) FreeDataStruct();
    return FALSE;
}

static bool8 ADMG_RunHillPartyConstructor(u16 trainerId, enum BattleTrainer trainer,
    const struct ADMG_HillPartyScope *scope)
{
    if (!ADMG_TryAdmitHillParty(trainerId, trainer) || !ADMG_HillPartyCurrent(scope, TRUE)) return FALSE;
    u32 slot = trainerId - 1;
    u32 floor = scope->layout - LAYOUT_TRAINER_HILL_1F;
    u32 level = GetHighestLevelInPlayerParty();
    if (!ADMG_HillPartyCurrent(scope, TRUE)) return FALSE;
    if (level > MAX_LEVEL)
    { ADMG_StateAccessRejected(level, __func__); return FALSE; }
    for (u32 i = 0; i < MULTI_PARTY_SIZE; i++)
    {
        u32 index = sTrainerPartySlots[slot][i];
        struct Pokemon *mon = &gParties[trainer][i];
        CreateBattleTowerMon(mon, &scope->data->floors[floor].trainers[slot].mons[index]);
        if (!ADMG_HillPartyCurrent(scope, TRUE)) return FALSE;
        SetTrainerHillMonLevel(mon, level);
        if (!ADMG_HillPartyCurrent(scope, TRUE)) return FALSE;
    }
    return TRUE;
}

static void CreateNPCTrainerHillParty(u16 trainerId, enum BattleTrainer trainer)
{
    if (!ADMG_TryAdmitHillParty(trainerId, trainer)) return;
    const struct ADMG_HillPartyScope *previous = sADMG_BorrowedHillParty;
    if (previous != NULL && previous->bound
        && ((trainer == B_TRAINER_OPPONENT_A && trainerId == previous->actorA)
            || (previous->second && trainer == B_TRAINER_OPPONENT_B && trainerId == previous->actorB)))
    {
        ADMG_RunHillPartyConstructor(trainerId, trainer, previous);
        return;
    }
    struct ADMG_HillPartyScope scope;
    if (!ADMG_CaptureHillPartyScope(&scope, FALSE, FALSE)) return;
    sADMG_BorrowedHillParty = &scope;
    struct Pokemon saved[PARTY_SIZE];
    memcpy(saved, gParties[trainer], sizeof(saved));
    bool8 okay = ADMG_RunHillPartyConstructor(trainerId, trainer, &scope);
    if (previous == NULL && sHillData == scope.data)
    {
        FreeDataStruct();
        if (!ADMG_HillPartyCurrent(&scope, FALSE)) okay = FALSE;
    }
    if (!okay) memcpy(gParties[trainer], saved, sizeof(saved));
    sADMG_BorrowedHillParty = previous;
}

static void ADMG_FillHillParties(bool8 second)
{
    u32 actorA = TRAINER_BATTLE_PARAM.opponentA;
    u32 actorB = second ? TRAINER_BATTLE_PARAM.opponentB : 0;
    if (!ADMG_TryAdmitHillParty(actorA, B_TRAINER_OPPONENT_A)
        || (second && !ADMG_TryAdmitHillParty(actorB, B_TRAINER_OPPONENT_B))) return;
    const struct ADMG_HillPartyScope *previous = sADMG_BorrowedHillParty;
    struct ADMG_HillPartyScope scope;
    if (!ADMG_CaptureHillPartyScope(&scope, TRUE, second)) return;
    sADMG_BorrowedHillParty = &scope;
    struct Pokemon savedA[PARTY_SIZE], savedB[PARTY_SIZE];
    memcpy(savedA, gParties[B_TRAINER_OPPONENT_A], sizeof(savedA));
    memcpy(savedB, gParties[B_TRAINER_OPPONENT_B], sizeof(savedB));
    u32 countA = gPartiesCount[B_TRAINER_OPPONENT_A], countB = gPartiesCount[B_TRAINER_OPPONENT_B];
    bool8 okay = ADMG_HillPartyCurrent(&scope, TRUE);
    if (okay)
    {
        ZeroEnemyPartyMons();
        okay = ADMG_HillPartyCurrent(&scope, TRUE);
    }
    if (okay)
    {
        CreateNPCTrainerHillParty(actorA, B_TRAINER_OPPONENT_A);
        okay = ADMG_HillPartyCurrent(&scope, TRUE);
    }
    if (okay && second)
    {
        CreateNPCTrainerHillParty(actorB, B_TRAINER_OPPONENT_B);
        okay = ADMG_HillPartyCurrent(&scope, TRUE);
    }
    if (previous == NULL && sHillData == scope.data)
    {
        FreeDataStruct();
        if (!ADMG_HillPartyCurrent(&scope, FALSE)) okay = FALSE;
    }
    if (!okay)
    {
        memcpy(gParties[B_TRAINER_OPPONENT_A], savedA, sizeof(savedA));
        memcpy(gParties[B_TRAINER_OPPONENT_B], savedB, sizeof(savedB));
        gPartiesCount[B_TRAINER_OPPONENT_A] = countA;
        gPartiesCount[B_TRAINER_OPPONENT_B] = countB;
    }
    sADMG_BorrowedHillParty = previous;
}

void FillHillTrainerParty(void)
{
    ADMG_FillHillParties(FALSE);
}

void FillHillTrainersParties(void)
{
    ADMG_FillHillParties(TRUE);
}

u8 GetTrainerEncounterMusicIdInTrainerHill(enum TrainerId trainerId)
{
    u32 facilityClass;
    if (!ADMG_TryHillTrainerFacilityClass(trainerId, &facilityClass)) return 0;
    for (u32 i = 0; i < ARRAY_COUNT(sTrainerClassesAndMusic); i++)
        if (sTrainerClassesAndMusic[i].trainerClass == gFacilityClassToTrainerClass[facilityClass])
            return sTrainerClassesAndMusic[i].musicId;
    return 0;
}

static void SetTrainerHillMonLevel(struct Pokemon *mon, u8 level)
{
    if (mon == NULL || level > MAX_LEVEL)
    { ADMG_StateAccessRejected(level, __func__); return; }
    u64 generation = ADMG_BeginStateAccessCheck();
    u32 species = GetMonData(mon, MON_DATA_SPECIES);
    if (!ADMG_EndStateAccessCheck(generation)) return;
    if (!IsSpeciesIdValid(species) || species == SPECIES_EGG || gSpeciesInfo[species].growthRate >= gADMG_ExperienceTableCount)
    { ADMG_StateAccessRejected(species, __func__); return; }
    u32 exp = gExperienceTables[gSpeciesInfo[species].growthRate][level];

    SetMonData(mon, MON_DATA_EXP, &exp);
    if (!ADMG_EndStateAccessCheck(generation)) return;
    SetMonData(mon, MON_DATA_LEVEL, &level);
    if (!ADMG_EndStateAccessCheck(generation)) return;
    CalculateMonStats(mon);
}

u8 GetNumFloorsInTrainerHillChallenge(void)
{
    u8 floors;

    SetUpDataStruct();
    floors = sHillData->challenge.numFloors;
    FreeDataStruct();

    return floors;
}

static void SetAllTrainerFlags(void)
{
    gSaveBlock2Ptr->frontier.trainerFlags = 0xFF;
}

// Palette never loaded, OnTrainerHillEReaderChallengeFloor always FALSE
void TryLoadTrainerHillEReaderPalette(void)
{
    if (OnTrainerHillEReaderChallengeFloor() == TRUE)
        LoadPalette(sEReader_Pal, BG_PLTT_ID(7), PLTT_SIZE_4BPP);
}

static void GetGameSaved(void)
{
    gSpecialVar_Result = gSaveBlock2Ptr->frontier.savedGame;
}

static void SetGameSaved(void)
{
    gSaveBlock2Ptr->frontier.savedGame = TRUE;
}

static void ClearGameSaved(void)
{
    gSaveBlock2Ptr->frontier.savedGame = FALSE;
}

// Always FALSE
bool32 OnTrainerHillEReaderChallengeFloor(void)
{
    if (!InTrainerHillChallenge() || GetCurrentTrainerHillMapId() == TRAINER_HILL_ENTRANCE)
        return FALSE;

    GetInEReaderMode();
    if (gSpecialVar_Result == FALSE)
        return FALSE;
    else
        return TRUE;
}

static void GetChallengeWon(void)
{
#if FREE_TRAINER_HILL == FALSE
    if (gSaveBlock1Ptr->trainerHill.hasLost)
        gSpecialVar_Result = FALSE;
    else
        gSpecialVar_Result = TRUE;
#endif //FREE_TRAINER_HILL
}

static void TrainerHillSetMode(void)
{
#if FREE_TRAINER_HILL == FALSE
    gSaveBlock1Ptr->trainerHill.mode = gSpecialVar_0x8005;
    gSaveBlock1Ptr->trainerHill.bestTime = gSaveBlock1Ptr->trainerHillTimes[gSpecialVar_0x8005];
#endif //FREE_TRAINER_HILL
}

// Determines which prize list to use from the set of prize lists.
#if FREE_TRAINER_HILL == FALSE
static u8 GetPrizeListId(bool8 allowTMs)
{
    u8 prizeListId, i, modBy;

    // The initial selection depends on the trainer numbers for the completed challenge.
    // These don't change with the available challenge modes, so Normal/Unique will always
    // have a prizeListId of 8, and Variety/Expert will have a prizeListId of 24.
    prizeListId = 0;
    for (i = 0; i < NUM_TRAINER_HILL_FLOORS; i++)
    {
        prizeListId ^= sHillData->floors[i].trainerNum1 & 0x1F;
        prizeListId ^= sHillData->floors[i].trainerNum2 & 0x1F;
    }

    // In practice, the conditional below is always true.
    // The 2nd half of the lists in both sets of lists all have a TM as the "grand prize", while the 1st half do not,
    // so taking the mod of the (total / 2) ensures that a prize list without a TM will be used.
    if (allowTMs)
        modBy = NUM_TRAINER_HILL_PRIZE_LISTS;
    else
        modBy = NUM_TRAINER_HILL_PRIZE_LISTS / 2;

    prizeListId %= modBy;
    return prizeListId;
}

static enum Item GetPrizeItemId(void)
{
    u8 i;
    const enum Item *prizeList;
    s32 trainerNumSum = 0, prizeListSetId, minutes, id;

    // First determine which set of prize lists to use. The sets of lists only differ in
    // what TMs they can offer as the "grand prize" for a time under 12 minutes.
    // Which set of lists gets used is based on the sum of all the trainer numbers for that
    // challenge. These don't change with the available challenge modes, so Normal will always
    // have a prizeListSetId of 0, and Unique/Variety/Expert will have a prizeListSetId of 1.
    for (i = 0; i < NUM_TRAINER_HILL_FLOORS; i++)
    {
        trainerNumSum += sHillData->floors[i].trainerNum1;
        trainerNumSum += sHillData->floors[i].trainerNum2;
    }
    prizeListSetId = trainerNumSum / 256;
    prizeListSetId %= (int)ARRAY_COUNT(sPrizeListSets);

    // Now get which prize list to use from the set. See GetPrizeListId for details.
    // The below conditional will always be true, because a Trainer Hill challenge can't be entered
    // until the player has entered the Hall of Fame (FLAG_SYS_GAME_CLEAR is set) and because all
    // of the available challenge modes have the full 8 trainers (NUM_TRAINER_HILL_TRAINERS).
    if (FlagGet(FLAG_SYS_GAME_CLEAR) && sHillData->challenge.numTrainers == NUM_TRAINER_HILL_TRAINERS)
        i = GetPrizeListId(TRUE);
    else
        i = GetPrizeListId(FALSE);

    // 1 is added to Expert mode's prize list selection because otherwise it has the same prizes as Variety
    if (gSaveBlock1Ptr->trainerHill.mode == HILL_MODE_EXPERT)
        i = (i + 1) % NUM_TRAINER_HILL_PRIZE_LISTS;

    // After the above (non-random) calculations, the following are the possible prize list selections:
    // sPrizeListSets[0][8] (Normal)
    // sPrizeListSets[1][4] (Variety)
    // sPrizeListSets[1][8] (Unique)
    // sPrizeListSets[1][5] (Expert)
    prizeList = sPrizeListSets[prizeListSetId][i];

    // Which prize is given from the list depends on the time scored.
    // The prize for any time after 12 minutes is the same in every list.
    // The prizes for a time under 12 minutes are:
    // - ITEM_TM_SUNNY_DAY     (Normal)
    // - ITEM_ELIXIR           (Variety)
    // - ITEM_TM_GIGA_DRAIN    (Unique)
    // - ITEM_TM_BRICK_BREAK   (Expert)
    // As an additional note, if players were allowed to enter a Trainer Hill challenge before
    // entering the Hall of Fame, there would be 1 additional prize possibility (ITEM_MAX_ETHER)
    // as Normal / Unique modes would use sPrizeListSets[0][3] / sPrizeListSets[1][3] respectively.
    minutes = (signed)(gSaveBlock1Ptr->trainerHill.timer) / (60 * 60);
    if (minutes < 12)
        id = 0; // Depends on list
    else if (minutes < 13)
        id = 1; // ITEM_ETHER
    else if (minutes < 14)
        id = 2; // ITEM_MAX_POTION
    else if (minutes < 16)
        id = 3; // ITEM_REVIVE
    else if (minutes < 18)
        id = 4; // ITEM_FLUFFY_TAIL
    else
        id = 5; // ITEM_GREAT_BALL

    return prizeList[id];
}
#endif //FREE_TRAINER_HILL
