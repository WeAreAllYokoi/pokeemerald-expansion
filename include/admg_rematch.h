#ifndef ADMG_REMATCH_H
#define ADMG_REMATCH_H

#include "admg_native.h"

#define ADMG_NATIVE_REMATCH_FAMILIES 221
#define ADMG_NATIVE_REMATCH_SLOTS 6
#define ADMG_NATIVE_READINESS_COUNT 100

/* Family identity never depends on current map, region or eligibility.
 * The original eligibility provider and explicit selection are separate.
 * A caller may supply every stage bit without imposing campaign predicates. */
extern const u16 gADMG_KantoRematchFamilies[ADMG_NATIVE_REMATCH_FAMILIES][ADMG_NATIVE_REMATCH_SLOTS];
ADMG_NATIVE_ENTRY bool8 ADMG_TryFindNativeRematch(u32 trainerId, bool8 baseOnly, u32 *family, u8 *position);
ADMG_NATIVE_ENTRY bool8 ADMG_TryGetNativeRematchProgress(u32 baseTrainerId, u32 *family, u8 *position);
ADMG_NATIVE_ENTRY bool8 ADMG_TrySelectNativeRematch(u32 baseTrainerId, u32 permittedStages, u16 *trainerId);
ADMG_NATIVE_ENTRY bool8 ADMG_TryGetNativeRematchTrainer(u32 baseTrainerId, u16 *trainerId);

struct ObjectEventTemplate;
/* General declaration/clone lookup also retains valid scriptless scenery.
 * Trainer-specific interfaces below keep their non-null script contract. */
ADMG_NATIVE_ENTRY bool8 ADMG_TryGetDeclaredObjectTemplate(MapId mapId, u32 localId,
                                                      const struct ObjectEventTemplate **template);
/* Resolve the declared NPC through its saved map/local-ID binding, including
 * clone provenance. Map binding scopes interaction; the trainer operand in the
 * resolved script remains the independent source identity. No state is written. */
ADMG_NATIVE_ENTRY bool8 ADMG_TryGetRematchNpcTemplate(MapId mapId, u32 localId,
                                                  const struct ObjectEventTemplate **template);
/* Header-backed source identity, independent of stale serialized script
 * pointers. Saved clones contribute provenance; ordinary NPCs use declarations. */
ADMG_NATIVE_ENTRY bool8 ADMG_TryGetDeclaredRematchNpcTemplate(MapId mapId, u32 localId,
                                                          const struct ObjectEventTemplate **template);
ADMG_NATIVE_ENTRY bool8 ADMG_TryGetNativeRematchNpcTrainer(MapId mapId, u32 localId,
                                                       enum TrainerId *trainerId);

#endif
