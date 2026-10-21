#include "global.h"
#include "admg_state.h"
#include "event_data.h"
#include "admg_rematch_state.h"

/* Passive logical game fields. They are not serialized by this scope and do
 * not grow or reinterpret any accepted legacy save structure. */
EWRAM_DATA u8 gADMG_KantoNpcReadiness[ADMG_NATIVE_READINESS_COUNT] = {0};
EWRAM_DATA u16 gADMG_KantoVsSeekerCounter = 0;
static EWRAM_DATA u64 sEpoch = 0;
static EWRAM_DATA bool8 sBound = FALSE;
static EWRAM_DATA bool8 sEpochExhausted = FALSE;
static EWRAM_DATA bool8 sRestorePending = FALSE;
static EWRAM_DATA MapId sMapId = 0;

static bool8 RejectState(u32 id, const char *operation)
{
    ADMG_StateAccessRejected(id, operation);
    return FALSE;
}

static bool8 BackingAvailable(void)
{
    if (gSaveBlock1Ptr == NULL || sEpochExhausted
     || !ADMG_IsMapIdValid(gSaveBlock1Ptr->location.mapId)
     || !ADMG_IsFlagIdValid(ADMG_KANTO_FLAG_SYS_VS_SEEKER_CHARGING))
        return RejectState(0, __func__);
    return TRUE;
}

static bool8 StateAvailable(void)
{
    if (sRestorePending)
        return RejectState(0, __func__);
    return BackingAvailable();
}

static bool8 RecordValid(void)
{
    u32 i;
    if ((gADMG_KantoVsSeekerCounter & 0xFF) > 100 || (gADMG_KantoVsSeekerCounter >> 8) > 100)
        return RejectState(gADMG_KantoVsSeekerCounter, __func__);
    for (i = 0; i < ADMG_NATIVE_READINESS_COUNT; ++i)
        if (gADMG_KantoNpcReadiness[i] > 5)
            return RejectState(i, __func__);
    return TRUE;
}

static bool8 AdvanceEpoch(void)
{
    if (sEpoch == ~(u64)0)
    {
        sEpochExhausted = TRUE;
        return RejectState(0, __func__);
    }
    ++sEpoch;
    sBound = FALSE;
    return TRUE;
}

bool8 ADMG_HasNativeRematchContext(void)
{
    return sBound && !sRestorePending && !sEpochExhausted;
}

bool8 ADMG_IsNativeRematchRestorePending(void)
{
    return sRestorePending;
}

u64 ADMG_GetNativeRematchEpoch(void)
{
    return sEpoch;
}

bool8 ADMG_TryPrepareNativeRematchRestore(void)
{
    /* Preserve the fields for the admitted-record boundary. Neither volatile
     * bytes nor a matching old map constitutes proof of a restored record. */
    sRestorePending = TRUE;
    return AdvanceEpoch();
}

static void Capture(struct ADMG_RematchContext *context)
{
    context->owner = ADMG_TRAINER_KANTO;
    context->service = ADMG_REMATCH_NATIVE_VS_SEEKER;
    context->instance = 0;
    context->selection = ADMG_REMATCH_NATIVE_SIX_SLOT;
    context->mapId = sMapId;
    context->epoch = sEpoch;
}

bool8 ADMG_IsNativeRematchContextCurrent(const struct ADMG_RematchContext *context)
{
    if (context == NULL || !StateAvailable() || !sBound
     || context->owner != ADMG_TRAINER_KANTO || context->service != ADMG_REMATCH_NATIVE_VS_SEEKER
     || context->instance != 0 || context->selection != ADMG_REMATCH_NATIVE_SIX_SLOT
     || context->epoch != sEpoch || context->mapId != sMapId
     || gSaveBlock1Ptr->location.mapId != sMapId)
        return RejectState(0, __func__);
    return TRUE;
}

bool8 ADMG_TryCaptureNativeRematchContext(struct ADMG_RematchContext *context)
{
    u32 i;
    if (context == NULL || !StateAvailable() || !RecordValid())
        return RejectState(0, __func__);
    if (sBound)
    {
        if (gSaveBlock1Ptr->location.mapId != sMapId)
            return RejectState(0, __func__);
    }
    else
    {
        /* Existing saved readiness requires the explicit resume boundary.
         * An ordinary capture must not silently adopt an unknown association. */
        for (i = 0; i < ADMG_NATIVE_READINESS_COUNT; ++i)
            if (gADMG_KantoNpcReadiness[i] != 0)
                return RejectState(i, __func__);
        sMapId = gSaveBlock1Ptr->location.mapId;
        sBound = TRUE;
    }
    Capture(context);
    return TRUE;
}

bool8 ADMG_TryReadNativeReadiness(const struct ADMG_RematchContext *context, u32 index, u8 *value)
{
    if (value == NULL || index >= ADMG_NATIVE_READINESS_COUNT || !ADMG_IsNativeRematchContextCurrent(context)
     || gADMG_KantoNpcReadiness[index] > 5)
        return RejectState(index, __func__);
    *value = gADMG_KantoNpcReadiness[index];
    return TRUE;
}

