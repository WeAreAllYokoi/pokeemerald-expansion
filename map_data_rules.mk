# Map JSON data

# Inputs
MAPS_DIR = $(DATA_ASM_SUBDIR)/maps
LAYOUTS_DIR = $(DATA_ASM_SUBDIR)/layouts

# Retain all passive regional content; MAP_VERSION still determines header format.
ADMG_STATE_CATALOG := tools/admg101/state-operands.json
ADMG_MAP_OPTIONS := --content-regions=all --map-count-output=$(DATA_SRC_SUBDIR)/map_group_count.h --heal-output=$(DATA_SRC_SUBDIR)/heal_locations.admg.json

# Outputs
MAPS_OUTDIR := $(MAPS_DIR)
LAYOUTS_OUTDIR := $(LAYOUTS_DIR)
INCLUDECONSTS_OUTDIR := include/constants

AUTO_GEN_TARGETS += $(INCLUDECONSTS_OUTDIR)/map_groups.h
AUTO_GEN_TARGETS += $(INCLUDECONSTS_OUTDIR)/layouts.h
AUTO_GEN_TARGETS += $(INCLUDECONSTS_OUTDIR)/map_event_ids.h
AUTO_GEN_TARGETS += $(DATA_SRC_SUBDIR)/map_group_count.h

MAP_DIRS := $(dir $(wildcard $(MAPS_DIR)/*/map.json))
MAP_CONNECTIONS := $(patsubst $(MAPS_DIR)/%/,$(MAPS_DIR)/%/connections.inc,$(MAP_DIRS))
MAP_EVENTS := $(patsubst $(MAPS_DIR)/%/,$(MAPS_DIR)/%/events.inc,$(MAP_DIRS))
MAP_HEADERS := $(patsubst $(MAPS_DIR)/%/,$(MAPS_DIR)/%/header.inc,$(MAP_DIRS))
MAP_JSONS := $(patsubst $(MAPS_DIR)/%/,$(MAPS_DIR)/%/map.json,$(MAP_DIRS))

$(DATA_ASM_BUILDDIR)/maps.o: $(DATA_ASM_SUBDIR)/maps.s $(LAYOUTS_DIR)/layouts.inc $(LAYOUTS_DIR)/layouts_table.inc $(MAPS_DIR)/headers.inc $(MAPS_DIR)/groups.inc $(MAPS_DIR)/connections.inc $(MAP_CONNECTIONS) $(MAP_HEADERS)
	$(PREPROC) -s $< charmap.txt | $(PYTHON) tools/admg101/rewrite_state_operands.py --source $< | $(CPP) $(CPPFLAGS) -I include - | $(PREPROC) -ie $< charmap.txt | $(AS) $(ASFLAGS) -o $@
$(DATA_ASM_BUILDDIR)/map_events.o: $(DATA_ASM_SUBDIR)/map_events.s $(MAPS_DIR)/events.inc $(MAP_EVENTS)
	$(PREPROC) -s $< charmap.txt | $(PYTHON) tools/admg101/rewrite_state_operands.py --source $< | $(CPP) $(CPPFLAGS) -I include - | $(PREPROC) -ie $< charmap.txt | $(AS) $(ASFLAGS) -o $@

$(MAPS_OUTDIR)/%/header.inc $(MAPS_OUTDIR)/%/events.inc $(MAPS_OUTDIR)/%/connections.inc: $(MAPS_DIR)/%/map.json $(INCLUDECONSTS_OUTDIR)/map_groups.h $(ADMG_STATE_CATALOG) $(MAPJSON)
	$(MAPJSON) map emerald $< $(LAYOUTS_DIR)/layouts.json $(@D) --state-catalog=$(ADMG_STATE_CATALOG)


$(MAPS_OUTDIR)/connections.inc $(MAPS_OUTDIR)/groups.inc $(MAPS_OUTDIR)/events.inc $(MAPS_OUTDIR)/headers.inc $(INCLUDECONSTS_OUTDIR)/map_groups.h $(DATA_SRC_SUBDIR)/map_group_count.h $(DATA_SRC_SUBDIR)/heal_locations.admg.json &: $(MAPS_DIR)/map_groups.json $(MAP_JSONS) $(DATA_SRC_SUBDIR)/heal_locations.json .map_version $(MAPJSON)
	@$(MAPJSON) groups $(MAP_VERSION) $(ADMG_MAP_OPTIONS) $(MAPS_DIR)/map_groups.json $(MAP_JSONS) $(MAPS_OUTDIR) $(INCLUDECONSTS_OUTDIR)
	@echo "$(MAPJSON) groups $(MAP_VERSION) $(ADMG_MAP_OPTIONS) $(MAPS_DIR)/map_groups.json <MAP_JSONS> $(MAPS_OUTDIR) $(INCLUDECONSTS_OUTDIR)"

$(LAYOUTS_OUTDIR)/layouts.inc $(LAYOUTS_OUTDIR)/layouts_table.inc $(INCLUDECONSTS_OUTDIR)/layouts.h: $(LAYOUTS_DIR)/layouts.json .map_version $(MAPJSON)
	$(MAPJSON) layouts $(MAP_VERSION) --content-regions=all $< $(LAYOUTS_OUTDIR) $(INCLUDECONSTS_OUTDIR)

# Generate constants for map events, which depend on data that's distributed across the map.json files.
# There's a lot of map.json files, so we print an abbreviated output with echo.
$(INCLUDECONSTS_OUTDIR)/map_event_ids.h: $(MAP_JSONS) $(MAPJSON)
	@$(MAPJSON) event_constants emerald $(MAP_JSONS) $(INCLUDECONSTS_OUTDIR)/map_event_ids.h
	@echo "$(MAPJSON) event_constants emerald <MAP_JSONS> $(INCLUDECONSTS_OUTDIR)/map_event_ids.h"

.map_version : FORCE
	@(echo "$(MAP_VERSION) all recipe-2" | cmp $@ -) || echo "$(MAP_VERSION) all recipe-2" > .map_version

FORCE:
.PHONY : FORCE
