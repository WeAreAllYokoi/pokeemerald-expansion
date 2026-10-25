#ifndef ADMG_TRAINER_SCRIPT_H
#define ADMG_TRAINER_SCRIPT_H
#include "admg_native.h"

struct ScriptStack;

// Which trainer-battle builder supplied the snippet list
enum ADMG_TrainerScriptKind
{
    ADMG_TRAINER_SCRIPT_TALK,               /* trainerbattle run from a script (ConfigureTrainerBattle) */
    ADMG_TRAINER_SCRIPT_APPROACH,           /* a trainer that saw the player (ConfigureApproachingTrainerBattle) */
    ADMG_TRAINER_SCRIPT_FACILITY,           /* Pyramid / Trainer Hill, talked to */
    ADMG_TRAINER_SCRIPT_FACILITY_APPROACH,  /* Pyramid / Trainer Hill, saw the player */
    ADMG_TRAINER_SCRIPT_KIND_COUNT,
};

// Snippets share SCRIPT_STACK_SIZE - 1 slots with the context's existing call frames
#define ADMG_TRAINER_SCRIPT_CAPACITY (SCRIPT_STACK_SIZE - 1)

// Mods can edit the built snippet list if it isn't empty and no snippets were dropped
// stack->stack[0] runs first; contextDepth is the number of call frames already in use
// The default accepts the list unchanged; return FALSE to select the refusal snippet
ADMG_NATIVE_ENTRY ADMG_Word ADMG_ComposeTrainerBattleScript(ADMG_Word kind, struct ScriptStack *stack, ADMG_Word contextDepth);

// Read the global script context's current call-stack depth
u8 ADMG_GetGlobalScriptStackDepth(void);

#endif
