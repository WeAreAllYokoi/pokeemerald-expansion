#!/usr/bin/env python3
"""Add include/config options Expansion reads directly (not through GetConfig) to the run-time configuration selector.

Each option's definition in its include/config header keeps its compiled value as ADMG_DEFAULT_<name>, and the option's
own name becomes ADMG_CONFIG_EXTRA(<name>): ADMG_GetConfig(CONFIG_<name>) in normal builds, the default in test builds.
Every C read therefore follows a selection; a read where a compile-time constant is required (#if, static initializer,
assembly) fails to build, so the build itself proves no read is left behind. The generated
include/constants/admg_config_extra.h gives each option a tag (from 0x1000, named CONFIG_<name> like Expansion's own
tags) and the largest value its consumers accept; config_changes.c answers the selector's default, maximum and known
queries from it, and the SDK lists the options in game.Config by their source names.

Usage: route_r38_config_extra.py <native source> [--check]
  writes the header and the config-header definitions; --check only reports whether they are current.
"""
import argparse
import re
import sys
from pathlib import Path

GENERATION = 'GEN_COUNT - 1'
# (option, largest accepted value): Booleans 1, generations GEN_COUNT - 1, enumerations their last named value, numbers
# the range their consumers use (percentages 100, byte-sized dimensions and colours 255).
OPTIONS = [
    ('B_CATCH_SWAP_INTO_PARTY', GENERATION), ('B_CATCH_SWAP_CHECK_HMS', '1'), ('B_QUICK_MOVE_CURSOR_TO_RUN', '1'),
    ('B_FAST_INTRO_PKMN_TEXT', '1'), ('B_FAST_INTRO_NO_SLIDE', '1'), ('B_FAST_HP_DRAIN', '1'), ('B_FAST_EXP_GROW', '1'),
    ('B_RARE_CANDY_CAP', '1'), ('B_SLEEP_CLAUSE', '1'), ('B_SHOW_TYPES', 'SHOW_TYPES_SEEN'), ('B_RUN_TRAINER_BATTLE', '1'),
    ('ASSUME_STAB_SEES_ABILITY', '1'), ('ASSUME_STATUS_MOVES_HAS_TUNING', '1'), ('ASSUME_STATUS_HIGH_ODDS', '100'),
    ('ASSUME_STATUS_MEDIUM_ODDS', '100'), ('ASSUME_STATUS_LOW_ODDS', '100'), ('ASSUME_ALL_STATUS_ODDS', '100'),
    ('SMART_SWITCHING_OMNISCIENT', '1'), ('PREDICT_SWITCH_CHANCE', '100'), ('PREDICT_MOVE_CHANCE', '100'),
    ('AI_DAMAGES_THROUGH_BERRIES', '1'), ('AI_IGNORE_BERRY_KO_THRESHOLD', '255'),
    ('P_EGG_HATCH_LEVEL', GENERATION), ('P_INCENSE_BREEDING', GENERATION),
    ('HGSS_DARK_MODE', '1'), ('HGSS_DECAPPED', '1'),
    ('OW_RUNNING_INDOORS', GENERATION), ('OW_WHITEOUT_CUTSCENE', GENERATION), ('OW_ENABLE_DNS', '1'),
    ('OW_POPUP_BW_COLOR', 'OW_POPUP_BW_COLOR_WHITE'), ('OW_POPUP_BW_ALPHA_BLEND', '1'),
    ('OW_POPUP_BW_TIME_MODE', 'OW_POPUP_BW_TIME_24_HR'), ('OW_HIDE_REPEAT_MAP_POPUP', '1'),
    ('OW_NAME_BOX_USE_DYNAMIC_WIDTH', '1'), ('OW_NAME_BOX_DEFAULT_WIDTH', '255'), ('OW_NAME_BOX_DEFAULT_HEIGHT', '255'),
    ('OW_NAME_BOX_FOREGROUND_COLOR', '255'), ('OW_NAME_BOX_SHADOW_COLOR', '255'), ('OW_NAME_BOX_NPC_TRAINER', '1'),
    ('I_FISHING_FOLLOWER_BOOST', '1'), ('I_FISHING_PROXIMITY', '1'), ('I_FISHING_TIME_OF_DAY_BOOST', '1'),
    ('I_FISHING_MINIGAME', GENERATION),
    # Bucket B: options read only in C code (work/classify_bucket_b.py)
    ('AI_BAD_SCORE_THRESHOLD', '255'), ('AI_DEFENSIVE_KO_THRESHOLD', '255'),
    ('AI_FLAG_ATTACKS_PARTNER_FOCUSES_PARTNER', '1'), ('AI_GOOD_SCORE_THRESHOLD', '255'),
    ('AI_ROLL_ATTACKING_PARTNER', 'AI_ROLL_RANDOM'), ('AI_ROLL_DEFENDING', 'AI_ROLL_RANDOM'),
    ('AI_ROLL_SHOULD_SETUP_DEFENDING', 'AI_ROLL_RANDOM'), ('AI_ROLL_SWITCHIN_ATTACKING', 'AI_ROLL_RANDOM'),
    ('AI_ROLL_SWITCHIN_DEFENDING', 'AI_ROLL_RANDOM'), ('AI_TYPE_MATCHUP_THRESHOLD', '65535'),
    ('AI_WISH_HEAL_THRESHOLD', '255'), ('ALL_MOVES_BAD_NEEDS_GOOD_SWITCHIN', '1'),
    ('ALL_MOVES_BAD_STATUS_MOVES_BAD', '1'), ('ALL_SCORES_BAD_NEEDS_GOOD_SWITCHIN', '1'),
    ('B_CATCHING_CHARM_BOOST', '255'), ('B_CRITICAL_CAPTURE', '1'), ('B_CRITICAL_CAPTURE_LOCAL_DEX', '1'),
    ('B_ENEMY_THROW_BALLS', GENERATION), ('B_ENEMY_THROW_BALLS_SOUND', GENERATION),
    ('B_EVOLUTION_AFTER_WHITEOUT', GENERATION), ('B_EV_ITEMS_CAP', '1'), ('B_HPBAR_COLOR_THRESHOLD', GENERATION),
    ('B_LEVEL_CAP_EXP_UP', '1'), ('B_MOVE_REARRANGEMENT_IN_BATTLE', GENERATION),
    ('B_PLAYER_THROW_BALLS_SOUND', GENERATION), ('B_PROTECT_FAILURE_RATE', GENERATION),
    ('B_SHOW_MOVE_DESCRIPTION', '1'),
    ('B_SNATCH', GENERATION), ('B_TOXIC_REVERSAL', GENERATION),
    ('B_TRY_CATCH_TRAINER_BALL', GENERATION),
    ('DOUBLE_TRICK_ROOM_ON_LAST_TURN_CHANCE', '100'), ('FAKE_OUT_SAVE_ALLY_CHANCE', '100'),
    ('FRIENDLY_FIRE_CONSERVATIVE_THRESHOLD', '255'), ('FRIENDLY_FIRE_NORMAL_THRESHOLD', '255'),
    ('FRIENDLY_FIRE_RISKY_THRESHOLD', '255'), ('HGSS_HIDE_UNOWNED_EVOLUTION_METHODS', '1'),
    ('HGSS_HIDE_UNSEEN_EVOLUTION_NAMES', '1'), ('HGSS_SHOW_EGG_MOVES_FOR_EVOS', '1'),
    ('I_FISHING_ENVIRONMENT', GENERATION), ('I_FISHING_STICKY_BOOST', GENERATION),
    ('I_REPEL_INCLUDE_FAINTED', GENERATION), ('I_VITAMIN_EV_CAP', GENERATION), ('LAST_MON_PREFERS_NOT_SACRIFICE', '1'),
    ('OW_OBJECT_SUBPRIORITY', '255'), ('OW_OBJECT_VANILLA_SHADOWS', '1'), ('OW_SHADOW_INTENSITY', '16'),
    ('P_SUMMARY_SCREEN_EV_ONLY', '1'), ('P_SUMMARY_SCREEN_IV_EV_BOX_ONLY', '1'),
    ('P_SUMMARY_SCREEN_IV_HYPERTRAIN', '1'), ('P_SUMMARY_SCREEN_IV_ONLY', '1'), ('RANDOMIZE_SWITCHIN_ANY_VALID', '1'),
    ('RISKY_AI_CRIT_STAGE_THRESHOLD', '255'), ('RISKY_AI_CRIT_THRESHOLD_GEN_1', '255'),
    ('SHOULD_SWITCH_ABSORBS_HIDDEN_POWER_PERCENTAGE', '100'), ('SHOULD_SWITCH_ABSORBS_MOVE_PERCENTAGE', '100'),
    ('SHOULD_SWITCH_ALL_SCORES_BAD_PERCENTAGE', '100'),
    ('SHOULD_SWITCH_ATTACKING_STAT_MINUS_THREE_PLUS_PERCENTAGE', '100'),
    ('SHOULD_SWITCH_ATTACKING_STAT_MINUS_TWO_PERCENTAGE', '100'), ('SHOULD_SWITCH_BADLY_POISONED_PERCENTAGE', '100'),
    ('SHOULD_SWITCH_BADLY_POISONED_STATS_RAISED_PERCENTAGE', '100'), ('SHOULD_SWITCH_CHOICE_LOCKED_PERCENTAGE', '100'),
    ('SHOULD_SWITCH_CURSED_PERCENTAGE', '100'), ('SHOULD_SWITCH_CURSED_STATS_RAISED_PERCENTAGE', '100'),
    ('SHOULD_SWITCH_DYN_FUNC_PERCENTAGE', '100'), ('SHOULD_SWITCH_ENCORE_DAMAGE_PERCENTAGE', '100'),
    ('SHOULD_SWITCH_ENCORE_STATUS_PERCENTAGE', '100'), ('SHOULD_SWITCH_FREE_TURN_PERCENTAGE', '100'),
    ('SHOULD_SWITCH_HASBADODDS_PERCENTAGE', '100'), ('SHOULD_SWITCH_INFATUATION_PERCENTAGE', '100'),
    ('SHOULD_SWITCH_INTIMIDATE_PERCENTAGE', '100'), ('SHOULD_SWITCH_INTIMIDATE_STATS_RAISED_PERCENTAGE', '100'),
    ('SHOULD_SWITCH_NATURAL_CURE_STRONG_PERCENTAGE', '100'),
    ('SHOULD_SWITCH_NATURAL_CURE_STRONG_STATS_RAISED_PERCENTAGE', '100'),
    ('SHOULD_SWITCH_NATURAL_CURE_WEAK_PERCENTAGE', '100'),
    ('SHOULD_SWITCH_NATURAL_CURE_WEAK_STATS_RAISED_PERCENTAGE', '100'), ('SHOULD_SWITCH_NIGHTMARE_PERCENTAGE', '100'),
    ('SHOULD_SWITCH_NIGHTMARE_STATS_RAISED_PERCENTAGE', '100'), ('SHOULD_SWITCH_PERISH_SONG_PERCENTAGE', '100'),
    ('SHOULD_SWITCH_REGENERATOR_PERCENTAGE', '100'), ('SHOULD_SWITCH_REGENERATOR_STATS_RAISED_PERCENTAGE', '100'),
    ('SHOULD_SWITCH_SEEDED_PERCENTAGE', '100'), ('SHOULD_SWITCH_SEEDED_STATS_RAISED_PERCENTAGE', '100'),
    ('SHOULD_SWITCH_TRAPPER_PERCENTAGE', '100'), ('SHOULD_SWITCH_TRUANT_PERCENTAGE', '100'),
    ('SHOULD_SWITCH_WISH_PASSING_PERCENTAGE', '100'), ('SHOULD_SWITCH_WONDER_GUARD_PERCENTAGE', '100'),
    ('SHOULD_SWITCH_YAWN_PERCENTAGE', '100'), ('STAY_IN_ABSORBING_PERCENTAGE', '100'), ('STAY_IN_STATS_RAISED', '255'),
    ('TAILWIND_IN_TRICK_ROOM_CHANCE', '100'), ('WE_DOUBLE_WILD_REQUIRE_2_MONS', '1'),
    # Bucket B: #if reads turned into run-time choices (caps checks, fishing odds, popup assets, battle windows)
    ('AI_SWITCHIN_DAMAGE_THRESHOLD', '255'), ('WE_DOUBLE_WILD_CHANCE', '100'),     ('B_EXP_CAP_TYPE', 'EXP_CAP_SOFT'), ('B_LEVEL_CAP_TYPE', 'LEVEL_CAP_VARIABLE'), ('B_EV_CAP_TYPE', 'EV_CAP_NO_GAIN'),
    ('I_FISHING_BITE_ODDS', GENERATION), ('OW_POPUP_GENERATION', GENERATION), ('B_LAST_USED_BALL', '1'),
    ('B_LAST_USED_BALL_BUTTON', '1023'), ('B_MOVE_DESCRIPTION_BUTTON', '1023'),
    # Read in code and data: the data takes the source value and the build measures it per value (data variants)
    ('I_EXP_SHARE_ITEM', GENERATION), ('B_ENEMY_MON_SHADOW_STYLE', GENERATION),
    # Step 3: compiled-in choices turned into run-time ones (patch 0053)
    ('B_TRAINER_CLASS_POKE_BALLS', GENERATION), ('B_SHOW_EFFECTIVENESS', 'SHOW_EFFECTIVENESS_SEEN'), ('P_EV_CAP', GENERATION),
    ('B_LAST_USED_BALL_CYCLE', '1'), ('OW_DEFOG_FIELD_MOVE', '1'), ('OW_ROCK_CLIMB_FIELD_MOVE', '1'),
    ('P_SUMMARY_SCREEN_IV_EV_TILESET', '1'), ('OW_POISON_DAMAGE', GENERATION), ('I_REPEL_LURE_MENU', '1'),
    # Read by the teachable-move generator: it builds every variant and GetSpeciesTeachableLearnset picks one (patch 0055)
    ('HGSS_SORT_TMS_BY_NUM', '1'), ('P_TM_LITERACY', GENERATION),
    # Read only in C code (config-inventory backlog-code, patch 0057). Maxima: Booleans 1, generations GEN_COUNT - 1,
    # percentages 100, enumerations their last value, otherwise the storage their consumers use.
    ('AI_CONSERVE_TERA_CHANCE_PER_MON', '100'), ('AI_TERA_PREDICT_CHANCE', '100'), ('BOOST_INTO_HAZE_CHANCE', '100'),
    ('EXPLOSION_HIGHER_HP_THRESHOLD', '100'), ('EXPLOSION_LOWER_HP_THRESHOLD', '100'), ('EXPLOSION_MAXIMUM_CHANCE', '100'),
    ('EXPLOSION_MINIMUM_CHANCE', '100'), ('FINAL_GAMBIT_CHANCE', '100'), ('LOW_ACCURACY_THRESHOLD', '100'),
    ('POWER_SPLIT_ALLY_PERCENTAGE', '65535'), ('POWER_SPLIT_ENEMY_PERCENTAGE', '65535'),
    ('PP_STALL_DISREGARD_MOVE_PERCENTAGE', '100'), ('PP_STALL_SCORE_REDUCTION', '255'), ('PRIORITIZE_LAST_CHANCE_CHANCE', '100'),
    ('SHOULD_PIVOT_BREAK_SASH_CHANCE', '100'), ('SHOULD_RECOVER_CHANCE', '100'), ('STATUS_MOVE_FOCUS_PUNCH_CHANCE', '100'),
    ('SUCKER_PUNCH_CHANCE', '100'), ('SUCKER_PUNCH_PREDICTION_CHANCE', '100'), ('Z_EFFECT_FOLLOW_ME_THRESHOLD', '100'),
    ('Z_EFFECT_RESTORE_HP_HIGHER_THRESHOLD', '100'), ('Z_EFFECT_RESTORE_HP_LOWER_THRESHOLD', '100'),
    # Battle: the timers fit their volatiles (confusionTimer below PERMANENT_VOLATILE, uproarTurns up to 5)
    ('B_CONFUSION_TURNS', '15'), ('B_UPROAR_TURN_COUNT', '5'), ('B_ANIMATE_MON_AFTER_FAILED_POKEBALL', '1'),
    ('B_ANIMATE_MON_AFTER_KO', '1'), ('B_HIDE_HEALTHBOX_IN_ANIMS', '1'), ('B_PARTNER_MONS_MARKED_SEEN', '1'),
    ('B_POOL_SETTING_CONSISTENT_RNG', '1'), ('B_POOL_SETTING_FIXED_SEED', '0xFFFFFFFF'), ('B_POOL_SETTING_USE_FIXED_SEED', '1'),
    ('B_SHOW_CATEGORY_ICON', '1'), ('B_SHOW_PARTNER_TARGET', '1'), ('B_SHOW_TARGETS', '1'), ('B_SHOW_USELESS_Z_MOVE_INFO', '1'),
    ('B_TERA_ORB_ALWAYS_CHARGED', '1'), ('B_TERRAIN_BG_CHANGE', '1'), ('B_THUNDERSTORM_TERRAIN', '1'),
    ('B_TRAINER_MON_RANDOM_ABILITY', '2'),
    # DexNav: tiles, a byte-sized chain and step counter, seconds (at most 1092: frames in a u16), percentages
    ('CREEPING_PROXIMITY', '255'), ('MAX_PROXIMITY', '255'), ('SNEAKING_PROXIMITY', '255'), ('DEXNAV_CHAIN_MAX', '255'),
    ('DEXNAV_TIMEOUT', '1092'), ('HIDDEN_MON_PROBABILTY', '100'), ('HIDDEN_MON_SEARCH_RATE', '100'), ('HIDDEN_MON_STEP_COUNT', '255'),
    *[(f'SEARCHLEVEL{level}_{kind}', '100') for level in (0, 5, 10, 25, 50, 100) for kind in ('MOVECHANCE', 'ONESTAR', 'TWOSTAR', 'THREESTAR')],
    # Follower NPCs, items (dowsing colours are BGR555), map preview (durations live in a task's s16 data)
    ('FNPC_FACE_NPC_FOLLOWER_ON_DOOR_EXIT', '1'), ('FNPC_NPC_FOLLOWER_PARTY_PREVIEW', '1'),
    ('FNPC_NPC_FOLLOWER_SHOW_AFTER_LEAVE_ROUTE', '1'), ('FNPC_NPC_FOLLOWER_WILD_BATTLE_VS_2', '1'),
    ('I_BERRY_EV_JUMP', GENERATION), ('I_ORAS_DOWSING_COLOR_FAST', '0x7FFF'), ('I_ORAS_DOWSING_COLOR_FASTER', '0x7FFF'),
    ('I_ORAS_DOWSING_COLOR_NONE', '0x7FFF'), ('I_ORAS_DOWSING_COLOR_NORMAL', '0x7FFF'), ('I_ORAS_DOWSING_COLOR_PAL', '15'),
    ('I_ORAS_DOWSING_COLOR_SLOW', '0x7FFF'), ('I_ORAS_DOWSING_SOUNDS', '1'), ('I_PREMIER_BALL_BONUS', GENERATION),
    ('I_ROTOM_CATALOG_THUNDER_SHOCK', GENERATION), ('I_SHINY_CHARM_ADDITIONAL_ROLLS', '255'), ('I_SHOW_NO_ID_TRAINER', '1'),
    ('MPS_BASIC_FADE_SPEED', '255'), ('MPS_DURATION_LONG', '32767'), ('MPS_DURATION_NO_FLAG', '32767'), ('MPS_DURATION_SHORT', '32767'),
    # Overworld
    ('BERRY_BLENDER_THROW_ALL_BERRIES_AT_ONCE', '1'), ('OW_ALTERED_TIME_RATIO', 'GEN_8_PLA'),
    ('OW_AMBIENT_CRIES', 'OW_AMBIENT_CRIES_OWE_ONLY'), ('OW_CHECK_FOR_TOTAL_EVS', '1'), ('OW_CHOOSE_FROM_PC_AND_PARTY', '1'),
    ('OW_DOUBLE_APPROACH_WITH_ONE_MON', '1'), ('OW_FLASH_FIRE', GENERATION), ('OW_FOLLOWERS_BOBBING', '1'),
    ('OW_FOLLOWERS_COPY_WILD_PKMN', '1'), ('OW_FOLLOWERS_SCRIPT_MOVEMENT', '1'), ('OW_HARVEST', GENERATION),
    ('OW_IGNORE_EGGS_ON_HEAL', GENERATION), ('OW_INFILTRATOR', GENERATION), ('OW_LIGHTNING_ROD', GENERATION),
    ('OW_MON_WANDER_WALK', '1'), ('OW_PC_HEAL', GENERATION), ('OW_PC_PRESS_B', GENERATION), ('OW_PC_RELEASE_ITEM', GENERATION),
    ('OW_REMATCH_BADGE_COUNT', '8'), ('OW_STORM_DRAIN', GENERATION), ('OW_SUPER_LUCK', GENERATION),
    # Pokémon (a Scatterbug form must be a valid one, as upstream requires), Pokérus (odds out of 65536), the summary-screen
    # relearner, saving, text (the slow modifier at most 31, the printer's limit) and units
    ('P_BASE_HP_1_SHEDINJA_HANDLING', '1'), ('P_EGG_CYCLE_LENGTH', GENERATION), ('P_ENABLE_ALL_LEVEL_UP_MOVES', '1'),
    ('P_ENABLE_ALL_TM_MOVES', '1'), ('P_ENABLE_MOVE_RELEARNERS', '1'), ('P_EVOLUTION_LEVEL_1_LEARN', GENERATION),
    ('P_KADABRA_EVERSTONE', GENERATION), ('P_MODIFIED_MEGA_CRY_MODE', '13'), ('P_NIDORAN_M_DITTO_BREED', GENERATION),
    ('P_NO_SHINIES_WITHOUT_POKEBALLS', '1'), ('P_ONLY_OBTAINABLE_SHINIES', '1'), ('P_POKERUS_INFECTION_ODDS', '65535'),
    ('P_POKERUS_SPREAD_ODDS', '65535'), ('P_POKERUS_STRAIN_DISTRIBUTION', GENERATION), ('P_POKERUS_VISIBLE_ON_EGG', GENERATION),
    ('P_PRE_EVO_MOVES', '1'), ('P_SCATTERBUG_LINE_FORM_BREED', 'SPECIES_SCATTERBUG_POKEBALL'), ('P_SHOW_TERA_TYPE', GENERATION),
    ('P_SHUCKLE_BERRY_JUICE', GENERATION), ('P_SKIP_POKEDEX_GAPS', 'SKIP_ALL_GAPS'), ('P_SORT_MOVES', '1'),
    ('P_SUMMARY_MOVE_RELEARNER_FULL_PP', '1'), ('P_TM_MOVES_RELEARNER', '1'), ('SKIP_SAVE_CONFIRMATION', '1'),
    ('AUTO_LOWERCASE_KEYBOARD', GENERATION), ('AUTO_SCROLL_TEXT', '1'), ('CHAR_DEC_SEPARATOR', '255'),
    ('NUM_FRAMES_AUTO_SCROLL_DELAY', '255'), ('TEXT_SPEED_INSTANT', '1'), ('TEXT_SPEED_SLOW_MODIFIER', '31'),
    ('UNITS', 'UNITS_METRIC'),
    # Battle volatile timers (the mixed backlog): each volatile field holds 15 (4 bits); Disable adds up to 3 random turns and
    # a rampage keeps its count plus one, so those two stop lower. B_PERISH_SONG_TIMER stays fixed: Expansion sets the
    # Perish Song count to a literal 3 and the option only sizes the field.
    ('B_DISABLE_TIMER', '12'), ('B_EMBARGO_TIMER', '15'), ('B_ENCORE_TIMER', '15'), ('B_HEAL_BLOCK_TIMER', '15'),
    ('B_LASER_FOCUS_TIMER', '15'), ('B_MAGNET_RISE_TIMER', '15'), ('B_RAMPAGE_TURNS', '14'), ('B_SLOW_START_TIMER', '15'),
    ('B_SYRUP_BOMB_TIMER', '15'), ('B_TAUNT_TIMER', '15'), ('B_TELEKINESIS_TIMER', '15'), ('B_THROAT_CHOP_TIMER', '15'),
    ('B_TORMENT_TIMER', '15'), ('B_WRAP_TURNS', '15'),
    # Text speed modifiers, like TEXT_SPEED_SLOW_MODIFIER (include/text.h allows up to 31)
    ('TEXT_SPEED_MEDIUM_MODIFIER', '31'), ('TEXT_SPEED_FAST_MODIFIER', '31'), ('TEXT_SPEED_INSTANT_MODIFIER', '31'),
    # DexNav hidden-ability chances per search level (percentages)
    ('SEARCHLEVEL0_ABILITYCHANCE', '100'), ('SEARCHLEVEL5_ABILITYCHANCE', '100'), ('SEARCHLEVEL10_ABILITYCHANCE', '100'),
    ('SEARCHLEVEL25_ABILITYCHANCE', '100'), ('SEARCHLEVEL50_ABILITYCHANCE', '100'), ('SEARCHLEVEL100_ABILITYCHANCE', '100'),
    # Mixed backlog batch 2
    ('SLOW_MOVEMENT_ON_STAIRS', '1'), ('OW_SYNCHRONIZE_NATURE', GENERATION), ('B_RESET_FLAGS_VARS_AFTER_WHITEOUT', '1'),
    ('ENABLE_RECOVERY_THRESHOLD', '100'), ('I_SELL_VALUE_FRACTION', GENERATION),
    # Mixed backlog batch 3
    ('B_MULTI_HALF_TEAMS', '1'), ('OW_USE_DAILY_SEED_FOR_VANILLA_VARIABLES', '1'), ('P_ASK_MOVE_CONFIRMATION', '1'),
    ('OW_UNION_DISABLE_CHECK', '1'),
    # Mixed backlog batch 4
    ('BATTLE_PYRAMID_RANDOM_ENCOUNTERS', '1'), ('OW_PC_JAPAN_WALDA_ICONS', '1'), ('OW_PC_MOVE_ORDER', GENERATION),
    ('OW_TIMES_OF_DAY', GENERATION),
    # Battle-only forms' overworld sprites (always built; species entries marked overworldBattleOnly)
    ('OW_BATTLE_ONLY_FORMS', '1'),
    # Female sprites, palettes and icons (always built; readers test the options), and Shedinja's Poke Ball (code reads the
    # option; Nincada's evolution list takes the source value and is measured per value)
    ('P_GENDER_DIFFERENCES', '1'), ('P_CUSTOM_GENDER_DIFF_ICONS', '1'), ('P_SHEDINJA_BALL', GENERATION),
    # Dynamax message (the battle script asks the selection; upstream compared two memory addresses) and the Perish Song count
    # (set from the selection instead of a literal 3; its volatile field holds ADMG_VOLATILE_TIMER_MAX)
    ('B_SHOW_DYNAMAX_MESSAGE', '1'), ('B_PERISH_SONG_TIMER', '15'),
    # Escape Rope: using it reads the option; its item data (key item from Gen 8) takes the source value and is measured
    ('I_KEY_ESCAPE_ROPE', GENERATION),
    # Friendship evolutions: species data hold ADMG_FRIENDSHIP_EVO_THRESHOLD_CONFIGURED, the evolution check reads the option
    ('P_FRIENDSHIP_EVO_THRESHOLD', GENERATION),
    # Reusable TMs: the relearner and the FRLG tutor scripts ask the selection; TM rows take the source value and are measured
    ('I_REUSABLE_TMS', '1'),
    # Battle animation graphics (ADMG_GetBattleAnimation, ADMG_GetBattleAnimBackground and the Surf wave choose at run time)
    ('B_NEW_SWORD_PARTICLE', '1'), ('B_NEW_LEECH_SEED_PARTICLE', '1'), ('B_NEW_HORN_ATTACK_PARTICLE', '1'),
    ('B_NEW_ROCKS_PARTICLE', '1'), ('B_NEW_LEAF_PARTICLE', '1'), ('B_NEW_EMBER_PARTICLES', '1'),
    ('B_NEW_MEAN_LOOK_PARTICLE', '1'), ('B_NEW_TEETH_PARTICLE', '1'), ('B_NEW_HANDS_FEET_PARTICLE', '1'),
    ('B_NEW_SPIKES_PARTICLE', '1'), ('B_NEW_FLY_BUBBLE_PARTICLE', '1'), ('B_NEW_CURSE_NAIL_PARTICLE', '1'),
    ('B_NEW_BATON_PASS_BALL_PARTICLE', '1'), ('B_NEW_MORNING_SUN_STAR_PARTICLE', '1'), ('B_NEW_IMPACT_PALETTE', '1'),
    ('B_NEW_SURF_PARTICLE_PALETTE', '1'), ('B_NEW_TERRAIN_BACKGROUNDS', '1'),
    # Overworld substitute sprite, two-frame front sprites, followers and cries: code reads the selection (the follower checks and
    # the cry tables keep the source value)
    ('OW_SUBSTITUTE_PLACEHOLDER', '1'), ('P_TWO_FRAME_FRONT_SPRITES', '1'), ('OW_FOLLOWERS_ENABLED', '1'), ('P_CRIES_ENABLED', '1'),
    # Overworld Pokémon graphics, footprints, follower Poké Balls, large overworld sprites: code reads the selection; data and
    # declarations keep the source value (everything stays built)
    ('OW_POKEMON_OBJECT_EVENTS', '1'), ('P_FOOTPRINTS', '1'), ('OW_FOLLOWERS_POKEBALLS', '1'), ('OW_LARGE_OW_SUPPORT', '1'),
    # GBA-style species graphics: battle code reads the selection; species data (both sets built) are measured per value
    ('P_GBA_STYLE_SPECIES_GFX', '1'),
    # Mega Evolution cries: sound.c reads the selection; the cries are always built and Mega forms' cryId is measured per value
    ('P_MODIFIED_MEGA_CRIES', '1'),
    # Shared overworld palettes: the palette loader reads the selection; the overworld palettes stay built
    ('OW_PKMN_OBJECTS_SHARE_PALETTES', '1'),
    # Low-HP beeps: the song is built once per count 1..16 and endless (-1, NUM_BEEPS_GEN_3, stored as 0xFFFFFFFF), and
    # ADMG_PlayLowHealthBeep plays the selected one (0: none; above 16: 16)
    ('B_NUM_LOW_HEALTH_BEEPS', '0xFFFFFFFF'),
    # Battle text pauses: B_WAIT_TIME_* assemble as marked units that waitmessage, pause and Wally's controller multiply by the
    # selection (ADMG_BattleWaitFrames; patch 0094)
    ('B_WAIT_TIME_MULTIPLIER', '255'),
]
HEADER = 'include/constants/admg_config_extra.h'
FIRST_TAG = 0x1000


