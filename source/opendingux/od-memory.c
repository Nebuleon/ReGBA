/* Per-platform code - ReGBA on GCW Zero
 *
 * Copyright (C) 2013 Dingoonity user Nebuleon
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

#include "common.h"
#include <sys/mman.h>

static FILE_TAG_TYPE MappedFile = FILE_TAG_INVALID;
static size_t MappedFileSize;

uint8_t* ReGBA_MapEntireROM(FILE_TAG_TYPE File, size_t Size)
{
	uint8_t* Result = mmap(NULL /* kernel chooses address */,
		Size,
		PROT_READ | PROT_WRITE,
		MAP_PRIVATE,
		fileno(File),
		0 /* offset into file */);
	if (Result != NULL)
	{
		MappedFile = File;
#if TRACE_MEMORY
		ReGBA_Trace("I: Mapped a ROM to memory via the operating system");
#endif
	}
	return Result;
}

void ReGBA_UnmapEntireROM(void* Mapping)
{
	munmap(Mapping, MappedFileSize);
	FILE_CLOSE(MappedFile);
#if TRACE_MEMORY
	ReGBA_Trace("I: Unmapped the previous ROM from memory");
#endif
	MappedFile = NULL;
	MappedFileSize = 0;
}

uint8_t* ReGBA_AllocateROM(size_t Size)
{
	uint8_t* Result = malloc(Size);
#if TRACE_MEMORY
	if (Result != NULL)
		ReGBA_Trace("I: Allocated space for a %u-byte ROM buffer", Size);
#endif
	return Result;
}

size_t ReGBA_AllocateOnDemandBuffer(void** Buffer)
{
	/* At the end of this function, the ROM buffer will have been sized so
	 * that at least 2 MiB are left for other operations.
	 */
	void* Result = NULL;

	/* Start with trying to get 34 MiB. Let go in 1 MiB increments. */
	size_t Size = 34 * 1024 * 1024;
	Result = malloc(Size);
	while (Result == NULL)
	{
		Size -= 1024 * 1024;
		Result = malloc(Size);
	}

	/* Now free the allocation and allocate it again, minus 2 MiB.
	 * The allocation of 34 MiB above is so that the full 32 MiB of the
	 * largest allowable GBA ROM size can be allocated on supported
	 * platforms. */
	free(Result);
	Size -= 2 * 1024 * 1024;
	Result = malloc(Size);

#if TRACE_MEMORY
	ReGBA_Trace("I: Allocated space for a %u-byte on-demand buffer",
		Size);
#endif

	*Buffer = Result;
	return Size;
}

void ReGBA_DeallocateROM(void* Buffer)
{
	free(Buffer);
#if TRACE_MEMORY
	ReGBA_Trace("I: Deallocated space for the previous buffer");
#endif
}
