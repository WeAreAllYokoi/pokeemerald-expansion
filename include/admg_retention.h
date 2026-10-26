#ifndef ADMG_RETENTION_H
#define ADMG_RETENTION_H

/* These passive ROM roots do not call functions, select alternatives, or spawn
 * gameplay behavior. The linker must KEEP .admg.exports* as well as the compiler
 * preserving the roots. Each root keeps the symbol's actual pointer type. */
#define ADMG_ROOT_ATTRIBUTES __attribute__((used, section(".admg.exports"), aligned(4)))
#define ADMG_RETAIN_FUNCTION(symbol) \
    static __typeof__(&(symbol)) const admg_root_##symbol ADMG_ROOT_ATTRIBUTES = &(symbol)
#define ADMG_RETAIN_DATA(symbol) \
    static __typeof__(&(symbol)) const admg_root_##symbol ADMG_ROOT_ATTRIBUTES = &(symbol)

#endif
