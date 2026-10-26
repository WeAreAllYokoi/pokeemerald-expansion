#ifndef ADMG_SCRIPT_H
#define ADMG_SCRIPT_H

#include "admg_state.h"
#include "script.h"

/* Rejected input stops this context without invoking StopScript's assertion
 * about an in-progress warp; a warning must not become a crash-screen effect. */
ADMG_NATIVE_ENTRY void ADMG_StopInvalidStateCommand(struct ScriptContext *ctx);

/* These expressions return from the real command on rejection, rather than
 * manufacturing a false flag or a scalar sentinel that could cause an effect.
 * Empty return arguments support void native commands; bool commands pass
 * FALSE. Each operand and its source read are evaluated exactly once.
 * Callers must parse/validate every input and destination before any effect. */
#define ADMG_SCRIPT_VALUE(context, operand, ...) \
    ({ \
        u16 admg_value; \
        if (!ADMG_TryReadVar((operand), &admg_value, TRUE)) \
        { \
            ADMG_StopInvalidStateCommand(context); \
            return __VA_ARGS__; \
        } \
        admg_value; \
    })

/* Native signed-byte operands also admit the documented negative halfword
 * literals. Other commands retain their ordinary identifier/literal rules.
 * Successful variable reads keep the entire stored u16, including 65535. */
#define ADMG_SCRIPT_SIGNED_BYTE_VALUE(context, operand, ...) \
    ({ \
        u32 admg_operand = (operand); \
        u16 admg_value; \
        if (admg_operand >= 0xFF80 && admg_operand <= 0xFFFF) \
            admg_value = admg_operand; \
        else if (!ADMG_TryReadVar(admg_operand, &admg_value, TRUE)) \
        { \
            ADMG_StopInvalidStateCommand(context); \
            return __VA_ARGS__; \
        } \
        admg_value; \
    })

/* Validate a required backing object before a command reads or changes it. */
#define ADMG_SCRIPT_REQUIRE_BACKING(context, backing, ...) \
    do \
    { \
        if ((backing) == NULL) \
        { \
            MgbaPrintf(MGBA_LOG_WARN, "%s: required state backing is unavailable", __func__); \
            ADMG_StopInvalidStateCommand(context); \
            return __VA_ARGS__; \
        } \
    } while (0)

/* Bound a consumer's semantic index before narrowing or forming a pointer. */
#define ADMG_SCRIPT_REQUIRE_INDEX(context, index, count, ...) \
    do \
    { \
        u32 admg_index = (index); \
        if (admg_index >= (count)) \
        { \
            ADMG_StateAccessRejected(admg_index, __func__); \
            ADMG_StopInvalidStateCommand(context); \
            return __VA_ARGS__; \
        } \
    } while (0)

/* A local ID is a wide operand until its byte width and lookup result have
 * been validated. Keep the original named lookup behind the checked boundary. */
#define ADMG_SCRIPT_OBJECT_ID(context, localId, ...) \
    ({ \
        u8 admg_object_id; \
        if (!ADMG_TryGetObjectEventId((localId), &admg_object_id)) \
        { \
            ADMG_StopInvalidStateCommand(context); \
            return __VA_ARGS__; \
        } \
        admg_object_id; \
    })

#define ADMG_SCRIPT_POINTER(context, identifier, ...) \
    ({ \
        u32 admg_identifier = (identifier); \
        u16 *admg_pointer = ADMG_GetVarSpan(admg_identifier, 1); \
        if (admg_pointer == NULL) \
        { \
            ADMG_StateAccessRejected(admg_identifier, __func__); \
            ADMG_StopInvalidStateCommand(context); \
            return __VA_ARGS__; \
        } \
        admg_pointer; \
    })

#define ADMG_SCRIPT_FLAG(context, identifier, ...) \
    ({ \
        bool8 admg_value; \
        if (!ADMG_TryReadFlag((identifier), &admg_value)) \
        { \
            ADMG_StopInvalidStateCommand(context); \
            return __VA_ARGS__; \
        } \
        admg_value; \
    })

#define ADMG_SCRIPT_FLAG_ID(context, identifier, ...) \
    ({ \
        u32 admg_identifier = (identifier); \
        if (!ADMG_IsFlagIdValid(admg_identifier)) \
        { \
            ADMG_StateAccessRejected(admg_identifier, __func__); \
            ADMG_StopInvalidStateCommand(context); \
            return __VA_ARGS__; \
        } \
        admg_identifier; \
    })

#endif
