#ifndef GUARD_DEXNAV_H
#define GUARD_DEXNAV_H

#include "admg_native.h"

#include "config/dexnav.h"

// GUI Info
enum RowGUIInfo
{
    ROW_WATER,
    ROW_LAND_TOP,
    ROW_LAND_BOT,
    ROW_HIDDEN,
    ROWS_COUNT
};

#define ROW_WATER_ICON_X        30
#define ROW_WATER_ICON_Y        35

#define ROW_LAND_ICON_X         20
#define ROW_LAND_TOP_ICON_Y     72
#define ROW_LAND_BOT_ICON_Y     (ROW_LAND_TOP_ICON_Y + 28)

#define ROW_HIDDEN_ICON_X       52
#define ROW_HIDDEN_ICON_Y       138

enum EncounterType
{
    ENCOUNTER_TYPE_LAND,
    ENCOUNTER_TYPE_WATER,
    ENCOUNTER_TYPE_HIDDEN // Get from species
};

#define COL_WATER_COUNT         5
#define COL_LAND_COUNT          6
#define COL_HIDDEN_COUNT        3

#define COL_WATER_MAX           (COL_WATER_COUNT - 1)
#define COL_LAND_MAX            (COL_LAND_COUNT - 1)
#define COL_HIDDEN_MAX          (COL_HIDDEN_COUNT - 1)

// SEARCH INFO
#define SCANSTART_X             0
#define SCANSTART_Y             0
#define SCANSIZE_X              12
#define SCANSIZE_Y              12

#define SPECIES_INFO_Y          5
#define TYPE_ICONS_Y            (SPECIES_INFO_Y + 24)
#define SEARCH_LEVEL_Y          (TYPE_ICONS_Y + 24)
#define HA_INFO_Y               (SEARCH_LEVEL_Y + 24)
#define CHAIN_BONUS_Y           (HA_INFO_Y + 24)

#define MON_LEVEL_NONEXISTENT   255 // If mon not in area GetEncounterLevel returns this to exit the search

// GUI tags
#define ICON_PAL_TAG            56000
#define ICON_GFX_TAG            55130
#define SELECTION_CURSOR_TAG    0x4005
#define CAPTURED_ALL_TAG        0x4002

// Search tags
#define OWNED_ICON_TAG          0x4003
#define HIDDEN_SEARCH_TAG       SELECTION_CURSOR_TAG
#define HIDDEN_MON_ICON_TAG     0x4006
#define LIT_STAR_TILE_TAG       0x4010
#define HELD_ITEM_TAG           0xd750

enum ADMG_DexNavGuiOpenStatus
{
    ADMG_DEXNAV_GUI_STARTED = 1,
    ADMG_DEXNAV_GUI_DISABLED,
    ADMG_DEXNAV_GUI_MISSING_BINDINGS,
    ADMG_DEXNAV_GUI_MISSING_STATE,
    ADMG_DEXNAV_GUI_FIELD_BUSY,
    ADMG_DEXNAV_GUI_ALLOCATION_FAILED,
    ADMG_DEXNAV_GUI_INVALID_CALLBACK,
};
enum ADMG_DexNavGuiResult
{
    ADMG_DEXNAV_GUI_PENDING,
    ADMG_DEXNAV_GUI_CLOSED,
    ADMG_DEXNAV_GUI_SEARCH_SELECTED,
    ADMG_DEXNAV_GUI_SETUP_FAILED,
};
enum ADMG_DexNavSearchState
{
    ADMG_DEXNAV_SEARCH_IDLE,
    ADMG_DEXNAV_SEARCH_TRACKING,
    ADMG_DEXNAV_SEARCH_HIDDEN,
    ADMG_DEXNAV_SEARCH_CANCELLED,
    ADMG_DEXNAV_SEARCH_TIMED_OUT,
    ADMG_DEXNAV_SEARCH_LOST_SIGNAL,
    ADMG_DEXNAV_SEARCH_MOVED_TOO_FAST,
    ADMG_DEXNAV_SEARCH_ENCOUNTER_READY,
    ADMG_DEXNAV_SEARCH_NOT_FOUND,
    ADMG_DEXNAV_SEARCH_TOO_DARK,
    ADMG_DEXNAV_SEARCH_ALLOCATION_FAILED,
    ADMG_DEXNAV_SEARCH_MAP_CHANGED,
    ADMG_DEXNAV_SEARCH_DISABLED,
};
enum ADMG_DexNavSearchStartStatus
{
    ADMG_DEXNAV_START_STARTED = 1,
    ADMG_DEXNAV_START_DISABLED,
    ADMG_DEXNAV_START_MISSING_BINDINGS,
    ADMG_DEXNAV_START_MISSING_STATE,
    ADMG_DEXNAV_START_FIELD_BUSY,
    ADMG_DEXNAV_START_ALREADY_SEARCHING,
    ADMG_DEXNAV_START_INVALID_SPECIES,
    ADMG_DEXNAV_START_INVALID_ENVIRONMENT,
    ADMG_DEXNAV_START_ALLOCATION_FAILED,
    ADMG_DEXNAV_START_TOO_DARK,
    ADMG_DEXNAV_START_NOT_FOUND,
};
/* Snapshot of transient DexNav search state, separate from saved search history.
 * Changing the snapshot does not change the search. */
