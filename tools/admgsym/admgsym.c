/*
 * admgsym: writes an ADMGSYM1 symbol table, the ROM functions and data objects of a GBA game's ELF and the ranges in
 * which they are complete, for the GBA loader of ADMG-101's Ghidra (gba-ghidra-loader) to read.
 *
 *   admgsym embed [--not-covered <symbol>]... <elf> <rom>                  write the table into the ROM's 0xFF padding
 *   admgsym sidecar [--not-covered <symbol>]... <elf> <rom> <out.admgsym>  write the table to its own file
 *
 * The format, little-endian throughout:
 *
 *   Header, 32 bytes:
 *     0   8  magic "ADMGSYM1"
 *     8   2  format version, 2
 *     10  2  flags, 0
 *     12  4  entry count
 *     16  4  string table length
 *     20  4  CRC-32 of the ROM: embedded, the file from 0 up to the table; a sidecar, the whole ROM file
 *     24  4  CRC-32 of the header and the stored payload, with this field read as 0
 *     28  4  reserved, 0
 *   Payload: a zlib stream (RFC 1950) holding the entries, then the string table.
 *   Entry, 12 bytes, sorted by address, then kind, then name, then size:
 *     0   4  address, 0x08000000-0x09FFFFFF; for a function, bit 0 set marks Thumb
 *     4   4  size in bytes, 0 if unknown; a coverage range's is never 0
 *     8   1  kind: 0 function, 1 data object, 2 coverage range
 *     9   3  offset of the name in the string table
 *   String table: NUL-terminated names of printable ASCII (0x21-0x7E), each stored once.
 *   Footer, embedded only, the last 16 bytes of the ROM:
 *     0   8  magic "ADMGSYMF"
 *     8   4  table offset in the file
 *     12  4  table length in bytes (header and payload)
 *
 * A coverage range says that inside it every function start is one of the table's functions, and every byte no
 * function covers is data or the cartridge header. A call into the middle of a function with a size (a second entry)
 * isn't a function start. Its name is the ELF section it lies in. A table with no coverage ranges claims nothing about
 * completeness. The ranges are worked out from the ELF alone:
 *   - The cartridge header (0x08000000-0x080000BF), its branch to the cartridge entry included, is covered.
 *   - In an executable section, a byte is covered when the ELF's mapping symbol in force there is $d, or is $a or $t and
 *     a function covers the byte. A function covers its size, or up to the next function start when it has none, never
 *     past the end of its section. Code in no function, and bytes with no mapping symbol in force, are not covered,
 *     except a stretch of them that holds only padding values: 0xFF bytes, or in $t Thumb halfwords 0x0000 and 0x46C0
 *     (nop), or in $a ARM words 0x00000000 and 0xE1A00000 (nop).
 *   - A non-executable section is covered.
 *   - Embedded programs are never covered: a multiboot image (from the 4 bytes before the Nintendo logo in its header
 *     up to the next symbol that isn't a local label, or the next function start, whichever comes first) and each
 *     symbol given with --not-covered (its size, or up to <symbol>_end).
 *   - Sections the ELF links at another address (code or data copied to RAM) are not in the ROM window, so they are
 *     never covered.
 *
 * Besides the ELF's function symbols, the table has a function at each piece of code that a BL in the ELF's code calls
 * and no function symbol starts or contains, in an executable section where the ELF doesn't mark it $d:
 *   - a Thumb `bx Rm` that a Thumb BL calls, named _call_via_<register>_<address>, size 2. GCC emits these at the end
 *     of a Thumb compilation unit for calls through a register on ARMv4T, under internal .L labels the ELF doesn't keep.
 *   - code with an STT_NOTYPE label, such as crt0's InitializeWorkingMemory, under its own name, sized up to the next
 *     $d, function start, BL target or end of its section.
 *   - the cartridge entry, the target of the header's ARM branch, when no function symbol starts there: sized the same
 *     way, under its STT_NOTYPE label, or as cartridge_entry when it has none.
 * A BL target inside a function with a size is a second entry into that function and gets nothing. These are close to
 * the classes ADMG-101's score.py (called_code) counts as call thunks and code labels; score.py also counts a `bx Rm`
 * that only an ARM BL reaches.
 *
 * The table starts at the first 16-byte boundary after the game's bytes: the end of the ELF's last segment loaded into
 * ROM. Embedding fails, and writes nothing, unless every byte from there to the end of the ROM is 0xFF and the table
 * and footer fit in it. Both modes first check that the ROM holds the ELF's ROM segments byte for byte after the
 * 0xC0-byte cartridge header, which builds often fill in on the ROM after linking (gbafix).
 *
 * The payload is compressed by miniz (miniz.c, miniz.h; MIT licence in LICENSE.miniz), built into this tool, so the
 * output is the same on every machine and needs no system zlib.
 */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "elf.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include "miniz.h"

#define ROM_START 0x08000000u
#define ROM_END 0x0A000000u
#define HEADER_SIZE 32u
#define FOOTER_SIZE 16u
#define CARTRIDGE_HEADER 0xC0u
#define MAX_STRINGS 0x01000000u /* a name offset is 3 bytes */
#define FORMAT_VERSION 2

enum
{
    KIND_FUNCTION = 0,
    KIND_DATA = 1,
    KIND_COVERAGE = 2,
};

/* An entry of the table. name points into the ELF, or into memory this tool allocated. */
struct Symbol
{
    uint32_t address;
    uint32_t size;
    uint8_t kind;
    const char *name;
    uint32_t nameOffset;
};

struct Symbols
{
    struct Symbol *items;
    size_t count, capacity;
};

/* A symbol from the ELF's symbol table that lies in the ROM window. */
struct RawSymbol
{
    uint32_t value;
    uint32_t size;
    unsigned type;
    const char *name;
};

/* A section with bytes in the ROM window, at its link address. */
struct Section
{
    uint32_t start, end;
    int executable;
    const char *name;
};

/* A mapping symbol: 'a' ARM code, 't' Thumb code, 'd' data from here on. */
struct Mark
{
    uint32_t address;
    char kind;
};

/* A function's start and the end of what it covers. */
struct Span
{
    uint32_t start, end;
};

struct Range
{
    uint32_t start, end;
};

struct Ranges
{
    struct Range *items;
    size_t count, capacity;
};

static void fail(const char *format, ...) __attribute__((noreturn, format(printf, 1, 2)));

