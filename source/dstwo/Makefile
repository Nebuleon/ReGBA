ifeq ($(strip $(SCDS2_TOOLS)),)
$(error "Please set SCDS2_TOOLS in your environment. export SCDS2_TOOLS=<path to scds2 tools>")
endif

include $(SCDS2_TOOLS)/scds2_rules

# libds2 has 'open' which is needed by libz; put libds2 after libz
LIBS     := -lz -lds2

INCLUDE     := -I. -I.. -I../mips

# - - - Names - - -
NAME        := tempgba
OUTPUT_DIR  := _dstwoplug
PLUGIN_DIR  := TEMPGBA

# - - - Sources, objects and headers - - -
C_SRC       := gpsp_main.c ../cpu_common.c ../cpu_asm.c ../video.c ../sha1.c  \
               gu.c ../memory.c ../sound.c ../input.c gui.c ../bios.c         \
               draw.c bdf_font.c bitmap.c ds2_main.c                          \
               ../stats.c port.c ds2sound.c ds2memory.c ../zip.c
# TODO Add these back: cheats.c charsets.c
ASM_SRC     := ../mips/stub.S port-asm.S
SRC         := $(C_SRC) $(ASM_SRC)
C_OBJS      := $(C_SRC:.c=.o)
ASM_OBJS    := $(ASM_SRC:.S=.o)
OBJS        := $(C_OBJS) $(ASM_OBJS)
HEADERS     := bdf_font.h bitmap.h ../sha1.h                                  \
               ../common.h ../cpu_common.h ../cpu.h draw.h gpsp_main.h gu.h   \
               gui.h ../input.h ../memory.h message.h ../mips/emit.h          \
               ../sound.h ../stats.h ../video.h port.h ds2sound.h ../zip.h
# TODO Add these back: cheats.h charsets.h

# - - - Compilation and linking flags - - -
DEFS        := -DMIPS_XBURST -DGIT_VERSION=$(shell git describe --always)

CFLAGS      += -fno-builtin -ffunction-sections -O3 -G 0                      \
               -fno-inline -fno-early-inlining                                \
               $(DEFS) $(INCLUDE)

ASFLAGS     += -G 0 $(DEFS) $(INCLUDE)

LDFLAGS     +=

TARGET      := $(NAME).elf

DATA_TO_CLEAN := $(OUTPUT_DIR)/$(NAME).plg $(NAME).plg $(NAME).dat            \
               $(NAME).pak $(PLUGIN_DIR)/game_config.txt $(NAME).zip

.PHONY: all release clean

all: $(NAME).plg

release: all
	-rm -f $(NAME).zip
	cp $(NAME).plg $(OUTPUT_DIR)
	cp ../../game_config.txt $(PLUGIN_DIR)
	cp ../../bios/gba_bios.bin $(PLUGIN_DIR)
	-mkdir $(PLUGIN_DIR)/GAMES
	-mkdir $(PLUGIN_DIR)/CHEATS
	-mkdir $(PLUGIN_DIR)/SAVES
	-mkdir $(PLUGIN_DIR)/PICS
	zip -r $(NAME).zip $(PLUGIN_DIR) $(OUTPUT_DIR)
	zip -j -r $(NAME).zip ../../doc ../../README.md

$(NAME).plg: $(NAME).pak

$(NAME).elf: $(OBJS)

clean:
	-rm -rf $(OBJS) $(TARGET) $(DATA_TO_CLEAN)

# The two below declarations ensure that editing a .c file recompiles only that
# file, but editing a .h file recompiles everything.
# Courtesy of Maarten ter Huurne.

# Each object file depends on its corresponding source file.
$(C_OBJS): %.o: %.c

# Object files all depend on all the headers.
$(OBJS): $(HEADERS)
