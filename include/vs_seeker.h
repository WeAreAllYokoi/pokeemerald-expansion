#ifndef GUARD_VS_SEEKER_H
#define GUARD_VS_SEEKER_H

#include "global.h"

struct ScriptContext;

void Task_InitVsSeekerAndCheckForTrainersOnScreen(u8 taskId);
bool8 UpdateVsSeekerStepCounter(void);
void MapResetTrainerRematches(MapId mapId);
void ClearRematchMovementByTrainerId(void);
u16 GetRematchTrainerIdVSSeeker(enum TrainerId trainerId);
bool32 IsVsSeekerEnabled(void);
void NativeVsSeekerRematchId(struct ScriptContext *ctx);

#define VSSEEKER_RECHARGE_STEPS 100

// Inspect nearby trainers or start a VS Seeker use from the field
struct ADMG_VsSeekerProbeRow
{
    u32 localId;
    u32 trainerId;
    u32 objectEventId;    // OBJECT_EVENTS_COUNT when the object is not spawned
    u32 visible;          // Spawned and within 7 tiles horizontally and 5 vertically of the player
    u32 fought;
    u32 rematchTrainerId; // GetRematchTrainerIdVSSeeker for a fought trainer, else 0
};

enum ADMG_VsSeekerProbeStatus
{
    ADMG_VSSEEKER_PROBE_NOT_CHARGED,     // the item's VSSEEKER_NOT_CHARGED
    ADMG_VSSEEKER_PROBE_NO_ONE_IN_RANGE, // VSSEEKER_NO_ONE_IN_RANGE
    ADMG_VSSEEKER_PROBE_CAN_USE,         // VSSEEKER_CAN_USE
    ADMG_VSSEEKER_PROBE_UNAVAILABLE,     // I_VS_SEEKER_CHARGING has no flag
    ADMG_VSSEEKER_PROBE_BUSY,            // a use is running
    ADMG_VSSEEKER_PROBE_FAILED,          // No save/map, allocation or gathering failure, or FREE_MATCH_CALL
};

enum ADMG_VsSeekerStartStatus
{
    ADMG_VSSEEKER_START_STARTED,
    ADMG_VSSEEKER_START_UNAVAILABLE,
    ADMG_VSSEEKER_START_BUSY,
    ADMG_VSSEEKER_START_FIELD_NOT_IDLE,
    ADMG_VSSEEKER_START_INVALID_LOCATION,
};

enum ADMG_VsSeekerResult
{
    ADMG_VSSEEKER_RESULT_NONE,
    ADMG_VSSEEKER_RESULT_PENDING,
    ADMG_VSSEEKER_RESULT_NOT_CHARGED,
    ADMG_VSSEEKER_RESULT_NO_ONE_IN_RANGE,
    ADMG_VSSEEKER_RESULT_NO_RESPONSE,
    ADMG_VSSEEKER_RESULT_UNFOUGHT_ONLY,
    ADMG_VSSEEKER_RESULT_REMATCHES,
    ADMG_VSSEEKER_RESULT_FAILED,
};

u32 ADMG_ProbeVsSeeker(struct ADMG_VsSeekerProbeRow *rows, u32 capacity, u32 *count);
u32 ADMG_StartVsSeekerUse(void);
u32 ADMG_GetVsSeekerResult(void);
bool32 ADMG_IsValidLocationForVsSeeker(void);

/* Explicit native provider; does not change ordinary item/default dispatch. */
bool8 ADMG_TryInitNativeVsSeeker(u32 taskId);
bool8 ADMG_TryClearNativeRematchFamilyMovement(u32 trainerId);

void ADMG_CancelNativeVsSeeker(void);
void ADMG_InitializeNativeRematchGameState(void);
void ADMG_PrepareNativeRematchContinue(void);
bool8 ADMG_TryResetNativeRematchMap(void);
bool8 ADMG_TryResetNativeRematchObjectMovements(void);
bool8 ADMG_TryResetNativeRematchTemplateMovements(void);

#endif //GUARD_VS_SEEKER_H
