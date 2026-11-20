CC ?= gcc
.PHONY: all clean

SRCS = admgsym.c miniz.c

# miniz's zlib-style compression and CRC-32 only: no ZIP archives, file times or stdio.
CFLAGS += -O2 -DMINIZ_NO_ARCHIVE_APIS -DMINIZ_NO_TIME -DMINIZ_NO_STDIO -DMINIZ_NO_ZLIB_COMPATIBLE_NAMES

ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

all: admgsym$(EXE)
	@:

admgsym$(EXE): $(SRCS) miniz.h elf.h
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)

clean:
	$(RM) admgsym admgsym.exe
