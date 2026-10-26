#include "global.h"
#include "admg_state.h"
#include "constants/apprentice.h"
#include "main.h"
#include "battle.h"
#include "battle_main.h"
#include "battle_frontier.h"
#include "battle_setup.h"
#include "battle_dome.h"
#include "battle_factory.h"
#include "battle_partner.h"
#include "battle_tower.h"
#include "battle_transition.h"
#include "event_data.h"
#include "frontier_util.h"
#include "overworld.h"
#include "script.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "trainer_util.h"
#include "constants/abilities.h"
#include "constants/battle_frontier.h"
#include "constants/battle_frontier_mons.h"

ADMG_NATIVE_ENTRY void FillTrainerParty(u16 trainerId, enum BattleTrainer trainer, u8 monCount);

// EWRAM vars.
EWRAM_DATA const struct BattleFrontierTrainer *gFacilityTrainers = NULL;
EWRAM_DATA const struct TrainerMon *gFacilityTrainerMons = NULL;

// IWRAM common
COMMON_DATA u16 gFrontierTempParty[MAX_FRONTIER_PARTY_SIZE] = {0};

static void HandleFacilityTrainerBattleEnd(void)
{
    u8 facility = gBattleScripting.specialTrainerBattleType;
    switch (facility)
    {
    case FACILITY_BATTLE_TOWER:
    case FACILITY_BATTLE_DOME:
    case FACILITY_BATTLE_PALACE:
    case FACILITY_BATTLE_ARENA:
    case FACILITY_BATTLE_FACTORY:
    case FACILITY_BATTLE_PIKE_SINGLE:
    case FACILITY_BATTLE_PIKE_DOUBLE:
    case FACILITY_BATTLE_PYRAMID:
        if (gSaveBlock2Ptr->frontier.battlesCount < 0xFFFFFF)
        {
            gSaveBlock2Ptr->frontier.battlesCount++;
            if (gSaveBlock2Ptr->frontier.battlesCount % 20 == 0)
                UpdateGymLeaderRematch();
        }
        else
        {
            gSaveBlock2Ptr->frontier.battlesCount = 0xFFFFFF;
        }
        break;
    case FACILITY_BATTLE_TRAINER_HILL:
    default:
        break;
    }

    SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

ADMG_NATIVE_ENTRY bool8 ADMG_TryAdmitFacilityParty(u32 identity, u32 target, u32 count,
    bool8 tent, u32 firstMon, struct ADMG_FacilityPartyInput *input);

/* This admission is stack-owned and has no persistence representation. */
struct ADMG_FacilityBattleInput
{
    struct SaveBlock2 *save;
    enum FrontierLevelMode levelMode;
    u16 battleMode;
    u16 opponentA, opponentB, partner;
    bool8 useA, useB, usePartner, partnerPublished;
    struct ADMG_FactoryPartyInput factory;
};

static bool8 ADMG_FacilityBattleContextCurrent(const struct ADMG_FacilityBattleInput *input,
    u64 generation, u32 facility)
{
    if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (gSaveBlock2Ptr != input->save || input->save->frontier.lvlMode != input->levelMode)
    { ADMG_StateAccessRejected(facility, __func__); return FALSE; }
    if (input->useA && TRAINER_BATTLE_PARAM.opponentA != input->opponentA)
    { ADMG_StateAccessRejected(TRAINER_BATTLE_PARAM.opponentA, __func__); return FALSE; }
    if (input->useB && TRAINER_BATTLE_PARAM.opponentB != input->opponentB)
    { ADMG_StateAccessRejected(TRAINER_BATTLE_PARAM.opponentB, __func__); return FALSE; }
    if (input->usePartner && input->save->frontier.trainerIds[17] != input->partner)
    { ADMG_StateAccessRejected(input->save->frontier.trainerIds[17], __func__); return FALSE; }
    if (input->usePartner && input->partnerPublished && gPartnerTrainerId != input->partner)
    { ADMG_StateAccessRejected(gPartnerTrainerId, __func__); return FALSE; }
    return TRUE;
}

static bool8 ADMG_TryAdmitFacilityBattle(u32 facility, struct ADMG_FacilityBattleInput *input)
{
    if (facility >= FACILITY_BATTLE_TRAINER_HILL) return TRUE;
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    if (save == NULL) { ADMG_StateAccessRejected(facility, __func__); return FALSE; }
    input->save = save;
    input->levelMode = save->frontier.lvlMode;
    u64 generation = ADMG_BeginStateAccessCheck();
    input->battleMode = FRONTIER_MODE_SINGLES;
    if (facility == FACILITY_BATTLE_TOWER || facility == FACILITY_BATTLE_DOME
     || facility == FACILITY_BATTLE_PALACE || facility == FACILITY_BATTLE_FACTORY)
    {
        input->battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
        if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
        if (input->battleMode >= FRONTIER_MODE_COUNT)
        { ADMG_StateAccessRejected(input->battleMode, __func__); return FALSE; }
    }
    if (gSaveBlock2Ptr != save || save->frontier.lvlMode != input->levelMode)
    { ADMG_StateAccessRejected(facility, __func__); return FALSE; }
    u32 a = TRAINER_BATTLE_PARAM.opponentA, b = TRAINER_BATTLE_PARAM.opponentB;
    input->opponentA = a; input->opponentB = b;
    input->useA = !(facility == FACILITY_BATTLE_FACTORY && input->levelMode == FRONTIER_LVL_TENT);
    input->useB = facility == FACILITY_BATTLE_PIKE_DOUBLE
        || (facility == FACILITY_BATTLE_TOWER && (input->battleMode == FRONTIER_MODE_MULTIS
            || input->battleMode == FRONTIER_MODE_LINK_MULTIS));
    input->usePartner = facility == FACILITY_BATTLE_TOWER && input->battleMode == FRONTIER_MODE_MULTIS;
    input->partner = input->usePartner ? save->frontier.trainerIds[17] : 0;
    input->partnerPublished = FALSE;
    u32 count = FRONTIER_PARTY_SIZE;
    bool8 second = FALSE;
    bool8 tent = FALSE;
    struct ADMG_FacilityPartyInput admitted;
    if (facility == FACILITY_BATTLE_FACTORY)
    {
        if (!ADMG_TryPrepareFactoryParty(a, input->levelMode == FRONTIER_LVL_TENT,
            0, &input->factory)) return FALSE;
    }
    else
    {
        if (facility == FACILITY_BATTLE_TOWER)
        {
            if (input->battleMode == FRONTIER_MODE_DOUBLES) count = FRONTIER_DOUBLES_PARTY_SIZE;
            if (input->battleMode == FRONTIER_MODE_MULTIS || input->battleMode == FRONTIER_MODE_LINK_MULTIS)
            { count = FRONTIER_MULTI_PARTY_SIZE; second = TRUE; }
        }
        else if (facility == FACILITY_BATTLE_DOME)
        {
            /* Non-brain Dome parties are already prepared by the Dome service. */
            struct ADMG_TrainerActorRef actor;
            if (!ADMG_TryClassifyTrainerActor(a, ADMG_ACTOR_FACILITY, ADMG_ACTOR_OPPONENT, &actor)) return FALSE;
            count = a == TRAINER_FRONTIER_BRAIN ? DOME_BATTLE_PARTY_SIZE : 0;
        }
        else if (facility == FACILITY_BATTLE_PALACE || facility == FACILITY_BATTLE_ARENA)
            tent = input->levelMode == FRONTIER_LVL_TENT;
        else if (facility == FACILITY_BATTLE_PIKE_DOUBLE)
        { count = 1; second = TRUE; }
        if (count != 0 && !ADMG_TryAdmitFacilityParty(a, B_TRAINER_OPPONENT_A, count, tent, 0, &admitted)) return FALSE;
        if (second && !ADMG_TryAdmitFacilityParty(b, B_TRAINER_OPPONENT_B, count, FALSE, 0, &admitted)) return FALSE;
        if (facility == FACILITY_BATTLE_TOWER && input->battleMode == FRONTIER_MODE_MULTIS
         && !ADMG_TryAdmitPartnerParty(input->partner)) return FALSE;
    }
    return ADMG_FacilityBattleContextCurrent(input, generation, facility);
}

/* This context occupies the original task's transient data, never save data. */
enum ADMG_FacilityTransitionTaskData
{
    ADMG_FT_SAVE = 0, ADMG_FT_FLAGS = 2, ADMG_FT_PREVIOUS_FLAGS = 4,
    ADMG_FT_A = 6, ADMG_FT_B, ADMG_FT_PARTNER, ADMG_FT_PREVIOUS_PARTNER,
    ADMG_FT_FACILITY, ADMG_FT_PREVIOUS_FACILITY, ADMG_FT_LEVEL,
    ADMG_FT_USES, ADMG_FT_TAG,
};

static void Task_StartBattleAfterTransition(u8 taskId);

static bool8 ADMG_FacilityTransitionTaskOwned(u8 taskId)
{
    return taskId < NUM_TASKS && gTasks[taskId].isActive
        && gTasks[taskId].func == Task_StartBattleAfterTransition
        && (u16)gTasks[taskId].data[ADMG_FT_TAG] == 0x5A3C;
}

static bool8 ADMG_FacilityTransitionContextCurrent(u8 taskId, u64 generation)
{
    if (!ADMG_FacilityTransitionTaskOwned(taskId))
    { ADMG_StateAccessRejected(taskId, __func__); return FALSE; }
    const s16 *data = gTasks[taskId].data;
    struct ADMG_FacilityBattleInput input = {0};
    input.save = (struct SaveBlock2 *)GetWordTaskArg(taskId, ADMG_FT_SAVE);
    input.levelMode = (u16)data[ADMG_FT_LEVEL];
    input.opponentA = data[ADMG_FT_A]; input.opponentB = data[ADMG_FT_B];
    input.partner = data[ADMG_FT_PARTNER];
    input.useA = (data[ADMG_FT_USES] & 1) != 0;
    input.useB = (data[ADMG_FT_USES] & 2) != 0;
    input.usePartner = (data[ADMG_FT_USES] & 4) != 0;
    input.partnerPublished = input.usePartner;
    if (gBattleTypeFlags != GetWordTaskArg(taskId, ADMG_FT_FLAGS)
     || gBattleScripting.specialTrainerBattleType != (u16)data[ADMG_FT_FACILITY])
    { ADMG_StateAccessRejected(taskId, __func__); return FALSE; }
    return ADMG_FacilityBattleContextCurrent(&input, generation, (u16)data[ADMG_FT_FACILITY]);
}

static bool8 ADMG_QueueFacilityBattleTransition(const struct ADMG_FacilityBattleInput *input,
    u32 previousFlags, u16 previousPartner, u8 previousFacility)
{
    /* CreateTask returns zero on exhaustion; establish a free slot first. */
    u32 freeSlot;
    for (freeSlot = 0; freeSlot < NUM_TASKS && gTasks[freeSlot].isActive; ++freeSlot) {}
    if (freeSlot == NUM_TASKS)
    { ADMG_StateAccessRejected(freeSlot, __func__); return FALSE; }
    u8 taskId = CreateTask(Task_StartBattleAfterTransition, 1);
    if (taskId != freeSlot || !gTasks[taskId].isActive
     || gTasks[taskId].func != Task_StartBattleAfterTransition)
    { ADMG_StateAccessRejected(taskId, __func__); return FALSE; }
    SetWordTaskArg(taskId, ADMG_FT_SAVE, (u32)input->save);
    SetWordTaskArg(taskId, ADMG_FT_FLAGS, gBattleTypeFlags);
    SetWordTaskArg(taskId, ADMG_FT_PREVIOUS_FLAGS, previousFlags);
    s16 *data = gTasks[taskId].data;
    data[ADMG_FT_A] = input->opponentA; data[ADMG_FT_B] = input->opponentB;
    data[ADMG_FT_PARTNER] = input->partner;
    data[ADMG_FT_PREVIOUS_PARTNER] = previousPartner;
    data[ADMG_FT_FACILITY] = gBattleScripting.specialTrainerBattleType;
    data[ADMG_FT_PREVIOUS_FACILITY] = previousFacility;
    data[ADMG_FT_LEVEL] = input->levelMode;
    data[ADMG_FT_USES] = input->useA | (input->useB << 1) | (input->usePartner << 2);
    data[ADMG_FT_TAG] = 0x5A3C;
    return TRUE;
}

static void Task_StartBattleAfterTransition(u8 taskId)
{
    u64 generation = ADMG_BeginStateAccessCheck();
    if (!ADMG_FacilityTransitionContextCurrent(taskId, generation)) goto rejected;
    bool8 done = IsBattleTransitionDone();
    if (!ADMG_FacilityTransitionContextCurrent(taskId, generation)) goto rejected;
    if (done == TRUE)
    {
        gMain.savedCallback = HandleFacilityTrainerBattleEnd;
        SetMainCallback2(CB2_InitBattle);
        DestroyTask(taskId);
    }
    return;
rejected:
    /* Do not alter another task if the callback's slot was destroyed or reused. */
    if (!ADMG_FacilityTransitionTaskOwned(taskId)) return;
    gBattleTypeFlags = GetWordTaskArg(taskId, ADMG_FT_PREVIOUS_FLAGS);
    gPartnerTrainerId = gTasks[taskId].data[ADMG_FT_PREVIOUS_PARTNER];
    gBattleScripting.specialTrainerBattleType = gTasks[taskId].data[ADMG_FT_PREVIOUS_FACILITY];
    DestroyTask(taskId);
    SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

ADMG_NATIVE_ENTRY void DoFacilityTrainerBattleInternal(u8 facility)
{
    struct ADMG_FacilityBattleInput admitted = {0};
    if (!ADMG_TryAdmitFacilityBattle(facility, &admitted)) return;
    u64 generation = ADMG_BeginStateAccessCheck();
    typeof(gBattleTypeFlags) previousFlags = gBattleTypeFlags;
    typeof(gPartnerTrainerId) previousPartner = gPartnerTrainerId;
    typeof(gBattleScripting.specialTrainerBattleType) previousFacility = gBattleScripting.specialTrainerBattleType;
    gBattleScripting.specialTrainerBattleType = facility;

    switch (facility)
    {
    case FACILITY_BATTLE_TOWER:
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_BATTLE_TOWER;
        switch (admitted.battleMode)
        {
        case FRONTIER_MODE_SINGLES:
            FillFrontierTrainerParty(FRONTIER_PARTY_SIZE);
            break;
        case FRONTIER_MODE_DOUBLES:
            FillFrontierTrainerParty(FRONTIER_DOUBLES_PARTY_SIZE);
            gBattleTypeFlags |= BATTLE_TYPE_DOUBLE;
            break;
        case FRONTIER_MODE_MULTIS:
            FillFrontierTrainersParties(FRONTIER_MULTI_PARTY_SIZE);
            if (!ADMG_FacilityBattleContextCurrent(&admitted, generation, facility)) goto rejected;
            gPartnerTrainerId = gSaveBlock2Ptr->frontier.trainerIds[17];
            admitted.partnerPublished = TRUE;
            FillPartnerParty(gPartnerTrainerId);
            if (!ADMG_FacilityBattleContextCurrent(&admitted, generation, facility)) goto rejected;
            gBattleTypeFlags |= BATTLE_TYPE_DOUBLE | BATTLE_TYPE_INGAME_PARTNER | BATTLE_TYPE_MULTI | BATTLE_TYPE_TWO_OPPONENTS;
            break;
        case FRONTIER_MODE_LINK_MULTIS:
            gBattleTypeFlags |= BATTLE_TYPE_DOUBLE | BATTLE_TYPE_LINK | BATTLE_TYPE_MULTI | BATTLE_TYPE_TOWER_LINK_MULTI;
            FillFrontierTrainersParties(FRONTIER_MULTI_PARTY_SIZE);
            if (!ADMG_FacilityBattleContextCurrent(&admitted, generation, facility)) goto rejected;
            break;
        }
        if (!ADMG_FacilityBattleContextCurrent(&admitted, generation, facility)) goto rejected;
        if (!ADMG_QueueFacilityBattleTransition(&admitted, previousFlags, previousPartner, previousFacility)) goto rejected;
        PlayMapChosenOrBattleBGM(0);
        BattleTransition_StartOnField(GetSpecialBattleTransition(B_TRANSITION_GROUP_B_TOWER));
        break;
    case FACILITY_BATTLE_DOME:
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_DOME;
        if (admitted.battleMode == FRONTIER_MODE_DOUBLES)
        gBattleTypeFlags |= BATTLE_TYPE_DOUBLE;
        if (TRAINER_BATTLE_PARAM.opponentA == TRAINER_FRONTIER_BRAIN)
        FillFrontierTrainerParty(DOME_BATTLE_PARTY_SIZE);
        if (!ADMG_FacilityBattleContextCurrent(&admitted, generation, facility)) goto rejected;
        if (!ADMG_QueueFacilityBattleTransition(&admitted, previousFlags, previousPartner, previousFacility)) goto rejected;
        CreateTask_PlayMapChosenOrBattleBGM(0);
        BattleTransition_StartOnField(GetSpecialBattleTransition(B_TRANSITION_GROUP_B_DOME));
        break;
    case FACILITY_BATTLE_PALACE:
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_PALACE;
        if (admitted.battleMode == FRONTIER_MODE_DOUBLES)
        gBattleTypeFlags |= BATTLE_TYPE_DOUBLE;
        if (gSaveBlock2Ptr->frontier.lvlMode != FRONTIER_LVL_TENT)
        FillFrontierTrainerParty(FRONTIER_PARTY_SIZE);
        else
        FillTentTrainerParty(FRONTIER_PARTY_SIZE);
        if (!ADMG_FacilityBattleContextCurrent(&admitted, generation, facility)) goto rejected;
        if (!ADMG_QueueFacilityBattleTransition(&admitted, previousFlags, previousPartner, previousFacility)) goto rejected;
        PlayMapChosenOrBattleBGM(0);
        BattleTransition_StartOnField(GetSpecialBattleTransition(B_TRANSITION_GROUP_B_PALACE));
        break;
    case FACILITY_BATTLE_ARENA:
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_ARENA;
        if (gSaveBlock2Ptr->frontier.lvlMode != FRONTIER_LVL_TENT)
        FillFrontierTrainerParty(FRONTIER_PARTY_SIZE);
        else
        FillTentTrainerParty(FRONTIER_PARTY_SIZE);
        if (!ADMG_FacilityBattleContextCurrent(&admitted, generation, facility)) goto rejected;
        if (!ADMG_QueueFacilityBattleTransition(&admitted, previousFlags, previousPartner, previousFacility)) goto rejected;
        PlayMapChosenOrBattleBGM(0);
        BattleTransition_StartOnField(GetSpecialBattleTransition(B_TRANSITION_GROUP_B_ARENA));
        break;
    case FACILITY_BATTLE_FACTORY:
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_FACTORY;
        if (admitted.battleMode == FRONTIER_MODE_DOUBLES)
        gBattleTypeFlags |= BATTLE_TYPE_DOUBLE;
        const struct ADMG_FactoryPartyInput *previous = ADMG_BorrowFactoryParty(&admitted.factory);
        FillFactoryTrainerParty();
        ADMG_BorrowFactoryParty(previous);
        if (!ADMG_FacilityBattleContextCurrent(&admitted, generation, facility)) goto rejected;
        if (!ADMG_QueueFacilityBattleTransition(&admitted, previousFlags, previousPartner, previousFacility)) goto rejected;
        PlayMapChosenOrBattleBGM(0);
        BattleTransition_StartOnField(GetSpecialBattleTransition(B_TRANSITION_GROUP_B_FACTORY));
        break;
    case FACILITY_BATTLE_PIKE_SINGLE:
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_BATTLE_TOWER;
        FillFrontierTrainerParty(FRONTIER_PARTY_SIZE);
        if (!ADMG_FacilityBattleContextCurrent(&admitted, generation, facility)) goto rejected;
        if (!ADMG_QueueFacilityBattleTransition(&admitted, previousFlags, previousPartner, previousFacility)) goto rejected;
        PlayMapChosenOrBattleBGM(0);
        BattleTransition_StartOnField(GetSpecialBattleTransition(B_TRANSITION_GROUP_B_PIKE));
        break;
    case FACILITY_BATTLE_PIKE_DOUBLE:
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_BATTLE_TOWER | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_TWO_OPPONENTS;
        FillFrontierTrainersParties(1);
        if (!ADMG_FacilityBattleContextCurrent(&admitted, generation, facility)) goto rejected;
        if (!ADMG_QueueFacilityBattleTransition(&admitted, previousFlags, previousPartner, previousFacility)) goto rejected;
        PlayMapChosenOrBattleBGM(0);
        BattleTransition_StartOnField(GetSpecialBattleTransition(B_TRANSITION_GROUP_B_PIKE));
        break;
    case FACILITY_BATTLE_PYRAMID:
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_PYRAMID;
        FillFrontierTrainerParty(FRONTIER_PARTY_SIZE);
        if (!ADMG_FacilityBattleContextCurrent(&admitted, generation, facility)) goto rejected;
        if (!ADMG_QueueFacilityBattleTransition(&admitted, previousFlags, previousPartner, previousFacility)) goto rejected;
        PlayMapChosenOrBattleBGM(0);
        BattleTransition_StartOnField(GetSpecialBattleTransition(B_TRANSITION_GROUP_B_PYRAMID));
        break;
    case FACILITY_BATTLE_TRAINER_HILL:
    default:
        break;
    }
    return;
rejected:
    gBattleTypeFlags = previousFlags;
    gPartnerTrainerId = previousPartner;
    gBattleScripting.specialTrainerBattleType = previousFacility;
}

void DoFacilityTrainerBattle(struct ScriptContext *ctx)
{
    u8 facility = ScriptReadByte(ctx);

    DoFacilityTrainerBattleInternal(facility);
}

void FacilityTrainerBattle(struct ScriptContext *ctx)
{
    u8 facility = ScriptReadByte(ctx);

    ConfigureFacilityTrainerBattle(facility, ctx->scriptPtr);
}

static const struct ADMG_FacilityPartyInput *sADMG_BorrowedFacilityParty;

ADMG_NATIVE_ENTRY bool8 ADMG_TryAdmitFacilityParty(u32 identity, u32 target, u32 count, bool8 tent,
    u32 firstMon, struct ADMG_FacilityPartyInput *input)
{
    if (input == NULL || (target != B_TRAINER_OPPONENT_A && target != B_TRAINER_OPPONENT_B)
     || count > MAX_FRONTIER_PARTY_SIZE || firstMon > PARTY_SIZE || count > PARTY_SIZE - firstMon)
    { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    struct ADMG_FacilityPartyInput selected = {0};
    if (!ADMG_TryClassifyTrainerActor(identity, ADMG_ACTOR_FACILITY, ADMG_ACTOR_OPPONENT, &selected.actor)) return FALSE;
    if (tent && selected.actor.kind != ADMG_ACTOR_FRONTIER)
    { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    if (save == NULL) { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    enum FrontierLevelMode mode = save->frontier.lvlMode;
    u64 generation = ADMG_BeginStateAccessCheck();
    selected.save = save; selected.mode = mode;
    selected.target = target; selected.requestedCount = count;
    selected.firstMon = firstMon; selected.tent = tent;
    if (selected.actor.kind == ADMG_ACTOR_FRONTIER)
    {
        const struct BattleFrontierTrainer *table;
        u32 trainerCount;
        if (!ADMG_TrySelectFacilityTrainerTables(tent || mode == FRONTIER_LVL_TENT, &table, &selected.mons, &trainerCount)
         || !ADMG_TryGetFacilityTrainerMonSet(table, identity, &selected.mons, &selected.monSet, &selected.choiceCount)) return FALSE;
        selected.table = table;
    }
    else if (selected.actor.kind == ADMG_ACTOR_RECORD_FRIEND)
    {
        memcpy(&selected.stored.record, &save->frontier.towerRecords[selected.actor.index], sizeof(selected.stored.record));
        if (count > ARRAY_COUNT(selected.stored.record.party))
        { ADMG_StateAccessRejected(count, __func__); return FALSE; }
    }
    else if (selected.actor.kind == ADMG_ACTOR_APPRENTICE)
    {
        memcpy(&selected.stored.apprentice, &save->apprentices[selected.actor.index], sizeof(selected.stored.apprentice));
        if (selected.stored.apprentice.id >= NUM_APPRENTICES)
        { ADMG_StateAccessRejected(selected.stored.apprentice.id, __func__); return FALSE; }
    }
    else if (selected.actor.kind == ADMG_ACTOR_EREADER)
    {
#if FREE_BATTLE_TOWER_E_READER == FALSE
        memcpy(&selected.stored.ereader, &save->frontier.ereaderTrainer, sizeof(selected.stored.ereader));
#endif
    }
    if (selected.actor.kind == ADMG_ACTOR_BRAIN
     && !ADMG_TryPrepareFrontierBrainSelection(&selected.stored.brain)) return FALSE;
    if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (gSaveBlock2Ptr != save || save->frontier.lvlMode != mode)
    { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    memcpy(input, &selected, sizeof(selected));
    return TRUE;
}

static bool8 ADMG_ResolveFacilityPartyLevel(struct ADMG_FacilityPartyInput *input)
{
    if (gSaveBlock2Ptr != input->save || input->save->frontier.lvlMode != input->mode)
    { ADMG_StateAccessRejected(input->actor.identifier, __func__); return FALSE; }
    u64 generation = ADMG_BeginStateAccessCheck();
    input->level = input->tent ? SetTentPtrsGetLevel() : SetFacilityPtrsGetLevel();
    if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (gSaveBlock2Ptr != input->save || input->save->frontier.lvlMode != input->mode
     || (input->actor.kind == ADMG_ACTOR_FRONTIER
      && (gFacilityTrainers != input->table || gFacilityTrainerMons != input->mons)))
    { ADMG_StateAccessRejected(input->actor.identifier, __func__); return FALSE; }
    return TRUE;
}

bool8 ADMG_TryPrepareFacilityParty(u32 identity, u32 target, u32 count, bool8 tent,
    u32 firstMon, struct ADMG_FacilityPartyInput *input)
{
    if (input == NULL) { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    struct ADMG_FacilityPartyInput selected;
    if (!ADMG_TryAdmitFacilityParty(identity, target, count, tent, firstMon, &selected)
     || !ADMG_ResolveFacilityPartyLevel(&selected)) return FALSE;
    memcpy(input, &selected, sizeof(selected));
    return TRUE;
}

const struct ADMG_FacilityPartyInput *ADMG_GetBorrowedFacilityParty(void)
{
    return sADMG_BorrowedFacilityParty;
}

const struct ADMG_FacilityPartyInput *ADMG_BorrowFacilityParty(const struct ADMG_FacilityPartyInput *input)
{
    const struct ADMG_FacilityPartyInput *previous = sADMG_BorrowedFacilityParty;
    sADMG_BorrowedFacilityParty = input;
    return previous;
}

static void ADMG_FillAdmittedFacilityParty(const struct ADMG_FacilityPartyInput *input)
{
    u16 trainerId = input->actor.identifier;
    enum BattleTrainer trainer = input->target;
    u8 monCount = input->requestedCount, level = input->level, firstMonId = input->firstMon;
    s32 i, j;
    u16 chosenMonIndices[MAX_FRONTIER_PARTY_SIZE];
    u8 fixedIV = 0;
    const u16 *monSet = input->monSet;
    u32 bfMonCount = input->choiceCount;
    u32 otID = 0;
    if (input->actor.kind == ADMG_ACTOR_EREADER)
    {
#if FREE_BATTLE_TOWER_E_READER == FALSE
        for (i = 0; i < FRONTIER_PARTY_SIZE; ++i)
        {
            struct BattleTowerPokemon mon = input->stored.ereader.party[i];
            CreateBattleTowerMon(&gParties[trainer][i], &mon);
        }
#endif
        return;
    }
    if (input->actor.kind == ADMG_ACTOR_BRAIN)
    { CreateFrontierBrainPokemon(); return; }
    if (input->actor.kind == ADMG_ACTOR_RECORD_FRIEND)
    {
        for (i = 0; i < monCount; ++i)
        {
            struct BattleTowerPokemon mon = input->stored.record.party[i];
            if (mon.species != SPECIES_NONE && mon.level <= level)
                CreateBattleTowerMon_HandleLevel(&gParties[trainer][i], &mon, FALSE);
        }
        return;
    }
    if (input->actor.kind == ADMG_ACTOR_APPRENTICE)
    {
        for (i = 0; i < FRONTIER_PARTY_SIZE; ++i)
            CreateApprenticeMon(&gParties[trainer][i], &input->stored.apprentice, i);
        return;
    }
    if (!input->tent) fixedIV = GetFrontierTrainerFixedIvs(trainerId);
    // Regular battle frontier trainer.
    // Attempt to fill the trainer's party with random Pokémon until 3 have been
    // successfully chosen. The trainer's party may not have duplicate Pokémon species
    // or duplicate held items.
    i = 0;
    otID = Random32();
    while (i != monCount)
    {
        u16 monId = monSet[Random() % bfMonCount];

        // "High tier" Pokémon are only allowed on open level mode
        // 20 is not a possible value for level here
        if (!input->tent && (level == FRONTIER_MAX_LEVEL_50 || level == 20) && monId > FRONTIER_MONS_HIGH_TIER)
            continue;

        // Ensure this Pokémon species isn't a duplicate.
        for (j = 0; j < i + firstMonId; j++)
        {
            if (GetMonData(&gParties[trainer][j], MON_DATA_SPECIES) == input->mons[monId].species)
                break;
        }
        if (j != i + firstMonId)
            continue;

        // Ensure this Pokemon's held item isn't a duplicate.
        for (j = 0; j < i + firstMonId; j++)
        {
            if (GetMonData(&gParties[trainer][j], MON_DATA_HELD_ITEM) != ITEM_NONE
             && GetMonData(&gParties[trainer][j], MON_DATA_HELD_ITEM) == input->mons[monId].heldItem)
                break;
        }
        if (j != i + firstMonId)
            continue;

        // Ensure this exact Pokémon index isn't a duplicate. This check doesn't seem necessary
        // because the species and held items were already checked directly above.
        for (j = 0; j < i; j++)
        {
            if (chosenMonIndices[j] == monId)
                break;
        }
        if (j != i)
            continue;

        chosenMonIndices[i] = monId;

        // Place the chosen Pokémon into the trainer's party.
        CreateFacilityMon(&input->mons[monId], level, fixedIV, otID, 0, &gParties[trainer][i + firstMonId]);

        // The Pokémon was successfully added to the trainer's party, so it's safe to move on to
        // the next party slot.
        i++;
    }
}

void ADMG_FillFacilityPartyFromArgs(u16 identity, enum BattleTrainer target,
    u8 count, bool8 tent, u8 firstMon)
{
    struct ADMG_FacilityPartyInput local;
    const struct ADMG_FacilityPartyInput *input = sADMG_BorrowedFacilityParty;
    if (input == NULL || input->actor.identifier != identity || input->target != target
     || input->requestedCount != count || input->tent != tent || input->firstMon != firstMon)
    {
        /* The legacy private Tent entry uses opponent A, not its opaque
         * trainerId argument. A root's matched admission is already captured. */
        if (tent) identity = TRAINER_BATTLE_PARAM.opponentA;
        if (!ADMG_TryPrepareFacilityParty(identity, target, count, tent, firstMon, &local)) return;
        input = &local;
    }
    const struct ADMG_FacilityPartyInput *previous = ADMG_BorrowFacilityParty(input);
    ADMG_FillAdmittedFacilityParty(input);
    ADMG_BorrowFacilityParty(previous);
}

void FillFrontierTrainerParty(u8 monsCount)
{
    u16 a = TRAINER_BATTLE_PARAM.opponentA;
    struct ADMG_FacilityPartyInput admittedA;
    if (!ADMG_TryPrepareFacilityParty(a, B_TRAINER_OPPONENT_A, monsCount, FALSE, 0, &admittedA)) return;
    ZeroEnemyPartyMons();
    const struct ADMG_FacilityPartyInput *previous = ADMG_BorrowFacilityParty(&admittedA);
    FillTrainerParty(a, B_TRAINER_OPPONENT_A, monsCount);
    ADMG_BorrowFacilityParty(previous);
}

void FillFrontierTrainersParties(u8 monsCount)
{
    u16 a = TRAINER_BATTLE_PARAM.opponentA;
    struct ADMG_FacilityPartyInput admittedA;
    u16 b = TRAINER_BATTLE_PARAM.opponentB;
    struct ADMG_FacilityPartyInput admittedB;
    if (!ADMG_TryAdmitFacilityParty(a, B_TRAINER_OPPONENT_A, monsCount, FALSE, 0, &admittedA)) return;
    if (!ADMG_TryAdmitFacilityParty(b, B_TRAINER_OPPONENT_B, monsCount, FALSE, 0, &admittedB)) return;
    if (!ADMG_ResolveFacilityPartyLevel(&admittedA) || !ADMG_ResolveFacilityPartyLevel(&admittedB)) return;
    ZeroEnemyPartyMons();
    const struct ADMG_FacilityPartyInput *previous = ADMG_BorrowFacilityParty(&admittedA);
    FillTrainerParty(a, B_TRAINER_OPPONENT_A, monsCount);
    ADMG_BorrowFacilityParty(previous);
    previous = ADMG_BorrowFacilityParty(&admittedB);
    FillTrainerParty(b, B_TRAINER_OPPONENT_B, monsCount);
    ADMG_BorrowFacilityParty(previous);
}

ADMG_NATIVE_ENTRY void FillTrainerParty(u16 trainerId, enum BattleTrainer trainer, u8 monCount)
{
    ADMG_FillFacilityPartyFromArgs(trainerId, trainer, monCount, FALSE, 0);
}

void CreateFacilityMon(const struct TrainerMon *fmon, u16 level, u8 fixedIV, u32 otID, u32 flags, struct Pokemon *dst)
{
    enum PokeBall ball = (fmon->ball == 0xFF) ? Random() % POKEBALL_COUNT : fmon->ball;
    enum Move move;
    u32 personality = 0, friendship, j;
    enum Ability ability;

    if (fmon->gender == TRAINER_MON_MALE)
    {
        personality = GeneratePersonalityForGender(MON_MALE, fmon->species) + 0x1000;
    }
    else if (fmon->gender == TRAINER_MON_FEMALE)
    {
        personality = GeneratePersonalityForGender(MON_FEMALE, fmon->species) + 0x1000;
    }

    ModifyPersonalityForNature(&personality, fmon->nature);
    CreateMonWithIVs(dst, fmon->species, level, personality, OTID_STRUCT_PRESET(otID), fixedIV);

    friendship = MAX_FRIENDSHIP;
    // Give the chosen Pokémon its specified moves.
    for (j = 0; j < MAX_MON_MOVES; j++)
    {
        move = fmon->moves[j];
        if (flags & FLAG_FRONTIER_MON_FACTORY && move == MOVE_RETURN)
            move = MOVE_FRUSTRATION;

        SetMonMoveSlot(dst, move, j);
        if (GetMoveEffect(move) == EFFECT_FRUSTRATION)
            friendship = 0;  // Frustration is more powerful the lower the Pokémon's friendship is.
    }

    SetMonData(dst, MON_DATA_FRIENDSHIP, &friendship);
    SetMonData(dst, MON_DATA_HELD_ITEM, &fmon->heldItem);

    // try to set ability. Otherwise, random of non-hidden as per vanilla
    if (fmon->ability != ABILITY_NONE)
    {
        const struct SpeciesInfo *speciesInfo = &gSpeciesInfo[fmon->species];
        u32 maxAbilities = ARRAY_COUNT(speciesInfo->abilities);
        for (ability = 0; ability < maxAbilities; ++ability)
        {
            if (speciesInfo->abilities[ability] == fmon->ability)
                break;
        }
        if (ability >= maxAbilities)
            ability = 0;
        SetMonData(dst, MON_DATA_ABILITY_NUM, &ability);
    }

    if (fmon->ev != NULL)
    {
        SetMonData(dst, MON_DATA_HP_EV, &(fmon->ev[0]));
        SetMonData(dst, MON_DATA_ATK_EV, &(fmon->ev[1]));
        SetMonData(dst, MON_DATA_DEF_EV, &(fmon->ev[2]));
        SetMonData(dst, MON_DATA_SPATK_EV, &(fmon->ev[3]));
        SetMonData(dst, MON_DATA_SPDEF_EV, &(fmon->ev[4]));
        SetMonData(dst, MON_DATA_SPEED_EV, &(fmon->ev[5]));
    }

    if (fmon->iv)
        SetMonData(dst, MON_DATA_IVS, &(fmon->iv));

    if (fmon->isShiny)
    {
        u32 data = TRUE;
        SetMonData(dst, MON_DATA_IS_SHINY, &data);
    }
    if (fmon->dynamaxLevel > 0)
    {
        u32 data = fmon->dynamaxLevel;
        SetMonData(dst, MON_DATA_DYNAMAX_LEVEL, &data);
    }
    if (fmon->gigantamaxFactor)
    {
        u32 data = fmon->gigantamaxFactor;
        SetMonData(dst, MON_DATA_GIGANTAMAX_FACTOR, &data);
    }
    if (fmon->teraType)
    {
        u32 data = fmon->teraType;
        SetMonData(dst, MON_DATA_TERA_TYPE, &data);
    }


    SetMonData(dst, MON_DATA_POKEBALL, &ball);
    CalculateMonStats(dst);
}
