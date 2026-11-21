/*
 * admgext: writes an extended ROM (.admgrom) for ADMG-101: a GBA game's cartridge image followed by the data its ELF
 * links into the extended ROM, the .romx section in the ROMX region at 0x40000000-0x4FFFFFFF (ld_script_modern.ld).
 * ADMG-101 is to map that data at its link address; it doesn't read .admgrom files yet. No GBA or emulator can run the
 * result.
 *
 *   admgext <elf> <cartridge> <out.admgrom>
 *
 * <cartridge> is the cartridge image made from the same ELF without .romx (objcopy -O binary -R .romx, then gbafix).
 *
 * The format, little-endian throughout:
 *
 *   0          the cartridge image, padded with 0xFF to exactly 0x2000000 bytes (32 MiB)
 *   0x2000000  header, 32 bytes:
 *                0   8  magic "ADMGEXT1"
 *                8   2  format version, 1
 *                10  2  segment count
 *                12  4  CRC-32 of the 32 MiB cartridge image
 *                16  4  CRC-32 of the header and the segment table, with this field read as 0
 *                20  12 reserved, 0
 *   0x2000020  segment table, 16 bytes per segment:
 *                0   4  load address
 *                4   4  size in bytes
 *                8   4  offset of the segment in the file, a multiple of 0x10000 (64 KiB), so it can be mapped
 *                12  4  flags: bit 0 read-only, always set in version 1
 *   Each segment at its offset. The bytes between the segment table and the first segment are 0.
 *
 * Version 1 has one segment, .romx. Only constant data goes in it. admgext writes nothing, and fails, when:
 *   - a section linked into ROMX is executable, or a function symbol or an $a or $t mapping symbol lies in ROMX;
 *   - a section linked into ROMX is writable, or one other than .romx has bytes there;
 *   - the cartridge image is over 32 MiB or an odd number of bytes;
 *   - the cartridge image doesn't hold the ELF's ROM segments byte for byte after the 0xC0-byte cartridge header,
 *     which builds fill in after linking (gbafix);
 *   - the ELF has no .romx section, or it's empty.
 *
 * CRC-32 is miniz's (tools/admgsym/miniz.c, MIT licence in tools/admgsym/LICENSE.miniz), as admgsym's is.
 */
#include <errno.h>
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
#define ROMX_START 0x40000000u
#define ROMX_END 0x50000000u
#define CARTRIDGE_SIZE 0x2000000u
#define CARTRIDGE_HEADER 0xC0u
#define HEADER_SIZE 32u
#define SEGMENT_ENTRY_SIZE 16u
#define SEGMENT_ALIGN 0x10000u
#define FORMAT_VERSION 1
#define FLAG_READ_ONLY 1u

static void fail(const char *format, ...) __attribute__((noreturn, format(printf, 1, 2)));

static void fail(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fputs("admgext: ", stderr);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);
    exit(1);
}

static uint8_t *readFile(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        fail("can't read %s: %s", path, strerror(errno));
    if (fseek(f, 0, SEEK_END) != 0)
        fail("can't read %s: %s", path, strerror(errno));
    long length = ftell(f);
    if (length < 0 || fseek(f, 0, SEEK_SET) != 0)
        fail("can't read %s: %s", path, strerror(errno));
    uint8_t *data = malloc(length ? (size_t)length : 1);
    if (data == NULL)
        fail("can't read %s: out of memory", path);
    if (fread(data, 1, (size_t)length, f) != (size_t)length)
        fail("can't read %s: %s", path, ferror(f) ? strerror(errno) : "the file ended early");
    fclose(f);
    *size = (size_t)length;
    return data;
}

/* Writes to <path>.tmp and renames it over path, so a failed write never leaves path partial. */
static void writeFile(const char *path, const uint8_t *data, size_t size)
{
    size_t length = strlen(path);
    char *temporary = malloc(length + 5);
    if (temporary == NULL)
        fail("can't write %s: out of memory", path);
    memcpy(temporary, path, length);
    memcpy(temporary + length, ".tmp", 5);
    FILE *f = fopen(temporary, "wb");
    if (f == NULL)
        fail("can't write %s: %s", temporary, strerror(errno));
    int written = fwrite(data, 1, size, f) == size;
    int error = errno;
    if (fclose(f) != 0 || !written)
    {
        remove(temporary);
        fail("can't write %s: %s", temporary, strerror(written ? errno : error));
    }
#ifdef _WIN32
    /* Windows' rename doesn't replace an existing file; MoveFileEx does, and leaves path as it was if it fails. */
    if (!MoveFileExA(temporary, path, MOVEFILE_REPLACE_EXISTING))
    {
        remove(temporary);
        fail("can't write %s: Windows error %lu", path, (unsigned long)GetLastError());
    }
#else
    if (rename(temporary, path) != 0)
    {
        int renameError = errno;
        remove(temporary);
        fail("can't write %s: %s", path, strerror(renameError));
    }
#endif
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
    const char *path;
    const uint8_t *data;
    size_t size;
    const Elf32_Ehdr *header;
};

