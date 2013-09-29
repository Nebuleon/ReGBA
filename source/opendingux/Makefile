TARGET      := regba

CROSS_COMPILE ?= mipsel-linux-
CC          := $(CROSS_COMPILE)gcc
STRIP       := $(CROSS_COMPILE)strip

SYSROOT     := $(shell $(CC) --print-sysroot)
SDL_CFLAGS  := $(shell $(SYSROOT)/usr/bin/sdl-config --cflags)
SDL_LIBS    := $(shell $(SYSROOT)/usr/bin/sdl-config --libs)

OBJS        := main.o draw.o port.o port-asm.o gcwinput.o ../video.o          \
              ../input.o ../bios.o ../zip.o ../sound.o ../mips/stub.o         \
              ../stats.o ../memory.o ../cpu_common.o ../cpu_asm.o od-sound.o  \
              ../sha1.o imageio.o ../unifont.o gui.o
              
HEADERS     := cheats.h ../common.h ../cpu_common.h ../cpu.h draw.h main.h    \
               ../input.h ../memory.h message.h ../mips/emit.h ../sound.h     \
               ../stats.h ../video.h ../zip.h port.h od-sound.h ../sha1.h     \
               imageio.h ../unifont.h od-input.h

INCLUDE     := -I. -I.. -I../mips
DEFS        := -DMIPS_XBURST
HAS_MIPS32R2 := $(shell echo | $(CC) -dM -E - |grep _MIPS_ARCH_MIPS32R2)
ifneq ($(HAS_MIPS32R2),)
	DEFS += -DMIPS_32R2
endif

CFLAGS       = $(SDL_CFLAGS) -mno-abicalls -Wall -Wno-unused-variable \
               -O2 -fomit-frame-pointer $(DEFS) $(INCLUDE)
ASFLAGS      = $(CFLAGS) -D__ASSEMBLY__
LDFLAGS     := $(SDL_LIBS) -lpthread -lz -lm -lpng

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
	$(CMD)cp ../../bios/gba_bios.bin .opk_data
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
