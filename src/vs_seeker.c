#include "global.h"
#include "main.h"
#include "admg_rematch_response.h"
#include "admg_rematch_movement.h"
#include "admg_rematch_battle.h"
#include "admg_rematch.h"
#include "admg_trainer.h"
#include "task.h"
#include "event_object_movement.h"
#include "item_use.h"
#include "event_scripts.h"
#include "event_data.h"
#include "admg_state.h"
#include "script.h"
#include "event_object_lock.h"
#include "field_specials.h"
#include "item.h"
#include "item_menu.h"
#include "field_effect.h"
#include "script_movement.h"
#include "battle.h"
#include "battle_setup.h"
#include "random.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "vs_seeker.h"
#include "menu.h"
#include "string_util.h"
#include "tv.h"
#include "malloc.h"
#include "field_screen_effect.h"
#include "gym_leader_rematch.h"
#include "sound.h"
#include "constants/event_object_movement.h"
#include "constants/event_objects.h"
#include "constants/items.h"
#include "constants/maps.h"
#include "constants/songs.h"
#include "constants/script_commands.h"
#include "constants/trainer_types.h"
#include "constants/field_effects.h"

// Documentation for the Vs. Seeker can be found in docs/tutorials/vs_seeker.md.

enum
{
   VSSEEKER_NOT_CHARGED,
   VSSEEKER_NO_ONE_IN_RANGE,
   VSSEEKER_CAN_USE,
};

typedef enum
{
    VSSEEKER_SINGLE_RESP_RAND,
    VSSEEKER_SINGLE_RESP_NO,
    VSSEEKER_SINGLE_RESP_YES
} VsSeekerSingleRespCode;

typedef enum
{
    VSSEEKER_RESPONSE_NO_RESPONSE,
    VSSEEKER_RESPONSE_UNFOUGHT_TRAINERS,
    VSSEEKER_RESPONSE_FOUND_REMATCHES
} VsSeekerResponseCode;

struct VsSeekerTrainerInfo
{
    const u8 *script;
    u16 trainerIdx;
    u8 localId;
    u8 objectEventId;
    s16 xCoord;
    s16 yCoord;
    u16 graphicsId;
    u8 templateIndex;
};

struct VsSeekerStruct
{
    struct VsSeekerTrainerInfo trainerInfo[OBJECT_EVENT_TEMPLATES_COUNT];
    u16 trainerIdxArray[OBJECT_EVENT_TEMPLATES_COUNT];
    u8 runningBehaviourEtcArray[OBJECT_EVENT_TEMPLATES_COUNT];
    u8 numRematchableTrainers;
    u8 trainerCount;
    bool8 native, nativeFailed;
    u8 taskId;
    u32 ticket;
    struct ADMG_RematchContext nativeContext;
    struct ADMG_RematchActor nativeActors[ADMG_REMATCH_COHORT_COUNT];
    u64 nativeYes;
};

#define END_TRAINER_INFO 0xFF

static bool8 InitVsSeekerField(u32, const struct ADMG_RematchContext *);
static bool8 NativeVsSeekerTaskCurrent(u32);
static u8 NativeVsSeekerCanUse(void);
static u8 NativeVsSeekerGetResponse(void);
static bool8 NativeVsSeekerStartIdleMovements(void);

static bool8 NativeRematchResetObjectsValid(void);
// static declarations
static EWRAM_DATA struct VsSeekerStruct *sVsSeeker = NULL;

static void VsSeekerResetInBagStepCounter(void);
static void VsSeekerResetChargingStepCounter(void);
static void Task_ResetObjectsRematchWantedState(u8 taskId);
static void ResetMovementOfRematchableTrainers(void);
static void Task_VsSeekerFrameCountdown(u8 taskId);
static void Task_VsSeeker_PlaySoundAndGetResponseCode(u8 taskId);
static bool8 GatherNearbyTrainerInfo(void);
static void Task_VsSeeker_ShowResponseToPlayer(u8 taskId);
static bool8 CanUseVsSeeker(void);
static u8 GetVsSeekerResponseInArea(void);
#if FREE_MATCH_CALL == FALSE
static u8 GetResponseMovementTypeFromTrainerGraphicsId(u8 graphicsId);
#endif //FREE_MATCH_CALL
static u16 GetTrainerFlagFromScript(const u8 * script);
static void ClearAllTrainerRematchStates(void);
#if FREE_MATCH_CALL == FALSE
static bool8 IsTrainerVisibleOnScreen(struct VsSeekerTrainerInfo * trainerInfo);
static bool32 HasFightableTrainers(void);
static void StartTrainerObjectMovementScript(struct VsSeekerTrainerInfo * trainerInfo, const u8 * script);
#endif //FREE_MATCH_CALL
static void StartAllRespondantIdleMovements(void);
static bool8 ObjectEventIdIsSane(u8 objectEventId);
static u8 GetRandomFaceDirectionMovementType();

static const u8 sMovementScript_Wait48[] = {
    MOVEMENT_ACTION_DELAY_16,
    MOVEMENT_ACTION_DELAY_16,
    MOVEMENT_ACTION_DELAY_16,
    MOVEMENT_ACTION_STEP_END
};

static const u8 sMovementScript_TrainerUnfought[] = {
    MOVEMENT_ACTION_EMOTE_EXCLAMATION_MARK,
    MOVEMENT_ACTION_STEP_END
};

static const u8 sMovementScript_TrainerUnknown[] = {
    MOVEMENT_ACTION_EMOTE_QUESTION_MARK,
    MOVEMENT_ACTION_STEP_END
};

static const u8 sMovementScript_TrainerNoRematch[] = {
    MOVEMENT_ACTION_EMOTE_X,
    MOVEMENT_ACTION_STEP_END
};

static const u8 sMovementScript_TrainerRematch[] = {
    MOVEMENT_ACTION_WALK_IN_PLACE_FASTER_DOWN,
    MOVEMENT_ACTION_EMOTE_DOUBLE_EXCL_MARK,
    MOVEMENT_ACTION_STEP_END
};

static const u8 sFaceDirectionMovementTypeByFacingDirection[] = {
    MOVEMENT_TYPE_FACE_DOWN,
    MOVEMENT_TYPE_FACE_DOWN,
    MOVEMENT_TYPE_FACE_UP,
    MOVEMENT_TYPE_FACE_LEFT,
    MOVEMENT_TYPE_FACE_RIGHT
};

void VsSeekerFreezeObjectsAfterChargeComplete(void)
{
    CreateTask(Task_ResetObjectsRematchWantedState, 80);
}

#define tIsPlayerFrozen data[0]
#define tAreObjectsFrozen data[1]

static void Task_ResetObjectsRematchWantedState(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    u32 i;

    if ((!task->tIsPlayerFrozen) && IsPlayerStandingStill())
    {
        PlayerFreeze();
        task->tIsPlayerFrozen = TRUE;
    }

    if (!task->tAreObjectsFrozen)
    {
        for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
        {
            if (!ObjectEventIdIsSane(i))
                continue;

            if (gObjectEvents[i].singleMovementActive)
                return;

            FreezeObjectEvent(&gObjectEvents[i]);
        }
    }

    task->tAreObjectsFrozen = TRUE;
    if (task->tIsPlayerFrozen)
    {
        DestroyTask(taskId);
        StopPlayerAvatar();
        ScriptContext_Enable();
    }
}
#undef tIsPlayerFrozen
#undef tAreObjectsFrozen

u16 VsSeekerConvertLocalIdToTableId(u16 localId)
{
    u32 localIdIndex = 0;
    u32 trainerId = 0;

    for (localIdIndex = 0; sVsSeeker != NULL && localIdIndex < sVsSeeker->trainerCount ; localIdIndex++)
    {
        if (sVsSeeker->trainerInfo[localIdIndex].localId == localId)
        {
            trainerId = sVsSeeker->trainerInfo[localIdIndex].trainerIdx;
            return TrainerIdToRematchTableId(gRematchTable,trainerId);
        }
    }
    return -1;
}

