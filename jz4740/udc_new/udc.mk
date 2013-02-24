
NAND_UDC_DISK=1
RAM_UDC_DISK = 0
SD_UDC_DISK = 1 

SOURCES	+= $(wildcard $(UDCDIR)/udc_bus/*.c)
SOURCES	+= $(wildcard $(UDCDIR)/mass_storage/*.c)
SOURCES	+= $(wildcard $(UDCDIR)/udc_dma/*.c)
SOURCES	+= $(wildcard $(UDCDIR)/block/ramdisk/*.c)
SOURCES	+= $(wildcard $(UDCDIR)/block/nanddisk/*.c)
SOURCES	+= $(wildcard $(UDCDIR)/block/sddisk/*.c)

CFLAGS	+= -DUDC=$(UDC)
CFLAGS	+= -DNAND_UDC_DISK=$(NAND_UDC_DISK)
CFLAGS	+= -DRAM_UDC_DISK=$(RAM_UDC_DISK)
CFLAGS	+= -DSD_UDC_DISK=$(SD_UDC_DISK)

CFLAGS	+= -I$(UDCDIR)/udc_bus
CFLAGS	+= -I$(UDCDIR)/udc_dma
CFLAGS	+= -I$(UDCDIR)/mass_storage
CFLAGS	+= -I$(UDCDIR)/block/ramdisk
CFLAGS	+= -I$(UDCDIR)/block/nanddisk
CFLAGS	+= -I$(UDCDIR)/block/sddisk

VPATH   += $(UDCDIR)/udc_bus
VPATH   += $(UDCDIR)/udc_dma
VPATH   += $(UDCDIR)/mass_storage
VPATH   += $(UDCDIR)/block/ramdisk
VPATH   += $(UDCDIR)/block/nanddisk
VPATH   += $(UDCDIR)/block/sddisk
