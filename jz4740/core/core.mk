SOURCES	:= $(wildcard $(SOCDIR)/*.c) \
	   $(wildcard $(ARCHDIR)/*.c) \
	   $(OSDIR)/ucos_ii.c

SOURCES	+= $(wildcard $(ARCHDIR)/*.S)

HEADS	+= $(SOCDIR)/head.S

CFLAGS  += -DJZ4740_PAV=$(JZ4740_PAV) \
           -DDM=$(DM)	

CFLAGS	+= -I$(OSDIR) -I$(SOCDIR)/include -I$(ARCHDIR) -I$(SOCDIR)


VPATH	:= $(ARCHDIR) $(SOCDIR) $(OSDIR)
