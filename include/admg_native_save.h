#ifndef GUARD_ADMG_NATIVE_SAVE_H
#define GUARD_ADMG_NATIVE_SAVE_H

// Native save hooks
//
// Each region gives a section ID, address and size
// The platform must replace these hooks and commit each write as one transaction, together with its own
// sections for registry identities, extended state and mod data
// The default hooks provide no storage

enum ADMG_SaveSection
{
    ADMG_SAVE_SECTION_SAVEBLOCK2 = 1,      // Player profile, options, Pokédex, Battle Frontier
    ADMG_SAVE_SECTION_SAVEBLOCK1 = 2,      // World state, party copy, bag, flags and variables
    ADMG_SAVE_SECTION_SAVEBLOCK3 = 3,      // Expansion's additional block
    ADMG_SAVE_SECTION_POKEMON_STORAGE = 4, // PC boxes
    ADMG_SAVE_SECTION_HALL_OF_FAME = 5,
    ADMG_SAVE_SECTION_TRAINER_HILL = 6,
    ADMG_SAVE_SECTION_RECORDED_BATTLE = 7,
    ADMG_SAVE_SECTION_EXTENDED_TRAINER_DEFEATS = 8, // admg_extended.h: bytes of a bit store, as many as it has
    ADMG_SAVE_SECTION_EXTENDED_DEX_SEEN = 9,
    ADMG_SAVE_SECTION_EXTENDED_DEX_CAUGHT = 10,
};

// The most regions one save hands over: the save slot, the Hall of Fame and every extended store.
#define ADMG_SAVE_MAX_REGIONS 8

struct ADMG_SaveRegion
{
    u32 section; // enum ADMG_SaveSection
    void *data;
    u32 size;
};

// How a write treats the sections it does not list.
enum ADMG_SaveWriteMode
{
    ADMG_SAVE_KEEP_OTHER_SECTIONS,   // Replace the listed sections; keep the others as committed
    ADMG_SAVE_ERASE_HALL_OF_FAME,    // As above, and drop the Hall of Fame (a different file)
};

// Commits the listed regions in one transaction. Returns SAVE_STATUS_OK or SAVE_STATUS_ERROR.
u8 ADMG_NativeSaveWrite(const struct ADMG_SaveRegion *regions, u32 count, u32 mode);
// Fills the listed regions from the last commit. SAVE_STATUS_OK, SAVE_STATUS_EMPTY (nothing saved yet),
// or SAVE_STATUS_CORRUPT / SAVE_STATUS_ERROR.
u8 ADMG_NativeSaveRead(const struct ADMG_SaveRegion *regions, u32 count);
// Erase game and mod sections while retaining platform-owned sections
void ADMG_NativeSaveErase(void);

#endif // GUARD_ADMG_NATIVE_SAVE_H
