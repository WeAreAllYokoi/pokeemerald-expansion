#ifndef ADMG_REMATCH_RESPONSE_H
#define ADMG_REMATCH_RESPONSE_H

#include "admg_rematch_state.h"

struct ADMG_RematchActor
{
    u32 trainerId;
    u32 localId;
    u32 objectId;
    u32 visible;
};

enum ADMG_RematchResponse
{
    ADMG_REMATCH_INVISIBLE,
    ADMG_REMATCH_UNFOUGHT,
    ADMG_REMATCH_NO,
    ADMG_REMATCH_YES,
};

/* Logical map cohorts include unloaded/offscreen NPC templates. Physical
 * object slots keep their existing independent sixteen-object capacity. */
enum { ADMG_REMATCH_COHORT_COUNT = OBJECT_EVENT_TEMPLATES_COUNT,
       ADMG_REMATCH_NO_OBJECT = OBJECT_EVENTS_COUNT };

/* A controller calls this in actor order, applying each original movement
 * effect before proceeding to the next actor. Earlier YES decisions are
 * represented by actor bits; RNG advances even for a forced cohort answer. */
ADMG_NATIVE_ENTRY bool8 ADMG_TryGetNativeRematchResponse(const struct ADMG_RematchContext *context,
    const struct ADMG_RematchActor *actors, u32 count, u32 index, u64 earlierYes,
    u32 *response, u8 *position);

#endif