void VsSeekerResetObjectMovementAfterChargeComplete(void)
{
    (void)ADMG_TryResetNativeRematchTemplateMovements();

    struct ObjectEventTemplate * templates = gSaveBlock1Ptr->objectEventTemplates;
    u32 i;
    u32 movementType;
    u8 objEventId;
    struct ObjectEvent * objectEvent;

    for (i = 0; i < gMapHeader.events->objectEventCount; i++)
    {
        if (templates[i].trainerType != TRAINER_TYPE_NORMAL
        && templates[i].trainerType != TRAINER_TYPE_BURIED)
            continue;

        if (templates[i].movementType != MOVEMENT_TYPE_ROTATE_CLOCKWISE)
            continue;

        movementType = GetRandomFaceDirectionMovementType();
        TryGetObjectEventIdByLocalIdAndMap(templates[i].localId, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup, &objEventId);
        objectEvent = &gObjectEvents[objEventId];

        if (!ObjectEventIdIsSane(objEventId))
            continue;

        SetTrainerMovementType(objectEvent, movementType);
        templates[i].movementType = movementType;
    }
}

bool8 UpdateVsSeekerStepCounter(void)
{
#if FREE_MATCH_CALL == FALSE
    u8 x = 0;

    if (!I_VS_SEEKER_CHARGING) return FALSE;

    // This condition helps in case your save file is switching between vs seeker and matchcall
    if (gSaveBlock1Ptr->trainerRematchStepCounter > VSSEEKER_RECHARGE_STEPS && gSaveBlock1Ptr->trainerRematchStepCounter <= 0xFF)
        gSaveBlock1Ptr->trainerRematchStepCounter = 0;
    if (CheckBagHasItem(ITEM_VS_SEEKER, 1))
    {
        if ((gSaveBlock1Ptr->trainerRematchStepCounter & 0xFF) < VSSEEKER_RECHARGE_STEPS)
            gSaveBlock1Ptr->trainerRematchStepCounter++;
    }

    if (FlagGet(I_VS_SEEKER_CHARGING))
    {
        if (((gSaveBlock1Ptr->trainerRematchStepCounter >> 8) & 0xFF) < VSSEEKER_RECHARGE_STEPS)
        {
            x = (((gSaveBlock1Ptr->trainerRematchStepCounter >> 8) & 0xFF) + 1);
            gSaveBlock1Ptr->trainerRematchStepCounter = (gSaveBlock1Ptr->trainerRematchStepCounter & 0xFF) | (x << 8);
        }
        if (((gSaveBlock1Ptr->trainerRematchStepCounter >> 8) & 0xFF) == VSSEEKER_RECHARGE_STEPS)
        {
            FlagClear(I_VS_SEEKER_CHARGING);
            VsSeekerResetChargingStepCounter();
            ClearAllTrainerRematchStates();
            return TRUE;
        }
    }
#endif //FREE_MATCH_CALL

    return FALSE;
}

void MapResetTrainerRematches(u16 mapGroup, u16 mapNum)
{
    (void)ADMG_TryResetNativeRematchMap();

    if (!I_VS_SEEKER_CHARGING) return;

    FlagClear(I_VS_SEEKER_CHARGING);
    VsSeekerResetChargingStepCounter();
    ClearAllTrainerRematchStates();
    ResetMovementOfRematchableTrainers();
}

static void ResetMovementOfRematchableTrainers(void)
{
    u32 i;
    u8 movementType = 0;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        struct ObjectEvent * objectEvent = &gObjectEvents[i];
        if (objectEvent->movementType != MOVEMENT_TYPE_ROTATE_CLOCKWISE)
            continue;

        movementType = GetRandomFaceDirectionMovementType();

        if (!objectEvent->active || gSprites[objectEvent->spriteId].data[0] != i)
            continue;

        gSprites[objectEvent->spriteId].x2 = 0;
        gSprites[objectEvent->spriteId].y2 = 0;
        SetTrainerMovementType(objectEvent, movementType);
    }
}

static void VsSeekerResetInBagStepCounter(void)
{
#if FREE_MATCH_CALL == FALSE
    gSaveBlock1Ptr->trainerRematchStepCounter &= 0xFF00;
#endif //FREE_MATCH_CALL
}

static void VsSeekerResetChargingStepCounter(void)
{
#if FREE_MATCH_CALL == FALSE
    gSaveBlock1Ptr->trainerRematchStepCounter &= 0x00FF;
#endif //FREE_MATCH_CALL
}

#define tCountdown      data[0]
#define tBeepDelay      data[1]
#define tNumBeeps       data[2]
#define tResponseCode   data[3]

void Task_InitVsSeekerAndCheckForTrainersOnScreen(u8 taskId)
{
    if (!I_VS_SEEKER_CHARGING) return;
    (void)InitVsSeekerField(taskId, NULL);
}



static void Task_VsSeekerFrameCountdown(u8 taskId)
{
    if (!NativeVsSeekerTaskCurrent(taskId)) return;

    if (--gTasks[taskId].tCountdown == 0)
    {
        gTasks[taskId].func = Task_VsSeeker_PlaySoundAndGetResponseCode;
        gTasks[taskId].tBeepDelay = 16;
    }
}

static void Task_VsSeeker_PlaySoundAndGetResponseCode(u8 taskId)
{
    if (!NativeVsSeekerTaskCurrent(taskId)) return;

    s16 * data = gTasks[taskId].data;

    if (tNumBeeps != 2 && --tBeepDelay == 0)
    {
        PlaySE(SE_CONTEST_MONS_TURN);
        tBeepDelay = 11;
        tNumBeeps++;
    }

    if (!FieldEffectActiveListContains(FLDEFF_USE_VS_SEEKER))
    {
        if (sVsSeeker->native)
        {
            if (!ADMG_TryResetNativeBagCharge(&sVsSeeker->nativeContext))
            {sVsSeeker->nativeFailed = TRUE; (void)NativeVsSeekerTaskCurrent(taskId); return;}
        }
        else VsSeekerResetInBagStepCounter();
        tResponseCode = GetVsSeekerResponseInArea();
        if (!NativeVsSeekerTaskCurrent(taskId)) return;
        ScriptMovement_StartObjectMovementScript(LOCALID_PLAYER, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup, sMovementScript_Wait48);
        gTasks[taskId].func = Task_VsSeeker_ShowResponseToPlayer;
    }
}

static void Task_VsSeeker_ShowResponseToPlayer(u8 taskId)
{
    if (!NativeVsSeekerTaskCurrent(taskId)) return;

    if (!ScriptMovement_IsObjectMovementFinished(LOCALID_PLAYER, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup))
        return;

    if (gTasks[taskId].tResponseCode == VSSEEKER_RESPONSE_NO_RESPONSE)
    {
        DisplayItemMessageOnField(taskId, VSSeeker_Text_TrainersNotReady, Task_ItemUse_CloseMessageBoxAndReturnToField_VsSeeker);
    }
    else
    {
        if (gTasks[taskId].tResponseCode == VSSEEKER_RESPONSE_FOUND_REMATCHES)
            StartAllRespondantIdleMovements();
        if (!NativeVsSeekerTaskCurrent(taskId)) return;

        ClearDialogWindowAndFrame(0, TRUE);
        ScriptUnfreezeObjectEvents();
        UnlockPlayerFieldControls();
        DestroyTask(taskId);
    }
    Free(sVsSeeker); sVsSeeker = NULL;
}

#undef tCountdown
#undef tBeepDelay
#undef tNumBeeps
#undef tResponseCode

