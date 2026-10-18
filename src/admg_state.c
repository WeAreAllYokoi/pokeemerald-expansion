#include "global.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "admg_state.h"
#include "data/admg_state_routes.h"
#include "constants/map_groups.h"
#include "data/map_group_count.h"

_Static_assert(ARRAY_COUNT(MAP_GROUP_COUNT) == MAP_GROUPS_COUNT + 1, "map counts must match the generated map groups");

EWRAM_DATA u8 gADMG_KantoFlags[288] = {0};
EWRAM_DATA u32 gADMG_KantoVars[224] = {0};
EWRAM_DATA u8 gADMG_UnionVisibility[2] = {0};
EWRAM_DATA u8 gADMG_NativeSpecialScratch[1] = {0};
EWRAM_DATA u32 gADMG_TextColorScratch[2] = {0};
EWRAM_DATA u8 gADMG_FeatureBindingFlags[5] = {0};
EWRAM_DATA u32 gADMG_FeatureBindingVars[8] = {0};
static EWRAM_DATA u64 sADMG_StateRejectionGeneration = 0;
static EWRAM_DATA bool8 sADMG_StateRejectionExhausted = FALSE;

u64 ADMG_BeginStateAccessCheck(void)
{
    return sADMG_StateRejectionGeneration;
}

bool8 ADMG_EndStateAccessCheck(u64 generation)
{
    return !sADMG_StateRejectionExhausted && generation == sADMG_StateRejectionGeneration;
}

static void RecordStateRejection(void)
{
    if (sADMG_StateRejectionGeneration == ~(u64)0)
        sADMG_StateRejectionExhausted = TRUE;
    else
        ++sADMG_StateRejectionGeneration;
}

static bool8 IsReserved(u16 ordinal, const u16 *positions, u32 count)
{
    u32 i;
    for (i = 0; i < count; ++i)
        if (positions[i] == ordinal)
            return TRUE;
    return FALSE;
}

u8 *ADMG_GetRegionalFlagPointer(u16 id)
{
    if (id >= ADMG_STATE_KANTO_NATIVE_FLAGS_FIRST && id <= ADMG_STATE_KANTO_NATIVE_FLAGS_LAST)
    {
        u16 ordinal = id - ADMG_STATE_KANTO_NATIVE_FLAGS_FIRST;
        if (IsReserved(ordinal, sADMG_ReservedFlagMirrors, ARRAY_COUNT(sADMG_ReservedFlagMirrors)))
            return NULL;
        if (ordinal >= ADMG_UNION_NATIVE_FIRST && ordinal <= ADMG_UNION_NATIVE_LAST)
            return &gADMG_UnionVisibility[ordinal / 8 - ADMG_UNION_NATIVE_FIRST / 8];
        return &gADMG_KantoFlags[ordinal / 8];
    }
    if (id == ADMG_KANTO_FLAG_SPECIAL_FLAG_UNUSED_0x4002 || id == ADMG_KANTO_FLAG_SPECIAL_FLAG_UNUSED_0x4004)
        return &gADMG_NativeSpecialScratch[0];
    if (id >= ADMG_STATE_FEATURE_BINDING_FLAGS_FIRST && id < ADMG_STATE_FEATURE_BINDING_FLAGS_FIRST + 38)
        return &gADMG_FeatureBindingFlags[(id - ADMG_STATE_FEATURE_BINDING_FLAGS_FIRST) / 8];
    return NULL;
}

u32 *ADMG_GetRegionalVarPointer(u16 id)
{
    if (id >= ADMG_STATE_KANTO_NATIVE_VARS_FIRST && id <= ADMG_STATE_KANTO_NATIVE_VARS_LAST)
    {
        u16 ordinal = id - ADMG_STATE_KANTO_NATIVE_VARS_FIRST;
        if (IsReserved(ordinal, sADMG_ReservedVarMirrors, ARRAY_COUNT(sADMG_ReservedVarMirrors)))
            return NULL;
        return &gADMG_KantoVars[ordinal];
    }
    if (id >= ADMG_STATE_TEXT_COLOR_SCRATCH_FIRST && id <= ADMG_STATE_TEXT_COLOR_SCRATCH_LAST)
        return &gADMG_TextColorScratch[id - ADMG_STATE_TEXT_COLOR_SCRATCH_FIRST];
    if (id >= ADMG_STATE_FEATURE_BINDING_VARS_FIRST && id <= ADMG_STATE_FEATURE_BINDING_VARS_LAST)
        return &gADMG_FeatureBindingVars[id - ADMG_STATE_FEATURE_BINDING_VARS_FIRST];
    return NULL;
}

void ADMG_StateAccessRejected(u32 id, const char *operation)
{
    RecordStateRejection();
    /* Reuse the existing format/severity/control path. S2-F must qualify its
     * ordinary native delivery; this scope does not introduce a transport. */
    DebugPrintfLevel(MGBA_LOG_WARN, "ADMG state access rejected: %s id=0x%lx (%s)", operation, (unsigned long)id, __FILE__);
}

