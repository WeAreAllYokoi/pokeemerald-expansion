#include "global.h"
#include "admg_trainer_actor.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_controllers.h"
#include "recorded_battle.h"
#include "main.h"
#include "pokemon.h"
#include "random.h"
#include "event_data.h"
#include "link.h"
#include "string_util.h"
#include "palette.h"
#include "save.h"
#include "malloc.h"
#include "util.h"
#include "task.h"
#include "test_runner.h"
#include "text.h"
#include "battle_setup.h"
#include "frontier_util.h"
#include "constants/trainers.h"
#include "constants/apprentice.h"
#include "constants/rgb.h"

struct PlayerInfo
{
    u32 trainerId;
    u8 name[PLAYER_NAME_LENGTH + 1];
    u8 gender;
    u16 battler;
    u16 language;
};


EWRAM_DATA rng_value_t gRecordedBattleRngSeed = RNG_VALUE_EMPTY;
EWRAM_DATA rng_value_t gBattlePalaceMoveSelectionRngValue = RNG_VALUE_EMPTY;
EWRAM_DATA static u8 sBattleRecords[MAX_BATTLERS_COUNT][BATTLER_RECORD_SIZE] = {0};
EWRAM_DATA static u16 sBattlerRecordSizes[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA static u16 sBattlerPrevRecordSizes[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA static u16 sBattlerSavedRecordSizes[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA static u8 sRecordMode = 0;
EWRAM_DATA static u8 sLvlMode = 0;
EWRAM_DATA static u8 sFrontierFacility = 0;
EWRAM_DATA static u8 sFrontierBrainSymbol = 0;
EWRAM_DATA static MainCallback sCallback2_AfterRecordedBattle = NULL;
EWRAM_DATA u8 gRecordedBattleMultiplayerId = 0;
EWRAM_DATA static u8 sFrontierPassFlag = 0;
EWRAM_DATA static u8 sBattleScene = 0;
EWRAM_DATA static u8 sTextSpeed = 0;
EWRAM_DATA static u32 sBattleFlags = 0;
EWRAM_DATA static u64 sAI_Scripts[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA static struct Pokemon *sSavedParties = NULL;
EWRAM_DATA static enum Move sPlayerMonMoves[MAX_BATTLERS_COUNT / 2][MAX_MON_MOVES] = {0};
EWRAM_DATA static struct PlayerInfo sPlayers[MAX_LINK_PLAYERS] = {0};
EWRAM_DATA static bool8 sIsPlaybackFinished = 0;
EWRAM_DATA static u8 sRecordMixFriendName[PLAYER_NAME_LENGTH + 1] = {0};
EWRAM_DATA static u8 sRecordMixFriendClass = 0;
EWRAM_DATA static u8 sApprenticeId = 0;
EWRAM_DATA static u16 sEasyChatSpeech[EASY_CHAT_BATTLE_WORDS_COUNT] = {0};
EWRAM_DATA static u8 sBattleOutcome = 0;

static u8 sRecordMixFriendLanguage;
static u8 sApprenticeLanguage;

static u8 GetNextRecordedDataByte(u8 *, u8 *, u8 *);
static bool32 CopyRecordedBattleFromSave(struct RecordedBattleSave *);
static void RecordedBattle_RestoreSavedParties(void);
static void CB2_RecordedBattle(void);

void RecordedBattle_Init(u8 mode)
{
    s32 i, j;

    sRecordMode = mode;
    sIsPlaybackFinished = FALSE;

    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        sBattlerRecordSizes[i] = 0;
        sBattlerPrevRecordSizes[i] = 0;
        sBattlerSavedRecordSizes[i] = 0;

        if (mode == B_RECORD_MODE_RECORDING)
        {
            for (j = 0; j < BATTLER_RECORD_SIZE; j++)
                sBattleRecords[i][j] = 0xFF;
            sBattleFlags = gBattleTypeFlags;
            sAI_Scripts[i] = gAiThinkingStruct->aiFlags[i];
        }
    }
}

void RecordedBattle_SetTrainerInfo(void)
{
    s32 i, j;

    if (sRecordMode == B_RECORD_MODE_RECORDING)
    {
        gRecordedBattleRngSeed = gRngValue;
        sFrontierFacility = VarGet(VAR_FRONTIER_FACILITY);
        sFrontierBrainSymbol = GetFronterBrainSymbol();
    }
    else if (sRecordMode == B_RECORD_MODE_PLAYBACK)
    {
        gRngValue = gRecordedBattleRngSeed;
    }

    if (gBattleTypeFlags & BATTLE_TYPE_LINK)
    {
        // Link recorded battle, record info for all trainers
        u8 linkPlayersCount;
        u8 text[30];

        gRecordedBattleMultiplayerId = GetMultiplayerId();
        linkPlayersCount = GetLinkPlayerCount();

        for (i = 0; i < MAX_LINK_PLAYERS; i++)
        {
            sPlayers[i].trainerId = gLinkPlayers[i].trainerId;
            sPlayers[i].gender = gLinkPlayers[i].gender;
            sPlayers[i].battler = gLinkPlayers[i].id;
            sPlayers[i].language = gLinkPlayers[i].language;

            // Record names
            if (i < linkPlayersCount)
            {
                StringCopy(text, gLinkPlayers[i].name);
                StripExtCtrlCodes(text);
                StringCopy(sPlayers[i].name, text);
            }
            else
            {
                for (j = 0; j < PLAYER_NAME_LENGTH + 1; j++)
                    sPlayers[i].name[j] = gLinkPlayers[i].name[j];
            }
        }
    }
    else
    {
        // Local battle, just record own info
        sPlayers[0].trainerId = READ_OTID_FROM_SAVE;

        sPlayers[0].gender = gSaveBlock2Ptr->playerGender;
        sPlayers[0].battler = 0;
        sPlayers[0].language = gGameLanguage;

        for (i = 0; i < PLAYER_NAME_LENGTH + 1; i++)
            sPlayers[0].name[i] = gSaveBlock2Ptr->playerName[i];
    }
}

void RecordedBattle_SetBattlerAction(enum BattlerId battler, u8 action)
{
    if ((u32)battler >= MAX_BATTLERS_COUNT) { ADMG_StateAccessRejected((u32)battler, __func__); return; }

    if (sBattlerRecordSizes[battler] < BATTLER_RECORD_SIZE && sRecordMode != B_RECORD_MODE_PLAYBACK)
        sBattleRecords[battler][sBattlerRecordSizes[battler]++] = action;
}

void RecordedBattle_ClearBattlerAction(enum BattlerId battler, u8 bytesToClear)
{
    if ((u32)battler >= MAX_BATTLERS_COUNT) { ADMG_StateAccessRejected((u32)battler, __func__); return; }

    if (sBattlerRecordSizes[battler] > BATTLER_RECORD_SIZE)
    { ADMG_StateAccessRejected(sBattlerRecordSizes[battler], __func__); return; }
    if (sBattlerRecordSizes[battler] == 0) return;
    s32 i;

    for (i = 0; i < bytesToClear; i++)
    {
        sBattlerRecordSizes[battler]--;
        sBattleRecords[battler][sBattlerRecordSizes[battler]] = 0xFF;
        if (sBattlerRecordSizes[battler] == 0)
            break;
    }
}

u8 RecordedBattle_GetBattlerAction(u32 actionType, enum BattlerId battler)
{
    if ((u32)battler >= MAX_BATTLERS_COUNT) { ADMG_StateAccessRejected((u32)battler, __func__); return B_ACTION_NONE; }

    if (gTestRunnerEnabled)
        TestRunner_Battle_CheckBattleRecordActionType(battler, sBattlerRecordSizes[battler], actionType);

    // Trying to read past array or invalid action byte, battle is over.
    if (sBattlerRecordSizes[battler] >= BATTLER_RECORD_SIZE || sBattleRecords[battler][sBattlerRecordSizes[battler]] == 0xFF)
    {
        gSpecialVar_Result = gBattleOutcome = B_OUTCOME_PLAYER_TELEPORTED; // hah
        ResetPaletteFadeControl();
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        SetMainCallback2(CB2_QuitRecordedBattle);
        return B_ACTION_NONE;
    }
    else
    {
        return sBattleRecords[battler][sBattlerRecordSizes[battler]++];
    }
}

static bool8 ADMG_RecordedActionPositionsValid(const u16 *positions)
{
    for (u32 battler = 0; battler < MAX_BATTLERS_COUNT; ++battler)
        if (sBattlerRecordSizes[battler] > BATTLER_RECORD_SIZE
         || positions[battler] > sBattlerRecordSizes[battler])
        { ADMG_StateAccessRejected(battler, __func__); return FALSE; }
    return TRUE;
}

/* The original transfer buffer is 256 bytes. Both original envelopes fit
 * when each payload is at most 250 bytes; all cursors stay full width. */
static u32 ADMG_PackRecordedActionChunk(u8 *dst, u16 *positions)
{
    u32 length = 0;
    for (u32 battler = 0; battler < MAX_BATTLERS_COUNT; ++battler)
    {
        u32 pending = sBattlerRecordSizes[battler] - positions[battler];
        if (pending == 0 || length + 2 >= ADMG_RECORDED_ACTION_PAYLOAD) continue;
        u32 count = ADMG_RECORDED_ACTION_PAYLOAD - length - 2;
        if (count > pending) count = pending;
        if (dst != NULL)
        {
            dst[length] = battler;
            dst[length + 1] = count;
            for (u32 action = 0; action < count; ++action)
                dst[length + 2 + action] = sBattleRecords[battler][positions[battler] + action];
        }
        positions[battler] += count;
        length += count + 2;
    }
    return length;
}

bool8 ADMG_TryPlanRecordedActionChunks(u16 *sizes, u32 capacity, u32 *count)
{
    if (sizes == NULL || count == NULL || capacity < ADMG_RECORDED_ACTION_CHUNKS)
    { ADMG_StateAccessRejected(capacity, __func__); return FALSE; }
    u16 positions[MAX_BATTLERS_COUNT];
    memcpy(positions, sBattlerPrevRecordSizes, sizeof(positions));
    if (!ADMG_RecordedActionPositionsValid(positions)) return FALSE;
    u32 chunks = 0, length;
    while ((length = ADMG_PackRecordedActionChunk(NULL, positions)) != 0)
    {
        if (chunks >= capacity) { ADMG_StateAccessRejected(chunks, __func__); return FALSE; }
        sizes[chunks++] = length;
    }
    if (chunks == 0) sizes[chunks++] = 0;
    *count = chunks;
    return TRUE;
}

u8 RecordedBattle_BufferNewBattlerData(u8 *dst)
{
    if (dst == NULL) { ADMG_StateAccessRejected(0, __func__); return 0; }
    u16 positions[MAX_BATTLERS_COUNT];
    memcpy(positions, sBattlerPrevRecordSizes, sizeof(positions));
    if (!ADMG_RecordedActionPositionsValid(positions)) return 0;
    u32 length = ADMG_PackRecordedActionChunk(dst, positions);
    memcpy(sBattlerPrevRecordSizes, positions, sizeof(positions));
    return length;
}

void RecordedBattle_RecordAllBattlerData(u8 *src)
{
    if (!(gBattleTypeFlags & BATTLE_TYPE_LINK)) return;
    u32 players = GetLinkPlayerCount();
    if (src == NULL || players > MAX_LINK_PLAYERS)
    { ADMG_StateAccessRejected(players, __func__); return; }
    for (u32 player = 0; player < players; ++player)
        if ((gLinkPlayers[player].version & 0xFF) != VERSION_EMERALD) return;
    if (gBattleTypeFlags & BATTLE_TYPE_IS_MASTER) return;

    /* The established byte-length envelope is retained. Validate all actor,
     * group and aggregate append bounds before writing any recorded state. */
    u32 length = src[0], cursor = 2, remaining = length;
    u16 projected[MAX_BATTLERS_COUNT];
    u8 snapshot[257];
    memcpy(snapshot, src, length + 2);
    for (u32 battler = 0; battler < MAX_BATTLERS_COUNT; ++battler)
    {
        projected[battler] = sBattlerSavedRecordSizes[battler];
        if (projected[battler] > BATTLER_RECORD_SIZE)
        { ADMG_StateAccessRejected(projected[battler], __func__); return; }
    }
    while (remaining != 0)
    {
        if (remaining < 2) { ADMG_StateAccessRejected(remaining, __func__); return; }
        u32 battler = snapshot[cursor++], count = snapshot[cursor++];
        remaining -= 2;
        if (battler >= MAX_BATTLERS_COUNT || count > remaining
         || count > BATTLER_RECORD_SIZE - projected[battler])
        { ADMG_StateAccessRejected(battler, __func__); return; }
        projected[battler] += count;
        cursor += count;
        remaining -= count;
    }
    cursor = 2; remaining = length;
    while (remaining != 0)
    {
        u32 battler = snapshot[cursor++], count = snapshot[cursor++];
        remaining -= 2;
        for (u32 action = 0; action < count; ++action)
        {
            u8 value;
            if (cursor <= 255)
            {
                /* Preserve the named byte-reader hook on its original domain;
                 * the full envelope cursor itself must not narrow or wrap. */
                u8 byteCursor = cursor, byteRemaining = remaining;
                value = GetNextRecordedDataByte(snapshot, &byteCursor, &byteRemaining);
            }
            else value = snapshot[cursor];
            sBattleRecords[battler][sBattlerSavedRecordSizes[battler]++] = value;
            ++cursor; --remaining;
        }
    }
}

static u8 GetNextRecordedDataByte(u8 *data, u8 *idx, u8 *size)
{
    if (data == NULL || idx == NULL || size == NULL || *size == 0)
    { ADMG_StateAccessRejected(0, __func__); return 0; }

    (*size)--;
    return data[(*idx)++];
}

bool32 CanCopyRecordedBattleSaveData(void)
{
    struct RecordedBattleSave *dst = AllocZeroed(sizeof(struct RecordedBattleSave));
    bool32 ret = CopyRecordedBattleFromSave(dst);
    Free(dst);
    return ret;
}


static bool8 RecordedTrainerActorsValid(const struct RecordedBattleSave *save)
{
    if (save == NULL || !ADMG_TryValidateBattleTrainerActors(save->battleFlags,
            save->opponentA, save->opponentB, save->partnerId)) return FALSE;
    if ((ADMG_IsBattleFacilityActor(save->battleFlags, ADMG_ACTOR_OPPONENT, save->opponentA, ADMG_ACTOR_APPRENTICE)
      || ADMG_IsBattleFacilityActor(save->battleFlags,
            save->battleFlags & BATTLE_TYPE_TWO_OPPONENTS && save->opponentB != 0xFFFF ? ADMG_ACTOR_OPPONENT : ADMG_ACTOR_UNUSED,
            save->opponentB, ADMG_ACTOR_APPRENTICE)
      || ADMG_IsBattleFacilityActor(save->battleFlags, ADMG_ACTOR_PARTNER, save->partnerId, ADMG_ACTOR_APPRENTICE))
     && save->apprenticeId >= NUM_APPRENTICES)
    { ADMG_StateAccessRejected(save->apprenticeId, __func__); return FALSE; }
    if ((ADMG_IsBattleFacilityActor(save->battleFlags, ADMG_ACTOR_OPPONENT, save->opponentA, ADMG_ACTOR_RECORD_FRIEND)
      || ADMG_IsBattleFacilityActor(save->battleFlags,
            save->battleFlags & BATTLE_TYPE_TWO_OPPONENTS && save->opponentB != 0xFFFF ? ADMG_ACTOR_OPPONENT : ADMG_ACTOR_UNUSED,
            save->opponentB, ADMG_ACTOR_RECORD_FRIEND)
      || ADMG_IsBattleFacilityActor(save->battleFlags, ADMG_ACTOR_PARTNER, save->partnerId, ADMG_ACTOR_RECORD_FRIEND))
     && save->recordMixFriendClass >= FACILITY_CLASSES_COUNT)
    { ADMG_StateAccessRejected(save->recordMixFriendClass, __func__); return FALSE; }
    return TRUE;
}

static bool32 IsRecordedBattleSaveValid(struct RecordedBattleSave *save)
{
    if (!RecordedTrainerActorsValid(save)) return FALSE;

    if (save->battleFlags == 0)
        return FALSE;
    if (save->battleFlags & BATTLE_TYPE_RECORDED_INVALID)
        return FALSE;
    if (CalcByteArraySum((void *)(save), sizeof(*save) - 4) != save->checksum)
        return FALSE;

    return TRUE;
}

static bool32 RecordedBattleToSave(struct RecordedBattleSave *battleSave, struct RecordedBattleSave *saveSector)
{
    if (battleSave == NULL || saveSector == NULL || !RecordedTrainerActorsValid(battleSave)) return FALSE;

    memset(saveSector, 0, sizeof(*saveSector));
    memcpy(saveSector, battleSave, sizeof(*battleSave));

    saveSector->checksum = CalcByteArraySum((void *)(saveSector), sizeof(*saveSector) - 4);

    if (TryWriteSpecialSaveSector(SECTOR_ID_RECORDED_BATTLE, (void *)(saveSector), sizeof(*saveSector)) != SAVE_STATUS_OK)
        return FALSE;
    else
        return TRUE;
}

bool32 MoveRecordedBattleToSaveData(void)
{
    if (gSaveBlock2Ptr == NULL || !ADMG_TryValidateBattleTrainerActors(sBattleFlags,
            TRAINER_BATTLE_PARAM.opponentA, TRAINER_BATTLE_PARAM.opponentB, gPartnerTrainerId)) return FALSE;

    /* Validate original bit-packed actor transport before narrowing/indexing. */
    if (gRecordedBattleMultiplayerId >= MAX_LINK_PLAYERS)
    { ADMG_StateAccessRejected(gRecordedBattleMultiplayerId, __func__); return FALSE; }
    for (u32 player = 0; player < MAX_LINK_PLAYERS; ++player)
        if (sPlayers[player].battler >= MAX_BATTLERS_COUNT)
        { ADMG_StateAccessRejected(sPlayers[player].battler, __func__); return FALSE; }

    s32 i, j;
    bool32 ret;
    struct RecordedBattleSave *battleSave, *savSection;
    u8 saveAttempts;

    saveAttempts = 0;
    battleSave = AllocZeroed(sizeof(struct RecordedBattleSave));
    savSection = AllocZeroed(sizeof(struct RecordedBattleSave));
    if (battleSave == NULL || savSection == NULL)
    {
        if (battleSave != NULL) Free(battleSave);
        if (savSection != NULL) Free(savSection);
        ADMG_StateAccessRejected(0, __func__);
        return FALSE;
    }


    memcpy(battleSave->parties, sSavedParties, sizeof(struct Pokemon[MAX_BATTLE_TRAINERS][PARTY_SIZE]));

    battleSave->playersGender = 0;

    for (i = 0; i < MAX_LINK_PLAYERS; i++)
    {
        for (j = 0; j < PLAYER_NAME_LENGTH + 1; j++)
            battleSave->playersName[i][j] = sPlayers[i].name[j];
        if (sPlayers[i].gender)
            battleSave->playersGender |= (1 << i);
        battleSave->playersLanguage[i] = sPlayers[i].language;
        // Player 2 battler and player 0 second battler, and player 3 battler and player 1 second battler can occupy the same 2 bits
        battleSave->playersBattlers |= (sPlayers[i].battler << (2 * (i >> 1) + 4 * (i & BIT_SIDE)));
        battleSave->playersTrainerId[i] = sPlayers[i].trainerId;
        battleSave->AI_scripts[i] = sAI_Scripts[i];
    }

    battleSave->rngSeed = gRecordedBattleRngSeed;

    if (sBattleFlags & BATTLE_TYPE_LINK)
    {
        battleSave->battleFlags = (sBattleFlags & ~(BATTLE_TYPE_LINK | BATTLE_TYPE_LINK_IN_BATTLE)) | BATTLE_TYPE_RECORDED_LINK;

        // BATTLE_TYPE_RECORDED_IS_MASTER set indicates battle will play
        // out from player's perspective (i.e. player with back to camera)
        // Otherwise player will appear on "opponent" side
        if (sBattleFlags & BATTLE_TYPE_IS_MASTER)
        {
            battleSave->battleFlags |= BATTLE_TYPE_RECORDED_IS_MASTER;
        }
        else if (sBattleFlags & BATTLE_TYPE_MULTI)
        {
            switch (sPlayers[0].battler)
            {
            case 0:
            case 2:
                if (!(sPlayers[gRecordedBattleMultiplayerId].battler & 1))
                    battleSave->battleFlags |= BATTLE_TYPE_RECORDED_IS_MASTER;
                break;
            case 1:
            case 3:
                if ((sPlayers[gRecordedBattleMultiplayerId].battler & 1))
                    battleSave->battleFlags |= BATTLE_TYPE_RECORDED_IS_MASTER;
                break;
            }
        }
    }
    else
    {
        battleSave->battleFlags = sBattleFlags;
    }

    battleSave->opponentA = TRAINER_BATTLE_PARAM.opponentA;
    battleSave->opponentB = TRAINER_BATTLE_PARAM.opponentB;
    battleSave->partnerId = gPartnerTrainerId;
    battleSave->multiplayerId = gRecordedBattleMultiplayerId;
    battleSave->lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    battleSave->frontierFacility = sFrontierFacility;
    battleSave->frontierBrainSymbol = sFrontierBrainSymbol;
    battleSave->battleScene = gSaveBlock2Ptr->optionsBattleSceneOff;
    battleSave->textSpeed = gSaveBlock2Ptr->optionsTextSpeed;

    if (ADMG_IsBattleFacilityActor(sBattleFlags, ADMG_ACTOR_OPPONENT, TRAINER_BATTLE_PARAM.opponentA, ADMG_ACTOR_RECORD_FRIEND))
    {
        for (i = 0; i < PLAYER_NAME_LENGTH + 1; i++)
            battleSave->recordMixFriendName[i] = gSaveBlock2Ptr->frontier.towerRecords[TRAINER_BATTLE_PARAM.opponentA - TRAINER_RECORD_MIXING_FRIEND].name[i];
        battleSave->recordMixFriendClass = gSaveBlock2Ptr->frontier.towerRecords[TRAINER_BATTLE_PARAM.opponentA - TRAINER_RECORD_MIXING_FRIEND].facilityClass;

        if (sBattleOutcome == B_OUTCOME_WON)
        {
            for (i = 0; i < EASY_CHAT_BATTLE_WORDS_COUNT; i++)
                battleSave->easyChatSpeech[i] = gSaveBlock2Ptr->frontier.towerRecords[TRAINER_BATTLE_PARAM.opponentA - TRAINER_RECORD_MIXING_FRIEND].speechLost[i];
        }
        else
        {
            for (i = 0; i < EASY_CHAT_BATTLE_WORDS_COUNT; i++)
                battleSave->easyChatSpeech[i] = gSaveBlock2Ptr->frontier.towerRecords[TRAINER_BATTLE_PARAM.opponentA - TRAINER_RECORD_MIXING_FRIEND].speechWon[i];
        }
        battleSave->recordMixFriendLanguage = gSaveBlock2Ptr->frontier.towerRecords[TRAINER_BATTLE_PARAM.opponentA - TRAINER_RECORD_MIXING_FRIEND].language;
    }
    else if (ADMG_IsBattleFacilityActor(sBattleFlags, (sBattleFlags & BATTLE_TYPE_TWO_OPPONENTS && TRAINER_BATTLE_PARAM.opponentB != 0xFFFF ? ADMG_ACTOR_OPPONENT : ADMG_ACTOR_UNUSED), TRAINER_BATTLE_PARAM.opponentB, ADMG_ACTOR_RECORD_FRIEND))
    {
        for (i = 0; i < PLAYER_NAME_LENGTH + 1; i++)
            battleSave->recordMixFriendName[i] = gSaveBlock2Ptr->frontier.towerRecords[TRAINER_BATTLE_PARAM.opponentB - TRAINER_RECORD_MIXING_FRIEND].name[i];
        battleSave->recordMixFriendClass = gSaveBlock2Ptr->frontier.towerRecords[TRAINER_BATTLE_PARAM.opponentB - TRAINER_RECORD_MIXING_FRIEND].facilityClass;

        if (sBattleOutcome == B_OUTCOME_WON)
        {
            for (i = 0; i < EASY_CHAT_BATTLE_WORDS_COUNT; i++)
                battleSave->easyChatSpeech[i] = gSaveBlock2Ptr->frontier.towerRecords[TRAINER_BATTLE_PARAM.opponentB - TRAINER_RECORD_MIXING_FRIEND].speechLost[i];
        }
        else
        {
            for (i = 0; i < EASY_CHAT_BATTLE_WORDS_COUNT; i++)
                battleSave->easyChatSpeech[i] = gSaveBlock2Ptr->frontier.towerRecords[TRAINER_BATTLE_PARAM.opponentB - TRAINER_RECORD_MIXING_FRIEND].speechWon[i];
        }
        battleSave->recordMixFriendLanguage = gSaveBlock2Ptr->frontier.towerRecords[TRAINER_BATTLE_PARAM.opponentB - TRAINER_RECORD_MIXING_FRIEND].language;
    }
    else if (ADMG_IsBattleFacilityActor(sBattleFlags, ADMG_ACTOR_PARTNER, gPartnerTrainerId, ADMG_ACTOR_RECORD_FRIEND))
    {
        for (i = 0; i < PLAYER_NAME_LENGTH + 1; i++)
            battleSave->recordMixFriendName[i] = gSaveBlock2Ptr->frontier.towerRecords[gPartnerTrainerId - TRAINER_RECORD_MIXING_FRIEND].name[i];
        battleSave->recordMixFriendClass = gSaveBlock2Ptr->frontier.towerRecords[gPartnerTrainerId - TRAINER_RECORD_MIXING_FRIEND].facilityClass;

        battleSave->recordMixFriendLanguage = gSaveBlock2Ptr->frontier.towerRecords[gPartnerTrainerId - TRAINER_RECORD_MIXING_FRIEND].language;
    }

    if (ADMG_IsBattleFacilityActor(sBattleFlags, ADMG_ACTOR_OPPONENT, TRAINER_BATTLE_PARAM.opponentA, ADMG_ACTOR_APPRENTICE))
    {
        battleSave->apprenticeId = gSaveBlock2Ptr->apprentices[TRAINER_BATTLE_PARAM.opponentA - TRAINER_RECORD_MIXING_APPRENTICE].id;
        for (i = 0; i < EASY_CHAT_BATTLE_WORDS_COUNT; i++)
            battleSave->easyChatSpeech[i] = gSaveBlock2Ptr->apprentices[TRAINER_BATTLE_PARAM.opponentA - TRAINER_RECORD_MIXING_APPRENTICE].speechWon[i];
        battleSave->apprenticeLanguage = gSaveBlock2Ptr->apprentices[TRAINER_BATTLE_PARAM.opponentA - TRAINER_RECORD_MIXING_APPRENTICE].language;
    }
    else if (ADMG_IsBattleFacilityActor(sBattleFlags, (sBattleFlags & BATTLE_TYPE_TWO_OPPONENTS && TRAINER_BATTLE_PARAM.opponentB != 0xFFFF ? ADMG_ACTOR_OPPONENT : ADMG_ACTOR_UNUSED), TRAINER_BATTLE_PARAM.opponentB, ADMG_ACTOR_APPRENTICE))
    {
        battleSave->apprenticeId = gSaveBlock2Ptr->apprentices[TRAINER_BATTLE_PARAM.opponentB - TRAINER_RECORD_MIXING_APPRENTICE].id;
        for (i = 0; i < EASY_CHAT_BATTLE_WORDS_COUNT; i++)
            battleSave->easyChatSpeech[i] = gSaveBlock2Ptr->apprentices[TRAINER_BATTLE_PARAM.opponentB - TRAINER_RECORD_MIXING_APPRENTICE].speechWon[i];
        battleSave->apprenticeLanguage = gSaveBlock2Ptr->apprentices[TRAINER_BATTLE_PARAM.opponentB - TRAINER_RECORD_MIXING_APPRENTICE].language;
    }
    else if (ADMG_IsBattleFacilityActor(sBattleFlags, ADMG_ACTOR_PARTNER, gPartnerTrainerId, ADMG_ACTOR_APPRENTICE))
    {
        battleSave->apprenticeId = gSaveBlock2Ptr->apprentices[gPartnerTrainerId - TRAINER_RECORD_MIXING_APPRENTICE].id;

        battleSave->apprenticeLanguage = gSaveBlock2Ptr->apprentices[gPartnerTrainerId - TRAINER_RECORD_MIXING_APPRENTICE].language;
    }

    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
        for (j = 0; j < BATTLER_RECORD_SIZE; j++)
            battleSave->battleRecord[i][j] = sBattleRecords[i][j];

    if (!RecordedTrainerActorsValid(battleSave))
    { Free(battleSave); Free(savSection); return FALSE; }

    while (1)
    {
        ret = RecordedBattleToSave(battleSave, savSection);
        if (ret == TRUE)
            break;
        saveAttempts++;
        if (saveAttempts >= 3)
            break;
    }

    Free(battleSave);
    Free(savSection);
    return ret;
}

static bool32 TryCopyRecordedBattleSaveData(struct RecordedBattleSave *dst, struct RecordedBattleSave *saveBuffer)
{
    if (dst == NULL || saveBuffer == NULL) { ADMG_StateAccessRejected(0, __func__); return FALSE; }

    if (TryReadSpecialSaveSector(SECTOR_ID_RECORDED_BATTLE, (void *)(saveBuffer), sizeof(*saveBuffer)) != SAVE_STATUS_OK)
        return FALSE;

    memcpy(dst, saveBuffer, sizeof(struct RecordedBattleSave));

    if (!IsRecordedBattleSaveValid(dst))
        return FALSE;

    return TRUE;
}

static bool32 CopyRecordedBattleFromSave(struct RecordedBattleSave *dst)
{
    if (dst == NULL) { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    struct RecordedBattleSave *savBuffer = AllocZeroed(sizeof(*savBuffer));
    if (savBuffer == NULL) { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    bool32 ret = TryCopyRecordedBattleSaveData(dst, savBuffer);
    Free(savBuffer);

    return ret;
}

static void CB2_RecordedBattleEnd(void)
{
    gSaveBlock2Ptr->frontier.lvlMode = sLvlMode;
    gBattleOutcome = 0;
    gBattleTypeFlags = 0;
    TRAINER_BATTLE_PARAM.opponentA = 0;
    TRAINER_BATTLE_PARAM.opponentB = 0;
    gPartnerTrainerId = 0;

    RecordedBattle_RestoreSavedParties();
    SetMainCallback2(sCallback2_AfterRecordedBattle);
}

#define tFramesToWait data[0]

static void Task_StartAfterCountdown(u8 taskId)
{
    if (--gTasks[taskId].tFramesToWait == 0)
    {
        gMain.savedCallback = CB2_RecordedBattleEnd;
        SetMainCallback2(CB2_InitBattle);
        DestroyTask(taskId);
    }
}

void SetPartiesFromRecordedSave(struct RecordedBattleSave *src)
{
    if (!RecordedTrainerActorsValid(src)) return;

    for (enum BattleTrainer trainer = B_TRAINER_PLAYER; trainer < MAX_BATTLE_TRAINERS; trainer++)
    {
        ZeroPartyMons(gParties[trainer]);
        for (s32 i = 0; i < PARTY_SIZE; i++)
            gParties[trainer][i] = src->parties[trainer][i];
    }
}

void SetVariablesForRecordedBattle(struct RecordedBattleSave *src)
{
    if (gSaveBlock2Ptr == NULL || !RecordedTrainerActorsValid(src)) return;

    bool8 var;
    s32 i, j;

    u64 admission = ADMG_BeginStateAccessCheck();
    SetPartiesFromRecordedSave(src);
    if (!ADMG_EndStateAccessCheck(admission)) return;
    for (i = 0; i < MAX_LINK_PLAYERS; i++)
    {
        for (var = FALSE, j = 0; j < PLAYER_NAME_LENGTH + 1; j++)
        {
            gLinkPlayers[i].name[j] = src->playersName[i][j];
            if (src->playersName[i][j] == EOS)
                var = TRUE;
        }
        gLinkPlayers[i].gender = (src->playersGender >> i) & 1;
        gLinkPlayers[i].language = src->playersLanguage[i];
        gLinkPlayers[i].id = (src->playersBattlers >> (2 * (i >> 1) + 4 * (i & BIT_SIDE))) & 0x3;
        gLinkPlayers[i].trainerId = src->playersTrainerId[i];
        sAI_Scripts[i] = src->AI_scripts[i];

        if (var)
            ConvertInternationalString(gLinkPlayers[i].name, gLinkPlayers[i].language);
    }

    gRecordedBattleRngSeed = src->rngSeed;
    gBattleTypeFlags = src->battleFlags | BATTLE_TYPE_RECORDED;
    TRAINER_BATTLE_PARAM.opponentA = src->opponentA;
    TRAINER_BATTLE_PARAM.opponentB = src->opponentB;
    gPartnerTrainerId = src->partnerId;
    gRecordedBattleMultiplayerId = src->multiplayerId;
    sLvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    sFrontierFacility = src->frontierFacility;
    sFrontierBrainSymbol = src->frontierBrainSymbol;
    sBattleScene = src->battleScene;
    sTextSpeed = src->textSpeed;

    for (i = 0; i < PLAYER_NAME_LENGTH + 1; i++)
        sRecordMixFriendName[i] = src->recordMixFriendName[i];

    sRecordMixFriendClass = src->recordMixFriendClass;
    sApprenticeId = src->apprenticeId;
    sRecordMixFriendLanguage = src->recordMixFriendLanguage;
    sApprenticeLanguage = src->apprenticeLanguage;

    for (i = 0; i < EASY_CHAT_BATTLE_WORDS_COUNT; i++)
        sEasyChatSpeech[i] = src->easyChatSpeech[i];

    gSaveBlock2Ptr->frontier.lvlMode = src->lvlMode;

    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
        for (j = 0; j < BATTLER_RECORD_SIZE; j++)
            sBattleRecords[i][j] = src->battleRecord[i][j];
}

void PlayRecordedBattle(void (*CB2_After)(void))
{
    if (gSaveBlock2Ptr == NULL) { ADMG_StateAccessRejected(0, __func__); return; }
    struct RecordedBattleSave *battleSave = AllocZeroed(sizeof(struct RecordedBattleSave));
    if (battleSave == NULL) { ADMG_StateAccessRejected(0, __func__); return; }
    if (CopyRecordedBattleFromSave(battleSave) == TRUE)
    {
        u8 taskId;

        u64 admission = ADMG_BeginStateAccessCheck();
        RecordedBattle_SaveParties();
        if (!ADMG_EndStateAccessCheck(admission)) { Free(battleSave); return; }
        SetVariablesForRecordedBattle(battleSave);
        if (!ADMG_EndStateAccessCheck(admission))
        { RecordedBattle_RestoreSavedParties(); Free(battleSave); return; }

        taskId = CreateTask(Task_StartAfterCountdown, 1);
        if (taskId >= NUM_TASKS)
        { ADMG_StateAccessRejected(taskId, __func__); RecordedBattle_RestoreSavedParties(); Free(battleSave); return; }
        gTasks[taskId].tFramesToWait = 128;

        sCallback2_AfterRecordedBattle = CB2_After;
        PlayMapChosenOrBattleBGM(FALSE);
        SetMainCallback2(CB2_RecordedBattle);
    }
    Free(battleSave);
}

#undef tFramesToWait

static void CB2_RecordedBattle(void)
{
    AnimateSprites();
    BuildOamBuffer();
    RunTasks();
}

u8 GetRecordedBattleFrontierFacility(void)
{
    return sFrontierFacility;
}

u8 GetRecordedBattleFronterBrainSymbol(void)
{
    return sFrontierBrainSymbol;
}

void RecordedBattle_SaveParties(void)
{
    sSavedParties = AllocZeroed(sizeof(struct Pokemon[MAX_BATTLE_TRAINERS][PARTY_SIZE]));
    if (sSavedParties == NULL) { ADMG_StateAccessRejected(0, __func__); return; }

    memcpy(sSavedParties, gParties, sizeof(struct Pokemon[MAX_BATTLE_TRAINERS][PARTY_SIZE]));
}

static void RecordedBattle_RestoreSavedParties(void)
{
    memcpy(gParties, sSavedParties, sizeof(struct Pokemon[MAX_BATTLE_TRAINERS][PARTY_SIZE]));

    Free(sSavedParties);
}

u8 GetBattlerLinkPlayerGender(enum BattlerId battler)
{
    s32 i;

    for (i = 0; i < MAX_LINK_PLAYERS; i++)
    {
        if (gLinkPlayers[i].id == battler)
            break;
    }

    if (i != MAX_LINK_PLAYERS)
        return gLinkPlayers[i].gender;

    return 0;
}

void RecordedBattle_ClearFrontierPassFlag(void)
{
    sFrontierPassFlag = 0;
}

// Set sFrontierPassFlag to received state of FLAG_SYS_FRONTIER_PASS
void RecordedBattle_SetFrontierPassFlagFromHword(u16 flags)
{
    sFrontierPassFlag |= (flags & (1 << 15)) >> 15;
}

u8 RecordedBattle_GetFrontierPassFlag(void)
{
    return sFrontierPassFlag;
}

u8 GetBattleSceneInRecordedBattle(void)
{
    return sBattleScene;
}

u8 GetTextSpeedInRecordedBattle(void)
{
    return sTextSpeed;
}

void RecordedBattle_CopyBattlerMoves(enum BattlerId battler)
{
    if ((u32)battler >= MAX_BATTLERS_COUNT) { ADMG_StateAccessRejected((u32)battler, __func__); return; }

    s32 i;

    if (!IsOnPlayerSide(battler))
        return;
    if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK))
        return;
    if (sRecordMode == B_RECORD_MODE_PLAYBACK)
        return;

    for (i = 0; i < MAX_MON_MOVES; i++)
        sPlayerMonMoves[battler / 2][i] = gBattleMons[battler].moves[i];
}

// This is a special battle action only used by this function
// It shares a value with B_ACTION_SAFARI_POKEBLOCK, which can never occur in a recorded battle.
#define ACTION_MOVE_CHANGE 6

static bool8 ADMG_RecordedMovesetFramesValid(u8 mode)
{
    if (gBattlersCount > MAX_BATTLERS_COUNT
     || (mode != B_RECORD_MODE_RECORDING && mode != B_RECORD_MODE_PLAYBACK))
    { ADMG_StateAccessRejected(mode, __func__); return FALSE; }
    for (u32 battler = 0; battler < gBattlersCount; ++battler)
    {
        if ((u32)GetBattlerPosition(battler) >= (u32)MAX_BATTLERS_COUNT)
        { ADMG_StateAccessRejected(battler, __func__); return FALSE; }
        if (!IsOnPlayerSide(battler)) continue;
        u32 cursor = sBattlerRecordSizes[battler];
        if (cursor > BATTLER_RECORD_SIZE)
        { ADMG_StateAccessRejected(cursor, __func__); return FALSE; }
        if (mode == B_RECORD_MODE_RECORDING)
        {
            /* Recording only writes, through RecordedBattle_SetBattlerAction, which stays inside the record and
             * writes nothing during playback. Nothing here can go out of bounds, so nothing is rejected: in
             * playback the saved moves are never filled (RecordedBattle_CopyBattlerMoves skips it), a move can
             * change to one that was not saved (Mimic, Sketch), and the record can be nearly full; refusing any
             * of these would stop action selection for good. */
            continue;
        }
        else
        {
            /* A consumed record ends in the existing action reader. Do not
             * peek beyond the record while merely checking for a change. */
            if (cursor == BATTLER_RECORD_SIZE || sBattleRecords[battler][cursor] != ACTION_MOVE_CHANGE) continue;
            if (cursor > BATTLER_RECORD_SIZE - (MAX_MON_MOVES + 1))
            { ADMG_StateAccessRejected(cursor, __func__); return FALSE; }
            for (u32 move = 0; move < MAX_MON_MOVES; ++move)
                if (sBattleRecords[battler][cursor + 1 + move] >= MAX_MON_MOVES)
                { ADMG_StateAccessRejected(sBattleRecords[battler][cursor + 1 + move], __func__); return FALSE; }
            if (gBattleStruct == NULL || gBattleStruct->chosenMovePositions[battler] >= MAX_MON_MOVES
             || (!gBattleMons[battler].volatiles.transformed && gBattlerPartyIndexes[battler] >= PARTY_SIZE))
            { ADMG_StateAccessRejected(battler, __func__); return FALSE; }
        }
    }
    return TRUE;
}

void RecordedBattle_CheckMovesetChanges(u8 mode)
{
    if (gBattlersCount > MAX_BATTLERS_COUNT) { ADMG_StateAccessRejected(gBattlersCount, __func__); return; }

    s32 j, k;

    if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK))
        return;
    if (!ADMG_RecordedMovesetFramesValid(mode)) return;

    for (enum BattlerId battler = 0; battler < gBattlersCount; battler++)
    {
        // Player's side only
        if (IsOnPlayerSide(battler))
        {
            if (mode == B_RECORD_MODE_RECORDING)
            {
                // Check if any of the battler's moves have changed.
                for (j = 0; j < MAX_MON_MOVES; j++)
                {
                    if (gBattleMons[battler].moves[j] != sPlayerMonMoves[battler / 2][j])
                        break;
                }
                if (j != MAX_MON_MOVES)
                {
                    // At least one of the moves has been changed
                    RecordedBattle_SetBattlerAction(battler, ACTION_MOVE_CHANGE);
                    for (j = 0; j < MAX_MON_MOVES; j++)
                    {
                        for (k = 0; k < MAX_MON_MOVES; k++)
                        {
                            if (gBattleMons[battler].moves[j] == sPlayerMonMoves[battler / 2][k])
                            {
                                RecordedBattle_SetBattlerAction(battler, k);
                                break;
                            }
                        }
                    }
                }
            }
            else // B_RECORD_MODE_PLAYBACK
            {
                if (sBattlerRecordSizes[battler] < BATTLER_RECORD_SIZE && sBattleRecords[battler][sBattlerRecordSizes[battler]] == ACTION_MOVE_CHANGE)
                {
                    u8 ppBonuses[MAX_MON_MOVES];
                    u8 moveSlots[MAX_MON_MOVES];
                    u8 mimickedMoveSlots[MAX_MON_MOVES];
                    struct ChooseMoveStruct movePP;
                    u8 ppBonusSet;

                    // We know the current action is ACTION_MOVE_CHANGE, retrieve
                    // it without saving it to move on to the next action.
                    RecordedBattle_GetBattlerAction(RECORDED_BYTE, battler);

                    for (j = 0; j < MAX_MON_MOVES; j++)
                        ppBonuses[j] = ((gBattleMons[battler].ppBonuses & (3 << (j << 1))) >> (j << 1));

                    for (j = 0; j < MAX_MON_MOVES; j++)
                    {
                        moveSlots[j] = RecordedBattle_GetBattlerAction(RECORDED_BYTE, battler);
                        if (moveSlots[j] >= MAX_MON_MOVES)
                        { ADMG_StateAccessRejected(moveSlots[j], __func__); return; }
                        movePP.moves[j] = gBattleMons[battler].moves[moveSlots[j]];
                        movePP.currentPP[j] = gBattleMons[battler].pp[moveSlots[j]];
                        movePP.maxPP[j] = ppBonuses[moveSlots[j]];
                        mimickedMoveSlots[j] = (gBattleMons[battler].volatiles.mimickedMoves & (1u << j)) >> j;
                    }
                    for (j = 0; j < MAX_MON_MOVES; j++)
                    {
                        gBattleMons[battler].moves[j] = movePP.moves[j];
                        gBattleMons[battler].pp[j] = movePP.currentPP[j];
                    }
                    gBattleMons[battler].ppBonuses = 0;
                    gBattleMons[battler].volatiles.mimickedMoves = 0;
                    for (j = 0; j < MAX_MON_MOVES; j++)
                    {
                        gBattleMons[battler].ppBonuses |= movePP.maxPP[j] << (j << 1);
                        gBattleMons[battler].volatiles.mimickedMoves |= mimickedMoveSlots[j] << j;
                    }

                    if (!(gBattleMons[battler].volatiles.transformed))
                    {
                        struct Pokemon *mon = GetBattlerMon(battler);
                        for (j = 0; j < MAX_MON_MOVES; j++)
                            ppBonuses[j] = (GetMonData(mon, MON_DATA_PP_BONUSES) & ((3 << (j << 1)))) >> (j << 1);

                        for (j = 0; j < MAX_MON_MOVES; j++)
                        {
                            movePP.moves[j] = GetMonData(mon, MON_DATA_MOVE1 + moveSlots[j]);
                            movePP.currentPP[j] = GetMonData(mon, MON_DATA_PP1 + moveSlots[j]);
                            movePP.maxPP[j] = ppBonuses[moveSlots[j]];
                        }
                        for (j = 0; j < MAX_MON_MOVES; j++)
                        {
                            SetMonData(mon, MON_DATA_MOVE1 + j, &movePP.moves[j]);
                            SetMonData(mon, MON_DATA_PP1 + j, &movePP.currentPP[j]);
                        }
                        ppBonusSet = 0;
                        for (j = 0; j < MAX_MON_MOVES; j++)
                            ppBonusSet |= movePP.maxPP[j] << (j << 1);

                        SetMonData(mon, MON_DATA_PP_BONUSES, &ppBonusSet);
                    }
                    gChosenMoveByBattler[battler] = GetBattlerChosenMove(battler);
                }
            }
        }
    }
}

u64 GetAiScriptsInRecordedBattle(enum BattlerId battler)
{
    if ((u32)battler >= MAX_BATTLERS_COUNT) { ADMG_StateAccessRejected((u32)battler, __func__); return 0; }

    return sAI_Scripts[battler];
}

// Used to determine when the player is allowed to press B to end a recorded battle's playback
void RecordedBattle_SetPlaybackFinished(void)
{
    sIsPlaybackFinished = TRUE;
}

bool8 RecordedBattle_CanStopPlayback(void)
{
    return (sIsPlaybackFinished == FALSE);
}

void GetRecordedBattleRecordMixFriendName(u8 *dst)
{
    s32 i;
    if (dst == NULL) { ADMG_StateAccessRejected(0, __func__); return; }
    if (!ADMG_TryValidateTrainerText(sRecordMixFriendName, sizeof(sRecordMixFriendName), 0, NULL)) return;

    for (i = 0; i < PLAYER_NAME_LENGTH + 1; i++)
        dst[i] = sRecordMixFriendName[i];
    dst[PLAYER_NAME_LENGTH] = EOS;
    ConvertInternationalString(dst, sRecordMixFriendLanguage);
}

u8 GetRecordedBattleRecordMixFriendClass(void)
{
    return sRecordMixFriendClass;
}

u8 GetRecordedBattleApprenticeId(void)
{
    return sApprenticeId;
}

u8 GetRecordedBattleRecordMixFriendLanguage(void)
{
    return sRecordMixFriendLanguage;
}

u8 GetRecordedBattleApprenticeLanguage(void)
{
    return sApprenticeLanguage;
}

void RecordedBattle_SaveBattleOutcome(void)
{
    sBattleOutcome = gBattleOutcome;
}

u16 *GetRecordedBattleEasyChatSpeech(void)
{
    return sEasyChatSpeech;
}
