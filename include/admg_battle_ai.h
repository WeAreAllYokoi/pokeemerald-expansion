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

#endif