bool8 ADMG_IsFlagIdValid(u32 id)
{
    return id <= 0xFFFF && (id == 0 || GetFlagPointer((u16)id) != NULL);
}

bool8 ADMG_TryReadFlag(u32 id, bool8 *value)
{
    if (value == NULL || !ADMG_IsFlagIdValid(id))
    {
        ADMG_StateAccessRejected(id, "flag read");
        return FALSE;
    }
    *value = FlagGet((u16)id);
    return TRUE;
}

bool8 ADMG_IsFlagClear(u32 id)
{
    bool8 value;
    /* A rejected read cannot become permission to spawn or advance state. */
    return ADMG_TryReadFlag(id, &value) && !value;
}

static bool8 TryGetIndexedStateId(u32 first, u32 index, u32 count, u32 stride, u16 *id, bool8 isFlag)
{
    u32 candidate;
    /* Validate the producer in a wide type before multiplication, addition or
     * narrowing. Count/stride are supplied by the owning immutable domain. */
    if (id == NULL || stride == 0 || first > 0xFFFF || index >= count
     || (index != 0 && (index > 0xFFFF || stride > 0xFFFF)))
    {
        RecordStateRejection();
        DebugPrintfLevel(MGBA_LOG_WARN,
            "ADMG indexed %s rejected: first=0x%lx index=0x%lx count=%lu stride=%lu (%s)",
            isFlag ? "flag" : "variable", (unsigned long)first, (unsigned long)index, (unsigned long)count,
            (unsigned long)stride, __FILE__);
        return FALSE;
    }
    /* Either index is zero or both factors fit u16, so this product cannot
     * overflow u32. Then constrain the sum before forming/narrowing the ID. */
    candidate = index * stride;
    if (candidate > 0xFFFF - first)
    {
        ADMG_StateAccessRejected(index, isFlag ? "indexed flag offset" : "indexed variable offset");
        return FALSE;
    }
    candidate += first;
    if (isFlag ? !ADMG_IsFlagIdValid(candidate) : ADMG_GetVarSpan(candidate, 1) == NULL)
    {
        ADMG_StateAccessRejected(candidate, isFlag ? "indexed flag" : "indexed variable");
        return FALSE;
    }
    *id = (u16)candidate;
    return TRUE;
}

bool8 ADMG_TryGetIndexedFlagId(u32 first, u32 index, u32 count, u32 stride, u16 *id)
{
    return TryGetIndexedStateId(first, index, count, stride, id, TRUE);
}

bool8 ADMG_TryGetIndexedVarId(u32 first, u32 index, u32 count, u32 stride, u16 *id)
{
    return TryGetIndexedStateId(first, index, count, stride, id, FALSE);
}

bool8 ADMG_TryGetFlagFromTable(const u16 *flags, u32 index, u32 count, u16 *id)
{
    /* The caller supplies the actual immutable table extent. Selecting named
     * bindings preserves independent zero entries instead of adding an index
     * to an unbound base and reaching unrelated state. */
    if (flags == NULL || id == NULL || index >= count)
    {
        ADMG_StateAccessRejected(index, "flag table index");
        return FALSE;
    }
    if (!ADMG_IsFlagIdValid(flags[index]))
    {
        ADMG_StateAccessRejected(flags[index], "flag table binding");
        return FALSE;
    }
    *id = flags[index];
    return TRUE;
}

bool8 ADMG_TryReadVar(u32 id, u32 *value, bool8 allowLiteral)
{
    u32 *ptr;
    if (value != NULL && id < VARS_START && allowLiteral)
    {
        *value = VarGet((u16)id);
        return TRUE;
    }
    ptr = id <= 0xFFFF ? GetVarPointer((u16)id) : NULL;
    if (value == NULL || ptr == NULL)
    {
        ADMG_StateAccessRejected(id, "variable read");
        return FALSE;
    }
    /* Keep the existing named read wrapper/hook on successful reads. The
     * pointer establishes validity; the scalar value is never the status. */
    *value = VarGet((u16)id);
    return TRUE;
}

bool8 ADMG_TryReadObjectGraphicsVar(u32 slot, u32 *value)
{
    /* Keep the slot wide until its complete native domain is validated. A
     * successful stored 65535 remains data, independent of this status. */
    if (value == NULL || slot >= 16 || GetVarPointer(VAR_OBJ_GFX_ID_0 + slot) == NULL)
    {
        ADMG_StateAccessRejected(slot, "object graphics variable");
        return FALSE;
    }
    *value = VarGetObjectEventGraphicsId((u8)slot);
    return TRUE;
}

bool8 ADMG_IsMapIdValid(u32 mapGroup, u32 mapNum)
{
    if (mapGroup >= MAP_GROUPS_COUNT || mapNum >= MAP_GROUP_COUNT[mapGroup])
    {
        ADMG_StateAccessRejected(mapGroup, "map identifier");
        return FALSE;
    }
    return TRUE;
}

