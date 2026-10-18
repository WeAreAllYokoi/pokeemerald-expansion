#ifndef GUARD_VS_SEEKER_H
#define GUARD_VS_SEEKER_H

#include "global.h"

struct ScriptContext;

void Task_InitVsSeekerAndCheckForTrainersOnScreen(u8 taskId);
bool8 UpdateVsSeekerStepCounter(void);
void MapResetTrainerRematches(u16 mapGroup, u16 mapNum);
void ClearRematchMovementByTrainerId(void);
u16 GetRematchTrainerIdVSSeeker(enum TrainerId trainerId);
bool32 IsVsSeekerEnabled(void);
void NativeVsSeekerRematchId(struct ScriptContext *ctx);

#define VSSEEKER_RECHARGE_STEPS 100

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
