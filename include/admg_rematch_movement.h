#ifndef ADMG_REMATCH_MOVEMENT_H
#define ADMG_REMATCH_MOVEMENT_H

#include "admg_native.h"

struct ObjectEvent;
struct ObjectEventGraphicsInfo;
struct Sprite;
struct SpriteFrameImage;
union AnimCmd;

/* Appended identifiers approved in the C4 movement addendum. No existing
 * Expansion movement or action is renumbered. */
enum { ADMG_NATIVE_REMATCH_STOP = 0x59, ADMG_NATIVE_REMATCH_JUMP = 0x5A,
       ADMG_NATIVE_REMATCH_SWIM = 0x5B };

struct ADMG_NativeRematchGraphics
{
    const struct ObjectEventGraphicsInfo *info;
    const struct SpriteFrameImage *images;
    u32 frames;
    u32 frameBytes;
    u16 graphicsId;
    u8 movementType;
};

extern const struct ADMG_NativeRematchGraphics gADMG_NativeRematchGraphics[];
extern const u32 gADMG_NativeRematchGraphicsCount;
extern const union AnimCmd *const gADMG_NativeRematchAnimations[];

/* Built-in metadata covers the retained compatible native payloads. Named
 * hooks may supply compatible mod-owned descriptors; this is not a campaign,
 * region or permanent graphics-registration restriction. */
ADMG_NATIVE_ENTRY bool8 ADMG_IsNativeRematchMovement(u32 movementType);
ADMG_NATIVE_ENTRY bool8 ADMG_TryGetNativeRematchGraphics(u32 graphicsId,
    const struct ADMG_NativeRematchGraphics **graphics);
ADMG_NATIVE_ENTRY bool8 ADMG_TryGetNativeRematchMovement(u32 graphicsId, u8 *movementType);
ADMG_NATIVE_ENTRY bool8 ADMG_IsNativeRematchSpritePair(const struct ObjectEvent *object, const struct Sprite *sprite);
ADMG_NATIVE_ENTRY bool8 ADMG_TryGetNativeRematchObjectForSprite(const struct Sprite *sprite, struct ObjectEvent **object);
ADMG_NATIVE_ENTRY bool8 ADMG_TryPrepareNativeRematchSprite(const struct ObjectEvent *object, struct Sprite *sprite);
ADMG_NATIVE_ENTRY bool8 ADMG_TryRestoreNativeRematchSprite(const struct ObjectEvent *object, struct Sprite *sprite);

#endif
