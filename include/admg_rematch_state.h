#ifndef ADMG_REMATCH_STATE_H
#define ADMG_REMATCH_STATE_H

#include "admg_rematch.h"
#include "admg_trainer.h"

/* The built-in FR/LG provider owns one original VS Seeker record. These
 * constants identify this provider, not limits on mod-owned services. */
enum { ADMG_REMATCH_NATIVE_VS_SEEKER = 1, ADMG_REMATCH_NATIVE_SIX_SLOT = 1 };

/* Transient capture only. S3-E has no codec here; no wire layout is assigned.
 * A map binds NPC-local readiness, never trainer content identity. */
struct ADMG_RematchContext
{
    u32 owner;
    u32 service;
    u32 instance;
    u32 selection;
    u32 mapGroup;
    u32 mapNum;
    u64 epoch;
};

extern u8 gADMG_KantoNpcReadiness[ADMG_NATIVE_READINESS_COUNT];
extern u16 gADMG_KantoVsSeekerCounter;

/* Transient restore admission, not a saved activation setting. A new continue
 * cannot borrow the prior session's live fields, even on the same map. */
ADMG_NATIVE_ENTRY bool8 ADMG_HasNativeRematchContext(void);
ADMG_NATIVE_ENTRY bool8 ADMG_IsNativeRematchRestorePending(void);
ADMG_NATIVE_ENTRY u64 ADMG_GetNativeRematchEpoch(void);
ADMG_NATIVE_ENTRY bool8 ADMG_TryPrepareNativeRematchRestore(void);

ADMG_NATIVE_ENTRY bool8 ADMG_TryCaptureNativeRematchContext(struct ADMG_RematchContext *context);
ADMG_NATIVE_ENTRY bool8 ADMG_IsNativeRematchContextCurrent(const struct ADMG_RematchContext *context);
ADMG_NATIVE_ENTRY bool8 ADMG_TryReadNativeReadiness(const struct ADMG_RematchContext *context, u32 index, u8 *value);
ADMG_NATIVE_ENTRY bool8 ADMG_TryWriteNativeReadiness(const struct ADMG_RematchContext *context, u32 index, u32 value);
ADMG_NATIVE_ENTRY bool8 ADMG_TryReadNativeVsSeekerCounter(const struct ADMG_RematchContext *context, u16 *value);
ADMG_NATIVE_ENTRY bool8 ADMG_TryResetNativeBagCharge(const struct ADMG_RematchContext *context);
ADMG_NATIVE_ENTRY bool8 ADMG_TryBeginNativeReadiness(const struct ADMG_RematchContext *context);
ADMG_NATIVE_ENTRY bool8 ADMG_TryStepNativeVsSeeker(const struct ADMG_RematchContext *context, u32 hasItem, bool8 *expired);
ADMG_NATIVE_ENTRY bool8 ADMG_TryResetNativeRematches(void);
ADMG_NATIVE_ENTRY bool8 ADMG_TryResumeNativeRematches(struct ADMG_RematchContext *context);
/* S3-E must first validate the typed native record and coherent main snapshot.
 * This performs semantic NPC reconciliation, not decoding or save I/O. */
ADMG_NATIVE_ENTRY bool8 ADMG_TryValidateNativeRematchBindings(void);
ADMG_NATIVE_ENTRY bool8 ADMG_TryResumeNativeRematchBindings(struct ADMG_RematchContext *context);
ADMG_NATIVE_ENTRY void ADMG_InitNativeRematchState(void);

#endif
