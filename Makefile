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

INCLUDE     := -Inds -I$(DS2SDKPATH)/include \
               -I$(FS_DIR) -I$(CONSOLE_DIR) -I$(KEY_DIR) -I$(ZLIB_DIR) \
               -I$(CORE_DIR)

LINK_SPEC   := $(DS2SDKPATH)/specs/link.xn
START_ASM   := $(DS2SDKPATH)/specs/start.S
START_O     := start.o

# - - - Names - - -
# Two emulator plugins are produced by this Makefile.
# One runs Pokémon games without desynchronising some parts of the music
# with the others. That's PokeGBA.
# The other runs Golden Sun - The Lost Age and some GBA Video cartridges
# without crappy sound. That's TempGBA.
# Both are placed in the release zip file.
PLUGIN_DIR  := TEMPGBA
RELEASE     := tempgba
OUTPUT1     := tempgba
OUTPUT2     := pokegba

# - - - Tools - - -
CC           = $(CROSS)gcc
AR           = $(CROSS)ar rcsv
LD           = $(CROSS)ld
OBJCOPY      = $(CROSS)objcopy
NM           = $(CROSS)nm
OBJDUMP      = $(CROSS)objdump

# - - - Sources and objects - - -
C_SOURCES    = source/nds/gpsp_main.c    \
               source/nds/cpu_common.c   \
               source/nds/cpu_asm.c      \
               source/nds/video.c       \
               source/nds/gu.c          \
               source/nds/memory.c      \
               source/nds/sound.c       \
               source/nds/input.c       \
               source/nds/gui.c         \
               source/nds/cheats.c      \
               source/nds/bios.c        \
               source/nds/draw.c        \
               source/nds/bdf_font.c    \
               source/nds/zip.c         \
               source/nds/bitmap.c      \
               source/nds/ds2_main.c    \
               source/nds/charsets.c    \
               source/nds/stats.c
# source/nds/cpu_c.c
CPP_SOURCES  = 
ASM_SOURCES  = source/nds/mips_stub.S
SOURCES      = $(C_SOURCES) $(CPP_SOURCES) $(ASM_SOURCES)

C_OBJECTS1   = $(C_SOURCES:.c=.1o)
CPP_OBJECTS1 = $(CPP_SOURCES:.cpp=.1o)
ASM_OBJECTS1 = $(ASM_SOURCES:.S=.1o)
OBJECTS1     = $(C_OBJECTS1) $(CPP_OBJECTS1) $(ASM_OBJECTS1)

C_OBJECTS2   = $(C_SOURCES:.c=.2o)
CPP_OBJECTS2 = $(CPP_SOURCES:.cpp=.2o)
ASM_OBJECTS2 = $(ASM_SOURCES:.S=.2o)
OBJECTS2     = $(C_OBJECTS2) $(CPP_OBJECTS2) $(ASM_OBJECTS2)

# - - - Compilation flags - - -
CFLAGS := -mips32 -mno-abicalls -fno-pic -fno-builtin \
	      -fno-exceptions -ffunction-sections -mno-long-calls \
	      -msoft-float -G 4 \
          -O3 -fomit-frame-pointer -fgcse-sm -fgcse-las -fgcse-after-reload \
          -fweb -fpeel-loops
CFLAGS1 = $(CFLAGS)
CFLAGS2 = $(CFLAGS)

DEFS   := -DNDS_LAYER -DNO_LOAD_DELAY_SLOT
DEFS1   = $(DEFS)
DEFS2   = $(DEFS) -DPOKEMON
# Usable flags are
# -DTEST_MODE
# -DUSE_DEBUG
# -DUSE_C_CORE
# -DNO_LOAD_DELAY_SLOT (for the XBurst architecture, which has no load delay
#   slots)

.PHONY: clean makedirs
.SUFFIXES: .elf .dat .plg .c .S .1o .2o

all: $(OUTPUT1).plg $(OUTPUT2).plg makedirs

release: all
	-rm -f $(RELEASE).zip
	zip -r $(RELEASE).zip $(PLUGIN_DIR) $(OUTPUT1).plg $(OUTPUT1).bmp $(OUTPUT1).ini $(OUTPUT2).plg $(OUTPUT2).bmp $(OUTPUT2).ini copyright installation.txt README.md source.txt

# $< is the source (OUTPUT.dat); $@ is the target (OUTPUT.plg)
.dat.plg:
	$(DS2SDKPATH)/tools/makeplug $< $@

# $< is the source (OUTPUT.elf); $@ is the target (OUTPUT.dat)
.elf.dat:
	$(OBJCOPY) -x -O binary $< $@

$(OUTPUT1).elf: Makefile $(OBJECTS1) $(START_O) $(LINK_SPEC) $(EXTLIBS)
	$(CC) -nostdlib -static -T $(LINK_SPEC) -o $@ $(START_O) $(OBJECTS1) $(EXTLIBS) $(LIBS)

$(OUTPUT2).elf: Makefile $(OBJECTS2) $(START_O) $(LINK_SPEC) $(EXTLIBS)
	$(CC) -nostdlib -static -T $(LINK_SPEC) -o $@ $(START_O) $(OBJECTS2) $(EXTLIBS) $(LIBS)

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
	-rm -rf $(OUTPUT1).plg $(OUTPUT1).dat $(OUTPUT1).elf $(OUTPUT2).plg $(OUTPUT2).dat $(OUTPUT2).elf depend $(OBJECTS) $(START_O)

.c.1o:
	$(CC) $(CFLAGS1) $(INCLUDE) $(DEFS1) -o $@ -c $<
.cpp.1o:
	$(CC) $(CFLAGS1) $(INCLUDE) $(DEFS1) -fno-rtti -o $@ -c $<
.S.1o:
	$(CC) $(CFLAGS1) $(INCLUDE) $(DEFS1) -D__ASSEMBLY__ -o $@ -c $<

.c.2o:
	$(CC) $(CFLAGS2) $(INCLUDE) $(DEFS2) -o $@ -c $<
.cpp.2o:
	$(CC) $(CFLAGS2) $(INCLUDE) $(DEFS2) -fno-rtti -o $@ -c $<
.S.2o:
	$(CC) $(CFLAGS2) $(INCLUDE) $(DEFS2) -D__ASSEMBLY__ -o $@ -c $<

Makefile: depend

depend: $(SOURCES)
	$(CC) -MM $(CFLAGS) $(INCLUDE) $(DEFS) $(SOURCES) > $@
	touch Makefile

-include depend
