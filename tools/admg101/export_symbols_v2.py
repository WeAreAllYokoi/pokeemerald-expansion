"""Export GBARecomp symbol JSON version 2 from a registry-capable build's ELF.

Version 2 matches GBARecomp's ELF reader (Symbol.ReadELF and Relocations.ReadELF) record for record:
the same symbols in the same order, data objects keeping their size (is_object), and every R_ARM_ABS32
relocation the linker kept with --emit-relocs, as {place, symbol, target}: where the ROM image holds
the word (data the game copies into RAM is found at its ROM copy), the unique non-section symbol it was
computed from or null, and the word itself. Section contents are never exported.

python -B tools/admg101/export_symbols_v2.py --elf pokeemerald.elf --output symbols.json
"""
import argparse
import json
import struct
from pathlib import Path

SHT_SYMTAB, SHT_NOBITS, SHT_REL = 2, 8, 9
SHF_ALLOC = 2
PT_LOAD = 1
STT_OBJECT, STT_FUNC, STT_SECTION, STT_FILE = 1, 2, 3, 4
R_ARM_ABS32 = 2
ROM_BASE = 0x08000000


def read(elf_bytes):
    if elf_bytes[:6] != b"\x7fELF\x01\x01":
        raise ValueError("expected a 32-bit little-endian ELF")
    shoff = struct.unpack_from("<I", elf_bytes, 0x20)[0]
    shentsize, shnum = struct.unpack_from("<HH", elf_bytes, 0x2E)
    phoff = struct.unpack_from("<I", elf_bytes, 0x1C)[0]
    phentsize, phnum = struct.unpack_from("<HH", elf_bytes, 0x2A)
    sections = [struct.unpack_from("<10I", elf_bytes, shoff + i * shentsize) for i in range(shnum)]
    loads = []
    for i in range(phnum):
        p_type, _, vaddr, paddr, filesz, _, _, _ = struct.unpack_from("<8I", elf_bytes, phoff + i * phentsize)
        if p_type == PT_LOAD:
            loads.append((vaddr, paddr, filesz))

    def symbol_table(index):
        sh = sections[index]
        names = elf_bytes[sections[sh[6]][4]:sections[sh[6]][4] + sections[sh[6]][5]]
        rows = []
        for offset in range(sh[4], sh[4] + sh[5], sh[9]):
            name_offset, value, size, info, _, shndx = struct.unpack_from("<IIIBBH", elf_bytes, offset)
            name = names[name_offset:names.index(0, name_offset)].decode("utf-8", errors="replace")
            rows.append((name, value, size, info & 15, shndx, (info >> 4) == 0))
        return rows

    symbols = []
    for index, sh in enumerate(sections):
        if sh[1] != SHT_SYMTAB:
            continue
        for name, value, size, kind, shndx, _ in symbol_table(index):
            if not name or name.startswith("$") or shndx in (0, 0xFFF1):
                continue
            if kind == STT_FUNC:
                symbols.append({"address": value & ~1, "size": size, "name": name, "is_thumb": bool(value & 1), "is_object": False})
            elif kind == STT_OBJECT:
                symbols.append({"address": value, "size": size, "name": name, "is_thumb": None, "is_object": True})
            else:
                symbols.append({"address": value, "size": 0, "name": name, "is_thumb": None, "is_object": False})

    def load_address(address):
        for vaddr, paddr, filesz in loads:
            if vaddr <= address and address - vaddr + 4 <= filesz:
                place = paddr + (address - vaddr)
                return place if place >= ROM_BASE else None
        return None

    relocations = []
    tables = {}
    for sh in sections:
        if sh[1] != SHT_REL:
            continue
        target = sections[sh[7]]
        if not (target[2] & SHF_ALLOC) or target[1] == SHT_NOBITS:
            continue
        if sh[6] not in tables:
            rows = symbol_table(sh[6])
            counts = {}
            for row in rows:
                if not (row[5] and row[3] in (STT_SECTION, STT_FILE)):
                    counts[row[0]] = counts.get(row[0], 0) + 1
            tables[sh[6]] = (rows, {n for n, c in counts.items() if c == 1})
        rows, unique = tables[sh[6]]
        for entry in range(sh[4], sh[4] + sh[5], max(sh[9], 8)):
            offset, info = struct.unpack_from("<II", elf_bytes, entry)
            if info & 0xFF != R_ARM_ABS32:
                continue
            if offset < target[3] or offset + 4 > target[3] + target[5]:
                raise ValueError(f"relocation at 0x{offset:08X} is outside its section")
            value = struct.unpack_from("<I", elf_bytes, target[4] + offset - target[3])[0]
            name, _, _, kind, shndx, _ = rows[info >> 8]
            if shndx in (0, 0xFFF1):
                continue
            place = load_address(offset)
            if place is None:
                continue
            symbol = None if kind in (STT_SECTION, STT_FILE) or not name or name not in unique else name
            relocations.append({"place": place, "symbol": symbol, "target": value})
    relocations.sort(key=lambda r: r["place"])
    return {"format": "gbarecomp-symbols", "format_version": 2, "symbols": symbols, "relocations": relocations}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    document = read(args.elf.read_bytes())
    args.output.write_text(json.dumps(document, separators=(",", ":"), ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({"symbols": len(document["symbols"]), "relocations": len(document["relocations"])}))


if __name__ == "__main__":
    main()