bool8 ADMG_TryWriteNativeReadiness(const struct ADMG_RematchContext *context, u32 index, u32 value)
{
    if (index >= ADMG_NATIVE_READINESS_COUNT || value > 5 || !ADMG_IsNativeRematchContextCurrent(context))
        return RejectState(index, __func__);
    gADMG_KantoNpcReadiness[index] = value;
    return TRUE;
}

bool8 ADMG_TryReadNativeVsSeekerCounter(const struct ADMG_RematchContext *context, u16 *value)
{
    if (value == NULL || !ADMG_IsNativeRematchContextCurrent(context) || !RecordValid())
        return RejectState(0, __func__);
    *value = gADMG_KantoVsSeekerCounter;
    return TRUE;
}

bool8 ADMG_TryResetNativeBagCharge(const struct ADMG_RematchContext *context)
{
    if (!ADMG_IsNativeRematchContextCurrent(context) || !RecordValid())
        return FALSE;
    gADMG_KantoVsSeekerCounter &= 0xFF00;
    return TRUE;
}

bool8 ADMG_TryBeginNativeReadiness(const struct ADMG_RematchContext *context)
{
    if (!ADMG_IsNativeRematchContextCurrent(context) || !RecordValid())
        return FALSE;
    FlagSet(ADMG_KANTO_FLAG_SYS_VS_SEEKER_CHARGING);
    if (!ADMG_IsNativeRematchContextCurrent(context) || !RecordValid())
        return FALSE;
    gADMG_KantoVsSeekerCounter &= 0x00FF;
    return TRUE;
}

bool8 ADMG_TryStepNativeVsSeeker(const struct ADMG_RematchContext *context, u32 hasItem, bool8 *expired)
{
    u16 counter;
    bool8 charging;
    bool8 didExpire = FALSE;
    if (expired == NULL || hasItem > TRUE || !ADMG_IsNativeRematchContextCurrent(context) || !RecordValid())
        return RejectState(0, __func__);
    counter = gADMG_KantoVsSeekerCounter;
    if (hasItem && (counter & 0xFF) < 100)
        ++gADMG_KantoVsSeekerCounter;
    if (!ADMG_TryReadFlag(ADMG_KANTO_FLAG_SYS_VS_SEEKER_CHARGING, &charging))
    {
        /* Preserve failed-read behavior without overwriting a new context
         * legitimately established by a nested accessor hook. */
        if (ADMG_IsNativeRematchContextCurrent(context))
            gADMG_KantoVsSeekerCounter = counter;
        return RejectState(0, __func__);
    }
    if (!ADMG_IsNativeRematchContextCurrent(context) || !RecordValid())
        return FALSE;
    if (charging)
    {
        if ((gADMG_KantoVsSeekerCounter >> 8) < 100)
            gADMG_KantoVsSeekerCounter += 0x100;
        if ((gADMG_KantoVsSeekerCounter >> 8) == 100)
        {
            u32 i;
            FlagClear(ADMG_KANTO_FLAG_SYS_VS_SEEKER_CHARGING);
            if (!ADMG_IsNativeRematchContextCurrent(context) || !RecordValid())
                return FALSE;
            gADMG_KantoVsSeekerCounter &= 0xFF;
            for (i = 0; i < ADMG_NATIVE_READINESS_COUNT; ++i)
                gADMG_KantoNpcReadiness[i] = 0;
            didExpire = TRUE;
        }
    }
    *expired = didExpire;
    return TRUE;
}

bool8 ADMG_TryResetNativeRematches(void)
{
    u32 i;
    u64 resetEpoch;
    if (!StateAvailable() || !RecordValid() || !AdvanceEpoch())
        return FALSE;
    resetEpoch = sEpoch;
    FlagClear(ADMG_KANTO_FLAG_SYS_VS_SEEKER_CHARGING);
    if (sEpoch != resetEpoch || sBound || !StateAvailable() || !RecordValid())
        return RejectState(0, __func__);
    gADMG_KantoVsSeekerCounter &= 0xFF;
    for (i = 0; i < ADMG_NATIVE_READINESS_COUNT; ++i)
        gADMG_KantoNpcReadiness[i] = 0;
    return TRUE;
}

bool8 ADMG_TryResumeNativeRematches(struct ADMG_RematchContext *context)
{
    /* This primitive requires the coherent main-record/native-record pair.
     * C4 must separately prove the saved descriptor association before wiring
     * this into normal continue. It is not a serializer or a restore policy. */
    if (context == NULL || !BackingAvailable() || !RecordValid() || !AdvanceEpoch())
        return RejectState(0, __func__);
    sMapId = gSaveBlock1Ptr->location.mapId;
    sBound = TRUE;
    sRestorePending = FALSE;
    Capture(context);
    return TRUE;
}

void ADMG_InitNativeRematchState(void)
{
    u32 i;
    if (!AdvanceEpoch())
        return;
    gADMG_KantoVsSeekerCounter = 0;
    for (i = 0; i < ADMG_NATIVE_READINESS_COUNT; ++i)
        gADMG_KantoNpcReadiness[i] = 0;
    sRestorePending = FALSE;
}
