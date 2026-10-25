#ifndef ADMG_BATTLE_AI_H
#define ADMG_BATTLE_AI_H
#include "admg_native.h"

// Set or read the AI callbacks used by setdynamicaifunc and setdynamicswitchaifunc
// Pass a Thumb function address, or 0 to clear the callback
// Setters return FALSE and leave the callback alone while gMain.inBattle is set,
// or if a nonzero address is missing the Thumb bit
// ResetDynamicAiFunctions clears both callbacks during battle cleanup
ADMG_NATIVE_ENTRY ADMG_Word ADMG_SetDynamicAiScoreFunc(ADMG_Word func);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_SetDynamicAiSwitchFunc(ADMG_Word func);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetDynamicAiScoreFunc(void);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetDynamicAiSwitchFunc(void);

// Upstream leaves AI flag bits 37–59 unused, so mods can use them for scoring
// These bits use ADMG_ModAiFlagScore, which leaves the score unchanged by default
// A wrapper can read the current bit number from gAiThinkingStruct->aiLogicId
enum { ADMG_AI_FLAG_MOD_FIRST = 37, ADMG_AI_FLAG_MOD_LAST = 59 };

#endif
