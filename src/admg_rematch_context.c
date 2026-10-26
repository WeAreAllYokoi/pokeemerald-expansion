#include "global.h"
#include "admg_rematch.h"
#include "admg_state.h"
#include "overworld.h"
#include "constants/event_objects.h"

static bool8 RejectNpc(u32 id, const char *operation)
{
    ADMG_StateAccessRejected(id, operation);
    return FALSE;
}

static bool8 FindNpc(u32 mapGroup, u32 mapNum, u32 localId, bool8 saved,
                     const struct ObjectEventTemplate **result)
{
    const struct MapHeader *header;
    const struct ObjectEventTemplate *templates, *found = NULL;
    u32 count, i;
    if (localId == 0 || localId > 0xFF || !ADMG_IsMapIdValid(mapGroup, mapNum))
        return RejectNpc(localId, __func__);
    header = Overworld_GetMapHeaderByGroupAndId(mapGroup, mapNum);
    if (header == NULL || header->events == NULL)
        return RejectNpc(localId, __func__);
    count = header->events->objectEventCount;
    if (count > OBJECT_EVENT_TEMPLATES_COUNT)
        return RejectNpc(count, __func__);
    templates = header->events->objectEvents;
    if (saved && gSaveBlock1Ptr != NULL
     && gSaveBlock1Ptr->location.mapGroup == mapGroup && gSaveBlock1Ptr->location.mapNum == mapNum)
    {
        if (count > ARRAY_COUNT(gSaveBlock1Ptr->objectEventTemplates))
            return RejectNpc(count, __func__);
        templates = gSaveBlock1Ptr->objectEventTemplates;
    }
    if (templates == NULL && count != 0)
        return RejectNpc(localId, __func__);
    for (i = 0; i < count; ++i)
    {
        if (templates[i].localId == localId)
        {
            /* A duplicate binding cannot silently select or merge readiness. */
            if (found != NULL)
                return RejectNpc(localId, __func__);
            found = &templates[i];
        }
    }
    if (found == NULL)
        return RejectNpc(localId, __func__);
    *result = found;
    return TRUE;
}

static bool8 NextNpc(const struct ObjectEventTemplate *current,
                     const struct ObjectEventTemplate **next)
{
    if (current->kind == OBJ_KIND_CLONE)
        return FindNpc(current->targetMapGroup, current->targetMapNum, current->targetLocalId, FALSE, next);
    *next = current;
    return TRUE;
}

static bool8 ResolveObjectTemplate(u32 mapGroup, u32 mapNum, u32 localId, bool8 declared,
                       bool8 requireScript, const struct ObjectEventTemplate **template)
{
    const struct ObjectEventTemplate *current, *declaration, *slow, *fast;
    if (template == NULL)
        return RejectNpc(localId, __func__);
    if (!FindNpc(mapGroup, mapNum, localId, TRUE, &current))
        return FALSE;
    if (declared)
    {
        /* Provenance identifies a source NPC only after its destination local
         * ID has been reconciled with the restored map. It cannot manufacture
         * an NPC on a different map from a previous saved template array. */
        if (!FindNpc(mapGroup, mapNum, localId, FALSE, &declaration))
            return FALSE;
        if (current->kind != OBJ_KIND_CLONE)
            current = declaration;
    }
    slow = fast = current;
    while (current->kind == OBJ_KIND_CLONE)
    {
        if (!NextNpc(current, &current) || !NextNpc(slow, &slow)
         || !NextNpc(fast, &fast) || !NextNpc(fast, &fast))
            return FALSE;
        /* The immutable map graph is finite. Detect cycles without allocating
         * a new save field or imposing a guessed maximum clone-chain length. */
        if (slow == fast && slow->kind == OBJ_KIND_CLONE)
            return RejectNpc(localId, __func__);
    }
    if (requireScript && current->script == NULL)
        return RejectNpc(localId, __func__);
    *template = current;
    return TRUE;
}

static bool8 ResolveNpc(u32 mapGroup, u32 mapNum, u32 localId, bool8 declared,
                       const struct ObjectEventTemplate **template)
{
    return ResolveObjectTemplate(mapGroup, mapNum, localId, declared, TRUE, template);
}

bool8 ADMG_TryGetDeclaredObjectTemplate(u32 mapGroup, u32 mapNum, u32 localId,
                                      const struct ObjectEventTemplate **template)
{
    /* Scenery may legitimately have no script. Trainer-specific readers retain
     * their existing non-null script contract through ResolveNpc. */
    return ResolveObjectTemplate(mapGroup, mapNum, localId, TRUE, FALSE, template);
}

bool8 ADMG_TryGetRematchNpcTemplate(u32 mapGroup, u32 mapNum, u32 localId,
                                  const struct ObjectEventTemplate **template)
{
    return ResolveNpc(mapGroup, mapNum, localId, FALSE, template);
}

bool8 ADMG_TryGetDeclaredRematchNpcTemplate(u32 mapGroup, u32 mapNum, u32 localId,
                                          const struct ObjectEventTemplate **template)
{
    return ResolveNpc(mapGroup, mapNum, localId, TRUE, template);
}
