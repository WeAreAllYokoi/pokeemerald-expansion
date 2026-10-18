#include "global.h"
#include "admg_rematch_movement.h"
#include "admg_state.h"
#include "event_object_movement.h"
#include "constants/event_objects.h"

static bool8 RejectRematchMovement(u32 id, const char *operation)
{
    ADMG_StateAccessRejected(id, operation);
    return FALSE;
}

bool8 ADMG_IsNativeRematchMovement(u32 type)
{
    return type >= ADMG_NATIVE_REMATCH_STOP && type <= ADMG_NATIVE_REMATCH_SWIM;
}

bool8 ADMG_TryGetNativeRematchGraphics(u32 graphicsId, const struct ADMG_NativeRematchGraphics **graphics)
{
    u32 resolved;
    u32 i;
    if (graphics == NULL || graphicsId > 0xFFFF)
        return RejectRematchMovement(graphicsId, __func__);
    resolved = graphicsId;
    if (graphicsId >= OBJ_EVENT_GFX_VARS && graphicsId <= OBJ_EVENT_GFX_VAR_F
     && !ADMG_TryReadObjectGraphicsVar(graphicsId - OBJ_EVENT_GFX_VARS, &resolved))
        return FALSE;
    /* Reject an unrecognized producer before calling the general graphics
     * lookup. Do not turn a bad ID into that lookup's ordinary fallback. */
    for (i = 0; i < gADMG_NativeRematchGraphicsCount; ++i)
    {
        const struct ADMG_NativeRematchGraphics *row = &gADMG_NativeRematchGraphics[i];
        if (row->graphicsId != resolved)
            continue;
        if (row->info == NULL || row->images == NULL || row->frames < 10 || row->frameBytes == 0
         || row->images[0].size != row->frameBytes || !row->images[0].relativeFrames
         || row->info->size != row->frameBytes || !ADMG_IsNativeRematchMovement(row->movementType)
         || GetObjectEventGraphicsInfo(resolved) != row->info)
            return RejectRematchMovement(graphicsId, __func__);
        *graphics = row;
        return TRUE;
    }
    return RejectRematchMovement(graphicsId, __func__);
}

bool8 ADMG_TryGetNativeRematchMovement(u32 graphicsId, u8 *movementType)
{
    const struct ADMG_NativeRematchGraphics *graphics;
    if (movementType == NULL || !ADMG_TryGetNativeRematchGraphics(graphicsId, &graphics))
        return RejectRematchMovement(graphicsId, __func__);
    /* Original dynamic graphics operands fall through the selector's STOP
     * case. Resolve the payload for safety without changing that policy. */
    *movementType = graphicsId == graphics->graphicsId ? graphics->movementType : ADMG_NATIVE_REMATCH_STOP;
    return TRUE;
}

bool8 ADMG_IsNativeRematchSpritePair(const struct ObjectEvent *object, const struct Sprite *sprite)
{
    u32 i;
    if (object == NULL || sprite == NULL)
        return RejectRematchMovement(0, __func__);
    /* Equality checks avoid dereferencing a foreign/unaligned object pointer. */
    for (i = 0; i < OBJECT_EVENTS_COUNT; ++i)
    {
        if (object != &gObjectEvents[i])
            continue;
        if (!object->active || object->spriteId >= MAX_SPRITES
         || sprite != &gSprites[object->spriteId] || sprite->data[0] != i)
            return RejectRematchMovement(i, __func__);
        return TRUE;
    }
    return RejectRematchMovement(OBJECT_EVENTS_COUNT, __func__);
}

bool8 ADMG_TryPrepareNativeRematchSprite(const struct ObjectEvent *object, struct Sprite *sprite)
{
    const struct ADMG_NativeRematchGraphics *graphics;
    u16 graphicsId;
    if (!ADMG_IsNativeRematchSpritePair(object, sprite))
        return FALSE;
    graphicsId = object->graphicsId;
    if (!ADMG_TryGetNativeRematchGraphics(graphicsId, &graphics))
        return FALSE;
    if (!ADMG_IsNativeRematchSpritePair(object, sprite) || object->graphicsId != graphicsId)
        return RejectRematchMovement(graphicsId, __func__);
    if ((sprite->anims != graphics->info->anims && sprite->anims != gADMG_NativeRematchAnimations)
     || (sprite->images != graphics->info->images && sprite->images != graphics->images))
        return RejectRematchMovement(object->graphicsId, __func__);
    /* Ordinary bindings are unchanged. Only this explicit native operation
     * selects the retained native frame payload and the added raise-hand entry. */
    sprite->images = graphics->images;
    sprite->anims = gADMG_NativeRematchAnimations;
    return TRUE;
}

bool8 ADMG_TryGetNativeRematchObjectForSprite(const struct Sprite *sprite, struct ObjectEvent **object)
{
    u32 i;
    if (object == NULL || sprite == NULL)
        return RejectRematchMovement(0, __func__);
    for (i = 0; i < MAX_SPRITES; ++i)
    {
        if (sprite != &gSprites[i])
            continue;
        if (sprite->data[0] < 0 || sprite->data[0] >= OBJECT_EVENTS_COUNT
         || !ADMG_IsNativeRematchSpritePair(&gObjectEvents[sprite->data[0]], sprite))
            return RejectRematchMovement(i, __func__);
        *object = &gObjectEvents[sprite->data[0]];
        return TRUE;
    }
    return RejectRematchMovement(MAX_SPRITES, __func__);
}

bool8 ADMG_TryRestoreNativeRematchSprite(const struct ObjectEvent *object, struct Sprite *sprite)
{
    const struct ADMG_NativeRematchGraphics *graphics;
    u16 graphicsId;
    if (!ADMG_IsNativeRematchSpritePair(object, sprite))
        return FALSE;
    if (sprite->anims != gADMG_NativeRematchAnimations)
        return TRUE;
    graphicsId = object->graphicsId;
    if (!ADMG_TryGetNativeRematchGraphics(graphicsId, &graphics))
        return FALSE;
    if (!ADMG_IsNativeRematchSpritePair(object, sprite) || object->graphicsId != graphicsId)
        return RejectRematchMovement(graphicsId, __func__);
    if (sprite->images != graphics->images)
        return RejectRematchMovement(object->graphicsId, __func__);
    sprite->images = graphics->info->images;
    sprite->anims = graphics->info->anims;
    return TRUE;
}
