#ifndef GUARD_FIELD_MOVE_H
#define GUARD_FIELD_MOVE_H

#include "global.h"
#include "admg_native.h"
#include "constants/party_menu.h"
#include "constants/field_move.h"

struct FieldMoveUnlock
{
   bool32 (*isUnlockedFunc)(enum FieldMove);
   const u8 *lockedMessage;
};

enum FieldMoveUnlockType
{
    CANT_UNLOCK,
    ALWAYS_UNLOCKED,
    BADGE_UNLOCK,
    FIELD_MOVE_UNLOCK_COUNT
};

struct FieldMoveInfo
{
    bool32 (*fieldMoveFunc)(void);
    enum FieldMoveUnlockType unlockType:3;
    enum Move moveID;
    u32 partyMsgID:7;
    u32 arg:8;
    u32 hideIfLocked:1;
    u32 padding:3;
};

extern const struct FieldMoveInfo gFieldMoveInfo[];
extern const struct FieldMoveUnlock gFieldMoveUnlocks[];
extern const u8 gText_EmptyString2[];
ADMG_NATIVE_ENTRY const struct FieldMoveInfo *FieldMove_GetInfo(enum FieldMove fieldMove);

static inline bool32 SetUpFieldMove(enum FieldMove fieldMove)
{
    const struct FieldMoveInfo *info = FieldMove_GetInfo(fieldMove);
    return info != NULL && info->fieldMoveFunc != NULL ? info->fieldMoveFunc() : FALSE;
}

static inline bool32 IsFieldMoveUnlocked(enum FieldMove fieldMove)
{
    const struct FieldMoveInfo *info = FieldMove_GetInfo(fieldMove);
    if (info == NULL || info->unlockType >= FIELD_MOVE_UNLOCK_COUNT
     || gFieldMoveUnlocks[info->unlockType].isUnlockedFunc == NULL)
        return FALSE;
    return gFieldMoveUnlocks[info->unlockType].isUnlockedFunc(fieldMove);
}

static inline const u8 *FieldMove_GetLockedMessage(enum FieldMove fieldMove)
{
    const struct FieldMoveInfo *info = FieldMove_GetInfo(fieldMove);
    if (info == NULL || info->unlockType >= FIELD_MOVE_UNLOCK_COUNT)
        return gText_EmptyString2;
    const u8 *message = gFieldMoveUnlocks[info->unlockType].lockedMessage;
    return message != NULL ? message : gText_EmptyString2;
}

static inline enum Move FieldMove_GetMoveId(enum FieldMove fieldMove)
{
    const struct FieldMoveInfo *info = FieldMove_GetInfo(fieldMove);
    return info != NULL ? info->moveID : MOVE_NONE;
}

static inline u32 FieldMove_GetPartyMsgID(enum FieldMove fieldMove)
{
    const struct FieldMoveInfo *info = FieldMove_GetInfo(fieldMove);
    return info != NULL ? info->partyMsgID : PARTY_MSG_CANT_USE_HERE;
}

static inline bool32 FieldMove_IsVisible(enum FieldMove fieldMove)
{
    const struct FieldMoveInfo *info = FieldMove_GetInfo(fieldMove);
    return info != NULL && (!info->hideIfLocked || IsFieldMoveUnlocked(fieldMove));
}

#endif //GUARD_FIELD_MOVE_H
