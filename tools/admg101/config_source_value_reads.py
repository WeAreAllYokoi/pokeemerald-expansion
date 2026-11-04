"""Reads of an option's compiled (source) value that do not change what a selection does, each with the accessor or
reason that makes it harmless. The inventory (inventory_r38_config.py) counts them as following the selection, and the
variant extractor does not count them as reads outside measured data. Kept apart from the extractor so editing this list
does not make a saved measurement stale: the inventory gate checks every compiled read against it either way."""

SOURCE_VALUE_READS = {
    # Overworld graphics options (patch 0076): data and declarations keep the source value so everything stays built; code reads the selection
    ('include/pokemon.h', 'ADMG_DEFAULT_OW_POKEMON_OBJECT_EVENTS'): 'SpeciesInfo keeps its overworld fields',
    ('src/data/graphics/pokemon.h', 'ADMG_DEFAULT_OW_POKEMON_OBJECT_EVENTS'): 'overworld graphics stay built',
    ('src/data/object_events/object_event_pic_tables_followers.h', 'ADMG_DEFAULT_OW_POKEMON_OBJECT_EVENTS'): 'follower frame tables stay built',
    ('src/data/pokemon/species_info.h', 'ADMG_DEFAULT_OW_POKEMON_OBJECT_EVENTS'): 'species overworld data stay built',
    ('src/event_object_movement.c', 'ADMG_DEFAULT_OW_POKEMON_OBJECT_EVENTS'): 'guards code that reads the fields; SpeciesToGraphicsInfo and the palette loader read the selection',
    ('include/pokemon.h', 'ADMG_DEFAULT_P_FOOTPRINTS'): 'SpeciesInfo keeps its footprint field',
    ('src/data/graphics/pokemon.h', 'ADMG_DEFAULT_P_FOOTPRINTS'): 'footprints stay built',
    ('src/data/pokemon/species_info.h', 'ADMG_DEFAULT_P_FOOTPRINTS'): 'species footprint fields stay set',
    ('src/pokedex.c', 'ADMG_DEFAULT_P_FOOTPRINTS'): 'guards the field read; DrawFootprint reads the selection',
    ('include/constants/event_objects.h', 'ADMG_DEFAULT_OW_FOLLOWERS_POKEBALLS'): 'ball graphics and palette tags stay defined',
    ('src/data/object_events/object_event_graphics.h', 'ADMG_DEFAULT_OW_FOLLOWERS_POKEBALLS'): 'ball graphics stay built',
    ('src/data/object_events/object_event_graphics_info_followers.h', 'ADMG_DEFAULT_OW_FOLLOWERS_POKEBALLS'): 'ball graphics info stays built',
    ('src/data/object_events/object_event_pic_tables.h', 'ADMG_DEFAULT_OW_FOLLOWERS_POKEBALLS'): 'ball frame tables stay built',
    ('src/event_object_movement.c', 'ADMG_DEFAULT_OW_FOLLOWERS_POKEBALLS'): 'ball palettes stay listed; ObjectEventSetPokeballGfx reads the selection',
    ('src/data/object_events/object_event_subsprites.h', 'ADMG_DEFAULT_OW_LARGE_OW_SUPPORT'): 'large subsprite tables stay built; the sprite code reads the selection',
    ('src/data/graphics/pokemon.h', 'ADMG_DEFAULT_OW_PKMN_OBJECTS_SHARE_PALETTES'): 'overworld palettes stay built',
    ('src/data/pokemon/species_info.h', 'ADMG_DEFAULT_OW_PKMN_OBJECTS_SHARE_PALETTES'): 'species overworld palette fields stay set',
    ('include/pokemon.h', 'ADMG_DEFAULT_OW_PKMN_OBJECTS_SHARE_PALETTES'): 'SpeciesInfo keeps its overworld palette fields',
    ('src/event_object_movement.c', 'ADMG_DEFAULT_OW_PKMN_OBJECTS_SHARE_PALETTES'): 'guards the palette field reads; the loader reads the selection',
    ('src/pokemon_sprite_visualizer.c', 'ADMG_DEFAULT_B_ENEMY_MON_SHADOW_STYLE'): 'debug sprite visualizer labels',
    ('src/pokemon_sprite_visualizer.c', 'ADMG_DEFAULT_P_GBA_STYLE_SPECIES_GFX'): 'debug sprite visualizer labels',
    ('tools/mid2agb/midi.cpp', 'ADMG_DEFAULT_B_NUM_LOW_HEALTH_BEEPS'): 'the stock low-HP song (only the debug sound list plays it); ADMG_PlayLowHealthBeep plays the selected variant',
    ('src/item_use.c', 'ADMG_DEFAULT_I_EXP_SHARE_ITEM'): 'build-time check that a Gen 6 Exp. Share has a flag',
    # Defaults that a run-time accessor replaces: the compiled value is only the unselected answer
    ('src/battle_message.c', 'ADMG_DEFAULT_B_SHOW_EFFECTIVENESS'): 'PP window colours; BattlePutTextOnWindow sets them from the selection',
    ('src/field_move.c', 'ADMG_DEFAULT_OW_ROCK_CLIMB_FIELD_MOVE'): 'unlock type; FieldMove_UnlockType reads the selection',
    ('src/field_move.c', 'ADMG_DEFAULT_OW_DEFOG_FIELD_MOVE'): 'unlock type; FieldMove_UnlockType reads the selection',
    ('src/pokemon.c', 'ADMG_DEFAULT_HGSS_SORT_TMS_BY_NUM'): 'the source learnset is the source variant; others come from sTeachableLearnsetVariants',
    ('src/pokemon.c', 'ADMG_DEFAULT_P_TM_LITERACY'): 'the source learnset is the source variant; others come from sTeachableLearnsetVariants',
    ('tools/learnset_helpers/make_teachables.py', 'HGSS_SORT_TMS_BY_NUM'): 'builds every TM-order and literacy variant',
    ('tools/learnset_helpers/make_teachables.py', 'P_TM_LITERACY'): 'builds every TM-order and literacy variant',
    ('src/berry.c', 'OW_BERRY_GROWTH_RATE'): "default field; ADMG_GetBerryGrowthHours reads the preset arrays",
    ('src/berry.c', 'OW_BERRY_YIELD_RATE'): 'default fields; the yield accessors read the preset arrays',
    ('src/berry.c', 'OW_BERRY_COLORS'): 'default field; the colour accessor reads colorXY/colorORAS',
    ('src/data/items.h', 'I_VS_SEEKER_CHARGING'): "Vs. Seeker field function; GetItemFieldFunc reads the binding",
    ('src/map_name_popup.c', 'ADMG_DEFAULT_OW_POPUP_GENERATION'): 'test builds only (!TESTING is always true in the game)',
    # The generator places the untimed JSON tables at the source fallback time; the shipped JSON has no timed tables,
    # so its OW_TIME_OF_DAY_ENCOUNTERS filter drops nothing. The run-time fallback and policies choose among the slots.
    ('tools/wild_encounters/wild_encounters_to_header.py', 'OW_TIME_OF_DAY_ENCOUNTERS'): 'encounter table placement',
    ('tools/wild_encounters/wild_encounters_to_header.py', 'OW_TIME_OF_DAY_DISABLE_FALLBACK'): 'encounter table placement',
    ('tools/wild_encounters/wild_encounters_to_header.py', 'OW_TIME_OF_DAY_FALLBACK'): 'encounter table placement',
    # SaveBlock3 keeps the source layout; these fields are unused because the features keep their state in ADMG feature state
    ('include/global.h', 'OW_USE_FAKE_RTC'): 'save layout; fake RTC state is ADMG feature state',
    ('include/global.h', 'FNPC_ENABLE_NPC_FOLLOWERS'): 'save layout; NPC follower state is ADMG feature state',
    ('include/global.h', 'OW_SHOW_ITEM_DESCRIPTIONS'): 'save layout; item-seen flags are ADMG feature state',
    ('include/global.h', 'USE_DEXNAV_SEARCH_LEVELS'): 'save layout; search levels are ADMG feature state',
}
