#ifndef GUARD_EVENT_DATA_H
#define GUARD_EVENT_DATA_H

#include "admg_native.h"

void InitEventData(void);
void ClearTempFieldEventData(void);
void ClearDailyFlags(void);
void DisableNationalPokedex(void);
void EnableNationalPokedex(void);
bool32 IsNationalPokedexEnabled(void);
bool32 IsMysteryEventEnabled(void);
bool32 IsMysteryGiftEnabled(void);
void ClearMysteryGiftFlags(void);
void ClearMysteryGiftVars(void);
void DisableResetRTC(void);
void EnableResetRTC(void);
bool32 CanResetRTC(void);
// Each script variable holds four bytes
// Use this to count how many variables a record needs and check their spacing
#define VAR_BYTES sizeof(u32)

ADMG_NATIVE_ENTRY u32 *GetVarPointer(u16 id);
ADMG_NATIVE_ENTRY u32 VarGet(u16 id);
ADMG_NATIVE_ENTRY u32 VarGetIfExist(u16 id);
ADMG_NATIVE_ENTRY bool8 VarSet(u16 id, u32 value);
ADMG_NATIVE_ENTRY u32 VarGetObjectEventGraphicsId(u8 id);
ADMG_NATIVE_ENTRY u8 *GetFlagPointer(u16 id);
ADMG_NATIVE_ENTRY u8 FlagSet(u16 id);
ADMG_NATIVE_ENTRY u8 FlagToggle(u16 id);
ADMG_NATIVE_ENTRY u8 FlagClear(u16 id);
ADMG_NATIVE_ENTRY bool8 FlagGet(u16 id);

extern u32 gSpecialVar_0x8000;
extern u32 gSpecialVar_0x8001;
extern u32 gSpecialVar_0x8002;
extern u32 gSpecialVar_0x8003;
extern u32 gSpecialVar_0x8004;
extern u32 gSpecialVar_0x8005;
extern u32 gSpecialVar_0x8006;
extern u32 gSpecialVar_0x8007;
extern u32 gSpecialVar_0x8008;
extern u32 gSpecialVar_0x8009;
extern u32 gSpecialVar_0x800A;
extern u32 gSpecialVar_0x800B;
extern u32 gSpecialVar_Result;
extern u32 gSpecialVar_LastTalked;
extern u32 gSpecialVar_Facing;
extern u32 gSpecialVar_MonBoxId;
extern u32 gSpecialVar_MonBoxPos;
extern u32 gSpecialVar_Unused_0x8014;

extern const u16 gBadgeFlags[NUM_BADGES];

#endif // GUARD_EVENT_DATA_H
