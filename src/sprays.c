#include "global.h"
#include "admg_feature_bindings.h"
#include "event_data.h"
#include "admg_state.h"
#include "script_menu.h"
#include "strings.h"
#include "constants/items.h"
#include "item.h"
#include "script_menu.h"
#include "menu.h"

#define SPRAY_COUNT 0
#define SPRAY_GET 1

#define NUM_SPRAY_STRENGTH 3
#define SPRAY_MENU_Y_COORD 8

#define LOCAL_VAR_SPRAY gSpecialVar_0x8004
#define LOCAL_VAR_SPRAY_CONST VAR_0x8004

u32 CountOrGetSprays(u32);
u32 GetNumberSprayStrength(void);
u32 GetSprayId(void);
u32 GetLastUsedSprayType(void);
u32 SetSprayMenuCursorPosition(int, int);
void DrawSprayMenu(void);
void HandleSprayMenuChoice(void);

// Validate the shared backing before selection arithmetic or menu effects.
static bool8 ADMG_ValidateSprayMenuState(void)
{
    if (ADMG_GetVarSpan(VAR_REPEL_STEP_COUNT, 1) == NULL)
    {
        ADMG_StateAccessRejected(VAR_REPEL_STEP_COUNT, __func__);
        return FALSE;
    }
    for (u32 i = 0; i < NUM_SPRAY_STRENGTH; ++i)
    {
        u16 id;
        if (!ADMG_TryGetIndexedVarId(LOCAL_VAR_SPRAY_CONST, i, NUM_SPRAY_STRENGTH, 1, &id))
            return FALSE;
    }
    if (ADMG_GetFeatureBinding(ADMG_BINDING_VAR_LAST_REPEL_LURE_USED) != 0 && ADMG_GetVarSpan(ADMG_GetFeatureBinding(ADMG_BINDING_VAR_LAST_REPEL_LURE_USED), 1) == NULL)
    {
        ADMG_StateAccessRejected(ADMG_GetFeatureBinding(ADMG_BINDING_VAR_LAST_REPEL_LURE_USED), __func__);
        return FALSE;
    }
    return TRUE;
}

u32 CountOrGetSprays(u32 func)
{
    u32 i, currentSpray, sprayCount = 0;
    u64 generation = ADMG_BeginStateAccessCheck();
    u32 spray = GetLastUsedSprayType();
    if (!ADMG_EndStateAccessCheck(generation))
        return 0;

    for (i = 0; i < NUM_SPRAY_STRENGTH; i++)
    {
        currentSpray = spray + i;

        if (!CheckBagHasItem(currentSpray,1))
            continue;

        if (func == SPRAY_COUNT)
            sprayCount++;
        else
            return (currentSpray);
    }
    return sprayCount;
}

u32 GetNumberSprayStrength(void)
{
    u32 count = CountOrGetSprays(SPRAY_COUNT);
    // With I_REPEL_LURE_MENU off, report at most one choice even if several spray types are available
    return (I_REPEL_LURE_MENU == TRUE || count < 2) ? count : 1;
}

u32 GetSprayId(void)
{
    return CountOrGetSprays(SPRAY_GET);
}

u32 GetLastUsedSprayType(void)
{
    u32 packed;
    if (!ADMG_TryReadVar(VAR_REPEL_STEP_COUNT, &packed, FALSE))
        return ITEM_NONE;
    if (IS_LAST_USED_LURE(packed))
        return ITEM_LURE;
    else
        return ITEM_REPEL;
}

u32 SetSprayMenuCursorPosition(int currentSpray, int count)
{
    u32 remembered;
    if (!ADMG_TryReadVar(ADMG_GetFeatureBinding(ADMG_BINDING_VAR_LAST_REPEL_LURE_USED), &remembered, ADMG_GetFeatureBinding(ADMG_BINDING_VAR_LAST_REPEL_LURE_USED) == 0))
        return 0;
    if (remembered == currentSpray)
        return count;

    return 0;
}

void DrawSprayMenu(void)
{
    if (!ADMG_ValidateSprayMenuState())
        return;

    struct MenuAction menuItems[NUM_SPRAY_STRENGTH+1] = {NULL};
    int sprayIndex, count = 0, menuPos = 0, currentSpray, yCoord = 0;
    u64 generation = ADMG_BeginStateAccessCheck();
    u32 spray = GetLastUsedSprayType();
    if (!ADMG_EndStateAccessCheck(generation))
        return;

    for (sprayIndex = 0; sprayIndex < (NUM_SPRAY_STRENGTH); sprayIndex++)
    {
        currentSpray = spray + sprayIndex;

        if (!CheckBagHasItem(currentSpray, 1))
            continue;

        menuItems[count].text = GetItemName(currentSpray);
        VarSet(LOCAL_VAR_SPRAY_CONST + count, currentSpray);

        if (ADMG_GetFeatureBinding(ADMG_BINDING_VAR_LAST_REPEL_LURE_USED) != 0)
            menuPos = SetSprayMenuCursorPosition(currentSpray, count);

        yCoord = SPRAY_MENU_Y_COORD - (2 * count);
        count++;
    }

    gSpecialVar_0x8003 = count;
    menuItems[count].text = gText_Cancel2;

    DrawMultichoiceMenuInternal(18, yCoord, 0, FALSE, menuPos, menuItems, count+1);
}

void HandleSprayMenuChoice(void)
{
    u16 choiceId;
    u32 choice;
    if (!ADMG_ValidateSprayMenuState()
        || !ADMG_TryGetIndexedVarId(LOCAL_VAR_SPRAY_CONST, gSpecialVar_Result, NUM_SPRAY_STRENGTH, 1, &choiceId)
        || !ADMG_TryReadVar(choiceId, &choice, FALSE))
        return;

    u64 generation = ADMG_BeginStateAccessCheck();
    u32 lureMask = (GetLastUsedSprayType() == ITEM_LURE) ? REPEL_LURE_MASK : 0;
    if (!ADMG_EndStateAccessCheck(generation))
        return;

    LOCAL_VAR_SPRAY = choice;

    VarSet(VAR_REPEL_STEP_COUNT, GetItemHoldEffectParam(LOCAL_VAR_SPRAY) | lureMask);

    if (ADMG_GetFeatureBinding(ADMG_BINDING_VAR_LAST_REPEL_LURE_USED) != 0)
        VarSet(ADMG_GetFeatureBinding(ADMG_BINDING_VAR_LAST_REPEL_LURE_USED), LOCAL_VAR_SPRAY);
}
