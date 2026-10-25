#ifndef GUARD_ADMG_DEVELOPER_TOOLS_H
#define GUARD_ADMG_DEVELOPER_TOOLS_H

#include "admg_native.h"
#include "admg_retention.h"

// Switch the field and battle debug menus, Pokémon sprite visualizer and quickstart on at runtime
// These switches start off, and the build's debug settings still apply
// Mods can change the enabled set through ADMG_GetDeveloperTools
#define ADMG_DEVELOPER_TOOLS_VERSION        1

#define ADMG_DEVTOOL_FIELD_MENU             (1 << 0)    // R+START in the field: Debug_ShowMainMenu
#define ADMG_DEVTOOL_BATTLE_MENU            (1 << 1)    // SELECT while choosing an action: CB2_BattleDebugMenu
#define ADMG_DEVTOOL_SPRITE_VISUALIZER      (1 << 2)    // SELECT in the summary screen: CB2_Pokemon_Sprite_Visualizer
#define ADMG_DEVTOOL_QUICKSTART             (1 << 3)    // SELECT on the title screen: Quickstart
#define ADMG_DEVTOOL_QUICKSTART_HUD         (1 << 4)    // the quickstart sprite on the title screen
#define ADMG_DEVTOOLS_ALL                   0x1F
#define ADMG_DEVELOPER_TOOLS_DEFAULT        0

/* ADMG_GetBattleDebugMenuState */
#define ADMG_BATTLE_DEBUG_CLOSED            0
#define ADMG_BATTLE_DEBUG_QUEUED            1           // Queued for a player action choice
#define ADMG_BATTLE_DEBUG_OPENING           2
#define ADMG_BATTLE_DEBUG_OPEN              3           // Input task is active; values and outcomes can be changed
#define ADMG_BATTLE_DEBUG_CLOSING           4

// Fields edited in gBattleMons by ADMG_BattleDebugSetValue
#define ADMG_BATTLE_DEBUG_HP                0
#define ADMG_BATTLE_DEBUG_MAX_HP            1
#define ADMG_BATTLE_DEBUG_STATUS1           2
#define ADMG_BATTLE_DEBUG_HELD_ITEM         3
#define ADMG_BATTLE_DEBUG_ABILITY           4
#define ADMG_BATTLE_DEBUG_MOVE              5           // index: move slot
#define ADMG_BATTLE_DEBUG_PP                6           // index: move slot
#define ADMG_BATTLE_DEBUG_STAT_STAGE        7           // index: STAT_*, value 0..MAX_STAT_STAGE
#define ADMG_BATTLE_DEBUG_FIELD_COUNT       8

// ADMG_GetSpriteVisualizerValue keys; returns 0xFFFFFFFF without an active input task or for an unknown key
#define ADMG_VISUALIZER_SPECIES             0
#define ADMG_VISUALIZER_SHINY               1
#define ADMG_VISUALIZER_FEMALE              2
#define ADMG_VISUALIZER_FRONT_SPRITE        3
#define ADMG_VISUALIZER_BACK_SPRITE         4
#define ADMG_VISUALIZER_ICON_SPRITE         5
#define ADMG_VISUALIZER_FRONT_ANIM          6
#define ADMG_VISUALIZER_BACK_ANIM           7
#define ADMG_VISUALIZER_BATTLE_ENVIRONMENT  8
#define ADMG_VISUALIZER_MOVE_BACKGROUND     9
#define ADMG_VISUALIZER_SUBMENU             10
#define ADMG_VISUALIZER_FRONT_ANIMATING     11          // Front sprite callback is not SpriteCallbackDummy
#define ADMG_VISUALIZER_BACK_ANIMATING      12
#define ADMG_VISUALIZER_ICON_IMAGE          13          // Icon sprite's image-data pointer
#define ADMG_VISUALIZER_OPTION_ROW          14          // the cursor row of the animations and backgrounds submenu
#define ADMG_VISUALIZER_KEY_COUNT           15

ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetDeveloperTools(void);
static inline bool32 ADMG_DeveloperToolEnabled(u32 tool)
{
    return (ADMG_GetDeveloperTools() & tool) != 0;
}

// Open the field debug menu through the same entry point as R+START
ADMG_NATIVE_ENTRY ADMG_Word ADMG_OpenFieldDebugMenu(void);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_IsFieldDebugMenuOpen(void);

// For a quickstart game, CB2_NewGame calls ADMG_QuickstartPrepare after NewGameInitData
// A provider can prepare the game and call WarpIntoMap to choose the starting map
// Return TRUE for the warp fade-in, or FALSE for the normal new-game entry
ADMG_NATIVE_ENTRY ADMG_Word ADMG_StartQuickstart(void);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_QuickstartPrepare(void);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_IsQuickstartGame(void);
bool32 ADMG_TakeQuickstartPending(void);
void ADMG_SetQuickstartPending(void);

/* Battle debug menu (src/battle_debug.c) */
ADMG_NATIVE_ENTRY ADMG_Word ADMG_RequestBattleDebugMenu(void);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_CancelBattleDebugRequest(void);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetBattleDebugMenuState(void);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetBattleDebugBattler(void);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_BattleDebugSetValue(ADMG_Word battler, ADMG_Word field, ADMG_Word index, ADMG_Word value);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_BattleDebugForceOutcome(ADMG_Word outcome);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_CloseBattleDebugMenu(void);
bool32 ADMG_TakeBattleDebugRequest(void);
bool32 ADMG_BattleDebugEndWithOutcome(u32 outcome);

// Open the Pokémon sprite visualizer from the field and return there when it closes
ADMG_NATIVE_ENTRY ADMG_Word ADMG_OpenSpriteVisualizer(ADMG_Word species, ADMG_Word shiny, ADMG_Word female);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetSpriteVisualizerValue(ADMG_Word key);

#endif // GUARD_ADMG_DEVELOPER_TOOLS_H
