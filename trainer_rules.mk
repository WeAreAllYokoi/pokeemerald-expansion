# party files are run through trainerproc, which is a tool that converts party data to an output file
# matching the current trainer .h formatting

AUTO_GEN_TARGETS += src/data/trainers.h
AUTO_GEN_TARGETS += src/data/trainers_frlg.h
AUTO_GEN_TARGETS += src/data/battle_partners.h
AUTO_GEN_TARGETS += test/battle/trainer_control.h
AUTO_GEN_TARGETS += test/battle/partner_control.h
AUTO_GEN_TARGETS += src/data/debug_trainers.h

%.h: %.party $(TRAINERPROC)
	$(CPP) $(CPPFLAGS) -traditional-cpp - < $< | $(TRAINERPROC) -o $@ -i $< -

# Only native array designators use native indices. Copy Pool remains a reference.
src/data/trainers_frlg.h: src/data/trainers_frlg.party $(TRAINERPROC) include/constants/admg_trainer_ids.h
	$(CPP) $(CPPFLAGS) -traditional-cpp - < $< | $(TRAINERPROC) -p ADMG_KANTO_INDEX_ -o $@ -i $< -