static bool8 GatherNearbyTrainerInfo(void)
{
    const struct ObjectEventTemplate *declared;
    struct ADMG_TrainerRef ref;
    u8 objectEventId;
    u32 i;
    s16 playerX, playerY;
    if (sVsSeeker == NULL || gSaveBlock1Ptr == NULL || gMapHeader.events == NULL
     || gMapHeader.events->objectEventCount > OBJECT_EVENT_TEMPLATES_COUNT)
    { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    sVsSeeker->trainerCount = 0;
    PlayerGetDestCoords(&playerX, &playerY);
    playerX -= MAP_OFFSET; playerY -= MAP_OFFSET;
    for (i = 0; i < gMapHeader.events->objectEventCount; ++i)
    {
        const struct ObjectEventTemplate *saved = &gSaveBlock1Ptr->objectEventTemplates[i];
        if (!ADMG_TryGetDeclaredObjectTemplate((u32)gSaveBlock1Ptr->location.mapGroup,
                (u32)gSaveBlock1Ptr->location.mapNum, saved->localId, &declared))
            return FALSE;
        if (sVsSeeker->native
         && declared->trainerType != TRAINER_TYPE_NORMAL && declared->trainerType != TRAINER_TYPE_BURIED)
            continue;
        u16 trainerIdx = GetTrainerFlagFromScript(declared->script);
        if (sVsSeeker->native)
        {
            if (!ADMG_TryResolveTrainer(trainerIdx, &ref)) return FALSE;
            if (ref.owner != ADMG_TRAINER_KANTO) continue;
            if (saved->localId == 0 || saved->localId >= ADMG_NATIVE_READINESS_COUNT)
            { ADMG_StateAccessRejected(saved->localId, __func__); return FALSE; }
        }
        else
        {
            if (trainerIdx == TRAINER_NONE && (!I_SHOW_NO_ID_TRAINER || declared->trainerType == TRAINER_TYPE_NONE))
                continue;
            if (trainerIdx == TRAINER_NONE && declared->trainerType != TRAINER_TYPE_NONE)
                DebugPrintf("Object event with local id %d is not TRAINER_TYPE_NONE but doesn't have a visible trainerID", saved->localId);
        }
        struct VsSeekerTrainerInfo *info = &sVsSeeker->trainerInfo[sVsSeeker->trainerCount];
        info->script = declared->script; info->trainerIdx = trainerIdx; info->localId = saved->localId;
        info->templateIndex = i;
        info->graphicsId = saved->kind == OBJ_KIND_CLONE ? declared->graphicsId : saved->graphicsId;
        objectEventId = OBJECT_EVENTS_COUNT;
        if (TryGetObjectEventIdByLocalIdAndMap(saved->localId, gSaveBlock1Ptr->location.mapNum,
                gSaveBlock1Ptr->location.mapGroup, &objectEventId) || !ObjectEventIdIsSane(objectEventId))
            objectEventId = OBJECT_EVENTS_COUNT;
        info->objectEventId = objectEventId;
        info->xCoord = saved->x; info->yCoord = saved->y;
        if (objectEventId < OBJECT_EVENTS_COUNT)
        {
            info->xCoord = gObjectEvents[objectEventId].currentCoords.x - MAP_OFFSET;
            info->yCoord = gObjectEvents[objectEventId].currentCoords.y - MAP_OFFSET;
        }
        if (sVsSeeker->native)
        {
            struct ADMG_RematchActor *actor = &sVsSeeker->nativeActors[sVsSeeker->trainerCount];
            actor->trainerId = trainerIdx; actor->localId = saved->localId; actor->objectId = objectEventId;
            actor->visible = objectEventId < OBJECT_EVENTS_COUNT
                && playerX - 7 <= info->xCoord && playerX + 7 >= info->xCoord
                && playerY - 5 <= info->yCoord && playerY + 5 >= info->yCoord;
        }
        ++sVsSeeker->trainerCount;
    }
    return TRUE;
}

static u8 CanUseVsSeeker(void)
{
    if (sVsSeeker != NULL && sVsSeeker->native) return NativeVsSeekerCanUse();

#if FREE_MATCH_CALL == FALSE
    u8 vsSeekerChargeSteps = gSaveBlock1Ptr->trainerRematchStepCounter;

    if ((vsSeekerChargeSteps == VSSEEKER_RECHARGE_STEPS) && !HasFightableTrainers())
        return VSSEEKER_NO_ONE_IN_RANGE;

    if (vsSeekerChargeSteps == VSSEEKER_RECHARGE_STEPS)
        return VSSEEKER_CAN_USE;

    ConvertIntToDecimalStringN(gStringVar1, (VSSEEKER_RECHARGE_STEPS - vsSeekerChargeSteps), STR_CONV_MODE_LEFT_ALIGN, 3);
    return VSSEEKER_NOT_CHARGED;
#else
    return VSSEEKER_NO_ONE_IN_RANGE;
#endif //FREE_MATCH_CALL
}

static u8 GetVsSeekerResponseInArea(void)
{
    if (sVsSeeker != NULL && sVsSeeker->native) return NativeVsSeekerGetResponse();

#if FREE_MATCH_CALL == FALSE
    u32 trainerIdx, rematchTrainerIdx;
    s32 vsSeekerIdx = 0;
    bool32 trainerHasNotYetBeenFought = FALSE;
    bool32 trainerWantsRematch = FALSE;
    for (vsSeekerIdx = 0; vsSeekerIdx < sVsSeeker->trainerCount; vsSeekerIdx++)
    {
        if (!IsTrainerVisibleOnScreen(&sVsSeeker->trainerInfo[vsSeekerIdx]))
            continue;

        trainerIdx = sVsSeeker->trainerInfo[vsSeekerIdx].trainerIdx;
        if (trainerIdx == TRAINER_NONE)
        {
            StartTrainerObjectMovementScript(&sVsSeeker->trainerInfo[vsSeekerIdx], sMovementScript_TrainerUnknown);
            continue;
        }

        if (!HasTrainerBeenFought(trainerIdx))
        {
            StartTrainerObjectMovementScript(&sVsSeeker->trainerInfo[vsSeekerIdx], sMovementScript_TrainerUnfought);
            trainerHasNotYetBeenFought = TRUE;
            continue;
        }

        rematchTrainerIdx = GetRematchTrainerIdVSSeeker(trainerIdx);
        if (rematchTrainerIdx == 0)
        {
            StartTrainerObjectMovementScript(&sVsSeeker->trainerInfo[vsSeekerIdx], sMovementScript_TrainerNoRematch);
            continue;
        }

        gSaveBlock1Ptr->trainerRematches[VsSeekerConvertLocalIdToTableId(sVsSeeker->trainerInfo[vsSeekerIdx].localId)] = rematchTrainerIdx;
        ShiftStillObjectEventCoords(&gObjectEvents[sVsSeeker->trainerInfo[vsSeekerIdx].objectEventId]);
        StartTrainerObjectMovementScript(&sVsSeeker->trainerInfo[vsSeekerIdx], sMovementScript_TrainerRematch);
        sVsSeeker->trainerIdxArray[sVsSeeker->numRematchableTrainers] = vsSeekerIdx;
        sVsSeeker->runningBehaviourEtcArray[sVsSeeker->numRematchableTrainers] = GetResponseMovementTypeFromTrainerGraphicsId(sVsSeeker->trainerInfo[vsSeekerIdx].graphicsId);
        sVsSeeker->numRematchableTrainers++;
        trainerWantsRematch = TRUE;
    }

    if (trainerWantsRematch)
    {
        PlaySE(SE_PIN);
        FlagSet(I_VS_SEEKER_CHARGING);
        VsSeekerResetChargingStepCounter();
        return VSSEEKER_RESPONSE_FOUND_REMATCHES;
    }

    if (trainerHasNotYetBeenFought)
        return VSSEEKER_RESPONSE_UNFOUGHT_TRAINERS;
#endif //FREE_MATCH_CALL

    return VSSEEKER_RESPONSE_NO_RESPONSE;
}

static bool32 ShouldChangeMovementForTrainerType(u32 trainerType)
{
    if (trainerType == TRAINER_TYPE_NORMAL)
        return TRUE;
    return (trainerType == TRAINER_TYPE_BURIED);
}

void ClearRematchMovementByTrainerId(void)
{
    s32 i;
    u8 objEventId = 0;
    struct ObjectEventTemplate *objectEventTemplates = gSaveBlock1Ptr->objectEventTemplates;
    struct ObjectEvent *objectEvent;

    int vsSeekerDataIdx = TrainerIdToRematchTableId(gRematchTable, TRAINER_BATTLE_PARAM.opponentA);

    if (!I_VS_SEEKER_CHARGING) return;

    if (vsSeekerDataIdx == -1)
        return;

    for (i = 0; i < gMapHeader.events->objectEventCount; i++)
    {
        if (!ShouldChangeMovementForTrainerType(objectEventTemplates[i].trainerType)
        || vsSeekerDataIdx != TrainerIdToRematchTableId(gRematchTable, GetTrainerFlagFromScript(objectEventTemplates[i].script)))
            continue;

        TryGetObjectEventIdByLocalIdAndMap(objectEventTemplates[i].localId, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup, &objEventId);
        objectEvent = &gObjectEvents[objEventId];
        TryOverrideTemplateCoordsForObjectEvent(objectEvent, sFaceDirectionMovementTypeByFacingDirection[objectEvent->facingDirection]);

        if (gSelectedObjectEvent == objEventId)
            objectEvent->movementType = sFaceDirectionMovementTypeByFacingDirection[objectEvent->facingDirection];
        else
            objectEvent->movementType = MOVEMENT_TYPE_FACE_DOWN;
    }
}

static u32 GetGameProgressFlags()
{
    const u32 gameProgressFlags[] = {
        FLAG_VISITED_LAVARIDGE_TOWN,
        FLAG_VISITED_FORTREE_CITY,
        FLAG_SYS_GAME_CLEAR,
        FLAG_DEFEATED_METEOR_FALLS_STEVEN
    };
    u32 i = 0, numGameProgressFlags = 0;
    u32 maxGameProgressFlags = ARRAY_COUNT(gameProgressFlags);

    for (i = 0; i < maxGameProgressFlags; i++)
    {
        bool8 reached;
        if (!ADMG_TryReadFlag(gameProgressFlags[i], &reached))
            return 0;
        if (reached)
            numGameProgressFlags++;
    }

    return numGameProgressFlags;
}

u16 GetRematchTrainerIdVSSeeker(u16 trainerId)
{
    struct ADMG_TrainerRef ref;
    u16 selected;
    if (!ADMG_TryResolveTrainer(trainerId, &ref))
        return 0;
    if (ref.owner == ADMG_TRAINER_KANTO)
        return ADMG_TryGetNativeRematchTrainer(trainerId, &selected) ? selected : 0;
    if (ref.owner != ADMG_TRAINER_HOENN)
    {
        ADMG_StateAccessRejected(trainerId, __func__);
        return 0;
    }
    u64 generation = ADMG_BeginStateAccessCheck();
    u32 tableId = FirstBattleTrainerIdToRematchTableId(gRematchTable, trainerId);
    u32 rematchTrainerIdx = GetGameProgressFlags();
    if (!ADMG_EndStateAccessCheck(generation))
        return 0;

    if (!I_VS_SEEKER_CHARGING)
        return 0;
    if (tableId == -1)
        return 0;
    if (tableId >= REMATCH_ELITE_FOUR_ENTRIES)
        return 0;
    if (tableId >= REMATCH_SPECIAL_TRAINER_START)
        return GetCurrentGymLeaderRematchLevel();

    while (rematchTrainerIdx != 0 && !HasTrainerBeenFought(gRematchTable[tableId].trainerIds[rematchTrainerIdx-1]))
    {
        if (rematchTrainerIdx== 0)
            break;

        rematchTrainerIdx--;
    }

    return gRematchTable[tableId].trainerIds[rematchTrainerIdx];
}

bool32 IsVsSeekerEnabled(void)
{
    if (I_VS_SEEKER_CHARGING == 0)
        return FALSE;

    return (CheckBagHasItem(ITEM_VS_SEEKER, 1));
}

static bool8 ObjectEventIdIsSane(u8 objectEventId)
{
    if (objectEventId >= OBJECT_EVENTS_COUNT || gSaveBlock1Ptr == NULL) return FALSE;
    const struct ObjectEvent *object = &gObjectEvents[objectEventId];
    return object->active && object->spriteId < MAX_SPRITES
        && gSprites[object->spriteId].data[0] == objectEventId
        && object->mapGroup == gSaveBlock1Ptr->location.mapGroup
        && object->mapNum == gSaveBlock1Ptr->location.mapNum;
}

static u8 GetRandomFaceDirectionMovementType()
{
    u16 randomFacingDirection = Random() % 4;

    switch (randomFacingDirection)
    {
    case 0:
        return MOVEMENT_TYPE_FACE_UP;
    case 1:
        return MOVEMENT_TYPE_FACE_DOWN;
    case 2:
        return MOVEMENT_TYPE_FACE_LEFT;
    case 3:
        return MOVEMENT_TYPE_FACE_RIGHT;
    default:
        return MOVEMENT_TYPE_FACE_DOWN;
    }
}

#if FREE_MATCH_CALL == FALSE
static bool32 IsRegularLandTrainer(u8 graphicsId)
{
    u32 i;
    u16 regularTrainersOnLand[] =
    {
        OBJ_EVENT_GFX_AQUA_MEMBER_F,
        OBJ_EVENT_GFX_AQUA_MEMBER_M,
        OBJ_EVENT_GFX_BEAUTY,
        OBJ_EVENT_GFX_BLACK_BELT,
        OBJ_EVENT_GFX_BOY_1,
        OBJ_EVENT_GFX_BOY_2,
        OBJ_EVENT_GFX_BOY_3,
        OBJ_EVENT_GFX_BUG_CATCHER,
        OBJ_EVENT_GFX_CAMPER,
        OBJ_EVENT_GFX_CYCLING_TRIATHLETE_F,
        OBJ_EVENT_GFX_CYCLING_TRIATHLETE_M,
        OBJ_EVENT_GFX_EXPERT_F,
        OBJ_EVENT_GFX_EXPERT_M,
        OBJ_EVENT_GFX_FAT_MAN,
        OBJ_EVENT_GFX_FISHERMAN,
        OBJ_EVENT_GFX_GENTLEMAN,
        OBJ_EVENT_GFX_GIRL_1,
        OBJ_EVENT_GFX_GIRL_2,
        OBJ_EVENT_GFX_GIRL_3,
        OBJ_EVENT_GFX_HEX_MANIAC,
        OBJ_EVENT_GFX_HIKER,
        OBJ_EVENT_GFX_LASS,
        OBJ_EVENT_GFX_LITTLE_BOY,
        OBJ_EVENT_GFX_LITTLE_GIRL,
        OBJ_EVENT_GFX_MAGMA_MEMBER_F,
        OBJ_EVENT_GFX_MAGMA_MEMBER_M,
        OBJ_EVENT_GFX_MAN_3,
        OBJ_EVENT_GFX_MAN_4,
        OBJ_EVENT_GFX_MAN_5,
        OBJ_EVENT_GFX_MANIAC,
        OBJ_EVENT_GFX_NINJA_BOY,
        OBJ_EVENT_GFX_PICNICKER,
        OBJ_EVENT_GFX_POKEFAN_F,
        OBJ_EVENT_GFX_POKEFAN_M,
        OBJ_EVENT_GFX_PSYCHIC_M,
        OBJ_EVENT_GFX_RICH_BOY,
        OBJ_EVENT_GFX_RUNNING_TRIATHLETE_F,
        OBJ_EVENT_GFX_RUNNING_TRIATHLETE_M,
        OBJ_EVENT_GFX_SAILOR,
        OBJ_EVENT_GFX_SCHOOL_KID_M,
        OBJ_EVENT_GFX_TUBER_F,
        OBJ_EVENT_GFX_TUBER_M,
        OBJ_EVENT_GFX_TWIN,
        OBJ_EVENT_GFX_WOMAN_1,
        OBJ_EVENT_GFX_WOMAN_2,
        OBJ_EVENT_GFX_WOMAN_4,
        OBJ_EVENT_GFX_WOMAN_5,
        OBJ_EVENT_GFX_YOUNGSTER,
        OBJ_EVENT_GFX_TUBER_F_FRLG,
        OBJ_EVENT_GFX_SAILOR_FRLG,
        OBJ_EVENT_GFX_HIKER_FRLG,
        OBJ_EVENT_GFX_PICNICKER_FRLG,
        OBJ_EVENT_GFX_CAMPER_FRLG,
        OBJ_EVENT_GFX_BEAUTY_FRLG,
        OBJ_EVENT_GFX_WOMAN_2_FRLG,
        OBJ_EVENT_GFX_FAT_MAN_FRLG,
        OBJ_EVENT_GFX_BUG_CATCHER_FRLG,
        OBJ_EVENT_GFX_WOMAN_1_FRLG,
        OBJ_EVENT_GFX_YOUNGSTER_FRLG,
        OBJ_EVENT_GFX_LASS_FRLG,
        OBJ_EVENT_GFX_LITTLE_GIRL_FRLG,
        OBJ_EVENT_GFX_LITTLE_BOY_FRLG,
    };

    for (i = 0; i < ARRAY_COUNT(regularTrainersOnLand); i++)
    {
        if (graphicsId == regularTrainersOnLand[i])
            return TRUE;
    }
    return FALSE;
}

static bool32 IsRegularWaterTrainer(u8 graphicsId)
{
    u32 i;
    u16 regularTrainersInWater[] =
    {
        OBJ_EVENT_GFX_SWIMMER_F,
        OBJ_EVENT_GFX_SWIMMER_M,
        OBJ_EVENT_GFX_TUBER_M_SWIMMING
    };

    for (i = 0; i < ARRAY_COUNT(regularTrainersInWater); i++)
    {
        if (graphicsId == regularTrainersInWater[i])
            return TRUE;
    }
    return FALSE;
}

static u8 GetResponseMovementTypeFromTrainerGraphicsId(u8 graphicsId)
{
    if (IsRegularLandTrainer(graphicsId) || IsRegularWaterTrainer(graphicsId))
        return MOVEMENT_TYPE_ROTATE_CLOCKWISE;

    return MOVEMENT_TYPE_FACE_DOWN;
}
#endif //FREE_MATCH_CALL

static u16 GetTrainerFlagFromScript(const u8 *script)
{
    if (script == NULL) return TRAINER_NONE;
    const u8 *operand;
    if (script[0] == SCR_OP_TRAINERBATTLE) operand = script + 3;
    else if (Script_MatchesCallNative(script, NativeVsSeekerRematchId, TRUE)) operand = script + 5;
    else return TRAINER_NONE;
    return operand[0] | (operand[1] << 8);
}

static void ClearAllTrainerRematchStates(void)
{
#if FREE_MATCH_CALL == FALSE
    u32 i;

    if (!CheckBagHasItem(ITEM_VS_SEEKER, 1))
        return;

    for (i = 0; i < ARRAY_COUNT(gSaveBlock1Ptr->trainerRematches); i++)
        gSaveBlock1Ptr->trainerRematches[i] = 0;
#endif //FREE_MATCH_CALL
}

#if FREE_MATCH_CALL == FALSE
static bool8 IsTrainerVisibleOnScreen(struct VsSeekerTrainerInfo * trainerInfo)
{
    s16 x;
    s16 y;

    PlayerGetDestCoords(&x, &y);
    x -= MAP_OFFSET;
    y -= MAP_OFFSET;

    if (   x - 7 <= trainerInfo->xCoord
        && x + 7 >= trainerInfo->xCoord
        && y - 5 <= trainerInfo->yCoord
        && y + 5 >= trainerInfo->yCoord
        && ObjectEventIdIsSane(trainerInfo->objectEventId))
        return TRUE;
    return FALSE;
}

static bool32 HasFightableTrainers(void)
{
    u32 i;

    for (i = 0; i < sVsSeeker->trainerCount; i++)
    {
        if (IsTrainerVisibleOnScreen(&sVsSeeker->trainerInfo[i]))
        {
            if (!HasTrainerBeenFought(sVsSeeker->trainerInfo[i].trainerIdx) || GetRematchTrainerIdVSSeeker(sVsSeeker->trainerInfo[i].trainerIdx))
                return TRUE;
        }
    }

    return FALSE;
}

static void StartTrainerObjectMovementScript(struct VsSeekerTrainerInfo * trainerInfo, const u8 * script)
{
    UnfreezeObjectEvent(&gObjectEvents[trainerInfo->objectEventId]);
    ScriptMovement_StartObjectMovementScript(trainerInfo->localId, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup, script);
}

#endif //FREE_MATCH_CALL

void NativeVsSeekerRematchId(struct ScriptContext *ctx)
{
    u16 trainerId = ScriptReadHalfword(ctx);
    if (ctx->breakOnTrainerBattle && HasTrainerBeenFought(trainerId) && !ShouldTryRematchBattleForTrainerId(trainerId))
        StopScript(ctx);
}


static void StartAllRespondantIdleMovements(void)
{
    if (sVsSeeker != NULL && sVsSeeker->native)
    {
        if (!NativeVsSeekerStartIdleMovements()) sVsSeeker->nativeFailed = TRUE;
        return;
    }

#if FREE_MATCH_CALL == FALSE
    s32 i;
    s32 j;

    for (i = 0; i < sVsSeeker->numRematchableTrainers; i++)
    {
        j = sVsSeeker->trainerIdxArray[i];
        struct ObjectEvent *objectEvent = &gObjectEvents[sVsSeeker->trainerInfo[j].objectEventId];

        if (ShouldChangeMovementForTrainerType(objectEvent->trainerType))
        {
            if (ObjectEventIdIsSane(sVsSeeker->trainerInfo[j].objectEventId))
                SetTrainerMovementType(objectEvent, sVsSeeker->runningBehaviourEtcArray[i]);
            TryOverrideTemplateCoordsForObjectEvent(objectEvent, sVsSeeker->runningBehaviourEtcArray[i]);
        }
        gSaveBlock1Ptr->trainerRematches[VsSeekerConvertLocalIdToTableId(sVsSeeker->trainerInfo[j].localId)] = GetRematchTrainerIdVSSeeker(sVsSeeker->trainerInfo[j].trainerIdx);
    }
#endif //FREE_MATCH_CALL
}

/* Explicit original native provider. Ordinary item dispatch remains unchanged. */
static EWRAM_DATA u32 sNativeVsSeekerTicket = 0;

static bool8 NativeVsSeekerTaskOwned(u32 taskId)
{
    if (sVsSeeker == NULL || !sVsSeeker->native || taskId >= NUM_TASKS
     || sVsSeeker->taskId != taskId || !gTasks[taskId].isActive)
        return FALSE;
    u32 ticket = (u16)gTasks[taskId].data[14] | ((u32)(u16)gTasks[taskId].data[15] << 16);
    return ticket == sVsSeeker->ticket
        && (gTasks[taskId].func == Task_VsSeekerFrameCountdown
         || gTasks[taskId].func == Task_VsSeeker_PlaySoundAndGetResponseCode
         || gTasks[taskId].func == Task_VsSeeker_ShowResponseToPlayer);
}

static bool8 NativeVsSeekerTaskCurrent(u32 taskId)
{
    if (taskId >= NUM_TASKS || sVsSeeker == NULL || !gTasks[taskId].isActive
     || sVsSeeker->taskId != taskId)
    {ADMG_StateAccessRejected(taskId, __func__); return FALSE;}
    if (!sVsSeeker->native) return TRUE;
    if (NativeVsSeekerTaskOwned(taskId) && !sVsSeeker->nativeFailed
     && ADMG_IsNativeRematchContextCurrent(&sVsSeeker->nativeContext))
        return TRUE;
    ADMG_StateAccessRejected(taskId, __func__);
    /* Never destroy an unrelated reused task or alter the new map's objects. */
    if (NativeVsSeekerTaskOwned(taskId)) DestroyTask(taskId);
    Free(sVsSeeker); sVsSeeker = NULL;
    return FALSE;
}

static bool8 NativeVsSeekerActorCurrent(u32 index)
{
    if (sVsSeeker == NULL || !sVsSeeker->native
     || sVsSeeker->trainerCount > ADMG_REMATCH_COHORT_COUNT
     || index >= sVsSeeker->trainerCount
     || gSaveBlock1Ptr == NULL || gMapHeader.events == NULL
     || gMapHeader.events->objectEventCount > OBJECT_EVENT_TEMPLATES_COUNT
     || !ADMG_IsNativeRematchContextCurrent(&sVsSeeker->nativeContext))
        return FALSE;
    const struct VsSeekerTrainerInfo *info = &sVsSeeker->trainerInfo[index];
    const struct ADMG_RematchActor *actor = &sVsSeeker->nativeActors[index];
    if (info->objectEventId > ADMG_REMATCH_NO_OBJECT || actor->trainerId != info->trainerIdx
     || actor->localId != info->localId || actor->objectId != info->objectEventId
     || info->templateIndex >= gMapHeader.events->objectEventCount
     || gSaveBlock1Ptr->objectEventTemplates[info->templateIndex].localId != info->localId
     || actor->visible > TRUE || (actor->visible && info->objectEventId == ADMG_REMATCH_NO_OBJECT))
        return FALSE;
    u16 current;
    if (!ADMG_TryGetNativeRematchNpcTrainer(sVsSeeker->nativeContext.mapGroup,
            sVsSeeker->nativeContext.mapNum, info->localId, &current) || current != info->trainerIdx)
        return FALSE;
    if (info->objectEventId < OBJECT_EVENTS_COUNT
     && (!ObjectEventIdIsSane(info->objectEventId)
      || gObjectEvents[info->objectEventId].localId != info->localId
      || gObjectEvents[info->objectEventId].graphicsId != info->graphicsId))
        return FALSE;
    return TRUE;
}

static bool8 NativeVsSeekerRosterCurrent(void)
{
    if (sVsSeeker == NULL || !sVsSeeker->native || sVsSeeker->trainerCount > ADMG_REMATCH_COHORT_COUNT
     || !ADMG_IsNativeRematchContextCurrent(&sVsSeeker->nativeContext)) return FALSE;
    for (u32 i = 0; i < sVsSeeker->trainerCount; ++i)
        if (!NativeVsSeekerActorCurrent(i)) return FALSE;
    return TRUE;
}

static bool8 NativeVsSeekerHasFightableTrainers(void)
{
    if (!NativeVsSeekerRosterCurrent()) {sVsSeeker->nativeFailed = TRUE; return FALSE;}
    for (u32 i = 0; i < sVsSeeker->trainerCount; ++i)
    {
        u16 flag, family;
        u8 position;
        bool8 fought;
        if (!sVsSeeker->nativeActors[i].visible) continue;
        if (!ADMG_TryGetTrainerDefeatFlagId(sVsSeeker->trainerInfo[i].trainerIdx, &flag)
         || !ADMG_TryReadFlag(flag, &fought)) {sVsSeeker->nativeFailed = TRUE; return FALSE;}
        if (!fought) return TRUE;
        u64 generation = ADMG_BeginStateAccessCheck();
        bool8 found = ADMG_TryGetNativeRematchProgress(sVsSeeker->trainerInfo[i].trainerIdx, &family, &position);
        if (!ADMG_EndStateAccessCheck(generation)) {sVsSeeker->nativeFailed = TRUE; return FALSE;}
        if (found && position != 0) return TRUE;
    }
    return FALSE;
}

static u8 NativeVsSeekerCanUse(void)
{
    u16 counter;
    if (!ADMG_TryReadNativeVsSeekerCounter(&sVsSeeker->nativeContext, &counter))
    {sVsSeeker->nativeFailed = TRUE; return VSSEEKER_NO_ONE_IN_RANGE;}
    if ((counter & 0xFF) == VSSEEKER_RECHARGE_STEPS)
        return NativeVsSeekerHasFightableTrainers() ? VSSEEKER_CAN_USE : VSSEEKER_NO_ONE_IN_RANGE;
    ConvertIntToDecimalStringN(gStringVar1, VSSEEKER_RECHARGE_STEPS - (counter & 0xFF), STR_CONV_MODE_LEFT_ALIGN, 3);
    return VSSEEKER_NOT_CHARGED;
}

static bool8 NativeVsSeekerStartMovement(u32 index, const u8 *script)
{
    if (!NativeVsSeekerActorCurrent(index)
     || !sVsSeeker->nativeActors[index].visible) return FALSE;
    const struct VsSeekerTrainerInfo *info = &sVsSeeker->trainerInfo[index];
    UnfreezeObjectEvent(&gObjectEvents[info->objectEventId]);
    return ScriptMovement_StartObjectMovementScript(info->localId, sVsSeeker->nativeContext.mapNum,
                sVsSeeker->nativeContext.mapGroup, script) == FALSE;
}

static u8 NativeVsSeekerGetResponse(void)
{
    bool8 unfought = FALSE, wants = FALSE;
    if (!NativeVsSeekerRosterCurrent()) goto failed;
    sVsSeeker->numRematchableTrainers = 0; sVsSeeker->nativeYes = 0;
    for (u32 i = 0; i < sVsSeeker->trainerCount; ++i)
    {
        u32 response;
        u8 position, movement;
        if (!ADMG_TryGetNativeRematchResponse(&sVsSeeker->nativeContext, sVsSeeker->nativeActors,
                sVsSeeker->trainerCount, i, sVsSeeker->nativeYes, &response, &position)) goto failed;
        if (response == ADMG_REMATCH_INVISIBLE) continue;
        if (response == ADMG_REMATCH_UNFOUGHT)
        {
            if (!NativeVsSeekerStartMovement(i, sMovementScript_TrainerUnfought)) goto failed;
            unfought = TRUE;
        }
        else if (response == ADMG_REMATCH_NO)
        {
            if (!NativeVsSeekerStartMovement(i, sMovementScript_TrainerNoRematch)) goto failed;
        }
        else
        {
            if (!ADMG_TryGetNativeRematchMovement(sVsSeeker->trainerInfo[i].graphicsId, &movement)
             || !NativeVsSeekerActorCurrent(i)) goto failed;
            /* Original native readiness is visible before the response
             * movement; grouped idle writes remain a separate later effect. */
            if (!ADMG_TryWriteNativeReadiness(&sVsSeeker->nativeContext,
                    sVsSeeker->trainerInfo[i].localId, position)) goto failed;
            ShiftStillObjectEventCoords(&gObjectEvents[sVsSeeker->trainerInfo[i].objectEventId]);
            if (!NativeVsSeekerStartMovement(i, sMovementScript_TrainerRematch)) goto failed;
            sVsSeeker->trainerIdxArray[sVsSeeker->numRematchableTrainers] = sVsSeeker->trainerInfo[i].trainerIdx;
            sVsSeeker->runningBehaviourEtcArray[sVsSeeker->numRematchableTrainers++] = movement;
            sVsSeeker->nativeYes |= (u64)1 << i;
            wants = TRUE;
        }
    }
    if (wants)
    {
        PlaySE(SE_PIN);
        if (!ADMG_TryBeginNativeReadiness(&sVsSeeker->nativeContext)) goto failed;
        return VSSEEKER_RESPONSE_FOUND_REMATCHES;
    }
    return unfought ? VSSEEKER_RESPONSE_UNFOUGHT_TRAINERS : VSSEEKER_RESPONSE_NO_RESPONSE;
failed:
    if (sVsSeeker != NULL) sVsSeeker->nativeFailed = TRUE;
    ADMG_StateAccessRejected(0, __func__);
    return VSSEEKER_RESPONSE_NO_RESPONSE;
}

static bool8 NativeVsSeekerStartIdleMovements(void)
{
    if (!NativeVsSeekerRosterCurrent() || sVsSeeker->numRematchableTrainers > sVsSeeker->trainerCount) return FALSE;
    /* Preflight every affected template, including unloaded duplicates, before
     * the first movement/readiness effect. No arbitrary graphics are dropped. */
    for (u32 j = 0; j < sVsSeeker->trainerCount; ++j)
    {
        for (u32 i = 0; i < sVsSeeker->numRematchableTrainers; ++i)
        {
            if (sVsSeeker->trainerInfo[j].trainerIdx != sVsSeeker->trainerIdxArray[i]) continue;
            u8 movement;
            if (!ADMG_TryGetNativeRematchMovement(sVsSeeker->trainerInfo[j].graphicsId, &movement)
             || !NativeVsSeekerActorCurrent(j)) return FALSE;
            break;
        }
    }
    for (u32 i = 0; i < sVsSeeker->numRematchableTrainers; ++i)
    {
        for (u32 j = 0; j < sVsSeeker->trainerCount; ++j)
        {
            const struct VsSeekerTrainerInfo *info = &sVsSeeker->trainerInfo[j];
            if (info->trainerIdx != sVsSeeker->trainerIdxArray[i]) continue;
            u8 position, movement = sVsSeeker->runningBehaviourEtcArray[i];
            u16 family;
            if (!ADMG_TryGetNativeRematchProgress(info->trainerIdx, &family, &position)
             || !NativeVsSeekerActorCurrent(j)) return FALSE;
            if (info->objectEventId < OBJECT_EVENTS_COUNT)
                SetTrainerMovementType(&gObjectEvents[info->objectEventId], movement);
            gSaveBlock1Ptr->objectEventTemplates[info->templateIndex].movementType = movement;
            if (!ADMG_TryWriteNativeReadiness(&sVsSeeker->nativeContext, info->localId, position)) return FALSE;
        }
    }
    return TRUE;
}

static bool8 InitVsSeekerField(u32 taskId, const struct ADMG_RematchContext *nativeContext)
{
    if (taskId >= NUM_TASKS || !gTasks[taskId].isActive || sVsSeeker != NULL
     || gSaveBlock1Ptr == NULL || gMapHeader.events == NULL
     || gMapHeader.events->objectEventCount > OBJECT_EVENT_TEMPLATES_COUNT
     || (nativeContext != NULL && (!ADMG_IsNativeRematchContextCurrent(nativeContext) || sNativeVsSeekerTicket == 0xFFFFFFFF)))
    {ADMG_StateAccessRejected(taskId, __func__); return FALSE;}
    sVsSeeker = AllocZeroed(sizeof(struct VsSeekerStruct));
    if (sVsSeeker == NULL) {ADMG_StateAccessRejected(taskId, __func__); return FALSE;}
    sVsSeeker->native = nativeContext != NULL; sVsSeeker->taskId = taskId;
    if (nativeContext != NULL) {sVsSeeker->nativeContext = *nativeContext; sVsSeeker->ticket = ++sNativeVsSeekerTicket;}
    if (!GatherNearbyTrainerInfo()) {Free(sVsSeeker); sVsSeeker = NULL; return FALSE;}
    u32 response = CanUseVsSeeker();
    if (sVsSeeker->nativeFailed) {Free(sVsSeeker); sVsSeeker = NULL; return FALSE;}
    for (u32 i = 0; i < NUM_TASK_DATA; ++i) gTasks[taskId].data[i] = 0;
    if (response == VSSEEKER_NOT_CHARGED || response == VSSEEKER_NO_ONE_IN_RANGE)
    {
        Free(sVsSeeker); sVsSeeker = NULL;
        DisplayItemMessageOnField(taskId, response == VSSEEKER_NOT_CHARGED
            ? VSSeeker_Text_BatteryNotChargedNeedXSteps : VSSeeker_Text_NoTrainersWithinRange,
            Task_ItemUse_CloseMessageBoxAndReturnToField_VsSeeker);
    }
    else
    {
        if (nativeContext != NULL)
        {
            gTasks[taskId].data[14] = (u16)sVsSeeker->ticket;
            gTasks[taskId].data[15] = (u16)(sVsSeeker->ticket >> 16);
        }
        FieldEffectStart(FLDEFF_USE_VS_SEEKER);
        gTasks[taskId].func = Task_VsSeekerFrameCountdown;
        gTasks[taskId].data[0] = 15;
    }
    return TRUE;
}

ADMG_NATIVE_ENTRY bool8 ADMG_TryClearNativeRematchFamilyMovement(u32 trainerId)
{
    struct ADMG_RematchContext context;
    u32 talked;
    u16 family;
    u8 position;
    u8 objectIds[OBJECT_EVENT_TEMPLATES_COUNT];
    bool8 matches[OBJECT_EVENT_TEMPLATES_COUNT] = {FALSE};
    if (!ADMG_TryGetNativeBattleNpc(trainerId, &context, &talked)) return FALSE;
    if (gSaveBlock1Ptr == NULL || gMapHeader.events == NULL
     || gMapHeader.events->objectEventCount > OBJECT_EVENT_TEMPLATES_COUNT
     || !ADMG_TryFindNativeRematch(trainerId, FALSE, &family, &position)) return FALSE;
    /* Preflight the complete family before RNG or movement/state effects. */
    for (u32 i = 0; i < gMapHeader.events->objectEventCount; ++i)
    {
        const struct ObjectEventTemplate *declared;
        struct ADMG_TrainerRef ref;
        struct ObjectEventTemplate *saved = &gSaveBlock1Ptr->objectEventTemplates[i];
        if (!ADMG_TryGetDeclaredObjectTemplate(context.mapGroup, context.mapNum, saved->localId, &declared)) return FALSE;
        if (declared->trainerType != TRAINER_TYPE_NORMAL && declared->trainerType != TRAINER_TYPE_BURIED) continue;
        u16 id = GetTrainerFlagFromScript(declared->script), npcFamily;
        if (!ADMG_TryResolveTrainer(id, &ref)) return FALSE;
        if (ref.owner != ADMG_TRAINER_KANTO) continue;
        u64 generation = ADMG_BeginStateAccessCheck();
        bool8 found = ADMG_TryFindNativeRematch(id, FALSE, &npcFamily, &position);
        if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
        if (!found || npcFamily != family) continue;
        u8 ready, objectId = OBJECT_EVENTS_COUNT;
        if (!ADMG_TryReadNativeReadiness(&context, saved->localId, &ready)) return FALSE;
        matches[i] = TRUE;
        if (TryGetObjectEventIdByLocalIdAndMap(saved->localId, context.mapNum, context.mapGroup, &objectId)
         || objectId >= OBJECT_EVENTS_COUNT || !gObjectEvents[objectId].active
         || gObjectEvents[objectId].localId != saved->localId
         || gObjectEvents[objectId].mapGroup != context.mapGroup
         || gObjectEvents[objectId].mapNum != context.mapNum) objectId = OBJECT_EVENTS_COUNT;
        if (objectId < OBJECT_EVENTS_COUNT)
        {
            struct ObjectEvent *object = &gObjectEvents[objectId];
            if (object->facingDirection >= ARRAY_COUNT(sFaceDirectionMovementTypeByFacingDirection))
            {ADMG_StateAccessRejected(object->facingDirection, __func__); return FALSE;}
            /* Battle sprites reuse the same pool. Their data is never an
             * overworld sprite binding merely because the old ID is retained. */
            if (!gMain.inBattle)
            {
                if (object->spriteId >= MAX_SPRITES
                 || !ADMG_IsNativeRematchSpritePair(object, &gSprites[object->spriteId])) return FALSE;
                if (gSprites[object->spriteId].anims == gADMG_NativeRematchAnimations)
                {
                    const struct ADMG_NativeRematchGraphics *graphics;
                    if (!ADMG_TryGetNativeRematchGraphics(object->graphicsId, &graphics)
                     || gSprites[object->spriteId].images != graphics->images) return FALSE;
                }
            }
        }
        objectIds[i] = objectId;
    }
    for (u32 i = 0; i < gMapHeader.events->objectEventCount; ++i)
    {
        if (!matches[i]) continue;
        if (!ADMG_IsNativeRematchContextCurrent(&context)) return FALSE;
        /* The original consumes this random direction even though the value
         * is discarded. Do not collapse repeated identities or RNG calls. */
        (void)GetRandomFaceDirectionMovementType();
        if (!ADMG_IsNativeRematchContextCurrent(&context)) return FALSE;
        struct ObjectEventTemplate *saved = &gSaveBlock1Ptr->objectEventTemplates[i];
        u8 objectId = objectIds[i];
        if (objectId < OBJECT_EVENTS_COUNT)
        {
            struct ObjectEvent *object = &gObjectEvents[objectId];
            if (!object->active || object->localId != saved->localId
             || object->mapGroup != context.mapGroup || object->mapNum != context.mapNum
             || object->facingDirection >= ARRAY_COUNT(sFaceDirectionMovementTypeByFacingDirection)) return FALSE;
            TryOverrideTemplateCoordsForObjectEvent(object, sFaceDirectionMovementTypeByFacingDirection[object->facingDirection]);
            if (!ADMG_IsNativeRematchContextCurrent(&context)
             || !ADMG_TryWriteNativeReadiness(&context, saved->localId, 0)) return FALSE;
            object->movementType = gSelectedObjectEvent == objectId
                ? sFaceDirectionMovementTypeByFacingDirection[object->facingDirection] : MOVEMENT_TYPE_FACE_DOWN;
            if (!gMain.inBattle && (object->spriteId >= MAX_SPRITES
             || !ADMG_TryRestoreNativeRematchSprite(object, &gSprites[object->spriteId]))) return FALSE;
        }
        else
        {
            /* All three native idle markers have the original south-facing
             * binding. Ordinary unloaded template defaults remain untouched. */
            if (ADMG_IsNativeRematchMovement(saved->movementType)) saved->movementType = MOVEMENT_TYPE_FACE_DOWN;
            if (!ADMG_TryWriteNativeReadiness(&context, saved->localId, 0)) return FALSE;
        }
    }
    return TRUE;
}

ADMG_NATIVE_ENTRY bool8 ADMG_TryInitNativeVsSeeker(u32 taskId)
{
    struct ADMG_RematchContext context;
    if (taskId >= NUM_TASKS || !gTasks[taskId].isActive || sVsSeeker != NULL)
    {ADMG_StateAccessRejected(taskId, __func__); return FALSE;}
    if (!ADMG_TryCaptureNativeRematchContext(&context)) return FALSE;
    return InitVsSeekerField(taskId, &context);
}

ADMG_NATIVE_ENTRY void ADMG_CancelNativeVsSeeker(void)
{
    if (sVsSeeker == NULL || !sVsSeeker->native) return;
    if (NativeVsSeekerTaskOwned(sVsSeeker->taskId)) DestroyTask(sVsSeeker->taskId);
    Free(sVsSeeker); sVsSeeker = NULL;
}

ADMG_NATIVE_ENTRY void ADMG_InitializeNativeRematchGameState(void)
{
    ADMG_CancelNativeVsSeeker();
    ADMG_DiscardNativeBattleAssociation();
    ADMG_InitNativeRematchState();
}

ADMG_NATIVE_ENTRY void ADMG_PrepareNativeRematchContinue(void)
{
    ADMG_CancelNativeVsSeeker();
    ADMG_DiscardNativeBattleAssociation();
    (void)ADMG_TryPrepareNativeRematchRestore();
}

static bool8 NativeRematchResetObjectValid(u32 objectId)
{
    if (objectId >= OBJECT_EVENTS_COUNT) return FALSE;
    struct ObjectEvent *object = &gObjectEvents[objectId];
    if (!object->active) return TRUE;
    if (gMain.inBattle || object->spriteId >= MAX_SPRITES
     || !ADMG_IsNativeRematchSpritePair(object, &gSprites[object->spriteId])) return FALSE;
    struct Sprite *sprite = &gSprites[object->spriteId];
    if (sprite->anims == gADMG_NativeRematchAnimations)
    {
        const struct ADMG_NativeRematchGraphics *graphics;
        if (!ADMG_TryGetNativeRematchGraphics(object->graphicsId, &graphics)
         || sprite->images != graphics->images) return FALSE;
    }
    return TRUE;
}

static bool8 NativeRematchResetObjectsValid(void)
{
    for (u32 i = 0; i < OBJECT_EVENTS_COUNT; ++i)
        if (ADMG_IsNativeRematchMovement(gObjectEvents[i].movementType)
         && !NativeRematchResetObjectValid(i)) return FALSE;
    return TRUE;
}

/* Reset operations retire their old scene before the next named RNG effect.
 * Epoch, allocation ticket and map/object containers identify that scene. */
static bool8 NativeRematchResetSceneCurrent(const struct SaveBlock1 *save,
    const struct MapEvents *events, u32 group, u32 number, u64 epoch, u32 ticket, bool8 bound)
{
    return gSaveBlock1Ptr == save && save != NULL && gMapHeader.events == events
        && (u32)save->location.mapGroup == group && (u32)save->location.mapNum == number
        && ADMG_GetNativeRematchEpoch() == epoch && sNativeVsSeekerTicket == ticket
        && ADMG_HasNativeRematchContext() == bound && !ADMG_IsNativeRematchRestorePending();
}

ADMG_NATIVE_ENTRY bool8 ADMG_TryResetNativeRematchObjectMovements(void)
{
    if (gSaveBlock1Ptr == NULL || ADMG_IsNativeRematchRestorePending()
     || !NativeRematchResetObjectsValid()) return FALSE;
    const struct SaveBlock1 *save = gSaveBlock1Ptr;
    const struct MapEvents *events = gMapHeader.events;
    u32 group = (u32)save->location.mapGroup, number = (u32)save->location.mapNum;
    u64 epoch = ADMG_GetNativeRematchEpoch();
    u32 ticket = sNativeVsSeekerTicket;
    bool8 bound = ADMG_HasNativeRematchContext();
    for (u32 i = 0; i < OBJECT_EVENTS_COUNT; ++i)
    {
        struct ObjectEvent *object = &gObjectEvents[i];
        if (!ADMG_IsNativeRematchMovement(object->movementType)) continue;
        u8 movement = GetRandomFaceDirectionMovementType();
        if (!NativeRematchResetSceneCurrent(save, events, group, number, epoch, ticket, bound)
         || !NativeRematchResetObjectValid(i)) return FALSE;
        /* Original RNG consumption includes inactive native movement slots. */
        if (!object->active) continue;
        struct Sprite *sprite = &gSprites[object->spriteId];
        sprite->x2 = sprite->y2 = 0;
        SetTrainerMovementType(object, movement);
    }
    return TRUE;
}

ADMG_NATIVE_ENTRY bool8 ADMG_TryResetNativeRematchTemplateMovements(void)
{
    bool8 matches[OBJECT_EVENT_TEMPLATES_COUNT] = {FALSE};
    u8 objectIds[OBJECT_EVENT_TEMPLATES_COUNT];
    if (gSaveBlock1Ptr == NULL || gMapHeader.events == NULL
     || gMapHeader.events->objectEventCount > OBJECT_EVENT_TEMPLATES_COUNT
     || gMain.inBattle || ADMG_IsNativeRematchRestorePending()) return FALSE;
    const struct SaveBlock1 *save = gSaveBlock1Ptr;
    const struct MapEvents *events = gMapHeader.events;
    u32 group = (u32)save->location.mapGroup, number = (u32)save->location.mapNum;
    u64 epoch = ADMG_GetNativeRematchEpoch();
    u32 ticket = sNativeVsSeekerTicket;
    bool8 bound = ADMG_HasNativeRematchContext();
    for (u32 i = 0; i < events->objectEventCount; ++i)
    {
        const struct ObjectEventTemplate *declared;
        const struct ObjectEventTemplate *saved = &save->objectEventTemplates[i];
        if (!ADMG_IsNativeRematchMovement(saved->movementType)) continue;
        if (!ADMG_TryGetDeclaredObjectTemplate(group, number, saved->localId, &declared)) return FALSE;
        if (declared->trainerType != TRAINER_TYPE_NORMAL && declared->trainerType != TRAINER_TYPE_BURIED) continue;
        u8 objectId = OBJECT_EVENTS_COUNT;
        if (TryGetObjectEventIdByLocalIdAndMap(saved->localId, number, group, &objectId)) objectId = OBJECT_EVENTS_COUNT;
        if (objectId < OBJECT_EVENTS_COUNT)
        {
            const struct ObjectEvent *object = &gObjectEvents[objectId];
            if (object->localId != saved->localId || object->mapGroup != group || object->mapNum != number
             || !NativeRematchResetObjectValid(objectId)) return FALSE;
        }
        else if (objectId != OBJECT_EVENTS_COUNT) return FALSE;
        matches[i] = TRUE; objectIds[i] = objectId;
    }
    for (u32 i = 0; i < events->objectEventCount; ++i)
    {
        if (!matches[i]) continue;
        u8 movement = GetRandomFaceDirectionMovementType();
        if (!NativeRematchResetSceneCurrent(save, events, group, number, epoch, ticket, bound)) return FALSE;
        u8 objectId = objectIds[i];
        struct ObjectEventTemplate *saved = &gSaveBlock1Ptr->objectEventTemplates[i];
        if (objectId < OBJECT_EVENTS_COUNT)
        {
            struct ObjectEvent *object = &gObjectEvents[objectId];
            if (object->localId != saved->localId || !NativeRematchResetObjectValid(objectId)) return FALSE;
            if (object->active) SetTrainerMovementType(object, movement);
        }
        /* Original saved template reset occurs even when its actor is unloaded. */
        saved->movementType = movement;
    }
    return TRUE;
}

ADMG_NATIVE_ENTRY bool8 ADMG_TryResetNativeRematchMap(void)
{
    if (!ADMG_HasNativeRematchContext()) return !ADMG_IsNativeRematchRestorePending();
    if (!NativeRematchResetObjectsValid()) return FALSE;
    ADMG_CancelNativeVsSeeker();
    ADMG_DiscardNativeBattleAssociation();
    if (!ADMG_TryResetNativeRematches()) return FALSE;
    return ADMG_TryResetNativeRematchObjectMovements();
}
