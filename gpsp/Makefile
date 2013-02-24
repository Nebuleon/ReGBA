# -x assembler-with-cpp
# gpSP makefile
# Gilead Kutnick - Exophase

# Global definitions

include ../Makefile.set
include ../build_count.inc

PSPSDK          = ${shell psp-config --pspsdk-path}
PREFIX          = ${shell psp-config --psp-prefix}

PSP_EBOOT_ICON  = ../image/up_gbsp_kai_logo01_trim.png
#PSP_EBOOT_ICON  = ../image/icon1.png
#文字列が表示されないので保留
PSP_EBOOT_PIC1  = ../image/up_gbsp_kai_launch01_s.png
#PSP_EBOOT_PIC1  = ../image/pic1.png 

OBJS            = main.o cpu_common.o cpu_asm.o cpu_c.o video.o memory.o sound.o input.o gu.o
OBJS           += gui.o zip.o cheats.o mips_stub.o fbm_print.o bios.o draw.o video_setting.o
OBJS           += pspDveManager.o homehook.o unicode.o

ifeq ($(USE_ADHOC), 1)
OBJS           += adhoc.o
COMMON_FILE += adhoc.h
endif

TARGET          = UO_gpSP_kai

PSP_FW_VERSION=300
VERSION_MODE = user

ifeq ($(TEST_MODE), 1)
VERSION_RELEASE = test
endif

VERSION_OPT = ${VERSION_RELEASE} ${VERSION_BUILD}

PSP_EBOOT_TITLE = UO gpSP kai ${VERSION_MAJOR}.${VERSION_MINOR} ${VERSION_OPT} Build ${BUILD_COUNT}

VPATH           += ..
CFLAGS          += -O3 -DPSP_BUILD -G0 -funsigned-char
CFLAGS          += -Wall
#CFLAGS          += -W -Wno-sign-compare
#CFLAGS          += -Wshadow -Wbad-function-cast -Wconversion -Wstrict-prototypes -Wmissing-declarations -Wmissing-prototypes -Wstrict-prototypes -Wnested-externs -Wredundant-decls 
CFLAGS          +=  -funsigned-char -ffast-math -fforce-addr -fmerge-all-constants -floop-optimize2 -funsafe-loop-optimizations -ftree-loop-linear

CFLAGS          += -DVERSION_MAJOR=${VERSION_MAJOR}
CFLAGS          += -DVERSION_MINOR=${VERSION_MINOR}
CFLAGS          += -DVERSION_BUILD=${VERSION_BUILD}
CFLAGS          += -DBUILD_COUNT=${BUILD_COUNT}

ifeq ($(TEST_MODE), 1)
CFLAGS          += -DTEST_MODE
endif

ifeq ($(USE_DEBUG), 1)
CFLAGS          += -DUSE_DEBUG
endif

ifeq ($(USE_ADHOC), 1)
CFLAGS          += -DUSE_ADHOC
endif

ifeq ($(USE_C_CORE), 1)
CFLAGS          += -DUSE_C_CORE
endif

ifeq ($(PSP_LARGE_MEMORY), 0)
CFLAGS          += -DUSE_EXT_MEM
endif

ASFLAGS         = ${CFLAGS}
EXTRA_TARGETS   = EBOOT.PBP

#LDFLAGS += -Wl,-Map gpSP.map

LIBS            += -lpspgu -lpsprtc -lpspaudio -lz -lpsppower -lpspkubridge -lpspgum -lpspsystemctrl_user -lm

ifdef USE_ADHOC
LIBS            += -lpspnet -lpspwlan -lpspnet_adhoc -lpspnet_adhocctl -lpspnet_adhocmatching
endif

include ${PSPSDK}/lib/build.mak

SRC_DIR = ../src/

COMMON_FILE       = ${SRC_DIR}common.h \
                                 ${SRC_DIR}cpu.h \
                                 ${SRC_DIR}memory.h \
                                 ${SRC_DIR}video.h \
                                 ${SRC_DIR}gu.h \
                                 ${SRC_DIR}input.h \
                                 ${SRC_DIR}sound.h \
                                 ${SRC_DIR}main.h \
                                 ${SRC_DIR}gui.h \
                                 ${SRC_DIR}zip.h \
                                 ${SRC_DIR}cheats.h \
                                 ${SRC_DIR}unicode.h

UI_COMMON_FILE  = ${SRC_DIR}fbm_print.h \
                                 ${SRC_DIR}draw.h \
                                 ${SRC_DIR}message.h

main.o			: ${COMMON_FILE} ${UI_COMMON_FILE} ${SRC_DIR}main.c
cpu_common.o	: ${COMMON_FILE} ${SRC_DIR}cpu_common.c
cpu_asm.o		: ${COMMON_FILE} ${SRC_DIR}cpu_asm.c
cpu_c.o			: ${COMMON_FILE} ${SRC_DIR}cpu_c.c
video.o			: ${COMMON_FILE} ${SRC_DIR}video.c 
gu.o			: ${COMMON_FILE} ${SRC_DIR}dvemgr.h ${SRC_DIR}gu.c 
memory.o		: ${COMMON_FILE} ${SRC_DIR}memory.c
sound.o			: ${COMMON_FILE} ${SRC_DIR}sound.c
input.o			: ${COMMON_FILE} ${UI_COMMON_FILE} ${SRC_DIR}homehook.h ${SRC_DIR}input.c
gui.o			: ${COMMON_FILE} ${UI_COMMON_FILE} ${SRC_DIR}homehook.h ${SRC_DIR}gui.c
zip.o			: ${COMMON_FILE} ${SRC_DIR}zip.c
cheats.o		: ${COMMON_FILE} ${SRC_DIR}cheats.c
mips_stub.o		: ${SRC_DIR}mips_stub.S
fbm_print.o		: ${COMMON_FILE} ${UI_COMMON_FILE} ${SRC_DIR}fbm_print.c
bios.o			: ${COMMON_FILE} ${SRC_DIR}bios.c ${SRC_DIR}bios.h
draw.o			: ${COMMON_FILE} ${UI_COMMON_FILE} ${SRC_DIR}draw.c ${SRC_DIR}draw.h
pspDveManager.o : ${SRC_DIR}pspDveManager.S
homehook.o		: ${SRC_DIR}homehook.S
video_setting.o	: ${COMMON_FILE} ${SRC_DIR}video_setting.c
unicode.o		: ${COMMON_FILE} ${SRC_DIR}unicode.c

ifdef USE_ADHOC
adhoc.o					: ${COMMON_FILE} ${SRC_DIR}adhoc.c ${SRC_DIR}adhoc.h
endif

%.o: ../src/%.c
	@echo Compiling $<...
	@$(CC) $(CDEFS) $(CFLAGS) -c $< -o$@

%.o: ../src/%.S
	@echo Assembling $<...
	@$(CC) $(CDEFS) $(CFLAGS) -c $< -o$@

%.o: ../src/%.s
	@echo Assembling $<...
	@$(AS) $(ASDEFS) $(ASFLAGS) -c $< -o$@

%.a:
	@echo Archiving $@...
	@$(AR) -r $@ $^

all:
	rm ./PARAM.SFO
	rm ./gui.o
	cp ./*.prx ../obj/
	cp ./*.PBP ../obj/
