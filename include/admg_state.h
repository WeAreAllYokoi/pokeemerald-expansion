#ifndef ADMG_STATE_H
#define ADMG_STATE_H

#include "admg_native.h"
#include "constants/admg_state_ids.h"

/* Checked interfaces retain a success channel independent of the stored value.
 * Producers must use the wide ID before narrowing. Zero flags are intentional
 * no-ops; variable literals require explicit permission from the operand type. */
ADMG_NATIVE_ENTRY bool8 ADMG_TryReadFlag(u32 id, bool8 *value);
ADMG_NATIVE_ENTRY bool8 ADMG_TryReadVar(u32 id, u32 *value, bool8 allowLiteral);
ADMG_NATIVE_ENTRY bool8 ADMG_TryReadVarIndex(u32 id, u32 count, u32 *value);
ADMG_NATIVE_ENTRY bool8 ADMG_IsMapIdValid(MapId mapId);
ADMG_NATIVE_ENTRY bool8 ADMG_TryGetObjectEventId(u32 localId, u8 *objectId);
ADMG_NATIVE_ENTRY bool8 ADMG_TryGetObjectEventFlagId(u32 localId, MapId mapId, u16 *flagId);
ADMG_NATIVE_ENTRY bool8 ADMG_TryReadObjectGraphicsVar(u32 slot, u32 *value);
ADMG_NATIVE_ENTRY bool8 ADMG_IsFlagIdValid(u32 id);
ADMG_NATIVE_ENTRY bool8 ADMG_IsFlagClear(u32 id);
ADMG_NATIVE_ENTRY bool8 ADMG_TryGetIndexedFlagId(u32 first, u32 index, u32 count, u32 stride, u16 *id);
ADMG_NATIVE_ENTRY bool8 ADMG_TryGetIndexedVarId(u32 first, u32 index, u32 count, u32 stride, u16 *id);
ADMG_NATIVE_ENTRY bool8 ADMG_TryGetFlagFromTable(const u16 *flags, u32 index, u32 count, u16 *id);
ADMG_NATIVE_ENTRY bool8 ADMG_TryReadLegacyTrainerFlag(u32 trainerId, bool8 *value);
ADMG_NATIVE_ENTRY bool8 ADMG_TryReadCurrentTrainerFlag(bool8 *value);
ADMG_NATIVE_ENTRY bool8 ADMG_TryReadTrainerFlagFromScript(const u8 *data, bool8 *value);
ADMG_NATIVE_ENTRY u32 *ADMG_GetVarSpan(u32 id, u32 words);
ADMG_NATIVE_ENTRY u8 *ADMG_GetRegionalFlagPointer(u16 id);
ADMG_NATIVE_ENTRY u32 *ADMG_GetRegionalVarPointer(u16 id);
ADMG_NATIVE_ENTRY bool8 ADMG_ReadHiddenItemFlag(u32 offset, bool8 *collected);
ADMG_NATIVE_ENTRY void ADMG_InitRegionalEventData(void);
ADMG_NATIVE_ENTRY void ADMG_ClearRegionalDailyFlags(void);
ADMG_NATIVE_ENTRY void ADMG_StateAccessRejected(u32 id, const char *operation);
/* Native-special status remains independent of data. A generation snapshot
 * survives nested checks and abandoned calls without retaining stack pointers.
 * The counter saturates instead of wrapping into a false success. */
ADMG_NATIVE_ENTRY u64 ADMG_BeginStateAccessCheck(void);
ADMG_NATIVE_ENTRY bool8 ADMG_EndStateAccessCheck(u64 generation);

/* Guard the complete backing object before a production consumer dereferences
 * or passes it on. Void consumers omit the failure return value. */
#define ADMG_REQUIRE_VAR_SPAN(identifier, wordCount, ...) \
    ({ \
        u32 admg_span_id = (identifier); \
        u32 *admg_span = ADMG_GetVarSpan(admg_span_id, (wordCount)); \
        if (admg_span == NULL) \
        { \
            ADMG_StateAccessRejected(admg_span_id, __func__); \
            return __VA_ARGS__; \
        } \
        admg_span; \
    })

/* A valid stored word is not necessarily a valid index. Keep the named read
 * and its status separate; reject before a caller indexes or performs effects. */
#define ADMG_VAR_INDEX(identifier, count, ...) \
    ({ \
        u32 admg_index_value; \
        if (!ADMG_TryReadVarIndex((identifier), (count), &admg_index_value)) \
            return __VA_ARGS__; \
        admg_index_value; \
    })

/* Preflight a complete identifier range before a consumer produces output or
 * changes state. Reuse the wide, owner-aware indexed producer for every ID. */
#define ADMG_REQUIRE_FLAG_RANGE(first, count, stride, ...) \
    do { \
        u32 admg_flag_first = (first), admg_flag_count = (count), admg_flag_stride = (stride); \
        for (u32 admg_flag_index = 0; admg_flag_index < admg_flag_count; ++admg_flag_index) \
        { \
            u16 admg_flag_id; \
            if (!ADMG_TryGetIndexedFlagId(admg_flag_first, admg_flag_index, admg_flag_count, admg_flag_stride, &admg_flag_id)) \
                return __VA_ARGS__; \
        } \
    } while (0)

/* Passive logical banks; S3-E owns serialization. No legacy save root grows.
 * Native mod persistence is a separate platform facility. */
extern u8 gADMG_KantoFlags[288];
extern u32 gADMG_KantoVars[224];
extern u8 gADMG_UnionVisibility[2];
extern u8 gADMG_NativeSpecialScratch[1];
extern u32 gADMG_TextColorScratch[2];
extern u8 gADMG_FeatureBindingFlags[5];
extern u32 gADMG_FeatureBindingVars[8];

#endif