def header():
    lines = ['/* Generated by tools/admg101/route_r38_config_extra.py',
             ' * Route these directly read include/config options through ADMG_GetConfig',
             ' * F(option, declared maximum) */',
             '#ifndef GUARD_CONSTANTS_ADMG_CONFIG_EXTRA_H', '#define GUARD_CONSTANTS_ADMG_CONFIG_EXTRA_H', '',
             '#define ADMG_CONFIG_EXTRA_DEFINITIONS(F) \\']
    lines += [f'    F({name}, {maximum}) \\' for name, maximum in OPTIONS]
    lines += ['', '#define ADMG_CONFIG_EXTRA_TAG(_name, _maximum) CONFIG_##_name,', '',
              '// Extra CONFIG_* tags start at 0x1000',
              'enum ADMG_ConfigExtraTag', '{', f'    ADMG_CONFIG_EXTRA_FIRST = 0x{FIRST_TAG - 1:X},',
              '    ADMG_CONFIG_EXTRA_DEFINITIONS(ADMG_CONFIG_EXTRA_TAG)', '    ADMG_CONFIG_EXTRA_END', '};', '',
              '#undef ADMG_CONFIG_EXTRA_TAG', '',
              '// Named option maxima for build-time checks that storage can hold the full selection range']
    lines += [f'#define ADMG_CONFIG_EXTRA_MAX_{name} ({maximum})' for name, maximum in OPTIONS]
    lines += ['', "// Battle and event scripts need numeric tags because the assembler can't read the enum",
              '// config_changes.c checks each value against its CONFIG_<name> tag']
    lines += [f'#define ADMG_CONFIG_TAG_{name} 0x{FIRST_TAG + index:X}' for index, (name, _) in enumerate(OPTIONS)]
    lines += ['', '#endif', '']
    return '\n'.join(lines)


