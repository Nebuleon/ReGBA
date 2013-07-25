#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <iorpg.h>

#include "cache.h"
#include "file_allocation_table.h"
#include "bit_ops.h"
#include "filetime.h"
#include "fatfile.h"

// origin fread/fwrite do not take advantage of continuous read/write function of SD/NAND, 
// it looks like the third parameter to _FAT_read_r()/_FAT_write_r() is a fixed '2'
// so they are much slower than these two....

// due to behavior of _FAT_read_r()/_FAT_write_r(), 
// FAT should have a at least 8192 cluster size to get better performance, 
// 16K or larger cluster size can get the best performance.



int freadex( void * buffer, int _size, int _n, FILE * f )
{
	if( 0 == _n )
		return 0;
	dbg_printf("freadx %d\n", _n );
	struct _reent r;
	int ret = _FAT_read_r( &r, (int)f->_file + 8, buffer, _size * _n );
	errno = r._errno;
	return ret;
}


int fwriteex( const void * buffer, int _size, int _n, FILE * f )
{
	if( 0 == _n )
		return 0;
	dbg_printf("fwritex new %d\n", _n );
	struct _reent r;
	int ret = _FAT_write_r( &r, (int)f->_file + 8, buffer, _size * _n );
	errno = r._errno;
	return ret;
}

