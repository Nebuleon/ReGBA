TARGET := regba

CROSS_COMPILE ?= mipsel-linux-
CC := $(CROSS_COMPILE)gcc
STRIP := $(CROSS_COMPILE)strip

SYSROOT := $(shell $(CC) --print-sysroot)
SDL_CFLAGS := $(shell $(SYSROOT)/usr/bin/sdl-config --cflags)
SDL_LIBS := $(shell $(SYSROOT)/usr/bin/sdl-config --libs)

OBJS := main.o draw.o port.o port-asm.o ../video.o ../input.o ../bios.o \
	../zip.o ../sound.o ../mips/stub.o ../stats.o ../memory.o \
	../cpu_common.o ../cpu_asm.o

INC := -I. -I.. -I../mips
CFLAGS := $(SDL_CFLAGS) $(INC) \
	-DZAURUS -DMIPS_XBURST -DMIPS_32R2 \
	-Wall -Wno-unused-variable -O2 -fomit-frame-pointer
ASFLAGS := $(CFLAGS)
LDFLAGS := $(SDL_LIBS) -lpthread -lz -lm

DATA_TO_CLEAN := .opk_data $(TARGET).opk

include ../Makefile.rules

.PHONY: all opk

all: $(TARGET)

opk: $(TARGET).opk

$(TARGET).opk: $(TARGET)
	$(SUM) "  OPK     $@"
	$(CMD)rm -rf .opk_data
	$(CMD)cp -r data .opk_data
	$(CMD)cp $< .opk_data/regba
	$(CMD)$(STRIP) .opk_data/regba
	$(CMD)mksquashfs .opk_data $@ -all-root -noappend -no-exports -no-xattrs -no-progress >/dev/null
