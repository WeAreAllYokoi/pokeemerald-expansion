#ifndef GUARD_TYPE_ICON_ANIM_H
#define GUARD_TYPE_ICON_ANIM_H

#include "constants/pokemon.h"

struct Sprite;

// Select a type icon through a named hook that mods can replace
void SetTypeIconSpriteAnim(struct Sprite *sprite, enum Type type);

#endif // GUARD_TYPE_ICON_ANIM_H
