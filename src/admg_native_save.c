#include "global.h"
#include "save.h"
#include "admg_native_save.h"

// Default hooks: writes fail, reads report an empty save, and erase does nothing

__attribute__((noinline)) u8 ADMG_NativeSaveWrite(const struct ADMG_SaveRegion *regions, u32 count, u32 mode)
{
    return SAVE_STATUS_ERROR;
}

__attribute__((noinline)) u8 ADMG_NativeSaveRead(const struct ADMG_SaveRegion *regions, u32 count)
{
    return SAVE_STATUS_EMPTY;
}

__attribute__((noinline)) void ADMG_NativeSaveErase(void)
{
}
