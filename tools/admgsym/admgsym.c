/*
 * admgsym: writes an ADMGSYM1 symbol table, the ROM functions and data objects of a GBA game's ELF, for the GBA loader
 * of ADMG-101's Ghidra (gba-ghidra-loader) to read.
 *
 *   admgsym embed <elf> <rom>                   write the table into the ROM's 0xFF padding, with a footer
 *   admgsym sidecar <elf> <rom> <out.admgsym>   write the table to its own file; the ROM is not changed
 *
 * The format, little-endian throughout:
 *
 *   Header, 32 bytes:
 *     0   8  magic "ADMGSYM1"
 *     8   2  format version, 1
 *     10  2  flags, 0
 *     12  4  entry count
 *     16  4  string table length
 *     20  4  CRC-32 of the ROM: embedded, the file from 0 up to the table; a sidecar, the whole ROM file
 *     24  4  CRC-32 of the header and the stored payload, with this field read as 0
 *     28  4  reserved, 0
 *   Payload: a zlib stream (RFC 1950) holding the entries, then the string table.
 *   Entry, 12 bytes, sorted by address, then kind, then name, then size:
 *     0   4  address, 0x08000000-0x09FFFFFF; for a function, bit 0 set marks Thumb
 *     4   4  size in bytes, 0 if unknown
 *     8   1  kind: 0 function, 1 data object
 *     9   3  offset of the name in the string table
 *   String table: NUL-terminated names of printable ASCII (0x21-0x7E), each stored once.
 *   Footer, embedded only, the last 16 bytes of the ROM:
 *     0   8  magic "ADMGSYMF"
 *     8   4  table offset in the file
 *     12  4  table length in bytes (header and payload)
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

struct Symbol
{
    uint32_t address;
    uint32_t size;
    uint8_t kind;
    const char *name;
    uint32_t nameOffset;
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

/*
 * The ELF's ROM functions (STT_FUNC, or STT_ARM_TFUNC from older toolchains, which marks Thumb by type rather than by
 * bit 0) and ROM data objects (STT_OBJECT), defined and named, sorted, with exact repeats removed.
 */
static struct Symbol *readSymbols(const struct Elf *elf, size_t *count)
{
    size_t capacity = 1024, n = 0;
    struct Symbol *symbols = allocate(capacity * sizeof(*symbols));
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
            unsigned type = ELF32_ST_TYPE(sym->st_info);
            if (type != STT_FUNC && type != STT_ARM_TFUNC && type != STT_OBJECT)
                continue;
            if (sym->st_shndx == SHN_UNDEF || sym->st_value < ROM_START || sym->st_value >= ROM_END)
                continue;
            if (sym->st_name >= strings->sh_size || memchr(names + sym->st_name, 0, strings->sh_size - sym->st_name) == NULL)
                fail("a symbol's name lies outside the ELF's string table");
            const char *name = names + sym->st_name;
            if (*name == '\0')
                continue;
            for (const char *c = name; *c; c++)
            {
                if (*c < 0x21 || *c > 0x7E)
                    fail("the symbol %s at 0x%08X has a character outside printable ASCII", name, sym->st_value);
            }
            if (n == capacity)
            {
                capacity *= 2;
                struct Symbol *grown = realloc(symbols, capacity * sizeof(*symbols));
                if (grown == NULL)
                    fail("out of memory");
                symbols = grown;
            }
            symbols[n].address = sym->st_value | (type == STT_ARM_TFUNC ? 1u : 0u);
            symbols[n].size = sym->st_size;
            symbols[n].kind = type == STT_OBJECT ? 1 : 0;
            symbols[n].name = name;
            n++;
        }
    }
    qsort(symbols, n, sizeof(*symbols), compareSymbols);
    size_t kept = 0;
    for (size_t i = 0; i < n; i++)
    {
        if (kept > 0 && compareSymbols(&symbols[kept - 1], &symbols[i]) == 0)
            continue;
        symbols[kept++] = symbols[i];
    }
    *count = kept;
    return symbols;
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
static uint8_t *buildTable(const struct Elf *elf, uint32_t romCrc, uint32_t *tableLength)
{
    size_t count;
    struct Symbol *symbols = readSymbols(elf, &count);
    if (count == 0)
        fail("the ELF has no named ROM functions or data objects");
    uint32_t stringLength;
    uint8_t *strings = buildStrings(symbols, count, &stringLength);

    size_t payloadSize = count * 12 + stringLength;
    uint8_t *payload = allocate(payloadSize);
    for (size_t i = 0; i < count; i++)
    {
        uint8_t *e = payload + i * 12;
        put32(e, symbols[i].address);
        put32(e + 4, symbols[i].size);
        e[8] = symbols[i].kind;
        e[9] = (uint8_t)symbols[i].nameOffset;
        e[10] = (uint8_t)(symbols[i].nameOffset >> 8);
        e[11] = (uint8_t)(symbols[i].nameOffset >> 16);
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
    put16(table + 8, 1);
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
    free(symbols);
    return table;
}

static void embed(const char *elfPath, const char *romPath)
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
    uint32_t tableLength;
    uint8_t *table = buildTable(&elf, crc32Of(rom, tableOffset), &tableLength);
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
    free(table);
    free(rom);
}

static void sidecar(const char *elfPath, const char *romPath, const char *outPath)
{
    struct Elf elf = openElf(elfPath);
    size_t romSize;
    uint8_t *rom = readFile(romPath, &romSize);
    checkRomMatchesElf(&elf, rom, romSize);
    uint32_t tableLength;
    uint8_t *table = buildTable(&elf, crc32Of(rom, romSize), &tableLength);
    writeFile(outPath, table, tableLength);
    free(table);
    free(rom);
}

int main(int argc, char **argv)
{
    if (argc == 4 && strcmp(argv[1], "embed") == 0)
        embed(argv[2], argv[3]);
    else if (argc == 5 && strcmp(argv[1], "sidecar") == 0)
        sidecar(argv[2], argv[3], argv[4]);
    else
    {
        fputs("Usage: admgsym embed <elf> <rom>\n"
              "       admgsym sidecar <elf> <rom> <out.admgsym>\n",
              stderr);
        return 2;
    }
    return 0;
}
