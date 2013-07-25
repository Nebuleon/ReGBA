#include <stdarg.h>
#include <errno.h>
#include "ds2_fcntl.h"
#include "fs_api.h"
#include "ds2_malloc.h"


/*typedef struct {
	FILE_STRUCT* stream;
	int   flags;
} _fd_t;

_fd_t* _fd_list  = NULL;
int    _fd_count = 0;*/

int open(const char* path, int oflag, ... ) {
	if(oflag & _O_UNSUPPORTED) {
		errno = EINVAL;
		return -1;
	}
	// TODO - Deal more correctly with certain flags.
	FILE_STRUCT* tempFile = fat_fopen(path, "rb");
	if(oflag & O_CREAT) {
		if(tempFile == NULL)
			tempFile = fat_fopen(path, "wb");
		if(tempFile == NULL)
			return -1;
	} else if(tempFile == NULL) {
		return -1;
	} else if(oflag & O_TRUNC) {
		tempFile = fat_fopen(path, "wb");
		if(tempFile == NULL)
			return -1;
	}
	fat_fclose(tempFile);

	char tempMode[16];
	if((oflag & 0x3) == O_RDONLY) {
		sprintf(tempMode, "rb");
	} else if((oflag & 0x3) == O_WRONLY) {
		if(oflag & O_APPEND)
			sprintf(tempMode, "ab");
		else
			sprintf(tempMode, "wb");
	} else if((oflag & 0x3) == O_RDWR) {
		if(oflag & O_APPEND)
			sprintf(tempMode, "ab+");
		else
			sprintf(tempMode, "rb+");
	}

	tempFile = fat_fopen(path, tempMode);
	if(tempFile == NULL)
		return -1;
		
	return tempFile -> fd;
}

int fcntl(int fildes, int cmd, ...) {
	/*if((fildes < 0) || (fildes >= _fd_count) || (_fd_list[fildes].stream == NULL)) {
		errno = EINVAL;
		return -1;
	}
	if((cmd <= 0) || (cmd > _F_LAST)) {
		errno = EINVAL;
		return -1;
	}

	va_list ap;

	int   arg;
	void* flock;
	switch(cmd) {
		case F_SETFD:
		case F_SETOWN:
			va_start(ap, cmd);
			arg = va_arg(ap, int);
			va_end(ap);
			break;
		case F_GETLK:
		case F_SETLK:
			va_start(ap, cmd);
			flock = va_arg(ap, void*);
			va_end(ap);
			break;
	}

	switch(cmd) {
		case F_DUPFD: // Duplicate file descriptors not supported.
			errno = EINVAL;
			return -1;
		case F_GETFD:
			return (_fd_list[fildes].flags & _F_FILE_DESC);
		case F_SETFD:
			arg &= _F_FILE_DESC;
			_fd_list[fildes].flags &= ~_F_FILE_DESC;
			_fd_list[fildes].flags |= arg;
			return 0;
		case F_GETFL:
			return (_fd_list[fildes].flags & (O_ACCMODE | _O_FILE_STATUS));
		case F_SETFL:
			arg &= (O_ACCMODE | _O_FILE_STATUS);
			if(arg & _O_UNSUPPORTED) {
				errno = EINVAL;
				return -1;
			}
			if((arg & O_ACCMODE) != (_fd_list[fildes].flags & O_ACCMODE)) {
				errno = EINVAL;
				return -1;
			}
			_fd_list[fildes].flags &= ~(O_ACCMODE | _O_FILE_STATUS);
			_fd_list[fildes].flags |= arg;
			return 0;
		case F_GETOWN:
		case F_SETOWN:
			errno = -1;
			return -1;
		case F_GETLK:
		case F_SETLK:
		case F_SETLKW:
			return -1;
	}

	errno = EINVAL;*/
	return -1;
}
