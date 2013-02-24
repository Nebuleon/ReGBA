#MMCTYPE: 1 is PAVO
#         2 is LYA
MMCTYPE  := 1
SOURCES	+= $(MMCDIR)/mmc.c
SOURCES	+= $(MMCDIR)/mmc_jz4740.c
SOURCES	+= $(MMCDIR)/lb_mmc.c

ifeq ($(USE_MIDWARE),1)
SOURCES	+= $(MMCDIR)/mmc_hotplug.c
endif

CFLAGS	+= -I$(MMCDIR) -DMMCTYPE=$(MMCTYPE)
VPATH   += $(MMCDIR)