static void fail(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fputs("admgsym: ", stderr);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);
    exit(1);
}

static void *allocate(size_t size)
{
    void *p = malloc(size ? size : 1);
    if (p == NULL)
        fail("out of memory");
    return p;
}

/* Makes room for one more item in an array of count items, doubling capacity when it's full. */
static void *grow(void *items, size_t count, size_t *capacity, size_t itemSize)
{
    if (count < *capacity)
        return items;
    *capacity = *capacity ? *capacity * 2 : 64;
    void *grown = realloc(items, *capacity * itemSize);
    if (grown == NULL)
        fail("out of memory");
    return grown;
}

static void addSymbol(struct Symbols *list, uint32_t address, uint32_t size, uint8_t kind, const char *name)
{
    list->items = grow(list->items, list->count, &list->capacity, sizeof(*list->items));
    list->items[list->count++] = (struct Symbol){ address, size, kind, name, 0 };
}

static void addRange(struct Ranges *list, uint32_t start, uint32_t end)
{
    list->items = grow(list->items, list->count, &list->capacity, sizeof(*list->items));
    list->items[list->count++] = (struct Range){ start, end };
}

static uint8_t *readFile(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        fail("cannot open %s", path);
    if (fseek(f, 0, SEEK_END) != 0)
        fail("cannot read %s", path);
    long length = ftell(f);
    if (length < 0 || fseek(f, 0, SEEK_SET) != 0)
        fail("cannot read %s", path);
    uint8_t *data = allocate((size_t)length);
    if (fread(data, 1, (size_t)length, f) != (size_t)length)
        fail("cannot read %s", path);
    fclose(f);
    *size = (size_t)length;
    return data;
}

/* Writes to <path>.tmp and renames it over path, so a failed write never leaves path partial. */
static void writeFile(const char *path, const uint8_t *data, size_t size)
{
    size_t length = strlen(path);
    char *temporary = allocate(length + 5);
    memcpy(temporary, path, length);
    memcpy(temporary + length, ".tmp", 5);
    FILE *f = fopen(temporary, "wb");
    if (f == NULL)
        fail("cannot write %s", temporary);
    int written = fwrite(data, 1, size, f) == size;
    if (fclose(f) != 0 || !written)
    {
        remove(temporary);
        fail("cannot write %s", temporary);
    }
#ifdef _WIN32
    /* Windows' rename doesn't replace an existing file; MoveFileEx does, and leaves path as it was if it fails. */
    if (!MoveFileExA(temporary, path, MOVEFILE_REPLACE_EXISTING))
#else
    if (rename(temporary, path) != 0)
#endif
    {
        remove(temporary);
        fail("cannot replace %s", path);
    }
    free(temporary);
}

