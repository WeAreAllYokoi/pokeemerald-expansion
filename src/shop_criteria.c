#include "global.h"
#include "item.h"
#include "script.h"
#include "event_data.h"
#include "admg_state.h"
#include "malloc.h"
#include "shop_criteria.h"

static EWRAM_DATA const u32 *sDynamicShopItemListRef = NULL;
// Remember the original count so closing Buy restores it along with the list
// Otherwise, reopening Buy would only scan part of the list
static EWRAM_DATA u16 *sDynamicShopItemTotal = NULL;
static EWRAM_DATA u16 sDynamicShopItemTotalRef = 0;

// Remove the UNUSED if you'll use the functions!
static UNUSED bool32 ShopCriteriaByBadgeCount(u32 count);
static UNUSED bool32 ShopCriteriaByFlag(u32 flagId);
static UNUSED bool32 ShopCriteriaByVar(u32 varId, u32 varValue);

void TryBuildDynamicShopItemList(const u32 **ogItemList, u16 *resultingTotal)
{
    sDynamicShopItemListRef = *ogItemList;
    sDynamicShopItemTotal = resultingTotal;
    sDynamicShopItemTotalRef = *resultingTotal;

    u32 *list = AllocZeroed((*resultingTotal + 1) * sizeof(*list));
    u32 overallIdx = 0, idx = 0;

    while (idx < *resultingTotal)
    {
        enum Item item = sDynamicShopItemListRef[idx];

        if (IsItemShopCriteriaFulfilled(item))
        {
            list[overallIdx] = item;
            overallIdx++;
        }

        idx++;
    }

    list[overallIdx] = ITEM_NONE;

    *ogItemList = list;
    *resultingTotal = overallIdx;
}

void TryFreeDynamicShopItemList(const u32 **ogItemList)
{
    Free((u32 *)*ogItemList);
    *ogItemList = sDynamicShopItemListRef;
    if (sDynamicShopItemTotal != NULL)
        *sDynamicShopItemTotal = sDynamicShopItemTotalRef;
    sDynamicShopItemTotal = NULL;
}

// Add new Criterias below!

static UNUSED bool32 ShopCriteriaByBadgeCount(u32 count)
{
    u32 badgeCount = 0;

    for (u32 badgeFlag = FLAG_BADGE01_GET; badgeFlag < FLAG_BADGE01_GET + NUM_BADGES; badgeFlag++)
    {
        if (FlagGet(badgeFlag))
            badgeCount++;
    }

    if (badgeCount >= count)
        return TRUE;

    return FALSE;
}

// These two below are somewhat identical to ShopCriteriaByBadgeCount
// but uses only one specific event var/flag check. Useful if you need
// a specific badge flag instead of just the badge total.

static UNUSED bool32 ShopCriteriaByFlag(u32 flagId)
{
    bool8 value;
    if (ADMG_TryReadFlag(flagId, &value) && value)
        return TRUE;

    return FALSE;
}

static UNUSED bool32 ShopCriteriaByVar(u32 varId, u32 varValue)
{
    u32 value;
    if (ADMG_TryReadVar(varId, &value, TRUE) && value >= varValue)
        return TRUE;

    return FALSE;
}
