#ifndef ADMG_FEATURE_BINDINGS_H
#define ADMG_FEATURE_BINDINGS_H
#include "admg_native.h"
#include "constants/admg_state_ids.h"
struct ScriptContext;
enum ADMG_FeatureBindingKind { ADMG_BINDING_KIND_NONE, ADMG_BINDING_KIND_FLAG, ADMG_BINDING_KIND_VAR };
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureBinding(ADMG_Word binding);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureBindingDefault(ADMG_Word binding);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureBindingSlot(ADMG_Word binding);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureBindingKind(ADMG_Word binding);
void ADMG_ScriptTestFeatureBinding(struct ScriptContext *ctx);
void ADMG_ScriptWriteFeatureFlag(struct ScriptContext *ctx);
void ADMG_ScriptCopyFeatureVar(struct ScriptContext *ctx);
#endif
