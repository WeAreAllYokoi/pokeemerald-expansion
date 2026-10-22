#ifndef GUARD_CONFIG_NAME_BOX_H
#define GUARD_CONFIG_NAME_BOX_H

#define OW_FLAG_SUPPRESS_NAME_BOX   0  // If this flag is set, any namebox (whether its from a macro or a code) will not show up until this flag is unset.

// Namebox Speaker configs
#define ADMG_DEFAULT_OW_NAME_BOX_USE_DYNAMIC_WIDTH TRUE  // When TRUE, the namebox window can use different width depending on the length of the speaker's name.
#define OW_NAME_BOX_USE_DYNAMIC_WIDTH ADMG_CONFIG_EXTRA(OW_NAME_BOX_USE_DYNAMIC_WIDTH)
#define ADMG_DEFAULT_OW_NAME_BOX_NPC_TRAINER       FALSE // Show approaching NPC trainers' names from their trainer data
#define OW_NAME_BOX_NPC_TRAINER ADMG_CONFIG_EXTRA(OW_NAME_BOX_NPC_TRAINER)
#define ADMG_DEFAULT_OW_NAME_BOX_DEFAULT_WIDTH     8     // Width in tiles, or the maximum width when dynamic sizing is enabled
#define OW_NAME_BOX_DEFAULT_WIDTH ADMG_CONFIG_EXTRA(OW_NAME_BOX_DEFAULT_WIDTH)
#define ADMG_DEFAULT_OW_NAME_BOX_DEFAULT_HEIGHT    2     // Name-box height in tiles
#define OW_NAME_BOX_DEFAULT_HEIGHT ADMG_CONFIG_EXTRA(OW_NAME_BOX_DEFAULT_HEIGHT)

// Text colors of Namebox. The numbers corresponds to the palette index.
// The BG color is not provided as it always needs to be 0.
#define ADMG_DEFAULT_OW_NAME_BOX_FOREGROUND_COLOR  1
#define OW_NAME_BOX_FOREGROUND_COLOR ADMG_CONFIG_EXTRA(OW_NAME_BOX_FOREGROUND_COLOR)
#define ADMG_DEFAULT_OW_NAME_BOX_SHADOW_COLOR      2
#define OW_NAME_BOX_SHADOW_COLOR ADMG_CONFIG_EXTRA(OW_NAME_BOX_SHADOW_COLOR)

#endif // GUARD_CONFIG_NAME_BOX_H
