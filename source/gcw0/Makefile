TARGET      := regba

CROSS_COMPILE ?= mipsel-linux-
CC          := $(CROSS_COMPILE)gcc
STRIP       := $(CROSS_COMPILE)strip

SYSROOT     := $(shell $(CC) --print-sysroot)
SDL_CFLAGS  := $(shell $(SYSROOT)/usr/bin/sdl-config --cflags)
SDL_LIBS    := $(shell $(SYSROOT)/usr/bin/sdl-config --libs)

OBJS        := main.o draw.o port.o port-asm.o gcwinput.o ../video.o          \
              ../input.o ../bios.o ../zip.o ../sound.o ../mips/stub.o         \
              ../stats.o ../memory.o ../cpu_common.o ../cpu_asm.o
              
HEADERS     := cheats.h ../common.h ../cpu_common.h ../cpu.h draw.h main.h    \
               ../input.h ../memory.h message.h ../mips/emit.h ../sound.h     \
               ../stats.h ../video.h ../zip.h port.h

INCLUDE     := -I. -I.. -I../mips
DEFS        := -DGAMEPAK_FITS_IN_RAM -DMIPS_XBURST -DMIPS_32R2
CFLAGS       = $(SDL_CFLAGS) -mno-abicalls -Wall -Wno-unused-variable \
               -O2 -fomit-frame-pointer $(DEFS) $(INCLUDE)
ASFLAGS      = $(CFLAGS) -D__ASSEMBLY__
LDFLAGS     := $(SDL_LIBS) -lpthread -lz -lm

DATA_TO_CLEAN := .opk_data $(TARGET).opk

include ../Makefile.rules

.PHONY: all opk

all: $(TARGET)

opk: $(TARGET).opk

$(TARGET).opk: $(TARGET)
	$(SUM) "  OPK     $@"
	$(CMD)rm -rf .opk_data
	$(CMD)cp -r data .opk_data
	$(CMD)cp ../../game_config.txt .opk_data
	$(CMD)cp $< .opk_data/regba
	$(CMD)$(STRIP) .opk_data/regba
	$(CMD)mksquashfs .opk_data $@ -all-root -noappend -no-exports -no-xattrs -no-progress >/dev/null

# The two below declarations ensure that editing a .c file recompiles only that
# file, but editing a .h file recompiles everything.
# Courtesy of Maarten ter Huurne.

# Each object file depends on its corresponding source file.
$(C_OBJS): %.o: %.c

# Object files all depend on all the headers.
$(OBJS): $(HEADERS)
