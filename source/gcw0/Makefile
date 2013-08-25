TARGET := gpsp

CROSS_COMPILE ?= mipsel-linux-
CC := $(CROSS_COMPILE)gcc

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

include ../Makefile.rules

.PHONY: all
all: $(TARGET)
