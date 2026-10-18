#ifndef GUARD_TYPE_ICON_ANIM_H
#define GUARD_TYPE_ICON_ANIM_H

#include "constants/pokemon.h"

struct Sprite;

// Select a type icon through a named hook that mods can replace
void SetTypeIconSpriteAnim(struct Sprite *sprite, enum Type type);

// Mark contest categories so the shared icon sprites can tell them apart from types
#define TYPE_ICON_CONTEST_CATEGORY_FLAG 0x80000000
#define TYPE_ICON_CONTEST_CATEGORY(category) (TYPE_ICON_CONTEST_CATEGORY_FLAG | (category))

#endif // GUARD_TYPE_ICON_ANIM_H
