
SOURCES	+= $(LWIPDIR)/lwip.c 
CFLAGS	+= -I$(LWIPDIR)  -I$(LWIPDIR)/include -I$(LWIPDIR)/include/lwip
VPATH   += $(LWIPDIR)
