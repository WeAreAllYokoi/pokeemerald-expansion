#ifndef GUARD_CONFIG_MAP_PREVIEW_H
#define GUARD_CONFIG_MAP_PREVIEW_H

#define MPS_ENABLE_MAP_PREVIEWS     IS_FRLG // Enables map previews.

#define ADMG_DEFAULT_MPS_DURATION_LONG       120 // This is the number of frames the map preview will last when the player enters the map for the first time.
#define MPS_DURATION_LONG ADMG_CONFIG_EXTRA(MPS_DURATION_LONG)
#define ADMG_DEFAULT_MPS_DURATION_SHORT      40  // This is the number of frames the map preview will last if the player has visited the map before.
#define MPS_DURATION_SHORT ADMG_CONFIG_EXTRA(MPS_DURATION_SHORT)
#define ADMG_DEFAULT_MPS_DURATION_NO_FLAG    40  // This is the number of frames the map preview will last if the map's flagId is MPS_FLAG_NULL.
#define MPS_DURATION_NO_FLAG ADMG_CONFIG_EXTRA(MPS_DURATION_NO_FLAG)
#define ADMG_DEFAULT_MPS_BASIC_FADE_SPEED    1   // This sets the speed that the map preview fades to black when MPS_TYPE_BASIC is used.
#define MPS_BASIC_FADE_SPEED ADMG_CONFIG_EXTRA(MPS_BASIC_FADE_SPEED)
                                    // Numbers less than 1 will have a shorter fade time (negative numbers can be used).
                                    // Numbers greater than 1 will have a longer fade time.

#endif // GUARD_CONFIG_MAP_PREVIEW_H