def route(source, write):
    """Rewrite each option's definition; returns the files that are (or would be) changed."""
    changed = []
    wanted = {name for name, _ in OPTIONS}
    found = set()
    for path in sorted((source/'include/config').glob('*.h')):
        text = path.read_text(encoding='utf-8')
        new_lines = []
        for line in text.split('\n'):
            match = re.match(r'#define\s+(\w+)(\s+)(.*)$', line)
            if match and match.group(1) in wanted and match.group(3).startswith('ADMG_CONFIG_EXTRA('):
                found.add(match.group(1))  # already routed
                new_lines.append(line)
            elif match and match.group(1) in wanted:
                name = match.group(1)
                found.add(name)
                new_lines.append(f'#define ADMG_DEFAULT_{name}{match.group(2)}{match.group(3)}')
                new_lines.append(f'#define {name} ADMG_CONFIG_EXTRA({name})')
            else:
                new_lines.append(line)
        new = '\n'.join(new_lines)
        if new != text:
            changed.append(path.relative_to(source).as_posix())
            if write:
                path.write_text(new, encoding='utf-8', newline='\n')
    missing = wanted - found
    already = {name for name in wanted - found
               if re.search(r'#define\s+' + name + r'\s+ADMG_CONFIG_EXTRA', ''.join(
                   p.read_text(encoding='utf-8') for p in (source/'include/config').glob('*.h')))}
    if missing - already:
        raise SystemExit('options with no include/config definition: ' + ', '.join(sorted(missing - already)))
    target = source/HEADER
    current = target.read_text(encoding='utf-8') if target.exists() else ''
    if current != header():
        changed.append(HEADER)
        if write:
            target.write_text(header(), encoding='utf-8', newline='\n')
    return changed


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('source', type=Path)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    changed = route(args.source.resolve(), write=not args.check)
    print(('stale: ' if args.check else 'written: ') + (', '.join(changed) if changed else 'nothing'))
    return 1 if args.check and changed else 0


if __name__ == '__main__':
    sys.exit(main())
