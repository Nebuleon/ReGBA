#fs.mk

SRC += $(FS_DIR)/cache.c	\
		$(FS_DIR)/directory.c	\
		$(FS_DIR)/fatdir.c	\
		$(FS_DIR)/fatfile.c	\
		$(FS_DIR)/file_allocation_table.c	\
		$(FS_DIR)/filetime.c	\
		$(FS_DIR)/fs_api.c	\
		$(FS_DIR)/fs_unicode.c	\
		$(FS_DIR)/libfat.c	\
		$(FS_DIR)/partition.c	\
		$(FS_DIR)/fat_misc.c	\
		$(FS_DIR)/disc_io/disc.c	\
		$(FS_DIR)/disc_io/io_ds2_mmcf.c	\
        $(FS_DIR)/ds2_fcntl.c	\
        $(FS_DIR)/ds2_unistd.c

SSRC +=

INC += -I$(FS_DIR) -I$(FS_DIR)/disc_io

CFLAGS += -DNDS

