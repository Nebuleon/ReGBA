# 
# 
# Author: Seeger Chin
# e-mail: seeger.chin@gmail.com
# 
# Copyright (C) 2006 Ingenic Semiconductor Inc.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
# 
# 

# select which module to compile, set value to 1 for the module to compile, 0 for the module not to compile
# -----------------------------
# CAMERA dependent on JPEG
# LWIP dependent on UCFS
# UCGUI denpedent on JPEG
# UCFS denpedent on MMC
# JPEG dependent on UCFS

CAMERA = 1
LWIP   = 1
UCGUI  = 0
UCFS   = 1
MMC    = 1
JPEG   = 1
TOUCH  = 1
# ------------------------------

CC	:= mipsel-linux-gcc
AR	:= mipsel-linux-ar rcsv
LD	:= mipsel-linux-ld
OBJCOPY	:= mipsel-linux-objcopy
NM	:= mipsel-linux-nm
OBJDUMP	:= mipsel-linux-objdump

CFLAGS	:= -mips32 -O2 -mno-abicalls -fno-pic -fno-builtin \
	   -fno-exceptions -ffunction-sections -finit-priority \
	   -fomit-frame-pointer -msoft-float -G 0

LIBS	:= -lstdc++ -lc -lm -lgcc

TOP	:= ..

OSDIR	:= $(TOP)/src
ARCHDIR	:= $(TOP)/mips
SOCDIR	:= $(TOP)/jz4730
MMCDIR	:= $(TOP)/mmc
UCFSDIR := $(TOP)/ucfs
LWIPDIR := $(TOP)/lwip/src
UCGUIDIR := $(TOP)/ucgui
LIBDIR	:= 

SOURCES	:= $(wildcard $(SOCDIR)/*.c) \
	   $(wildcard $(ARCHDIR)/*.c) \
	   $(OSDIR)/ucos_ii.c

SOURCES	+= $(wildcard $(ARCHDIR)/*.S)

ifeq ($(MMC),1)
SOURCES	+= $(MMCDIR)/mmc.c
endif

ifeq ($(UCFS),1)
SOURCES	+= $(UCFSDIR)/ucfs.c
#SOURCES	+= $(wildcard $(UCFSDIR)/*.c)
endif

ifeq ($(LWIP),1)
SOURCES	+= $(LWIPDIR)/lwip.c 
endif

ifeq ($(UCGUI),1)
SOURCES	+= $(wildcard $(UCGUIDIR)/Core/*.c) $(wildcard $(UCGUIDIR)/LCDDriver/*.c) $(wildcard $(UCGUIDIR)/Font/*.c) \
	   $(wildcard $(UCGUIDIR)/ConvertColor/*.c)
SOURCES	+= $(wildcard $(UCGUIDIR)/WM/*.c) $(wildcard $(UCGUIDIR)/Widget/*.c) 
endif

ifeq ($(CAMERA),1)
SOURCES	+= $(wildcard $(SOCDIR)/camera/*.c) $(SOCDIR)/drv/i2c.c 
endif

ifeq ($(JPEG),1)
SOURCES	+= $(wildcard $(UCGUIDIR)/JPEG/*.c)
endif

ifeq ($(TOUCH),1)
SOURCES	+= $(wildcard $(SOCDIR)/drv/touch/touch.c)
endif 

HEADS	:= $(SOCDIR)/head.S $(SOCDIR)/sleep.S

OBJS	:= $(addsuffix .o , $(basename $(notdir $(SOURCES))))
HEADO	:= $(addsuffix .o , $(basename $(notdir $(HEADS))))


CFLAGS	+= -I$(OSDIR) -I$(ARCHDIR) -I$(SOCDIR)/include -I$(MMCDIR) 

ifeq ($(UCFS),1)
CFLAGS	+= -I$(UCFSDIR)  
endif

ifeq ($(UCGUI),1)
CFLAGS	+= -I$(UCGUIDIR)/Font  -I$(UCGUIDIR)/LCDDriver -I$(UCGUIDIR)/Config -I$(UCGUIDIR)/Core \
           -I$(UCGUIDIR)/ConvertColor
CFLAGS	+= -I$(UCGUIDIR)/WM -I$(UCGUIDIR)/Widget           
endif

ifeq ($(LWIP),1)
CFLAGS	+= -I$(LWIPDIR)  -I$(LWIPDIR)/include -I$(LWIPDIR)/include/lwip 
endif

ifeq ($(CAMERA),1)
CFLAGS	+= -I$(SOCDIR)/camera 
CFLAGS	+= -DUSE_CAMERA
endif

ifeq ($(JPEG),1)
CFLAGS	+= -I$(UCGUIDIR)/JPEG
ifeq ($(UCGUI),0)
CFLAGS	+= -I$(UCGUIDIR)/Core  -I$(UCGUIDIR)/Config
endif
endif

ifeq ($(TOUCH),1)
CFLAGS	+= -I$(SOCDIR)/drv/touch
CFLAGS	+= -DTOUCH
endif 


APP	:= ucos.elf
VPATH	:= $(ARCHDIR) $(SOCDIR) $(OSDIR) $(MMCDIR) $(UCFSDIR) $(LWIPDIR) $(FSDIR) $(UCGUIDIR) $(UCGUIDIR)/Core $(UCGUIDIR)/LCDDriver $(UCGUIDIR)/Font $(UCGUIDIR)/Config $(UCGUIDIR)/ConvertColor  $(UCGUIDIR)/JPEG \
           $(TOP)/ucfs  $(SOCDIR)/camera $(SOCDIR)/drv 
VPATH  +=  $(UCGUIDIR)/WM  $(UCGUIDIR)/Widget $(SOCDIR)/drv/touch  

all:	$(APP)
	$(OBJCOPY) -O binary $(APP) ucos.bin
	$(OBJDUMP) -d $(APP) > ucos.dump
	$(NM) $(APP) | sort > ucos.sym
	$(OBJDUMP) -h $(APP) > ucos.map

$(APP):	$(HEADO) $(OBJS) $(EXTLIBS) link.xn makefile
	$(CC) -nostdlib -T link.xn -o $@ $(HEADO) $(OBJS) $(EXTLIBS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<
.cpp.o:
	$(CC) $(CFLAGS) -fno-rtti -fvtable-gc -o $@ -c $<
.S.o:
	$(CC) $(CFLAGS) -D_ASSEMBLER_ -D__ASSEMBLY__ -o $@ -c $<

clean:
	rm -fr *.o $(APP) $(OBJS) core $(OTHER) *.sym *.map *.dump *.bin *.lib

