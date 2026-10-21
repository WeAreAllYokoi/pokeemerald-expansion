#ifndef GUARD_SCRIPT_MENU_H
#define GUARD_SCRIPT_MENU_H

#include "list_menu.h"
#include "constants/script_menu.h"
#include "menu.h"

// The default size the stack for dynamic multichoice is initialized to
// If you try to push an element when the stack is full, it will be reallocated
// With increasing capacity of MULTICHOICE_DYNAMIC_STACK_INC

#define MULTICHOICE_DYNAMIC_STACK_SIZE 5
#define MULTICHOICE_DYNAMIC_STACK_INC 5
// Maximum rows shown before scrolling when maxBeforeScroll is 0xFF
#define DYN_MULTICHOICE_DEFAULT_MAX_BEFORE_SCROLL 6

extern const u8 *const gStdStrings[];

struct DynamicMultichoiceStack
{
    s32 top;
    u32 capacity;
    struct ListMenuItem *elements;
};

void MultichoiceDynamic_InitStack(u32 capacity);
void MultichoiceDynamic_ReallocStack(u32 newCapacity);
bool32 MultichoiceDynamic_StackFull(void);
bool32 MultichoiceDynamic_StackEmpty(void);
u32 MultichoiceDynamic_StackSize(void);
void MultichoiceDynamic_PushElement(struct ListMenuItem item);
struct ListMenuItem *MultichoiceDynamic_PopElement(void);
struct ListMenuItem *MultichoiceDynamic_PeekElement(void);
struct ListMenuItem *MultichoiceDynamic_PeekElementAt(u32 index);
void MultichoiceDynamic_DestroyStack(void);
bool8 ScriptMenu_MultichoiceDynamic(u8 left, u8 top, u8 argc, struct ListMenuItem *items, bool8 ignoreBPress, u8 maxBeforeScroll, u32 initialRow, u32 callbackSet);
// The platform API uses the same menu code as dynmultipush and dynmultistack
// Pass the label and ID separately instead of passing ListMenuItem by value
void ADMG_DynamicMultichoice_Push(const u8 *name, u32 id);
bool32 ADMG_DynamicMultichoice_ShowStack(u32 left, u32 top, bool32 ignoreBPress, u32 maxBeforeScroll, bool32 shouldSort, u32 initialSelected, u32 callbackSet);
u32 ADMG_DynamicMultichoice_PendingCount(void);
void ADMG_DynamicMultichoice_Discard(void);
bool32 ADMG_DynamicMultichoice_CanPreview(u32 callbackSet, u32 id);
bool8 ScriptMenu_Multichoice(u8 left, u8 top, u8 multichoiceId, bool8 ignoreBPress);
bool8 ScriptMenu_MultichoiceWithDefault(u8 left, u8 top, u8 multichoiceId, bool8 ignoreBPress, u8 defaultChoice);
void DrawMultichoiceMenuInternal(u8 left, u8 top, u8 multichoiceId, bool8 ignoreBPress, u8 cursorPos, const struct MenuAction *actions, int count);
bool8 ScriptMenu_YesNo(u8 left, u8 top);
bool8 ScriptMenu_MultichoiceGrid(u8 left, u8 top, u8 multichoiceId, bool8 ignoreBPress, u8 columnCount);
bool8 ScriptMenu_ShowPokemonPic(enum Species species, u8 x, u8 y);
bool8 (*ScriptMenu_HidePokemonPic(void))(void);
int ConvertPixelWidthToTileWidth(int width);
u8 CreateWindowFromRect(u8 x, u8 y, u8 width, u8 height);
void ClearToTransparentAndRemoveWindow(u8 windowId);
int DisplayTextAndGetWidth(const u8 *str, int prevWidth);
int ScriptMenu_AdjustLeftCoordFromWidth(int left, int width);
bool16 ScriptMenu_CreatePCMultichoice(void);
void ScriptMenu_DisplayPCStartupPrompt(void);

#endif //GUARD_SCRIPT_MENU_H
