# - - - Modifiable paths - - -
DS2SDKPATH  := /opt/ds2sdk
CROSS       := /opt/mipsel-4.1.2-nopic/bin/mipsel-linux-

# - - - Libraries and includes - - -
FS_DIR       = $(DS2SDKPATH)/libsrc/fs
CONSOLE_DIR  = $(DS2SDKPATH)/libsrc/console
KEY_DIR      = $(DS2SDKPATH)/libsrc/key
ZLIB_DIR     = $(DS2SDKPATH)/libsrc/zlib
CORE_DIR     = $(DS2SDKPATH)/libsrc/core

LIBS        := $(DS2SDKPATH)/lib/libds2b.a -lc -lm -lgcc
EXTLIBS     := $(DS2SDKPATH)/lib/libds2a.a

INCLUDE     := -I. -I.. -I../mips -I$(DS2SDKPATH)/include \
               -I$(FS_DIR) -I$(CONSOLE_DIR) -I$(KEY_DIR) -I$(ZLIB_DIR) \
               -I$(CORE_DIR)

LINK_SPEC   := $(DS2SDKPATH)/specs/link.xn
START_ASM   := start.S
START_O     := start.o

# - - - Names - - -
OUTPUT      := tempgba
OUTPUT_DIR  := _dstwoplug
PLUGIN_DIR  := TEMPGBA

# - - - Tools - - -
CC           = $(CROSS)gcc
AR           = $(CROSS)ar rcsv
LD           = $(CROSS)ld
OBJCOPY      = $(CROSS)objcopy
NM           = $(CROSS)nm
OBJDUMP      = $(CROSS)objdump

# - - - Sources, objects and headers - - -
C_SOURCES    = gpsp_main.c ../cpu_common.c ../cpu_asm.c ../video.c            \
               gu.c ../memory.c sound.c ../input.c gui.c cheats.c ../bios.c   \
               draw.c bdf_font.c ../zip.c bitmap.c ds2_main.c charsets.c      \
               ../stats.c guru_meditation.c port.c
# cpu_c.c
CPP_SOURCES  = 
ASM_SOURCES  = ../mips/stub.S port-asm.S
SOURCES      = $(C_SOURCES) $(CPP_SOURCES) $(ASM_SOURCES)
C_OBJECTS    = $(C_SOURCES:.c=.o)
CPP_OBJECTS  = $(CPP_SOURCES:.cpp=.o)
ASM_OBJECTS  = $(ASM_SOURCES:.S=.o)
OBJECTS      = $(C_OBJECTS) $(CPP_OBJECTS) $(ASM_OBJECTS)
HEADERS      = bdf_font.h bitmap.h charsets.h cheats.h                        \
               ../common.h ../cpu_common.h ../cpu.h draw.h gpsp_main.h gu.h   \
               gui.h ../input.h ../memory.h message.h ../mips/emit.h sound.h  \
               ../stats.h ../video.h ../zip.h port.h

# - - - Compilation flags - - -
CFLAGS := -mips32 -mno-abicalls -fno-pic -fno-builtin \
	      -fno-exceptions -ffunction-sections -mno-long-calls \
	      -msoft-float -G 4 \
          -O3 -fomit-frame-pointer -fgcse-sm -fgcse-las -fgcse-after-reload \
          -fweb -fpeel-loops -fno-inline -fno-early-inlining

DEFS   := -DNDS_LAYER -DMIPS_XBURST -DGIT_VERSION=$(shell git describe --always)
# Usable flags are
# -DTEST_MODE
# -DUSE_DEBUG
# -DUSE_C_CORE
# -DMIPS_XBURST, -DMIPS_32R2 (explained in mips_stub.S)
# -DPERFORMANCE_IMPACTING_STATISTICS (if you want to get statistics that are
#   so expensive to collect that the emulator's performance would plummet)
# -DTRACE, -DTRACE_FRAMESKIP, -DTRACE_TRANSLATION_REQUESTS
# -DTRACE_RECOMPILATION, -DTRACE_REUSE, -DTRACE_FLUSHING
# -TRACE_SOUND

.PHONY: clean makedirs
.SUFFIXES: .elf .dat .plg .c .S .o

all: $(OUTPUT_DIR)/$(OUTPUT).plg

release: all makedirs
	-rm -f $(OUTPUT).zip
	zip -r $(OUTPUT).zip $(PLUGIN_DIR) $(OUTPUT_DIR)
	zip -j -r $(OUTPUT).zip ../../doc ../../README.md

# $< is the source (OUTPUT.dat); $@ is the target (OUTPUT.plg)
$(OUTPUT_DIR)/$(OUTPUT).plg: $(OUTPUT).dat
	$(DS2SDKPATH)/tools/makeplug $< $@

# $< is the source (OUTPUT.elf); $@ is the target (OUTPUT.dat)
.elf.dat:
	$(OBJCOPY) -x -O binary $< $@

$(OUTPUT).elf: Makefile $(OBJECTS) $(START_O) $(LINK_SPEC) $(EXTLIBS)
	$(CC) -nostdlib -static -T $(LINK_SPEC) -o $@ $(START_O) $(OBJECTS) $(EXTLIBS) $(LIBS)

$(EXTLIBS):
	$(MAKE) -C $(DS2SDKPATH)/source/

$(START_O): $(START_ASM)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

makedirs:
	-mkdir $(PLUGIN_DIR)/GAMES
	-mkdir $(PLUGIN_DIR)/CHEATS
	-mkdir $(PLUGIN_DIR)/SAVES
	-mkdir $(PLUGIN_DIR)/PICS

clean:
	-rm -rf $(OUTPUT).plg $(OUTPUT).dat $(OUTPUT).elf depend $(OBJECTS) $(START_O)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) $(DEFS) -o $@ -c $<
.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDE) $(DEFS) -fno-rtti -o $@ -c $<
.S.o:
	$(CC) $(CFLAGS) $(INCLUDE) $(DEFS) -D__ASSEMBLY__ -o $@ -c $<

# The two below declarations ensure that editing a .c file recompiles only that
# file, but editing a .h file recompiles everything.
# Courtesy of Maarten ter Huurne.

# Each object file depends on its corresponding source file.
$(C_OBJECTS): %.o: %.c

# Object files all depend on all the headers.
$(OBJECTS): $(HEADERS)
