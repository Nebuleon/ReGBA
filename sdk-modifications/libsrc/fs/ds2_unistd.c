#include "ds2_unistd.h"
#include "fs_api.h"

int close(int fildes) {
	return _FAT_close_r (&__REENT, fildes);
}

int lseek(int fildes, int offset, int whence) {
	return _FAT_seek_r (&__REENT, fildes, (int)offset, whence);
}

int read(int fildes, void* buf, size_t len) {
	return _FAT_read_r (&__REENT, fildes, (char*)buf, len);
}

int write(int fildes, const void* buf, size_t len) {
	return _FAT_write_r (&__REENT, fildes, (const char*)buf, len);
}