static void put16(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void put32(uint8_t *p, uint32_t v)
{
    put16(p, v & 0xFFFF);
    put16(p + 2, v >> 16);
}

static uint32_t crc32Of(const uint8_t *data, size_t size)
{
    return (uint32_t)mz_crc32(MZ_CRC32_INIT, data, size);
}

/* The ELF's parts this tool reads, each checked to lie inside the file. */
struct Elf
{
    const uint8_t *data;
    size_t size;
    const Elf32_Ehdr *header;
};

static const void *elfRange(const struct Elf *elf, uint64_t offset, uint64_t size, const char *what)
{
    if (offset > elf->size || size > elf->size - offset)
        fail("the ELF's %s lies outside the file", what);
    return elf->data + offset;
}

static struct Elf openElf(const char *path)
{
    struct Elf elf;
    elf.data = readFile(path, &elf.size);
    if (elf.size < sizeof(Elf32_Ehdr))
        fail("%s is not an ELF file", path);
    elf.header = (const Elf32_Ehdr *)elf.data;
    const unsigned char *id = elf.header->e_ident;
    if (memcmp(id, ELFMAG, SELFMAG) != 0)
        fail("%s is not an ELF file", path);
    if (id[EI_CLASS] != ELFCLASS32 || id[EI_DATA] != ELFDATA2LSB || elf.header->e_machine != EM_ARM)
        fail("%s is not a 32-bit little-endian ARM ELF", path);
    return elf;
}

static const Elf32_Phdr *programHeader(const struct Elf *elf, unsigned i)
{
    const Elf32_Ehdr *h = elf->header;
    if (h->e_phentsize != sizeof(Elf32_Phdr))
        fail("the ELF's program headers have an unexpected size");
    return elfRange(elf, (uint64_t)h->e_phoff + (uint64_t)i * sizeof(Elf32_Phdr), sizeof(Elf32_Phdr), "program headers");
}

static const Elf32_Shdr *sectionHeader(const struct Elf *elf, unsigned i)
{
    const Elf32_Ehdr *h = elf->header;
    if (h->e_shentsize != sizeof(Elf32_Shdr))
        fail("the ELF's section headers have an unexpected size");
    return elfRange(elf, (uint64_t)h->e_shoff + (uint64_t)i * sizeof(Elf32_Shdr), sizeof(Elf32_Shdr), "section headers");
}

/* A loaded segment with file bytes in the ROM window. */
static int isRomSegment(const Elf32_Phdr *p)
{
    return p->p_type == PT_LOAD && p->p_filesz != 0 && p->p_paddr >= ROM_START && p->p_paddr < ROM_END;
}

/*
 * Checks that the ROM holds every ROM segment of the ELF byte for byte after the cartridge header, and returns the
 * offset in the ROM where the game's bytes end: the end of the last such segment.
 */
static uint32_t checkRomMatchesElf(const struct Elf *elf, const uint8_t *rom, size_t romSize)
{
    uint32_t end = 0;
    for (unsigned i = 0; i < elf->header->e_phnum; i++)
    {
        const Elf32_Phdr *p = programHeader(elf, i);
        if (!isRomSegment(p))
            continue;
        uint32_t offset = p->p_paddr - ROM_START;
        if (p->p_filesz > ROM_END - p->p_paddr)
            fail("the ELF's segment at 0x%08X runs past the ROM window", p->p_paddr);
        if (offset > romSize || p->p_filesz > romSize - offset)
            fail("the ROM is smaller than the ELF's segment at 0x%08X", p->p_paddr);
        const uint8_t *bytes = elfRange(elf, p->p_offset, p->p_filesz, "segment");
        for (uint32_t j = offset < CARTRIDGE_HEADER ? CARTRIDGE_HEADER - offset : 0; j < p->p_filesz; j++)
        {
            if (rom[offset + j] != bytes[j])
                fail("the ROM differs from the ELF at 0x%08X, so it isn't the ELF's build", p->p_paddr + j);
        }
        if (offset + p->p_filesz > end)
            end = offset + p->p_filesz;
    }
    if (end == 0)
        fail("the ELF has no segment loaded into ROM");
    return end;
}

static int compareSymbols(const void *a, const void *b)
{
    const struct Symbol *x = a, *y = b;
    if (x->address != y->address)
        return x->address < y->address ? -1 : 1;
    if (x->kind != y->kind)
        return x->kind < y->kind ? -1 : 1;
    int byName = strcmp(x->name, y->name);
    if (byName != 0)
        return byName;
    if (x->size != y->size)
        return x->size < y->size ? -1 : 1;
    return 0;
}

static int compareU32(const void *a, const void *b)
{
    uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b;
    return x < y ? -1 : x > y;
}

static int compareMarks(const void *a, const void *b)
{
    const struct Mark *x = a, *y = b;
    return x->address < y->address ? -1 : x->address > y->address;
}

static int compareSpans(const void *a, const void *b)
{
    const struct Span *x = a, *y = b;
    return x->start < y->start ? -1 : x->start > y->start;
}

static int compareRanges(const void *a, const void *b)
{
    const struct Range *x = a, *y = b;
    return x->start < y->start ? -1 : x->start > y->start;
}

static void checkName(const char *name, uint32_t address)
{
    for (const char *c = name; *c; c++)
    {
        if (*c < 0x21 || *c > 0x7E)
            fail("the symbol %s at 0x%08X has a character outside printable ASCII", name, address);
    }
}

/* Every defined, named symbol of the ELF in the ROM window, in the order of the symbol table. */
static struct RawSymbol *readRawSymbols(const struct Elf *elf, size_t *count)
{
    size_t capacity = 0, n = 0;
    struct RawSymbol *out = NULL;
    for (unsigned s = 0; s < elf->header->e_shnum; s++)
    {
        const Elf32_Shdr *section = sectionHeader(elf, s);
        if (section->sh_type != SHT_SYMTAB)
            continue;
        if (section->sh_entsize != sizeof(Elf32_Sym))
            fail("the ELF's symbol table has an unexpected entry size");
        const Elf32_Shdr *strings = sectionHeader(elf, section->sh_link);
        const char *names = elfRange(elf, strings->sh_offset, strings->sh_size, "string table");
        const Elf32_Sym *table = elfRange(elf, section->sh_offset, section->sh_size, "symbol table");
        size_t entries = section->sh_size / sizeof(Elf32_Sym);
        for (size_t i = 0; i < entries; i++)
        {
            const Elf32_Sym *sym = &table[i];
            if (sym->st_shndx == SHN_UNDEF || sym->st_value < ROM_START || sym->st_value >= ROM_END)
                continue;
            if (sym->st_name >= strings->sh_size || memchr(names + sym->st_name, 0, strings->sh_size - sym->st_name) == NULL)
                fail("a symbol's name lies outside the ELF's string table");
            const char *name = names + sym->st_name;
            if (*name == '\0')
                continue;
            out = grow(out, n, &capacity, sizeof(*out));
            out[n++] = (struct RawSymbol){ sym->st_value, sym->st_size, ELF32_ST_TYPE(sym->st_info), name };
        }
    }
    *count = n;
    return out;
}

static int isFunctionType(unsigned type)
{
    return type == STT_FUNC || type == STT_ARM_TFUNC;
}

/* A mapping symbol's kind ('a', 't' or 'd'), or 0 for any other name: "$a", "$t", "$d", optionally followed by ".". */
static char markKind(const char *name)
{
    if (name[0] != '$' || (name[1] != 'a' && name[1] != 't' && name[1] != 'd'))
        return 0;
    return name[2] == '\0' || name[2] == '.' ? name[1] : 0;
}

/* score.py's LOCAL_LABEL: _XXXXXXXX (8 hex digits), $..., or .... */
static int isLocalLabel(const char *name)
{
    if (name[0] == '$' || name[0] == '.')
        return 1;
    if (name[0] != '_' || strlen(name) != 9)
        return 0;
    for (int i = 1; i < 9; i++)
    {
        char c = name[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
            return 0;
    }
    return 1;
}

/* The ELF's sections with bytes at a link address in the ROM window. */
static struct Section *readSections(const struct Elf *elf, size_t *count)
{
    const Elf32_Shdr *nameSection = sectionHeader(elf, elf->header->e_shstrndx);
    const char *names = elfRange(elf, nameSection->sh_offset, nameSection->sh_size, "section names");
    size_t capacity = 0, n = 0;
    struct Section *out = NULL;
    for (unsigned s = 0; s < elf->header->e_shnum; s++)
    {
        const Elf32_Shdr *section = sectionHeader(elf, s);
        if (section->sh_type != SHT_PROGBITS || !(section->sh_flags & SHF_ALLOC) || section->sh_size == 0 ||
            section->sh_addr < ROM_START || section->sh_addr >= ROM_END)
            continue;
        if (section->sh_size > ROM_END - section->sh_addr)
            fail("the ELF's section at 0x%08X runs past the ROM window", section->sh_addr);
        if (section->sh_name >= nameSection->sh_size ||
            memchr(names + section->sh_name, 0, nameSection->sh_size - section->sh_name) == NULL)
            fail("a section's name lies outside the ELF's section names");
        const char *name = names + section->sh_name;
        checkName(name, section->sh_addr);
        out = grow(out, n, &capacity, sizeof(*out));
        out[n++] = (struct Section){ section->sh_addr, section->sh_addr + section->sh_size,
                                     (section->sh_flags & SHF_EXECINSTR) != 0, *name ? name : "section" };
    }
    *count = n;
    return out;
}

static const struct Section *sectionAt(const struct Section *sections, size_t count, uint32_t address)
{
    for (size_t i = 0; i < count; i++)
    {
        if (sections[i].start <= address && address < sections[i].end)
            return &sections[i];
    }
    return NULL;
}

/* The mapping symbol in force at address within its section (0 when none is), from marks sorted by address. */
static char markAt(const struct Mark *marks, size_t count, const struct Section *section, uint32_t address)
{
    size_t lo = 0, hi = count;
    while (lo < hi)
    {
        size_t mid = (lo + hi) / 2;
        if (marks[mid].address <= address)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo == 0 || marks[lo - 1].address < section->start)
        return 0;
    return marks[lo - 1].kind;
}

/* The first value in sorted[] greater than address, or limit when none is below limit. */
static uint32_t nextAfter(const uint32_t *sorted, size_t count, uint32_t address, uint32_t limit)
{
    size_t lo = 0, hi = count;
    while (lo < hi)
    {
        size_t mid = (lo + hi) / 2;
        if (sorted[mid] <= address)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo < count && sorted[lo] < limit ? sorted[lo] : limit;
}

static int containsU32(const uint32_t *sorted, size_t count, uint32_t value)
{
    return bsearch(&value, sorted, count, sizeof(*sorted), compareU32) != NULL;
}

static uint16_t rom16(const uint8_t *rom, size_t romSize, uint32_t address)
{
    uint32_t o = address - ROM_START;
    return o + 2 <= romSize ? (uint16_t)(rom[o] | rom[o + 1] << 8) : 0;
}

static uint32_t rom32(const uint8_t *rom, size_t romSize, uint32_t address)
{
    uint32_t o = address - ROM_START;
    return o + 4 <= romSize ? (uint32_t)rom[o] | (uint32_t)rom[o + 1] << 8 | (uint32_t)rom[o + 2] << 16 |
                                  (uint32_t)rom[o + 3] << 24
                            : 0;
}

/* A BL's target, and whether a Thumb BL reaches it. */
struct Call
{
    uint32_t target;
    int thumb;
};

static int compareCalls(const void *a, const void *b)
{
    const struct Call *x = a, *y = b;
    if (x->target != y->target)
        return x->target < y->target ? -1 : 1;
    return y->thumb - x->thumb; /* a Thumb call first */
}

/*
 * The targets of every BL in the ELF's code: Thumb BL pairs in its $t runs and ARM BL (any condition) in its $a runs,
 * within executable sections, and in every function with a size (which covers an ELF without mapping symbols). A BL
 * whose first halfword or word the ELF marks $d is skipped. The same scan as score.py's _bl_targets.
 */
static struct Call *findCalls(const uint8_t *rom, size_t romSize, const struct Section *sections, size_t sectionCount,
                              const struct Mark *marks, size_t markCount, const struct RawSymbol *raw, size_t rawCount,
                              size_t *count)
{
    struct Run
    {
        uint32_t start, end;
        int thumb;
    } *runs = NULL;
    size_t runCount = 0, runCapacity = 0;
    for (size_t s = 0; s < sectionCount; s++)
    {
        if (!sections[s].executable)
            continue;
        for (size_t m = 0; m < markCount; m++)
        {
            if (marks[m].address < sections[s].start || marks[m].address >= sections[s].end || marks[m].kind == 'd')
                continue;
            uint32_t end = sections[s].end;
            if (m + 1 < markCount && marks[m + 1].address < end)
                end = marks[m + 1].address;
            runs = grow(runs, runCount, &runCapacity, sizeof(*runs));
            runs[runCount++] = (struct Run){ marks[m].address, end, marks[m].kind == 't' };
        }
    }
    for (size_t i = 0; i < rawCount; i++)
    {
        if (!isFunctionType(raw[i].type) || raw[i].size == 0)
            continue;
        uint32_t start = raw[i].value & ~1u;
        runs = grow(runs, runCount, &runCapacity, sizeof(*runs));
        runs[runCount++] = (struct Run){ start, start + raw[i].size,
                                         (raw[i].value & 1) || raw[i].type == STT_ARM_TFUNC };
    }

    struct Call *calls = NULL;
    size_t n = 0, capacity = 0;
    for (size_t r = 0; r < runCount; r++)
    {
        uint32_t lo = runs[r].start, hi = runs[r].end;
        if (hi < lo || hi - ROM_START > romSize)
            continue;
        if (runs[r].thumb)
        {
            for (uint32_t x = lo; x + 4 <= hi; x += 2)
            {
                uint16_t h = rom16(rom, romSize, x), h2 = rom16(rom, romSize, x + 2);
                const struct Section *section = sectionAt(sections, sectionCount, x);
                if (h < 0xF000 || h > 0xF7FF || h2 < 0xF800 ||
                    (section != NULL && markAt(marks, markCount, section, x) == 'd'))
                    continue;
                uint32_t offset = (uint32_t)(h & 0x7FF) << 12 | (uint32_t)(h2 & 0x7FF) << 1;
                int32_t signedOffset = offset & 0x400000 ? (int32_t)offset - 0x800000 : (int32_t)offset;
                calls = grow(calls, n, &capacity, sizeof(*calls));
                calls[n++] = (struct Call){ x + 4 + (uint32_t)signedOffset, 1 };
            }
        }
        else
        {
            for (uint32_t x = (lo + 3) & ~3u; x + 4 <= hi; x += 4)
            {
                uint32_t w = rom32(rom, romSize, x);
                const struct Section *section = sectionAt(sections, sectionCount, x);
                if (w >> 28 == 0xF || (w >> 24 & 0xF) != 0xB ||
                    (section != NULL && markAt(marks, markCount, section, x) == 'd'))
                    continue;
                uint32_t offset = (w & 0xFFFFFF) << 2;
                int32_t signedOffset = offset & 0x2000000 ? (int32_t)offset - 0x4000000 : (int32_t)offset;
                calls = grow(calls, n, &capacity, sizeof(*calls));
                calls[n++] = (struct Call){ x + 8 + (uint32_t)signedOffset, 0 };
            }
        }
    }
    free(runs);
    qsort(calls, n, sizeof(*calls), compareCalls);
    size_t kept = 0;
    for (size_t i = 0; i < n; i++)
    {
        if (kept > 0 && calls[kept - 1].target == calls[i].target)
            continue; /* a Thumb call sorts first, so a target any Thumb BL reaches stays Thumb */
        calls[kept++] = calls[i];
    }
    *count = kept;
    return calls;
}

static const char *const REGISTER_NAMES[16] = { "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
                                                "r8", "sb", "sl", "fp", "ip", "sp", "lr", "pc" };

static char *textf(const char *fmt, ...) __attribute__((__format__(printf, 1, 2)));

static char *textf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int length = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    char *out = allocate((size_t)length + 1);
    va_start(args, fmt);
    vsnprintf(out, (size_t)length + 1, fmt, args);
    va_end(args);
    return out;
}

/* Statistics for the summary line. */
struct Summary
{
    size_t functions, thunks, labels, data, ranges;
    uint64_t coveredBytes, uncoveredCode;
};

/* The alphabetically first STT_NOTYPE symbol at an address (bit 0 cleared), mapping symbols left out, or NULL. */
static const char *labelAt(const struct RawSymbol *raw, size_t rawCount, uint32_t address)
{
    const char *found = NULL;
    for (size_t i = 0; i < rawCount; i++)
    {
        if (raw[i].type == STT_NOTYPE && raw[i].name[0] != '$' && (raw[i].value & ~1u) == address &&
            (found == NULL || strcmp(raw[i].name, found) < 0))
            found = raw[i].name;
    }
    return found;
}

/*
 * Adds the functions the ELF has no symbol for (call thunks and code labels a BL reaches, and a labelled cartridge
 * entry) as the comment at the top describes.
 */
static void addCalledCode(struct Symbols *table, const uint8_t *rom, size_t romSize, const struct Section *sections,
                          size_t sectionCount, const struct Mark *marks, size_t markCount,
                          const struct RawSymbol *raw, size_t rawCount, struct Summary *summary)
{
    /* Function starts, and the sized functions as spans for "inside". */
    size_t startCount = 0, spanCount = 0;
    uint32_t *starts = allocate(rawCount * sizeof(*starts));
    struct Span *spans = allocate(rawCount * sizeof(*spans));
    for (size_t i = 0; i < rawCount; i++)
    {
        if (!isFunctionType(raw[i].type))
            continue;
        starts[startCount++] = raw[i].value & ~1u;
        if (raw[i].size)
            spans[spanCount++] = (struct Span){ raw[i].value & ~1u, (raw[i].value & ~1u) + raw[i].size };
    }
    qsort(starts, startCount, sizeof(*starts), compareU32);
    qsort(spans, spanCount, sizeof(*spans), compareSpans);

    size_t callCount;
    struct Call *calls = findCalls(rom, romSize, sections, sectionCount, marks, markCount, raw, rawCount, &callCount);

    /* The cartridge entry: the header's ARM branch (b, condition always) at 0x08000000. */
    uint32_t reset = rom32(rom, romSize, ROM_START);
    if (reset >> 24 == 0xEA)
    {
        uint32_t offset = (reset & 0xFFFFFF) << 2;
        int32_t signedOffset = offset & 0x2000000 ? (int32_t)offset - 0x4000000 : (int32_t)offset;
        struct Call *grown = realloc(calls, (callCount + 1) * sizeof(*calls));
        if (grown == NULL)
            fail("out of memory");
        calls = grown;
        calls[callCount++] = (struct Call){ ROM_START + 8 + (uint32_t)signedOffset, -1 };
    }

    /* Starts known so far, for sizing the labels: the ELF's functions and every target added below. */
    uint32_t *allStarts = allocate((startCount + callCount) * sizeof(*allStarts));
    memcpy(allStarts, starts, startCount * sizeof(*starts));
    size_t allCount = startCount;
    for (size_t i = 0; i < callCount; i++)
        allStarts[allCount++] = calls[i].target & ~1u;
    qsort(allStarts, allCount, sizeof(*allStarts), compareU32);

    uint32_t *dataMarks = allocate((markCount ? markCount : 1) * sizeof(*dataMarks));
    size_t dataCount = 0;
    for (size_t m = 0; m < markCount; m++)
    {
        if (marks[m].kind == 'd')
            dataMarks[dataCount++] = marks[m].address;
    }

    for (size_t i = 0; i < callCount; i++)
    {
        uint32_t t = calls[i].target;
        int cartridgeEntry = calls[i].thumb < 0;
        const struct Section *section = sectionAt(sections, sectionCount, t);
        if ((t & 1) || containsU32(starts, startCount, t) || section == NULL || !section->executable ||
            markAt(marks, markCount, section, t) == 'd')
            continue;
        /* Inside the sized function with the nearest start before it: a second entry. */
        size_t lo = 0, hi = spanCount;
        while (lo < hi)
        {
            size_t mid = (lo + hi) / 2;
            if (spans[mid].start <= t)
                lo = mid + 1;
            else
                hi = mid;
        }
        if (lo > 0 && t < spans[lo - 1].end)
            continue;
        uint16_t h = rom16(rom, romSize, t);
        if (!cartridgeEntry && calls[i].thumb && (h & 0xFF87) == 0x4700)
        {
            addSymbol(table, t | 1u, 2, KIND_FUNCTION, textf("_call_via_%s_%08X", REGISTER_NAMES[h >> 3 & 15], t));
            summary->thunks++;
            continue;
        }
        const char *label = labelAt(raw, rawCount, t);
        if (label == NULL && cartridgeEntry)
            label = "cartridge_entry";
        if (label == NULL)
            continue;
        uint32_t end = nextAfter(allStarts, allCount, t, section->end);
        end = nextAfter(dataMarks, dataCount, t, end);
        addSymbol(table, t | (calls[i].thumb > 0 ? 1u : 0u), end - t, KIND_FUNCTION, label);
        summary->labels++;
    }
    free(dataMarks);
    free(allStarts);
    free(calls);
    free(spans);
    free(starts);
}

/* Embedded programs, which are never covered, as the comment at the top describes. */
static struct Ranges findEmbeddedPrograms(const uint8_t *rom, size_t romSize, const struct Section *sections,
                                          size_t sectionCount, const struct RawSymbol *raw, size_t rawCount,
                                          const struct Symbols *table, char **notCovered, int notCoveredCount)
{
    static const uint8_t LOGO[16] = { 0x24, 0xFF, 0xAE, 0x51, 0x69, 0x9A, 0xA2, 0x21,
                                      0x3D, 0x84, 0x82, 0x0A, 0x84, 0xE4, 0x09, 0xAD };
    struct Ranges out = { 0 };
    uint32_t *bounds = allocate((rawCount ? rawCount : 1) * sizeof(*bounds));
    size_t boundCount = 0;
    for (size_t i = 0; i < rawCount; i++)
    {
        if (!isLocalLabel(raw[i].name))
            bounds[boundCount++] = raw[i].value & ~1u;
    }
    qsort(bounds, boundCount, sizeof(*bounds), compareU32);
    uint32_t *starts = allocate((table->count ? table->count : 1) * sizeof(*starts));
    size_t startCount = 0;
    for (size_t i = 0; i < table->count; i++)
    {
        if (table->items[i].kind == KIND_FUNCTION)
            starts[startCount++] = table->items[i].address & ~1u;
    }
    qsort(starts, startCount, sizeof(*starts), compareU32);

    for (size_t s = 0; s < sectionCount; s++)
    {
        uint32_t lo = sections[s].start, hi = sections[s].end;
        if (hi - ROM_START > romSize)
            continue;
        for (uint32_t a = lo; a + sizeof(LOGO) <= hi; a++)
        {
            if (memcmp(rom + (a - ROM_START), LOGO, sizeof(LOGO)) != 0)
                continue;
            uint32_t header = a - 4;
            if (a < 4 || header < ROM_START + 0x100)
                continue;
            uint32_t end = nextAfter(bounds, boundCount, header, hi);
            end = nextAfter(starts, startCount, header, end);
            if (end > header)
                addRange(&out, header, end);
        }
    }
    for (int i = 0; i < notCoveredCount; i++)
    {
        const struct RawSymbol *found = NULL;
        for (size_t j = 0; j < rawCount && found == NULL; j++)
        {
            if (strcmp(raw[j].name, notCovered[i]) == 0)
                found = &raw[j];
        }
        if (found == NULL)
            fail("--not-covered %s: the ELF has no such symbol in the ROM", notCovered[i]);
        uint32_t start = found->value & ~1u, end = start + found->size;
        if (found->size == 0)
        {
            char *endName = textf("%s_end", notCovered[i]);
            end = 0;
            for (size_t j = 0; j < rawCount; j++)
            {
                if (strcmp(raw[j].name, endName) == 0)
                    end = raw[j].value & ~1u;
            }
            if (end <= start)
                fail("--not-covered %s: the symbol has no size and no %s after it", notCovered[i], endName);
            free(endName);
        }
        addRange(&out, start, end);
    }
    free(starts);
    free(bounds);
    qsort(out.items, out.count, sizeof(*out.items), compareRanges);
    return out;
}

/* Adds the coverage ranges, as the comment at the top describes, and reports the code left uncovered. */
static void addCoverage(struct Symbols *table, const uint8_t *rom, size_t romSize, const struct Section *sections,
                        size_t sectionCount, const struct Mark *marks, size_t markCount, const struct Ranges *excluded,
                        struct Summary *summary)
{
    /* What each function covers: its size, or up to the next start, never past the end of its section. */
    uint32_t *starts = allocate((table->count ? table->count : 1) * sizeof(*starts));
    size_t startCount = 0;
    for (size_t i = 0; i < table->count; i++)
    {
        if (table->items[i].kind == KIND_FUNCTION)
            starts[startCount++] = table->items[i].address & ~1u;
    }
    qsort(starts, startCount, sizeof(*starts), compareU32);
    struct Span *spans = allocate((table->count ? table->count : 1) * sizeof(*spans));
    size_t spanCount = 0;
    for (size_t i = 0; i < table->count; i++)
    {
        const struct Symbol *f = &table->items[i];
        if (f->kind != KIND_FUNCTION)
            continue;
        uint32_t start = f->address & ~1u;
        const struct Section *section = sectionAt(sections, sectionCount, start);
        uint32_t limit = section ? section->end : start;
        uint32_t end = f->size ? start + f->size : nextAfter(starts, startCount, start, limit);
        if (end > limit)
            end = limit;
        if (end > start)
            spans[spanCount++] = (struct Span){ start, end };
    }
    qsort(spans, spanCount, sizeof(*spans), compareSpans);

    size_t romWindow = romSize < ROM_END - ROM_START ? romSize : ROM_END - ROM_START;
    uint8_t *inFunction = allocate(romWindow);
    memset(inFunction, 0, romWindow);
    for (size_t i = 0; i < spanCount; i++)
    {
        uint32_t lo = spans[i].start - ROM_START, hi = spans[i].end - ROM_START;
        if (hi > romWindow)
            hi = (uint32_t)romWindow;
        if (lo < hi)
            memset(inFunction + lo, 1, hi - lo);
    }

    /* Each byte of a section: covered, not covered, or code (or a byte with no mapping symbol) in no function. */
    enum
    {
        NOT_COVERED,
        COVERED,
        UNCOVERED_CODE,
    };
    struct Range shown[10];
    size_t shownCount = 0, uncoveredRuns = 0;
    for (size_t s = 0; s < sectionCount; s++)
    {
        const struct Section *section = &sections[s];
        if (section->start - ROM_START >= romWindow)
            continue;
        uint32_t end = section->end - ROM_START > romWindow ? (uint32_t)(ROM_START + romWindow) : section->end;
        uint32_t length = end - section->start;
        uint8_t *state = allocate(length);
        size_t e = 0;
        for (uint32_t a = section->start; a < end; a++)
        {
            while (e < excluded->count && excluded->items[e].end <= a)
                e++;
            uint8_t value;
            if (a < ROM_START + CARTRIDGE_HEADER)
                value = COVERED;
            else if (e < excluded->count && excluded->items[e].start <= a)
                value = NOT_COVERED;
            else if (!section->executable)
                value = COVERED;
            else
            {
                char kind = markAt(marks, markCount, section, a);
                value = kind == 'd' || ((kind == 'a' || kind == 't') && inFunction[a - ROM_START]) ? COVERED
                                                                                                  : UNCOVERED_CODE;
            }
            state[a - section->start] = value;
        }
        /* Code in no function that is only alignment padding is covered: 0xFF bytes, or Thumb halfwords 0x0000 and
           0x46C0 (nop), or ARM words 0x00000000 and 0xE1A00000 (nop). */
        for (uint32_t i = 0; i < length;)
        {
            if (state[i] != UNCOVERED_CODE)
            {
                i++;
                continue;
            }
            uint32_t j = i;
            while (j < length && state[j] == UNCOVERED_CODE)
                j++;
            uint32_t lo = section->start + i, hi = section->start + j;
            char kind = markAt(marks, markCount, section, lo);
            int padding = 1;
            for (uint32_t a = lo; a < hi && padding; a++)
                padding = rom[a - ROM_START] == 0xFF;
            if (!padding && kind == 't' && !(lo & 1) && !((hi - lo) & 1))
            {
                padding = 1;
                for (uint32_t a = lo; a < hi && padding; a += 2)
                {
                    uint16_t h = rom16(rom, romSize, a);
                    padding = h == 0x0000 || h == 0x46C0;
                }
            }
            if (!padding && kind == 'a' && !(lo & 3) && !((hi - lo) & 3))
            {
                padding = 1;
                for (uint32_t a = lo; a < hi && padding; a += 4)
                {
                    uint32_t w = rom32(rom, romSize, a);
                    padding = w == 0x00000000 || w == 0xE1A00000;
                }
            }
            if (padding)
                memset(state + i, COVERED, j - i);
            else
            {
                summary->uncoveredCode += hi - lo;
                if (shownCount < 10)
                    shown[shownCount++] = (struct Range){ lo, hi };
                uncoveredRuns++;
            }
            i = j;
        }
        for (uint32_t i = 0; i < length;)
        {
            if (state[i] != COVERED)
            {
                i++;
                continue;
            }
            uint32_t j = i;
            while (j < length && state[j] == COVERED)
                j++;
            addSymbol(table, section->start + i, j - i, KIND_COVERAGE, section->name);
            summary->ranges++;
            summary->coveredBytes += j - i;
            i = j;
        }
        free(state);
    }
    if (summary->uncoveredCode)
    {
        fprintf(stderr, "admgsym: %llu bytes of code in no function, or with no mapping symbol, in %zu stretches, are left out of "
                "coverage:",
                (unsigned long long)summary->uncoveredCode, uncoveredRuns);
        for (size_t i = 0; i < shownCount; i++)
            fprintf(stderr, " 0x%08X-0x%08X", shown[i].start, shown[i].end - 1);
        fputs(uncoveredRuns > shownCount ? " ...\n" : "\n", stderr);
    }
    free(inFunction);
    free(spans);
    free(starts);
}

/*
 * The table's entries: the ELF's ROM functions (STT_FUNC, or STT_ARM_TFUNC from older toolchains, which marks Thumb
 * by type rather than by bit 0) and ROM data objects (STT_OBJECT), the called code it has no function symbol for, and
 * the coverage ranges; sorted, with exact repeats removed.
 */
static struct Symbols readTable(const struct Elf *elf, const uint8_t *rom, size_t romSize, char **notCovered,
                                int notCoveredCount, struct Summary *summary)
{
    size_t rawCount;
    struct RawSymbol *raw = readRawSymbols(elf, &rawCount);
    struct Symbols table = { 0 };
    size_t markCount = 0;
    struct Mark *marks = allocate((rawCount ? rawCount : 1) * sizeof(*marks));
    for (size_t i = 0; i < rawCount; i++)
    {
        unsigned type = raw[i].type;
        char kind = markKind(raw[i].name);
        if (kind)
        {
            marks[markCount++] = (struct Mark){ raw[i].value & ~1u, kind };
            continue;
        }
        if (!isFunctionType(type) && type != STT_OBJECT)
            continue;
        checkName(raw[i].name, raw[i].value);
        addSymbol(&table, raw[i].value | (type == STT_ARM_TFUNC ? 1u : 0u), raw[i].size,
                  type == STT_OBJECT ? KIND_DATA : KIND_FUNCTION, raw[i].name);
    }
    qsort(marks, markCount, sizeof(*marks), compareMarks);

    size_t sectionCount;
    struct Section *sections = readSections(elf, &sectionCount);
    addCalledCode(&table, rom, romSize, sections, sectionCount, marks, markCount, raw, rawCount, summary);
    for (size_t i = 0; i < table.count; i++)
    {
        if (table.items[i].kind == KIND_FUNCTION)
            checkName(table.items[i].name, table.items[i].address);
    }
    struct Ranges excluded =
        findEmbeddedPrograms(rom, romSize, sections, sectionCount, raw, rawCount, &table, notCovered, notCoveredCount);
    addCoverage(&table, rom, romSize, sections, sectionCount, marks, markCount, &excluded, summary);

    qsort(table.items, table.count, sizeof(*table.items), compareSymbols);
    size_t kept = 0;
    for (size_t i = 0; i < table.count; i++)
    {
        if (kept > 0 && compareSymbols(&table.items[kept - 1], &table.items[i]) == 0)
            continue;
        table.items[kept++] = table.items[i];
    }
    table.count = kept;
    for (size_t i = 0; i < table.count; i++)
    {
        if (table.items[i].kind == KIND_FUNCTION)
            summary->functions++;
        else if (table.items[i].kind == KIND_DATA)
            summary->data++;
    }
    free(excluded.items);
    free(sections);
    free(marks);
    free(raw);
    return table;
}

static uint32_t hashName(const char *s)
{
    uint32_t h = 2166136261u; /* FNV-1a */
    for (; *s; s++)
        h = (h ^ (uint8_t)*s) * 16777619u;
    return h;
}

/* Builds the string table, each name once in order of first use, and sets each symbol's name offset. */
static uint8_t *buildStrings(struct Symbol *symbols, size_t count, uint32_t *length)
{
    size_t buckets = 1;
    while (buckets < count * 2 + 1)
        buckets <<= 1;
    size_t *slots = allocate(buckets * sizeof(*slots)); /* index + 1 of the first symbol with that name; 0 is empty */
    memset(slots, 0, buckets * sizeof(*slots));
    size_t total = 0;
    for (size_t i = 0; i < count; i++)
        total += strlen(symbols[i].name) + 1;
    uint8_t *strings = allocate(total);
    uint32_t used = 0;
    for (size_t i = 0; i < count; i++)
    {
        size_t slot = hashName(symbols[i].name) & (buckets - 1);
        while (slots[slot] != 0 && strcmp(symbols[slots[slot] - 1].name, symbols[i].name) != 0)
            slot = (slot + 1) & (buckets - 1);
        if (slots[slot] != 0)
        {
            symbols[i].nameOffset = symbols[slots[slot] - 1].nameOffset;
            continue;
        }
        size_t size = strlen(symbols[i].name) + 1;
        if (used + size > MAX_STRINGS)
            fail("the names take more than %u bytes, the most a 3-byte name offset reaches", MAX_STRINGS);
        symbols[i].nameOffset = used;
        memcpy(strings + used, symbols[i].name, size);
        used += (uint32_t)size;
        slots[slot] = i + 1;
    }
    free(slots);
    *length = used;
    return strings;
}

/* The header and compressed payload, with both CRCs; romCrc is the ROM CRC-32 the caller computed. */
static uint8_t *buildTable(struct Symbols *symbols, uint32_t romCrc, uint32_t *tableLength)
{
    size_t count = symbols->count;
    if (count == 0)
        fail("the ELF has no named ROM functions or data objects");
    uint32_t stringLength;
    uint8_t *strings = buildStrings(symbols->items, count, &stringLength);

    size_t payloadSize = count * 12 + stringLength;
    uint8_t *payload = allocate(payloadSize);
    for (size_t i = 0; i < count; i++)
    {
        const struct Symbol *s = &symbols->items[i];
        uint8_t *e = payload + i * 12;
        put32(e, s->address);
        put32(e + 4, s->size);
        e[8] = s->kind;
        e[9] = (uint8_t)s->nameOffset;
        e[10] = (uint8_t)(s->nameOffset >> 8);
        e[11] = (uint8_t)(s->nameOffset >> 16);
    }
    memcpy(payload + count * 12, strings, stringLength);

    mz_ulong stored = mz_compressBound((mz_ulong)payloadSize);
    uint8_t *table = allocate(HEADER_SIZE + stored);
    if (mz_compress2(table + HEADER_SIZE, &stored, payload, (mz_ulong)payloadSize, MZ_BEST_COMPRESSION) != MZ_OK)
        fail("compressing the table failed");

    /* Read it back: the table must decompress to exactly the payload. */
    uint8_t *check = allocate(payloadSize);
    mz_ulong checkSize = (mz_ulong)payloadSize;
    if (mz_uncompress(check, &checkSize, table + HEADER_SIZE, stored) != MZ_OK || checkSize != payloadSize ||
        memcmp(check, payload, payloadSize) != 0)
        fail("the compressed table doesn't decompress to the table");

    memcpy(table, "ADMGSYM1", 8);
    put16(table + 8, FORMAT_VERSION);
    put16(table + 10, 0);
    put32(table + 12, (uint32_t)count);
    put32(table + 16, stringLength);
    put32(table + 20, romCrc);
    put32(table + 24, 0);
    put32(table + 28, 0);
    *tableLength = HEADER_SIZE + (uint32_t)stored;
    put32(table + 24, crc32Of(table, *tableLength));

    free(check);
    free(payload);
    free(strings);
    return table;
}

static void printSummary(const struct Summary *s)
{
    fprintf(stderr,
            "admgsym: %zu functions (%zu call thunks and %zu code labels without a function symbol), %zu data objects, "
            "%zu coverage ranges over %llu bytes\n",
            s->functions, s->thunks, s->labels, s->data, s->ranges, (unsigned long long)s->coveredBytes);
}

static void embed(const char *elfPath, const char *romPath, char **notCovered, int notCoveredCount)
{
    struct Elf elf = openElf(elfPath);
    size_t romSize;
    uint8_t *rom = readFile(romPath, &romSize);
    if (romSize > ROM_END - ROM_START)
        fail("%s is larger than the GBA's 32 MiB ROM window", romPath);
    uint32_t gameEnd = checkRomMatchesElf(&elf, rom, romSize);
    for (size_t i = gameEnd; i < romSize; i++)
    {
        if (rom[i] != 0xFF)
            fail("the byte at 0x%zX after the game isn't 0xFF padding (does the ROM already have a table?)", i);
    }
    uint32_t tableOffset = (gameEnd + 15) & ~15u;
    if (tableOffset + HEADER_SIZE + FOOTER_SIZE > romSize)
        fail("the symbol table needs more than %zu bytes, but only %zu bytes of 0xFF padding follow the game at 0x%X",
             (size_t)(tableOffset - gameEnd) + HEADER_SIZE + FOOTER_SIZE, romSize - gameEnd, gameEnd);
    struct Summary summary = { 0 };
    struct Symbols symbols = readTable(&elf, rom, romSize, notCovered, notCoveredCount, &summary);
    uint32_t tableLength;
    uint8_t *table = buildTable(&symbols, crc32Of(rom, tableOffset), &tableLength);
    size_t needed = (size_t)(tableOffset - gameEnd) + tableLength + FOOTER_SIZE;
    if (needed > romSize - gameEnd)
        fail("the symbol table needs %zu bytes, but only %zu bytes of 0xFF padding follow the game at 0x%X", needed,
             romSize - gameEnd, gameEnd);
    memcpy(rom + tableOffset, table, tableLength);
    uint8_t *footer = rom + romSize - FOOTER_SIZE;
    memcpy(footer, "ADMGSYMF", 8);
    put32(footer + 8, tableOffset);
    put32(footer + 12, tableLength);
    writeFile(romPath, rom, romSize);
    printSummary(&summary);
    free(table);
    free(rom);
}

static void sidecar(const char *elfPath, const char *romPath, const char *outPath, char **notCovered,
                    int notCoveredCount)
{
    struct Elf elf = openElf(elfPath);
    size_t romSize;
    uint8_t *rom = readFile(romPath, &romSize);
    checkRomMatchesElf(&elf, rom, romSize);
    struct Summary summary = { 0 };
    struct Symbols symbols = readTable(&elf, rom, romSize, notCovered, notCoveredCount, &summary);
    uint32_t tableLength;
    uint8_t *table = buildTable(&symbols, crc32Of(rom, romSize), &tableLength);
    writeFile(outPath, table, tableLength);
    printSummary(&summary);
    free(table);
    free(rom);
}

static int usage(void)
{
    fputs("Usage: admgsym embed [--not-covered <symbol>]... <elf> <rom>\n"
          "       admgsym sidecar [--not-covered <symbol>]... <elf> <rom> <out.admgsym>\n",
          stderr);
    return 2;
}

int main(int argc, char **argv)
{
    if (argc < 2)
        return usage();
    char **notCovered = allocate((size_t)argc * sizeof(*notCovered));
    char **paths = allocate((size_t)argc * sizeof(*paths));
    int notCoveredCount = 0, pathCount = 0;
    for (int i = 2; i < argc; i++)
    {
        if (strcmp(argv[i], "--not-covered") == 0)
        {
            if (i + 1 >= argc)
                return usage();
            notCovered[notCoveredCount++] = argv[++i];
        }
        else
            paths[pathCount++] = argv[i];
    }
    if (strcmp(argv[1], "embed") == 0 && pathCount == 2)
        embed(paths[0], paths[1], notCovered, notCoveredCount);
    else if (strcmp(argv[1], "sidecar") == 0 && pathCount == 3)
        sidecar(paths[0], paths[1], paths[2], notCovered, notCoveredCount);
    else
        return usage();
    return 0;
}
