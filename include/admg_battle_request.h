#ifndef ADMG_BATTLE_REQUEST_H
#define ADMG_BATTLE_REQUEST_H
#include "admg_native.h"

struct ScriptContext;

// Multi-battle formats supported by ADMG_ScriptStartMultiBattle
enum ADMG_MultiBattleFormat
{
    ADMG_MULTI_BATTLE_TWO_VS_TWO = 1,   /* the player and a partner against two trainers (multi_2_vs_2) */
    ADMG_MULTI_BATTLE_TWO_VS_ONE,       /* the player and a partner against one trainer with two Pokémon out (multi_2_vs_1) */
};

// ADMG_ScriptStartMultiBattle puts its status in VAR_RESULT
// STARTED means the battle start task is queued; waitstate waits for the battle to finish
// The battle's end then replaces VAR_RESULT with its B_OUTCOME_*
// Refusal statuses start at ADMG_BATTLE_REQUEST_REFUSED_MIN, above all B_OUTCOME_* values
// Handle a refusal without entering waitstate
enum ADMG_BattleRequestStatus
{
    ADMG_BATTLE_REQUEST_STARTED = 0,
    ADMG_BATTLE_REQUEST_REFUSED_MIN = 0x100,
    ADMG_BATTLE_REQUEST_INVALID_FORMAT = ADMG_BATTLE_REQUEST_REFUSED_MIN,
    ADMG_BATTLE_REQUEST_INVALID_PARTNER,
    ADMG_BATTLE_REQUEST_INVALID_OPPONENT,
    ADMG_BATTLE_REQUEST_NOT_ENOUGH_MONS,
    ADMG_BATTLE_REQUEST_REFUSED,        // Start refused after the initial checks; BattleSetup_StartMultiBattle uses this too
};

// Start a multi battle without party selection, using the same setup as
// setmultitrainerbattle and BattleSetup_StartMultiBattle
// Supply these operands after callnative ADMG_ScriptStartMultiBattle:
// .2byte format, .4byte opponentA, .4byte defeatTextA,
// .4byte opponentB, .4byte defeatTextB, .2byte partner (PARTNER_*)
// Checks the format, partner, opponents and usable player Pokémon
// Check VAR_RESULT before waitstate; only ADMG_BATTLE_REQUEST_STARTED should wait
ADMG_NATIVE_ENTRY void ADMG_ScriptStartMultiBattle(struct ScriptContext *ctx);

#endif
