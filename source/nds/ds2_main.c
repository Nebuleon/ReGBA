/* ds2_main.c
 *
 * Copyright (C) 2010 dking <dking024@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licens e as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdio.h>
#include "console.h"
#include "fs_api.h"
#include "ds2io.h"
#include "ds2_cpu.h"
#include "ds2_timer.h"
#include "ds2_malloc.h"
#include "gui.h"

#define BLACK_COLOR		RGB15(0, 0, 0)
#define WHITE_COLOR		RGB15(31, 31, 31)

extern int gpsp_main (int argc, char **argv);

#if 0
void ddump_mem(unsigned char* addr, unsigned int len)
{
	unsigned int i;

	for(i= 0; i < len; i++)
	{
		if(i%16 == 0) cprintf("\n%08x: ", i);
		cprintf("%02x ", addr[i]);
	}
}
#endif

void ds2_main(void)
{
	int err;
	HighFrequencyCPU();
	//Initial video and audio and other input and output
	err = ds2io_initb(1024 /* buffer size in samples, matches sound.c's AUDIO_LEN */, 44100 /* sample rate */, 0, 0);
	if(err) goto _failure;

	//Initial file system
	err = fat_init();
	if(err) goto _failure;

	//go to user main funtion
	gpsp_main (0, 0);

_failure:
	ds2_plug_exit();
}

