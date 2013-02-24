#gba.mk

GBADIR = $(TOP)/gpsp/src

include $(GBADIR)/../Makefile.set

PSP_EBOOT_ICON  = ../image/up_gbsp_kai_logo01_trim.png
#PSP_EBOOT_ICON  = ../image/icon1.png
#猟忖双が燕幣されないので隠藻
PSP_EBOOT_PIC1  = ../image/up_gbsp_kai_launch01_s.png
#PSP_EBOOT_PIC1  = ../image/pic1.png 

PSP_FW_VERSION=300
VERSION_MODE = user

ifeq ($(TEST_MODE), 1)
VERSION_RELEASE = test
endif

VERSION_OPT = ${VERSION_RELEASE} ${VERSION_BUILD}

PSP_EBOOT_TITLE = UO gpSP kai ${VERSION_MAJOR}.${VERSION_MINOR} ${VERSION_OPT} Build ${BUILD_COUNT}

SOURCES += $(GBADIR)/gpsp_main.c    \
           $(GBADIR)/cpu_common.c   \
           $(GBADIR)/cpu_asm.c      \
            $(GBADIR)/cpu_c.c       \
            $(GBADIR)/video.c       \
            $(GBADIR)/gu.c          \
            $(GBADIR)/memory.c      \
            $(GBADIR)/sound.c       \
            $(GBADIR)/input.c       \
            $(GBADIR)/gui.c         \
            $(GBADIR)/cheats.c      \
            $(GBADIR)/mips_stub.S   \
            $(GBADIR)/bios.c        \
            $(GBADIR)/draw.c        \
            $(GBADIR)/bdf_font.c    \
            $(GBADIR)/unicode.c     \
            $(GBADIR)/zip.c         \


ifeq ($(TEST_MODE), 1)
CFLAGS          += -DTEST_MODE
endif

ifeq ($(USE_DEBUG), 1)
CFLAGS          += -DUSE_DEBUG
endif

ifeq ($(USE_C_CORE), 1)
CFLAGS          += -DUSE_C_CORE
endif

#CFLAGS          += -Wshadow -Wbad-function-cast -Wconversion -Wstrict-prototypes -Wmissing-declarations -Wmissing-#prototypes -Wstrict-prototypes -Wnested-externs -Wredundant-decls 
#CFLAGS          +=  -funsigned-char -ffast-math -fforce-addr -fmerge-all-constants -floop-optimize2 -funsafe-loop-#optimizations -ftree-loop-linear

CFLAGS          += -DVERSION_MAJOR=${VERSION_MAJOR}
CFLAGS          += -DVERSION_MINOR=${VERSION_MINOR}
CFLAGS          += -DVERSION_BUILD=${VERSION_BUILD}
CFLAGS          += -DBUILD_COUNT=${BUILD_COUNT}

CFLAGS	+= -I$(GBADIR)
VPATH   += $(GBADIR)

