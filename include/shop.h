#ifndef GUARD_SHOP_H
#define GUARD_SHOP_H

extern struct ItemSlot gMartPurchaseHistory[3];

// Shop lists use 32-bit IDs (.4byte in scripts), ending in ITEM_NONE or DECOR_NONE
// Keep the supplied list alive while the shop is open
void CreatePokemartMenu(const u32 *itemsForSale);
void CreateDecorationShop1Menu(const u32 *itemsForSale);
void CreateDecorationShop2Menu(const u32 *itemsForSale);
void CB2_ExitSellMenu(void);

#endif // GUARD_SHOP_H
