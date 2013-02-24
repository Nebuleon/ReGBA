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

# select which module to compile, set value to 1 for the module to compile, remain blank for the module not to compile
# -----------------------------
JZ4740_PAV = 0
JZ4740_VIRGO = 1

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

TOP	:= ../..

OSDIR	:= $(TOP)/src
ARCHDIR	:= $(TOP)/mips
SOCDIR	:= $(TOP)/jz4740
NANDBOOT:= $(TOP)/jz4740/nandboot

LIBDIR	:= 

SOURCES	:= $(wildcard $(NANDBOOT)/*.c)

#SOURCES	+= $(wildcard $(ARCHDIR)/*.S)

HEADS	+= $(NANDBOOT)/start.S 

CFLAGS  += -DJZ4740_PAV=$(JZ4740_PAV)
CFLAGS  += -DJZ4740_VIRGO=$(JZ4740_VIRGO)

CFLAGS	+= -I$(SOCDIR)/include -I$(ARCHDIR) -I$(NANDBOOT)


VPATH	:= $(ARCHDIR) $(NANDBOOT)


OBJS	:= $(addsuffix .o , $(basename $(notdir $(SOURCES))))
HEADO	:= $(addsuffix .o , $(basename $(notdir $(HEADS))))


APP	:= nand.elf


all:	$(APP)
	$(OBJCOPY) -O binary $(APP) nand.bin
	$(OBJDUMP) -d $(APP) > nand.dump
	$(NM) $(APP) | sort > nand.sym
	$(OBJDUMP) -h $(APP) > nand.map

$(APP):	$(HEADO) $(OBJS) $(EXTLIBS) u-boot-nand.lds makefile
	$(CC) -nostdlib -T u-boot-nand.lds -o $@ $(HEADO) $(OBJS) $(EXTLIBS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<
.cpp.o:
	$(CC) $(CFLAGS) -fno-rtti -fvtable-gc -o $@ -c $<
.S.o:
	$(CC) $(CFLAGS) -D_ASSEMBLER_ -D__ASSEMBLY__ -o $@ -c $<

clean:
	rm -fr *.o $(APP) $(OBJS) core $(OTHER) *.sym *.map *.dump *.bin *.lib