struct ADMG_DexNavSearchSnapshot
{
    u32 state;
    u32 species;
    u32 environment;
    u32 proximity;
    s32 tileX;
    s32 tileY;
    u32 elapsedFrames;
    u32 searchLevel;
    u32 hiddenSearch;
    u32 movementCount;
};
ADMG_NATIVE_ENTRY u32 ADMG_TryStartDexNavSearch(u32 species, u32 environment);
ADMG_NATIVE_ENTRY u32 ADMG_TryStartRegisteredDexNavSearch(void);
ADMG_NATIVE_ENTRY bool32 ADMG_ReadDexNavSearch(struct ADMG_DexNavSearchSnapshot *snapshot);
ADMG_NATIVE_ENTRY bool32 ADMG_CancelDexNavSearch(void);
ADMG_NATIVE_ENTRY u32 ADMG_GetDexNavDetectorEnabled(void);
ADMG_NATIVE_ENTRY bool32 ADMG_SetDexNavDetectorEnabled(bool32 enabled);
ADMG_NATIVE_ENTRY u32 ADMG_GetDexNavDetectorSteps(void);
ADMG_NATIVE_ENTRY bool32 ADMG_AdvanceDexNavSearchStep(void);
ADMG_NATIVE_ENTRY bool32 ADMG_AdvanceDexNavDetectorStep(void);

ADMG_NATIVE_ENTRY u32 ADMG_TryOpenDexNavGui(void (*returnCallback)(void));
ADMG_NATIVE_ENTRY u32 ADMG_GetDexNavGuiResult(void);
ADMG_NATIVE_ENTRY u32 ADMG_GetDexNavGuiExitSpecies(void);
ADMG_NATIVE_ENTRY u32 ADMG_GetDexNavGuiExitEnvironment(void);
ADMG_NATIVE_ENTRY bool32 ADMG_DexNavEnabled(void);
ADMG_NATIVE_ENTRY bool32 ADMG_DexNavSelectionAvailable(void);
ADMG_NATIVE_ENTRY u32 ADMG_GetDexNavRegisteredSpecies(void);
ADMG_NATIVE_ENTRY u32 ADMG_GetDexNavRegisteredEnvironment(void);
ADMG_NATIVE_ENTRY bool32 ADMG_SetDexNavSelection(u32 species, u32 environment);
/* Search history uses host-owned state and can be accessed even when DexNav gameplay is disabled.
 * ADMG_GetDexNavSearchLevel returns 0xFFFFFFFF for SPECIES_NONE, an invalid species ID,
 * or unavailable state. Zero is a valid stored search level. */
ADMG_NATIVE_ENTRY bool32 ADMG_DexNavSearchLevelsAvailable(void);
ADMG_NATIVE_ENTRY u32 ADMG_GetDexNavSearchLevel(u32 species);
ADMG_NATIVE_ENTRY bool32 ADMG_SetDexNavSearchLevel(u32 species, u32 level);
ADMG_NATIVE_ENTRY bool32 ADMG_IncrementDexNavSearchLevel(u32 species);
ADMG_NATIVE_ENTRY u32 ADMG_GetEffectiveDexNavSearchLevel(u32 species);
void EndDexNavSearch(void);
void Task_OpenDexNavFromStartMenu(u8 taskId);
bool32 TryStartDexNavSearch(void);
void TryIncrementSpeciesSearchLevel(void);
void ResetDexNavSearch(void);
bool32 TryFindHiddenPokemon(void);
u32 CalculateDexNavShinyRolls(void);
void IncrementDexNavChain(void);
bool32 OnStep_DexNavSearch(void);

extern enum Species gDexNavSpecies;

#endif // GUARD_DEXNAV_H