static const void *elfRange(const struct Elf *elf, uint64_t offset, uint64_t size, const char *what)
{
    if (offset > elf->size || size > elf->size - offset)
        fail("can't read %s: its %s lie outside the file", elf->path, what);
    return elf->data + offset;
}

static struct Elf openElf(const char *path)
{
    struct Elf elf;
    elf.path = path;
    elf.data = readFile(path, &elf.size);
    if (elf.size < sizeof(Elf32_Ehdr) || memcmp(elf.data, ELFMAG, SELFMAG) != 0)
        fail("can't read %s: it isn't an ELF file", path);
    elf.header = (const Elf32_Ehdr *)elf.data;
    const unsigned char *id = elf.header->e_ident;
    if (id[EI_CLASS] != ELFCLASS32 || id[EI_DATA] != ELFDATA2LSB || elf.header->e_machine != EM_ARM)
        fail("can't read %s: it isn't a 32-bit little-endian ARM ELF", path);
    if (elf.header->e_phentsize != sizeof(Elf32_Phdr) || elf.header->e_shentsize != sizeof(Elf32_Shdr))
        fail("can't read %s: its headers have an unexpected size", path);
    return elf;
}

static const Elf32_Phdr *programHeader(const struct Elf *elf, unsigned i)
{
    return elfRange(elf, (uint64_t)elf->header->e_phoff + (uint64_t)i * sizeof(Elf32_Phdr), sizeof(Elf32_Phdr),
                    "program headers");
}

static const Elf32_Shdr *sectionHeader(const struct Elf *elf, unsigned i)
{
    return elfRange(elf, (uint64_t)elf->header->e_shoff + (uint64_t)i * sizeof(Elf32_Shdr), sizeof(Elf32_Shdr),
                    "section headers");
}

/* A NUL-terminated name at offset in the string table section strings. */
static const char *nameAt(const struct Elf *elf, const Elf32_Shdr *strings, uint32_t offset)
{
    const char *names = elfRange(elf, strings->sh_offset, strings->sh_size, "string tables");
    if (offset >= strings->sh_size || memchr(names + offset, 0, strings->sh_size - offset) == NULL)
        fail("can't read %s: a name lies outside its string table", elf->path);
    return names + offset;
}

static int inRomx(uint32_t address)
{
    return address >= ROMX_START && address < ROMX_END;
}

/* Refuses a function symbol in ROMX (functions), or an $a or $t mapping symbol there (!functions). */
static void checkSymbols(const struct Elf *elf, const Elf32_Shdr *section, int functions)
{
    if (section->sh_type != SHT_SYMTAB)
        return;
    if (section->sh_entsize != sizeof(Elf32_Sym))
        fail("can't read %s: its symbol table has an unexpected entry size", elf->path);
    const Elf32_Shdr *strings = sectionHeader(elf, section->sh_link);
    const Elf32_Sym *table = elfRange(elf, section->sh_offset, section->sh_size, "symbol tables");
    size_t entries = section->sh_size / sizeof(Elf32_Sym);
    for (size_t i = 0; i < entries; i++)
    {
        const Elf32_Sym *sym = &table[i];
        if (sym->st_shndx == SHN_UNDEF || !inRomx(sym->st_value))
            continue;
        const char *name = nameAt(elf, strings, sym->st_name);
        unsigned type = ELF32_ST_TYPE(sym->st_info);
        if (functions && (type == STT_FUNC || type == STT_ARM_TFUNC))
            fail("function %s at 0x%08X is in the extended ROM; only data can go there.", name, sym->st_value & ~1u);
        if (!functions && name[0] == '$' && (name[1] == 'a' || name[1] == 't') && (name[2] == '\0' || name[2] == '.'))
            fail("there's code at 0x%08X in the extended ROM ($%c); only data can go there.", sym->st_value, name[1]);
    }
}

/*
 * Refuses code in ROMX (an executable section there, a function symbol, or an $a or $t mapping symbol), a writable
 * section there, and any section other than .romx with bytes there. Returns the .romx section, or fails when the ELF
 * has none or it's empty.
 */