bool8 ADMG_TryGetObjectEventId(u32 localId, u8 *objectId)
{
    u8 candidate;
    if (objectId == NULL || localId > 0xFF)
    {
        ADMG_StateAccessRejected(localId, "object local identifier");
        return FALSE;
    }
    candidate = GetObjectEventIdByLocalId(localId);
    if (candidate >= OBJECT_EVENTS_COUNT)
    {
        ADMG_StateAccessRejected(localId, "object lookup");
        return FALSE;
    }
    *objectId = candidate;
    return TRUE;
}

bool8 ADMG_TryGetObjectEventFlagId(u32 localId, u32 mapNum, u32 mapGroup, u16 *flagId)
{
    u16 candidate;
    if (flagId == NULL || localId > 0xFF || !ADMG_IsMapIdValid(mapGroup, mapNum)
     || GetObjectEventTemplateByLocalIdAndMap(localId, mapNum, mapGroup) == NULL)
    {
        ADMG_StateAccessRejected(localId, "object flag binding");
        return FALSE;
    }
    candidate = GetObjectEventFlagIdByLocalIdAndMap(localId, mapNum, mapGroup);
    if (!ADMG_IsFlagIdValid(candidate))
    {
        ADMG_StateAccessRejected(candidate, "object flag identifier");
        return FALSE;
    }
    *flagId = candidate;
    return TRUE;
}

bool8 ADMG_TryReadVarIndex(u32 id, u32 count, u32 *value)
{
    u32 candidate;
    if (value == NULL || count == 0)
    {
        ADMG_StateAccessRejected(id, "variable index domain");
        return FALSE;
    }
    if (!ADMG_TryReadVar(id, &candidate, FALSE))
        return FALSE;
    if (candidate >= count)
    {
        ADMG_StateAccessRejected(candidate, "stored variable index");
        return FALSE;
    }
    *value = candidate;
    return TRUE;
}

static bool8 SpanFitsBank(u32 id, u32 words, u32 first, u32 last)
{
    return id >= first && id <= last && words - 1 <= last - id;
}

u32 *ADMG_GetVarSpan(u32 id, u32 words)
{
    u32 *first;
    u32 i;
    if (words == 0 || id > 0xFFFF)
        return NULL;
    /* Address adjacency cannot make separate banks or individual special
     * scalars one C array. Keep the entire record within one backing object. */
    if (!(SpanFitsBank(id, words, VARS_START, VARS_END)
       || SpanFitsBank(id, words, ADMG_STATE_KANTO_NATIVE_VARS_FIRST, ADMG_STATE_KANTO_NATIVE_VARS_LAST)
       || SpanFitsBank(id, words, ADMG_STATE_TEXT_COLOR_SCRATCH_FIRST, ADMG_STATE_TEXT_COLOR_SCRATCH_LAST)
       || SpanFitsBank(id, words, ADMG_STATE_FEATURE_BINDING_VARS_FIRST, ADMG_STATE_FEATURE_BINDING_VARS_LAST)
       || SpanFitsBank(id, words, ADMG_STATE_VAR_TESTING_FIRST, ADMG_STATE_VAR_TESTING_LAST)
       || (words == 1 && id >= SPECIAL_VARS_START && id <= SPECIAL_VARS_END)))
        return NULL;
    first = GetVarPointer((u16)id);
    if (first == NULL)
        return NULL;
    for (i = 1; i < words; ++i)
        if ((u32)GetVarPointer((u16)(id + i)) != (u32)first + i * VAR_BYTES)
            return NULL;
    return first;
}

bool8 ADMG_ReadHiddenItemFlag(u32 offset, bool8 *collected)
{
    if (offset >= (1u << 13))
    {
        ADMG_StateAccessRejected(offset, "hidden item offset");
        return FALSE;
    }
    return ADMG_TryReadFlag(offset + FLAG_HIDDEN_ITEMS_START, collected);
}

void ADMG_InitRegionalEventData(void)
{
    memset(gADMG_KantoFlags, 0, sizeof(gADMG_KantoFlags));
    memset(gADMG_KantoVars, 0, sizeof(gADMG_KantoVars));
    memset(gADMG_UnionVisibility, 0, sizeof(gADMG_UnionVisibility));
    memset(gADMG_NativeSpecialScratch, 0, sizeof(gADMG_NativeSpecialScratch));
    memset(gADMG_TextColorScratch, 0, sizeof(gADMG_TextColorScratch));
    memset(gADMG_FeatureBindingFlags, 0, sizeof(gADMG_FeatureBindingFlags));
    memset(gADMG_FeatureBindingVars, 0, sizeof(gADMG_FeatureBindingVars));
}

void ADMG_ClearRegionalDailyFlags(void)
{
    memset(&gADMG_KantoFlags[ADMG_KANTO_DAILY_FIRST / 8], 0, ADMG_KANTO_DAILY_COUNT / 8);
}
