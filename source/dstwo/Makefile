# - - - Modifiable paths - - -
DS2SDKPATH  := /opt/ds2sdk
CROSS       := /opt/mipsel-4.1.2-nopic/bin/mipsel-linux-

# - - - Libraries and includes - - -
FS_DIR       = $(DS2SDKPATH)/libsrc/fs
CONSOLE_DIR  = $(DS2SDKPATH)/libsrc/console
KEY_DIR      = $(DS2SDKPATH)/libsrc/key
ZLIB_DIR     = $(DS2SDKPATH)/libsrc/zlib
CORE_DIR     = $(DS2SDKPATH)/libsrc/core

LIBS         = $(DS2SDKPATH)/lib/libds2b.a -lc -lm -lgcc
EXTLIBS      = $(DS2SDKPATH)/lib/libds2a.a

INCLUDE      = -I. -I.. -I../mips -I$(DS2SDKPATH)/include -I$(FS_DIR)         \
               -I$(CONSOLE_DIR) -I$(KEY_DIR) -I$(ZLIB_DIR) -I$(CORE_DIR)

LINK_SPEC    = $(DS2SDKPATH)/specs/link.xn
START_ASM   := start.S
START_O     := start.o

# - - - Names - - -
NAME        := tempgba
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
C_SRC        = gpsp_main.c ../cpu_common.c ../cpu_asm.c ../video.c ../sha1.c  \
               gu.c ../memory.c ../sound.c ../input.c gui.c cheats.c ../bios.c\
               draw.c bdf_font.c ../zip.c bitmap.c ds2_main.c charsets.c      \
               ../stats.c guru_meditation.c port.c ds2sound.c
CPP_SRC      = 
ASM_SRC      = ../mips/stub.S port-asm.S
SRC          = $(C_SRC) $(CPP_SRC) $(ASM_SRC)
C_OBJS       = $(C_SRC:.c=.o)
CPP_OBJS     = $(CPP_SRC:.cpp=.o)
ASM_OBJS     = $(ASM_SRC:.S=.o)
OBJS         = $(C_OBJS) $(CPP_OBJS) $(ASM_OBJS) $(START_O)
HEADERS      = bdf_font.h bitmap.h charsets.h cheats.h ../sha1.h              \
               ../common.h ../cpu_common.h ../cpu.h draw.h gpsp_main.h gu.h   \
               gui.h ../input.h ../memory.h message.h ../mips/emit.h          \
               ../sound.h ../stats.h ../video.h ../zip.h port.h ds2sound.h

# - - - Compilation and linking flags - - -
DEFS         = -DNDS_LAYER -DMIPS_XBURST \
               -DGIT_VERSION=$(shell git describe --always)

CFLAGS       = -mips32 -mno-abicalls -fno-pic -fno-builtin -fno-exceptions    \
               -ffunction-sections -mno-long-calls -msoft-float -G 4          \
               -O3 -fomit-frame-pointer                                       \
               -fgcse-sm -fgcse-las -fgcse-after-reload                       \
               -fweb -fpeel-loops -fno-inline -fno-early-inlining             \
               $(DEFS) $(INCLUDE)

ASFLAGS      = $(CFLAGS) -D__ASSEMBLY__

LDFLAGS      = -nostdlib -static -T $(LINK_SPEC) -o $@ $(EXTLIBS) $(LIBS)

TARGET       = $(NAME).elf

DATA_TO_CLEAN= $(OUTPUT_DIR)/$(NAME).plg $(NAME).plg $(NAME).dat              \
               $(PLUGIN_DIR)/game_config.txt $(NAME).zip

.SUFFIXES: .elf .dat .plg .c .S .o

.PHONY: all release

all: $(OUTPUT_DIR)/$(NAME).plg

include ../Makefile.rules

release: all
	-rm -f $(NAME).zip
	cp ../../game_config.txt $(PLUGIN_DIR)
	cp ../../bios/gba_bios.bin $(PLUGIN_DIR)
	-mkdir $(PLUGIN_DIR)/GAMES
	-mkdir $(PLUGIN_DIR)/CHEATS
	-mkdir $(PLUGIN_DIR)/SAVES
	-mkdir $(PLUGIN_DIR)/PICS
	zip -r $(NAME).zip $(PLUGIN_DIR) $(OUTPUT_DIR)
	zip -j -r $(NAME).zip ../../doc ../../README.md

$(OUTPUT_DIR)/$(NAME).plg: $(TARGET)
	$(DS2SDKPATH)/tools/makeplug $< $@

# $< is the source (OUTPUT.elf); $@ is the target (OUTPUT.dat)
.elf.dat:
	$(OBJCOPY) -x -O binary $< $@

# The two below declarations ensure that editing a .c file recompiles only that
# file, but editing a .h file recompiles everything.
# Courtesy of Maarten ter Huurne.

# Each object file depends on its corresponding source file.
$(C_OBJS): %.o: %.c

# Object files all depend on all the headers.
$(OBJS): $(HEADERS)