static const Elf32_Shdr *findRomx(const struct Elf *elf)
{
    const Elf32_Shdr *romx = NULL;
    const Elf32_Shdr *sectionNames = sectionHeader(elf, elf->header->e_shstrndx);
    for (unsigned s = 0; s < elf->header->e_shnum; s++)
    {
        const Elf32_Shdr *section = sectionHeader(elf, s);
        const char *name = nameAt(elf, sectionNames, section->sh_name);
        if (!(section->sh_flags & SHF_ALLOC) || !inRomx(section->sh_addr))
            continue;
        if (section->sh_flags & SHF_EXECINSTR)
            fail("section %s at 0x%08X is code; only data can go in the extended ROM.", name, section->sh_addr);
        if (section->sh_flags & SHF_WRITE)
            fail("section %s at 0x%08X is writable; the extended ROM is read-only, so its data must be const.", name,
                 section->sh_addr);
        if (strcmp(name, ".romx") == 0 && section->sh_type == SHT_PROGBITS && section->sh_size != 0)
            romx = section;
        else if (section->sh_type != SHT_NOBITS && section->sh_size != 0)
            fail("section %s at 0x%08X is in the extended ROM, which holds only .romx.", name, section->sh_addr);
    }

    /* Functions first, then mapping symbols, so a function is named even where an $a or $t marks its code */
    for (int pass = 0; pass < 2; pass++)
    {
        for (unsigned s = 0; s < elf->header->e_shnum; s++)
            checkSymbols(elf, sectionHeader(elf, s), pass == 0);
    }

    if (romx == NULL)
        fail("%s has no .romx section; build without ADMG_ROMX to get a .gba.", elf->path);
    return romx;
}

/* Checks that the cartridge image holds every ROM segment of the ELF byte for byte after the cartridge header. */
static void checkCartridgeMatchesElf(const struct Elf *elf, const char *cartridgePath, const uint8_t *rom,
                                     size_t romSize)
{
    for (unsigned i = 0; i < elf->header->e_phnum; i++)
    {
        const Elf32_Phdr *p = programHeader(elf, i);
        if (p->p_type != PT_LOAD || p->p_filesz == 0 || p->p_paddr < ROM_START || p->p_paddr >= ROM_END)
            continue;
        uint32_t offset = p->p_paddr - ROM_START;
        const uint8_t *bytes = elfRange(elf, p->p_offset, p->p_filesz, "segments");
        for (uint32_t j = offset < CARTRIDGE_HEADER ? CARTRIDGE_HEADER - offset : 0; j < p->p_filesz; j++)
        {
            if ((uint64_t)offset + j >= romSize || rom[offset + j] != bytes[j])
                fail("%s doesn't match %s at 0x%08X; rebuild both.", cartridgePath, elf->path, p->p_paddr + j);
        }
    }
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        fputs("usage: admgext <elf> <cartridge> <out.admgrom>\n", stderr);
        return 2;
    }
    const char *elfPath = argv[1], *cartridgePath = argv[2], *outPath = argv[3];

    struct Elf elf = openElf(elfPath);
    const Elf32_Shdr *romx = findRomx(&elf);
    const uint8_t *segment = elfRange(&elf, romx->sh_offset, romx->sh_size, "sections");

    size_t cartridgeSize;
    uint8_t *cartridge = readFile(cartridgePath, &cartridgeSize);
    if (cartridgeSize > CARTRIDGE_SIZE || (cartridgeSize & 1) != 0)
        fail("%s is %zu bytes; the cartridge image must be at most 33554432 bytes (32 MiB) and an even number of "
             "bytes.", cartridgePath, cartridgeSize);
    checkCartridgeMatchesElf(&elf, cartridgePath, cartridge, cartridgeSize);

    /* The cartridge, the header and segment table, then the segment at the next 64 KiB boundary */
    uint32_t segmentOffset = CARTRIDGE_SIZE + SEGMENT_ALIGN;
    size_t outSize = (size_t)segmentOffset + romx->sh_size;
    uint8_t *out = calloc(outSize, 1);
    if (out == NULL)
        fail("can't write %s: out of memory", outPath);
    memset(out, 0xFF, CARTRIDGE_SIZE);
    memcpy(out, cartridge, cartridgeSize);

    uint8_t *header = out + CARTRIDGE_SIZE;
    memcpy(header, "ADMGEXT1", 8);
    put16(header + 8, FORMAT_VERSION);
    put16(header + 10, 1);
    put32(header + 12, crc32Of(out, CARTRIDGE_SIZE));
    uint8_t *entry = header + HEADER_SIZE;
    put32(entry, romx->sh_addr);
    put32(entry + 4, romx->sh_size);
    put32(entry + 8, segmentOffset);
    put32(entry + 12, FLAG_READ_ONLY);
    put32(header + 16, crc32Of(header, HEADER_SIZE + SEGMENT_ENTRY_SIZE));
    memcpy(out + segmentOffset, segment, romx->sh_size);

    writeFile(outPath, out, outSize);
    free(out);
    free(cartridge);
    return 0;
}
