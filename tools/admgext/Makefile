CC ?= gcc
.PHONY: all clean

# The ELF definitions and miniz (for its CRC-32) are admgsym's, so the fork carries one copy of each.
ADMGSYM_DIR = ../admgsym
SRCS = admgext.c $(ADMGSYM_DIR)/miniz.c

# miniz's CRC-32 only: no ZIP archives, file times or stdio.
CFLAGS += -O2 -I$(ADMGSYM_DIR) -DMINIZ_NO_ARCHIVE_APIS -DMINIZ_NO_TIME -DMINIZ_NO_STDIO -DMINIZ_NO_ZLIB_COMPATIBLE_NAMES

ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

all: admgext$(EXE)
	@:

admgext$(EXE): $(SRCS) $(ADMGSYM_DIR)/miniz.h $(ADMGSYM_DIR)/elf.h
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)

clean:
	$(RM) admgext admgext.exe
