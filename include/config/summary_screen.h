#ifndef GUARD_CONFIG_SUMMARY_SCREEN_H
#define GUARD_CONFIG_SUMMARY_SCREEN_H

// Settings
#define P_SUMMARY_SCREEN_NATURE_COLORS   TRUE   // If TRUE, nature-based stat boosts and reductions will be red and blue in the summary screen.
#define P_SUMMARY_SCREEN_RENAME          TRUE   // If TRUE, an option to change Pokémon nicknames replaces the cancel prompt on the summary screen info page.

// IV/EV settings
#define P_SUMMARY_SCREEN_IV_EV_INFO      FALSE  // If TRUE, will allow player to cycle through the Stats, IVs, and EVs in the summary screen skills page.
#define ADMG_DEFAULT_P_SUMMARY_SCREEN_IV_EV_BOX_ONLY  FALSE  // If TRUE, will allow player to cycle through the Stats, IVs, and EVs in the summary screen skills page, but only in the PC storage box.
#define P_SUMMARY_SCREEN_IV_EV_BOX_ONLY ADMG_CONFIG_EXTRA(P_SUMMARY_SCREEN_IV_EV_BOX_ONLY)
#define ADMG_DEFAULT_P_SUMMARY_SCREEN_IV_HYPERTRAIN   TRUE   // If TRUE, stats that have been hyper trained will show as 31/S when viewing them in the summary screen
#define P_SUMMARY_SCREEN_IV_HYPERTRAIN ADMG_CONFIG_EXTRA(P_SUMMARY_SCREEN_IV_HYPERTRAIN)
#define ADMG_DEFAULT_P_SUMMARY_SCREEN_IV_EV_TILESET   FALSE  // If TRUE, use the alternate summary tileset that allows the STATS label to change
#define P_SUMMARY_SCREEN_IV_EV_TILESET ADMG_CONFIG_EXTRA(P_SUMMARY_SCREEN_IV_EV_TILESET)
#define P_SUMMARY_SCREEN_IV_EV_VALUES    FALSE  // If TRUE, will show the actual IV value instead of the letter grade.
/*
LETTER GRADE GUIDE:

    F = 0
    D = 1 - 15
    C = 16 - 25
    B = 26 - 29
    A = 30
    S = 31

Info taken from https://bulbapedia.bulbagarden.net/wiki/Stats_judge.
*/
#define ADMG_DEFAULT_P_SUMMARY_SCREEN_IV_ONLY         FALSE  // If TRUE, will only show IV info in the summary screen.
#define P_SUMMARY_SCREEN_IV_ONLY ADMG_CONFIG_EXTRA(P_SUMMARY_SCREEN_IV_ONLY)
#define ADMG_DEFAULT_P_SUMMARY_SCREEN_EV_ONLY         FALSE  // If TRUE, will only show EV info in the summary screen.
#define P_SUMMARY_SCREEN_EV_ONLY ADMG_CONFIG_EXTRA(P_SUMMARY_SCREEN_EV_ONLY)

// IV/EV flags
#define P_FLAG_SUMMARY_SCREEN_IV_EV_INFO 0      // If this flag is set, will allow player to cycle through the Stats, IVs, and EVs in the summary screen skills page. Note: if P_SUMMARY_SCREEN_IV_EV_INFO is TRUE, this flag does nothing.

// Move Relearner settings
#define ADMG_DEFAULT_P_ENABLE_MOVE_RELEARNERS         FALSE   // If TRUE, it enables move relearners for egg, TM and tutor. (see below for specific configs /flags)
#define P_ENABLE_MOVE_RELEARNERS ADMG_CONFIG_EXTRA(P_ENABLE_MOVE_RELEARNERS)
#define ADMG_DEFAULT_P_SORT_MOVES                     FALSE   // If TRUE, sorts all moves alphabetically in the relearner's list.
#define P_SORT_MOVES ADMG_CONFIG_EXTRA(P_SORT_MOVES)

// Level up Relearner
#define ADMG_DEFAULT_P_PRE_EVO_MOVES                  FALSE   // If TRUE, it enables the Pokémon to learn moves from it's pre evolution.
#define P_PRE_EVO_MOVES ADMG_CONFIG_EXTRA(P_PRE_EVO_MOVES)
#define ADMG_DEFAULT_P_ENABLE_ALL_LEVEL_UP_MOVES      FALSE   // If TRUE, it enables the Pokémon to learn all level up moves, regardless of its level.
#define P_ENABLE_ALL_LEVEL_UP_MOVES ADMG_CONFIG_EXTRA(P_ENABLE_ALL_LEVEL_UP_MOVES)

// TM Relearner
#define ADMG_DEFAULT_P_TM_MOVES_RELEARNER             FALSE   // If TRUE, enables machine move relearner.
#define P_TM_MOVES_RELEARNER ADMG_CONFIG_EXTRA(P_TM_MOVES_RELEARNER)
#define ADMG_DEFAULT_P_ENABLE_ALL_TM_MOVES            FALSE   // If TRUE, it enables the Pokémon to learn all TMs its compatible with, regardless of it being in the bag.
#define P_ENABLE_ALL_TM_MOVES ADMG_CONFIG_EXTRA(P_ENABLE_ALL_TM_MOVES)

// Relearner flags - Redundant if P_ENABLE_MOVE_RELEARNERS is TRUE, but still added here incase you don't want all relearners unlocked at the same time.
// To use the following features in scripting, replace the 0s with the flag ID you're assigning it to.
// Eg: Replace with FLAG_UNUSED_0x264 so you can use that flag to toggle the feature.
#define P_FLAG_EGG_MOVES                 0       // If this flag is set, enables egg move relearner.
#define P_FLAG_TUTOR_MOVES               0       // If this flag is set, enables tutor move relearner.

// Move Relearner summary screen
#define P_SUMMARY_SCREEN_MOVE_RELEARNER  TRUE   // If TRUE, shows an option for Pokémon to relearn moves on the summary screen moves page.
#define ADMG_DEFAULT_P_SUMMARY_MOVE_RELEARNER_FULL_PP TRUE   // If TRUE, the move relearner in the summary screen restores relearned moves' PP to full.
#define P_SUMMARY_MOVE_RELEARNER_FULL_PP ADMG_CONFIG_EXTRA(P_SUMMARY_MOVE_RELEARNER_FULL_PP)

#endif // GUARD_CONFIG_SUMMARY_SCREEN_H
